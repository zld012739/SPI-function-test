; /**********************************************************
; The contents of this file are subject to the Pumpkin Salvo
; License (the "License").  You may not use this file except
; in compliance with the License. You may obtain a copy of
; the License at http://www.pumpkininc.com, or from
; warranty@pumpkininc.com.
; 
; Software distributed under the License is distributed on an
; "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either express
; or implied. See the License for specific language governing
; the warranty and the rights and limitations under the
; License.
; 
; The Original Code is Salvo - The RTOS that runs in tiny
; places(TM). Copyright (C) 1995-2005 Pumpkin, Inc. and its
; Licensor(s). All Rights Reserved.
; 
; $Source: C:\\RCS\\D\\Pumpkin\\Salvo\\Src\\ARMRV\\salvoportarmrv4.s,v $
; $Author: aek $
; $Revision: 3.0 $
; $Date: 2006-09-29 19:44:37-07 $
; 
; Context switcher for ARM/Keil RV-MDK toolset for ARM7 (and maybe others).
; 
; ARM AAPCS Calling Convention:
;   R0-R3		scratch					
;   R4-R11		preserved by called function
;	R12			scratch
;   R13	(SP)	stack pointer 	
;   R14	(LR)	link register
;   R15 (PC)	program counter
;
; Register Usage:
;   R0-R3		all used					
;   R4-R11		explicitly preserved on stack
;	R12			not used
;   R13	(SP)	stack pointer 	
;   R14	(LR)	pushed onto and popped of stack
;   R15 (PC)	not used
;
; We use R0 as pointing to the start of the current 
;  task's tcb, as the ARM instruction modes allow
;  us to access the elements of the tcb structure rather
;  easily via LDM/STM's immediate offset.
; 
; **********************************************************/

                REQUIRE8
                PRESERVE8

                AREA ||.text||, CODE, READONLY, align=2
                IMPORT           OScTcbP
                IMPORT           OSframeP

; /**********************************************************
; ****                                                   ****
; **                                                       **
; void OSDispatch (void)
;
; Dispatch Salvo task after first restoring R4-R11 that were
;  saved in the task's tcb prior to its most recent return 
;  to the scheduler.
; 
; **                                                       **
; ****                                                   ****
; **********************************************************/

                ALIGN
OSDispatch		PROC
				EXPORT		OSDispatch
				
				;*********************************************
				;
				; ARM mode version.
				;
				IF {CONFIG} = 32					; ARM mode	
				CODE32								; ARM entry
				
				;
				; Save R4-R11, and return-to address (LR).
				;
				
				STMFD       SP!,{R4-R11,LR}			; save working registers and
													;  LR (= return address in OSSched())
				
				;
				; OSframeP = SP;
				; OSframeP now holds SP just past saved R4-R7 & LR.
				;
				MOV			R3,SP					; save stack pointer for use 
				LDR         R1,=OSframeP 			;  as frame pointer in OSCtxSw()
				STR         R3,[R1,#0x0]			;  ""

				;
				; SP = SP - OScTcbP->status.bits.frameSize;
				;
				
													; R3: SP
				LDR         R0,=OScTcbP				; R0 -> beginning of current 
				LDR         R0,[R0,#0x0]			;  tasks's tcb
				LDR         R1,[R0,#0x0]			; R1[31:16] = current tasks's
				     								;  status.bits.frameSize
				MOV         R1,R1,LSR #0x10			;  ""
				SUB			R3,R3,R1			    ; R3 = SP - frameSize
				MOV			SP,R3					; redefine SP (atomic)		

				;
                ; Restore R4-R11 from tcb's savedRegs.
                ;
                MOV         R1,R0					; savedRegs always start at
                ADD         R1,#8     				;  this offset in the tcb
                LDMIA       R1,{R4-R11}				; update all 8 regs at once

				;
				; OScTcbP->tFP();
				; This is effectively a jump-to-subroutine, as we won't be coming
				;  back to this function ...
				;
													; R0: -> beginning of current 
													;  tasks's tcb
				LDR         R1,[R0,#0x4]			; R1 = current tasks's tFP
				BX          R1						; branch to task's start/resume
				NOP									;  address
				
				
				;*********************************************
				;
				; Thumb mode version.
				;
				ELIF {CONFIG} = 16    				; Thumb mode
				CODE16								; Thumb entry
				
				;
				; Save R4-R11, and return-to address (LR).
				;
                PUSH        {R4-R7,LR}				; save working registers and
                MOV			R4,R8					;  LR (= return address in OSSched())
                MOV			R5,R9					;  ""
                MOV			R6,R10					;  ""
                MOV			R7,R11					;  ""
                PUSH		{R4-R7}					;  ""
				
				;
				; OSframeP = SP;
				; OSframeP now holds SP just past saved R4-R7 & LR.
				;
				MOV			R3,SP					; save stack pointer for use 
				LDR         R1,=OSframeP 			;  as frame pointer in OSCtxSw()
				STR         R3,[R1,#0x0]			;  ""

				;
				; SP = SP - OScTcbP->status.bits.frameSize;
				;
				
													; R3: SP
				LDR         R0,=OScTcbP				; R0 -> beginning of current 
				LDR         R0,[R0,#0x0]			;  tasks's tcb
				LDR         R1,[R0,#0x0]			; R1[31:16] = current tasks's
													;  status.bits.frameSize
				LSR         R1,R1,#0x10				;  ""
				SUB			R3,R3,R1				; R3 = SP - frameSize
				MOV			SP,R3					; redefine SP (atomic)		

				;
                ; Restore R4-R11 from tcb's savedRegs.
                ;
                MOV			R1,R0					; a wee bit more complicated ...	
                ADD			R1,#24					;  copy saved R8-R11 first down 
                LDMIA		R1!,{R4-R7}				;  to R4-R7, move them up to 
                MOV			R8,R4					;  R8-R11, and then copy saved
                MOV			R9,R5					;  R4-R7 to R4-R7
                MOV			R10,R6					;  ""
                MOV			R11,R7					;  ""
                MOV			R1,R0					;  ""
                ADD			R1,#8					;  ""
                LDMIA		R1!,{R4-R7}				;  ""

				;
				; OScTcbP->tFP();
				; This is effectively a jump-to-subroutine, as we won't be coming
				;  back to this function ...
				;
													; R0: -> beginning of current 
													;  tasks's tcb
				LDR         R1,[R0,#0x4]			; R1 = current tasks's tFP
				BX          R1						; branch to task's start/resume
				NOP
				
				ENDIF								; ARM or Thumb mode
                
                ;
                ; End of OSDispatch().
                ;
				ENDP


; /**********************************************************
; ****                                                   ****
; **                                                       **
; void OSCtxSw ()
; 
; Return from Salvo task to scheduler  Salvo after first 
;  saving task's R4-R11 to the tcb. 
;
; **                                                       **
; ****                                                   ****
; **********************************************************/

				ALIGN
OSCtxSw			PROC
				EXPORT		OSCtxSw

				;*********************************************
				;
				; ARM mode version.
				;
				IF {CONFIG} = 32					; ARM mode	
				CODE32								; ARM entry

				;
				; OScTcbP->tFP = return address on stack;
				; Return address is in LR because OSCtxSw()
				;  has no arguments and no return value.
				;
				LDR         R0,=OScTcbP 			; R0 -> beginning of current  
				LDR         R0,[R0,#0x0] 			;  tasks's tcb
				MOV			R1,LR					; write return-to address (in
				STR         R1,[R0,#0x4]			;  LR) to task's tcb's tFP field

				;
                ; Save R4-R11 to tcb's savedRegs.
                ;
                MOV         R1,R0					; savedRegs always start at
                ADD         R1,#8     				;  this offset in the tcb
                STMIA       R1,{R4-R11}				; update all 8 regs at once

                ;
                ; OScTcbP->status.bits.frameSize = OSframeP - SP;
                ;
                LDR         R1,=OSframeP			; R1 = R2 = OSframeP
                LDR         R1,[R1,#0x0]			;  (save in R2 for later
                MOV			R2,R1					;  use)
                MOV			R3,SP					; R1 = OSframeP - SP
                SUB         R1,R1,R3				;  ""
                									; R0: -> beginning of current
                									;  tasks's tcb
                 									; R1: frameSize 
                									; R2: OSframeP
                MOV			R1,R1,LSL #0x10			; R1[31:16] = frameSize,
                LDRH        R3,[R0,#0x0]			;  write new frameSize to tcb
                ORR         R3,R3,R1				;  without affecting [15:0]
                STR         R3,[R0,#0x0]			;  "" 
                
                ;
                ; SP = OSframeP;
                ;
                									; R2: OSframeP
				MOV			SP,R2					; redefine SP (atomic)
				
                ;
                ; Restore R4-R11 & LR, return to OSSched();
                ;
                LDMFD       SP!,{R4-R11}			; restore R4-R11 & LR and return to           			 						
            	POP			{R0}					;  scheduler
            	BX			R0						;  ""
            	NOP

				
				;*********************************************
				;
				; Thumb mode version.
				;
				ELIF {CONFIG} = 16    				; Thumb mode
				CODE16								; Thumb entry
                
				;
				; OScTcbP->tFP = return address on stack;
				; Return address is in LR because OSCtxSw()
				;  has no arguments and no return value.
				;
				LDR         R0,=OScTcbP 			; R0 -> beginning of current  
				LDR         R0,[R0,#0x0] 			;  tasks's tcb
				MOV			R1,LR					; write return-to address (in
				STR         R1,[R0,#0x4]			;  LR) to task's tcb's tFP field

				;
                ; Save R4-R11 to tcb's savedRegs.
                ;
                MOV         R1,R0					; a wee bit more complicated ...
                ADD         R1,#8     				;  copy R4-R7 first to saved
                STMIA		R1!,{R4-R7}				;  R4-R7, then copy R8-R11
                MOV			R4,R8					;  to R4-R7, and then copy
                MOV			R5,R9					;  R4-R7 to saved R4-R7
                MOV			R6,R10					;  ""
                MOV			R7,R11					;  ""
                MOV			R1,R0					;  ""
                ADD			R1,#24					;  ""
                STMIA		R1!,{R4-R7}				;  ""

                ;
                ; OScTcbP->status.bits.frameSize = OSframeP - SP;
                ;
                LDR         R1,=OSframeP			; R1 = R2 = OSframeP
                LDR         R1,[R1,#0x0]			;  (save in R2 for later
                MOV			R2,R1					;  use)
                MOV			R3,SP					; R1 = OSframeP - SP
                SUB         R1,R1,R3				;  ""
                									; R0: -> beginning of current
                									;  tasks's tcb
                 									; R1: frameSize 
                									; R2: OSframeP
                LSL			R1,R1,#0x10				; R1[31:16] = frameSize,
                LDRH        R3,[R0,#0x0]			;  write new frameSize to tcb
                ORR         R3,R1					;  without affecting [15:0]
                STR         R3,[R0,#0x0]			;  "" 
                
                ;
                ; SP = OSframeP;
                ;
                									; R2: OSframeP
				MOV			SP,R2					; redefine SP (atomic)
				
                ;
                ; Restore R4-R11 & LR, return to OSSched().
                ;
            	POP			{R4-R7} 				; restore R4-R11 & LR and return to
            	MOV			R8,R4					;  OSSched() via LR that was 
            	MOV			R9,R5					;  pushed in OSDispatch(). Use
            	MOV			R10,R6					;  BX Rn to ensure ARM/Thumb  
            	MOV			R11,R7					;  change if required. 
            	POP			{R4-R7}					;  "" 
            	POP			{R0}					;  ""
               	BX			R0						;  ""
				NOP
				NOP	  								; added to avoid padding warning
				
				ENDIF								; ARM or Thumb mode
            
                ;
                ; End of OSCtxSw()
                ;
                ENDP
    				

; /**********************************************************
; ****                                                   ****
; **                                                       **
; End of module
; **                                                       **
; ****                                                   ****
; **********************************************************/

				END