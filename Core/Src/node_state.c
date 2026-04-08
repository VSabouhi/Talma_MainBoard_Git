#include "node_state.h"

// === assembly state ===
uint8_t  chunk_mask[NODES];
uint32_t asm_start_ms[NODES];
uint32_t cycles_ok[NODES];

// === node timing ===
uint32_t node_last_frame_ms[NODES];
uint32_t node_last_complete_ms[NODES];

// === node state ===
uint8_t  node_state[NODES];

// === global CAN timing ===
uint32_t last_rx_ms = 0;
uint8_t  can_ok = 0;
