#include "motor_can.h"
#include "can_rtos_tx.h"

// === MOTOR CAN LAYER ===
// این فایل فقط CAN frame می‌سازد.
// هیچ delay یا queue سطح موتور اینجا نباید اضافه شود.

static uint16_t MotorCan_GetStdId(uint8_t board_id)
{
  return (uint16_t)(MOTOR_CAN_BASE_ID + board_id);
}

static void MotorCan_PutI16LE(uint8_t *lo, uint8_t *hi, int16_t v)
{
  *lo = (uint8_t)((uint16_t)v & 0xFFU);
  *hi = (uint8_t)(((uint16_t)v >> 8) & 0xFFU);
}

BaseType_t MotorCan_SendSingleMove(uint8_t board_id, uint8_t motor_idx, int16_t delta)
{
  uint8_t d[8] = {0};

  // DATA = 03 idx delta_L delta_H 00 00 00 00
  d[0] = MOTOR_CMD_SINGLE_MOVE;
  d[1] = motor_idx;
  MotorCan_PutI16LE(&d[2], &d[3], delta);

  return CanRtosTx_SendStd_Async(MotorCan_GetStdId(board_id), d, MOTOR_CAN_DLC);
}

BaseType_t MotorCan_SendMaskMove(uint8_t board_id, uint32_t mask, int16_t delta)
{
  uint8_t d[8] = {0};

  // DATA = 04 mask0 mask1 mask2 mask3 delta_L delta_H 00
  d[0] = MOTOR_CMD_MASK_MOVE;
  d[1] = (uint8_t)(mask & 0xFFU);
  d[2] = (uint8_t)((mask >> 8) & 0xFFU);
  d[3] = (uint8_t)((mask >> 16) & 0xFFU);
  d[4] = (uint8_t)((mask >> 24) & 0xFFU);
  MotorCan_PutI16LE(&d[5], &d[6], delta);

  return CanRtosTx_SendStd_Async(MotorCan_GetStdId(board_id), d, MOTOR_CAN_DLC);
}

BaseType_t MotorCan_SendHomeOne(uint8_t board_id, uint8_t motor_idx)
{
  uint8_t d[8] = {0};

  // DATA = 06 idx 00 00 00 00 00 00
  d[0] = MOTOR_CMD_HOME_ONE;
  d[1] = motor_idx;

  return CanRtosTx_SendStd_Async(MotorCan_GetStdId(board_id), d, MOTOR_CAN_DLC);
}

BaseType_t MotorCan_SendHomeAll(uint8_t board_id)
{
  uint8_t d[8] = {0};

  // DATA = 07 00 00 00 00 00 00 00
  d[0] = MOTOR_CMD_HOME_ALL;

  return CanRtosTx_SendStd_Async(MotorCan_GetStdId(board_id), d, MOTOR_CAN_DLC);
}

BaseType_t MotorCan_SendVectorBegin(uint8_t board_id, uint8_t count)
{
  uint8_t d[8] = {0};

  // DATA = 08 count 00 00 00 00 00 00
  d[0] = MOTOR_CMD_VECTOR_BEGIN;
  d[1] = count;

  return CanRtosTx_SendStd_Async(MotorCan_GetStdId(board_id), d, MOTOR_CAN_DLC);
}

BaseType_t MotorCan_SendVectorItem2(uint8_t board_id,
                                    uint8_t idx_a, int16_t delta_a,
                                    uint8_t idx_b, int16_t delta_b)
{
  uint8_t d[8] = {0};

  // DATA = 09 idxA deltaA_L deltaA_H idxB deltaB_L deltaB_H 00
  d[0] = MOTOR_CMD_VECTOR_ITEM;
  d[1] = idx_a;
  MotorCan_PutI16LE(&d[2], &d[3], delta_a);
  d[4] = idx_b;
  MotorCan_PutI16LE(&d[5], &d[6], delta_b);

  return CanRtosTx_SendStd_Async(MotorCan_GetStdId(board_id), d, MOTOR_CAN_DLC);
}

BaseType_t MotorCan_SendVectorCommit(uint8_t board_id)
{
  uint8_t d[8] = {0};

  // DATA = 0A 00 00 00 00 00 00 00
  d[0] = MOTOR_CMD_VECTOR_COMMIT;

  return CanRtosTx_SendStd_Async(MotorCan_GetStdId(board_id), d, MOTOR_CAN_DLC);
}
