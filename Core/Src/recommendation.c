#include "recommendation.h"

void Recommendation_Run(const RiskResult_t *risk,
                        const AlertResult_t *alert,
                        const MovementResult_t *movement,
                        RecommendationResult_t *rec)
{
  if ((risk == 0) || (alert == 0) || (movement == 0) || (rec == 0))
    return;

  // === default ===
  rec->code = RECOMMEND_NONE;
  rec->priority = 0U;

  // === urgent case ===
  if ((alert->active != 0U) && (alert->severity == ALERT_SEV_HIGH))
  {
    rec->code = RECOMMEND_URGENT_REPOSITION;
    rec->priority = 3U;
    return;
  }

  // === active alert ===
  if (alert->active != 0U)
  {
    rec->code = RECOMMEND_OFFLOAD_SACRUM;
    rec->priority = 2U;
    return;
  }

  // === high risk without current alert ===
  if (risk->level == RISK_LEVEL_HIGH || risk->level == RISK_LEVEL_CRITICAL)
  {
    rec->code = RECOMMEND_REPOSITION;
    rec->priority = 2U;
    return;
  }

  // === moderate risk and no recent movement event ===
  if ((risk->level == RISK_LEVEL_MODERATE) && (movement->detected == 0U))
  {
    rec->code = RECOMMEND_MONITOR;
    rec->priority = 1U;
    return;
  }

  // === otherwise no action ===
  rec->code = RECOMMEND_NONE;
  rec->priority = 0U;
}
