#ifndef INC_MOTOR_CAN_H_
#define INC_MOTOR_CAN_H_
/*--------------------------------------------------------------------------------*/

#include <stdint.h>
#include "FreeRTOS.h"
/*--------------------------------------------------------------------------------*/

// === MOTOR CAN LAYER ===
// فقط ساخت payload موتور و ارسال به CAN TX.
// queue/pacing/vector atomic اینجا نیست؛ در motor_scheduler انجام می‌شود.

// NOTE:
// Generic node commands use 0x300 + BOARD_ID.
// Motor commands use 0x400 + BOARD_ID.
#define MOTOR_CAN_BASE_ID   0x400U
#define MOTOR_CAN_DLC       8U

#define MOTOR_CMD_SINGLE_MOVE    0x03U
#define MOTOR_CMD_MASK_MOVE      0x04U
#define MOTOR_CMD_HOME_ONE       0x06U
#define MOTOR_CMD_HOME_ALL       0x07U
#define MOTOR_CMD_VECTOR_BEGIN   0x08U
#define MOTOR_CMD_VECTOR_ITEM    0x09U
#define MOTOR_CMD_VECTOR_COMMIT  0x0AU
// === motor feedback/query commands ===
// POS_QUERY از Main به Node می‌رود و Node با POS_RESPONSE = 0x86 جواب می‌دهد.
#define MOTOR_CMD_POS_QUERY      0x85U
/*--------------------------------------------------------------------------------*/

BaseType_t MotorCan_SendSingleMove(uint8_t board_id, uint8_t motor_idx, int16_t delta);
BaseType_t MotorCan_SendMaskMove(uint8_t board_id, uint32_t mask, int16_t delta);
BaseType_t MotorCan_SendHomeOne(uint8_t board_id, uint8_t motor_idx);
BaseType_t MotorCan_SendHomeAll(uint8_t board_id);

BaseType_t MotorCan_SendVectorBegin(uint8_t board_id, uint8_t count);
BaseType_t MotorCan_SendVectorItem2(uint8_t board_id,
                                    uint8_t idx_a, int16_t delta_a,
                                    uint8_t idx_b, int16_t delta_b);
BaseType_t MotorCan_SendVectorCommit(uint8_t board_id);
// === request current software position of one motor ===
BaseType_t MotorCan_SendPosQuery(uint8_t board_id, uint8_t motor_idx);
/*--------------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------------*/


#endif
