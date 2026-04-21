#include "Serial_link.h"
#include "uart_pkt.h"
#include <string.h>
#include "FreeRTOS.h"
#include "queue.h"
#include "bed_model.h"
#include "node_state.h"
#include  <stdio.h>

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
          // باید از بافر پایدار send استفاده شود، نه از بافر زنده
          UartPkt_SendBedSnapshot(m.payload.bed.bed_cycle, bed_value_send);
          break;
        case SL_MSG_TYPE_BED_STATUS:
          // === ارسال status پایدار تخت ===
          UartPkt_SendBedStatus(m.payload.status.bed_cycle, bed_status_send);
          break;
        case SL_MSG_TYPE_NODE_HEALTH:
          // === ارسال وضعیت همه نودها ===
          UartPkt_SendNodeHealth(m.payload.health.bed_cycle, node_state);
          break;

        case SL_MSG_TYPE_SUMMARY:
          // === ارسال summary packet برای UI ===
        	UartPkt_SendSummary(
				  m.payload.summary.frame_id,
				  m.payload.summary.uptime_s,
				  m.payload.summary.risk_score,
				  m.payload.summary.risk_level,
				  m.payload.summary.movement_detected,
				  m.payload.summary.time_since_last_movement_s,
				  m.payload.summary.alert_active,
				  m.payload.summary.alert_type,
				  m.payload.summary.alert_severity,
				  m.payload.summary.alert_duration_s,
				  m.payload.summary.recommendation_code,
				  m.payload.summary.recommendation_priority,
				  m.payload.summary.sacrum_avg,
				  m.payload.summary.sacrum_peak,
				  m.payload.summary.heel_left_avg,
				  m.payload.summary.heel_right_avg,
				  m.payload.summary.shoulders_avg,
				  m.payload.summary.shoulders_peak,
				  m.payload.summary.pressure_exposure_threshold,
				  m.payload.summary.sacrum_exposure_s,
				  m.payload.summary.heels_exposure_s,
				  m.payload.summary.shoulders_exposure_s,
				  m.payload.summary.zones_valid_mask,
				  m.payload.summary.summary_flags);

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
										uint8_t summary_flags)
{
  SL_Msg m;

  // === این پیام از نوع summary است ===
  m.type = SL_MSG_TYPE_SUMMARY;

  m.payload.summary.frame_id = frame_id;
  m.payload.summary.uptime_s = uptime_s;
  m.payload.summary.risk_score = risk_score;
  m.payload.summary.risk_level = risk_level;
  m.payload.summary.movement_detected = movement_detected;
  m.payload.summary.time_since_last_movement_s = time_since_last_movement_s;
  m.payload.summary.alert_active = alert_active;
  m.payload.summary.alert_type = alert_type;
  m.payload.summary.alert_severity = alert_severity;
  m.payload.summary.alert_duration_s = alert_duration_s;
  m.payload.summary.recommendation_code = recommendation_code;
  m.payload.summary.recommendation_priority = recommendation_priority;
  m.payload.summary.sacrum_avg = sacrum_avg;
  m.payload.summary.sacrum_peak = sacrum_peak;
  m.payload.summary.heel_left_avg = heel_left_avg;
  m.payload.summary.heel_right_avg = heel_right_avg;
  m.payload.summary.shoulders_avg = shoulders_avg;
  m.payload.summary.shoulders_peak = shoulders_peak;
  m.payload.summary.pressure_exposure_threshold = pressure_exposure_threshold;
  m.payload.summary.sacrum_exposure_s = sacrum_exposure_s;
  m.payload.summary.heels_exposure_s = heels_exposure_s;
  m.payload.summary.shoulders_exposure_s = shoulders_exposure_s;
  m.payload.summary.zones_valid_mask = zones_valid_mask;
  m.payload.summary.summary_flags = summary_flags;

  if (qSerialTx == NULL) return pdFAIL;

  if (xQueueSend(qSerialTx, &m, 0) != pdPASS) {
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

/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/

