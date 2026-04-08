#ifndef INC_SERIAL_LINK_H_
#define INC_SERIAL_LINK_H_
/*----------------------------------------------------------------------------*/

#include <stdint.h>
#include "FreeRTOS.h"
#include "queue.h"
/*----------------------------------------------------------------------------*/

// === serial link message types ===
// نوع پیام خروجی برای UI
#define SL_MSG_TYPE_NODE32         1U
#define SL_MSG_TYPE_BED_SNAPSHOT   2U
#define SL_MSG_TYPE_BED_STATUS     3U   //
#define SL_MSG_TYPE_NODE_HEALTH    4U   // وضعیت همه نودهاپیام نقشه وضعیت کل تخت
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
/*----------------------------------------------------------------------------*/
extern QueueHandle_t qSerialTx;

/*----------------------------------------------------------------------------*/

void SerialLink_Init(void);
BaseType_t SerialLink_SendNode32_Async(uint8_t node, uint16_t cycle, uint8_t flags, const uint8_t s32[32]);
void SerialLink_TxTask(void *argument);
uint32_t SerialLink_TxDropped(void);
/*----------------------------------------------------------------------------*/


#endif /* INC_SERIAL_LINK_H_ */
