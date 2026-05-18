#ifndef INC_MOTOR_STATE_MODEL_H_
#define INC_MOTOR_STATE_MODEL_H_
/*--------------------------------------------------------------------------------*/

#include <stdint.h>
#include "motor_scheduler.h"
/*--------------------------------------------------------------------------------*/

#define MOTOR_STATE_MAX_BOARDS   16U
#define MOTOR_STATE_MAX_MOTORS   32U

typedef struct {
  // === soft estimated positions ===
  // این مقدارها توسط Main بعد از command موفق تخمین زده می‌شوند.
  int32_t soft_position[MOTOR_STATE_MAX_MOTORS];

  // === real feedback from Node POS_RESPONSE ===
  // این مقدارها از 0x86 POS_RESPONSE پر می‌شوند.
  int32_t actual_position[MOTOR_STATE_MAX_MOTORS];
  int32_t target_position[MOTOR_STATE_MAX_MOTORS];
  uint8_t moving[MOTOR_STATE_MAX_MOTORS];
  uint8_t motor_fault[MOTOR_STATE_MAX_MOTORS];
  uint32_t last_pos_update_ms[MOTOR_STATE_MAX_MOTORS];

  // === board-level state ===
  uint8_t busy;
  uint8_t fault;
  uint8_t last_fault_motor;

  // === last status feedback from Node ===
  uint8_t last_status_type;
  uint8_t last_cmd;
  uint8_t last_result;
  uint8_t last_fault_code;
  uint8_t last_seq;
  uint32_t last_status_ms;

  uint32_t command_count;
  uint32_t fault_count;
} MotorBoardState_t;
/*--------------------------------------------------------------------------------*/

void MotorStateModel_Init(void);

void MotorStateModel_SetBusy(uint8_t board_id, uint8_t busy);
void MotorStateModel_RecordFault(uint8_t board_id, uint8_t motor_idx);

void MotorStateModel_ApplySingleMove(uint8_t board_id, uint8_t motor_idx, int16_t delta);
void MotorStateModel_ApplyMaskMove(uint8_t board_id, uint32_t mask, int16_t delta);
void MotorStateModel_ApplyHomeOne(uint8_t board_id, uint8_t motor_idx);
void MotorStateModel_ApplyHomeAll(uint8_t board_id);
void MotorStateModel_ApplyVectorMove(uint8_t board_id,
                                     const MotorVectorItem_t *items,
                                     uint8_t count);

const MotorBoardState_t *MotorStateModel_GetBoard(uint8_t board_id);
/*--------------------------------------------------------------------------------*/
// === apply feedback from Node status frames ===
void MotorStateModel_UpdateAck(uint8_t board_id,
                               uint8_t cmd,
                               uint8_t result,
                               uint8_t busy,
                               uint8_t fault,
                               uint8_t seq,
                               uint32_t now_ms);

void MotorStateModel_UpdateDone(uint8_t board_id,
                                uint8_t cmd,
                                uint8_t result,
                                uint8_t busy,
                                uint8_t fault,
                                uint8_t seq,
                                uint32_t now_ms);

void MotorStateModel_UpdateRuntimeFault(uint8_t board_id,
                                        uint8_t cmd,
                                        uint8_t fault_code,
                                        uint8_t motor_idx,
                                        uint8_t busy,
                                        uint8_t seq,
                                        uint32_t now_ms);

void MotorStateModel_UpdatePosResponse(uint8_t board_id,
                                       uint8_t motor_idx,
                                       int16_t position,
                                       int16_t target,
                                       uint8_t moving,
                                       uint8_t fault,
                                       uint32_t now_ms);
/*--------------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------------*/

#endif
