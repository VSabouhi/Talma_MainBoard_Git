#include "Serial_link.h"
#include "uart_pkt.h"
#include <string.h>
#include "FreeRTOS.h"
#include "queue.h"
#include "bed_model.h"
#include "node_state.h"
/*----------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------*/

QueueHandle_t qSerialTx = NULL;
static volatile uint32_t sl_tx_dropped = 0;
/*----------------------------------------------------------------------------*/
// === serial TX queue configuration ===
#define SERIAL_TX_QUEUE_LEN  32
// === static queue storage (avoid heap usage) ===
static StaticQueue_t qSerialTxCtrl;
// === حافظه صف UART TX بر اساس پیام generic ===
static uint8_t qSerialTxStorage[SERIAL_TX_QUEUE_LEN * sizeof(SL_Msg)];
/*----------------------------------------------------------------------------*/
void SerialLink_Init(void)
{
  // === create UART TX queue using static allocation ===
	qSerialTx = xQueueCreateStatic(
	    SERIAL_TX_QUEUE_LEN,
	    sizeof(SL_Msg),
	    qSerialTxStorage,
	    &qSerialTxCtrl
	);

  configASSERT(qSerialTx != NULL);
}
/*----------------------------------------------------------------------------*/
uint32_t SerialLink_TxDropped(void) { return sl_tx_dropped; }
/*----------------------------------------------------------------------------*/
BaseType_t SerialLink_SendBedStatus_Async(uint16_t bed_cycle)
{
  SL_Msg m;

  // === این پیام از نوع bed status است ===
  m.type = SL_MSG_TYPE_BED_STATUS;

  // === فقط شماره cycle داخل صف قرار می‌گیرد ===
  // خود داده‌های status هنگام ارسال از bed_model خوانده می‌شوند
  m.payload.status.bed_cycle = bed_cycle;

  if (qSerialTx == NULL) return pdFAIL;

  if (xQueueSend(qSerialTx, &m, 0) != pdPASS) {

    // === اگر صف پر بود: قدیمی‌ترین پیام حذف شود و یک بار retry شود ===
    SL_Msg dummy;
    xQueueReceive(qSerialTx, &dummy, 0);

    if (xQueueSend(qSerialTx, &m, 0) != pdPASS) {
      sl_tx_dropped++;
      return pdFAIL;
    }
  }

  return pdPASS;
}
/*----------------------------------------------------------------------------*/

BaseType_t SerialLink_SendBedSnapshot_Async(uint16_t bed_cycle)
{
  SL_Msg m;

  // === این پیام از نوع snapshot کامل تخت است ===
  m.type = SL_MSG_TYPE_BED_SNAPSHOT;

  // === فقط شماره cycle را داخل queue می‌گذاریم ===
  // خود داده‌های تخت موقع ارسال واقعی از bed_model خوانده می‌شوند
  m.payload.bed.bed_cycle = bed_cycle;

  if (qSerialTx == NULL) return pdFAIL;

  if (xQueueSend(qSerialTx, &m, 0) != pdPASS) {

    // === اگر صف پر بود: قدیمی‌ترین پیام حذف شود و یک بار retry شود ===
    SL_Msg dummy;
    xQueueReceive(qSerialTx, &dummy, 0);

    if (xQueueSend(qSerialTx, &m, 0) != pdPASS) {
      sl_tx_dropped++;
      return pdFAIL;
    }
  }

  return pdPASS;
}
/*----------------------------------------------------------------------------*/
BaseType_t SerialLink_SendNode32_Async(uint8_t node, uint16_t cycle, uint8_t flags, const uint8_t s32[32])
{
  SL_Msg m;

  // === این پیام از نوع node32 است ===
  m.type = SL_MSG_TYPE_NODE32;

  // === پر کردن payload مربوط به node ===
  m.payload.node32.node = node;
  m.payload.node32.cycle = cycle;
  m.payload.node32.flags = flags;
  memcpy(m.payload.node32.s, s32, 32);

  if (qSerialTx == NULL) return pdFAIL;

  if (xQueueSend(qSerialTx, &m, 0) != pdPASS) {

    // === اگر صف پر بود: قدیمی‌ترین پیام حذف شود و یک بار retry شود ===
    SL_Msg dummy;
    xQueueReceive(qSerialTx, &dummy, 0);

    if (xQueueSend(qSerialTx, &m, 0) != pdPASS) {
      sl_tx_dropped++;
      return pdFAIL;
    }
  }

  return pdPASS;
}

/*----------------------------------------------------------------------------*/


void SerialLink_TxTask(void *argument)
{
  (void)argument;

  SL_Msg m;

  for (;;)
  {
    if (xQueueReceive(qSerialTx, &m, portMAX_DELAY) == pdPASS)
    {
      switch (m.type)
      {
        case SL_MSG_TYPE_NODE32:
          // === ارسال packet مربوط به یک نود ===
          UartPkt_SendNode32(m.payload.node32.node,
                             m.payload.node32.cycle,
                             m.payload.node32.flags,
                             m.payload.node32.s);
          break;

        case SL_MSG_TYPE_BED_SNAPSHOT:
          // === ارسال snapshot کامل تخت ===
          // داده‌های واقعی تخت از bed_model خوانده می‌شوند
          UartPkt_SendBedSnapshot(m.payload.bed.bed_cycle, bed_value);
          break;
        case SL_MSG_TYPE_BED_STATUS:
          // === ارسال نقشه وضعیت کل تخت ===
          // داده واقعی از bed_model خوانده می‌شود
          UartPkt_SendBedStatus(m.payload.status.bed_cycle, bed_status);
          break;
        case SL_MSG_TYPE_NODE_HEALTH:
          // === ارسال وضعیت همه نودها ===
          UartPkt_SendNodeHealth(m.payload.health.bed_cycle, node_state);
          break;

        default:
          // === نوع پیام ناشناخته: فعلاً نادیده بگیر ===
          break;
      }
    }
  }
}
/*----------------------------------------------------------------------------*/
BaseType_t SerialLink_SendNodeHealth_Async(uint16_t bed_cycle)
{
  SL_Msg m;

  // === این پیام از نوع node health است ===
  m.type = SL_MSG_TYPE_NODE_HEALTH;

  // === فقط شماره cycle داخل صف قرار می‌گیرد ===
  // داده واقعی node_state موقع ارسال از node_state خوانده می‌شود
  m.payload.health.bed_cycle = bed_cycle;

  if (qSerialTx == NULL) return pdFAIL;

  if (xQueueSend(qSerialTx, &m, 0) != pdPASS) {

    // === اگر صف پر بود: قدیمی‌ترین پیام حذف شود و یک بار retry شود ===
    SL_Msg dummy;
    xQueueReceive(qSerialTx, &dummy, 0);

    if (xQueueSend(qSerialTx, &m, 0) != pdPASS) {
      sl_tx_dropped++;
      return pdFAIL;
    }
  }

  return pdPASS;
}
/*----------------------------------------------------------------------------*/
