#ifndef INC_NODE_STATE_H_
#define INC_NODE_STATE_H_

#include <stdint.h>
#include "main.h"

// === assembly state ===
// وضعیت دریافت chunkهای CAN برای هر نود
extern uint8_t  chunk_mask[NODES];

// زمان شروع assemble برای هر نود
extern uint32_t asm_start_ms[NODES];

// تعداد سیکل‌های کامل دریافت‌شده برای هر نود
extern uint32_t cycles_ok[NODES];


// === node timing ===
// آخرین زمان دریافت هر فریم از نود
extern uint32_t node_last_frame_ms[NODES];

// آخرین زمان کامل شدن 4 chunk برای نود
extern uint32_t node_last_complete_ms[NODES];


// === node state ===
// وضعیت نود از دید Main:
// ONLINE / STALE / OFFLINE
extern uint8_t  node_state[NODES];


// === global CAN timing ===
// آخرین زمان دریافت هر فریم CAN
extern uint32_t last_rx_ms;

// وضعیت کلی CAN (OK / FAIL)
extern uint8_t  can_ok;

#endif /* INC_NODE_STATE_H_ */
