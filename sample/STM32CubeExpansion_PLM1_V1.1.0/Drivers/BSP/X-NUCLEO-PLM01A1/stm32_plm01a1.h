/**
******************************************************************************
* @file    stm32_plm01a1.h
* @author  CLAB
* @version 1.1.0
* @date    18-Sept-2017
* @brief   Header file for HAL related functionality of X-CUBE-PLM1
******************************************************************************
* @attention
*
* <h2><center>&copy; COPYRIGHT(c) 2017 STMicroelectronics</center></h2>
*
* Redistribution and use in source and binary forms, with or without modification,
* are permitted provided that the following conditions are met:
*   1. Redistributions of source code must retain the above copyright notice,
*      this list of conditions and the following disclaimer.
*   2. Redistributions in binary form must reproduce the above copyright notice,
*      this list of conditions and the following disclaimer in the documentation
*      and/or other materials provided with the distribution.
*   3. Neither the name of STMicroelectronics nor the names of its contributors
*      may be used to endorse or promote products derived from this software
*      without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
* FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
* DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
* SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
* CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
* OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*
******************************************************************************
*/

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __PLM_MODULE_CONF_H
#define __PLM_MODULE_CONF_H

/* Includes ------------------------------------------------------------------*/

#ifdef USE_STM32L0XX_NUCLEO
#include "stm32l0xx_hal.h"
#include "stm32l0xx_nucleo.h"
#include "stm32l0xx_hal_rcc.h"
#include "stm32l0xx_hal_rcc_ex.h"
#include "stm32l0xx_ll_usart.h"
#endif

#ifdef USE_STM32F4XX_NUCLEO  
#include "stm32f4xx_hal.h"
#include "stm32f4xx_nucleo.h"
#include "stm32f4xx_hal_rcc.h"
#include "stm32f4xx_hal_rcc_ex.h"
#include "stm32f4xx_ll_usart.h"
#endif

#include "ST7580_Serial.h"
/* Exported types ------------------------------------------------------------*/

typedef struct sPlmDriver
{
    void( *Init )();
    int ( *Reset )();
	  int ( *MibRead )(uint8_t , uint8_t * , uint8_t );
	  int ( *MibWrite )(uint8_t, const uint8_t*,  uint8_t );
	  int ( *MibErase )(uint8_t index );
	  int ( *Ping )(const uint8_t*, uint8_t );
	  int ( *PhyData )(uint8_t, const uint8_t*, uint8_t, uint8_t* );
	  int ( *DlData )(uint8_t, const uint8_t*, uint8_t, uint8_t* );
	  int ( *SsData )(uint8_t, const uint8_t*, uint8_t, uint8_t, uint8_t* );
	  ST7580Frame * ( *NextIndicationFrame )();
}PlmDriver_t;

/* Exported constants --------------------------------------------------------*/
  
  
/* Exported macro ------------------------------------------------------------*/


/* PLM control usart */	
#define PLM_USART_RxBufferSize 512

#define PLM_USART                           USART1
#define PLM_USART_BAUDRATE                  57600
#define PLM_USART_CLK_ENABLE()              __USART1_CLK_ENABLE();
#define PLM_USART_RX_GPIO_CLK_ENABLE()      __GPIOA_CLK_ENABLE()
#define PLM_USART_TX_GPIO_CLK_ENABLE()      __GPIOA_CLK_ENABLE()

#define PLM_USART_FORCE_RESET()             __USART1_FORCE_RESET()
#define PLM_USART_RELEASE_RESET()           __USART1_RELEASE_RESET()

/* Definition for USARTx Pins */
#define PLM_USART_TX_PIN                    GPIO_PIN_9
#define PLM_USART_TX_GPIO_PORT              GPIOA

#define PLM_USART_RX_PIN                    GPIO_PIN_10
#define PLM_USART_RX_GPIO_PORT              GPIOA

/* Definition for USARTx's NVIC */
#define PLM_USART_IRQn                      USART1_IRQn
#define PLM_USART_IRQHandler                USART1_IRQHandler

#if defined(USE_STM32F4XX_NUCLEO)
#define PLM_USART_TX_AF                     GPIO_AF7_USART1 
#define PLM_USART_RX_AF                     GPIO_AF7_USART1
#elif defined(USE_STM32L0XX_NUCLEO)
#define PLM_USART_TX_AF                     GPIO_AF4_USART1 
#define PLM_USART_RX_AF                     GPIO_AF4_USART1
#endif


/* Message debug usart */
#define MSG_USART                           USART2
#define MSG_USART_BAUDRATE                  115200
#define MSG_USART_CLK_ENABLE()              __USART2_CLK_ENABLE();
#define MSG_USART_RX_GPIO_CLK_ENABLE()      __GPIOA_CLK_ENABLE()
#define MSG_USART_TX_GPIO_CLK_ENABLE()      __GPIOA_CLK_ENABLE()

#define MSG_USART_FORCE_RESET()             __USART2_FORCE_RESET()
#define MSG_USART_RELEASE_RESET()           __USART2_RELEASE_RESET()

/* Definition for USARTx Pins */
#define MSG_USART_TX_PIN                    GPIO_PIN_2
#define MSG_USART_TX_GPIO_PORT              GPIOA

#define MSG_USART_RX_PIN                    GPIO_PIN_3
#define MSG_USART_RX_GPIO_PORT              GPIOA

#if defined(USE_STM32F4XX_NUCLEO)
#define MSG_USART_TX_AF                     GPIO_AF7_USART2 
#define MSG_USART_RX_AF                     GPIO_AF7_USART2

#elif defined(USE_STM32L0XX_NUCLEO)
#define MSG_USART_TX_AF                     GPIO_AF4_USART2 
#define MSG_USART_RX_AF                     GPIO_AF4_USART2
#endif

/* PLM Gpio */
#define PLM_GPIO_T_REQ_PORT                         GPIOA
#define PLM_GPIO_T_REQ_PIN                          GPIO_PIN_5
#define PLM_GPIO_T_REQ_CLOCK_ENABLE()               __GPIOA_CLK_ENABLE()
#define PLM_GPIO_T_REQ_CLOCK_DISABLE()              __GPIOA_CLK_DISABLE()   
#define PLM_GPIO_T_REQ_SPEED                        GPIO_SPEED_HIGH
#define PLM_GPIO_T_REQ_PUPD                         GPIO_NOPULL

/******************************************************************************/

#define PLM_GPIO_RESETN_PORT                        GPIOA
#define PLM_GPIO_RESETN_PIN                         GPIO_PIN_8
#define PLM_GPIO_RESETN_CLOCK_ENABLE()              __GPIOA_CLK_ENABLE()
#define PLM_GPIO_RESETN_CLOCK_DISABLE()             __GPIOA_CLK_DISABLE()   
#define PLM_GPIO_RESETN_SPEED                       GPIO_SPEED_HIGH
#define PLM_GPIO_RESETN_PUPD                        GPIO_NOPULL

/******************************************************************************/

#define PLM_PL_TX_ON_PORT                          	GPIOC
#define PLM_PL_TX_ON_PIN                            GPIO_PIN_0
#define PLM_PL_TX_ON_CLOCK_ENABLE()                 __GPIOC_CLK_ENABLE()
#define PLM_PL_TX_ON_CLOCK_DISABLE()                __GPIOC_CLK_DISABLE()   
#define PLM_PL_TX_ON_SPEED                          GPIO_SPEED_HIGH
#define PLM_PL_TX_ON_PUPD                           GPIO_NOPULL

/******************************************************************************/

#define PLM_PL_RX_ON_PORT                           GPIOC
#define PLM_PL_RX_ON_PIN                            GPIO_PIN_1
#define PLM_PL_RX_ON_CLOCK_ENABLE()                 __GPIOC_CLK_ENABLE()
#define PLM_PL_RX_ON_CLOCK_DISABLE()                __GPIOC_CLK_DISABLE()   
#define PLM_PL_RX_ON_SPEED                          GPIO_SPEED_HIGH
#define PLM_PL_RX_ON_PUPD                           GPIO_NOPULL
#define PLM_PL_RX_ON_EXTI_LINE                      GPIO_PIN_1
#define PLM_PL_RX_ON_EXTI_MODE                      GPIO_MODE_IT_RISING_FALLING
//#define PLM_PL_RX_ON_EXTI_IRQN                      EXTI1_IRQn 
#define PLM_PL_RX_ON_EXTI_PREEMPTION_PRIORITY       2
#define PLM_PL_RX_ON_EXTI_SUB_PRIORITY              2
#define PLM_PL_RX_ON_EXTI_IRQ_HANDLER               EXTI1_IRQHandler


/* Exported Variables --------------------------------------------------------*/
extern UART_HandleTypeDef pUartPlmHandle; 
extern UART_HandleTypeDef pUartMsgHandle;

extern PlmDriver_t *pPlmDriver;

void GPIO_PLM_Configuration(void);
void UART_PLM_Configuration(void);
void USART_PRINT_MSG_Configuration(void);

void BSP_PLM_Init(void);
int BSP_PLM_Reset(void);
int BSP_PLM_Mib_Write(uint8_t indexMib, const uint8_t* bufMib, uint8_t lenBuf);
int BSP_PLM_Mib_Read(uint8_t indexMib, uint8_t* bufMib, uint8_t lenBuf);
int BSP_PLM_Mib_Erase(uint8_t indexMib);
int BSP_PLM_Ping(const uint8_t* pingBuf, uint8_t pingLen);
int BSP_PLM_Send_Data(uint8_t plmOpts, const uint8_t* dataBuf, uint8_t dataLen, uint8_t* confData);
int BSP_PLM_Send_Secure_data(uint8_t plmOpts, const uint8_t* dataBuf, uint8_t clrLen, uint8_t encLen, uint8_t* retData);
ST7580Frame *BSP_PLM_Receive_Frame(void);
#endif //__PLM_MODULE_CONF_H
