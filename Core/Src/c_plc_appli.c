/**
 ******************************************************************************
 * @file    c_plc_appli.c
 * @brief   ST7580 PLC P2P 애플리케이션 구현
 * @details X-NUCLEO-PLM01A1 + NUCLEO-F446RE 기반 전력선 통신 P2P 테스트.
 *          샘플 펌웨어(st7580_appli.c, F401RE 기준)를 F446RE 환경에 맞게 포팅.
 *
 *          동작 방식:
 *          - 부팅 시 USER 버튼(PC13) 상태로 역할 결정
 *            * 버튼 누름(GPIO_PIN_RESET) → MASTER: TRIGGER 메시지 전송
 *            * 버튼 미누름(GPIO_PIN_SET)  → SLAVE : TRIGGER 수신 후 ACK 응답
 ******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/
#include <stdio.h>
#include <string.h>
#include "c_plc_appli.h"
#include "stm32_plm01a1.h"

/* Private defines -----------------------------------------------------------*/
/** @brief TRIGGER 메시지 페이로드 크기 (바이트) */
#define TRIG_BUF_SIZE   21
/** @brief ACK 메시지 페이로드 크기 (바이트) */
#define ACK_BUF_SIZE    17

/* Private variables ---------------------------------------------------------*/
/** @brief P2P 상태 머신 현재 상태 */
static SM_State_t SM_State;

/** @brief PLC 신뢰성 테스트 통계 */
static PLC_Stats_t s_stats;

/** @brief 디버그 UART 출력 버퍼 */
static char MsgOut[100];

/* Private function prototypes -----------------------------------------------*/
static void AppliMasterBoard(void);
static void AppliSlaveBoard(void);

/* Exported functions --------------------------------------------------------*/

/**
 * @brief  P2P 통신 초기화
 * @details MIB 설정 쓰기 후 USER 버튼 상태로 MASTER/SLAVE 역할 결정
 * @retval None
 */
void P2P_Init(void)
{    /* 통계 초기화 */
    s_stats.tx_count        = 0u;
    s_stats.rx_success      = 0u;
    s_stats.rx_fail_timeout = 0u;
    s_stats.rx_fail_wrong   = 0u;
    s_stats.per             = 0.0f;
    /* 모뎀 MIB 설정 쓰기 */
    BSP_PLM_Mib_Write(MIB_MODEM_CONF, modem_config, sizeof(modem_config));
    HAL_Delay(500);

    /* PHY MIB 설정 쓰기 */
    BSP_PLM_Mib_Write(MIB_PHY_CONF, phy_config, sizeof(phy_config));
    HAL_Delay(500);

    /* USER 버튼(PC13) 상태로 역할 결정
     * GPIO_PIN_RESET = 버튼 누름  → MASTER
     * GPIO_PIN_SET   = 버튼 미누름 → SLAVE   */
    if (BSP_PB_GetState(BUTTON_KEY) == GPIO_PIN_SET)
    {
        SM_State = SM_STATE_SLAVE;
    }
    else
    {
        SM_State = SM_STATE_MASTER;
    }
}

/**
 * @brief  P2P 상태 머신 실행 (메인 루프에서 반복 호출)
 * @retval None
 */
void P2P_Process(void)
{
    switch (SM_State)
    {
        case SM_STATE_MASTER:
            AppliMasterBoard();
            break;

        case SM_STATE_SLAVE:
            AppliSlaveBoard();
            break;

        default:
            break;
    }
}

/**
 * @brief  PLC 통신 통계 반환
 * @retval 통계 구조체 포인터 (read-only)
 */
const PLC_Stats_t *P2P_GetStats(void)
{
    return &s_stats;
}

SM_State_t P2P_GetRole(void)
{
    return SM_State;
}

/* Private functions ---------------------------------------------------------*/

/**
 * @brief  마스터 보드 동작: TRIGGER 전송 → ACK 수신 대기
 * @retval None
 */
static void AppliMasterBoard(void)
{
    uint8_t ret;
    uint8_t cRxLen;
    ST7580Frame *RxFrame;
    uint8_t lastIDRcv = 0;
    int it = 0;

    /* TRIGGER 메시지 버퍼: 마지막 바이트가 순환 ID('@' 이후 A~Z) */
    uint8_t aTrsBuffer[TRIG_BUF_SIZE] = {
        'T','R','I','G','G','E','R',' ',
        'M','E','S','S','A','G','E',' ',
        'I','D',':',' ','@'
    };
    uint8_t aRcvBuffer[ACK_BUF_SIZE];

    sprintf(MsgOut, "P2P Communication Test - Master Board Side\n\r\n\r");
    HAL_UART_Transmit(&pUartMsgHandle, (uint8_t *)MsgOut, strlen(MsgOut), 500);

    while (1)
    {
        /* ID 바이트 순환 증가 (A ~ Z) */
        aTrsBuffer[TRIG_BUF_SIZE - 1]++;
        if (aTrsBuffer[TRIG_BUF_SIZE - 1] > 'Z')
        {
            aTrsBuffer[TRIG_BUF_SIZE - 1] = 'A';
        }

        sprintf(MsgOut, "Iteration %d\n\r", ++it);
        HAL_UART_Transmit(&pUartMsgHandle, (uint8_t *)MsgOut, strlen(MsgOut), 500);

        /* TRIGGER 메시지 전송 */
        ret = BSP_PLM_Send_Data(DATA_OPT, aTrsBuffer, TRIG_BUF_SIZE, NULL);

        if (ret)
        {
            sprintf(MsgOut, "Trigger Transmission Err\n\r");
            HAL_UART_Transmit(&pUartMsgHandle, (uint8_t *)MsgOut, strlen(MsgOut), 500);
            continue;
        }

        sprintf(MsgOut, "Trigger Msg Sent, ID: %c\n\r", aTrsBuffer[TRIG_BUF_SIZE - 1]);
        HAL_UART_Transmit(&pUartMsgHandle, (uint8_t *)MsgOut, strlen(MsgOut), 500);
        sprintf(MsgOut, "PAYLOAD: ");
        HAL_UART_Transmit(&pUartMsgHandle, (uint8_t *)MsgOut, strlen(MsgOut), 500);
        HAL_UART_Transmit(&pUartMsgHandle, aTrsBuffer, TRIG_BUF_SIZE, 500);
        sprintf(MsgOut, "\n\r");
        HAL_UART_Transmit(&pUartMsgHandle, (uint8_t *)MsgOut, strlen(MsgOut), 500);

        /* ACK 수신 대기 (최대 10회 폴링, 200ms 간격) */
        RxFrame = NULL;
        for (int j = 0; (j < 10) && (RxFrame == NULL); j++)
        {
            RxFrame = BSP_PLM_Receive_Frame();
            if (RxFrame != NULL)
            {
                /* STX=0x03 중복 IND 프레임 필터 */
                if ((RxFrame->stx == ST7580_STX_03) &&
                    (lastIDRcv == RxFrame->data[3 + ACK_BUF_SIZE]))
                {
                    RxFrame = NULL;
                }
                else
                {
                    lastIDRcv = RxFrame->data[3 + ACK_BUF_SIZE];
                    break;
                }
            }
            HAL_Delay(200);
        }

        /* ACK 수신 결과 확인 */
        if (RxFrame == NULL)
        {
            sprintf(MsgOut, "ACK Timeout - No ACK Received\n\r");
            HAL_UART_Transmit(&pUartMsgHandle, (uint8_t *)MsgOut, strlen(MsgOut), 500);
            continue;
        }

        cRxLen = (RxFrame->length - 4);

        sprintf(MsgOut, "ACK Msg Received\n\r");
        HAL_UART_Transmit(&pUartMsgHandle, (uint8_t *)MsgOut, strlen(MsgOut), 500);

        if (cRxLen != ACK_BUF_SIZE)
        {
            sprintf(MsgOut, "Wrong ACK Length\n\r");
            HAL_UART_Transmit(&pUartMsgHandle, (uint8_t *)MsgOut, strlen(MsgOut), 500);
            continue;
        }

        /* 페이로드 추출 */
        memcpy(aRcvBuffer, &(RxFrame->data[4]), cRxLen);

        /* ID 검증 */
        if (aRcvBuffer[ACK_BUF_SIZE - 1] == aTrsBuffer[TRIG_BUF_SIZE - 1])
        {
            sprintf(MsgOut, "ACK Msg Received, ID: %c\n\r", aRcvBuffer[ACK_BUF_SIZE - 1]);
        }
        else
        {
            sprintf(MsgOut, "WRONG ACK Msg Received, ID: %c\n\r", aRcvBuffer[ACK_BUF_SIZE - 1]);
        }
        HAL_UART_Transmit(&pUartMsgHandle, (uint8_t *)MsgOut, strlen(MsgOut), 500);
        sprintf(MsgOut, "PAYLOAD: ");
        HAL_UART_Transmit(&pUartMsgHandle, (uint8_t *)MsgOut, strlen(MsgOut), 500);
        HAL_UART_Transmit(&pUartMsgHandle, aRcvBuffer, ACK_BUF_SIZE, 500);
        sprintf(MsgOut, "\n\r\n\r");
        HAL_UART_Transmit(&pUartMsgHandle, (uint8_t *)MsgOut, strlen(MsgOut), 500);

        HAL_Delay(1000);
    }
}

/**
 * @brief  슬레이브 보드 동작: TRIGGER 수신 대기 → ACK 전송
 * @retval None
 */
static void AppliSlaveBoard(void)
{
    ST7580Frame *RxFrame;
    uint8_t cRxLen;
    int ret;
    uint8_t lastIDRcv = 0;
    int it = 0;

    /* ACK 메시지 버퍼: 마지막 바이트에 수신된 TRIGGER ID를 복사 */
    uint8_t aTrsBuffer[ACK_BUF_SIZE] = {
        'A','C','K',' ','M','E','S','S',
        'A','G','E',' ','I','D',':',' ','@'
    };
    uint8_t aRcvBuffer[TRIG_BUF_SIZE];

    sprintf(MsgOut, "P2P Communication Test - Slave Board Side\n\r\n\r");
    HAL_UART_Transmit(&pUartMsgHandle, (uint8_t *)MsgOut, strlen(MsgOut), 500);

    while (1)
    {
        sprintf(MsgOut, "Iteration %d\n\r", ++it);
        HAL_UART_Transmit(&pUartMsgHandle, (uint8_t *)MsgOut, strlen(MsgOut), 500);

        /* TRIGGER 메시지 수신 대기 */
        RxFrame = NULL;
        do
        {
            RxFrame = BSP_PLM_Receive_Frame();
            if (RxFrame != NULL)
            {
                /* STX=0x03 중복 IND 프레임 필터 */
                if ((RxFrame->stx == ST7580_STX_03) &&
                    (lastIDRcv == RxFrame->data[3 + TRIG_BUF_SIZE]))
                {
                    RxFrame = NULL;
                }
                else
                {
                    lastIDRcv = RxFrame->data[3 + TRIG_BUF_SIZE];
                    break;
                }
            }
            HAL_Delay(200);
        } while (RxFrame == NULL);

        /* 페이로드 추출 */
        cRxLen = (RxFrame->length - 4);
        memcpy(aRcvBuffer, &(RxFrame->data[4]), cRxLen);

        sprintf(MsgOut, "Trigger Msg Received, ID: %c\n\r", aRcvBuffer[TRIG_BUF_SIZE - 1]);
        HAL_UART_Transmit(&pUartMsgHandle, (uint8_t *)MsgOut, strlen(MsgOut), 500);
        sprintf(MsgOut, "PAYLOAD: ");
        HAL_UART_Transmit(&pUartMsgHandle, (uint8_t *)MsgOut, strlen(MsgOut), 500);
        HAL_UART_Transmit(&pUartMsgHandle, aRcvBuffer, TRIG_BUF_SIZE, 500);
        sprintf(MsgOut, "\n\r");
        HAL_UART_Transmit(&pUartMsgHandle, (uint8_t *)MsgOut, strlen(MsgOut), 500);

        /* 수신된 TRIGGER ID를 ACK에 복사 후 전송 (최대 5회 재시도) */
        aTrsBuffer[ACK_BUF_SIZE - 1] = aRcvBuffer[TRIG_BUF_SIZE - 1];
        {
            int retry;
            for (retry = 0; retry < 5; retry++)
            {
                ret = BSP_PLM_Send_Data(DATA_OPT, aTrsBuffer, ACK_BUF_SIZE, NULL);
                if (ret == 0) { break; }
            }
        }

        sprintf(MsgOut, "ACK Msg Sent, ID: %c\n\r", aTrsBuffer[ACK_BUF_SIZE - 1]);
        HAL_UART_Transmit(&pUartMsgHandle, (uint8_t *)MsgOut, strlen(MsgOut), 500);
        sprintf(MsgOut, "PAYLOAD: ");
        HAL_UART_Transmit(&pUartMsgHandle, (uint8_t *)MsgOut, strlen(MsgOut), 500);
        HAL_UART_Transmit(&pUartMsgHandle, aTrsBuffer, ACK_BUF_SIZE, 500);
        sprintf(MsgOut, "\n\r\n\r");
        HAL_UART_Transmit(&pUartMsgHandle, (uint8_t *)MsgOut, strlen(MsgOut), 500);
    }
}
