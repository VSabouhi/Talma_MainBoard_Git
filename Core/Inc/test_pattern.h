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
  // === real ===
  PATTERN_REAL_DATA = 0,

  // === debug ===
  PATTERN_GRADIENT_DEBUG,
  PATTERN_CHECKERBOARD_DEBUG,

  // === base body ===
  PATTERN_BODY_REALISTIC_SUPINE,
  PATTERN_BODY_ADULT_NORMAL,
  PATTERN_BODY_SHORT_LIGHT,
  //PATTERN_BODY_TALL_HEAVY,   // فقط اگر implement کردی

  // === spatial variations ===
  PATTERN_BODY_SHIFT_LEFT,
  PATTERN_BODY_SHIFT_RIGHT,

  // === clinical scenarios ===
  PATTERN_BODY_SACRUM_DOMINANT,
  PATTERN_BODY_ONE_HEEL_DOMINANT,
  PATTERN_BODY_PARTIAL_FAULT,
  PATTERN_BODY_RESTLESS,

  PATTERN_BODY_SIDE_LEFT,
  PATTERN_BODY_SIDE_RIGHT,

  PATTERN_BODY_TURNING_CYCLE,
  PATTERN_BODY_TURNING_CYCLE_SMOOTH,
} TestPatternMode_t;


// === currently selected synthetic pattern ===
#define TEST_PATTERN_MODE   PATTERN_BODY_TURNING_CYCLE_SMOOTH


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
