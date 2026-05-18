#include "motor_state_model.h"
#include <string.h>
/*--------------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------------*/

// === MOTOR STATE MODEL ===
// چون فعلاً ACK/position feedback نداریم، این state فقط soft estimate است.
// بعداً اگر Node ACK بدهد، اینجا با feedback واقعی sync می‌شود.

static MotorBoardState_t g_motor_state[MOTOR_STATE_MAX_BOARDS];

static uint8_t MotorStateModel_IsBoardValid(uint8_t board_id)
{
  return (board_id < MOTOR_STATE_MAX_BOARDS) ? 1U : 0U;
}
/*--------------------------------------------------------------------------------*/

static uint8_t MotorStateModel_IsMotorValid(uint8_t motor_idx)
{
  return (motor_idx < MOTOR_STATE_MAX_MOTORS) ? 1U : 0U;
}
/*--------------------------------------------------------------------------------*/

void MotorStateModel_Init(void)
{
  memset(g_motor_state, 0, sizeof(g_motor_state));
}
/*--------------------------------------------------------------------------------*/

void MotorStateModel_SetBusy(uint8_t board_id, uint8_t busy)
{
  if (!MotorStateModel_IsBoardValid(board_id))
    return;

  g_motor_state[board_id].busy = busy ? 1U : 0U;
}
/*--------------------------------------------------------------------------------*/

void MotorStateModel_RecordFault(uint8_t board_id, uint8_t motor_idx)
{
  if (!MotorStateModel_IsBoardValid(board_id))
    return;

  g_motor_state[board_id].fault = 1U;
  g_motor_state[board_id].last_fault_motor = motor_idx;
  g_motor_state[board_id].fault_count++;
}
/*--------------------------------------------------------------------------------*/

void MotorStateModel_ApplySingleMove(uint8_t board_id, uint8_t motor_idx, int16_t delta)
{
  if (!MotorStateModel_IsBoardValid(board_id))
    return;

  if (!MotorStateModel_IsMotorValid(motor_idx))
    return;

  g_motor_state[board_id].soft_position[motor_idx] += delta;
  g_motor_state[board_id].command_count++;
}
/*--------------------------------------------------------------------------------*/

void MotorStateModel_ApplyMaskMove(uint8_t board_id, uint32_t mask, int16_t delta)
{
  if (!MotorStateModel_IsBoardValid(board_id))
    return;

  for (uint8_t i = 0; i < MOTOR_STATE_MAX_MOTORS; i++)
  {
    if ((mask & (1UL << i)) != 0UL)
      g_motor_state[board_id].soft_position[i] += delta;
  }

  g_motor_state[board_id].command_count++;
}
/*--------------------------------------------------------------------------------*/

void MotorStateModel_ApplyHomeOne(uint8_t board_id, uint8_t motor_idx)
{
  if (!MotorStateModel_IsBoardValid(board_id))
    return;

  if (!MotorStateModel_IsMotorValid(motor_idx))
    return;

  g_motor_state[board_id].soft_position[motor_idx] = 0;
  g_motor_state[board_id].command_count++;
}
/*--------------------------------------------------------------------------------*/

void MotorStateModel_ApplyHomeAll(uint8_t board_id)
{
  if (!MotorStateModel_IsBoardValid(board_id))
    return;

  for (uint8_t i = 0; i < MOTOR_STATE_MAX_MOTORS; i++)
    g_motor_state[board_id].soft_position[i] = 0;

  g_motor_state[board_id].command_count++;
}
/*--------------------------------------------------------------------------------*/

void MotorStateModel_ApplyVectorMove(uint8_t board_id,
                                     const MotorVectorItem_t *items,
                                     uint8_t count)
{
  if (!MotorStateModel_IsBoardValid(board_id) || (items == 0))
    return;

  for (uint8_t i = 0; i < count; i++)
  {
    uint8_t idx = items[i].idx;

    if (MotorStateModel_IsMotorValid(idx))
      g_motor_state[board_id].soft_position[idx] += items[i].delta;
  }

  g_motor_state[board_id].command_count++;
}
/*--------------------------------------------------------------------------------*/

const MotorBoardState_t *MotorStateModel_GetBoard(uint8_t board_id)
{
  if (!MotorStateModel_IsBoardValid(board_id))
    return 0;

  return &g_motor_state[board_id];
}
/*--------------------------------------------------------------------------------*/
void MotorStateModel_UpdateAck(uint8_t board_id,
                               uint8_t cmd,
                               uint8_t result,
                               uint8_t busy,
                               uint8_t fault,
                               uint8_t seq,
                               uint32_t now_ms)
{
  if (!MotorStateModel_IsBoardValid(board_id))
    return;

  MotorBoardState_t *b = &g_motor_state[board_id];

  // === ACK means command accepted/rejected by Node ===
  // ACK completion نیست؛ فقط وضعیت پذیرش command است.
  b->busy = busy ? 1U : 0U;
  b->fault = fault ? 1U : b->fault;

  b->last_status_type = 0x80U;
  b->last_cmd = cmd;
  b->last_result = result;
  b->last_seq = seq;
  b->last_status_ms = now_ms;

  if (result != 0U)
  {
    b->fault_count++;
  }
}
/*--------------------------------------------------------------------------------*/

void MotorStateModel_UpdateDone(uint8_t board_id,
                                uint8_t cmd,
                                uint8_t result,
                                uint8_t busy,
                                uint8_t fault,
                                uint8_t seq,
                                uint32_t now_ms)
{
  if (!MotorStateModel_IsBoardValid(board_id))
    return;

  MotorBoardState_t *b = &g_motor_state[board_id];

  // === DONE means command completed successfully/terminally ===
  b->busy = busy ? 1U : 0U;
  b->fault = fault ? 1U : b->fault;

  b->last_status_type = 0x81U;
  b->last_cmd = cmd;
  b->last_result = result;
  b->last_seq = seq;
  b->last_status_ms = now_ms;

  if (result != 0U)
  {
    b->fault_count++;
  }
}
/*--------------------------------------------------------------------------------*/

void MotorStateModel_UpdateRuntimeFault(uint8_t board_id,
                                        uint8_t cmd,
                                        uint8_t fault_code,
                                        uint8_t motor_idx,
                                        uint8_t busy,
                                        uint8_t seq,
                                        uint32_t now_ms)
{
  if (!MotorStateModel_IsBoardValid(board_id))
    return;

  MotorBoardState_t *b = &g_motor_state[board_id];

  // === FAULT is terminal; no DONE expected after this ===
  b->busy = busy ? 1U : 0U;
  b->fault = 1U;
  b->last_fault_motor = motor_idx;
  b->last_fault_code = fault_code;

  b->last_status_type = 0x82U;
  b->last_cmd = cmd;
  b->last_result = 6U; // MOTOR_RESULT_FAULT
  b->last_seq = seq;
  b->last_status_ms = now_ms;

  if (MotorStateModel_IsMotorValid(motor_idx))
    b->motor_fault[motor_idx] = 1U;

  b->fault_count++;
}
/*--------------------------------------------------------------------------------*/

void MotorStateModel_UpdatePosResponse(uint8_t board_id,
                                       uint8_t motor_idx,
                                       int16_t position,
                                       int16_t target,
                                       uint8_t moving,
                                       uint8_t fault,
                                       uint32_t now_ms)
{
  if (!MotorStateModel_IsBoardValid(board_id))
    return;

  if (!MotorStateModel_IsMotorValid(motor_idx))
    return;

  MotorBoardState_t *b = &g_motor_state[board_id];

  // === real Node feedback ===
  // actual_position/target_position از POS_RESPONSE می‌آید.
  b->actual_position[motor_idx] = position;
  b->target_position[motor_idx] = target;
  b->moving[motor_idx] = moving ? 1U : 0U;
  b->motor_fault[motor_idx] = fault ? 1U : 0U;
  b->last_pos_update_ms[motor_idx] = now_ms;

  if (fault != 0U)
  {
    b->fault = 1U;
    b->last_fault_motor = motor_idx;
  }
}
/*--------------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------------*/

