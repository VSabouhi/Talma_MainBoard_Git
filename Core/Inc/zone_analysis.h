#ifndef INC_ZONE_ANALYSIS_H_
#define INC_ZONE_ANALYSIS_H_
/*--------------------------------------------------------------------------------*/
#include <stdint.h>
#include "main.h"
/*--------------------------------------------------------------------------------*/

// === zone identifiers ===
// ناحیه‌های بالینی اصلی که در فاز اول بررسی می‌شوند
typedef enum
{
  ZONE_SACRUM = 0,
  ZONE_LEFT_HEEL,
  ZONE_RIGHT_HEEL,
  ZONE_LEFT_SHOULDER,
  ZONE_RIGHT_SHOULDER,
  ZONE_COUNT
} ZoneId_t;


// === one zone rectangle on bed map ===
// هر zone فعلاً به صورت یک مستطیل روی bed تعریف می‌شود
typedef struct
{
  uint8_t row_start;   // سطر شروع
  uint8_t row_end;     // سطر پایان (شامل این سطر)
  uint8_t col_start;   // ستون شروع
  uint8_t col_end;     // ستون پایان (شامل این ستون)
} ZoneRect_t;


// === computed metrics for one zone ===
// خروجی تحلیلی هر zone
typedef struct
{
  uint16_t sum;          // مجموع فشارهای معتبر zone
  uint8_t avg;           // میانگین فشار zone
  uint8_t peak;          // بیشترین فشار zone
  uint16_t active_cells; // تعداد سلول‌های فعال/دارای فشار
  uint16_t valid_cells;  // تعداد سلول‌های معتبر
} ZoneMetrics_t;


// === all zone metrics ===
// خروجی کامل تحلیل zoneها
typedef struct
{
  ZoneMetrics_t zone[ZONE_COUNT];
} ZoneAnalysisResult_t;
/*--------------------------------------------------------------------------------*/


// === initialize zone layout ===
// تعریف محدوده‌ی هندسی zoneها روی تخت
void ZoneAnalysis_Init(void);

// === run zone analysis on a stable bed snapshot ===
// تحلیل zoneها روی snapshot پایدار
void ZoneAnalysis_Run(const uint8_t bed_value[BED_ROWS][BED_COLS],
                      const uint8_t bed_valid[BED_ROWS][BED_COLS],
                      ZoneAnalysisResult_t *result);

/*--------------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------------*/

// === get static rectangle definition of one zone ===
const ZoneRect_t* ZoneAnalysis_GetRect(ZoneId_t zone);

#endif /* INC_ZONE_ANALYSIS_H_ */
