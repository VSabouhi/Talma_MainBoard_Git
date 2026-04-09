#ifndef INC_APP_CONFIG_H_
#define INC_APP_CONFIG_H_

#include <stdint.h>

// === system topology ===
#define NODES 16U

// === bed dimensions ===
#define BED_ROWS   32U
#define BED_COLS   16U

// === bed sync config ===
// [TEMP TEST] only node 1 connected
#define BED_REQUIRED_NODE_MASK   (1UL << 1)

// === sensor config ===
#define SENSOR_COUNT_PER_NODE   32U

#endif
