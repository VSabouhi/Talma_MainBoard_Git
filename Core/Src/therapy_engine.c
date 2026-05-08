#include "therapy_engine.h"
#include <string.h>
#include <stdio.h>
/*--------------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------------*/

// === INTERVENTION PLANNER CONFIG ===
// این لایه فقط پیشنهاد می‌سازد، نه اجرای خودکار.
//
// تصمیم مهم ایمنی:
// منطقه پر فشار را مستقیم بالا نمی‌بریم.
// برای offload، از support نقاط اطراف/کم‌فشار استفاده می‌کنیم.

#define THERAPY_PLAN_COOLDOWN_MS      30000U
#define THERAPY_DEFAULT_BOARD_ID      1U

// DEBUG:
// فقط برای تست ساخت plan.
// بعد از تست باید 0 شود.
#define THERAPY_DEBUG_FORCE_PLAN      0U

#define THERAPY_REASON_HIGH_RISK      1U
#define THERAPY_REASON_URGENT_REC     2U
#define THERAPY_REASON_OFFLOAD_REC    3U

// === temporary mechanical mapping ===
// TODO:
// این mapping باید بعداً بر اساس layout واقعی motor/pixel اصلاح شود.
// فعلاً فقط برای ساخت plan پیشنهادی است، نه clinical final.

// sacrum high pressure:
// support اطراف لگن/ران، نه فشار مستقیم روی sacrum
#define MAP_SACRUM_BOARD_ID           1U
#define MAP_SACRUM_MOTOR_A            0U
#define MAP_SACRUM_MOTOR_B            1U
#define MAP_SACRUM_MOTOR_C            2U
#define MAP_SACRUM_SUPPORT_DELTA      80

// left/right heel high pressure:
// ایده: حمایت از calf/lower-leg اطراف heel، نه بالا بردن مستقیم heel
#define MAP_LEFT_HEEL_BOARD_ID        1U
#define MAP_LEFT_HEEL_MOTOR_A         3U
#define MAP_LEFT_HEEL_MOTOR_B         4U
#define MAP_HEEL_SUPPORT_DELTA        60

#define MAP_RIGHT_HEEL_BOARD_ID       1U
#define MAP_RIGHT_HEEL_MOTOR_A        5U
#define MAP_RIGHT_HEEL_MOTOR_B        6U
/*--------------------------------------------------------------------------------*/

static void TherapyEngine_ClearPlan(TherapyPlan_t *p)
{
  if (p == 0)
    return;

  memset(p, 0, sizeof(*p));
  p->status = THERAPY_PLAN_STATUS_EMPTY;
}

void TherapyEngine_Init(TherapyEngineState_t *st)
{
  if (st == 0)
    return;

  memset(st, 0, sizeof(*st));
  st->enabled = 0U;
  TherapyEngine_ClearPlan(&st->pending_plan);

  // NOTE:
  // planner به صورت پیش‌فرض disabled است.
  // حتی وقتی enabled شود، فقط plan می‌سازد و موتور را خودکار حرکت نمی‌دهد.
}
/*--------------------------------------------------------------------------------*/

void TherapyEngine_SetEnabled(TherapyEngineState_t *st, uint8_t enabled)
{
  if (st == 0)
    return;

  st->enabled = enabled ? 1U : 0U;

  printf("THERAPY PLANNER: enabled=%u\r\n", (unsigned)st->enabled);
}
/*--------------------------------------------------------------------------------*/

const TherapyPlan_t *TherapyEngine_GetPendingPlan(const TherapyEngineState_t *st)
{
  if (st == 0)
    return 0;

  if (st->pending_plan.valid == 0U)
    return 0;

  return &st->pending_plan;
}
/*--------------------------------------------------------------------------------*/

void TherapyEngine_RejectPendingPlan(TherapyEngineState_t *st)
{
  if (st == 0)
    return;

  if (st->pending_plan.valid != 0U)
  {
    st->pending_plan.status = THERAPY_PLAN_STATUS_REJECTED;
    printf("THERAPY PLAN: rejected id=%lu\r\n",
           (unsigned long)st->pending_plan.plan_id);
  }

  TherapyEngine_ClearPlan(&st->pending_plan);
}
/*--------------------------------------------------------------------------------*/

static uint8_t TherapyEngine_HasPendingPlan(const TherapyEngineState_t *st)
{
  if (st == 0)
    return 0U;

  if (st->pending_plan.valid == 0U)
    return 0U;

  if (st->pending_plan.status == THERAPY_PLAN_STATUS_PENDING_APPROVAL)
    return 1U;

  return 0U;
}
/*--------------------------------------------------------------------------------*/

static uint8_t TherapyEngine_SelectTargetZone(const ZoneAnalysisResult_t *zone)
{
  uint8_t best_zone = ZONE_SACRUM;
  uint8_t best_peak = 0U;

  if (zone == 0)
    return ZONE_SACRUM;

  for (uint8_t z = 0U; z < ZONE_COUNT; z++)
  {
    if (zone->zone[z].valid_cells == 0U)
      continue;

    if (zone->zone[z].peak > best_peak)
    {
      best_peak = zone->zone[z].peak;
      best_zone = z;
    }
  }

  return best_zone;
}
/*--------------------------------------------------------------------------------*/

static uint8_t TherapyEngine_ShouldCreatePlan(const RiskResult_t *risk,
                                              const AlertResult_t *alert,
                                              const RecommendationResult_t *rec,
                                              uint8_t *reason_code)
{
  if ((risk == 0) || (alert == 0) || (rec == 0) || (reason_code == 0))
    return 0U;

  if (rec->code == RECOMMEND_URGENT_REPOSITION)
  {
    *reason_code = THERAPY_REASON_URGENT_REC;
    return 1U;
  }

  if (rec->code == RECOMMEND_OFFLOAD_SACRUM)
  {
    *reason_code = THERAPY_REASON_OFFLOAD_REC;
    return 1U;
  }

  if ((alert->active != 0U) && (risk->level >= RISK_LEVEL_HIGH))
  {
    *reason_code = THERAPY_REASON_HIGH_RISK;
    return 1U;
  }

  return 0U;
}
/*--------------------------------------------------------------------------------*/

static void TherapyEngine_BuildSacrumPlan(TherapyPlan_t *p)
{
  p->type = THERAPY_PLAN_OFFLOAD_SACRUM;
  p->target_zone = ZONE_SACRUM;
  p->board_id = MAP_SACRUM_BOARD_ID;

  // NOTE:
  // برای sacrum، خود نقطه پر فشار را مستقیم بالا نمی‌بریم.
  // این motorها فعلاً نماینده support اطراف pelvis/thigh هستند.
  p->motor_count = 3U;
  p->motors[0].idx = MAP_SACRUM_MOTOR_A;
  p->motors[0].delta = MAP_SACRUM_SUPPORT_DELTA;
  p->motors[1].idx = MAP_SACRUM_MOTOR_B;
  p->motors[1].delta = MAP_SACRUM_SUPPORT_DELTA;
  p->motors[2].idx = MAP_SACRUM_MOTOR_C;
  p->motors[2].delta = MAP_SACRUM_SUPPORT_DELTA;
}
/*--------------------------------------------------------------------------------*/

static void TherapyEngine_BuildLeftHeelPlan(TherapyPlan_t *p)
{
  p->type = THERAPY_PLAN_OFFLOAD_LEFT_HEEL;
  p->target_zone = ZONE_LEFT_HEEL;
  p->board_id = MAP_LEFT_HEEL_BOARD_ID;

  // NOTE:
  // برای heel، support از اطراف calf/lower-leg می‌آید.
  // خود heel مستقیم بالا برده نمی‌شود.
  p->motor_count = 2U;
  p->motors[0].idx = MAP_LEFT_HEEL_MOTOR_A;
  p->motors[0].delta = MAP_HEEL_SUPPORT_DELTA;
  p->motors[1].idx = MAP_LEFT_HEEL_MOTOR_B;
  p->motors[1].delta = MAP_HEEL_SUPPORT_DELTA;
}
/*--------------------------------------------------------------------------------*/

static void TherapyEngine_BuildRightHeelPlan(TherapyPlan_t *p)
{
  p->type = THERAPY_PLAN_OFFLOAD_RIGHT_HEEL;
  p->target_zone = ZONE_RIGHT_HEEL;
  p->board_id = MAP_RIGHT_HEEL_BOARD_ID;

  // NOTE:
  // برای heel، support از اطراف calf/lower-leg می‌آید.
  // خود heel مستقیم بالا برده نمی‌شود.
  p->motor_count = 2U;
  p->motors[0].idx = MAP_RIGHT_HEEL_MOTOR_A;
  p->motors[0].delta = MAP_HEEL_SUPPORT_DELTA;
  p->motors[1].idx = MAP_RIGHT_HEEL_MOTOR_B;
  p->motors[1].delta = MAP_HEEL_SUPPORT_DELTA;
}
/*--------------------------------------------------------------------------------*/

static void TherapyEngine_BuildPlanForZone(TherapyPlan_t *p, uint8_t zone)
{
  if (p == 0)
    return;

  switch (zone)
  {
    case ZONE_LEFT_HEEL:
      TherapyEngine_BuildLeftHeelPlan(p);
      break;

    case ZONE_RIGHT_HEEL:
      TherapyEngine_BuildRightHeelPlan(p);
      break;

    case ZONE_SACRUM:
    default:
      TherapyEngine_BuildSacrumPlan(p);
      break;
  }
}
/*--------------------------------------------------------------------------------*/

void TherapyEngine_Run(TherapyEngineState_t *st,
                       const ZoneAnalysisResult_t *zone,
                       const MovementResult_t *movement,
                       const RiskResult_t *risk,
                       const AlertResult_t *alert,
                       const RecommendationResult_t *rec,
                       uint32_t now_ms)
{
  uint8_t reason_code = 0U;

  (void)movement;

  if ((st == 0) || (zone == 0) || (risk == 0) || (alert == 0) || (rec == 0))
    return;

  if (st->enabled == 0U)
    return;

  // NOTE:
  // اگر یک plan منتظر تایید است، plan جدید نمی‌سازیم.
  // UI/پرستار باید اول approve یا reject کند.
  if (TherapyEngine_HasPendingPlan(st) != 0U)
    return;

	#if THERAPY_DEBUG_FORCE_PLAN
	  // DEBUG:
	  // ساخت plan اجباری برای تست مسیر planner بدون وابستگی به risk واقعی.
	  reason_code = THERAPY_REASON_HIGH_RISK;
	#else
	  if (TherapyEngine_ShouldCreatePlan(risk, alert, rec, &reason_code) == 0U)
		return;
	#endif

	#if THERAPY_DEBUG_FORCE_PLAN
	  // DEBUG:
	  // در تست force plan، cooldown را bypass می‌کنیم تا فوراً pending_plan ساخته شود.
	#else
	  if ((now_ms - st->last_plan_ms) < THERAPY_PLAN_COOLDOWN_MS)
	  {
		st->blocked_count++;
		return;
	  }
	#endif

  TherapyEngine_ClearPlan(&st->pending_plan);

  uint8_t target_zone = TherapyEngine_SelectTargetZone(zone);
  TherapyEngine_BuildPlanForZone(&st->pending_plan, target_zone);

  st->plan_count++;
  st->last_plan_ms = now_ms;

  st->pending_plan.valid = 1U;
  st->pending_plan.status = THERAPY_PLAN_STATUS_PENDING_APPROVAL;
  st->pending_plan.created_ms = now_ms;
  st->pending_plan.plan_id = st->plan_count;
  // === new pending plan has not been sent to UI yet ===
  st->pending_plan.ui_sent = 0U;

  st->pending_plan.risk_score = risk->score;
  st->pending_plan.risk_level = risk->level;
  st->pending_plan.recommendation_code = rec->code;
  st->pending_plan.reason_code = reason_code;

  printf("THERAPY PLAN: pending id=%lu type=%u zone=%u motors=%u risk=%u level=%u reason=%u\r\n",
         (unsigned long)st->pending_plan.plan_id,
         (unsigned)st->pending_plan.type,
         (unsigned)st->pending_plan.target_zone,
         (unsigned)st->pending_plan.motor_count,
         (unsigned)st->pending_plan.risk_score,
         (unsigned)st->pending_plan.risk_level,
         (unsigned)st->pending_plan.reason_code);
}
/*--------------------------------------------------------------------------------*/

uint8_t TherapyEngine_ApproveAndExecutePendingPlan(TherapyEngineState_t *st)
{
  if (st == 0)
    return 0U;

  TherapyPlan_t *p = &st->pending_plan;

  if (p->valid == 0U)
    return 0U;

  if (p->status != THERAPY_PLAN_STATUS_PENDING_APPROVAL)
    return 0U;

  if (p->motor_count == 0U)
    return 0U;

  // NOTE:
  // این تنها نقطه اجرای موتور از plan است.
  // این تابع نباید اتوماتیک از TherapyEngine_Run صدا زده شود.
  // فقط بعد از approval از UI/پرستار مجاز است.
  if (MotorScheduler_EnqueueVectorMove(p->board_id, p->motors, p->motor_count) != pdPASS)
  {
    printf("THERAPY PLAN: execute fail id=%lu\r\n",
           (unsigned long)p->plan_id);
    return 0U;
  }

  p->status = THERAPY_PLAN_STATUS_EXECUTED;

  printf("THERAPY PLAN: executed id=%lu board=%u motors=%u\r\n",
         (unsigned long)p->plan_id,
         (unsigned)p->board_id,
         (unsigned)p->motor_count);

  TherapyEngine_ClearPlan(p);

  return 1U;
}
/*--------------------------------------------------------------------------------*/
uint8_t TherapyEngine_HasPendingPlanId(const TherapyEngineState_t *st,
                                       uint32_t plan_id)
{
  if (st == 0)
    return 0U;

  if (st->pending_plan.valid == 0U)
    return 0U;

  if (st->pending_plan.status != THERAPY_PLAN_STATUS_PENDING_APPROVAL)
    return 0U;

  if (st->pending_plan.plan_id != plan_id)
    return 0U;

  return 1U;
}
/*--------------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------------*/

