/************************************************************ 
Copyright (C) 1995-2006 Pumpkin, Inc. and its
Licensor(s). Freely distributable.

$Source: C:\\RCS\\D\\Pumpkin\\Salvo\\Src\\ARMRV\\salvohook_interrupt_AT91SAM7S_IRQ.c,v $
$Author: aek $
$Revision: 3.1 $
$Date: 2006-09-29 19:29:44-07 $

File illustrates user interrupt control functions 
for use with Salvo applications built with the
ARM RealView C compiler for a target in the Atmel AT91SAM7S 
family.

In this implementation, IRQ interrupts are controlled globally
and blindly, i.e. without regard to the interrupt control
bit's prior setting.
 
Without disabling the global interrupts during Salvo's 
critical sections, corruption will occur when OSTimer() 
is called from within an IRQ.

Dummy versions of OSDisable|Enable|Restore|SaveInts() 
are included in all Salvo libraries. If a Salvo 
application does not use interrupts, then the user
need not create any application-specific versions of
these functions.

Similarly, if a Salvo application uses only a particular
interrupt (e.g. the PITC to call OSTimer()), then interrupt
control with finer granularity can be used to enhance overall
system performance and eliminate interrupt latency in all
interrupt sources that do not call Salvo services. E.g. see
salvohook_interrupt_AT91SAM7S_PITC.c

************************************************************/

#include <salvo.h>

__inline void enable_IRQ(void)
{
	
		int tmp;
		__asm
		{
			MRS	tmp, CPSR
			BIC	tmp, tmp, #0x80
			MSR CPSR_c, tmp
			
		}
}


__inline void disable_IRQ(void)
{
	
		int tmp;
		__asm
		{
			MRS	tmp, CPSR
			ORR	tmp, tmp, #0x80
			MSR CPSR_c, tmp
			
		}
}


/* Simple global interrupt enable. */
void OSEnableHook(void)
{
	enable_IRQ();
}

/* Since we don't care about the previous state of global */
/*  interrupt enables, there's nothing to save ...        */
void OSSaveHook(void)
{
	;
}


/* Simple global interrupt disable. */
void OSDisableHook(void)
{
	disable_IRQ(); 
}


/* ... and nothing to restore. */
void OSRestoreHook(void)
{
	enable_IRQ();
}
