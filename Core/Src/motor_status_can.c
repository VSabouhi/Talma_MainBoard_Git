#include "motor_status_can.h"
#include "app_config.h"
#include "motor_scheduler.h"   // notify scheduler about ACK/DONE/FAULT
#include <stdio.h>
#include <string.h>
/*--------------------------------------------------------------------------------*/

// === MOTOR STATUS CAN LAYER ===
// این لایه فقط statusهای موتور را از Node decode می‌کند.
// تصمیم‌گیری، retry، recovery و scheduling در این فایل انجام نمی‌شود.

static MotorStatusFrame_t g_last_motor_status[NODES];

static int16_t MotorStatusCan_GetI16LE(uint8_t lo, uint8_t hi)
{
  return (int16_t)((uint16_t)lo | ((uint16_t)hi << 8));
}
/*--------------------------------------------------------------------------------*/

uint8_t MotorStatusCan_IsStatusId(uint16_t std_id)
{
  if (std_id < MOTOR_STATUS_BASE_ID)
    return 0U;

  uint16_t board_id = (uint16_t)(std_id - MOTOR_STATUS_BASE_ID);

  if (board_id >= NODES)
    return 0U;

  return 1U;
}
/*--------------------------------------------------------------------------------*/

static uint8_t MotorStatusCan_Decode(uint16_t std_id,
                                     const uint8_t data[8],
                                     MotorStatusFrame_t *out)
{
  if ((data == 0) || (out == 0))
    return 0U;

  if (MotorStatusCan_IsStatusId(std_id) == 0U)
    return 0U;

  memset(out, 0, sizeof(*out));

  out->valid = 1U;
  out->board_id = (uint8_t)(std_id - MOTOR_STATUS_BASE_ID);

  switch (data[0])
  {
    case MOTOR_ST_ACK:
      // ACK:
      // [80][cmd][result][busy][fault][seq][00][00]
      out->type = MOTOR_STATUS_TYPE_ACK;
      out->cmd = data[1];
      out->result = data[2];
      out->busy = data[3];
      out->fault = data[4];
      out->seq = data[5];
      break;

    case MOTOR_ST_DONE:
      // DONE:
      // [81][cmd][result][busy][fault][seq][00][00]
      out->type = MOTOR_STATUS_TYPE_DONE;
      out->cmd = data[1];
      out->result = data[2];
      out->busy = data[3];
      out->fault = data[4];
      out->seq = data[5];
      break;

    case MOTOR_ST_FAULT:
      // FAULT:
      // [82][cmd][fault_code][motor_idx][busy][seq][00][00]
      out->type = MOTOR_STATUS_TYPE_FAULT;
      out->cmd = data[1];
      out->fault_code = data[2];
      out->motor_idx = data[3];
      out->busy = data[4];
      out->seq = data[5];
      out->fault = 1U;
      out->result = MOTOR_RESULT_FAULT;
      break;

    case MOTOR_ST_POS_RESPONSE:
      // POS_RESPONSE:
      // [86][idx][pos_L][pos_H][target_L][target_H][moving][fault]
      out->type = MOTOR_STATUS_TYPE_POS_RESPONSE;
      out->motor_idx = data[1];
      out->position = MotorStatusCan_GetI16LE(data[2], data[3]);
      out->target = MotorStatusCan_GetI16LE(data[4], data[5]);
      out->moving = data[6];
      out->fault = data[7];
      break;

    default:
      out->valid = 0U;
      out->type = MOTOR_STATUS_TYPE_UNKNOWN;
      return 0U;
  }

  return 1U;
}
/*--------------------------------------------------------------------------------*/

uint8_t MotorStatusCan_HandleFrame(uint16_t std_id,
                                   const uint8_t data[8],
                                   uint32_t now_ms)
{
  (void)now_ms;

  MotorStatusFrame_t st;

  if (MotorStatusCan_Decode(std_id, data, &st) == 0U)
    return 0U;

  if (st.board_id < NODES)
  {
    // === keep last decoded motor status per board ===
    // این فعلاً state ساده برای تست integration است.
    // در فاز بعدی به motor_state_model واقعی وصل می‌شود.
    g_last_motor_status[st.board_id] = st;
  }

  // === notify motor scheduler ===
  // scheduler از این event برای wait کردن روی DONE/FAULT استفاده می‌کند.
  MotorScheduler_OnMotorStatus(st.board_id,
                               st.type,
                               st.cmd,
                               st.result,
                               st.fault_code,
                               st.busy);

  switch (st.type)
  {
    case MOTOR_STATUS_TYPE_ACK:
      printf("MOTOR ST: board=%u ACK cmd=0x%02X result=%u busy=%u fault=%u seq=%u\r\n",
             (unsigned)st.board_id,
             (unsigned)st.cmd,
             (unsigned)st.result,
             (unsigned)st.busy,
             (unsigned)st.fault,
             (unsigned)st.seq);
      break;

    case MOTOR_STATUS_TYPE_DONE:
      printf("MOTOR ST: board=%u DONE cmd=0x%02X result=%u busy=%u fault=%u seq=%u\r\n",
             (unsigned)st.board_id,
             (unsigned)st.cmd,
             (unsigned)st.result,
             (unsigned)st.busy,
             (unsigned)st.fault,
             (unsigned)st.seq);
      break;

    case MOTOR_STATUS_TYPE_FAULT:
      printf("MOTOR ST: board=%u FAULT cmd=0x%02X fault_code=%u motor=%u busy=%u seq=%u\r\n",
             (unsigned)st.board_id,
             (unsigned)st.cmd,
             (unsigned)st.fault_code,
             (unsigned)st.motor_idx,
             (unsigned)st.busy,
             (unsigned)st.seq);
      break;

    case MOTOR_STATUS_TYPE_POS_RESPONSE:
      printf("MOTOR ST: board=%u POS idx=%u pos=%d target=%d moving=%u fault=%u\r\n",
             (unsigned)st.board_id,
             (unsigned)st.motor_idx,
             (int)st.position,
             (int)st.target,
             (unsigned)st.moving,
             (unsigned)st.fault);
      break;

    default:
      break;
  }

  return 1U;
}
/*--------------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------------*/
