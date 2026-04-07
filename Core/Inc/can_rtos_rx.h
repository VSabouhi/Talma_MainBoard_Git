
#ifndef INC_CAN_RTOS_RX_H_
#define INC_CAN_RTOS_RX_H_
/*----------------------------------------------------------*/

#pragma once
#include "stm32f7xx_hal.h"
#include "FreeRTOS.h"
#include "queue.h"
#include <stdint.h>
/*----------------------------------------------------------*/

typedef struct {
  CAN_RxHeaderTypeDef h;
  uint8_t d[8];
} CanRxMsg;
/*----------------------------------------------------------*/

void CanRtosRx_Init(void);                 // ساخت Queue
void CanRtosRx_OnFifo0Pending(CAN_HandleTypeDef *hcan); // از ISR صدا زده می‌شود
uint32_t CanRtosRx_Dropped(void);
/*----------------------------------------------------------*/

extern QueueHandle_t qCanRx;   // اگر CMSIS-RTOS2 استفاده می‌کنی، در استپ بعدی تبدیلش می‌کنیم
/*----------------------------------------------------------*/

/*----------------------------------------------------------*/

#endif /* INC_CAN_RTOS_RX_H_ */
