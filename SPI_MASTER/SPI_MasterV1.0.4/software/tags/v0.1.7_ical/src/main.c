/******************************************************************************
* File name:  main.c
*
* File description:  main is the center of the system universe,
* it all starts here. And never ends.
*
******************************************************************************/

/* Includes */
#include <stddef.h>
#include "stm32f2xx.h"
#include "bsp.h"

#include "debug_usart.h"
#include "debug.h"
#include "salvodefs.h"

#include "commandLine.h"

#include "taskDataAcquisition.h"
#include "taskUserCommunication.h"


void TaskHeartbeat(void);
void TaskCommandLine(void);

int main(void)
{
    SystemInit();

    /* board support package initialize */
	BSP_init();

    InitDebugSerialCommunication();
    DebugPrintString("\r\nDMU380 system\r\n");
    // FIXME: put boot cause message here

    OSInit();

    OSCreateTask(TaskDataAcquisition, DATA_ACQ_TASK, DATA_ACQ_PRIORITY);
    OSCreateTask(TaskCommandLine, COMMAND_LINE_TASK, COMMAND_LINE_PRIORITY);
    OSCreateTask(TaskUserCommunication, USER_COMMUNICATION_TASK, USER_COMMUNICATION_PRIORITY);
    OSCreateTask(TaskHeartbeat, HEARTBEAT_TASK, HEARTBEAT_PRIORITY);

    OSCreateBinSem(BINSEM_SERIAL_RX, 0);
    OSCreateBinSem(BINSEM_DATA_ACQ_TIMER, 0);
    // may want to disable interrupts until here... OSTimer may be called before OS running?
    OSCreateEFlag(EFLAGS_DATA_READY, EFLAGS_DATA_READY_CB_P, 0x00);
    OSCreateEFlag(EFLAGS_DATA_DONE, EFLAGS_DATA_DONE_CB_P, 0x00);
    OSCreateEFlag(EFLAGS_USER, EFLAGS_USER_CB_P, 0x00);


    while (1) {
        OSSched();
        // FIXME: put watchdog here
    }
}


void TaskHeartbeat(void)
{
    static int heartbeat = 0;
	/* board support package initialize */
	while(1)
	{
        heartbeat++;
		/* Turn on LEDs */
		led_on(HEARTBEAT_LED);
        OS_Delay(OS_TICKS_PER_SECOND/4);
		led_off(HEARTBEAT_LED);
        OS_Delay(OS_TICKS_PER_SECOND/4);
	}
}

void TaskCommandLine(void)
{
    InitCommandLine();

    while (1) {
        OS_WaitBinSem(BINSEM_SERIAL_RX, OSNO_TIMEOUT);
		led_toggle(LED2);
        CmdLineLookup(gCommands);
    }
}