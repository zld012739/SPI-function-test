/*******************************************************************************
* File name: 
*	dmu680_log.c
*
* Description:
*	 for the log driver
*
* $Revision: 15239 $
*
* $Date: 2010-12-17 14:18:56 +0800 (Fri, 17 Dec 2010) $
*
* $Author: jsun $
*******************************************************************************/

#ifndef __DMU680LOG_H
#define __DMU680LOG_H

#include "stdint.h"
#include "stm32f10x.h"
#include "stdarg.h"

#define USE_USART1
//#define USE_USART2
//#define USE_USART3
//#define USE_UART4
//#define USE_UART5

#ifdef USE_USART1
  #define  USARTx                     USART1
  #define  GPIOx                      GPIOA
  #define  RCC_APB2Periph_GPIOx       RCC_APB2Periph_GPIOA
  #define  GPIO_RxPin                 GPIO_Pin_10
  #define  GPIO_TxPin                 GPIO_Pin_9
#elif defined USE_USART2 && defined USE_STM3210B_EVAL
  #define  USARTx                     USART2
  #define  RCC_APB1Periph_USARTx      RCC_APB1Periph_USART2
  #define  GPIOx                      GPIOD
  #define  RCC_APB2Periph_GPIOx       RCC_APB2Periph_GPIOD
  #define  GPIO_TxPin                 GPIO_Pin_5
  #define  GPIO_RxPin                 GPIO_Pin_6
#elif defined USE_USART2 && defined   USE_STM3210E_EVAL  
  #define  USARTx                     USART2
  #define  RCC_APB1Periph_USARTx      RCC_APB1Periph_USART2
  #define  GPIOx                      GPIOA
  #define  RCC_APB2Periph_GPIOx       RCC_APB2Periph_GPIOA
  #define  GPIO_TxPin                 GPIO_Pin_2
  #define  GPIO_RxPin                 GPIO_Pin_3
#elif defined USE_USART3
  #define  USARTx                     USART3
  #define  GPIOx                      GPIOB
  #define  RCC_APB2Periph_GPIOx       RCC_APB2Periph_GPIOB
  #define  RCC_APB1Periph_USARTx      RCC_APB1Periph_USART3
  #define  GPIO_RxPin                 GPIO_Pin_11
  #define  GPIO_TxPin                 GPIO_Pin_10
#elif defined USE_UART4
  #define  USARTx                     UART4
  #define  GPIOx                      GPIOC
  #define  RCC_APB2Periph_GPIOx       RCC_APB2Periph_GPIOC
  #define  RCC_APB1Periph_USARTx      RCC_APB1Periph_UART4
  #define  GPIO_RxPin                 GPIO_Pin_11
  #define  GPIO_TxPin                 GPIO_Pin_10
#elif defined USE_UART5
  #define  USARTx                     UART5
  #define  GPIOx                      GPIOC
  #define  RCC_APB2Periph_GPIOx       RCC_APB2Periph_GPIOC
  #define  RCC_APB1Periph_USARTx      RCC_APB1Periph_UART5
  #define  GPIO_RxPin                 GPIO_Pin_2
  #define  GPIO_TxPin                 GPIO_Pin_12
#endif

int printf_log( const char * format, ... );
void UART_log_init();
void GPIO_log_init();

#endif
