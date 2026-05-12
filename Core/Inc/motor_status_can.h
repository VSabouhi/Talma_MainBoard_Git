#ifndef INC_MOTOR_STATUS_CAN_H_
#define INC_MOTOR_STATUS_CAN_H_

#include <stdint.h>

// === MOTOR STATUS CAN LAYER ===
// Node -> Main feedback
// Main receives motor feedback on: 0x480 + BOARD_ID

#define MOTOR_STATUS_BASE_ID      0x480U

#define MOTOR_ST_ACK              0x80U
#define MOTOR_ST_DONE             0x81U
#define MOTOR_ST_FAULT            0x82U
#define MOTOR_ST_POS_RESPONSE     0x86U

#define MOTOR_RESULT_OK            0U
#define MOTOR_RESULT_INVALID_CMD   1U
#define MOTOR_RESULT_INVALID_MOTOR 2U
#define MOTOR_RESULT_BUSY          3U
#define MOTOR_RESULT_LIMIT         4U
#define MOTOR_RESULT_QUEUE_FULL    5U
#define MOTOR_RESULT_FAULT         6U

#define MOTOR_FAULT_STALL          1U
#define MOTOR_FAULT_LIMIT_HIT      2U
#define MOTOR_FAULT_DRIVER         3U
#define MOTOR_FAULT_TIMEOUT        4U
#define MOTOR_FAULT_INVALID_STATE  5U

typedef enum
{
  MOTOR_STATUS_TYPE_UNKNOWN = 0,
  MOTOR_STATUS_TYPE_ACK,
  MOTOR_STATUS_TYPE_DONE,
  MOTOR_STATUS_TYPE_FAULT,
  MOTOR_STATUS_TYPE_POS_RESPONSE
} MotorStatusType_t;

typedef struct
{
  uint8_t valid;
  uint8_t board_id;
  uint8_t type;

  uint8_t cmd;
  uint8_t result;
  uint8_t busy;
  uint8_t fault;
  uint8_t seq;

  uint8_t motor_idx;
  uint8_t fault_code;

  int16_t position;
  int16_t target;
  uint8_t moving;
} MotorStatusFrame_t;

// === check whether CAN ID belongs to motor status range ===
uint8_t MotorStatusCan_IsStatusId(uint16_t std_id);

// === decode and handle one motor status CAN frame ===
uint8_t MotorStatusCan_HandleFrame(uint16_t std_id,
                                   const uint8_t data[8],
                                   uint32_t now_ms);

#endif
