
#ifndef INC_UART_PKT_H_
#define INC_UART_PKT_H_
/*----------------------------------------------------------------------------*/
#include "main.h"
#include <stdint.h>
/*----------------------------------------------------------------------------*/
/*  PKT_TYPE_SUMMARY = 0x40
[0]  SOF0
[1]  SOF1
[2]  TYPE = 0x40
[3]  SEQ
[4]  frame_id L
[5]  frame_id H
[6]  risk_score
[7]  risk_level
[8]  movement_detected
[9]  time_since_last_movement_s L
[10] time_since_last_movement_s H
[11] alert_active
[12] alert_type
[13] alert_severity
[14] alert_duration_s L
[15] alert_duration_s H
[16] recommendation_code
[17] recommendation_priority
[18] sacrum_avg
[19] sacrum_peak
[20] heel_left_avg
[21] heel_right_avg
[22] CRC0
[23] CRC1
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
                         uint8_t heel_right_avg);
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
//void UartPkt_SendNode32(uint8_t node,uint16_t cycle,uint8_t flags,uint8_t *data);    // ASCI Data
/*----------------------------------------------------------------------------*/
void UartPkt_Init(void);
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/


#endif /* INC_UART_PKT_H_ */
