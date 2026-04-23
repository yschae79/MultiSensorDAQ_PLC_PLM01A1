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

  /* PLC 태스크 생성 (우선순위 3, 1KB 스택) */
  ret = tx_byte_allocate(byte_pool, &p_stack, 1024u, TX_NO_WAIT);
  if (ret != TX_SUCCESS) { return ret; }
  static TX_THREAD s_plcTask;
  ret = tx_thread_create(&s_plcTask, "PLC Task", PLC_Task_Entry, 0u,
                         p_stack, 1024u,
                         3u, 3u, TX_NO_TIME_SLICE, TX_AUTO_START);
  if (ret != TX_SUCCESS) { return ret; }

  /* LCD 태스크 생성 (우선순위 5, 2KB 스택) */
  ret = tx_byte_allocate(byte_pool, &p_stack, 2048u, TX_NO_WAIT);
  if (ret != TX_SUCCESS) { return ret; }
  static TX_THREAD s_lcdTask;
  ret = tx_thread_create(&s_lcdTask, "LCD Task", LCD_Task_Entry, 0u,
                         p_stack, 2048u,
                         5u, 5u, TX_NO_TIME_SLICE, TX_AUTO_START);
  if (ret != TX_SUCCESS) { return ret; }

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
    char buf[40];

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
        LCD_DrawString(4u, 40u, "Waiting for TRIGGER", LCD_WHITE, LCD_BLACK);
        LCD_DrawString(4u, 60u, "from MASTER...",      LCD_GRAY,  LCD_BLACK);
        LCD_DrawString(4u, 90u, "RX  :    0",          LCD_GREEN, LCD_BLACK);

        uint32_t rx_cnt = 0u;
        for (;;)
        {
            /* SLAVE 수신 횟수: tx_count를 수신 카운터로 재활용 (P2P_Init에서 0으로 초기화) */
            /* AppliSlaveBoard에서 수신 카운터를 별도 제공하지 않으므로 폴링 방식으로 표시 */
            snprintf(buf, sizeof(buf), "RX  : %4lu", (unsigned long)rx_cnt);
            LCD_DrawString(4u, 90u, buf, LCD_GREEN, LCD_BLACK);
            tx_thread_sleep(500u);
        }
    }
    else
    {
        /* ── MASTER 화면 ── */
        LCD_SetFont(&Font_16x24);
        LCD_DrawString(4u, 4u, "PLC RELIABILITY", LCD_CYAN, LCD_BLACK);

        LCD_SetFont(&Font_8x16);
        LCD_DrawString(4u, 34u, "MODE   :", LCD_WHITE, LCD_BLACK);
        LCD_DrawString(4u, 50u, "PAYLOAD:", LCD_WHITE, LCD_BLACK);
        LCD_DrawString(4u,  72u, "-------------------", LCD_GRAY, LCD_BLACK);
        LCD_DrawString(4u,  88u, "TX  :", LCD_WHITE, LCD_BLACK);
        LCD_DrawString(4u, 104u, "OK  :", LCD_GREEN, LCD_BLACK);
        LCD_DrawString(4u, 120u, "TMO :", LCD_YELLOW, LCD_BLACK);
        LCD_DrawString(4u, 136u, "ERR :", LCD_RED,    LCD_BLACK);
        LCD_DrawString(4u, 152u, "PER :", LCD_ORANGE, LCD_BLACK);

        for (;;)
        {
            const PLC_Stats_t *s = P2P_GetStats();

            snprintf(buf, sizeof(buf), "TX  : %4lu / %4u",
                     (unsigned long)s->tx_count, (unsigned)TEST_PACKET_COUNT);
            LCD_DrawString(4u,  88u, buf, LCD_WHITE, LCD_BLACK);

            snprintf(buf, sizeof(buf), "OK  : %4lu", (unsigned long)s->rx_success);
            LCD_DrawString(4u, 104u, buf, LCD_GREEN, LCD_BLACK);

            snprintf(buf, sizeof(buf), "TMO : %4lu", (unsigned long)s->rx_fail_timeout);
            LCD_DrawString(4u, 120u, buf, LCD_YELLOW, LCD_BLACK);

            snprintf(buf, sizeof(buf), "ERR : %4lu", (unsigned long)s->rx_fail_wrong);
            LCD_DrawString(4u, 136u, buf, LCD_RED, LCD_BLACK);

            uint32_t per_int  = (uint32_t)s->per;
            uint32_t per_frac = (uint32_t)((s->per - (float)per_int) * 100.0f);
            snprintf(buf, sizeof(buf), "PER : %2lu.%02lu%%", (unsigned long)per_int, (unsigned long)per_frac);
            LCD_DrawString(4u, 152u, buf, LCD_ORANGE, LCD_BLACK);

            tx_thread_sleep(500u);
        }
    }
}

/* USER CODE END 1 */
