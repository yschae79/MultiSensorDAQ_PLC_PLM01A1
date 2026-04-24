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
    uint32_t rx_fail_wrong;     /**< 잘못된 ACK ID 수신 횟수 (진짜 오류) */
    uint32_t rx_stale;          /**< 이전 iteration ACK 지연 도착 폐기 횟수 */
    char     last_tx_id;        /**< 마지막 전송 ID ('A'~'Z', 초기값 '-') */
    char     last_rx_id;        /**< 마지막 수신 ACK/TRIGGER ID */
    uint32_t uart_pe;           /**< USART1 패리티 에러 횟수 */
    uint32_t uart_fe;           /**< USART1 프레임 에러 횟수 */
    uint32_t uart_ne;           /**< USART1 노이즈 에러 횟수 */
    uint32_t uart_oe;           /**< USART1 오버런 에러 횟수 */
    uint32_t last_ack_latency_ms; /**< 마지막 ACK 수신까지 걸린 시간 [ms] (TRIGGER 전송~ACK 수신) */
    float    per;               /**< 패킷 에러율 [%] = (TMO+ERR)/TX*100 */
} PLC_Stats_t;

/* Exported constants --------------------------------------------------------*/
#define TEST_PAYLOAD_SIZE    21u    /**< 페이로드 크기 (최대 DL_DATALEN_MAX=242) */
#define ACK_POLL_COUNT       25u    /**< ACK 폴링 최대 횟수 (25 × 200ms = 5,000ms) */
#define ACK_POLL_INTERVAL_MS 200u   /**< ACK 폴링 간격 [ms] */

/* Exported function prototypes ----------------------------------------------*/
void P2P_Init(void);
void P2P_Process(void);
const PLC_Stats_t *P2P_GetStats(void);
SM_State_t P2P_GetRole(void);
void P2P_UART1_ErrorInc(uint8_t pe, uint8_t fe, uint8_t ne, uint8_t oe);

#ifdef __cplusplus
}
#endif

#endif /* __C_PLC_APPLI_H */
