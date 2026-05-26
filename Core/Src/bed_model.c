#include "bed_model.h"
#include <string.h>   // برای memcpy
#include "main.h"   // برای دسترسی به BED_REQUIRED_NODE_MASK و تنظیمات سیستم
#include "app_config.h"
#include "test_pattern.h"
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/

// === full bed model storage ===
// حافظه اصلی نگهداری وضعیت کل تخت
uint8_t bed_value[BED_ROWS][BED_COLS];
uint8_t bed_status[BED_ROWS][BED_COLS];
uint8_t bed_valid[BED_ROWS][BED_COLS];
uint8_t bed_confidence[BED_ROWS][BED_COLS];

// === send buffer for UI (double buffer) ===
// این بافرها فقط برای ارسال به UI استفاده می‌شوند
// داده‌ها از بافر اصلی bed_* در لحظه تشکیل snapshot کامل کپی می‌شوند
uint8_t bed_value_send[BED_ROWS][BED_COLS];
uint8_t bed_status_send[BED_ROWS][BED_COLS];
uint8_t bed_valid_send[BED_ROWS][BED_COLS];
uint8_t bed_confidence_send[BED_ROWS][BED_COLS];

// === bed synchronization state ===
uint32_t bed_sync_mask = 0U;     // بیت هر نود: آیا در این cycle آپدیت شده؟
uint32_t bed_cycle = 0U;         // شمارنده snapshot کامل تخت
uint8_t  bed_snapshot_ready = 0U; // اگر 1 شود یعنی snapshot کامل آماده است

/*----------------------------------------------------------------------------*/

// === map node + sensor index to bed row ===
// هر نود دو ردیف دارد:
// sensor 0..15  → row = 2*node
// sensor 16..31 → row = 2*node + 1
uint8_t BedMap_GetRow(uint8_t node, uint8_t sensor_idx)
{
  return (uint8_t)((2U * node) + (sensor_idx / 16U));
}

/*----------------------------------------------------------------------------*/

// === map sensor index to column ===
// هر ردیف 16 ستون دارد (0..15)
uint8_t BedMap_GetCol(uint8_t sensor_idx)
{
  return (uint8_t)(sensor_idx % 16U);
}

/*----------------------------------------------------------------------------*/

// === copy one node into global bed model ===
// وقتی 32 سنسور یک نود کامل شد:
// داده‌های آن نود در مدل کل تخت کپی می‌شود
void BedModel_UpdateNode(uint8_t node,
                         const uint8_t sensor_value[NODES][SENSOR_COUNT_PER_NODE],
                         const uint8_t sensor_status[NODES][SENSOR_COUNT_PER_NODE],
                         const uint8_t valid_mask[NODES][SENSOR_COUNT_PER_NODE],
                         const uint8_t sensor_confidence[NODES][SENSOR_COUNT_PER_NODE])
{
  if (node >= (uint8_t)NODES)
    return;

  for (uint8_t i = 0; i < SENSOR_COUNT_PER_NODE; i++)
  {
    uint8_t row = BedMap_GetRow(node, i);
    uint8_t col = BedMap_GetCol(i);

    // محافظت در برابر out-of-bound
    if (row >= BED_ROWS) continue;
    if (col >= BED_COLS) continue;

    // کپی داده‌ها به مدل کل تخت
    bed_value[row][col]      = sensor_value[node][i];
    bed_status[row][col]     = sensor_status[node][i];
    bed_valid[row][col]      = valid_mask[node][i];
    bed_confidence[row][col] = sensor_confidence[node][i];
  }
}

/*----------------------------------------------------------------------------*/

// === update bed synchronization ===
// هر بار یک نود کامل شد:
// این تابع صدا زده می‌شود
void BedSync_OnNodeUpdated(uint8_t node)
{
  if (node >= (uint8_t)NODES)
    return;

  // علامت بزن که این نود در cycle جاری تخت آپدیت شده
  bed_sync_mask |= (1UL << node);

  // اگر همه نودهای لازم حاضر شدند:
  if ((bed_sync_mask & BED_REQUIRED_NODE_MASK) == BED_REQUIRED_NODE_MASK)
  {
    // === یک snapshot کامل جدید از تخت تشکیل شده ===
    bed_cycle++;



	#if TEST_PATTERN_ENABLE
		// === inject artificial bed pattern for UI testing ===
		// این بخش فقط برای تست UI است و داده واقعی تخت را override می‌کند
		TestPattern_Generate(bed_value, bed_status, bed_valid);

		/* ----------------------------------------------------------------------
		 * DEBUG / UI PATTERN TEST:
		 *
		 * Force all generated pattern cells to OK/valid.
		 * This prevents UI from overlaying old/raw fault status as X marks
		 * while bed_value is synthetic.
		 * ---------------------------------------------------------------------- */
		for (uint8_t r = 0U; r < BED_ROWS; r++)
		{
		  for (uint8_t c = 0U; c < BED_COLS; c++)
		  {
		    bed_status[r][c] = 0U;       /* OK */
		    bed_valid[r][c] = 1U;        /* valid */
		    bed_confidence[r][c] = 255U; /* full confidence */
		  }
		}
	#endif


    // === کپی snapshot پایدار برای UI ===
    // این کپی دقیقاً در لحظه‌ای انجام می‌شود که
    // همه نودهای لازم آپدیت شده‌اند
    // بنابراین داده‌ی ارسالی به UI کاملاً sync و پایدار خواهد بود
    memcpy(bed_value_send,      bed_value,      sizeof(bed_value));
    memcpy(bed_status_send,     bed_status,     sizeof(bed_status));
    memcpy(bed_valid_send,      bed_valid,      sizeof(bed_valid));
    memcpy(bed_confidence_send, bed_confidence, sizeof(bed_confidence));

    // === اعلام کن که snapshot کامل جدید آماده ارسال است ===
    bed_snapshot_ready = 1U;

    // reset برای cycle بعدی
    bed_sync_mask = 0U;
  }
}
/*----------------------------------------------------------------------------*/

