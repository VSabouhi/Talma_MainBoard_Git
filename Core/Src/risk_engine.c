#include "risk_engine.h"
/*--------------------------------------------------------------------------------*/

// === clamp helper ===
static uint8_t Risk_ClampTo100(int32_t x)
{
  if (x < 0)   return 0U;
  if (x > 100) return 100U;
  return (uint8_t)x;
}


// === map score to discrete level ===
static uint8_t Risk_MapLevel(uint8_t score)
{
  if (score >= 75U) return RISK_LEVEL_CRITICAL;
  if (score >= 50U) return RISK_LEVEL_HIGH;
  if (score >= 25U) return RISK_LEVEL_MODERATE;
  return RISK_LEVEL_LOW;
}

/*--------------------------------------------------------------------------------*/

// === first simple rule-based risk model ===
// این نسخه اولیه و سبک است و بعداً دقیق‌تر می‌شود
void RiskEngine_Run(const ZoneAnalysisResult_t *zone_res,
                    const MovementResult_t *movement_res,
                    RiskResult_t *risk_res)
{
  if ((zone_res == 0) || (movement_res == 0) || (risk_res == 0))
    return;

  int32_t score = 0;

  // === focus on sacrum for phase 1 ===
  // === convert distance → pressure (inverted) ===
  // چون مقدار کمتر یعنی فشار بیشتر
  uint8_t sacrum_avg  = 63U - zone_res->zone[ZONE_SACRUM].avg;
  uint8_t sacrum_peak = 63U - zone_res->zone[ZONE_SACRUM].peak;

  // === average pressure contribution ===
  // وزن اصلی از فشار متوسط می‌آید
  score += sacrum_avg;

  // === peak pressure penalty ===
  // اگر peak خیلی بالاست، کمی جریمه اضافه بده
  score += (sacrum_peak / 2);

  // === movement effect ===
  // اگر حرکت دیده نشده، ریسک کمی بالا برود
  // اگر حرکت دیده شده، کمی ریسک کم شود
  if (movement_res->detected == 0U)
    score += 10;
  else
    score -= 5;

  // === clamp and classify ===
  risk_res->score = Risk_ClampTo100(score);
  risk_res->level = Risk_MapLevel(risk_res->score);
}
/*--------------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------------*/
