#ifndef INC_THERAPY_ENGINE_H_
#define INC_THERAPY_ENGINE_H_
/*--------------------------------------------------------------------------------*/

#include <stdint.h>
#include "zone_analysis.h"
#include "movement.h"
#include "risk_engine.h"
#include "alert_engine.h"
#include "recommendation.h"
#include "motor_scheduler.h"
/*--------------------------------------------------------------------------------*/

// === THERAPY / INTERVENTION PLANNER ===
// این لایه هیچ موتور را مستقیم حرکت نمی‌دهد.
// فقط plan پیشنهادی می‌سازد و منتظر تایید UI/پرستار می‌ماند.

#define THERAPY_PLAN_MAX_MOTORS   8U
/*--------------------------------------------------------------------------------*/

typedef enum
{
  THERAPY_PLAN_NONE = 0,
  THERAPY_PLAN_OFFLOAD_SACRUM,
  THERAPY_PLAN_OFFLOAD_LEFT_HEEL,
  THERAPY_PLAN_OFFLOAD_RIGHT_HEEL,
  THERAPY_PLAN_OFFLOAD_SHOULDER
} TherapyPlanType_t;

typedef enum
{
  THERAPY_PLAN_STATUS_EMPTY = 0,
  THERAPY_PLAN_STATUS_PENDING_APPROVAL,
  THERAPY_PLAN_STATUS_APPROVED,
  THERAPY_PLAN_STATUS_EXECUTED,
  THERAPY_PLAN_STATUS_REJECTED
} TherapyPlanStatus_t;

typedef struct
{
  uint8_t valid;
  uint8_t status;
  uint8_t type;
  uint8_t target_zone;

  uint8_t board_id;
  uint8_t motor_count;
  MotorVectorItem_t motors[THERAPY_PLAN_MAX_MOTORS];

  uint8_t risk_score;
  uint8_t risk_level;
  uint8_t recommendation_code;

  uint32_t created_ms;
  uint32_t plan_id;
  // === UI delivery state ===
  // جلوگیری از ارسال تکراری یک plan در هر snapshot cycle
  uint8_t ui_sent;

  // NOTE:
  // reason_code عددی است تا فعلاً بدون string سنگین در packetهای UI استفاده شود.
  // بعداً UI می‌تواند این code را به متن قابل نمایش تبدیل کند.
  uint8_t reason_code;
} TherapyPlan_t;

typedef struct
{
  uint8_t enabled;              // 0: planner خاموش، 1: اجازه ساخت plan
  uint32_t last_plan_ms;        // زمان آخرین plan
  uint32_t plan_count;          // تعداد planهای ساخته‌شده
  uint32_t blocked_count;       // دفعات block به دلیل cooldown/شرایط ناکافی
  TherapyPlan_t pending_plan;   // آخرین plan منتظر تایید
} TherapyEngineState_t;


/* --------------------------------------------------------------------------
 * Global therapy engine instance
 * -------------------------------------------------------------------------- */
extern TherapyEngineState_t g_therapy;
/*--------------------------------------------------------------------------------*/

void TherapyEngine_Init(TherapyEngineState_t *st);
void TherapyEngine_SetEnabled(TherapyEngineState_t *st, uint8_t enabled);

void TherapyEngine_Run(TherapyEngineState_t *st,
                       const ZoneAnalysisResult_t *zone,
                       const MovementResult_t *movement,
                       const RiskResult_t *risk,
                       const AlertResult_t *alert,
                       const RecommendationResult_t *rec,
                       uint32_t now_ms);

const TherapyPlan_t *TherapyEngine_GetPendingPlan(const TherapyEngineState_t *st);

void TherapyEngine_RejectPendingPlan(TherapyEngineState_t *st);

// === clear pending plan after approval/execution handoff ===
// این reject نیست؛ فقط plan از pending approval خارج می‌شود.
void TherapyEngine_ClearPendingPlan(TherapyEngineState_t *st);

// این تابع بعداً با approval از UI صدا زده می‌شود.
// فعلاً هیچ جای اتوماتیکی نباید این را صدا بزند.
uint8_t TherapyEngine_ApproveAndExecutePendingPlan(TherapyEngineState_t *st);
/*--------------------------------------------------------------------------------*/
// === approval helpers ===
// بررسی اینکه آیا plan_id هنوز pending و معتبر است یا نه.
uint8_t TherapyEngine_HasPendingPlanId(const TherapyEngineState_t *st,
                                       uint32_t plan_id);
/*--------------------------------------------------------------------------------*/
void TherapyEngine_DebugCreateUiTestPlan(TherapyEngineState_t *st,
                                         uint32_t plan_id,
                                         uint32_t now_ms);
/*--------------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------------*/

#endif
