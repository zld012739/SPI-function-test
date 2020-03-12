/**
  ******************************************************************************
  * @file USART/DMA_Interrupt/main.c
  * @author  MCD Application Team
  * @version  V3.0.0
  * @date  04/06/2009
  * @brief  Main program body
  ******************************************************************************
  * @copy
  *
  * THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
  * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE
  * TIME. AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY
  * DIRECT, INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING
  * FROM THE CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE
  * CODING INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
  *
  * <h2><center>&copy; COPYRIGHT 2009 STMicroelectronics</center></h2>
  */

/* Includes ------------------------------------------------------------------*/
#include "stm32f10x.h"
#include "platform_config.h"
#include "port_def.h"
#include "uart.h"
#include "stm32f10x_dma.h"

/* Private define ------------------------------------------------------------*/
#define USART1_DR_Base  0x40013804
#define USART2_DR_Base  0x40004404
#define TxBufferSize1   (countof(TxBuffer1) - 1)
#define TxBufferSize2   (countof(TxBuffer2) - 1)

/* Private macro -------------------------------------------------------------*/
#define countof(a)   (sizeof(a) / sizeof(*(a)))

/* Private variables ---------------------------------------------------------*/
#if 1
uint8_t TxBuffer1[] =
    "\r01,02,03,09\n"
    "\rsunjian\n"
    "\rjiayou\n"
    "\rnihao\r\n";
#endif
uint8_t TxBuffer2[] = "USART DMA Interrupt: USART2 -> USART1 using DMA Tx and Rx Interrupt";
uint8_t buffer_tx[512];
uint8_t buffer_rx[512];

/* Private function prototypes -----------------------------------------------*/
void RCC_Configuration(void);
void GPIO_Configuration(void);

int main(void)
{
        USART_InitTypeDef USART_InitStructure;

	circ_buf_init(&port1,buffer_tx,buffer_rx,512,512);
	circ_buf_in(&(port1.xmit_buf),TxBuffer1, TxBufferSize1);

	/* port1 configuration */
	port1.hw.DMA_add = USART1_DR_Base;
	port1.hw.DMA_channel = DMA1_Channel4;
	port1.hw.DMA_memory_add =(uint32_t)(port1.xmit_buf.buf_add + port1.xmit_buf.buf_outptr);
	port1.hw.DMA_interrupt = DMA1_Channel4_IRQn;
	port1.hw.DMA_preemption_priority = 1;
	port1.hw.DMA_sub_priority = 0;
	port1.hw.port_tx_flag = UNUSE;
	port1.hw.DMA_tx = 0;
	port1.hw.DMA_transfer_dir = DMA_DIR_PeripheralDST;
	port1.hw.DMA_tc_clear = DMA1_FLAG_GL4;      /* DMA tc clear */
	port1.hw.uart_add = USART1;
	port1.hw.baudrate = 115200;
	port1.hw.word_length= USART_WordLength_8b;
	port1.hw.stop_bits = USART_StopBits_1;
	port1.hw.parity = USART_Parity_No;
	port1.hw.hardware_flow_control = USART_HardwareFlowControl_None;
	port1.hw.usart_mode = USART_Mode_Rx | USART_Mode_Tx;
	port1.hw.usart_interrupt = USART1_IRQn;  /* interrupt */
	port1.hw.usart_preemption_priority = 1;
	port1.hw.usart_sub_priority = 1;

	/* System Clocks Configuration */
	RCC_Configuration();

	/* Configure the GPIO ports */
	GPIO_Configuration();

	uart_init(&(port1));
	write_UART(&(port1));
	while(1)
	{
	}
}

void RCC_Configuration(void)
{
        /* Setup the microcontroller system. Initialize the Embedded Flash Interface,
	initialize the PLL and update the SystemFrequency variable. */
	/*  SystemInit(); */

	/* DMA clock enable */
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
	/* Enable USART1, GPIOA, GPIOx and AFIO clocks */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1 | RCC_APB2Periph_GPIOA |
						   RCC_APB2Periph_GPIOx | RCC_APB2Periph_AFIO, ENABLE);
	/* Enable USART2 clock */
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);
}

/**
  * @brief  Configures the different GPIO ports.
  * @param  None
  * @retval : None
  */
void GPIO_Configuration(void)
{
        GPIO_InitTypeDef GPIO_InitStructure;

	/* Configure USART1 Rx (PA.10) as input floating */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	/* Configure USART2 Rx as input floating */
	GPIO_InitStructure.GPIO_Pin = GPIO_RxPin;
	GPIO_Init(GPIOx, &GPIO_InitStructure);

	/* Configure USART1 Tx (PA.09) as alternate function push-pull */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	/* Configure USART2 Tx as alternate function push-pull */
	GPIO_InitStructure.GPIO_Pin = GPIO_TxPin;
	GPIO_Init(GPIOx, &GPIO_InitStructure);
}


void assert_failed(uint8_t* file, uint32_t line)
{
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

  /* Infinite loop */
  while (1)
  {
  }
}





