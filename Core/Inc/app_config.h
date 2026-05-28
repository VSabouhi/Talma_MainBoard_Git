#ifndef INC_APP_CONFIG_H_
#define INC_APP_CONFIG_H_
/*----------------------------------------------------------------------------*/

#include <stdint.h>
/*----------------------------------------------------------------------------*/

// === system topology ===
#define NODES 16U

// === bed dimensions ===
#define BED_ROWS   32U
#define BED_COLS   16U

// === bed sync config ===
// [TEMP TEST] only node 1 connected
#define BED_REQUIRED_NODE_MASK   (1UL << 1)

// === sensor config ===
#define SENSOR_COUNT_PER_NODE   32U


/*----------------------------------------------------------------------------*/

/* --------------------------------------------------------------------------
 * THERAPY / RISK ENGINE CONFIGURATION
 *
 * تمام threshold ها و timing های clinical engine
 * از این struct خوانده می‌شوند.
 *
 * هدف:
 *   - قابل تنظیم بودن demo mode
 *   - قابل تنظیم بودن clinical mode
 *   - امکان تغییر از UI در آینده
 * -------------------------------------------------------------------------- */
typedef struct
{
  /* ------------------------------------------------------------------------
   * PRESSURE THRESHOLDS
   *
   * اگر average pressure یک ناحیه از این مقدار بیشتر شود،
   * exposure accumulation شروع می‌شود.
   *
   * Range:
   *   0..50
   * ------------------------------------------------------------------------ */

  uint8_t pressure_threshold_sacrum;
  uint8_t pressure_threshold_heel;
  uint8_t pressure_threshold_shoulder;


  /* ------------------------------------------------------------------------
   * EXPOSURE TIME THRESHOLDS (seconds)
   *
   * WATCH:
   *   فشار طولانی ولی هنوز clinical alert نیست
   *
   * ALERT:
   *   نیازمند بررسی / reposition
   *
   * CRITICAL:
   *   high-risk prolonged exposure
   * ------------------------------------------------------------------------ */

  uint16_t exposure_watch_s;
  uint16_t exposure_alert_s;
  uint16_t exposure_critical_s;


  /* ------------------------------------------------------------------------
   * MOVEMENT / IMMOBILITY THRESHOLDS
   *
   * اگر بیمار برای مدت طولانی movement نداشته باشد،
   * risk level سریع‌تر افزایش پیدا می‌کند.
   * ------------------------------------------------------------------------ */

  uint16_t no_movement_warn_s;
  uint16_t no_movement_alert_s;


  /* ------------------------------------------------------------------------
   * MOVEMENT DETECTION SENSITIVITY
   *
   * حداقل delta pressure برای اینکه movement واقعی
   * تشخیص داده شود.
   *
   * Range:
   *   0..50
   * ------------------------------------------------------------------------ */

  uint8_t movement_delta_threshold;


  /* ------------------------------------------------------------------------
   * PRESSURE DECAY
   *
   * وقتی فشار کاهش پیدا می‌کند،
   * exposure با چه سرعتی کم شود.
   *
   * 0:
   *   reset فوری
   *
   * 100:
   *   decay بسیار کند
   * ------------------------------------------------------------------------ */

  uint8_t exposure_decay_percent;


  /* ------------------------------------------------------------------------
   * DEMO MODE
   *
   * 0:
   *   clinical timings واقعی
   *
   * 1:
   *   واکنش سریع برای نمایشگاه/تست UI
   * ------------------------------------------------------------------------ */

  uint8_t demo_mode;

} TherapyRiskConfig_t;
/* --------------------------------------------------------------------------
 * THERAPY RISK CONFIG PRESETS
 *
 * DEMO:
 *   واکنش سریع برای UI/demo/testing
 *
 * CLINICAL_TEST:
 *   زمان‌بندی نزدیک‌تر به تست واقعی
 * -------------------------------------------------------------------------- */
typedef enum
{
  THERAPY_CONFIG_DEMO = 0,
  THERAPY_CONFIG_CLINICAL_TEST

} TherapyRiskPreset_t;
/* --------------------------------------------------------------------------
 * Global runtime risk configuration.
 *
 * همه risk/recommendation logic از این object استفاده می‌کنند.
 * بعداً UI می‌تواند این config را تغییر دهد.
 * -------------------------------------------------------------------------- */
extern TherapyRiskConfig_t g_risk_cfg;
/* --------------------------------------------------------------------------
 * Load predefined therapy/risk presets.
 * -------------------------------------------------------------------------- */
void TherapyRisk_LoadPreset(TherapyRiskPreset_t preset);






#endif
