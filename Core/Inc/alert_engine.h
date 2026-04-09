#ifndef INC_ALERT_ENGINE_H_
#define INC_ALERT_ENGINE_H_
/*--------------------------------------------------------------------------------*/

#include <stdint.h>
#include "risk_engine.h"
#include "movement.h"
/*--------------------------------------------------------------------------------*/

// === alert types ===
typedef enum
{
  ALERT_NONE = 0,
  ALERT_HIGH_PRESSURE
} AlertType_t;

// === alert severity ===
typedef enum
{
  ALERT_SEV_LOW = 0,
  ALERT_SEV_MED,
  ALERT_SEV_HIGH
} AlertSeverity_t;

// === alert result ===
typedef struct
{
  uint8_t active;
  uint8_t type;
  uint8_t severity;

  uint32_t start_time_ms;   // زمان شروع داخلی بر حسب ms
  uint32_t duration_s;      // مدت فعال بودن برای منطق/نمایش بر حسب ثانیه
} AlertResult_t;
/*--------------------------------------------------------------------------------*/

void AlertEngine_Init(void);

void AlertEngine_Run(const RiskResult_t *risk,
                     const MovementResult_t *movement,
                     AlertResult_t *alert);
/*--------------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------------*/

#endif
