#ifndef INC_SENSOR_STORE_H_
#define INC_SENSOR_STORE_H_

#include <stdint.h>
#include "main.h"

// === raw sensor bytes per node ===
// هر نود 32 بایت خام دارد (بعد از assemble)
extern uint8_t sensors32[NODES][32];

// === decoded sensor data ===
// مقدار واقعی سنسور (0..63)
extern uint8_t sensor_value[NODES][SENSOR_COUNT_PER_NODE];

// وضعیت سنسور (OK / WARNING / ERROR / DISCONNECTED)
extern uint8_t sensor_status[NODES][SENSOR_COUNT_PER_NODE];

// آیا این سنسور قابل استفاده است یا نه
extern uint8_t valid_mask[NODES][SENSOR_COUNT_PER_NODE];

// میزان اعتماد به داده سنسور (0..255)
extern uint8_t sensor_confidence[NODES][SENSOR_COUNT_PER_NODE];

#endif /* INC_SENSOR_STORE_H_ */
