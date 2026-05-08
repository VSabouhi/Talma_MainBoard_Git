
#include "uart_pkt.h"
#include "usart.h"
#include <string.h>
/*----------------------------------------------------------------------------*/
// === STATIC TX BUFFERS (to avoid stack overflow) ===

static uint8_t pkt_node32[42];
static uint8_t pkt_bed_snapshot[522];
static uint8_t pkt_bed_status[522];
static uint8_t pkt_node_health[26];
static uint8_t pkt_summary[37];
// === intervention plan packet buffer ===
// حداکثر 8 موتور × 3 بایت + header/status fields + CRC
static uint8_t pkt_intervention_plan[40];
/*----------------------------------------------------------------------------*/
static uint8_t g_seq = 0;
/*----------------------------------------------------------------------------*/
void UartPkt_Init(void)
{
  g_seq = 0;
}
/*----------------------------------------------------------------------------*/
// Packet = 42 bytes
// AA 55 10 seq node cycleL cycleH flags [32 bytes] 12 34
void UartPkt_SendNode32(uint8_t nodeId, uint16_t cycle, uint8_t flags, const uint8_t sensors32[32])
{
	uint8_t *pkt = pkt_node32;

  pkt[0] = PKT_SOF0;
  pkt[1] = PKT_SOF1;
  pkt[2] = PKT_TYPE_NODE32;
  pkt[3] = g_seq++;

  pkt[4] = nodeId;
  pkt[5] = (uint8_t)(cycle & 0xFF);
  pkt[6] = (uint8_t)((cycle >> 8) & 0xFF);
  pkt[7] = flags;

  memcpy(&pkt[8], sensors32, 32);

  pkt[40] = PKT_CRC0;
  pkt[41] = PKT_CRC1;


  HAL_UART_Transmit(&huart4, pkt, 42, 100);
}
/*----------------------------------------------------------------------------*/
/*void UartPkt_SendNode32(uint8_t node, uint16_t cycle, uint8_t flags,uint8_t *data)
{
    char buf[256];
    int len = 0;

    len += sprintf(&buf[len], "NODE=%u C=%u | ", node, cycle);

    for (int i = 0; i < 32; i++)
    {
        len += sprintf(&buf[len], "%u ", data[i]);
    }

    len += sprintf(&buf[len], "\r\n");

    HAL_UART_Transmit(&huart4, (uint8_t*)buf, len, 100);
}*/
/*----------------------------------------------------------------------------*/
// === ارسال کل تخت به UI ===
// شامل 512 بایت داده (32×16)
void UartPkt_SendBedSnapshot(uint16_t bed_cycle,
                             const uint8_t bed_value[BED_ROWS][BED_COLS])
{
  // ساختار packet:
  // [0] SOF0
  // [1] SOF1
  // [2] TYPE
  // [3] SEQ
  // [4] cycle L
  // [5] cycle H
  // [6] rows
  // [7] cols
  // [8..519] data (512 bytes)
  // [520] CRC0
  // [521] CRC1

	// NOTE:
	// bed_cycle در این مرحله نقش frame_id برای UI را دارد

  uint8_t *pkt = pkt_bed_snapshot;
  uint16_t k = 8;

  pkt[0] = PKT_SOF0;
  pkt[1] = PKT_SOF1;
  pkt[2] = PKT_TYPE_BED_SNAPSHOT;
  pkt[3] = g_seq++;

  pkt[4] = (uint8_t)(bed_cycle & 0xFF);
  pkt[5] = (uint8_t)((bed_cycle >> 8) & 0xFF);

  pkt[6] = BED_ROWS;
  pkt[7] = BED_COLS;

  // === کپی کل داده‌های تخت ===
  for (uint8_t r = 0; r < BED_ROWS; r++)
  {
    for (uint8_t c = 0; c < BED_COLS; c++)
    {
      pkt[k++] = bed_value[r][c];
    }
  }

  // === CRC موقت (فعلاً ثابت) ===
  pkt[k++] = PKT_CRC0;
  pkt[k++] = PKT_CRC1;

  HAL_UART_Transmit(&huart4, pkt, 522, 200);
}
/*----------------------------------------------------------------------------*/
// === ارسال نقشه وضعیت کل تخت به UI ===
// شامل 512 بایت (32×16) که هر خانه status همان سنسور است
void UartPkt_SendBedStatus(uint16_t bed_cycle,
                           const uint8_t bed_status[BED_ROWS][BED_COLS])
{
  // ساختار packet:
  // [0] SOF0
  // [1] SOF1
  // [2] TYPE = PKT_TYPE_BED_STATUS
  // [3] SEQ
  // [4] cycle L
  // [5] cycle H
  // [6] rows
  // [7] cols
  // [8..519] status map (512 bytes)
  // [520] CRC0
  // [521] CRC1

  uint8_t *pkt = pkt_bed_status;
  uint16_t k = 8;

  pkt[0] = PKT_SOF0;
  pkt[1] = PKT_SOF1;
  pkt[2] = PKT_TYPE_BED_STATUS;
  pkt[3] = g_seq++;

  pkt[4] = (uint8_t)(bed_cycle & 0xFF);
  pkt[5] = (uint8_t)((bed_cycle >> 8) & 0xFF);

  pkt[6] = BED_ROWS;
  pkt[7] = BED_COLS;

  // === کپی کل status map ===
  for (uint8_t r = 0; r < BED_ROWS; r++)
  {
    for (uint8_t c = 0; c < BED_COLS; c++)
    {
      pkt[k++] = bed_status[r][c];
    }
  }

  // === CRC موقت ===
  pkt[k++] = PKT_CRC0;
  pkt[k++] = PKT_CRC1;

  HAL_UART_Transmit(&huart4, pkt, 522, 200);
}
/*----------------------------------------------------------------------------*/
// === ارسال وضعیت همه نودها به UI ===
// برای هر نود یک بایت state ارسال می‌شود
void UartPkt_SendNodeHealth(uint16_t bed_cycle,
                            const uint8_t node_state[NODES])
{
  // ساختار packet:
  // [0] SOF0
  // [1] SOF1
  // [2] TYPE = PKT_TYPE_NODE_HEALTH
  // [3] SEQ
  // [4] cycle L
  // [5] cycle H
  // [6] node_count
  // [7] reserved
  // [8..23] state of 16 nodes
  // [24] CRC0 (فعلاً ثابت)
  // [25] CRC1 (فعلاً ثابت)

  uint8_t *pkt = pkt_node_health;

  pkt[0] = PKT_SOF0;
  pkt[1] = PKT_SOF1;
  pkt[2] = PKT_TYPE_NODE_HEALTH;
  pkt[3] = g_seq++;

  pkt[4] = (uint8_t)(bed_cycle & 0xFF);
  pkt[5] = (uint8_t)((bed_cycle >> 8) & 0xFF);

  pkt[6] = NODES;   // تعداد نودها
  pkt[7] = 0U;      // reserved for future use

  for (uint8_t i = 0; i < NODES; i++)
  {
    // state هر نود: OFFLINE / ONLINE / STALE
    pkt[8 + i] = node_state[i];
  }

  // === CRC موقت ===
  pkt[24] = PKT_CRC0;
  pkt[25] = PKT_CRC1;

  HAL_UART_Transmit(&huart4, pkt, 26, 100);
}
/*----------------------------------------------------------------------------*/
// === ارسال summary packet برای UI ===
// این packet فشرده است و مهم‌ترین متریک‌های بالادستی را یکجا می‌فرستد
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
						 uint8_t summary_flags)

{
  uint8_t *pkt = pkt_summary;

  pkt[0]  = PKT_SOF0;
  pkt[1]  = PKT_SOF1;
  pkt[2]  = PKT_TYPE_SUMMARY;
  pkt[3]  = g_seq++;

  pkt[4]  = (uint8_t)(frame_id & 0xFF);
  pkt[5]  = (uint8_t)((frame_id >> 8) & 0xFF);

  pkt[6]  = (uint8_t)(uptime_s & 0xFF);
  pkt[7]  = (uint8_t)((uptime_s >> 8) & 0xFF);

  pkt[8]  = risk_score;
  pkt[9]  = risk_level;
  pkt[10] = movement_detected;

  pkt[11] = (uint8_t)(time_since_last_movement_s & 0xFF);
  pkt[12] = (uint8_t)((time_since_last_movement_s >> 8) & 0xFF);

  pkt[13] = alert_active;
  pkt[14] = alert_type;
  pkt[15] = alert_severity;

  pkt[16] = (uint8_t)(alert_duration_s & 0xFF);
  pkt[17] = (uint8_t)((alert_duration_s >> 8) & 0xFF);

  pkt[18] = recommendation_code;
  pkt[19] = recommendation_priority;

  pkt[20] = sacrum_avg;
  pkt[21] = sacrum_peak;
  pkt[22] = heel_left_avg;
  pkt[23] = heel_right_avg;
  pkt[24] = shoulders_avg;
  pkt[25] = shoulders_peak;

  pkt[26] = pressure_exposure_threshold;

  pkt[27] = (uint8_t)(sacrum_exposure_s & 0xFF);
  pkt[28] = (uint8_t)((sacrum_exposure_s >> 8) & 0xFF);

  pkt[29] = (uint8_t)(heels_exposure_s & 0xFF);
  pkt[30] = (uint8_t)((heels_exposure_s >> 8) & 0xFF);

  pkt[31] = (uint8_t)(shoulders_exposure_s & 0xFF);
  pkt[32] = (uint8_t)((shoulders_exposure_s >> 8) & 0xFF);

  pkt[33] = zones_valid_mask;
  pkt[34] = summary_flags;

  pkt[35] = PKT_CRC0;
  pkt[36] = PKT_CRC1;



  HAL_UART_Transmit(&huart4, pkt, 37, 100);
}
/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
// === ارسال intervention plan به UI ===
// این packet فقط پیشنهاد است؛ اجرای موتور انجام نمی‌دهد.
//
// Packet 0x50:
// [0]  SOF0
// [1]  SOF1
// [2]  TYPE = 0x50
// [3]  SEQ
// [4]  plan_id L
// [5]  plan_id H
// [6]  status
// [7]  plan type
// [8]  target zone
// [9]  risk score
// [10] risk level
// [11] reason code
// [12] board id
// [13] motor count
// [14..] repeated: motor_idx, delta_L, delta_H
// [last-2] CRC0
// [last-1] CRC1
void UartPkt_SendInterventionPlan(const TherapyPlan_t *p)
{
  if (p == 0) return;
  if (p->valid == 0U) return;

  uint8_t *pkt = pkt_intervention_plan;
  uint16_t k = 0U;

  pkt[k++] = PKT_SOF0;
  pkt[k++] = PKT_SOF1;
  pkt[k++] = PKT_TYPE_INTERVENTION_PLAN;
  pkt[k++] = g_seq++;

  pkt[k++] = (uint8_t)(p->plan_id & 0xFFU);
  pkt[k++] = (uint8_t)((p->plan_id >> 8) & 0xFFU);

  pkt[k++] = p->status;
  pkt[k++] = p->type;
  pkt[k++] = p->target_zone;

  pkt[k++] = p->risk_score;
  pkt[k++] = p->risk_level;
  pkt[k++] = p->reason_code;

  pkt[k++] = p->board_id;
  pkt[k++] = p->motor_count;

  for (uint8_t i = 0U; i < p->motor_count; i++)
  {
    pkt[k++] = p->motors[i].idx;
    pkt[k++] = (uint8_t)((uint16_t)p->motors[i].delta & 0xFFU);
    pkt[k++] = (uint8_t)(((uint16_t)p->motors[i].delta >> 8) & 0xFFU);
  }

  pkt[k++] = PKT_CRC0;
  pkt[k++] = PKT_CRC1;

  // NOTE:
  // پروژه فعلی همه packetهای UI را روی huart4 می‌فرستد.
  // huart1 در این پروژه برای UI تعریف نشده است.
  HAL_UART_Transmit(&huart4, pkt, k, 100);
}
/*----------------------------------------------------------------------------*/
// === UI intervention approval packet ===
// Packet:
// AA 55 52 seq planL planH crc0 crc1
uint8_t UartPkt_ParseInterventionApprove(const uint8_t *pkt,
                                         uint16_t len,
                                         uint32_t *plan_id)
{
  if ((pkt == 0) || (plan_id == 0))
    return 0U;

  if (len < 8U)
    return 0U;

  if (pkt[0] != PKT_SOF0) return 0U;
  if (pkt[1] != PKT_SOF1) return 0U;

  if (pkt[2] != PKT_TYPE_INTERVENTION_APPROVE)
    return 0U;

  *plan_id =
      ((uint32_t)pkt[4]) |
      (((uint32_t)pkt[5]) << 8);

  return 1U;
}
/*----------------------------------------------------------------------------*/
// === UI intervention reject packet ===
// Packet:
// AA 55 53 seq planL planH crc0 crc1
uint8_t UartPkt_ParseInterventionReject(const uint8_t *pkt,
                                        uint16_t len,
                                        uint32_t *plan_id)
{
  if ((pkt == 0) || (plan_id == 0))
    return 0U;

  if (len < 8U)
    return 0U;

  if (pkt[0] != PKT_SOF0) return 0U;
  if (pkt[1] != PKT_SOF1) return 0U;

  if (pkt[2] != PKT_TYPE_INTERVENTION_REJECT)
    return 0U;

  *plan_id =
      ((uint32_t)pkt[4]) |
      (((uint32_t)pkt[5]) << 8);

  return 1U;
}
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
