/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f7xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "app_config.h"
/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define LED1_Pin GPIO_PIN_0
#define LED1_GPIO_Port GPIOB
#define LED2_Pin GPIO_PIN_7
#define LED2_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */
#define TARGET_BOARD_ID  0
#define BASE_ID          0x100

// ===  bed sync config ===
// ماسک نودهایی که باید برای تشکیل snapshot کامل تخت حاضر باشند
// فعلاً برای همه نودها:
// bit0 -> node0
// bit1 -> node1
// ...
/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f7xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define LED1_Pin GPIO_PIN_0
#define LED1_GPIO_Port GPIOB
#define LED2_Pin GPIO_PIN_7
#define LED2_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */
#define NODES 16
#define TARGET_BOARD_ID  0
#define BASE_ID          0x100

// === [NEW] bed model dimensions ===
// کل تخت: 32 ردیف × 16 ستون
#define BED_ROWS   32U
#define BED_COLS   16U

// ===  bed sync config ===
// ماسک نودهایی که باید برای تشکیل snapshot کامل تخت حاضر باشند
// فعلاً برای همه نودها:
// bit0 -> node0
// bit1 -> node1
// ...
//#define BED_REQUIRED_NODE_MASK   ((1UL << NODES) - 1UL)    // Full Nodes

// === [TEMP TEST] only node 1 is required for a complete bed snapshot ===
#define BED_REQUIRED_NODE_MASK   (1UL << 1)

// === [NEW] Sensor decoding config ===
// تعداد سنسورها در هر نود
#define SENSOR_COUNT_PER_NODE   32U

// تعداد chunk های CAN برای هر نود
#define SENSOR_CHUNK_COUNT      4U

// تعداد بایت در هر chunk
#define SENSOR_BYTES_PER_CHUNK  8U

// ماسک برای استخراج مقدار سنسور (6 بیت پایین)
#define SENSOR_VALUE_MASK       0x3FU

// شیفت برای status (2 بیت بالا)
#define SENSOR_STATUS_SHIFT     6U

// ماسک status (بعد از شیفت)
#define SENSOR_STATUS_MASK      0x03U


// === [NEW] sensor status values from node ===
#define SENSOR_STATUS_OK            0U
#define SENSOR_STATUS_WARNING       1U
#define SENSOR_STATUS_ERROR         2U
#define SENSOR_STATUS_DISCONNECTED  3U

//  node state definitions ===
// وضعیت کلی نود از نگاه Main Board
#define NODE_STATE_OFFLINE   0U
#define NODE_STATE_ONLINE    1U
#define NODE_STATE_STALE     2U
// === UART flags encoding ===
// بیت‌های 0..1 برای state نود استفاده می‌شوند
#define UART_FLAG_NODE_STATE_MASK   0x03U
#define UART_FLAG_NODE_STATE_SHIFT  0U
// === [TUNED] measured node period is about 1 second ===
#define NODE_EXPECTED_PERIOD_MS  1000U

// === [TUNED] timeout thresholds with some jitter margin ===
#define NODE_STALE_TIMEOUT_MS    1600U
#define NODE_OFFLINE_TIMEOUT_MS  3500U



// === DEBUG FLAGS ===
// 1 = enable related debug prints
// 0 = disable related debug prints

// === BASE SYSTEM DEBUG ===
#define DEBUG_CAN_RX_COMPLETE    0
#define DEBUG_BED_SYNC           0
#define DEBUG_NODE_MONITOR       0
#define DEBUG_SERIAL_TX_STATS    0

// === SUMMARY LAYER DEBUG ===
#define DEBUG_ZONE_ANALYSIS      0
#define DEBUG_MOVEMENT           0
#define DEBUG_RISK               0
#define DEBUG_ALERT              0
#define DEBUG_RECOMMENDATION     0
#define DEBUG_SUMMARY            0
// === DEBUG CONTROL FLAGS ===
// فعال/غیرفعال کردن لاگ‌های enqueue برای UART
#define DEBUG_SUMMARY_ENQUEUE    0
// === debug print throttle period ===
// جلوگیری از شلوغ شدن UART debug
#define DEBUG_PRINT_PERIOD_MS    1000U
/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */


// === [NEW] Sensor decoding config ===
// تعداد سنسورها در هر نود
#define SENSOR_COUNT_PER_NODE   32U

// تعداد chunk های CAN برای هر نود
#define SENSOR_CHUNK_COUNT      4U

// تعداد بایت در هر chunk
#define SENSOR_BYTES_PER_CHUNK  8U

// ماسک برای استخراج مقدار سنسور (6 بیت پایین)
#define SENSOR_VALUE_MASK       0x3FU

// شیفت برای status (2 بیت بالا)
#define SENSOR_STATUS_SHIFT     6U

// ماسک status (بعد از شیفت)
#define SENSOR_STATUS_MASK      0x03U


// === [NEW] sensor status values from node ===
#define SENSOR_STATUS_OK            0U
#define SENSOR_STATUS_WARNING       1U
#define SENSOR_STATUS_ERROR         2U
#define SENSOR_STATUS_DISCONNECTED  3U

//  node state definitions ===
// وضعیت کلی نود از نگاه Main Board
#define NODE_STATE_OFFLINE   0U
#define NODE_STATE_ONLINE    1U
#define NODE_STATE_STALE     2U
// === UART flags encoding ===
// بیت‌های 0..1 برای state نود استفاده می‌شوند
#define UART_FLAG_NODE_STATE_MASK   0x03U
#define UART_FLAG_NODE_STATE_SHIFT  0U
// === [TUNED] measured node period is about 1 second ===
#define NODE_EXPECTED_PERIOD_MS  1000U

// === [TUNED] timeout thresholds with some jitter margin ===
#define NODE_STALE_TIMEOUT_MS    1600U
#define NODE_OFFLINE_TIMEOUT_MS  3500U



// === DEBUG FLAGS ===
// 1 = enable related debug prints
// 0 = disable related debug prints

#define DEBUG_CAN_RX_COMPLETE    0
#define DEBUG_BED_SYNC           1
#define DEBUG_NODE_MONITOR       0
#define DEBUG_SERIAL_TX_STATS    0

#define DEBUG_ZONE_ANALYSIS      0
#define DEBUG_MOVEMENT           0
#define DEBUG_RISK               0
#define DEBUG_ALERT              0
#define DEBUG_RECOMMENDATION     0
#define DEBUG_SUMMARY            1

// === debug print throttle period ===
// جلوگیری از شلوغ شدن UART debug
#define DEBUG_PRINT_PERIOD_MS    1000U




#define ENABLE_NODE1_TEST_CMD_TASK   0


/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
