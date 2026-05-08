#ifndef INC_APP_INTERVENTION_H_
#define INC_APP_INTERVENTION_H_

#include <stdint.h>

// === UI approval bridge ===
// SerialLink نباید مستقیم به g_therapy دسترسی داشته باشد.
// approval/reject از این bridge وارد app layer می‌شود.

uint8_t AppIntervention_Approve(uint32_t plan_id);
uint8_t AppIntervention_Reject(uint32_t plan_id);

#endif
