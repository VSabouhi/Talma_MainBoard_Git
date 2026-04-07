#include "Serial_link.h"
#include "uart_pkt.h"
#include <string.h>

/*----------------------------------------------------------------------------*/

QueueHandle_t qSerialTx = NULL;
static volatile uint32_t sl_tx_dropped = 0;
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
void SerialLink_Init(void)
{
  // 32 پیام در صف (قابل تغییر)
  qSerialTx = xQueueCreate(32, sizeof(SL_Node32Msg));
}
/*----------------------------------------------------------------------------*/
uint32_t SerialLink_TxDropped(void) { return sl_tx_dropped; }
/*----------------------------------------------------------------------------*/
BaseType_t SerialLink_SendNode32_Async(uint8_t node, uint16_t cycle, uint8_t flags, const uint8_t s32[32])
{
  SL_Node32Msg m;
  m.node = node;
  m.cycle = cycle;
  m.flags = flags;
  memcpy(m.s, s32, 32);

  if (qSerialTx == NULL) return pdFAIL;

  if (xQueueSend(qSerialTx, &m, 0) != pdPASS) {
	    SL_Node32Msg dummy;
	    xQueueReceive(qSerialTx, &dummy, 0);   // drop oldest
	    if (xQueueSend(qSerialTx, &m, 0) != pdPASS)
	        sl_tx_dropped++;
  }
  return pdPASS;
}
/*----------------------------------------------------------------------------*/
void SerialLink_TxTask(void *argument)
{
  SL_Node32Msg m;

  for (;;)
  {
    if (xQueueReceive(qSerialTx, &m, portMAX_DELAY) == pdPASS)
    {
      // اینجا ارسال واقعی UART انجام میشه
      UartPkt_SendNode32(m.node, m.cycle, m.flags, m.s);
    }
  }
}
/*----------------------------------------------------------------------------*/
