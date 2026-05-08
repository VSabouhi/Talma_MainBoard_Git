#include "motor_scheduler.h"
#include "motor_can.h"
#include "motor_state_model.h"

#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include <string.h>
#include <stdio.h>

// === MOTOR SCHEDULER ===
// این لایه مسئول queue + pacing است.
// چون فعلاً ACK از Node نداریم، فرمان‌ها نباید interleave شوند.
// مخصوصاً vector باید atomic بماند: BEGIN -> ITEM(s) -> COMMIT

#define MOTOR_SCHED_QUEUE_LEN        16U
#define MOTOR_SCHED_FRAME_GAP_MS     20U
#define MOTOR_SCHED_COMMAND_GAP_MS   80U

typedef enum {
  MOTOR_REQ_SINGLE_MOVE = 1,
  MOTOR_REQ_MASK_MOVE,
  MOTOR_REQ_HOME_ONE,
  MOTOR_REQ_HOME_ALL,
  MOTOR_REQ_VECTOR_MOVE
} MotorReqType_t;

typedef struct {
  MotorReqType_t type;
  uint8_t board_id;

  union {
    struct {
      uint8_t idx;
      int16_t delta;
    } single;

    struct {
      uint32_t mask;
      int16_t delta;
    } mask;

    struct {
      uint8_t idx;
    } home_one;

    struct {
      uint8_t count;
      MotorVectorItem_t item[MOTOR_SCHED_MAX_VECTOR_ITEMS];
    } vector;
  } u;
} MotorReq_t;

static QueueHandle_t qMotorReq = NULL;
static StaticQueue_t qMotorReqCtrl;
static uint8_t qMotorReqStorage[MOTOR_SCHED_QUEUE_LEN * sizeof(MotorReq_t)];

static BaseType_t MotorScheduler_Enqueue(const MotorReq_t *req)
{
  if ((qMotorReq == NULL) || (req == NULL))
    return pdFAIL;

  return xQueueSend(qMotorReq, req, 0);
}

void MotorScheduler_Init(void)
{
  qMotorReq = xQueueCreateStatic(MOTOR_SCHED_QUEUE_LEN,
                                 sizeof(MotorReq_t),
                                 qMotorReqStorage,
                                 &qMotorReqCtrl);

  configASSERT(qMotorReq != NULL);

  // === init soft motor state ===
  // state_model بعداً برای therapy_engine منبع وضعیت نرم موتور خواهد بود.
  MotorStateModel_Init();
}

BaseType_t MotorScheduler_EnqueueSingleMove(uint8_t board_id, uint8_t motor_idx, int16_t delta)
{
  MotorReq_t r;
  memset(&r, 0, sizeof(r));

  r.type = MOTOR_REQ_SINGLE_MOVE;
  r.board_id = board_id;
  r.u.single.idx = motor_idx;
  r.u.single.delta = delta;

  return MotorScheduler_Enqueue(&r);
}

BaseType_t MotorScheduler_EnqueueMaskMove(uint8_t board_id, uint32_t mask, int16_t delta)
{
  MotorReq_t r;
  memset(&r, 0, sizeof(r));

  r.type = MOTOR_REQ_MASK_MOVE;
  r.board_id = board_id;
  r.u.mask.mask = mask;
  r.u.mask.delta = delta;

  return MotorScheduler_Enqueue(&r);
}

BaseType_t MotorScheduler_EnqueueHomeOne(uint8_t board_id, uint8_t motor_idx)
{
  MotorReq_t r;
  memset(&r, 0, sizeof(r));

  r.type = MOTOR_REQ_HOME_ONE;
  r.board_id = board_id;
  r.u.home_one.idx = motor_idx;

  return MotorScheduler_Enqueue(&r);
}

BaseType_t MotorScheduler_EnqueueHomeAll(uint8_t board_id)
{
  MotorReq_t r;
  memset(&r, 0, sizeof(r));

  r.type = MOTOR_REQ_HOME_ALL;
  r.board_id = board_id;

  return MotorScheduler_Enqueue(&r);
}

BaseType_t MotorScheduler_EnqueueVectorMove(uint8_t board_id,
                                            const MotorVectorItem_t *items,
                                            uint8_t count)
{
  if ((items == NULL) || (count == 0U) || (count > MOTOR_SCHED_MAX_VECTOR_ITEMS))
    return pdFAIL;

  MotorReq_t r;
  memset(&r, 0, sizeof(r));

  r.type = MOTOR_REQ_VECTOR_MOVE;
  r.board_id = board_id;
  r.u.vector.count = count;

  memcpy(r.u.vector.item, items, count * sizeof(MotorVectorItem_t));

  return MotorScheduler_Enqueue(&r);
}

static void MotorScheduler_PaceFrame(void)
{
  vTaskDelay(pdMS_TO_TICKS(MOTOR_SCHED_FRAME_GAP_MS));
}

static void MotorScheduler_PaceCommand(void)
{
  vTaskDelay(pdMS_TO_TICKS(MOTOR_SCHED_COMMAND_GAP_MS));
}

static void MotorScheduler_HandleVector(const MotorReq_t *r)
{
  uint8_t count = r->u.vector.count;

  // === atomic vector send ===
  // تا پایان COMMIT هیچ فرمان دیگری از queue برداشته نمی‌شود.
  if (MotorCan_SendVectorBegin(r->board_id, count) != pdPASS)
  {
    MotorStateModel_RecordFault(r->board_id, 0xFFU);
    return;
  }

  MotorScheduler_PaceFrame();

  for (uint8_t i = 0; i < count; i += 2U)
  {
    uint8_t idx_a = r->u.vector.item[i].idx;
    int16_t delta_a = r->u.vector.item[i].delta;

    uint8_t idx_b = 0xFFU;
    int16_t delta_b = 0;

    if ((i + 1U) < count)
    {
      idx_b = r->u.vector.item[i + 1U].idx;
      delta_b = r->u.vector.item[i + 1U].delta;
    }

    if (MotorCan_SendVectorItem2(r->board_id, idx_a, delta_a, idx_b, delta_b) != pdPASS)
    {
      MotorStateModel_RecordFault(r->board_id, idx_a);
      return;
    }

    MotorScheduler_PaceFrame();
  }

  if (MotorCan_SendVectorCommit(r->board_id) != pdPASS)
  {
    MotorStateModel_RecordFault(r->board_id, 0xFFU);
    return;
  }

  MotorStateModel_ApplyVectorMove(r->board_id, r->u.vector.item, count);
}

void MotorScheduler_Task(void *argument)
{
  (void)argument;

  MotorReq_t r;

  printf("MotorScheduler_Task started\r\n");

  for (;;)
  {
    if (xQueueReceive(qMotorReq, &r, portMAX_DELAY) != pdPASS)
      continue;

    MotorStateModel_SetBusy(r.board_id, 1U);

    switch (r.type)
    {
      case MOTOR_REQ_SINGLE_MOVE:
        if (MotorCan_SendSingleMove(r.board_id, r.u.single.idx, r.u.single.delta) == pdPASS)
          MotorStateModel_ApplySingleMove(r.board_id, r.u.single.idx, r.u.single.delta);
        else
          MotorStateModel_RecordFault(r.board_id, r.u.single.idx);
        break;

      case MOTOR_REQ_MASK_MOVE:
        if (MotorCan_SendMaskMove(r.board_id, r.u.mask.mask, r.u.mask.delta) == pdPASS)
          MotorStateModel_ApplyMaskMove(r.board_id, r.u.mask.mask, r.u.mask.delta);
        else
          MotorStateModel_RecordFault(r.board_id, 0xFFU);
        break;

      case MOTOR_REQ_HOME_ONE:
        if (MotorCan_SendHomeOne(r.board_id, r.u.home_one.idx) == pdPASS)
          MotorStateModel_ApplyHomeOne(r.board_id, r.u.home_one.idx);
        else
          MotorStateModel_RecordFault(r.board_id, r.u.home_one.idx);
        break;

      case MOTOR_REQ_HOME_ALL:
      {
        // DEBUG:
        // بررسی می‌کنیم آیا home all وارد CAN TX queue می‌شود یا نه.
        BaseType_t ok = MotorCan_SendHomeAll(r.board_id);

        printf("MOTOR SCHED: home all send=%ld board=%u\r\n",
               (long)ok,
               (unsigned)r.board_id);

        if (ok == pdPASS)
          MotorStateModel_ApplyHomeAll(r.board_id);
        else
          MotorStateModel_RecordFault(r.board_id, 0xFFU);

        break;
      }

      case MOTOR_REQ_VECTOR_MOVE:
        // DEBUG:
        // بررسی می‌کنیم scheduler واقعاً به vector رسیده یا نه.
        printf("MOTOR SCHED: vector start board=%u count=%u\r\n",
               (unsigned)r.board_id,
               (unsigned)r.u.vector.count);

        MotorScheduler_HandleVector(&r);

        printf("MOTOR SCHED: vector done board=%u\r\n",
               (unsigned)r.board_id);

        break;

      default:
        break;
    }

    MotorStateModel_SetBusy(r.board_id, 0U);

    // === gap بین commandهای منطقی ===
    // چون ACK نداریم، این delay جلوی پشت‌سرهم رفتن سریع commandها را می‌گیرد.
    MotorScheduler_PaceCommand();
  }
}
