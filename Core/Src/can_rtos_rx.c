#include "can_rtos_rx.h"
#include "Freertos.h"
#include "queue.h"
#include <string.h>
#include <stdio.h>
#include "main.h"
/*----------------------------------------------------------*/
#define CAN_RX_QUEUE_LEN 128
/*----------------------------------------------------------*/

QueueHandle_t qCanRx = NULL;
/*----------------------------------------------------------*/

static volatile uint32_t dropped = 0;
static StaticQueue_t qCanRxCtrl;
static uint8_t qCanRxStorage[ CAN_RX_QUEUE_LEN * sizeof(CanRxMsg) ];
/*----------------------------------------------------------*/

void CanRtosRx_Init(void)
{
	  qCanRx = xQueueCreateStatic(
	      CAN_RX_QUEUE_LEN,
	      sizeof(CanRxMsg),
	      qCanRxStorage,
	      &qCanRxCtrl
	  );

	  printf("qCanRx=%p (static) storage=%u bytes\r\n", qCanRx, (unsigned)(CAN_RX_QUEUE_LEN * sizeof(CanRxMsg)));

	  configASSERT(qCanRx != NULL);
}
/*----------------------------------------------------------*/

uint32_t CanRtosRx_Dropped(void){ return dropped; }
/*----------------------------------------------------------*/

/*void CanRtosRx_OnFifo0Pending(CAN_HandleTypeDef *hcan)
{
  BaseType_t hpw = pdFALSE;
  CanRxMsg m;

  while (HAL_CAN_GetRxFifoFillLevel(hcan, CAN_RX_FIFO0) > 0)
  {
    if (HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &m.h, m.d) != HAL_OK) break;

    if (qCanRx)
    {
      if (xQueueSendFromISR(qCanRx, &m, &hpw) != pdPASS)
        dropped++;
    }
  }

  portYIELD_FROM_ISR(hpw);
}*/


void CanRtosRx_OnFifo0Pending(CAN_HandleTypeDef *hcan)
{
  BaseType_t hpw = pdFALSE;
  CanRxMsg m;

  while (HAL_CAN_GetRxFifoFillLevel(hcan, CAN_RX_FIFO0) > 0)
  {
    if (HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &m.h, m.d) != HAL_OK)
      break;

    HAL_GPIO_TogglePin(LED2_GPIO_Port, LED2_Pin);

    if (qCanRx)
    {
      BaseType_t ok = xQueueSendFromISR(qCanRx, &m, &hpw);
      if (ok != pdPASS)
      {
        dropped++;
      }
    }
    else
    {
      dropped++;
    }
  }

  portYIELD_FROM_ISR(hpw);
}
/*----------------------------------------------------------*/

/*----------------------------------------------------------*/

/*----------------------------------------------------------*/

/*----------------------------------------------------------*/
