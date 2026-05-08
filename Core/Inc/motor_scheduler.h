#ifndef INC_MOTOR_SCHEDULER_H_
#define INC_MOTOR_SCHEDULER_H_

#include <stdint.h>
#include "FreeRTOS.h"

#define MOTOR_SCHED_MAX_VECTOR_ITEMS  16U

typedef struct {
  uint8_t idx;
  int16_t delta;
} MotorVectorItem_t;

void MotorScheduler_Init(void);
void MotorScheduler_Task(void *argument);

BaseType_t MotorScheduler_EnqueueSingleMove(uint8_t board_id, uint8_t motor_idx, int16_t delta);
BaseType_t MotorScheduler_EnqueueMaskMove(uint8_t board_id, uint32_t mask, int16_t delta);
BaseType_t MotorScheduler_EnqueueHomeOne(uint8_t board_id, uint8_t motor_idx);
BaseType_t MotorScheduler_EnqueueHomeAll(uint8_t board_id);

BaseType_t MotorScheduler_EnqueueVectorMove(uint8_t board_id,
                                            const MotorVectorItem_t *items,
                                            uint8_t count);

#endif
