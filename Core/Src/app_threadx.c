/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    app_threadx.c
  * @author  MCD Application Team
  * @brief   ThreadX applicative file
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2021 STMicroelectronics.
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
#include "app_threadx.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "c_plc_appli.h"
#include "c_lcd_ili9341.h"
#include "c_font.h"
#include "c_debug.h"
#include <stdio.h>
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

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */
static void PLC_Task_Entry(ULONG argument);
static void LCD_Task_Entry(ULONG argument);
/* USER CODE END PFP */

/**
  * @brief  Application ThreadX Initialization.
  * @param memory_ptr: memory pointer
  * @retval int
  */
UINT App_ThreadX_Init(VOID *memory_ptr)
{
  UINT ret = TX_SUCCESS;
  TX_BYTE_POOL *byte_pool = (TX_BYTE_POOL*)memory_ptr;

  /* USER CODE BEGIN App_ThreadX_Init */
  VOID *p_stack;

  /* 비동기 DMA 디버그 출력 초기화 — 커널 시작 후 여기서 호출해야 tx_mutex_create()가 정상 동작 */
  Debug_Init();

  /* PLC 태스크 생성 (우선순위 3, 2KB 스택) — UART 콜백/상태머신 중첩 호출로 1KB 부족 */
  ret = tx_byte_allocate(byte_pool, &p_stack, 2048u, TX_NO_WAIT);
  if (ret != TX_SUCCESS) { Error_Handler(); return ret; }
  static TX_THREAD s_plcTask;
  ret = tx_thread_create(&s_plcTask, "PLC Task", PLC_Task_Entry, 0u,
                         p_stack, 2048u,
                         3u, 3u, TX_NO_TIME_SLICE, TX_AUTO_START);
  if (ret != TX_SUCCESS) { Error_Handler(); return ret; }

  /* LCD 태스크 생성 (우선순위 5, 2KB 스택) */
  ret = tx_byte_allocate(byte_pool, &p_stack, 2048u, TX_NO_WAIT);
  if (ret != TX_SUCCESS) { Error_Handler(); return ret; }
  static TX_THREAD s_lcdTask;
  ret = tx_thread_create(&s_lcdTask, "LCD Task", LCD_Task_Entry, 0u,
                         p_stack, 2048u,
                         5u, 5u, TX_NO_TIME_SLICE, TX_AUTO_START);
  if (ret != TX_SUCCESS) { Error_Handler(); return ret; }

  /* USER CODE END App_ThreadX_Init */

  return ret;
}

/**
  * @brief  MX_ThreadX_Init
  * @param  None
  * @retval None
  */
void MX_ThreadX_Init(void)
{
  /* USER CODE BEGIN  Before_Kernel_Start */

  /* USER CODE END  Before_Kernel_Start */

  tx_kernel_enter();

  /* USER CODE BEGIN  Kernel_Start_Error */

  /* USER CODE END  Kernel_Start_Error */
}

/* USER CODE BEGIN 1 */

/**
 * @brief PLC 태스크 진입 함수
 * @note  P2P_Process()는 실행 시간이 길 수 있어 우선순위를 LCD보다 높게 설정
 */
static void PLC_Task_Entry(ULONG argument)
{
    (void)argument;
    for (;;)
    {
        P2P_Process();
    }
}

/**
 * @brief LCD 태스크 진입 함수
 * @note  LCD_Init()은 스케줄러 시작 후 태스크 내부에서만 호용 (내부에서 tx_thread_create 호출하므로)
 */
static void LCD_Task_Entry(ULONG argument)
{
    (void)argument;
    char buf[48];

    printf("[APP] LCD_Task started\n");
    LCD_Init();
    printf("[APP] LCD_Init done, sleeping 300ms\n");
    tx_thread_sleep(300u);    /* HW 초기화 완료 대기 (RST + 레지스터 초기화 시간) */

    SM_State_t role = P2P_GetRole();
    printf("[APP] role=%s\n", (role == SM_STATE_SLAVE) ? "SLAVE" : "MASTER");

    printf("[APP] FillScreen BLACK\n");
    LCD_FillScreen(LCD_BLACK);

    if (role == SM_STATE_SLAVE)
    {
        /* ── SLAVE 화면 ── */
        LCD_SetFont(&Font_16x24);
        LCD_DrawString(4u, 4u, "PLC SLAVE", LCD_YELLOW, LCD_BLACK);

        LCD_SetFont(&Font_8x16);
        LCD_DrawString(4u, 40u, "RX(-):    0 / TX(-):    0", LCD_GREEN,  LCD_BLACK);
        LCD_DrawString(4u, 58u, "Waiting for TRIGGER...",   LCD_GRAY,   LCD_BLACK);
        LCD_DrawString(4u, 90u, "--- UART1 Errors --------", LCD_GRAY,   LCD_BLACK);
        LCD_DrawString(4u, 108u, "PE:    0/FE:    0/NE:    0/ORE:    0", LCD_CYAN, LCD_BLACK);

        for (;;)
        {
            const PLC_Stats_t *s = P2P_GetStats();

            snprintf(buf, sizeof(buf), "RX(%c):%5lu / TX(%c):%5lu",
                     s->last_rx_id, (unsigned long)s->rx_success,
                     s->last_tx_id, (unsigned long)s->tx_count);
            LCD_DrawString(4u, 40u, buf, LCD_GREEN, LCD_BLACK);

            snprintf(buf, sizeof(buf), "PE:%4lu/FE:%4lu/NE:%4lu/ORE:%4lu",
                     (unsigned long)s->uart_pe, (unsigned long)s->uart_fe,
                     (unsigned long)s->uart_ne, (unsigned long)s->uart_oe);
            LCD_DrawString(4u, 108u, buf, LCD_CYAN, LCD_BLACK);

            tx_thread_sleep(500u);
        }
    }
    else
    {
        /* ── MASTER 화면 ── */
        LCD_SetFont(&Font_16x24);
        LCD_DrawString(4u, 4u, "PLC RELIABILITY", LCD_CYAN, LCD_BLACK);

        LCD_SetFont(&Font_8x16);
        LCD_DrawString(4u, 34u, "MODE:P2P  PAYLOAD:21B",    LCD_WHITE, LCD_BLACK);
        LCD_DrawString(4u, 52u, "------------------------", LCD_GRAY,  LCD_BLACK);
        /* 정적 라벨 (ID 부분만 동적 업데이트) */
        LCD_DrawString(4u,  70u, "TX(-):     0 / OK(-):     0 (  0.0%)", LCD_WHITE,  LCD_BLACK);
        LCD_DrawString(4u,  90u, "TMO:    0/STA:    0/ERR:    0 ( 0.0%)", LCD_YELLOW, LCD_BLACK);
        LCD_DrawString(4u, 110u, "--- UART1 Errors --------",             LCD_GRAY,   LCD_BLACK);
        LCD_DrawString(4u, 128u, "PE:    0/FE:    0/NE:    0/ORE:    0",  LCD_CYAN,   LCD_BLACK);
        LCD_DrawString(4u, 146u, "ACK:    --- ms",                         LCD_GREEN,  LCD_BLACK);

        for (;;)
        {
            const PLC_Stats_t *s = P2P_GetStats();

            float per_ok   = (s->tx_count > 0u)
                             ? (float)s->rx_success / (float)s->tx_count * 100.0f
                             : 0.0f;
            float per_fail = (s->tx_count > 0u)
                             ? (float)(s->rx_fail_timeout + s->rx_fail_wrong)
                               / (float)s->tx_count * 100.0f
                             : 0.0f;

            /* TX(id): nnnnn / OK(id): nnnnn (sss.s%) */
            snprintf(buf, sizeof(buf), "TX(%c):%5lu / OK(%c):%5lu (%5.1f%%)",
                     s->last_tx_id, (unsigned long)s->tx_count,
                     s->last_rx_id, (unsigned long)s->rx_success,
                     (double)per_ok);
            LCD_DrawString(4u, 70u, buf, LCD_WHITE, LCD_BLACK);

            /* TMO: n / STA: n / ERR: n (f.f%) */
            snprintf(buf, sizeof(buf), "TMO:%4lu/STA:%4lu/ERR:%4lu (%4.1f%%)",
                     (unsigned long)s->rx_fail_timeout,
                     (unsigned long)s->rx_stale,
                     (unsigned long)s->rx_fail_wrong,
                     (double)per_fail);
            LCD_DrawString(4u, 90u, buf, LCD_YELLOW, LCD_BLACK);

            /* PE / FE / NE / ORE */
            snprintf(buf, sizeof(buf), "PE:%4lu/FE:%4lu/NE:%4lu/ORE:%4lu",
                     (unsigned long)s->uart_pe, (unsigned long)s->uart_fe,
                     (unsigned long)s->uart_ne, (unsigned long)s->uart_oe);
            LCD_DrawString(4u, 128u, buf, LCD_CYAN, LCD_BLACK);

            /* ACK RTT */
            snprintf(buf, sizeof(buf), "ACK: %6lu ms", (unsigned long)s->last_ack_latency_ms);
            LCD_DrawString(4u, 146u, buf, LCD_GREEN, LCD_BLACK);

            tx_thread_sleep(500u);
        }
    }
}

/* USER CODE END 1 */
