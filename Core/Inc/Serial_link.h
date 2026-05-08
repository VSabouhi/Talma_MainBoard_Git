#ifndef INC_SERIAL_LINK_H_
#define INC_SERIAL_LINK_H_
/*----------------------------------------------------------------------------*/

#include <stdint.h>
#include "FreeRTOS.h"
#include "queue.h"
#include "therapy_engine.h"
/*----------------------------------------------------------------------------*/

// === serial link message types ===
// نوع پیام خروجی برای UI
#define SL_MSG_TYPE_NODE32         1U
#define SL_MSG_TYPE_BED_SNAPSHOT   2U
#define SL_MSG_TYPE_BED_STATUS     3U   //
#define SL_MSG_TYPE_NODE_HEALTH    4U   // وضعیت همه نودهاپیام نقشه وضعیت کل تخت
#define SL_MSG_TYPE_SUMMARY        5U   // summary packet for UI
#define SL_MSG_TYPE_INTERVENTION_PLAN  6U
/*----------------------------------------------------------------------------*/
// === node32 payload ===
// payload مربوط به یک نود (32 سنسور)
typedef struct {
  uint8_t node;      // شماره نود
  uint16_t cycle;    // cycle همان نود
  uint8_t flags;     // فلگ‌ها (مثل state نود)
  uint8_t s[32];     // 32 بایت داده سنسورها
} SL_Node32Payload;


// === bed snapshot payload ===
// payload مربوط به snapshot کامل تخت
typedef struct {
  uint16_t bed_cycle;   // شماره cycle کامل تخت
} SL_BedSnapshotPayload;


// === bed status payload ===
// پیام مربوط به نقشه وضعیت کل تخت
typedef struct {
  uint16_t bed_cycle;   // شماره cycle کامل تخت
} SL_BedStatusPayload;

// === node health payload ===
// این پیام مربوط به وضعیت همه نودها است
typedef struct {
  uint16_t bed_cycle;   // cycle مرجع این گزارش
} SL_NodeHealthPayload;


// === summary payload ===
// این ساختار همه متریک‌های لازم برای UI را داخل queue حمل می‌کند
typedef struct {
	  uint16_t frame_id;
	  uint16_t uptime_s;
	  uint8_t risk_score;
	  uint8_t risk_level;
	  uint8_t movement_detected;
	  uint16_t time_since_last_movement_s;
	  uint8_t alert_active;
	  uint8_t alert_type;
	  uint8_t alert_severity;
	  uint16_t alert_duration_s;
	  uint8_t recommendation_code;
	  uint8_t recommendation_priority;
	  uint8_t sacrum_avg;
	  uint8_t sacrum_peak;
	  uint8_t heel_left_avg;
	  uint8_t heel_right_avg;
	  uint8_t shoulders_avg;
	  uint8_t shoulders_peak;
	  uint8_t pressure_exposure_threshold;
	  uint16_t sacrum_exposure_s;
	  uint16_t heels_exposure_s;
	  uint16_t shoulders_exposure_s;
	  uint8_t zones_valid_mask;
	  uint8_t summary_flags;
} SL_SummaryPayload;

// === intervention plan payload ===
// کل plan کپی می‌شود تا هنگام ارسال از queue پایدار باشد.
typedef struct {
  TherapyPlan_t plan;
} SL_InterventionPlanPayload;

// === generic serial link message ===
// این پیام می‌تواند یکی از چند نوع خروجی UI باشد
typedef struct {
  uint8_t type;   // نوع پیام: NODE32 یا BED_SNAPSHOT

  // === payloadهای ممکن برای خروجی UI ===
  union {
    SL_Node32Payload node32;       // پیام 32 سنسور یک نود
    SL_BedSnapshotPayload bed;     // snapshot کامل تخت
    SL_BedStatusPayload status;    // status کل تخت
    SL_NodeHealthPayload health;   // وضعیت همه نودها
    SL_SummaryPayload summary;
    SL_InterventionPlanPayload intervention;
  } payload;

} SL_Msg;
/*----------------------------------------------------------------------------*/

// === درخواست ارسال snapshot کامل تخت ===
// خود داده‌های تخت در bed_model نگهداری می‌شوند؛
// این پیام فقط اعلام می‌کند که یک snapshot جدید باید ارسال شود
BaseType_t SerialLink_SendBedSnapshot_Async(uint16_t bed_cycle);
BaseType_t SerialLink_SendBedStatus_Async(uint16_t bed_cycle);
// === درخواست ارسال وضعیت همه نودها ===
// داده واقعی node_state از node_state module خوانده می‌شود
BaseType_t SerialLink_SendNodeHealth_Async(uint16_t bed_cycle);

// === enqueue summary packet for UI ===
BaseType_t SerialLink_SendSummary_Async(uint16_t frame_id,
                                        uint16_t uptime_s,
                                        uint8_t risk_score,
                                        uint8_t risk_level,
                                        uint8_t movement_detected,
                                        uint16_t time_since_last_movement_s,
                                        uint8_t alert_active,
                                        uint8_t alert_type,
                                        uint8_t alert_severity,
                                        uint16_t alert_duration_s,
                                        uint8_t recommendation_code,
                                        uint8_t recommendation_priority,
                                        uint8_t sacrum_avg,
                                        uint8_t sacrum_peak,
                                        uint8_t heel_left_avg,
                                        uint8_t heel_right_avg,
                                        uint8_t shoulders_avg,
                                        uint8_t shoulders_peak,
                                        uint8_t pressure_exposure_threshold,
                                        uint16_t sacrum_exposure_s,
                                        uint16_t heels_exposure_s,
                                        uint16_t shoulders_exposure_s,
										uint8_t zones_valid_mask,
										uint8_t summary_flags);
/*----------------------------------------------------------------------------*/
extern QueueHandle_t qSerialTx;

/*----------------------------------------------------------------------------*/

void SerialLink_Init(void);
BaseType_t SerialLink_SendNode32_Async(uint8_t node, uint16_t cycle, uint8_t flags, const uint8_t s32[32]);
void SerialLink_TxTask(void *argument);
// === UI RX task ===
// دریافت approve/reject و commandهای آینده از UI
void SerialLink_RxTask(void *argument);
uint32_t SerialLink_TxDropped(void);
BaseType_t SerialLink_SendInterventionPlan_Async(const TherapyPlan_t *plan);
/*----------------------------------------------------------------------------*/


#endif /* INC_SERIAL_LINK_H_ */
