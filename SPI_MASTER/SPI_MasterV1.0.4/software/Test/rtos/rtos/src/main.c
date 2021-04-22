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
#include "os.h"
//#include "dmu680_log.h"
#include "os_mbox.h"

#define START_STK_SIZE			(128)
#define START_TASK_prio			(10)
#define START_TIME_SLICE		(1)

uint32_t task1_start_stack[START_STK_SIZE];
uint32_t task2_start_stack[START_STK_SIZE];
void task_1(void);
void task_2(void);
/* Abstract: main program  */
int main(void)
{
	/* board support package initialize */
	NVIC_config();
	//UART_log_init();
	//GPIO_log_init();
#if 1/* Test task context*/
	/* initialize The Real-Time Kernel */
	OS_initialize();
	to_user_mode();
	/* create the  task 1 to turn led on */
	OS_task_create(&task1_start_stack[START_STK_SIZE-1], START_STK_SIZE, task_1, 1, READY, START_TIME_SLICE);
	OS_task_create(&task2_start_stack[START_STK_SIZE-1], START_STK_SIZE, task_2, 2, READY, START_TIME_SLICE);
	/* start  multitasking */
	OS_start();

#else
	task_1();/* Test task */
	//task_2();
#endif
	return 1;
}

void task_1(void)
{
	/* board support package initialize */
	BSP_init();
	while(1)
	{
		/* Turn on LEDs */
		led_on(LED1);
		delay(1000);
		led_off(LED1);
		delay(1000);
	}
}

void task_2()
{
	BSP_init();
	while(1)
	{
		led_on(LED2);
		delay(1000);
		led_off(LED2);
		delay(1000);
	}
}
