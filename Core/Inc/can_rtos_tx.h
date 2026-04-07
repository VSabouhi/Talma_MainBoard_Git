#ifndef INC_CAN_RTOS_TX_H_
#define INC_CAN_RTOS_TX_H_
/*----------------------------------------------------------------------------*/
#include "stm32f7xx_hal.h"
#include "FreeRTOS.h"
#include "queue.h"
#include <stdint.h>
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
typedef struct {
  uint16_t stdId;   // Standard ID
  uint8_t  dlc;     // 0..8
  uint8_t  d[8];
} CanTxMsg;
/*----------------------------------------------------------------------------*/
extern QueueHandle_t qCanTx;
/*----------------------------------------------------------------------------*/
void CanRtosTx_Init(void);
BaseType_t CanRtosTx_SendStd_Async(uint16_t stdId, const uint8_t data[8], uint8_t dlc);
uint32_t CanRtosTx_Dropped(void);
void CanTxTask(void *argument); // Task that actually sends via HAL
/*----------------------------------------------------------------------------*/


#endif /* INC_CAN_RTOS_TX_H_ */
