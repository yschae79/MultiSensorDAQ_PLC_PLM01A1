/**
 ******************************************************************************
 * @file    cube_hal.h
 * @brief   ST7580 샘플 라이브러리 호환 HAL 래퍼 헤더
 * @details ST7580_Serial.h / stm32_plm01a1.h 가 요구하는 cube_hal.h.
 *          샘플(F401RE)의 cube_hal.h를 NUCLEO-F446RE 프로젝트 구조에 맞게 대체.
 ******************************************************************************
 */

#ifndef _CUBE_HAL_H_
#define _CUBE_HAL_H_

#ifdef __cplusplus
extern "C" {
#endif

/* USE_STM32F4XX_NUCLEO 는 CMakeLists.txt 에서 전역 define으로 설정됨 */
#ifdef USE_STM32F4XX_NUCLEO
  #include "stm32f4xx_hal.h"
  #include "stm32f4xx.h"
  #include "stm32f4xx_it.h"
  #include "stm32f4xx_nucleo.h"
#endif

void SystemClock_Config(void);

#ifdef __cplusplus
}
#endif

#endif /* _CUBE_HAL_H_ */
