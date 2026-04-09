
#include "uart_pkt.h"
#include "usart.h"
#include <string.h>
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
static uint8_t g_seq = 0;
/*----------------------------------------------------------------------------*/
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
  uint8_t pkt[42];

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


  HAL_UART_Transmit(&huart4, pkt, sizeof(pkt), 100);
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

  uint8_t pkt[522];
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

  HAL_UART_Transmit(&huart4, pkt, sizeof(pkt), 200);
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

  uint8_t pkt[522];
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

  HAL_UART_Transmit(&huart4, pkt, sizeof(pkt), 200);
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

  uint8_t pkt[26];

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

  HAL_UART_Transmit(&huart4, pkt, sizeof(pkt), 100);
}
/*----------------------------------------------------------------------------*/
