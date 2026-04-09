#ifndef INC_RECOMMENDATION_H_
#define INC_RECOMMENDATION_H_

#include <stdint.h>
#include "risk_engine.h"
#include "alert_engine.h"
#include "movement.h"

// === recommendation codes ===
typedef enum
{
  RECOMMEND_NONE = 0,
  RECOMMEND_MONITOR,
  RECOMMEND_REPOSITION,
  RECOMMEND_URGENT_REPOSITION,
  RECOMMEND_OFFLOAD_SACRUM
} RecommendationCode_t;


// === recommendation result ===
typedef struct
{
  uint8_t code;      // RecommendationCode_t
  uint8_t priority;  // 0..3
} RecommendationResult_t;


// === compute recommendation from current summary state ===
void Recommendation_Run(const RiskResult_t *risk,
                        const AlertResult_t *alert,
                        const MovementResult_t *movement,
                        RecommendationResult_t *rec);

#endif /* INC_RECOMMENDATION_H_ */
