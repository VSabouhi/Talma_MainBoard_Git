#ifndef INC_SERIAL_LINK_H_
#define INC_SERIAL_LINK_H_
/*----------------------------------------------------------------------------*/

#include <stdint.h>
#include "FreeRTOS.h"
#include "queue.h"
/*----------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------*/

typedef struct {
  uint8_t node;
  uint16_t cycle;
  uint8_t flags;
  uint8_t s[32];
} SL_Node32Msg;
/*----------------------------------------------------------------------------*/
extern QueueHandle_t qSerialTx;

/*----------------------------------------------------------------------------*/

void SerialLink_Init(void);
BaseType_t SerialLink_SendNode32_Async(uint8_t node, uint16_t cycle, uint8_t flags, const uint8_t s32[32]);
void SerialLink_TxTask(void *argument);
uint32_t SerialLink_TxDropped(void);
/*----------------------------------------------------------------------------*/


#endif /* INC_SERIAL_LINK_H_ */
