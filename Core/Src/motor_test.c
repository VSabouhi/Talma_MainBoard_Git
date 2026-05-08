#include "motor_test.h"
#include "motor_scheduler.h"

#include "cmsis_os.h"
#include <stdio.h>

// === HARDCODED MOTOR TESTS ===
// این task فقط برای bring-up موتور است.
// بعد از تست موفق، disable شود و therapy_engine از motor_scheduler API استفاده کند.

#define MOTOR_TEST_BOARD_ID   1U

void MotorTest_Task(void *argument)
{
  (void)argument;

  // کمی صبر برای بالا آمدن CAN و Node
  osDelay(2000);

  printf("MOTOR TEST: start\r\n");

  // 1) Node 1 motor 0 +500
  printf("MOTOR TEST: motor0 +500\r\n");
  MotorScheduler_EnqueueSingleMove(MOTOR_TEST_BOARD_ID, 0U, 500);
  osDelay(1000);

  // 2) home motor 0
  printf("MOTOR TEST: home motor0\r\n");
  MotorScheduler_EnqueueHomeOne(MOTOR_TEST_BOARD_ID, 0U);
  osDelay(1000);

  // 3) motors 0,1,2 +300
  printf("MOTOR TEST: mask motors 0,1,2 +300\r\n");
  MotorScheduler_EnqueueMaskMove(MOTOR_TEST_BOARD_ID,
                                 (1UL << 0) | (1UL << 1) | (1UL << 2),
                                 300);
  osDelay(1000);

  // 4) home all
  printf("MOTOR TEST: home all\r\n");
  MotorScheduler_EnqueueHomeAll(MOTOR_TEST_BOARD_ID);
  osDelay(1000);

  // 5) vector move: motor0=+100, motor1=+200, motor2=+150
  printf("MOTOR TEST: vector 0=100, 1=200, 2=150\r\n");

  MotorVectorItem_t v[3] = {
    { .idx = 0U, .delta = 100 },
    { .idx = 1U, .delta = 200 },
    { .idx = 2U, .delta = 150 },
  };

  MotorScheduler_EnqueueVectorMove(MOTOR_TEST_BOARD_ID, v, 3U);

  printf("MOTOR TEST: queued all tests\r\n");

  for (;;)
  {
    osDelay(1000);
  }
}
