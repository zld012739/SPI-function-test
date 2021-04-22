/************************************************************ 
Copyright (C) 1995-2006 Pumpkin, Inc. and its
Licensor(s). Freely distributable.

$Source: C:\\RCS\\D\\Pumpkin\\Salvo\\Src\\IARARM\\salvohook_interrupt_IRQ.c,v $
$Author: aek $
$Revision: 3.5 $
$Date: 2011-07-06 17:31:09-07 $

Most basic example of user-defined interrupt control functions. 
Can be replaced by user functions that control interrupts 
associated with Salvo services.

************************************************************/
#include "salvo.h"
#include "intrinsics.h"


/************************************************************
****                                                     ****
**                                                         **
Hooks for IAR Embedded Workbench for ARM -- intelligent 
control of interrupts. 

In mainline code, interrupts are under user control, and 
these hooks restore it at the end of a Salvo critical section.

This sequence only restores those interrupts that were 
enabled prior to the Salvo critical section.

For EWARM v5 and later. As per example in EWARM Development
Guide.

**                                                         **
****                                                     ****
************************************************************/
__istate_t s; 


void OSDisableHook(void) {
  s = __get_interrupt_state();
  __disable_interrupt();
}


void OSEnableHook(void) {
  __set_interrupt_state(s);
}

