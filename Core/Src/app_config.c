#include "app_config.h"

/* --------------------------------------------------------------------------
 * Global runtime therapy/risk configuration.
 * -------------------------------------------------------------------------- */
TherapyRiskConfig_t g_risk_cfg;




/* --------------------------------------------------------------------------
 * Load predefined therapy/risk preset values.
 * -------------------------------------------------------------------------- */
void TherapyRisk_LoadPreset(TherapyRiskPreset_t preset)
{
  switch (preset)
  {
    /* ----------------------------------------------------------------------
     * DEMO MODE
     *
     * واکنش سریع برای نمایش UI و تست development
     * ---------------------------------------------------------------------- */
    case THERAPY_CONFIG_DEMO:

      g_risk_cfg.pressure_threshold_sacrum  = 18U;
      g_risk_cfg.pressure_threshold_heel    = 15U;
      g_risk_cfg.pressure_threshold_shoulder= 15U;

      g_risk_cfg.exposure_watch_s    = 20U;     // 10
      g_risk_cfg.exposure_alert_s    = 40U;     // 20
      g_risk_cfg.exposure_critical_s = 60U;     // 40

      g_risk_cfg.no_movement_warn_s  = 15U;
      g_risk_cfg.no_movement_alert_s = 30U;

      g_risk_cfg.movement_delta_threshold = 4U;

      g_risk_cfg.exposure_decay_percent = 20U;

      g_risk_cfg.demo_mode = 1U;

      break;

    /* ----------------------------------------------------------------------
     * CLINICAL TEST MODE
     *
     * نزدیک‌تر به رفتار واقعی clinical
     * ---------------------------------------------------------------------- */
    case THERAPY_CONFIG_CLINICAL_TEST:

      g_risk_cfg.pressure_threshold_sacrum  = 22U;
      g_risk_cfg.pressure_threshold_heel    = 18U;
      g_risk_cfg.pressure_threshold_shoulder= 18U;

      g_risk_cfg.exposure_watch_s    = 15U * 60U;
      g_risk_cfg.exposure_alert_s    = 45U * 60U;
      g_risk_cfg.exposure_critical_s = 120U * 60U;

      g_risk_cfg.no_movement_warn_s  = 10U * 60U;
      g_risk_cfg.no_movement_alert_s = 20U * 60U;

      g_risk_cfg.movement_delta_threshold = 3U;

      g_risk_cfg.exposure_decay_percent = 5U;

      g_risk_cfg.demo_mode = 0U;

      break;

    default:
      break;
  }
}
