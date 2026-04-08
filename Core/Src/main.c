/* USER CODE BEGIN Header */
/**   STM32_F767_CAN_4frame_main_1.0
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "cmsis_os.h"
#include "can.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "FreeRTOS.h"
#include "queue.h"
#include "can_rtos_rx.h"
#include "uart_pkt.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

uint8_t tes1;


volatile uint32_t rx_dropped = 0;
extern QueueHandle_t qCanRx;
// -------- Sensor Assembler --------
//static uint8_t sensors32[32];
//static uint8_t chunk_mask = 0;      // بیت0..3
//static uint32_t cycles_ok = 0;
uint32_t last_rx_ms = 0;
uint8_t can_ok = 0;
uint8_t  sensors32[NODES][32];
uint8_t  chunk_mask[NODES];
uint32_t cycles_ok[NODES];
uint32_t asm_start_ms[NODES];


// مقدار واقعی سنسور (0..63)
uint8_t  sensor_value[NODES][SENSOR_COUNT_PER_NODE];

// وضعیت سنسور (0..3)
uint8_t  sensor_status[NODES][SENSOR_COUNT_PER_NODE];

// === [NEW] sensor validity mask ===
// 1 = usable, 0 = invalid
uint8_t  valid_mask[NODES][SENSOR_COUNT_PER_NODE];

// === full bed model ===
// مدل رسمی کل تخت روی Main
// row = 0..31
// col = 0..15
uint8_t bed_value[BED_ROWS][BED_COLS];
uint8_t bed_status[BED_ROWS][BED_COLS];
uint8_t bed_valid[BED_ROWS][BED_COLS];
uint8_t bed_confidence[BED_ROWS][BED_COLS];


// ===  bed cycle sync state ===
// بیت هر نود نشان می‌دهد که در cycle جاری تخت آپدیت شده یا نه
uint32_t bed_sync_mask = 0U;


/*test 1*/

// شماره cycle سراسری تخت
uint32_t bed_cycle = 0U;

// اگر 1 شود یعنی یک snapshot کامل جدید از کل تخت آماده شده
uint8_t bed_snapshot_ready = 0U;
// sensor confidence level ===
// 0   = no trust
// 255 = full trust
uint8_t  sensor_confidence[NODES][SENSOR_COUNT_PER_NODE];

// node health/state tracking ===
// آخرین زمان دریافت هر فریم از هر نود
uint32_t node_last_frame_ms[NODES];

// آخرین زمان کامل شدن 4 chunk برای هر نود
uint32_t node_last_complete_ms[NODES];

// وضعیت فعلی هر نود: ONLINE / STALE / OFFLINE
uint8_t  node_state[NODES];
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MPU_Config(void);
void MX_FREERTOS_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */


/*----------------------------------------------------------------------------*/
int _write(int file, char *ptr, int len)
{
  HAL_UART_Transmit(&huart3, (uint8_t*)ptr, len, HAL_MAX_DELAY);
  return len;
}
/*----------------------------------------------------------------------------*/
static void CAN_Filter_0x100_to_0x1FF(void)
{
    CAN_FilterTypeDef f = {0};

    f.FilterBank = 0;
    f.FilterMode = CAN_FILTERMODE_IDMASK;
    f.FilterScale = CAN_FILTERSCALE_32BIT;
    f.FilterFIFOAssignment = CAN_RX_FIFO0;
    f.FilterActivation = ENABLE;
    f.SlaveStartFilterBank = 14;

    // StdID در رجیستر 32بیتی باید << 5 شود (مثل کاری که قبلاً کردی)
    // می‌خواهیم 0x100 تا 0x1FF را بگیریم:
    // ماسک: فقط بیت‌های بالایی مهم باشند => 0x700 (برای محدوده 0x100..0x1FF)
    // در قالب شیفت‌شده: (0x700 << 5)
    uint32_t id   = (0x100U << 5);
    uint32_t mask = (0x700U << 5);

    f.FilterIdHigh     = (uint16_t)(id >> 16);
    f.FilterIdLow      = (uint16_t)(id & 0xFFFF);
    f.FilterMaskIdHigh = (uint16_t)(mask >> 16);
    f.FilterMaskIdLow  = (uint16_t)(mask & 0xFFFF);

    if (HAL_CAN_ConfigFilter(&hcan1, &f) != HAL_OK)
        Error_Handler();

    //printf("Filter: accept StdID 0x100..0x1FF\r\n");
}


/*static void CAN_Filter_Node0_Node1(void)
{
  CAN_FilterTypeDef f = {0};

  f.FilterMode = CAN_FILTERMODE_IDLIST;
  f.FilterScale = CAN_FILTERSCALE_16BIT;
  f.FilterFIFOAssignment = CAN_RX_FIFO0;
  f.FilterActivation = ENABLE;
  f.SlaveStartFilterBank = 14;

  // -----------------------
  // Bank 0 → Node0
  // -----------------------
  f.FilterBank = 0;

  f.FilterIdHigh     = (0x100 << 5);                      // StdId = BASE_ID + (BOARD_ID<<2) + chunk
  f.FilterIdLow      = (0x101 << 5);
  f.FilterMaskIdHigh = (0x102 << 5);
  f.FilterMaskIdLow  = (0x103 << 5);

  if (HAL_CAN_ConfigFilter(&hcan1, &f) != HAL_OK)
      Error_Handler();

  // -----------------------
  // Bank 1 → Node1
  // -----------------------
  f.FilterBank = 1;

  f.FilterIdHigh     = (0x104 << 5);
  f.FilterIdLow      = (0x105 << 5);
  f.FilterMaskIdHigh = (0x106 << 5);
  f.FilterMaskIdLow  = (0x107 << 5);

  if (HAL_CAN_ConfigFilter(&hcan1, &f) != HAL_OK)
      Error_Handler();

  // -----------------------
  // Bank 1 → Node1
  // -----------------------
  f.FilterBank = 2;

  f.FilterIdHigh     = (0x108 << 5);
  f.FilterIdLow      = (0x109 << 5);
  f.FilterMaskIdHigh = (0x10A << 5);
  f.FilterMaskIdLow  = (0x10B << 5);

  if (HAL_CAN_ConfigFilter(&hcan1, &f) != HAL_OK)
      Error_Handler();



  printf("Filter set: Node0(0x100-0x103) & Node1(0x104-0x107)\r\n");
}*/
/*----------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------*/

/*static void CAN_StartWithIrq(void)
{
  if (HAL_CAN_Start(&hcan1) != HAL_OK)
    Error_Handler();

  if (HAL_CAN_ActivateNotification(&hcan1, CAN_IT_RX_FIFO0_MSG_PENDING) != HAL_OK)
    Error_Handler();
}*/

static void CAN_StartWithIrq(void)
{
    if (HAL_CAN_Start(&hcan1) == HAL_OK &&
        HAL_CAN_ActivateNotification(&hcan1, CAN_IT_RX_FIFO0_MSG_PENDING) == HAL_OK)
    {
        can_ok = 1;
        return;
    }

    can_ok = 0;
    // اینجا Error_Handler نرو؛ فقط گزارش بده
    //printf("CAN offline (no ACK?) err=0x%08lX\r\n", HAL_CAN_GetError(&hcan1));
}
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/

void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
	  CanRtosRx_OnFifo0Pending(hcan);
}
/*----------------------------------------------------------------------------*/



/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MPU Configuration--------------------------------------------------------*/
  MPU_Config();

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_CAN1_Init();
  MX_USART3_UART_Init();
  MX_UART4_Init();
  /* USER CODE BEGIN 2 */
  HAL_Delay(100);
  CAN_Filter_0x100_to_0x1FF();
 // CAN_FilterAcceptSensorRange();
  CAN_StartWithIrq();

  HAL_Delay(500);


  /* USER CODE END 2 */

  /* Init scheduler */
  osKernelInitialize();

  /* Call init function for freertos objects (in cmsis_os2.c) */
  MX_FREERTOS_Init();

  /* Start scheduler */
  osKernelStart();

  /* We should never get here as control is now taken by the scheduler */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure LSE Drive Capability
  */
  HAL_PWR_EnableBkUpAccess();

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI|RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 216;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 2;
  RCC_OscInitStruct.PLL.PLLR = 2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Activate the Over-Drive mode
  */
  if (HAL_PWREx_EnableOverDrive() != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_7) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/*----------------------------------------------------------------------------*/



/*----------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------*/

/* USER CODE END 4 */

 /* MPU Configuration */

void MPU_Config(void)
{
  MPU_Region_InitTypeDef MPU_InitStruct = {0};

  /* Disables the MPU */
  HAL_MPU_Disable();

  /** Initializes and configures the Region and the memory to be protected
  */
  MPU_InitStruct.Enable = MPU_REGION_ENABLE;
  MPU_InitStruct.Number = MPU_REGION_NUMBER0;
  MPU_InitStruct.BaseAddress = 0x0;
  MPU_InitStruct.Size = MPU_REGION_SIZE_4GB;
  MPU_InitStruct.SubRegionDisable = 0x87;
  MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
  MPU_InitStruct.AccessPermission = MPU_REGION_NO_ACCESS;
  MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_DISABLE;
  MPU_InitStruct.IsShareable = MPU_ACCESS_SHAREABLE;
  MPU_InitStruct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
  MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);
  /* Enables the MPU */
  HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);

}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
	  //  HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_0);
	    HAL_Delay(200);
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
