#include "sensor_store.h"

// === raw sensor storage ===
// بایت‌های خام 32 سنسور هر نود
uint8_t sensors32[NODES][32];

// === decoded sensor storage ===
// داده‌های پردازش‌شده سنسورها
uint8_t sensor_value[NODES][SENSOR_COUNT_PER_NODE];
uint8_t sensor_status[NODES][SENSOR_COUNT_PER_NODE];
uint8_t valid_mask[NODES][SENSOR_COUNT_PER_NODE];
uint8_t sensor_confidence[NODES][SENSOR_COUNT_PER_NODE];
