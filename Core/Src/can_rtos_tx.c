#include "can_rtos_tx.h"
#include "FreeRTOS.h"
#include "task.h"
#include <string.h>
#include <stdio.h>
#include "main.h"

/*----------------------------------------------------------*/

/*----------------------------------------------------------*/
#define CAN_TX_QUEUE_LEN 64
/*----------------------------------------------------------*/
QueueHandle_t qCanTx = NULL;
/*----------------------------------------------------------*/

/*----------------------------------------------------------*/
static volatile uint32_t tx_dropped = 0;
static StaticQueue_t qCanTxCtrl;
static uint8_t qCanTxStorage[ CAN_TX_QUEUE_LEN * sizeof(CanTxMsg) ];
/*----------------------------------------------------------*/
void CanRtosTx_Init(void)
{
  qCanTx = xQueueCreateStatic(
      CAN_TX_QUEUE_LEN,
      sizeof(CanTxMsg),
      qCanTxStorage,
      &qCanTxCtrl
  );
  configASSERT(qCanTx != NULL);
}
/*----------------------------------------------------------*/
uint32_t CanRtosTx_Dropped(void) { return tx_dropped; }
/*----------------------------------------------------------*/
BaseType_t CanRtosTx_SendStd_Async(uint16_t stdId, const uint8_t data[8], uint8_t dlc)
{
  if (!qCanTx) return pdFAIL;
  if (dlc > 8U) dlc = 8U;

  CanTxMsg m;
  m.stdId = stdId;
  m.dlc   = dlc;
  if (data) memcpy(m.d, data, dlc);
  if (dlc < 8U) memset(&m.d[dlc], 0, 8U - dlc);

  if (xQueueSend(qCanTx, &m, 0) != pdPASS) {
    // drop oldest (مثل SerialLink)
    CanTxMsg dummy;
    xQueueReceive(qCanTx, &dummy, 0);
    if (xQueueSend(qCanTx, &m, 0) != pdPASS) {
      tx_dropped++;
      return pdFAIL;
    }
  }
  return pdPASS;
}
/*----------------------------------------------------------*/
void CanTxTask(void *argument)
{
  (void)argument;

  extern CAN_HandleTypeDef hcan1;

  CanTxMsg m;
  CAN_TxHeaderTypeDef th;
  uint32_t mailbox;

  th.IDE = CAN_ID_STD;
  th.RTR = CAN_RTR_DATA;

  for (;;)
  {

    if (xQueueReceive(qCanTx, &m, portMAX_DELAY) != pdPASS)
      continue;

    th.StdId = m.stdId;
    th.DLC   = m.dlc;


    while (HAL_CAN_GetTxMailboxesFreeLevel(&hcan1) == 0U) {
      vTaskDelay(pdMS_TO_TICKS(1));
    }

    //
    printf("CAN TX -> ID:0x%03X DLC:%u DATA:",
           (unsigned)m.stdId, (unsigned)m.dlc);
    for (uint8_t i = 0; i < m.dlc; i++)
      printf("%02X ", m.d[i]);
    printf("\r\n");

    // ارسال واقعی
    if (HAL_CAN_AddTxMessage(&hcan1, &th, m.d, &mailbox) != HAL_OK) {
      tx_dropped++;
      printf("CAN TX !! FAIL id=0x%03X err=%lu\r\n",
             (unsigned)m.stdId, (unsigned long)HAL_CAN_GetError(&hcan1));
    }
  }
}
/*----------------------------------------------------------*/

/*----------------------------------------------------------*/

/*----------------------------------------------------------*/

/*----------------------------------------------------------*/
