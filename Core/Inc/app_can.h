#ifndef INC_APP_CAN_H_
#define INC_APP_CAN_H_
/*----------------------------------------------------------------------------*/

#pragma once
#include "stm32f7xx_hal.h"
#include <stdint.h>
#include "FreeRTOS.h"
/*----------------------------------------------------------------------------*/
uint8_t AppCan_Read(CAN_RxHeaderTypeDef *h, uint8_t *d);
BaseType_t CanCmd_SendToNode(uint8_t node, uint8_t cmd,
                            uint8_t a0, uint8_t a1, uint8_t a2,
                            uint32_t param32);
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/


void AppCan_Init(void);       // تنظیم filter و آماده‌سازی
void AppCan_Start(void);      // Start CAN + RX interrupt
uint8_t AppCan_IsOk(void);    // وضعیت
/*----------------------------------------------------------------------------*/

#endif /* INC_APP_CAN_H_ */
