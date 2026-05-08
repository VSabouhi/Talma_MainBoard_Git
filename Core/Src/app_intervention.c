#include "app_intervention.h"
#include "therapy_engine.h"
#include <stdio.h>

// g_therapy داخل freertos.c تعریف شده است.
// اینجا فقط extern می‌کنیم تا SerialLink مستقیم وابسته به freertos.c نشود.
extern TherapyEngineState_t g_therapy;

uint8_t AppIntervention_Approve(uint32_t plan_id)
{
  printf("APP INTERVENTION: approve request id=%lu\r\n",
         (unsigned long)plan_id);

  if (TherapyEngine_HasPendingPlanId(&g_therapy, plan_id) == 0U)
  {
    printf("APP INTERVENTION: invalid approve id=%lu\r\n",
           (unsigned long)plan_id);
    return 0U;
  }

  if (TherapyEngine_ApproveAndExecutePendingPlan(&g_therapy) == 0U)
  {
    printf("APP INTERVENTION: execute FAIL id=%lu\r\n",
           (unsigned long)plan_id);
    return 0U;
  }

  printf("APP INTERVENTION: executed id=%lu\r\n",
         (unsigned long)plan_id);

  return 1U;
}

uint8_t AppIntervention_Reject(uint32_t plan_id)
{
  printf("APP INTERVENTION: reject request id=%lu\r\n",
         (unsigned long)plan_id);

  if (TherapyEngine_HasPendingPlanId(&g_therapy, plan_id) == 0U)
  {
    printf("APP INTERVENTION: invalid reject id=%lu\r\n",
           (unsigned long)plan_id);
    return 0U;
  }

  TherapyEngine_RejectPendingPlan(&g_therapy);

  printf("APP INTERVENTION: rejected id=%lu\r\n",
         (unsigned long)plan_id);

  return 1U;
}
