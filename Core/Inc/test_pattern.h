#ifndef INC_TEST_PATTERN_H_
#define INC_TEST_PATTERN_H_

#include <stdint.h>
#include "app_config.h"

// === master enable for synthetic bed patterns ===
// 0 = use real sensor data
// 1 = override bed snapshot with synthetic pattern
#define TEST_PATTERN_ENABLE   1

// === available pattern modes ===
typedef enum
{
  PATTERN_REAL_DATA = 0,
  PATTERN_GRADIENT_DEBUG = 1,
  PATTERN_CHECKERBOARD_DEBUG = 2,
  PATTERN_BODY_ADULT_NORMAL = 3,
  PATTERN_BODY_SHORT_LIGHT = 4,
  PATTERN_BODY_TALL_HEAVY = 5,
  PATTERN_BODY_SHIFT_LEFT = 6,
  PATTERN_BODY_SHIFT_RIGHT = 7,
  PATTERN_BODY_ONE_HEEL_DOMINANT = 8,
  PATTERN_BODY_RESTLESS = 9,
  PATTERN_BODY_SACRUM_DOMINANT = 10,
  PATTERN_BODY_PARTIAL_FAULT = 11,
  PATTERN_BODY_REALISTIC_SUPINE= 12

} TestPatternMode_t;


// === currently selected synthetic pattern ===
#define TEST_PATTERN_MODE   PATTERN_BODY_REALISTIC_SUPINE


// === synthetic body metadata ===
// این اطلاعات برای debug و مقایسه با UI مفیدند
typedef struct
{
  uint8_t body_top_row;
  uint8_t body_bottom_row;
  uint8_t center_col;
} TestPatternInfo_t;


// === generate one artificial bed snapshot ===
void TestPattern_Generate(uint8_t bed_value[BED_ROWS][BED_COLS],
                          uint8_t bed_status[BED_ROWS][BED_COLS],
                          uint8_t bed_valid[BED_ROWS][BED_COLS]);

// === get metadata of current synthetic pattern ===
const TestPatternInfo_t* TestPattern_GetInfo(void);

#endif /* INC_TEST_PATTERN_H_ */
