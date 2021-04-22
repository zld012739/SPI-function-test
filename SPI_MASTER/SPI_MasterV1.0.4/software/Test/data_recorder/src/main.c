/***********************************************************************
* File name:  main.c
*
* File description:  use the file to test application module
*
* $Rev: 15739 $
* $Date: 2011-01-24 10:21:21 -0800 (Mon, 24 Jan 2011) $
* $Author: dwang $
***********************************************************************/

/* Includes */
#include <stddef.h>
#include "stdio.h"
#include "stm32f10x.h"
#include "SDCard_driver.h"
#include "stm32f10x_usart.h"
#include "data_type.h"
#include "dmu680_log.h"

/* Private macro */
#define BLOCK_SIZE	(512)
#define BUFFER_WORDS_SIZE      (BLOCK_SIZE >> 2)

#define NUMBER_OF_BLOCKS       ((uint32_t)10)  /* For Multi Blocks operation (Read/Write) */
#define MULTI_BUFFER_WORDS_SIZE ((BLOCK_SIZE * NUMBER_OF_BLOCKS) >> 2)
/* Private variables */
 USART_InitTypeDef USART_InitStructure;
extern uint16_t RCA;
uint32_t Buffer_Block_Tx[BUFFER_WORDS_SIZE];
uint32_t Buffer_Block_Rx[BUFFER_WORDS_SIZE];
uint32_t Buffer_MultiBlock_Tx[MULTI_BUFFER_WORDS_SIZE];
uint32_t Buffer_MultiBlock_Rx[MULTI_BUFFER_WORDS_SIZE];


/* Private function prototypes */
/* Private functions */

void RCC_Configuration(void);
void NVIC_Configuration(void);
void delay(uint16_t);
void fill_buffer(uint32_t *, uint16_t , uint32_t);

/*******************************************************************************
* Function Name  : main
*
* Description: 
*		Main test function
* Input parameter:
*		None
* Output parameter:
*		None
* Return value:
*		SD Card Error code.
*******************************************************************************/
int main(void)
{
	RCC_Configuration();
	NVIC_Configuration();

	UART_log_init();
	GPIO_log_init();

	sd_state_t status;
	
	/* ---------- Initiate the card ---------- */
	status = SD_Init();
	if (status == SD_OK)
  	{
		printf_log("\nSD card is initiated successfully!\n");
		/* ---------- Select Card ----------*/
		status = SD_SelectDeselect((uint32_t)(RCA << 16));
		SD_Erase(0x00,0x10000);
  	}

	/*--------------Single Block Read/Write ------------*/
  	/* ---------- Fill the buffer to send ------*/
  	fill_buffer(Buffer_Block_Tx, BUFFER_WORDS_SIZE, 0x01);

  	if (status == SD_OK)
  	{	
  		printf_log("\r\nSD_ReadMultiBlocks finished.....\n ");  
		/* Write block of 512 bytes on address 0 */
		status = SD_WriteBlock(0x200, Buffer_Block_Tx, BLOCK_SIZE);
		fill_buffer(Buffer_Block_Tx, BUFFER_WORDS_SIZE, 0x10);
		status = SD_WriteBlock(0x00, Buffer_Block_Tx, BLOCK_SIZE);
  	}
	if(status == SD_OK)
	{
		printf_log("\r\nSD SD_WriteBlock finished.....\n ");
		/* Read block of 512 bytes from address 0 */
		status = SD_ReadBlock(0x000, Buffer_Block_Rx, (BLOCK_SIZE>>1));
		status = SD_ReadBlock(0x100, Buffer_Block_Rx, (BLOCK_SIZE>>1));
		status = SD_ReadBlock(0x200, Buffer_Block_Rx, (BLOCK_SIZE>>1));
		status = SD_ReadBlock(0x300, Buffer_Block_Rx, (BLOCK_SIZE>>1));

		status = SD_ReadBlock(0x00, Buffer_Block_Rx, BLOCK_SIZE);
		status = SD_ReadBlock(0x200, Buffer_Block_Rx, BLOCK_SIZE);

	}


	/*--------------Multiple Block Read/Write ------------*/
  	/* ---------- Fill the buffer to send ------*/
	fill_buffer(Buffer_MultiBlock_Tx, MULTI_BUFFER_WORDS_SIZE, 0x00);
  	if (status == SD_OK)
  	{	
		/* Write block of 512 bytes on address 0 */
		status = SD_WriteMultiBlocks(0x00, Buffer_MultiBlock_Tx, BLOCK_SIZE, NUMBER_OF_BLOCKS);
  	}
	if(status == SD_OK)
	{
		printf_log("\r\nSD SD_WriteBlock finished.....\n ");
		/* Read block of 512 bytes from address 0 */
		status = SD_ReadBlock(0x00, Buffer_Block_Rx, BLOCK_SIZE);
		status = SD_ReadBlock(0x200, Buffer_Block_Rx, BLOCK_SIZE);
	}	

	if (status == SD_OK)
	{
		printf_log("\r\nSD SD_ReadBlock finished.....\n ");
	}
	
	uint16_t count = 0;
  	/* Infinite loop */
	while (1)
  	{
 		 delay(0xffff);
 		 count++;
  	}
}

/*******************************************************************************
* Function Name  : RCC_Configuration
* Description    : Configures the different system clocks.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void RCC_Configuration(void)
{																
	ErrorStatus HSEStartUpStatus;
	/* RCC system reset(for debug purpose) */
	RCC_DeInit();

	/* Enable HSE */
	RCC_HSEConfig(RCC_HSE_ON);
	/* Wait till HSE is ready */
	HSEStartUpStatus = RCC_WaitForHSEStartUp();

	if(HSEStartUpStatus == SUCCESS)
	{
		/* HCLK = SYSCLK */
		RCC_HCLKConfig(RCC_SYSCLK_Div1);

		/* PCLK2 = HCLK */
		RCC_PCLK2Config(RCC_HCLK_Div1);

		/* PCLK1 = HCLK/2 */
		RCC_PCLK1Config(RCC_HCLK_Div2);

		/* Flash 2 wait state */
		FLASH_SetLatency(FLASH_Latency_2);
		/* Enable Prefetch Buffer */
		FLASH_PrefetchBufferCmd(FLASH_PrefetchBuffer_Enable);

		/* PLLCLK = 8MHz * 9 = 72 MHz */
		RCC_PLLConfig(RCC_PLLSource_HSE_Div1, RCC_PLLMul_9);

		/* Enable PLL */
		RCC_PLLCmd(ENABLE);

		/* Wait till PLL is ready */
		while(RCC_GetFlagStatus(RCC_FLAG_PLLRDY) == RESET)
		{
		}

		/* Select PLL as system clock source */
		RCC_SYSCLKConfig(RCC_SYSCLKSource_PLLCLK);

		/* Wait till PLL is used as system clock source */
		while(RCC_GetSYSCLKSource() != 0x08)
		{
		}
	}

	/* Enable GPIOA¡¢GPIOB and USART1 clocks */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB |
                          RCC_APB2Periph_USART1, ENABLE);
}

/*******************************************************************************
* Function Name  : NVIC_Config
* Description    : Configures SDIO IRQ channel.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void NVIC_Configuration(void)
{
	NVIC_InitTypeDef NVIC_InitStructure;

	/* Configure the NVIC Preemption Priority Bits */
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);

	NVIC_InitStructure.NVIC_IRQChannel = SDIO_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
}

void delay(uint16_t cycles)
{
	int i = 0;
	for(; i <= cycles; i++);
}

void fill_buffer(uint32_t *pBuffer, uint16_t BufferLenght, uint32_t Offset)
{
	uint16_t index = 0;

	/* Put in global buffer same values */
	for (index = 0; index < BufferLenght; index++ )
	{
		pBuffer[index] = index + Offset;
	}
}

