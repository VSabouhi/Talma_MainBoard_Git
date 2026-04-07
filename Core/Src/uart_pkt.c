
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
