/**
 ******************************************************************************
 * @file    c_plc_appli.h
 * @brief   ST7580 PLC P2P 애플리케이션 헤더
 * @details X-NUCLEO-PLM01A1 + NUCLEO-F446RE 기반 전력선 통신 P2P 테스트
 *          샘플 펌웨어(F401RE 기준) 포팅 버전
 ******************************************************************************
 */

#ifndef __C_PLC_APPLI_H
#define __C_PLC_APPLI_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"
#include "ST7580_Config.h"
#include "stm32_plm01a1.h"

/* Exported types ------------------------------------------------------------*/

/**
 * @brief P2P 상태 머신 열거형
 */
typedef enum {
    SM_STATE_MASTER = 0,    /**< 마스터 역할: 트리거 메시지 전송 */
    SM_STATE_SLAVE  = 0xFF  /**< 슬레이브 역할: 트리거 수신 후 ACK 응답 */
} SM_State_t;

/* Exported function prototypes ----------------------------------------------*/
void P2P_Init(void);
void P2P_Process(void);

#ifdef __cplusplus
}
#endif

#endif /* __C_PLC_APPLI_H */
