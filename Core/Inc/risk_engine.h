#ifndef INC_RISK_ENGINE_H_
#define INC_RISK_ENGINE_H_
/*--------------------------------------------------------------------------------*/

#include <stdint.h>
#include "zone_analysis.h"
#include "movement.h"
/*--------------------------------------------------------------------------------*/

// === risk levels ===
typedef enum
{
  RISK_LEVEL_LOW = 0,
  RISK_LEVEL_MODERATE,
  RISK_LEVEL_HIGH,
  RISK_LEVEL_CRITICAL
} RiskLevel_t;


// === risk engine output ===
typedef struct
{
  uint8_t score;   // 0..100
  uint8_t level;   // RiskLevel_t
} RiskResult_t;

/*--------------------------------------------------------------------------------*/

// === compute current pressure injury risk ===
void RiskEngine_Run(const ZoneAnalysisResult_t *zone_res,
                    const MovementResult_t *movement_res,
                    RiskResult_t *risk_res);
/*--------------------------------------------------------------------------------*/


/*--------------------------------------------------------------------------------*/

#endif /* INC_RISK_ENGINE_H_ */
