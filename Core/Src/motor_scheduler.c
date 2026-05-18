#include "motor_scheduler.h"
#include "motor_can.h"
#include "motor_state_model.h"
#include "motor_status_can.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include <string.h>
#include <stdio.h>
#include "app_intervention.h"
/*--------------------------------------------------------------------------------*/

// === MOTOR SCHEDULER ===
// این لایه مسئول queue + pacing است.
// چون فعلاً ACK از Node نداریم، فرمان‌ها نباید interleave شوند.
// مخصوصاً vector باید atomic بماند: BEGIN -> ITEM(s) -> COMMIT

#define MOTOR_SCHED_QUEUE_LEN        16U
#define MOTOR_SCHED_FRAME_GAP_MS     20U
#define MOTOR_SCHED_COMMAND_GAP_MS   80U

// === feedback-aware scheduler timeouts ===
// ACK باید سریع برسد.
// DONE وابسته به motion است و طولانی‌تر در نظر گرفته می‌شود.
#define MOTOR_SCHED_ACK_TIMEOUT_MS       300U
#define MOTOR_SCHED_DONE_TIMEOUT_MS      5000U
#define MOTOR_SCHED_VECTOR_DONE_TIMEOUT_MS 8000U

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
  // === optional intervention lifecycle tag ===
  // اگر notify_intervention=1 باشد، بعد از پایان execution نتیجه به app_intervention اعلام می‌شود.
  uint8_t notify_intervention;
  uint32_t plan_id;

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
/*--------------------------------------------------------------------------------*/

static QueueHandle_t qMotorReq = NULL;

// === latest motor feedback event ===
// این state توسط MotorStatusCan_HandleFrame به‌روز می‌شود.
// scheduler task روی notification بیدار می‌شود.
typedef struct
{
  uint8_t valid;
  uint8_t board_id;
  uint8_t status_type;
  uint8_t cmd;
  uint8_t result;
  uint8_t fault_code;
  uint8_t busy;
} MotorSchedFeedback_t;

static TaskHandle_t g_motor_sched_task = NULL;
static volatile MotorSchedFeedback_t g_motor_feedback;

static StaticQueue_t qMotorReqCtrl;
static uint8_t qMotorReqStorage[MOTOR_SCHED_QUEUE_LEN * sizeof(MotorReq_t)];
/*--------------------------------------------------------------------------------*/

static BaseType_t MotorScheduler_Enqueue(const MotorReq_t *req)
{
  if ((qMotorReq == NULL) || (req == NULL))
    return pdFAIL;

  return xQueueSend(qMotorReq, req, 0);
}
/*--------------------------------------------------------------------------------*/

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

/*--------------------------------------------------------------------------------*/


// === receive motor status from CAN RX task ===
// این تابع از context تسک CAN RX صدا زده می‌شود، نه ISR.
void MotorScheduler_OnMotorStatus(uint8_t board_id,
                                  uint8_t status_type,
                                  uint8_t cmd,
                                  uint8_t result,
                                  uint8_t fault_code,
                                  uint8_t busy)
{
  g_motor_feedback.valid = 1U;
  g_motor_feedback.board_id = board_id;
  g_motor_feedback.status_type = status_type;
  g_motor_feedback.cmd = cmd;
  g_motor_feedback.result = result;
  g_motor_feedback.fault_code = fault_code;
  g_motor_feedback.busy = busy;

  if (g_motor_sched_task != NULL)
  {
    xTaskNotifyGive(g_motor_sched_task);
  }
}
/*--------------------------------------------------------------------------------*/

static uint8_t MotorScheduler_FeedbackMatches(uint8_t board_id,
                                              uint8_t status_type,
                                              uint8_t cmd)
{
  if (g_motor_feedback.valid == 0U)
    return 0U;

  if (g_motor_feedback.board_id != board_id)
    return 0U;

  if (g_motor_feedback.status_type != status_type)
    return 0U;

  if (g_motor_feedback.cmd != cmd)
    return 0U;

  return 1U;
}
/*--------------------------------------------------------------------------------*/

static void MotorScheduler_ClearFeedback(void)
{
  // === clear stale feedback ===
  // قبل از ارسال command جدید، feedback قبلی پاک می‌شود
  // تا ACK/DONE قدیمی باعث عبور اشتباه scheduler نشود.
  g_motor_feedback.valid = 0U;
}
/*--------------------------------------------------------------------------------*/

static uint8_t MotorScheduler_WaitAck(uint8_t board_id,
                                      uint8_t cmd,
                                      uint32_t timeout_ms)
{
  TickType_t deadline = xTaskGetTickCount() + pdMS_TO_TICKS(timeout_ms);

  for (;;)
  {
    if (MotorScheduler_FeedbackMatches(board_id, MOTOR_STATUS_TYPE_ACK, cmd) != 0U)
    {
      uint8_t result = g_motor_feedback.result;

      if (result == MOTOR_RESULT_OK)
        return 1U;

      printf("MOTOR SCHED: ACK reject board=%u cmd=0x%02X result=%u\r\n",
             (unsigned)board_id,
             (unsigned)cmd,
             (unsigned)result);

      return 0U;
    }

    TickType_t now = xTaskGetTickCount();

    if ((int32_t)(deadline - now) <= 0)
    {
      printf("MOTOR SCHED: ACK timeout board=%u cmd=0x%02X\r\n",
             (unsigned)board_id,
             (unsigned)cmd);
      return 0U;
    }

    ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(20));
  }
}
/*--------------------------------------------------------------------------------*/

static uint8_t MotorScheduler_WaitDoneOrFault(uint8_t board_id,
                                              uint8_t cmd,
                                              uint32_t timeout_ms)
{
  TickType_t deadline = xTaskGetTickCount() + pdMS_TO_TICKS(timeout_ms);

  for (;;)
  {
    if (MotorScheduler_FeedbackMatches(board_id, MOTOR_STATUS_TYPE_DONE, cmd) != 0U)
    {
      if (g_motor_feedback.result == MOTOR_RESULT_OK)
        return 1U;

      printf("MOTOR SCHED: DONE result fail board=%u cmd=0x%02X result=%u\r\n",
             (unsigned)board_id,
             (unsigned)cmd,
             (unsigned)g_motor_feedback.result);
      return 0U;
    }

    if (MotorScheduler_FeedbackMatches(board_id, MOTOR_STATUS_TYPE_FAULT, cmd) != 0U)
    {
      printf("MOTOR SCHED: FAULT board=%u cmd=0x%02X fault=%u\r\n",
             (unsigned)board_id,
             (unsigned)cmd,
             (unsigned)g_motor_feedback.fault_code);
      return 0U;
    }

    TickType_t now = xTaskGetTickCount();

    if ((int32_t)(deadline - now) <= 0)
    {
      printf("MOTOR SCHED: DONE timeout board=%u cmd=0x%02X\r\n",
             (unsigned)board_id,
             (unsigned)cmd);
      return 0U;
    }

    ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(20));
  }
}
/*--------------------------------------------------------------------------------*/

static uint8_t MotorScheduler_SendAndWaitMotion(uint8_t board_id,
                                                uint8_t cmd,
                                                BaseType_t send_ok,
                                                uint32_t done_timeout_ms)
{
	MotorScheduler_ClearFeedback();

  if (send_ok != pdPASS)
  {
    printf("MOTOR SCHED: CAN enqueue fail board=%u cmd=0x%02X\r\n",
           (unsigned)board_id,
           (unsigned)cmd);
    return 0U;
  }

  if (MotorScheduler_WaitAck(board_id, cmd, MOTOR_SCHED_ACK_TIMEOUT_MS) == 0U)
    return 0U;

  if (MotorScheduler_WaitDoneOrFault(board_id, cmd, done_timeout_ms) == 0U)
    return 0U;

  return 1U;
}
/*--------------------------------------------------------------------------------*/

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
/*--------------------------------------------------------------------------------*/

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
/*--------------------------------------------------------------------------------*/

BaseType_t MotorScheduler_EnqueueHomeOne(uint8_t board_id, uint8_t motor_idx)
{
  MotorReq_t r;
  memset(&r, 0, sizeof(r));

  r.type = MOTOR_REQ_HOME_ONE;
  r.board_id = board_id;
  r.u.home_one.idx = motor_idx;

  return MotorScheduler_Enqueue(&r);
}
/*--------------------------------------------------------------------------------*/

BaseType_t MotorScheduler_EnqueueHomeAll(uint8_t board_id)
{
  MotorReq_t r;
  memset(&r, 0, sizeof(r));

  r.type = MOTOR_REQ_HOME_ALL;
  r.board_id = board_id;

  return MotorScheduler_Enqueue(&r);
}
/*--------------------------------------------------------------------------------*/

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
/*--------------------------------------------------------------------------------*/


BaseType_t MotorScheduler_EnqueueInterventionVectorMove(uint32_t plan_id,
                                                        uint8_t board_id,
                                                        const MotorVectorItem_t *items,
                                                        uint8_t count)
{
  if ((items == NULL) || (count == 0U) || (count > MOTOR_SCHED_MAX_VECTOR_ITEMS))
    return pdFAIL;

  MotorReq_t r;
  memset(&r, 0, sizeof(r));

  r.type = MOTOR_REQ_VECTOR_MOVE;
  r.board_id = board_id;
  r.notify_intervention = 1U;
  r.plan_id = plan_id;

  r.u.vector.count = count;
  memcpy(r.u.vector.item, items, count * sizeof(MotorVectorItem_t));

  return MotorScheduler_Enqueue(&r);
}
/*--------------------------------------------------------------------------------*/


/*--------------------------------------------------------------------------------*/

static void MotorScheduler_PaceCommand(void)
{
  vTaskDelay(pdMS_TO_TICKS(MOTOR_SCHED_COMMAND_GAP_MS));
}
/*--------------------------------------------------------------------------------*/

static uint8_t MotorScheduler_HandleVector(const MotorReq_t *r)
{
  uint8_t count = r->u.vector.count;

  // === atomic vector send with ACK/DONE awareness ===
  // BEGIN و ITEMها فقط ACK می‌خواهند.
  // COMMIT هم ACK می‌خواهد و بعد DONE/FAULT کل vector.

  MotorScheduler_ClearFeedback();

  if (MotorCan_SendVectorBegin(r->board_id, count) != pdPASS)
  {
    MotorStateModel_RecordFault(r->board_id, 0xFFU);
    return 0U;
  }

  if (MotorScheduler_WaitAck(r->board_id,
                             MOTOR_CMD_VECTOR_BEGIN,
                             MOTOR_SCHED_ACK_TIMEOUT_MS) == 0U)
  {
    MotorStateModel_RecordFault(r->board_id, 0xFFU);
    return 0U;
  }

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

    MotorScheduler_ClearFeedback();
    if (MotorCan_SendVectorItem2(r->board_id,
                                 idx_a,
                                 delta_a,
                                 idx_b,
                                 delta_b) != pdPASS)
    {
      MotorStateModel_RecordFault(r->board_id, idx_a);
      return 0U;
    }

    if (MotorScheduler_WaitAck(r->board_id,
                               MOTOR_CMD_VECTOR_ITEM,
                               MOTOR_SCHED_ACK_TIMEOUT_MS) == 0U)
    {
      MotorStateModel_RecordFault(r->board_id, idx_a);
      return 0U;
    }
  }

  MotorScheduler_ClearFeedback();

  if (MotorCan_SendVectorCommit(r->board_id) != pdPASS)
  {
    MotorStateModel_RecordFault(r->board_id, 0xFFU);
    return 0U;
  }

  if (MotorScheduler_WaitAck(r->board_id,
                             MOTOR_CMD_VECTOR_COMMIT,
                             MOTOR_SCHED_ACK_TIMEOUT_MS) == 0U)
  {
    MotorStateModel_RecordFault(r->board_id, 0xFFU);
    return 0U;
  }

  if (MotorScheduler_WaitDoneOrFault(r->board_id,
                                     MOTOR_CMD_VECTOR_COMMIT,
                                     MOTOR_SCHED_VECTOR_DONE_TIMEOUT_MS) == 0U)
  {
    MotorStateModel_RecordFault(r->board_id, 0xFFU);
    return 0U;
  }

  MotorStateModel_ApplyVectorMove(r->board_id, r->u.vector.item, count);
  return 1U;
}
/*--------------------------------------------------------------------------------*/

void MotorScheduler_Task(void *argument)
{
  (void)argument;

  // === save scheduler task handle ===
  // status feedback از CAN RX task با notify این task را بیدار می‌کند.
  g_motor_sched_task = xTaskGetCurrentTaskHandle();

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
      {
        // === feedback-aware single move ===
        // command بعدی فقط بعد از DONE/FAULT پردازش می‌شود.
        uint8_t ok = MotorScheduler_SendAndWaitMotion(
            r.board_id,
            MOTOR_CMD_SINGLE_MOVE,
            MotorCan_SendSingleMove(r.board_id, r.u.single.idx, r.u.single.delta),
            MOTOR_SCHED_DONE_TIMEOUT_MS);

        if (ok != 0U)
          MotorStateModel_ApplySingleMove(r.board_id, r.u.single.idx, r.u.single.delta);
        else
          MotorStateModel_RecordFault(r.board_id, r.u.single.idx);

        break;
      }

      case MOTOR_REQ_MASK_MOVE:
      {
        // === feedback-aware mask move ===
        uint8_t ok = MotorScheduler_SendAndWaitMotion(
            r.board_id,
            MOTOR_CMD_MASK_MOVE,
            MotorCan_SendMaskMove(r.board_id, r.u.mask.mask, r.u.mask.delta),
            MOTOR_SCHED_DONE_TIMEOUT_MS);

        if (ok != 0U)
          MotorStateModel_ApplyMaskMove(r.board_id, r.u.mask.mask, r.u.mask.delta);
        else
          MotorStateModel_RecordFault(r.board_id, 0xFFU);

        break;
      }

      case MOTOR_REQ_HOME_ONE:
      {
        // === feedback-aware home one ===
        uint8_t ok = MotorScheduler_SendAndWaitMotion(
            r.board_id,
            MOTOR_CMD_HOME_ONE,
            MotorCan_SendHomeOne(r.board_id, r.u.home_one.idx),
            MOTOR_SCHED_DONE_TIMEOUT_MS);

        if (ok != 0U)
          MotorStateModel_ApplyHomeOne(r.board_id, r.u.home_one.idx);
        else
          MotorStateModel_RecordFault(r.board_id, r.u.home_one.idx);

        break;
      }

      case MOTOR_REQ_HOME_ALL:
      {
        // === feedback-aware home all ===
        uint8_t ok = MotorScheduler_SendAndWaitMotion(
            r.board_id,
            MOTOR_CMD_HOME_ALL,
            MotorCan_SendHomeAll(r.board_id),
            MOTOR_SCHED_DONE_TIMEOUT_MS);

        printf("MOTOR SCHED: home all complete ok=%u board=%u\r\n",
               (unsigned)ok,
               (unsigned)r.board_id);

        if (ok != 0U)
          MotorStateModel_ApplyHomeAll(r.board_id);
        else
          MotorStateModel_RecordFault(r.board_id, 0xFFU);

        break;
      }

      case MOTOR_REQ_VECTOR_MOVE:
      {
        // === feedback-aware atomic vector move ===
        // اگر این request مربوط به intervention باشد، نتیجه execution به app_intervention برمی‌گردد.
        printf("MOTOR SCHED: vector start board=%u count=%u\r\n",
               (unsigned)r.board_id,
               (unsigned)r.u.vector.count);

        uint8_t ok = MotorScheduler_HandleVector(&r);

        printf("MOTOR SCHED: vector done board=%u ok=%u\r\n",
               (unsigned)r.board_id,
               (unsigned)ok);

        if (r.notify_intervention != 0U)
        {
          AppIntervention_OnMotorExecutionDone(r.plan_id, ok);
        }

        break;
      }

      default:
        break;
    }

    MotorStateModel_SetBusy(r.board_id, 0U);

    // === gap بین commandهای منطقی ===
    // با وجود ACK/DONE هم یک فاصله کوچک برای کاهش burst روی CAN نگه می‌داریم.
    MotorScheduler_PaceCommand();
  }
}
/*--------------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------------*/

