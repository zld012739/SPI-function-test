/*******************************************************************************
* File : main.c
*
* Description :
* 	main function for test timer.
*
* $Revision: 15510 $
*
* $Date: 2011-01-04 02:31:01 -0800 (Tue, 04 Jan 2011) $
*
* $Author: jsun $
******************************************************************************/

/* Includes */
#include <stddef.h>
#include "stdio.h"
#include "stm32f10x.h"
#include "timer.h"
#include "stdarg.h"
#include "dmu680_log.h"

#define TIM3_PERIOD (uint16_t)(35999)	/* timer3 period */
#define TIM3_PRESCALER (uint16_t)(999)  /* timer3 prescaler */
#define LED	(uint32_t)(0x00000010)
/**
**===========================================================================
**
**  Abstract: main program
**
**===========================================================================
*/
uint8_t temp = 0;

void timer_test()
{
	 TIM_TimeBaseInitTypeDef  TIM_init_struct = {0,0,0,0,0};
	 NVIC_InitTypeDef  NVIC_init_structure;

	 /* TIM3 clock enable */
	 RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);

	 /*TIME3 INTERRUPT CONFIGURE*/
	 NVIC_init_structure.NVIC_IRQChannel = TIM3_IRQn;
	 NVIC_init_structure.NVIC_IRQChannelPreemptionPriority = 0;
	 NVIC_init_structure.NVIC_IRQChannelSubPriority = 4;
	 NVIC_init_structure.NVIC_IRQChannelCmd = ENABLE;
	 NVIC_Init(&NVIC_init_structure);

	 /* Time3 Base configuration 1 second interrupt*/
	 TIM_init_struct.TIM_Prescaler = TIM3_PRESCALER;    /*999*/
	 TIM_init_struct.TIM_CounterMode = TIM_CounterMode_Down;
	 TIM_init_struct.TIM_Period = TIM3_PERIOD;          /*35999*/
	 TIM_init_struct.TIM_ClockDivision = 0;
	 TIM_init_struct.TIM_RepetitionCounter = 0x0;
	 /*time3 init*/
	 TIM_TimeBaseInit(TIM3,&TIM_init_struct);

	 TIM_ClearFlag(TIM3,TIM_FLAG_Update);
	 TIM_ITConfig(TIM3,TIM_IT_Update,ENABLE);
	 TIM_Cmd(TIM3,ENABLE);
	 GPIO_Configuration();
}

void GPIO_Configuration(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	/* GPIOA clock enable */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

	/* GPIOA Configuration:for TIM3 outputing 1pps signal */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1 | GPIO_Pin_4;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_0;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
}

void TIM3_IRQ()
{
	if(temp == 0)
	{
		GPIOA->ODR &= (~LED);
		temp = 1;
	}
	else
	{
		GPIOA->ODR |= LED;
		temp = 0;
	}
	TIM_ClearITPendingBit(TIM3, TIM_FLAG_Update );
}

int main(void)
{
	init_sys_timer();
	timer_test();
	GPIO_log_init();
	UART_log_init();
	while(1)
	{
	}
}




