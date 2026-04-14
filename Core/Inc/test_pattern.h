#ifndef INC_TEST_PATTERN_H_
#define INC_TEST_PATTERN_H_
/*----------------------------------------------------------------------------*/

#include <stdint.h>
#include "app_config.h"
/*----------------------------------------------------------------------------*/

// === enable / disable synthetic test pattern injection ===
#define TEST_PATTERN_ENABLE   1

// === available test patterns ===
#define TEST_PATTERN_LINEAR       1U
#define TEST_PATTERN_ROW_GRAD     2U
#define TEST_PATTERN_COL_GRAD     3U
#define TEST_PATTERN_CENTER       4U
#define TEST_PATTERN_HEELS        5U
#define TEST_PATTERN_CHECKER      6U
#define TEST_PATTERN_WAVE         7U

// === current selected pattern ===
#define TEST_PATTERN_MODE         TEST_PATTERN_CHECKER
/*----------------------------------------------------------------------------*/

// === generate one artificial bed snapshot ===
void TestPattern_Generate(uint8_t bed_value[BED_ROWS][BED_COLS],
                          uint8_t bed_status[BED_ROWS][BED_COLS],
                          uint8_t bed_valid[BED_ROWS][BED_COLS]);
/*----------------------------------------------------------------------------*/

#endif /* INC_TEST_PATTERN_H_ */
