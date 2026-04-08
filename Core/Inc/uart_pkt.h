
#ifndef INC_UART_PKT_H_
#define INC_UART_PKT_H_
/*----------------------------------------------------------------------------*/
#include "main.h"
#include <stdint.h>
/*----------------------------------------------------------------------------*/

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
/*----------------------------------------------------------------------------*/
// === ارسال snapshot کامل تخت (32×16) ===
void UartPkt_SendBedSnapshot(uint16_t bed_cycle, const uint8_t bed_value[BED_ROWS][BED_COLS]);
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
