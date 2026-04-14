#include "test_pattern.h"
/*----------------------------------------------------------------------------*/

void TestPattern_Generate(uint8_t bed_value[BED_ROWS][BED_COLS],
                          uint8_t bed_status[BED_ROWS][BED_COLS],
                          uint8_t bed_valid[BED_ROWS][BED_COLS])
{
    static uint8_t phase = 0;
    phase++;

    for (int r = 0; r < BED_ROWS; r++)
    {
        for (int c = 0; c < BED_COLS; c++)
        {
            uint8_t val = 50;

#if TEST_PATTERN_MODE == TEST_PATTERN_LINEAR
            val = (uint8_t)((r * BED_COLS + c) & 0x3F);

#elif TEST_PATTERN_MODE == TEST_PATTERN_ROW_GRAD
            val = (uint8_t)((r * 2) & 0x3F);

#elif TEST_PATTERN_MODE == TEST_PATTERN_COL_GRAD
            val = (uint8_t)((c * 4) & 0x3F);

#elif TEST_PATTERN_MODE == TEST_PATTERN_CENTER
            {
                int dr = r - 16;
                int dc = c - 8;
                int dist = dr * dr + dc * dc;

                if (dist < 20)
                    val = 10;   // distance کم = pressure زیاد
                else if (dist < 80)
                    val = 25;
                else
                    val = 50;
            }

#elif TEST_PATTERN_MODE == TEST_PATTERN_HEELS
            if ((r > 24 && c < 4) || (r > 24 && c > 11))
                val = 10;
            else
                val = 50;

#elif TEST_PATTERN_MODE == TEST_PATTERN_CHECKER
            val = ((r + c) % 2) ? 10 : 50;

#elif TEST_PATTERN_MODE == TEST_PATTERN_WAVE
            val = (uint8_t)((r * 3 + c * 2 + phase) & 0x3F);

#else
            val = 50;
#endif

            bed_value[r][c] = val;
            bed_status[r][c] = 0U;   // OK
            bed_valid[r][c] = 1U;    // valid
        }
    }
}
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
