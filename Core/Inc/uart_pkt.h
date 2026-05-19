
#ifndef INC_UART_PKT_H_
#define INC_UART_PKT_H_
/*----------------------------------------------------------------------------*/
#include "main.h"
#include <stdint.h>
#include "therapy_engine.h"
/*----------------------------------------------------------------------------*/
/*  PKT_TYPE_SUMMARY = 0x40
[0]  SOF0
[1]  SOF1
[2]  TYPE = 0x40
[3]  SEQ
[4]  frame_id L
[5]  frame_id H
[6]  uptime_s L
[7]  uptime_s H
[8]  risk_score
[9]  risk_level
[10] movement_detected
[11] time_since_last_movement_s L
[12] time_since_last_movement_s H
[13] alert_active
[14] alert_type
[15] alert_severity
[16] alert_duration_s L
[17] alert_duration_s H
[18] recommendation_code
[19] recommendation_priority
[20] sacrum_avg
[21] sacrum_peak
[22] heel_left_avg
[23] heel_right_avg
[24] shoulders_avg
[25] shoulders_peak
[26] pressure_exposure_threshold
[27] sacrum_exposure_s L
[28] sacrum_exposure_s H
[29] heels_exposure_s L
[30] heels_exposure_s H
[31] shoulders_exposure_s L
[32] shoulders_exposure_s H
[33] zones_valid_mask
[34] summary_flags
[35] CRC0
[36] CRC1
*/
/*----------------------------------------------------------------------------*/
#define PKT_SOF0        0xAA
#define PKT_SOF1        0x55
#define PKT_TYPE_NODE32 0x10
#define PKT_CRC0        0x12   // فعلاً ثابت
#define PKT_CRC1        0x34   // فعلاً ثابت


/*----------------------------------------------------------------------------*/
#define PKT_TYPE_BED_SNAPSHOT  0x20   // packet جدید برای کل تخت
#define PKT_TYPE_BED_STATUS    0x22   //
#define PKT_TYPE_NODE_HEALTH   0x30   // وضعیت همه نودهانقشه وضعیت کل تخت
#define PKT_TYPE_SUMMARY       0x40   // summary packet for UI
#define PKT_HAS_FRAME_ID   1
// === intervention planner packet ===
// پیشنهاد intervention برای UI؛ اجرای موتور فقط بعد از approval مجاز است.
#define PKT_TYPE_INTERVENTION_PLAN   0x50
// === UI -> Main intervention control ===
// UI/پرستار تصمیم نهایی اجرای plan را می‌گیرد.
#define PKT_TYPE_INTERVENTION_APPROVE   0x52
#define PKT_TYPE_INTERVENTION_REJECT    0x53
// === Main -> UI intervention lifecycle result ===
// نتیجه اجرای intervention بعد از approve/reject
#define PKT_TYPE_INTERVENTION_RESULT    0x54
/*----------------------------------------------------------------------------*/
// === ارسال snapshot کامل تخت (32×16) ===
void UartPkt_SendBedSnapshot(uint16_t frame_id,
                              const uint8_t bed_value[BED_ROWS][BED_COLS]);

void UartPkt_SendBedStatus(uint16_t frame_id,
                            const uint8_t bed_status[BED_ROWS][BED_COLS]);

void UartPkt_SendNodeHealth(uint16_t frame_id,
                            const uint8_t node_state[NODES]);

// === ارسال summary فشرده برای UI ===
// این packet خروجی سطح بالا و مستقل از UI است
void UartPkt_SendSummary(uint16_t frame_id,
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
void UartPkt_Init(void);
void UartTx_Init(void);
void UartPkt_SendNode32(uint8_t nodeId, uint16_t cycle, uint8_t flags, const uint8_t sensors32[32]);   // HEX Data
/*----------------------------------------------------------------------------*/
// === ارسال نقشه وضعیت کل تخت ===
// هر خانه: status واقعی سنسور (0..3)
void UartPkt_SendBedStatus(uint16_t bed_cycle,
                           const uint8_t bed_status[BED_ROWS][BED_COLS]);
/*----------------------------------------------------------------------------*/
// === ارسال وضعیت همه نودها به UI ===
// هر بایت: state یک نود (ONLINE / STALE / OFFLINE)
void UartPkt_SendNodeHealth(uint16_t bed_cycle,
                            const uint8_t node_state[NODES]);
/*----------------------------------------------------------------------------*/
// === intervention result packet ===
// ارسال وضعیت lifecycle intervention به UI.
void UartPkt_SendInterventionResult(uint32_t plan_id,
                                    uint8_t state,
                                    uint8_t board_id,
                                    uint8_t motor_count);
/*--------------------------------------------------------------------------------*/


//void UartPkt_SendNode32(uint8_t node,uint16_t cycle,uint8_t flags,uint8_t *data);    // ASCI Data
/*----------------------------------------------------------------------------*/
void UartPkt_Init(void);
/*----------------------------------------------------------------------------*/
// === ارسال plan پیشنهادی به UI ===
// این packet فقط proposal است و هیچ motor command اجرا نمی‌کند.
void UartPkt_SendInterventionPlan(const TherapyPlan_t *p);
/*----------------------------------------------------------------------------*/
uint8_t UartPkt_ParseInterventionApprove(const uint8_t *pkt,
                                         uint16_t len,
                                         uint32_t *plan_id);

uint8_t UartPkt_ParseInterventionReject(const uint8_t *pkt,
                                        uint16_t len,
                                        uint32_t *plan_id);
/*----------------------------------------------------------------------------*/

#endif /* INC_UART_PKT_H_ */
