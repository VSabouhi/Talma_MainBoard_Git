#include "alert_engine.h"
#include "stm32f7xx_hal.h"
/*--------------------------------------------------------------------------------*/

static uint8_t alert_active = 0;
static uint32_t alert_start = 0;
/*--------------------------------------------------------------------------------*/

// === alert escalation thresholds ===
// این‌ها برای فاز اول مقادیر عملیاتی اولیه هستند
// بعداً با تست واقعی و سناریوهای بالینی تیون می‌شوند

// اگر ریسک بالا و بی‌حرکتی برقرار باشد، بعد از 10 ثانیه alert فعال شود
#define ALERT_ON_DELAY_S          10U                 // تا 10 ثانیه: فقط pending
													  // از 10 ثانیه به بعد: alert active با severity پایین
// بعد از 30 ثانیه severity بالاتر برود
#define ALERT_SEV_MED_DELAY_S     60U                 // بعد از 60 ثانیه: severity متوسط

// بعد از 60 ثانیه severity به بالا برسد
#define ALERT_SEV_HIGH_DELAY_S    180U 					// بعد از 180 ثانیه: severity بالا
/*--------------------------------------------------------------------------------*/

void AlertEngine_Init(void)
{
  alert_active = 0;
  alert_start = 0;
}
/*--------------------------------------------------------------------------------*/
void AlertEngine_Run(const RiskResult_t *risk,
                     const MovementResult_t *movement,
                     AlertResult_t *alert)
{
  if ((risk == 0) || (movement == 0) || (alert == 0))
    return;

  uint8_t condition = 0;
  uint32_t now = HAL_GetTick();

  // === شرط خام: فشار/ریسک بالا + بدون حرکت ===
  if ((risk->score > 50U) && (movement->detected == 0U))
  {
    condition = 1U;
  }

  if (condition)
  {
    // === شروع پنجره زمانی alert ===
    if (!alert_active)
    {
      alert_active = 1U;
      alert_start = now;
    }

    uint32_t elapsed_ms = now - alert_start;
    uint32_t elapsed_s  = elapsed_ms / 1000U;

    // === فقط بعد از یک تاخیر اولیه alert را active کن ===
    if (elapsed_s >= ALERT_ON_DELAY_S)
    {
      alert->active = 1U;
      alert->type = ALERT_HIGH_PRESSURE;
      alert->start_time_ms = alert_start;
      alert->duration_s = elapsed_s;

      // === escalation based on duration ===
      if (elapsed_s >= ALERT_SEV_HIGH_DELAY_S)
        alert->severity = ALERT_SEV_HIGH;
      else if (elapsed_s >= ALERT_SEV_MED_DELAY_S)
        alert->severity = ALERT_SEV_MED;
      else
        alert->severity = ALERT_SEV_LOW;
    }
    else
    {
      // هنوز به تاخیر اولیه نرسیده، فقط pending است
      alert->active = 0U;
      alert->type = ALERT_NONE;
      alert->severity = 0U;
      alert->start_time_ms = alert_start;
      alert->duration_s = elapsed_s;
    }
  }
  else
  {
    // === شرط از بین رفته: alert reset ===
    alert_active = 0U;
    alert_start = 0U;

    alert->active = 0U;
    alert->type = ALERT_NONE;
    alert->severity = 0U;
    alert->start_time_ms = 0U;
    alert->duration_s = 0U;
  }
}
/*--------------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------------*/
