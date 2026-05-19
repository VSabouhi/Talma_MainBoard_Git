/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
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
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "can.h"        // <<< اضافه کن
#include "can_rtos_rx.h"
#include "app_can.h"
#include "Serial_link.h"   // ارسال از طریق queue#include "uart_pkt.h"
#include  <stdio.h>
#include <string.h>
#include "can_rtos_tx.h"
#include "bed_model.h"
#include "sensor_store.h"
#include "node_state.h"
#include "bed_model.h"   // دسترسی به bed_cycle و bed_value
#include "uart_pkt.h"
#include "zone_analysis.h"   // تحلیل ناحیه‌های بالینی
#include "movement.h"
#include "risk_engine.h"   // محاسبه risk score
#include "alert_engine.h"
#include "recommendation.h"   // تولید توصیه عملی
#include "app_config.h"
#include "motor_scheduler.h"
#include "motor_test.h"
#include "therapy_engine.h"   // تصمیم سطح بالا برای therapy و ارسال به motor_scheduler
#include "motor_status_can.h"   // decode Node -> Main motor feedback on 0x480 + BOARD_ID
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
typedef StaticTask_t osStaticThreadDef_t;

/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

// === minimum time gap between full-bed snapshots sent to UI ===
// جلوگیری از ارسال بیش از حد snapshot کامل
#define SUMMARY_FIXED_PERIOD_MS   1000U

// === UI stream throttle ===
// برای اینکه UART RX فرصت دریافت command از UI داشته باشد، stream خروجی را محدود می‌کنیم.
#define UI_NODE32_SEND_EVERY_N_CYCLES   50U
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */
// ===  zone analysis result ===
// خروجی تحلیل ناحیه‌های تخت
static ZoneAnalysisResult_t g_zone_result;
static MovementResult_t g_movement;
// ===  risk engine result ===
static RiskResult_t g_risk_result;
static AlertResult_t g_alert;
// === recommendation result ===
static RecommendationResult_t g_recommendation;
// === global therapy/intervention planner state ===
// app_intervention.c برای approve/reject به این state دسترسی دارد.
TherapyEngineState_t g_therapy;
// === debug print throttle ===
// جلوگیری از spam شدن UART
static uint32_t last_debug_print = 0;

// === exposure tracking (seconds above threshold) ===
static uint32_t g_sacrum_exposure_s = 0U;
static uint32_t g_heels_exposure_s = 0U;
static uint32_t g_shoulders_exposure_s = 0U;

// === exposure thresholds per zone ===
// این thresholdها برای تست trend/exposure روی zoneهای مختلف جدا تعریف شده‌اند
static const uint8_t g_sacrum_exposure_threshold    = 20U;
static const uint8_t g_heels_exposure_threshold     = 8U;
static const uint8_t g_shoulders_exposure_threshold = 12U;

// === last time exposure counters were updated ===
static uint32_t g_last_exposure_update_ms = 0U;



extern volatile uint32_t rx_dropped;


osThreadId_t uartTestTaskHandle;
osThreadId_t serialTxTaskHandle;
// === SerialLink RX task handle ===
// دریافت commandهای UI روی UART
osThreadId_t serialRxTaskHandle;
osThreadId_t canTxTaskHandle;
osThreadId_t nodeMonitorTaskHandle;   // === task handle for node state monitor ===

osThreadId_t motorSchedulerTaskHandle;
osThreadId_t motorTestTaskHandle;
/* USER CODE END Variables */
/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .stack_size = 512 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for v_CanRxTask */
osThreadId_t v_CanRxTaskHandle;
uint32_t v_CanRxTaskBuffer[ 1024 ];
osStaticThreadDef_t v_CanRxTaskControlBlock;
const osThreadAttr_t v_CanRxTask_attributes = {
  .name = "v_CanRxTask",
  .cb_mem = &v_CanRxTaskControlBlock,
  .cb_size = sizeof(v_CanRxTaskControlBlock),
  .stack_mem = &v_CanRxTaskBuffer[0],
  .stack_size = sizeof(v_CanRxTaskBuffer),
  .priority = (osPriority_t) osPriorityAboveNormal,
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
void UartTestTask(void *argument);
static uint8_t BuildNodeFlags(uint8_t node);   // ===  build UART flags for one node ===
static uint8_t SensorStatus_IsValid(uint8_t status);   // validity policy for one sensor status ===
static uint8_t SensorStatus_GetConfidence(uint8_t status);   // map sensor status to confidence ===
static void UpdateExposureCounters(const ZoneAnalysisResult_t *zone_res, uint32_t now_ms);
/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void *argument);
void CanRxTask(void *argument);
void NodeMonitorTask(void *argument);   //  monitor node online/stale/offline state ===
void Node1CmdTask(void *argument);
void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */
	CanRtosRx_Init();
	CanRtosTx_Init();
	// === init motor scheduler queue/state ===
	// این لایه فرمان‌های موتور را queue می‌کند و با pacing امن روی CAN TX می‌فرستد.
	MotorScheduler_Init();
	UartPkt_Init();
	configASSERT(qCanRx != NULL);

	SerialLink_Init();
	// === initialize clinical zone layout ===
	ZoneAnalysis_Init();
	Movement_Init();
	AlertEngine_Init();
	// === initialize therapy engine ===
	// therapy فعلاً disabled است تا حرکت خودکار بدون تصمیم ما اجرا نشود.
	TherapyEngine_Init(&g_therapy);

	// DEBUG:
	// فقط برای تست planner.
	// این فعال‌سازی موتور را حرکت نمی‌دهد؛ فقط اجازه ساخت pending_plan می‌دهد.
	TherapyEngine_SetEnabled(&g_therapy, 1U);

	// DEBUG ONLY:
	// ساخت pending plan دستی برای تست approve lifecycle.
	// بعد از تست حتماً کامنت شود.
	//TherapyEngine_DebugCreatePendingPlan(&g_therapy, HAL_GetTick());

	const osThreadAttr_t serialTxTask_attributes = {
	  .name = "serialTxTask",
	  .stack_size = 512 * 4,
	  .priority = (osPriority_t) osPriorityNormal,
	};

	const osThreadAttr_t serialRxTask_attributes = {
	  .name = "serialRxTask",
	  .stack_size = 512 * 4,
	  .priority = (osPriority_t) osPriorityAboveNormal,
	};

	const osThreadAttr_t canTxTask_attributes = {
	  .name = "canTxTask",
	  .stack_size = 512 * 4,
	  .priority = (osPriority_t) osPriorityNormal,
	};

	const osThreadAttr_t motorSchedulerTask_attributes = {
	  .name = "motorSched",
	  .stack_size = 512 * 4,
	  .priority = (osPriority_t) osPriorityNormal,
	};

	#if ENABLE_MOTOR_TEST_TASK
		const osThreadAttr_t motorTestTask_attributes = {
		  .name = "motorTest",
		  .stack_size = 512 * 4,
		  .priority = (osPriority_t) osPriorityLow,
		};
	#endif

	#if ENABLE_NODE1_TEST_CMD_TASK
		const osThreadAttr_t node1CmdTask_attributes = {
		  .name = "node1CmdTask",
		  .stack_size = 512 * 4,
		  .priority = (osPriority_t) osPriorityLow,
		};
	#endif

	// === attributes for node monitor task ===
	const osThreadAttr_t nodeMonitorTask_attributes = {
	  .name = "nodeMonitorTask",
	  .stack_size = 512 * 4,
	  .priority = (osPriority_t) osPriorityLow,
	};
  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of defaultTask */
  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* creation of v_CanRxTask */
  v_CanRxTaskHandle = osThreadNew(CanRxTask, NULL, &v_CanRxTask_attributes);
  canTxTaskHandle = osThreadNew(CanTxTask, NULL, &canTxTask_attributes);

  // === motor command scheduler task ===
  // همه commandهای موتور از این task عبور می‌کنند تا interleave نشوند.
  motorSchedulerTaskHandle = osThreadNew(MotorScheduler_Task, NULL, &motorSchedulerTask_attributes);

  #if ENABLE_MOTOR_TEST_TASK
  // === temporary hardcoded motor tests ===
  // بعد از bring-up موفق، ENABLE_MOTOR_TEST_TASK را 0 کن.
  motorTestTaskHandle = osThreadNew(MotorTest_Task, NULL, &motorTestTask_attributes);
  #endif
  /* USER CODE BEGIN RTOS_THREADS */

  //uartTestTaskHandle = osThreadNew(UartTestTask, NULL, &uartTestTask_attributes);
  serialTxTaskHandle  = osThreadNew(SerialLink_TxTask, NULL, &serialTxTask_attributes);
  // === create SerialLink RX task ===
  // approve/reject commandها از UI
  serialRxTaskHandle = osThreadNew(SerialLink_RxTask, NULL, &serialRxTask_attributes);

	#if ENABLE_NODE1_TEST_CMD_TASK
	  // === create periodic test command task only when enabled ===
	  osThreadNew(Node1CmdTask, NULL, &node1CmdTask_attributes);
	#endif

  // === create node monitor task ===
  nodeMonitorTaskHandle = osThreadNew(NodeMonitorTask, NULL, &nodeMonitorTask_attributes);


  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void *argument)
{
  /* USER CODE BEGIN StartDefaultTask */
	//HAL_CAN_ActivateNotification(&hcan1, CAN_IT_RX_FIFO0_MSG_PENDING);
	//printf("CAN notif enabled\r\n");

  /* Infinite loop */
	for(;;)
	{
	 //   HAL_GPIO_TogglePin(LED1_GPIO_Port, LED1_Pin);

	#if DEBUG_SERIAL_TX_STATS
	    printf("SerialLink dropped=%lu\r\n",
	           (unsigned long)SerialLink_TxDropped());
	#endif


	    // === اگر یک snapshot کامل از تخت آماده شده ===
	   /* if (bed_snapshot_ready != 0U)
	    {
	        // === ارسال کل تخت به UI ===
	        UartPkt_SendBedSnapshot((uint16_t)bed_cycle, bed_value);

	        // === reset flag تا دوباره ارسال نشود ===
	        bed_snapshot_ready = 0U;
	    }*/

	    osDelay(500);   // کمی سریع‌تر برای پاسخ بهتر
	}
  /* USER CODE END StartDefaultTask */
}

/* USER CODE BEGIN Header_CanRxTask */
/**
* @brief Function implementing the v_CanRxTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_CanRxTask */
void CanRxTask(void *argument)
{
  (void)argument;

  CanRxMsg m;

  // === آخرین زمان enqueue شدن packetهای سنگین UI ===
  static uint32_t last_summary_tx_ms = 0U;

  // === [CONFIG] حداکثر فاصله مجاز بین chunk های یک assemble ===
  const uint32_t ASM_TIMEOUT_MS = 100U;

  // === [STAT] شمارش خطای enqueue به UART TX queue ===
  static volatile uint32_t tx_drop = 0;

  // === شناسه فریم ارسالی به UI ===
  static uint16_t ui_frame_id = 0U;

  uint16_t frame_id = 0;

  for (;;)
  {
    // === [RX] منتظر پیام از صف CAN RX ===
    if (xQueueReceive(qCanRx, &m, portMAX_DELAY) != pdPASS)
      continue;

    const uint32_t now = HAL_GetTick();
    last_rx_ms = now;

    // === [FILTER] فقط فریم استاندارد data با طول 8 بایت ===
    if (m.h.IDE != CAN_ID_STD)   continue;
    if (m.h.RTR != CAN_RTR_DATA) continue;
    if (m.h.DLC != 8U)           continue;

    const uint16_t id = (uint16_t)m.h.StdId;

    // === [MOTOR STATUS RX] Node -> Main motor feedback ===
    // Motor status frames use 0x480 + BOARD_ID.
    // این frameها مربوط به sensor assembly نیستند و باید قبل از sensor decode جدا شوند.
    if (MotorStatusCan_IsStatusId(id) != 0U)
    {
      MotorStatusCan_HandleFrame(id, m.d, now);
      continue;
    }

    // === [FILTER] فقط ID های بازه سنسورها ===
    if (id < (uint16_t)BASE_ID) continue;

    // === [DECODE] استخراج node و chunk از CAN ID ===
    const uint16_t rel   = (uint16_t)(id - (uint16_t)BASE_ID);
    const uint8_t  node  = (uint8_t)(rel >> 2);
    const uint8_t  chunk = (uint8_t)(rel & 0x03U);

    // === [BOUND CHECK] اگر node خارج از بازه معتبر بود ===
    if (node >= (uint8_t)NODES) continue;

    // === [TRACK] ثبت آخرین زمان دریافت هر فریم از این نود ===
    node_last_frame_ms[node] = now;

    // === [ASSEMBLY TIMEOUT] اگر assemble نیمه‌کاره قدیمی مانده، ریست شود ===
    if (chunk_mask[node] != 0U)
    {
      if ((now - asm_start_ms[node]) > ASM_TIMEOUT_MS)
      {
        chunk_mask[node] = 0U;
        asm_start_ms[node] = now;
      }
    }

    // === [ASSEMBLY START] اگر این اولین chunk این سیکل باشد ===
    if (chunk_mask[node] == 0U)
    {
      asm_start_ms[node] = now;
    }

    // === [DECODE SENSOR BYTES] decode کردن raw/value/status ===
    const uint8_t base = (uint8_t)((uint32_t)chunk * SENSOR_BYTES_PER_CHUNK);

    for (uint8_t i = 0; i < SENSOR_BYTES_PER_CHUNK; i++)
    {
      const uint8_t raw = m.d[i];
      const uint8_t idx = (uint8_t)(base + i);

      sensors32[node][idx] = raw;

      sensor_value[node][idx] =
          (uint8_t)(raw & SENSOR_VALUE_MASK);

      sensor_status[node][idx] =
          (uint8_t)((raw >> SENSOR_STATUS_SHIFT) & SENSOR_STATUS_MASK);

      valid_mask[node][idx] =
          SensorStatus_IsValid(sensor_status[node][idx]);

      sensor_confidence[node][idx] =
          SensorStatus_GetConfidence(sensor_status[node][idx]);
    }

    // === [MARK] این chunk برای این نود دریافت شد ===
    chunk_mask[node] |= (uint8_t)(1U << chunk);

    // === [COMPLETE] اگر هر 4 chunk رسیده‌اند ===
    if (chunk_mask[node] == 0x0FU)
    {
      chunk_mask[node] = 0U;
      cycles_ok[node]++;

      // === [MEASURE] اندازه‌گیری فاصله بین complete cycle های این نود ===
      {
        uint32_t prev_complete = node_last_complete_ms[node];
        node_last_complete_ms[node] = now;

        if (prev_complete != 0U)
        {
          /*
          printf("NODE %u complete_dt=%lu ms\r\n",
                 node,
                 (unsigned long)(now - prev_complete));
          */
        }
      }

      node_state[node] = NODE_STATE_ONLINE;

      // === copy completed node sensor data into global bed model ===
      BedModel_UpdateNode(node,
                          sensor_value,
                          sensor_status,
                          valid_mask,
                          sensor_confidence);

      // === update bed cycle sync state ===
      BedSync_OnNodeUpdated(node);

      // === اگر یک snapshot کامل از تخت آماده شده ===
      if (bed_snapshot_ready != 0U)
      {
        // === run zone analysis on stable snapshot ===
        ZoneAnalysis_Run(bed_value_send,
                         bed_valid_send,
                         &g_zone_result);

        // === run movement detection on stable snapshot ===
        Movement_Run(bed_value_send,
                     bed_valid_send,
                     &g_movement);

        // === run risk score engine ===
        RiskEngine_Run(&g_zone_result,
                       &g_movement,
                       &g_risk_result);

        AlertEngine_Run(&g_risk_result,
                        &g_movement,
                        &g_alert);

        // === generate recommendation from current clinical summary ===
        Recommendation_Run(&g_risk_result,
                           &g_alert,
                           &g_movement,
                           &g_recommendation);

        // === build intervention plan only ===
        // هیچ حرکت موتوری اینجا انجام نمی‌شود.
        TherapyEngine_Run(&g_therapy,
                          &g_zone_result,
                          &g_movement,
                          &g_risk_result,
                          &g_alert,
                          &g_recommendation,
                          now);

        // === send pending intervention plan only once ===
        {
          TherapyPlan_t *p =
              (TherapyPlan_t *)TherapyEngine_GetPendingPlan(&g_therapy);

          if (p != 0)
          {
            if (p->ui_sent == 0U)
            {
              if (SerialLink_SendInterventionPlan_Async(p) == pdPASS)
              {
                p->ui_sent = 1U;

                printf("THERAPY PLAN: delivered to UI id=%lu\r\n",
                       (unsigned long)p->plan_id);
              }
            }
          }
        }

        // === derived summary fields for analytics/trend ===
        uint16_t uptime_s = (uint16_t)(now / 1000U);

        uint8_t shoulders_avg = (uint8_t)(
            ((uint16_t)g_zone_result.zone[ZONE_LEFT_SHOULDER].avg +
             (uint16_t)g_zone_result.zone[ZONE_RIGHT_SHOULDER].avg) / 2U);

        uint8_t shoulders_peak =
            (g_zone_result.zone[ZONE_LEFT_SHOULDER].peak >
             g_zone_result.zone[ZONE_RIGHT_SHOULDER].peak)
            ? g_zone_result.zone[ZONE_LEFT_SHOULDER].peak
            : g_zone_result.zone[ZONE_RIGHT_SHOULDER].peak;

        UpdateExposureCounters(&g_zone_result, now);

        uint8_t zones_valid_mask = 0U;

        if (g_zone_result.zone[ZONE_SACRUM].valid_cells > 0U)
          zones_valid_mask |= (1U << 0);

        if (g_zone_result.zone[ZONE_LEFT_HEEL].valid_cells > 0U)
          zones_valid_mask |= (1U << 1);

        if (g_zone_result.zone[ZONE_RIGHT_HEEL].valid_cells > 0U)
          zones_valid_mask |= (1U << 2);

        if (g_zone_result.zone[ZONE_LEFT_SHOULDER].valid_cells > 0U)
          zones_valid_mask |= (1U << 3);

        if (g_zone_result.zone[ZONE_RIGHT_SHOULDER].valid_cells > 0U)
          zones_valid_mask |= (1U << 4);

        // bit0 = raw averages
        // bit1 = smoothed averages
        // bit2 = exposure counters active
        uint8_t summary_flags = 0x05U;

        if ((now - last_debug_print) >= DEBUG_PRINT_PERIOD_MS)
        {
          last_debug_print = now;

#if DEBUG_ZONE_ANALYSIS
          printf("ZONE: sacrum avg=%u peak=%u valid=%u active=%u | heelL avg=%u | heelR avg=%u\r\n",
                 g_zone_result.zone[ZONE_SACRUM].avg,
                 g_zone_result.zone[ZONE_SACRUM].peak,
                 g_zone_result.zone[ZONE_SACRUM].valid_cells,
                 g_zone_result.zone[ZONE_SACRUM].active_cells,
                 g_zone_result.zone[ZONE_LEFT_HEEL].avg,
                 g_zone_result.zone[ZONE_RIGHT_HEEL].avg);
#endif

#if DEBUG_ALERT
          printf("ALERT: active=%u type=%u sev=%u dur=%lu s\r\n",
                 g_alert.active,
                 g_alert.type,
                 g_alert.severity,
                 (unsigned long)g_alert.duration_s);
#endif

#if DEBUG_RISK
          printf("RISK: score=%u level=%u\r\n",
                 g_risk_result.score,
                 g_risk_result.level);
#endif

#if DEBUG_MOVEMENT
          printf("MOV: energy=%u detected=%u\r\n",
                 g_movement.energy,
                 g_movement.detected);
#endif

#if DEBUG_RECOMMENDATION
          printf("REC: code=%u priority=%u sac_avg=%u\r\n",
                 g_recommendation.code,
                 g_recommendation.priority,
                 g_zone_result.zone[ZONE_SACRUM].avg);
#endif
        }

        // === UI heavy packet scheduler ===
        // قبلاً در هر period چهار packet پشت‌سرهم enqueue می‌شد:
        // BED_SNAPSHOT + BED_STATUS + NODE_HEALTH + SUMMARY
        // این باعث اشباع UART می‌شد.
        // الان در هر period فقط یکی از آن‌ها enqueue می‌شود.
        if ((now - last_summary_tx_ms) >= SUMMARY_FIXED_PERIOD_MS)
        {
          uint16_t time_since_last_movement_s = 0xFFFFU;

          if (g_movement.last_movement_ms != 0U)
          {
            uint32_t dt_ms = now - g_movement.last_movement_ms;
            uint32_t dt_s = dt_ms / 1000U;

            if (dt_s > 0xFFFFU)
              dt_s = 0xFFFFU;

            time_since_last_movement_s = (uint16_t)dt_s;
          }

          frame_id = ++ui_frame_id;

          // === round-robin heavy UI packets ===
          // هر بار فقط یک packet سنگین وارد qSerialTx می‌شود.
          static uint8_t ui_heavy_slot = 0U;
          BaseType_t ui_tx_ok = pdFAIL;

#if DEBUG_SUMMARY_ENQUEUE
          printf("UI FRAME: frame_id=%u slot=%u\r\n",
                 frame_id,
                 (unsigned)ui_heavy_slot);
#endif

          switch (ui_heavy_slot)
          {
            case 0U:
            {
              ui_tx_ok = SerialLink_SendBedSnapshot_Async(frame_id);

#if DEBUG_SUMMARY_ENQUEUE
              printf("ENQ: BED_SNAPSHOT %s\r\n",
                     (ui_tx_ok == pdPASS) ? "ok" : "fail");
#endif
              break;
            }

            case 1U:
            {
              ui_tx_ok = SerialLink_SendBedStatus_Async(frame_id);

#if DEBUG_SUMMARY_ENQUEUE
              printf("ENQ: BED_STATUS %s\r\n",
                     (ui_tx_ok == pdPASS) ? "ok" : "fail");
#endif
              break;
            }

            case 2U:
            {
              ui_tx_ok = SerialLink_SendNodeHealth_Async(frame_id);

#if DEBUG_SUMMARY_ENQUEUE
              printf("ENQ: NODE_HEALTH %s\r\n",
                     (ui_tx_ok == pdPASS) ? "ok" : "fail");
#endif
              break;
            }

            case 3U:
            default:
            {
              ui_tx_ok = SerialLink_SendSummary_Async(
                  frame_id,
                  uptime_s,
                  g_risk_result.score,
                  g_risk_result.level,
                  g_movement.detected,
                  time_since_last_movement_s,
                  g_alert.active,
                  g_alert.type,
                  g_alert.severity,
                  (uint16_t)g_alert.duration_s,
                  g_recommendation.code,
                  g_recommendation.priority,
                  g_zone_result.zone[ZONE_SACRUM].avg,
                  g_zone_result.zone[ZONE_SACRUM].peak,
                  g_zone_result.zone[ZONE_LEFT_HEEL].avg,
                  g_zone_result.zone[ZONE_RIGHT_HEEL].avg,
                  shoulders_avg,
                  shoulders_peak,
                  g_sacrum_exposure_threshold,
                  (uint16_t)g_sacrum_exposure_s,
                  (uint16_t)g_heels_exposure_s,
                  (uint16_t)g_shoulders_exposure_s,
                  zones_valid_mask,
                  summary_flags);

#if DEBUG_SUMMARY_ENQUEUE
              printf("ENQ: SUMMARY %s\r\n",
                     (ui_tx_ok == pdPASS) ? "ok" : "fail");
#endif
              break;
            }
          }

          if (ui_tx_ok == pdPASS)
          {
            ui_heavy_slot++;

            if (ui_heavy_slot >= 4U)
              ui_heavy_slot = 0U;

            if (last_summary_tx_ms == 0U)
              last_summary_tx_ms = now;
            else
              last_summary_tx_ms += SUMMARY_FIXED_PERIOD_MS;
          }
          else
          {
            // اگر queue پر بود، slot را جلو نمی‌بریم تا packet از دست نرود.
            // در cycle بعد دوباره همین نوع packet تلاش می‌شود.
          }
        }

        // === این snapshot پردازش شد ===
        bed_snapshot_ready = 0U;
      }

#if DEBUG_CAN_RX_COMPLETE
      printf("NODE %u state=%u flags=0x%02X node_cycle=%lu bed_cycle=%lu sync=0x%08lX | "
             "S0->R%uC%u V=%u S=%u M=%u C=%u | "
             "S16->R%uC%u V=%u S=%u M=%u C=%u\r\n",
             node,
             node_state[node],
             BuildNodeFlags(node),
             (unsigned long)cycles_ok[node],
             (unsigned long)bed_cycle,
             (unsigned long)bed_sync_mask,

             BedMap_GetRow(node, 0),
             BedMap_GetCol(0),
             sensor_value[node][0],
             sensor_status[node][0],
             valid_mask[node][0],
             sensor_confidence[node][0],

             BedMap_GetRow(node, 16),
             BedMap_GetCol(16),
             sensor_value[node][16],
             sensor_status[node][16],
             valid_mask[node][16],
             sensor_confidence[node][16]);
#endif

      // === ساخت flags برای UI و ارسال Node32 با throttle ===
      {
        uint8_t uart_flags = BuildNodeFlags(node);

        static uint32_t ui_node32_div = 0U;
        ui_node32_div++;

        if ((ui_node32_div % UI_NODE32_SEND_EVERY_N_CYCLES) == 0U)
        {
          if (SerialLink_SendNode32_Async(node,
                                          (uint16_t)cycles_ok[node],
                                          uart_flags,
                                          sensors32[node]) != pdPASS)
          {
            tx_drop++;
            printf("ENQ NODE32 FAIL\r\n");
          }
          else
          {
            printf("ENQ NODE32 OK\r\n");
          }
        }
      }
    }
  }
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

// ===  Build UART flags for a node ===
// فعلاً فقط state نود را در بیت‌های 0..1 قرار می‌دهد
static uint8_t BuildNodeFlags(uint8_t node)
{
  uint8_t flags = 0U;

  // اگر node نامعتبر بود، flags صفر برگردد
  if (node >= (uint8_t)NODES)
    return 0U;

  // state نود را در بیت‌های 0..1 قرار بده
  flags |= (uint8_t)((node_state[node] & UART_FLAG_NODE_STATE_MASK) << UART_FLAG_NODE_STATE_SHIFT);

  return flags;
}
/*--------------------------------------------------------------------------------*/
// === [NEW] sensor validity policy ===
// خروجی:
// 1 = این سنسور usable است
// 0 = این سنسور فعلاً invalid است
static uint8_t SensorStatus_IsValid(uint8_t status)
{
  switch (status)
  {
    case SENSOR_STATUS_OK:
      return 1U;

    case SENSOR_STATUS_WARNING:
      // فعلاً warning را usable در نظر می‌گیریم
      return 1U;

    case SENSOR_STATUS_ERROR:
      return 0U;

    case SENSOR_STATUS_DISCONNECTED:
      return 0U;

    default:
      // هر status ناشناخته را invalid فرض کن
      return 0U;
  }
}

/*--------------------------------------------------------------------------------*/
// === [NEW] sensor confidence policy ===
// خروجی:
// 255 = اعتماد کامل
// 128 = قابل استفاده ولی با احتیاط
// 0   = غیرقابل اعتماد
static uint8_t SensorStatus_GetConfidence(uint8_t status)
{
  switch (status)
  {
    case SENSOR_STATUS_OK:
      return 255U;

    case SENSOR_STATUS_WARNING:
      return 128U;

    case SENSOR_STATUS_ERROR:
      return 0U;

    case SENSOR_STATUS_DISCONNECTED:
      return 0U;

    default:
      return 0U;
  }
}
/*--------------------------------------------------------------------------------*/
static void UpdateExposureCounters(const ZoneAnalysisResult_t *zone_res, uint32_t now_ms)
{
  if (zone_res == 0)
    return;

  if (g_last_exposure_update_ms == 0U)
  {
    g_last_exposure_update_ms = now_ms;
    return;
  }

  uint32_t dt_ms = now_ms - g_last_exposure_update_ms;

  // فقط وقتی حداقل 1 ثانیه گذشته باشد، counterها را جلو ببر
  if (dt_ms < 1000U)
    return;

  uint32_t dt_s = dt_ms / 1000U;
  g_last_exposure_update_ms += dt_s * 1000U;

  uint8_t sacrum_avg = zone_res->zone[ZONE_SACRUM].avg;
  uint8_t heel_left_avg = zone_res->zone[ZONE_LEFT_HEEL].avg;
  uint8_t heel_right_avg = zone_res->zone[ZONE_RIGHT_HEEL].avg;
  uint8_t shoulders_left_avg = zone_res->zone[ZONE_LEFT_SHOULDER].avg;
  uint8_t shoulders_right_avg = zone_res->zone[ZONE_RIGHT_SHOULDER].avg;

  // === FIX: use max instead of average for exposure detection ===
  // دلیل: avg خیلی low میشه و exposure هیچوقت trigger نمیشه

  uint8_t heels_metric =
      (heel_left_avg > heel_right_avg) ? heel_left_avg : heel_right_avg;

  uint8_t shoulders_metric =
      (shoulders_left_avg > shoulders_right_avg) ? shoulders_left_avg : shoulders_right_avg;



  // === exposure accumulation per zone ===
  // هر zone threshold مخصوص خودش را دارد
  if (sacrum_avg >= g_sacrum_exposure_threshold)
    g_sacrum_exposure_s += dt_s;

  if (heels_metric >= g_heels_exposure_threshold)
    g_heels_exposure_s += dt_s;

  if (shoulders_metric >= g_shoulders_exposure_threshold)
    g_shoulders_exposure_s += dt_s;

 /* printf("DBG EXP SRC: sac=%u heelL=%u heelR=%u shL=%u shR=%u thr=%u\n",
         (unsigned int)zone_res->zone[ZONE_SACRUM].avg,
         (unsigned int)zone_res->zone[ZONE_LEFT_HEEL].avg,
         (unsigned int)zone_res->zone[ZONE_RIGHT_HEEL].avg,
         (unsigned int)zone_res->zone[ZONE_LEFT_SHOULDER].avg,
         (unsigned int)zone_res->zone[ZONE_RIGHT_SHOULDER].avg,
         (unsigned int)g_sacrum_exposure_threshold);*/

  // === DEBUG: verify exposure counters are increasing ===
  // این print برای اینه که ببینیم counterها واقعا در MCU افزایش پیدا میکنن یا نه
  printf("DBG EXP CNT: sac=%lu heel=%lu sh=%lu\r\n",
         (unsigned long)g_sacrum_exposure_s,
         (unsigned long)g_heels_exposure_s,
         (unsigned long)g_shoulders_exposure_s);
}
/*--------------------------------------------------------------------------------*/

void vApplicationMallocFailedHook(void)
{
  taskDISABLE_INTERRUPTS();
  for(;;);
}
/*--------------------------------------------------------------------------------*/

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
  (void)xTask; (void)pcTaskName;
  taskDISABLE_INTERRUPTS();
  for(;;);
}
/*--------------------------------------------------------------------------------*/
void UartTestTask(void *argument)
{
    uint8_t test[32];

    for(int i=0;i<32;i++)
        test[i] = i;

    for(;;)
    {
        UartPkt_SendNode32(1, 123, 0x01, test);
        osDelay(1000);
    }
}

/*--------------------------------------------------------------------------------*/


/*void Node1CmdTask(void *argument)
{
  (void)argument;

  //const uint8_t node = 1;
  //const uint32_t period_ms = 100;   // “دائم” یعنی دوره‌ای؛ اینجا 100ms

  // مثال: cmd=0x01 (PING) یا هر کامند دلخواه خودت
  //const uint8_t cmd = 0x01;

  for (;;)
  {


    // نمونه: arg0/1/2 و param32 رو هر چی لازم داری پر کن
		CanCmd_SendToNode(1, 0x01, 0, 0, 0, 0);

    osDelay(500);
  }
}*/

/*--------------------------------------------------------------------------------*/

void NodeMonitorTask(void *argument)
{
  (void)argument;

  // === [CONFIG] هر چند ms یک بار وضعیت نودها بررسی شود ===
  const uint32_t monitor_period_ms = 100U;

  // === [DEBUG] نگهداری state قبلی برای تشخیص تغییر ===
  static uint8_t prev_state[NODES];

  // === [INIT] در شروع، مقدار نامعتبر می‌گذاریم تا اولین مقایسه معنی‌دار شود ===
  for (uint8_t n = 0; n < NODES; n++)
  {
    prev_state[n] = 0xFFU;
  }

  printf("NodeMonitorTask started\r\n");

  for (;;)
  {
    const uint32_t now = HAL_GetTick();

    for (uint8_t n = 0; n < NODES; n++)
    {
      uint8_t new_state;

      // === [POLICY] اگر هنوز هیچ complete cycle از این نود نگرفته‌ایم ===
      if (node_last_complete_ms[n] == 0U)
      {
        new_state = NODE_STATE_OFFLINE;
      }
      else
      {
        const uint32_t age = now - node_last_complete_ms[n];

        if (age >= NODE_OFFLINE_TIMEOUT_MS)
        {
          new_state = NODE_STATE_OFFLINE;
        }
        else if (age >= NODE_STALE_TIMEOUT_MS)
        {
          new_state = NODE_STATE_STALE;
        }
        else
        {
          new_state = NODE_STATE_ONLINE;
        }
      }

      // === [UPDATE] ثبت state جدید ===
      node_state[n] = new_state;

      // === [EVENT] فقط وقتی state تغییر کرد ===
      if (prev_state[n] != new_state)
      {
		#if DEBUG_NODE_MONITOR
			  if (n == 1U)
			  {
				printf("NODE %u state -> %u (age=%lu ms)\r\n",
					   n,
					   new_state,
					   (unsigned long)((node_last_complete_ms[n] == 0U) ? 0U : (now - node_last_complete_ms[n])));
			  }
		#endif
        // === [NEW] وقتی state عوض شد، یک packet برای UI بفرست ===
        // با آخرین raw data موجود و flags جدید
        {
          uint8_t uart_flags = BuildNodeFlags(n);


			  if (SerialLink_SendNode32_Async((uint8_t)n,
											  (uint16_t)cycles_ok[n],
											  uart_flags,
											  sensors32[n]) != pdPASS)
			  {
				// === [DEBUG] اگر خواستی، این print را نگه دار ===
				printf("NodeMonitorTask: UI resend failed for node %u\r\n", n);
			  }

        }

        // update previous state after successful processing
        prev_state[n] = new_state;
      }
    }


    osDelay(monitor_period_ms);
  }
}
/* USER CODE END Application */

