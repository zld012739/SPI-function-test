/************************************************************ 
Copyright (C) 1995-2006 Pumpkin, Inc. and its
Licensor(s). Freely distributable.

$Source: C:\\RCS\\D\\Pumpkin\\Salvo\\Src\\ARMRV\\salvohook_interrupt_AT91SAM7S_PITC.c,v $
$Author: aek $
$Revision: 3.1 $
$Date: 2006-09-29 19:29:44-07 $

File illustrates user interrupt control functions 
for use with Salvo applications built with the
ARM RealView C compiler for a target in the Atmel AT91SAM7S 
family.

In this implementation, the interrupt enable bit for
Periodic Interval Timer is controlled to ensure that 
interrupts are disabled during critical sections. When
controlling interrupts in this manner, the only ISR
that calls any of Salvo's services should be the PITC
ISR. Normally, it will call OSTimer() at a periodic
rate, e.g. 10ms.
 
Without disabling the PIT interrupts during Salvo's 
critical sections, corruption will occur when OSTimer() 
is called from within the PIT ISR.

Dummy versions of OSDisable|Enable|Restore|SaveInts() 
are included in all Salvo libraries. If a Salvo 
application does not use interrupts, then the user
need not create any application-specific versions of
these functions.

************************************************************/
#include <AT91SAM7S64.H>
#include <salvo.h>


/* Disable PIT interrupts by clearing the PITIEN 	*/
/*  bit.											*/
void OSDisableHook(void)
{
	*AT91C_PITC_PIMR &= ~AT91C_PITC_PITIEN;
}


/* Enable PIT interrupts by setting the PITIEN bit. */
void OSEnableHook(void)
{
	*AT91C_PITC_PIMR |=  AT91C_PITC_PITIEN;
}


/* Save user's PITC settings. Not required because 	*/
/*  all we're doing is bit-twiddling the PITIEN.	*/ 
void OSSaveHook(void)
{
	;
}


/* Restore user's PITC settings. Since we're only	*/
/*  bit twiddling, it's sufficient to simply set	*/
/*  the PITIEN bit.									*/ 
void OSRestoreHook(void)
{
	*AT91C_PITC_PIMR |=  AT91C_PITC_PITIEN;
}
