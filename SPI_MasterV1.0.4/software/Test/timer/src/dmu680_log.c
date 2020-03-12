/*******************************************************************************
* File name: 
*	dmu680_log.c
*
* Description:
*	 This file is for outputing log information.
*
* $Revision: 15239 $
*
* $Date: 2010-12-17 14:18:56 +0800 (Fri, 17 Dec 2010) $
*
* $Author: jsun $
*******************************************************************************/

#include "dmu680_log.h"

#define STRINGS_LENGTH  150
#define USART_BAUDRATE  115200

/*********************************************************************************
* Function name: 
*	printf_log( const char * format, ... )
*
* Description: 
*	For outputing log information.
*
* Trace: 
*
* Input parameters:	
*	printf strings
*
* Output parameters: 
*	NONE
*	
* Return value: 
*	NONE
************************************************************************************/
int printf_log( const char * format, ... )
{
        int i,length;
        char strings[STRINGS_LENGTH];

        va_list list;
        va_start(list,format);
        length = vsprintf(strings,format,list);

        if(length >= STRINGS_LENGTH)
        {
        	return -1;
        }
        for(i=0;i<length;i++)
        {
          USART_SendData(USARTx,strings[i]);
          while(USART_GetFlagStatus(USARTx, USART_FLAG_TXE) == RESET)
          {
          }
        }
        va_end(list);
		return 0;
}

/*********************************************************************************
* Function name: 
*	UART_log_init()
*
* Description: 
*	Init USART1
*
* Trace: 
*
* Input parameters:	
*	NONE
*
* Output parameters: 
*	NONE
*	
* Return value: 
*	NONE
************************************************************************************/
void UART_log_init()
{
	USART_InitTypeDef USART_init_structure;

	/* Enable USARTx clocks */
	#ifdef USE_USART1
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
	#else
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USARTx, ENABLE);
	#endif

	USART_init_structure.USART_BaudRate = USART_BAUDRATE;
	USART_init_structure.USART_WordLength = USART_WordLength_8b;
	USART_init_structure.USART_StopBits = USART_StopBits_1;
	USART_init_structure.USART_Parity = USART_Parity_No ;
	USART_init_structure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_init_structure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;

	/* Configure the USARTx */
	USART_Init(USARTx, &USART_init_structure);

	/* Enable the USARTx */
	USART_Cmd(USARTx, ENABLE);

}

/*********************************************************************************
* Function name: 
*	GPIO_log_init()
*
* Description: 
*	Init GPIO for USART1
*
* Trace: 
*
* Input parameters:	
*	NONE
*
* Output parameters: 
*	NONE
*	
* Return value: 
*	NONE
************************************************************************************/
void GPIO_log_init()
{
	GPIO_InitTypeDef GPIO_init_structure;

	/* Enable GPIOx clock */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOx, ENABLE);
	/* Configure USARTx_Tx as alternate function push-pull */
	GPIO_init_structure.GPIO_Pin = GPIO_TxPin;
	GPIO_init_structure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_init_structure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(GPIOx, &GPIO_init_structure);

	/* Configure USARTx_Rx as input floating */
	GPIO_init_structure.GPIO_Pin = GPIO_RxPin;
	GPIO_init_structure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOx, &GPIO_init_structure);
}





