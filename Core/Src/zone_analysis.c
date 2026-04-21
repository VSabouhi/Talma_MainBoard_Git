#include "zone_analysis.h"
#include <string.h>

// === pressure threshold for an "active" cell ===
// اگر مقدار فشار از این بیشتر باشد، آن سلول را active حساب می‌کنیم
#define ZONE_ACTIVE_THRESHOLD   1U

// === static layout for all clinical zones ===
// فعلاً این مقادیر اولیه و قابل تنظیم هستند
static ZoneRect_t g_zone_rects[ZONE_COUNT];


// === initialize rectangular zones on the 32x16 bed ===
// این محدوده‌ها فعلاً تقریبی هستند و بعداً با چیدمان واقعی بدن تنظیم می‌شوند
void ZoneAnalysis_Init(void)
{
  // === sacrum zone ===
  g_zone_rects[ZONE_SACRUM].row_start = 15U;
  g_zone_rects[ZONE_SACRUM].row_end   = 20U;
  g_zone_rects[ZONE_SACRUM].col_start = 5U;
  g_zone_rects[ZONE_SACRUM].col_end   = 10U;

  // === left heel zone ===
  g_zone_rects[ZONE_LEFT_HEEL].row_start = 28U;
  g_zone_rects[ZONE_LEFT_HEEL].row_end   = 31U;
  g_zone_rects[ZONE_LEFT_HEEL].col_start = 5U;
  g_zone_rects[ZONE_LEFT_HEEL].col_end   = 7U;

  // === right heel zone ===
  g_zone_rects[ZONE_RIGHT_HEEL].row_start = 28U;
  g_zone_rects[ZONE_RIGHT_HEEL].row_end   = 31U;
  g_zone_rects[ZONE_RIGHT_HEEL].col_start = 8U;
  g_zone_rects[ZONE_RIGHT_HEEL].col_end   = 10U;

  // === left shoulder zone ===
  g_zone_rects[ZONE_LEFT_SHOULDER].row_start = 6U;
  g_zone_rects[ZONE_LEFT_SHOULDER].row_end   = 10U;
  g_zone_rects[ZONE_LEFT_SHOULDER].col_start = 3U;
  g_zone_rects[ZONE_LEFT_SHOULDER].col_end   = 6U;

  // === right shoulder zone ===
  g_zone_rects[ZONE_RIGHT_SHOULDER].row_start = 6U;
  g_zone_rects[ZONE_RIGHT_SHOULDER].row_end   = 10U;
  g_zone_rects[ZONE_RIGHT_SHOULDER].col_start = 9U;
  g_zone_rects[ZONE_RIGHT_SHOULDER].col_end   = 12U;
}


// === get rectangle definition of one zone ===
const ZoneRect_t* ZoneAnalysis_GetRect(ZoneId_t zone)
{
  if (zone >= ZONE_COUNT)
    return 0;

  return &g_zone_rects[zone];
}


// === compute metrics for all zones ===
// فقط روی سلول‌های valid محاسبه انجام می‌شود
void ZoneAnalysis_Run(const uint8_t bed_value[BED_ROWS][BED_COLS],
                      const uint8_t bed_valid[BED_ROWS][BED_COLS],
                      ZoneAnalysisResult_t *result)
{
  if (result == 0)
    return;

  // === clear previous result ===
  memset(result, 0, sizeof(*result));

  for (uint8_t z = 0; z < (uint8_t)ZONE_COUNT; z++)
  {
    const ZoneRect_t *rect = &g_zone_rects[z];
    ZoneMetrics_t *m = &result->zone[z];

    for (uint8_t r = rect->row_start; r <= rect->row_end; r++)
    {
      for (uint8_t c = rect->col_start; c <= rect->col_end; c++)
      {
        // محافظت اضافه
        if (r >= BED_ROWS) continue;
        if (c >= BED_COLS) continue;

        // فقط سلول‌های معتبر در محاسبه شرکت می‌کنند
        if (bed_valid[r][c] == 0U)
          continue;

        uint8_t v = bed_value[r][c];

        m->valid_cells++;
        m->sum += v;

        if (v > m->peak)
          m->peak = v;

        if (v >= ZONE_ACTIVE_THRESHOLD)
          m->active_cells++;
      }
    }

    // === average pressure of zone ===
    if (m->valid_cells > 0U)
      m->avg = (uint8_t)(m->sum / m->valid_cells);
    else
      m->avg = 0U;
  }
}
