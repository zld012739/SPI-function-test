/*******************************************************************************
* File name:	comm.c
*
* File description:
*		Low level I/O terminal application functions.
*
* $Rev: 15953 $
* $Date: 2011-02-17 16:26:00 -0800 (Thu, 17 Feb 2011) $
* $Author: dwang $
*******************************************************************************/

#include "stm32f10x.h"
#include "comm.h"

#define USARTx USART1

/*******************************************************************************
* Function Name  : comm_test
*
* Description: 
*		Test if the read data register is not empty.
* Input parameter:
*		None
* Output parameter:
*		None
* Return value:
*		int: The state of read data register
*			0:	The register is empty.
*			1:	The register is not empty.
*******************************************************************************/
int comm_test(void)
{
	return (USART_GetFlagStatus(USARTx, USART_FLAG_RXNE) == RESET ) ? 0 : 1;
}

/*******************************************************************************
* Function Name  : comm_get
*
* Description: 
*		Get a character from data register.
* Input parameter:
*		None
* Output parameter:
*		None
* Return value:
*		char: Read data from data register.
*******************************************************************************/
char comm_get(void)
{
	while(USART_GetFlagStatus(USARTx, USART_FLAG_RXNE) == RESET) { ; }
	return (char)USART_ReceiveData(USARTx);
}

/*******************************************************************************
* Function Name  : comm_put
*
* Description: 
*		Put a character to data register.
* Input parameter:
*		char d: The character to be write to register.
* Output parameter:
*		None
* Return value:
*		None
*******************************************************************************/
void comm_put(char d)
{
	while(USART_GetFlagStatus(USARTx, USART_FLAG_TXE) == RESET) { ; }
	USART_SendData(USARTx, (uint16_t)d);
}

/*******************************************************************************
* Function Name  : comm_puts
*
* Description: 
*		Put a string to data register.
* Input parameter:
*		const char* s: The string to be write to register.
* Output parameter:
*		None
* Return value:
*		None
*******************************************************************************/
void comm_puts(const char* s)
{
	char c;
	while ( ( c = *s++) != '\0' ) 
	{
		comm_put(c);
	}
}

/*******************************************************************************
* Function Name  : comm_init
*
* Description: 
*		Initialize the usart.
* Input parameter:
*		None
* Output parameter:
*		None
* Return value:
*		None
*******************************************************************************/
void comm_init (void)
{
	
}

