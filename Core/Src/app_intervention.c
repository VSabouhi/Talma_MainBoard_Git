#include "app_intervention.h"
#include "therapy_engine.h"
#include <stdio.h>
#include "motor_scheduler.h"
#include "Serial_link.h"
/*----------------------------------------------------------------------------*/

// g_therapy داخل freertos.c تعریف شده است.
// اینجا فقط extern می‌کنیم تا SerialLink مستقیم وابسته به freertos.c نشود.
extern TherapyEngineState_t g_therapy;

typedef struct {
  uint8_t active;
  uint32_t plan_id;
  uint8_t state;
  uint8_t board_id;
  uint8_t motor_count;
} AppInterventionRuntime_t;

static AppInterventionRuntime_t g_app_intervention;
/*----------------------------------------------------------------------------*/

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

  const TherapyPlan_t *p = TherapyEngine_GetPendingPlan(&g_therapy);

  if (p == 0)
    return 0U;

  // === move plan from pending approval to executing lifecycle ===
  // اجرای واقعی موتور فقط بعد از approval انسانی انجام می‌شود.
  if (MotorScheduler_EnqueueInterventionVectorMove(p->plan_id,
                                                   p->board_id,
                                                   p->motors,
                                                   p->motor_count) != pdPASS)
  {
    printf("APP INTERVENTION: enqueue FAIL id=%lu\r\n",
           (unsigned long)plan_id);

    g_app_intervention.active = 1U;
    g_app_intervention.plan_id = plan_id;
    g_app_intervention.state = APP_INTERVENTION_FAILED;

    return 0U;
  }

  g_app_intervention.active = 1U;
  g_app_intervention.plan_id = plan_id;
  g_app_intervention.state = APP_INTERVENTION_EXECUTING;
  g_app_intervention.board_id = p->board_id;
  g_app_intervention.motor_count = p->motor_count;

  TherapyEngine_ClearPendingPlan(&g_therapy);


  // === intervention lifecycle ===
  // plan approve شده و اجرای vector شروع می‌شود.
  g_app_intervention.state = APP_INTERVENTION_EXECUTING;

  printf("APP INTERVENTION: executing id=%lu board=%u motors=%u\r\n",
         (unsigned long)plan_id,
         (unsigned)g_app_intervention.board_id,
         (unsigned)g_app_intervention.motor_count);

  // === notify UI: intervention is executing ===
  // UI باید بداند action بعد از approval وارد اجرای واقعی شده است.
  SerialLink_SendInterventionResult_Async(
      g_app_intervention.plan_id,
      g_app_intervention.state,
      g_app_intervention.board_id,
      g_app_intervention.motor_count);

  return 1U;
}
/*----------------------------------------------------------------------------*/

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

  // === intervention lifecycle ===
  // plan توسط UI/پرستار reject شده است.
  g_app_intervention.active = 1U;
  g_app_intervention.plan_id = plan_id;
  g_app_intervention.state = APP_INTERVENTION_REJECTED;
  g_app_intervention.board_id = 0U;
  g_app_intervention.motor_count = 0U;

  printf("APP INTERVENTION: rejected id=%lu\r\n",
         (unsigned long)plan_id);

  // === notify UI: intervention rejected ===
  SerialLink_SendInterventionResult_Async(
      g_app_intervention.plan_id,
      g_app_intervention.state,
      g_app_intervention.board_id,
      g_app_intervention.motor_count);

  return 1U;
}
/*----------------------------------------------------------------------------*/
void AppIntervention_OnMotorExecutionDone(uint32_t plan_id, uint8_t ok)
{
  if (g_app_intervention.active == 0U)
    return;

  if (g_app_intervention.plan_id != plan_id)
    return;

  if (ok != 0U)
  {
    g_app_intervention.state = APP_INTERVENTION_COMPLETED;

    printf("APP INTERVENTION: completed id=%lu\r\n",
           (unsigned long)plan_id);
  }
  else
  {
    g_app_intervention.state = APP_INTERVENTION_FAILED;

    printf("APP INTERVENTION: failed id=%lu\r\n",
           (unsigned long)plan_id);
  }


  // === notify UI: intervention execution result ===
  // نتیجه نهایی اجرای motor vector به UI ارسال می‌شود.
  SerialLink_SendInterventionResult_Async(
      g_app_intervention.plan_id,
      g_app_intervention.state,
      g_app_intervention.board_id,
      g_app_intervention.motor_count);
}
/*----------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
// === intervention runtime state getter ===
// وضعیت فعلی lifecycle intervention را برمی‌گرداند.
AppInterventionState_t AppIntervention_GetState(void)
{
  return (AppInterventionState_t)g_app_intervention.state;
}
/*----------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
