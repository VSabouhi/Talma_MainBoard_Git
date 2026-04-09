#ifndef INC_MOVEMENT_H_
#define INC_MOVEMENT_H_
/*--------------------------------------------------------------------------------*/

#include <stdint.h>
#include "main.h"
/*--------------------------------------------------------------------------------*/

// === movement result ===
typedef struct
{
  uint16_t energy;          // شدت تغییرات
  uint8_t  detected;        // 0/1
  uint32_t last_movement_ms;// آخرین حرکت
} MovementResult_t;

/*--------------------------------------------------------------------------------*/

// === init ===
void Movement_Init(void);

// === run detection ===
void Movement_Run(const uint8_t bed_value[BED_ROWS][BED_COLS],
                  const uint8_t bed_valid[BED_ROWS][BED_COLS],
                  MovementResult_t *res);
/*--------------------------------------------------------------------------------*/


/*--------------------------------------------------------------------------------*/


/*--------------------------------------------------------------------------------*/

#endif
