/*******************************************************************************
* File name: 
*	timer.c
*
* Description:
*	The time base used for the RTOS system Tick and the elapse time counter are 
*	driven by the Cortex system timer PCLK 2 which is derived from the 20 MHz 
*	external oscillator. The millisecond timer ¡°SYS_TICK¡± will generate an interrupt 
*	every 1 millisecond and drive the seconds timer which generates an interrupt 
*	every second which will increment the elapsed time count in seconds.  
*	In systems that have a GPS attached the 1 pulse per second input (1PPS) will be 
*	used to synchronize the millisecond clock and generate an offset to the seconds 
*	counter. This allows the system events to be synchronized to the GPS time and to 
*	correct for system clock error.
*
* $Revision: 16165 $
*
* $Date: 2011-03-09 11:52:30 -0800 (Wed, 09 Mar 2011) $
*
* $Author: whpeng $
*******************************************************************************/
#include "stdint.h"
#include "timer.h"
#include "stm32f2xx.h"
#include "debug.h"
#include <salvo.h>
#include "salvodefs.h"

/**
** @brief  SysTick_Handler handles SysTick Handler interrupt, incrementing 
**          gSysTick global variable to show passage of time.
** @param  None
** @retval None
*/
volatile static uint32_t gSysTicks = 0;
volatile static uint32_t gOsSysTicks = SALVO_TIMER_PRESCALE; // OS runs at 1/10 system runs
void SysTick_Handler(void)
{
    gSysTicks++;
    gOsSysTicks--;
    if (gOsSysTicks == 0) {
        OSTimer();
        gOsSysTicks = SALVO_TIMER_PRESCALE;
    }
}
/**
** @brief  DelayMs busy-waits for the amount of time given. Note that this
**          does not free the processor for other tasks so it should only be 
**          used in init and extreme cases.
** @param  Number of milliseconds to delay.
** @retval None
*/
void DelayMs(tTime delay)
{
   uint32_t start = gSysTicks;
   while ((gSysTicks - start) < delay) { ; }
}
/**
** @brief  TimeNow and TimePassed should be used together to determine the
**         amount of time passed (useful for timing how long actions take)
** @param  TimePassed takes the time returned from TimeNow
** @retval TimePassed returns the number of milliseconds that have passed since 
**          TimeNow was called.
*/
tTime TimeNow() { return gSysTicks; }
tTime TimePassed(tTime since)
{
    tTime now = gSysTicks;
    if (now >= since) {return (now - since);}
    // rollover has occurred
    return (now + (1 + TIME_MAX - since));
}

/***********************************************************************************
* Function name: 
*	init_sys_timer()
*
* Description: 
*    1ms tick, used as the OS tick
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
 void InitSystemTimer(void)
 {	      
     // need 1s interupt as well from GPS, hooked in to the system to force
     //   sampling on that boundary.
     SysTick_Config(SystemCoreClock / 1000);
 }

