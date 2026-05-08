#include "motor_state_model.h"
#include <string.h>

// === MOTOR STATE MODEL ===
// چون فعلاً ACK/position feedback نداریم، این state فقط soft estimate است.
// بعداً اگر Node ACK بدهد، اینجا با feedback واقعی sync می‌شود.

static MotorBoardState_t g_motor_state[MOTOR_STATE_MAX_BOARDS];

static uint8_t MotorStateModel_IsBoardValid(uint8_t board_id)
{
  return (board_id < MOTOR_STATE_MAX_BOARDS) ? 1U : 0U;
}

static uint8_t MotorStateModel_IsMotorValid(uint8_t motor_idx)
{
  return (motor_idx < MOTOR_STATE_MAX_MOTORS) ? 1U : 0U;
}

void MotorStateModel_Init(void)
{
  memset(g_motor_state, 0, sizeof(g_motor_state));
}

void MotorStateModel_SetBusy(uint8_t board_id, uint8_t busy)
{
  if (!MotorStateModel_IsBoardValid(board_id))
    return;

  g_motor_state[board_id].busy = busy ? 1U : 0U;
}

void MotorStateModel_RecordFault(uint8_t board_id, uint8_t motor_idx)
{
  if (!MotorStateModel_IsBoardValid(board_id))
    return;

  g_motor_state[board_id].fault = 1U;
  g_motor_state[board_id].last_fault_motor = motor_idx;
  g_motor_state[board_id].fault_count++;
}

void MotorStateModel_ApplySingleMove(uint8_t board_id, uint8_t motor_idx, int16_t delta)
{
  if (!MotorStateModel_IsBoardValid(board_id))
    return;

  if (!MotorStateModel_IsMotorValid(motor_idx))
    return;

  g_motor_state[board_id].soft_position[motor_idx] += delta;
  g_motor_state[board_id].command_count++;
}

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

void MotorStateModel_ApplyHomeOne(uint8_t board_id, uint8_t motor_idx)
{
  if (!MotorStateModel_IsBoardValid(board_id))
    return;

  if (!MotorStateModel_IsMotorValid(motor_idx))
    return;

  g_motor_state[board_id].soft_position[motor_idx] = 0;
  g_motor_state[board_id].command_count++;
}

void MotorStateModel_ApplyHomeAll(uint8_t board_id)
{
  if (!MotorStateModel_IsBoardValid(board_id))
    return;

  for (uint8_t i = 0; i < MOTOR_STATE_MAX_MOTORS; i++)
    g_motor_state[board_id].soft_position[i] = 0;

  g_motor_state[board_id].command_count++;
}

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

const MotorBoardState_t *MotorStateModel_GetBoard(uint8_t board_id)
{
  if (!MotorStateModel_IsBoardValid(board_id))
    return 0;

  return &g_motor_state[board_id];
}
