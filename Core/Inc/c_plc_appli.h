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

/**
 * @brief PLC 신뢰성 테스트 통계 구조체
 */
typedef struct {
    uint32_t tx_count;          /**< 총 전송 시도 횟수 */
    uint32_t rx_success;        /**< ACK 정상 수신 횟수 */
    uint32_t rx_fail_timeout;   /**< ACK 타임아웃 실패 횟수 */
    uint32_t rx_fail_wrong;     /**< 잘못된 ACK 수신 횟수 */
    float    per;               /**< 패킷 에러율 [%] */
} PLC_Stats_t;

/* Exported constants --------------------------------------------------------*/
#define TEST_PACKET_COUNT   100u    /**< 테스트 패킷 수 (1회 실험 기준) */
#define TEST_PAYLOAD_SIZE   21u     /**< 페이로드 크기 (최대 DL_DATALEN_MAX=242) */

/* Exported function prototypes ----------------------------------------------*/
void P2P_Init(void);
void P2P_Process(void);
const PLC_Stats_t *P2P_GetStats(void);
SM_State_t P2P_GetRole(void);

#ifdef __cplusplus
}
#endif

#endif /* __C_PLC_APPLI_H */
