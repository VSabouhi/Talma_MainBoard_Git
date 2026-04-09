#include "movement.h"
#include <string.h>
#include "stm32f7xx_hal.h"
/*--------------------------------------------------------------------------------*/

// === threshold ===
#define MOVEMENT_DIFF_THRESHOLD   3U
#define MOVEMENT_ENERGY_THRESHOLD 20U
/*--------------------------------------------------------------------------------*/

// === previous snapshot ===
static uint8_t prev_bed[BED_ROWS][BED_COLS];
static uint8_t initialized = 0;
/*--------------------------------------------------------------------------------*/

void Movement_Init(void)
{
  memset(prev_bed, 0, sizeof(prev_bed));
  initialized = 0;
}
/*--------------------------------------------------------------------------------*/
void Movement_Run(const uint8_t bed_value[BED_ROWS][BED_COLS],
                  const uint8_t bed_valid[BED_ROWS][BED_COLS],
                  MovementResult_t *res)
{
  if (res == 0) return;

  uint16_t energy = 0;

  // === first run ===
  if (!initialized)
  {
    memcpy(prev_bed, bed_value, sizeof(prev_bed));
    initialized = 1;
    res->energy = 0;
    res->detected = 0;
    return;
  }

  for (uint8_t r = 0; r < BED_ROWS; r++)
  {
    for (uint8_t c = 0; c < BED_COLS; c++)
    {
      if (bed_valid[r][c] == 0U)
        continue;

      uint8_t curr = bed_value[r][c];
      uint8_t prev = prev_bed[r][c];

      uint8_t diff = (curr > prev) ? (curr - prev) : (prev - curr);

      if (diff >= MOVEMENT_DIFF_THRESHOLD)
        energy += diff;
    }
  }

  res->energy = energy;

  if (energy >= MOVEMENT_ENERGY_THRESHOLD)
  {
    res->detected = 1;
    res->last_movement_ms = HAL_GetTick();
  }
  else
  {
    res->detected = 0;
  }

  // update previous snapshot
  memcpy(prev_bed, bed_value, sizeof(prev_bed));
}
/*--------------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------------*/
