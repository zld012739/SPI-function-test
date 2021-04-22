/******************************************************************************
* File name:  main.c
*
* File description:  use the file to test application module
*
* $Rev: 16166 $
* $Date: 2011-03-09 11:53:45 -0800 (Wed, 09 Mar 2011) $ 
* $Author: whpeng $
******************************************************************************/

/* Includes */
#include <stddef.h>
#include "stm32f2xx.h"
#include "bsp.h"

#include "debug_usart.h"
#include "debug.h"
#include <salvo.h>
#include "salvodefs.h"

void task_1(void);
void task_2(void);

int main(void)
{
    SystemInit();
    InitDebugSerialCommunication();
    DebugPrintString("\r\nDMU380 rtos tester\r\n");

    	/* board support package initialize */
	BSP_init();

    OSInit();
    
    OSCreateTask(task_1, OSTCBP(1), 10);
    OSCreateTask(task_2, OSTCBP(2), 10);
    while (1) {
        OSSched();
    }
}


void task_1(void)
{
	/* board support package initialize */
//	BSP_init();
	while(1)
	{
		/* Turn on LEDs */
		led_on(LED1);
        OS_Delay(OS_TICKS_PER_SECOND);
		led_off(LED1);
        OS_Delay(OS_TICKS_PER_SECOND);
	}
}

void task_2()
{
	//BSP_init();
	while(1)
	{
		led_on(LED2);
        OS_Delay(OS_TICKS_PER_SECOND);
		led_off(LED2);
        OS_Delay(OS_TICKS_PER_SECOND);
	}
}
