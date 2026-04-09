#ifndef INC_BED_MODEL_H_
#define INC_BED_MODEL_H_
/*----------------------------------------------------------------------------*/

#include <stdint.h>
#include "main.h"
/*----------------------------------------------------------------------------*/

// === full bed model arrays ===
// مدل کامل تخت (32×16)
// هر خانه نماینده یک سنسور در کل سیستم است
extern uint8_t bed_value[BED_ROWS][BED_COLS];
extern uint8_t bed_status[BED_ROWS][BED_COLS];
extern uint8_t bed_valid[BED_ROWS][BED_COLS];
extern uint8_t bed_confidence[BED_ROWS][BED_COLS];

// === send buffer for UI ===
// این بافرها فقط برای ارسال snapshot پایدار به UI استفاده می‌شوند
extern uint8_t bed_value_send[BED_ROWS][BED_COLS];
extern uint8_t bed_status_send[BED_ROWS][BED_COLS];
extern uint8_t bed_valid_send[BED_ROWS][BED_COLS];
extern uint8_t bed_confidence_send[BED_ROWS][BED_COLS];

// === bed synchronization state ===
// بیت هر نود: آیا در cycle جاری آپدیت شده یا نه
extern uint32_t bed_sync_mask;

// شماره snapshot کامل تخت (هر بار که همه نودها برسند، افزایش می‌یابد)
extern uint32_t bed_cycle;

// اگر 1 شود یعنی یک snapshot کامل جدید از کل تخت آماده شده
extern uint8_t  bed_snapshot_ready;


/*----------------------------------------------------------------------------*/

// === mapping helpers ===
// تبدیل node + sensor index → مختصات تخت

// محاسبه سطر (row) در تخت
uint8_t BedMap_GetRow(uint8_t node, uint8_t sensor_idx);

// محاسبه ستون (col) در تخت
uint8_t BedMap_GetCol(uint8_t sensor_idx);


// === update one node into bed model ===
// وقتی 32 سنسور یک نود کامل شد:
// این تابع آن را داخل مدل کل تخت کپی می‌کند
void BedModel_UpdateNode(uint8_t node,
                         const uint8_t sensor_value[NODES][SENSOR_COUNT_PER_NODE],
                         const uint8_t sensor_status[NODES][SENSOR_COUNT_PER_NODE],
                         const uint8_t valid_mask[NODES][SENSOR_COUNT_PER_NODE],
                         const uint8_t sensor_confidence[NODES][SENSOR_COUNT_PER_NODE]);


// === synchronization ===
// وقتی یک نود کامل شد این تابع صدا زده می‌شود
// اگر همه نودهای لازم رسیدند:
// → bed_cycle++
// → bed_snapshot_ready = 1
void BedSync_OnNodeUpdated(uint8_t node);
/*----------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------*/

#endif /* INC_BED_MODEL_H_ */
