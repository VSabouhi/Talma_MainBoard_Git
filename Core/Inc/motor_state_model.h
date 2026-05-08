#ifndef INC_MOTOR_STATE_MODEL_H_
#define INC_MOTOR_STATE_MODEL_H_

#include <stdint.h>
#include "motor_scheduler.h"

#define MOTOR_STATE_MAX_BOARDS   16U
#define MOTOR_STATE_MAX_MOTORS   32U

typedef struct {
  int32_t soft_position[MOTOR_STATE_MAX_MOTORS];
  uint8_t busy;
  uint8_t fault;
  uint8_t last_fault_motor;
  uint32_t command_count;
  uint32_t fault_count;
} MotorBoardState_t;

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

#endif
