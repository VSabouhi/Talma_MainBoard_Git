#ifndef INC_APP_INTERVENTION_H_
#define INC_APP_INTERVENTION_H_
/*----------------------------------------------------------------------------*/

#include <stdint.h>
/*----------------------------------------------------------------------------*/

// === UI approval bridge ===
// SerialLink نباید مستقیم به g_therapy دسترسی داشته باشد.
// approval/reject از این bridge وارد app layer می‌شود.


typedef enum {
  APP_INTERVENTION_IDLE = 0,
  APP_INTERVENTION_EXECUTING,
  APP_INTERVENTION_COMPLETED,
  APP_INTERVENTION_FAILED,
  APP_INTERVENTION_REJECTED
} AppInterventionState_t;
/*----------------------------------------------------------------------------*/

uint8_t AppIntervention_Approve(uint32_t plan_id);
uint8_t AppIntervention_Reject(uint32_t plan_id);
/*----------------------------------------------------------------------------*/
// === called by motor_scheduler when tagged intervention execution ends ===
void AppIntervention_OnMotorExecutionDone(uint32_t plan_id, uint8_t ok);
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/


#endif
