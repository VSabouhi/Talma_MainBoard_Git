#include "Serial_link.h"
#include "uart_pkt.h"
#include <string.h>
#include "FreeRTOS.h"
#include "queue.h"
#include "bed_model.h"
#include "node_state.h"
#include  <stdio.h>
#include "usart.h"
#include "cmsis_os.h"
#include "app_intervention.h"
#include "therapy_engine.h"
/*----------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------*/

QueueHandle_t qSerialTx = NULL;
static volatile uint32_t sl_tx_dropped = 0;
/* --------------------------------------------------------------------------
 * Fake UI intervention state.
 *
 * Used only for UI integration test:
 *   0x5A debug command -> fake 0x51 plan
 *   0x52 approve       -> fake 0x54 EXECUTING / COMPLETED
 *   0x53 reject        -> fake 0x54 REJECTED
 *
 * This does NOT run real motor scheduler.
 * -------------------------------------------------------------------------- */
static volatile uint8_t  g_fake_ui_plan_pending = 0U;
static volatile uint32_t g_fake_ui_plan_id = 0U;
/*----------------------------------------------------------------------------*/
// === serial TX queue configuration ===
#define SERIAL_TX_QUEUE_LEN  32
// === static queue storage (avoid heap usage) ===
static StaticQueue_t qSerialTxCtrl;
// === حافظه صف UART TX بر اساس پیام generic ===
static uint8_t qSerialTxStorage[SERIAL_TX_QUEUE_LEN * sizeof(SL_Msg)];
/* --------------------------------------------------------------------------
 * Serial stream pause flag
 *
 * Used by UI debug command to temporarily stop periodic/large packets
 * before sending fake intervention plan.
 * -------------------------------------------------------------------------- */
static volatile uint8_t g_serial_stream_paused = 0U;
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
  /* --------------------------------------------------------------------------
   * If stream is paused, skip normal periodic packets.
   * Debug/control packets must still be allowed.
   * -------------------------------------------------------------------------- */


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

  /* --------------------------------------------------------------------------
   * If stream is paused, skip normal periodic packets.
   * Debug/control packets must still be allowed.
   * -------------------------------------------------------------------------- */


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

  /* --------------------------------------------------------------------------
   * If stream is paused, skip normal periodic packets.
   * Debug/control packets must still be allowed.
   * -------------------------------------------------------------------------- */


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
        case SL_MSG_TYPE_INTERVENTION_PLAN:

          printf("SERIAL LINK: send intervention plan id=%lu motors=%u\r\n",
                 (unsigned long)m.payload.intervention.plan.plan_id,
                 (unsigned)m.payload.intervention.plan.motor_count);

          UartPkt_SendInterventionPlan(&m.payload.intervention.plan);

          break;

        case SL_MSG_TYPE_FAKE_UI_PLAN:
        {
          /* --------------------------------------------------------------------------
           * Send fake/test UI PLAN packet.
           *
           * This runs only inside SerialLink_TxTask, so packet ordering is safe:
           *   previous packet ... 12 34
           *   AA 55 51 ...
           *   next packet AA 55 ...
           * -------------------------------------------------------------------------- */

          uint8_t tx[15];

          tx[0]  = PKT_SOF0;
          tx[1]  = PKT_SOF1;
          tx[2]  = 0x51U;
          tx[3]  = 0x00U;

          tx[4]  = 0x01U;
          tx[5]  = 0x00U;
          tx[6]  = 0x02U;
          tx[7]  = 0x03U;
          tx[8]  = 0x0CU;
          tx[9]  = 0x80U;
          tx[10] = 0x03U;
          tx[11] = 0x01U;
          tx[12] = 0x02U;

          tx[13] = PKT_CRC0;
          tx[14] = PKT_CRC1;

          HAL_UART_Transmit(&huart4, tx, sizeof(tx), 100);

          printf("UI DEBUG TX: fake 0x51 plan sent\r\n");

          char raw_log[128];

          snprintf(raw_log,
                   sizeof(raw_log),
                   "raw = "
                   "%02X %02X %02X %02X %02X "
                   "%02X %02X %02X %02X %02X "
                   "%02X %02X %02X %02X %02X\r\n",
                   tx[0], tx[1], tx[2], tx[3], tx[4],
                   tx[5], tx[6], tx[7], tx[8], tx[9],
                   tx[10], tx[11], tx[12], tx[13], tx[14]);

          printf("%s", raw_log);

          break;
        }

        case SL_MSG_TYPE_INTERVENTION_RESULT:
          /* --------------------------------------------------------------------------
           * Send intervention lifecycle result to UI.
           * Packet type: 0x54
           * -------------------------------------------------------------------------- */
          UartPkt_SendInterventionResult(
              m.payload.intervention_result.plan_id,
              m.payload.intervention_result.state,
              m.payload.intervention_result.board_id,
              m.payload.intervention_result.motor_count);
          break;

        default:
          // === نوع پیام ناشناخته: فعلاً نادیده بگیر ===
          break;
      }

      // === UART TX pacing ===
      // UI command RX روی همین UART می‌آید.
      // بنابراین بعد از هر packet کمی فاصله می‌دهیم تا خط UART کاملاً اشباع نشود.
      switch (m.type)
      {
        case SL_MSG_TYPE_BED_SNAPSHOT:
        case SL_MSG_TYPE_BED_STATUS:
          // packetهای 522 بایتی سنگین هستند
          osDelay(20);
          break;

        case SL_MSG_TYPE_NODE32:
          // packet حدود 42 بایت است
          osDelay(3);
          break;

        case SL_MSG_TYPE_NODE_HEALTH:
        case SL_MSG_TYPE_SUMMARY:
        case SL_MSG_TYPE_INTERVENTION_PLAN:
        case SL_MSG_TYPE_INTERVENTION_RESULT:
        case SL_MSG_TYPE_FAKE_UI_PLAN:
          /* Short pacing only */
          osDelay(2);
          break;
          /* --------------------------------------------------------------------------
           * UART TX pacing only.
           *
           * IMPORTANT:
           * Do NOT send any packet here.
           * Actual packet transmission must happen only in the main switch above.
           * -------------------------------------------------------------------------- */
          osDelay(5);
          break;
      }
    }
  }
}
/*----------------------------------------------------------------------------*/
BaseType_t SerialLink_SendNodeHealth_Async(uint16_t bed_cycle)
{
  SL_Msg m;

  /* --------------------------------------------------------------------------
   * If stream is paused, skip normal periodic packets.
   * Debug/control packets must still be allowed.
   * -------------------------------------------------------------------------- */


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

  /* --------------------------------------------------------------------------
   * If stream is paused, skip normal periodic packets.
   * Debug/control packets must still be allowed.
   * -------------------------------------------------------------------------- */


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
BaseType_t SerialLink_SendInterventionPlan_Async(const TherapyPlan_t *plan)
{
  if (plan == 0)
    return pdFAIL;

  if (plan->valid == 0U)
    return pdFAIL;

  SL_Msg m;

  // === این پیام از نوع intervention plan است ===
  // plan کامل کپی می‌شود تا در queue پایدار بماند.
  m.type = SL_MSG_TYPE_INTERVENTION_PLAN;
  m.payload.intervention.plan = *plan;

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
BaseType_t SerialLink_SendInterventionResult_Async(uint32_t plan_id,
                                                   uint8_t state,
                                                   uint8_t board_id,
                                                   uint8_t motor_count)
{
  SL_Msg m;

  // === intervention result message ===
  // این پیام lifecycle اجرای intervention را به UI می‌فرستد.
  m.type = SL_MSG_TYPE_INTERVENTION_RESULT;

  m.payload.intervention_result.plan_id = plan_id;
  m.payload.intervention_result.state = state;
  m.payload.intervention_result.board_id = board_id;
  m.payload.intervention_result.motor_count = motor_count;

  if (qSerialTx == NULL)
    return pdFAIL;

  if (xQueueSend(qSerialTx, &m, 0) != pdPASS)
  {
    SL_Msg dummy;
    xQueueReceive(qSerialTx, &dummy, 0);

    if (xQueueSend(qSerialTx, &m, 0) != pdPASS)
    {
      sl_tx_dropped++;
      return pdFAIL;
    }
  }

  // DEBUG:
  // بررسی ارسال result lifecycle به UI.
  printf("SERIAL LINK: send intervention result id=%lu state=%u board=%u motors=%u\r\n",
         (unsigned long)plan_id,
         (unsigned)state,
         (unsigned)board_id,
         (unsigned)motor_count);

  return pdPASS;
}
/*----------------------------------------------------------------------------*//*----------------------------------------------------------------------------*/
// === SerialLink RX task ===
// دریافت packetهای approve/reject از UI روی همان UART لینک UI.
//
// Packet:
// AA 55 TYPE SEQ planL planH 12 34
//
// TYPE:
// 0x52 approve
// 0x53 reject
void SerialLink_RxTask(void *argument)
{
  (void)argument;

  uint8_t b = 0U;
  uint8_t pkt[8];
  uint8_t idx = 0U;
  uint32_t plan_id = 0U;

  printf("SerialLink_RxTask started\r\n");

  for (;;)
  {
	  /* --------------------------------------------------------------------------
	   * UI command RX is temporarily moved to UART3.
	   *
	   * UART4 remains for normal online data stream TX.
	   * UART3 is used only to receive UI commands during debug/testing.
	   * -------------------------------------------------------------------------- */
	  if (HAL_UART_Receive(&huart3, &b, 1U, 20) != HAL_OK)
    {
      osDelay(1);
      continue;
    }

    if (idx == 0U)
    {
      if (b != PKT_SOF0)
        continue;

      pkt[idx++] = b;
      continue;
    }

    if (idx == 1U)
    {
      if (b != PKT_SOF1)
      {
        idx = 0U;
        continue;
      }

      pkt[idx++] = b;
      continue;
    }

    pkt[idx++] = b;

    if (idx < 8U)
      continue;

    idx = 0U;

    /* ----------------------------------------------------------------------
     * DEBUG:
     * Print complete 8-byte UI command packet.
     * ---------------------------------------------------------------------- */
    printf("UI RX PKT: %02X %02X %02X %02X %02X %02X %02X %02X\r\n",
           pkt[0], pkt[1], pkt[2], pkt[3],
           pkt[4], pkt[5], pkt[6], pkt[7]);

    /* ----------------------------------------------------------------------
     * Footer check:
     * All current UI command packets are 8 bytes:
     *   AA 55 TYPE SEQ B4 B5 12 34
     * ---------------------------------------------------------------------- */
    if (pkt[6] != PKT_CRC0) continue;
    if (pkt[7] != PKT_CRC1) continue;

    /* ----------------------------------------------------------------------
     * Approve/Reject plan_id format:
     *   pkt[4] = plan_id low
     *   pkt[5] = plan_id high
     *
     * For DEBUG_COMMAND:
     *   pkt[4] = command_id
     *   pkt[5] = param
     * ---------------------------------------------------------------------- */
    plan_id =
        ((uint32_t)pkt[4]) |
        (((uint32_t)pkt[5]) << 8);

    if (pkt[2] == PKT_TYPE_INTERVENTION_APPROVE)
    {
      printf("UI CMD: approve id=%lu\r\n",
             (unsigned long)plan_id);



      /* --------------------------------------------------------------------------
       * Fake UI plan approve path.
       *
       * If the current plan was created by DEBUG_COMMAND, do NOT call real
       * AppIntervention_Approve(), because that expects a real TherapyEngine plan
       * and may trigger motor scheduling.
       * -------------------------------------------------------------------------- */
      if ((g_fake_ui_plan_pending != 0U) &&
          (g_fake_ui_plan_id == plan_id))
      {
        g_fake_ui_plan_pending = 0U;

        SerialLink_SendInterventionResult_Async(
            plan_id,
            APP_INTERVENTION_EXECUTING,
            2U,
            12U);

        SerialLink_SendInterventionResult_Async(
            plan_id,
            APP_INTERVENTION_COMPLETED,
            2U,
            12U);

        continue;
      }



      /* --------------------------------------------------------------------------
       * Fake UI approve path.
       *
       * If plan was created by DEBUG_COMMAND, answer UI with RESULT packets
       * without touching real TherapyEngine / MotorScheduler.
       * -------------------------------------------------------------------------- */
      if ((g_fake_ui_plan_pending != 0U) &&
          (g_fake_ui_plan_id == plan_id))
      {
        g_fake_ui_plan_pending = 0U;

        /* state = 1 : EXECUTING */
        SerialLink_SendInterventionResult_Async(
            plan_id,
            APP_INTERVENTION_EXECUTING,
            2U,
            12U);

        /* state = 2 : COMPLETED */
        SerialLink_SendInterventionResult_Async(
            plan_id,
            APP_INTERVENTION_COMPLETED,
            2U,
            12U);

        continue;
      }
      /* --------------------------------------------------------------------
       * Execute only after UI approval.
       * Operator/nurse approval enters the system from UI path.
       * -------------------------------------------------------------------- */
      AppIntervention_Approve(plan_id);
    }
    else if (pkt[2] == PKT_TYPE_INTERVENTION_REJECT)
    {
      printf("UI CMD: reject id=%lu\r\n",
             (unsigned long)plan_id);



      /* --------------------------------------------------------------------------
       * Fake UI plan reject path.
       * -------------------------------------------------------------------------- */
      if ((g_fake_ui_plan_pending != 0U) &&
          (g_fake_ui_plan_id == plan_id))
      {
        g_fake_ui_plan_pending = 0U;

        SerialLink_SendInterventionResult_Async(
            plan_id,
            APP_INTERVENTION_REJECTED,
            2U,
            12U);

        continue;
      }


      /* --------------------------------------------------------------------------
       * Fake UI reject path.
       *
       * If plan was created by DEBUG_COMMAND, answer UI with REJECTED result
       * without touching real TherapyEngine / MotorScheduler.
       * -------------------------------------------------------------------------- */
      if ((g_fake_ui_plan_pending != 0U) &&
          (g_fake_ui_plan_id == plan_id))
      {
        g_fake_ui_plan_pending = 0U;

        /* state = 4 : REJECTED */
        SerialLink_SendInterventionResult_Async(
            plan_id,
            APP_INTERVENTION_REJECTED,
            2U,
            12U);

        continue;
      }
      /* --------------------------------------------------------------------
       * Reject pending intervention plan.
       *
       * RX:
       *   AA 55 53 SEQ planL planH 12 34
       * -------------------------------------------------------------------- */
      AppIntervention_Reject(plan_id);
    }
    else if (pkt[2] == PKT_TYPE_DEBUG_COMMAND)
    {
      uint8_t command_id = pkt[4];
      uint8_t param = pkt[5];

      printf("UI CMD: debug cmd=%u param=%u\r\n",
             (unsigned)command_id,
             (unsigned)param);

      if (command_id == 1U)
      {



    	  /* --------------------------------------------------------------------------
    	   * Mark fake plan as pending so approve/reject can be tested by UI.
    	   * param is the fake plan_id.
    	   * -------------------------------------------------------------------------- */
    	  g_fake_ui_plan_pending = 1U;
    	  g_fake_ui_plan_id = (uint32_t)param;
    	  /* --------------------------------------------------------------------------
    	   * CMD 1:
    	   * Queue fake UI PLAN with priority.
    	   *
    	   * IMPORTANT:
    	   * We do NOT transmit directly from RX task.
    	   * We also do NOT pause the normal stream.
    	   *
    	   * xQueueSendToFront() makes fake 0x51 the next packet after the currently
    	   * transmitting packet is fully completed by SerialLink_TxTask.
    	   * -------------------------------------------------------------------------- */
    	  SL_Msg m;
    	  m.type = SL_MSG_TYPE_FAKE_UI_PLAN;

    	  if (qSerialTx != NULL)
    	  {
    	    if (xQueueSendToFront(qSerialTx, &m, 0) != pdPASS)
    	    {
    	      /* If queue is full, drop one old normal packet and retry once */
    	      SL_Msg dummy;
    	      xQueueReceive(qSerialTx, &dummy, 0);

    	      if (xQueueSendToFront(qSerialTx, &m, 0) != pdPASS)
    	      {
    	        sl_tx_dropped++;

    	        printf("UI DEBUG TX: fake 0x51 priority enqueue failed\r\n");
    	      }
    	    }
    	  }
    	  else
    	  {
    	    printf("UI DEBUG TX: qSerialTx is NULL\r\n");
    	  }

      }
    }
    else
    {
      printf("UI CMD: unknown type=0x%02X\r\n", pkt[2]);
    }
  }
}
/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/

