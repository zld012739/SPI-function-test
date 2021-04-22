/************************************************************ 
The contents of this file are subject to the Pumpkin Salvo
License (the "License").  You may not use this file except
in compliance with the License. You may obtain a copy of
the License at http://www.pumpkininc.com, or from
warranty@pumpkininc.com.

Software distributed under the License is distributed on an
"AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either express
or implied. See the License for specific language governing
the warranty and the rights and limitations under the
License.

The Original Code is Salvo - The RTOS that runs in tiny
places(TM). Copyright (C) 1995-2006 Pumpkin, Inc. and its
Licensor(s). All Rights Reserved.

$Source: C:\\RCS\\D\\Pumpkin\\Salvo\\Src\\IARARM\\salvohook_idle.c,v $
$Author: aek $
$Revision: 3.2 $
$Date: 2011-07-06 17:27:28-07 $

For IAR Embedded Workbench for ARM and ARM targets.

Default idling function for libraries. Executed whenever 
the scheduler is idling, i.e. there are no eligible tasks to run.

************************************************************/
#include "salvo.h"

/************************************************************ 
****                                                     ****
**                                                         **
OSIdlingHook()

User service called whenever system is idling (i.e. no tasks
are eligible to run).

**                                                         **
****                                                     ****
************************************************************/
void OSIdlingHook(void) {
  ;
}
