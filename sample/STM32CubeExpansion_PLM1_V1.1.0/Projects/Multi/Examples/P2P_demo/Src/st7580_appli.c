/**
******************************************************************************
* @file    st7580_appli.c
* @author  CLAB
* @version 1.1.0
* @date    18-Sept-2017
* @brief   user file to configure ST7580 PLC Modem.
*         
@verbatim
===============================================================================
##### How to use this driver #####
===============================================================================
[..]
This file is generated automatically by STM32CubeMX and eventually modified 
by the user

@endverbatim
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

/* Includes ------------------------------------------------------------------*/
#include <string.h>
#include "cube_hal.h"
#include "st7580_appli.h"
#include "stm32_plm01a1.h"

/** @addtogroup USER
* @{
*/

/** @defgroup ST7580_APPLI
* @brief User file to configure ST7580 PLC modem.
* @{
*/

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/
#define TRIG_BUF_SIZE   21
#define ACK_BUF_SIZE   	17
/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
SM_State_t SM_State;
char MsgOut[100];

/* Private function prototypes -----------------------------------------------*/
void AppliMasterBoard(void);
void AppliSlaveBoard(void);

/* Private functions ---------------------------------------------------------*/

/** @defgroup ST7580_APPLI_Private_Functions
* @{
*/

/**
* @brief  This function initializes the point-to-point communication
* @param  None
* @retval None
*/
void P2P_Init(void){
	
	/* Modem MIBs configuration */
  BSP_PLM_Mib_Write(MIB_MODEM_CONF, modem_config, sizeof(modem_config));
	HAL_Delay(500);
	
  /* Phy MIBs configuration */
  BSP_PLM_Mib_Write(MIB_PHY_CONF, phy_config, sizeof(phy_config));
	HAL_Delay(500);
	
	
	/* Check User Button state */
	if (BSP_PB_GetState(BUTTON_KEY) == GPIO_PIN_SET)
	{
		/* User Button released */
		SM_State = SM_STATE_SLAVE; 
	}
	else
	{
		/* User Button pressed */
	  SM_State = SM_STATE_MASTER;
	}
	
	return;
}

/**
* @brief  ST7580 P2P Process State machine
* @retval None.
*/
void P2P_Process() {
	switch(SM_State){
		case SM_STATE_MASTER:
		AppliMasterBoard();
		break;
		
		case SM_STATE_SLAVE:
		AppliSlaveBoard();
		break;
	}
	return;
}

/**
* @brief  This function handles the point-to-point Master Board Communication
* @retval None
*/
void AppliMasterBoard(){
	uint8_t ret;
	uint8_t cRxLen;
	ST7580Frame* RxFrame;
	uint8_t lastIDRcv = 0;
	int it = 0;
	
	uint8_t aTrsBuffer[TRIG_BUF_SIZE] = {'T','R','I','G','G','E','R',' ',\
																						'M','E','S','S','A','G','E',' ',\
																						'I','D',':',' ','@'};
	uint8_t aRcvBuffer[ACK_BUF_SIZE];

	sprintf(MsgOut, "P2P Communication Test - Master Board Side\n\r\n\r");
	HAL_UART_Transmit(&pUartMsgHandle, (uint8_t *)MsgOut, strlen(MsgOut), 500);
	
	while(1)
	{	
		/* Initialize Trigger Msg */
		aTrsBuffer[TRIG_BUF_SIZE-1]++;
		if (aTrsBuffer[TRIG_BUF_SIZE-1] > 'Z')
		{
			aTrsBuffer[TRIG_BUF_SIZE-1] = 'A';
		}
	
	sprintf(MsgOut, "Iteration %d\n\r", ++it);
	HAL_UART_Transmit(&pUartMsgHandle, (uint8_t *)MsgOut, strlen(MsgOut), 500);
		
		/* Send Trigger Msg send */
		ret = BSP_PLM_Send_Data(DATA_OPT, aTrsBuffer, TRIG_BUF_SIZE, NULL);

		/* Check TRIGGER Msg send result */
		if(ret)
		{
			/* Transmission Error */
			sprintf( MsgOut, "Trigger Transmission Err\n\r");
			HAL_UART_Transmit( &pUartMsgHandle, (uint8_t *)MsgOut, strlen(MsgOut), 500 );
			continue;
		}
		
		sprintf( MsgOut, "Trigger Msg Sent, ID: %c\n\r", aTrsBuffer[TRIG_BUF_SIZE-1]);
		HAL_UART_Transmit( &pUartMsgHandle, (uint8_t *)MsgOut, strlen(MsgOut), 500 );
		sprintf( MsgOut, "PAYLOAD: ");
		HAL_UART_Transmit( &pUartMsgHandle, (uint8_t *)MsgOut, strlen(MsgOut), 500 );
		HAL_UART_Transmit( &pUartMsgHandle, aTrsBuffer, TRIG_BUF_SIZE, 500 );
		sprintf( MsgOut, "\n\r");
		HAL_UART_Transmit( &pUartMsgHandle, (uint8_t *)MsgOut, strlen(MsgOut), 500 );
		
		/* Wait ACK Msg sent back from slave */
		RxFrame=NULL;
		for (int j=0;((j<10) && (RxFrame==NULL));j++)
		{
			RxFrame = BSP_PLM_Receive_Frame();
			if (RxFrame != NULL)
			{
				/* Check if a duplicated indication frame with STX = 03 is received */
				if ((RxFrame->stx == ST7580_STX_03)&&(lastIDRcv == RxFrame->data[3+ACK_BUF_SIZE]))
				{
					RxFrame = NULL;
				}
				else
				{
					lastIDRcv = RxFrame->data[3+ACK_BUF_SIZE];
					break;
				}
			}	
			HAL_Delay(200);
		}
		/* Check received ACK Msg */
		if (RxFrame == NULL)
		{
			/* No ACK Msg received until timeout */
			sprintf( MsgOut, "ACK Timeout - No ACK Received\n\r");
			HAL_UART_Transmit( &pUartMsgHandle, (uint8_t *)MsgOut, strlen(MsgOut), 500 );
			continue;
		}
		
		cRxLen = (RxFrame->length - 4);
		
		sprintf( MsgOut, "ACK Msg Received\n\r");
		HAL_UART_Transmit( &pUartMsgHandle, (uint8_t *)MsgOut, strlen(MsgOut), 500 );

		if (cRxLen != ACK_BUF_SIZE){
			/* ACK len mismatch */
			sprintf( MsgOut, "Wrong ACK Length\n\r");
			HAL_UART_Transmit( &pUartMsgHandle, (uint8_t *)MsgOut, strlen(MsgOut), 500 );
			continue;
		}
		
		/* Copy payload from RX frame */
		memcpy(aRcvBuffer,&(RxFrame->data[4]),cRxLen);

		/* Check ID to verify if the right ACK has been received */
		if (aRcvBuffer[ACK_BUF_SIZE-1] == aTrsBuffer[TRIG_BUF_SIZE-1])
		{
			sprintf( MsgOut, "ACK Msg Received, ID: %c\n\r", aRcvBuffer[ACK_BUF_SIZE-1]);
			HAL_UART_Transmit( &pUartMsgHandle, (uint8_t *)MsgOut, strlen(MsgOut), 500 );
		}
		else
		{
			sprintf( MsgOut, "WRONG ACK Msg Received, ID: %c\n\r", aRcvBuffer[ACK_BUF_SIZE-1]);
			HAL_UART_Transmit( &pUartMsgHandle, (uint8_t *)MsgOut, strlen(MsgOut), 500 );
		}
		sprintf( MsgOut, "PAYLOAD: ");
		HAL_UART_Transmit( &pUartMsgHandle, (uint8_t *)MsgOut, strlen(MsgOut), 500 );
		HAL_UART_Transmit( &pUartMsgHandle, aRcvBuffer, ACK_BUF_SIZE, 500 );
		sprintf( MsgOut, "\n\r\n\r");
		HAL_UART_Transmit( &pUartMsgHandle, (uint8_t *)MsgOut, strlen(MsgOut), 500 );

		HAL_Delay(1000);
	}
		
}

/**
* @brief  This function handles the point-to-point Slave Board Communication
* @retval None
*/
void AppliSlaveBoard(){
	ST7580Frame* RxFrame;
	uint8_t cRxLen;
	int ret;
	uint8_t lastIDRcv = 0;
	int it =0;
	
	uint8_t aTrsBuffer[ACK_BUF_SIZE] = {'A','C','K',' ','M','E','S','S',\
																						'A','G','E',' ','I','D',':',' ',\
																						'@'};
	uint8_t aRcvBuffer[TRIG_BUF_SIZE];
	
	sprintf( MsgOut, "P2P Communication Test - Slave Board Side\n\r\n\r");
	HAL_UART_Transmit( &pUartMsgHandle, (uint8_t *)MsgOut, strlen(MsgOut), 500 );
	
	while(1)
	{
		sprintf(MsgOut, "Iteration %d\n\r", ++it);
		HAL_UART_Transmit(&pUartMsgHandle, (uint8_t *)MsgOut, strlen(MsgOut), 500);
		
		/* Receive Trigger Msg from M board */
		RxFrame=NULL;
		do
		{
			RxFrame = BSP_PLM_Receive_Frame();
			
			if (RxFrame != NULL)
			{
				/* Check if a duplicated indication frame with STX = 03 is received */
				if ((RxFrame->stx == ST7580_STX_03)&&(lastIDRcv == RxFrame->data[3+TRIG_BUF_SIZE]))
				{
					RxFrame = NULL;
				}
				else
				{
					lastIDRcv = RxFrame->data[3+TRIG_BUF_SIZE];
					break;
				}
			}
			HAL_Delay(200);
		} while(RxFrame==NULL);
	
		cRxLen = (RxFrame->length - 4);
		memcpy(aRcvBuffer,&(RxFrame->data[4]),cRxLen);
		
		sprintf( MsgOut, "Trigger Msg Received, ID: %c\n\r", aRcvBuffer[TRIG_BUF_SIZE-1]);
		HAL_UART_Transmit( &pUartMsgHandle, (uint8_t *)MsgOut, strlen(MsgOut), 500 );
		sprintf( MsgOut, "PAYLOAD: ");
		HAL_UART_Transmit( &pUartMsgHandle, (uint8_t *)MsgOut, strlen(MsgOut), 500 );
		HAL_UART_Transmit( &pUartMsgHandle, aRcvBuffer, TRIG_BUF_SIZE, 500 );
		sprintf( MsgOut, "\n\r");
		HAL_UART_Transmit( &pUartMsgHandle, (uint8_t *)MsgOut, strlen(MsgOut), 500 );
		

		/* Send back ACK Msg to Master Board */
		aTrsBuffer[ACK_BUF_SIZE-1] = aRcvBuffer[TRIG_BUF_SIZE-1];
		do
		{
			ret = BSP_PLM_Send_Data(DATA_OPT, aTrsBuffer, ACK_BUF_SIZE, NULL);
		} while (ret!=0);
		
		sprintf( MsgOut, "ACK Msg Sent, ID: %c\n\r",aTrsBuffer[ACK_BUF_SIZE-1]);
		HAL_UART_Transmit( &pUartMsgHandle, (uint8_t *)MsgOut, strlen(MsgOut), 500 );
		sprintf( MsgOut, "PAYLOAD: ");
		HAL_UART_Transmit( &pUartMsgHandle, (uint8_t *)MsgOut, strlen(MsgOut), 500 );
		HAL_UART_Transmit( &pUartMsgHandle, aTrsBuffer, ACK_BUF_SIZE, 500 );
		sprintf( MsgOut, "\n\r\n\r");
		HAL_UART_Transmit( &pUartMsgHandle, (uint8_t *)MsgOut, strlen(MsgOut), 500 );
}

}
/**
* @}
*/

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
