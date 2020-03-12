###########################################################################################
#
#	Salvo Library Makefiles for ARM RealView C Compiler 
#
#

###########################################################################################
#
#	"Include Guard"
#
BUILD_FOR_ARMRV = FALSE

ifeq ($(F),armrv4)
  BUILD_FOR_ARMRV = TRUE
endif
ifeq ($(F),armrvcm3)
  BUILD_FOR_ARMRV = TRUE
endif

ifeq ($(BUILD_FOR_ARMRV),TRUE)



###########################################################################################
#
#	Overrides, etc.
#
CUSTOM_CC_RULES 		=1


###########################################################################################
#
#	Home directory & executables used
#
ARMRV-HOME	   			=C:/Keil/ARM

ARMRV-CC        		=$(ARMRV-HOME)/BIN30/ArmCC
ARMRV-AS        		=$(ARMRV-HOME)/BIN30/ArmAsm
ARMRV-LIB        		=$(ARMRV-HOME)/BIN30/ArmAr


###########################################################################################
#
#	Default command-line options
#
ARMRV-CC-OPTS  			=$(LIBRARY-OPTS-E) -I$(INC)/ -I$(INC2)/ -I$(ARMRV-HOME)/INC/ 


###########################################################################################
#
#	Compile & Assemble Each Salvo Source-code Module (C & Assembly)
#
#   ARM RealView C compiler
#
#	salvo[f|l]armrv4[b|l][a|t][-|i][-|i][a|d|e|m|t].lib
#
#
ifeq ($(F),armrv4)

	CC        =$(ARMRV-CC)
	CC_OPT1   =-c --device DARMP -O0
	CC_OPT2   = 
	CFLAGS    =$(ARMRV-CC-OPTS)
	ASSEMBLER =$(ARMRV-AS)
	AFLAGS    =--device DARMP --xref
	LIB_FLY   =1
	LIB_NAME  =salvoarmrv4.lib
	LIBDIR    =ARMRV
	LIBRARIAN =$(ARMRV-LIB)
	LIBFLAGS  =--create -r
	OBJ_SUFFIX=.o
	LIB_SUFFIX=.lib
  EXTRA_OBJS=$(LIBDIR)/salvohook_idle $(LIBDIR)/salvohook_interrupt $(LIBDIR)/salvohook_wdt
  
  
	##############################################
	#
	# ARM or Thumb mode (default: ARM)
	#
	ifeq ($(ISET),OSA)
		CFLAGS +=--arm
		AFLAGS +=--32
	endif

	ifeq ($(ISET),OST)
		CFLAGS +=--thumb
		AFLAGS +=--16
	endif


	##############################################
	#
	# Interwork veneer (default: no interwork)
	#
	ifeq ($(IW),OSI)
		CC_OPT2 +=--apcs=interwork
		AFLAGS  +=--apcs=interwork
	endif


	##############################################
	#
	# Include debugging information
	#
	ifeq ($(O),OSI)
		CC_OPT2 +=-g
		AFLAGS  +=-g
	endif
 
endif  


###########################################################################################
#
#	Compile & Assemble Each Salvo Source-code Module (C & Assembly)
#
#   ARM RealView C compiler
#
#	salvo[f|l]armrvcm3[-|i][a|d|e|m|t].lib
#
#
ifeq ($(F),armrvcm3)

  CC        =$(ARMRV-CC)
  CC_OPT1   =-c --device DLM -O0
  CC_OPT2   = 
  CFLAGS    =$(ARMRV-CC-OPTS) -I$(ARMRV-HOME)/INC/Luminary/
  ASSEMBLER =$(ARMRV-AS)
  AFLAGS    =--device DLM --apcs=interwork --xref --pd "FULL_VTABLE SETA 1" 
  LIB_FLY   =1
  LIB_NAME  =salvoarmrvcm3.lib
  LIBDIR    =ARMRV
  LIBRARIAN =$(ARMRV-LIB)
  LIBFLAGS  =--create -r
  OBJ_SUFFIX=.o
  LIB_SUFFIX=.lib
  
  
  ##############################################
  #
  # Include debugging information
  #
  ifeq ($(O),OSI)
    CC_OPT2 +=-g
    AFLAGS  +=-g
  endif
 
endif  


###########################################################################################
#
#	Rules for Assembly-language Context Switcher   
#
salvoportarmrv4.o : $(LIBDIR)/salvoportarmrv4.s
	# $(CC) --device DLM -O0 -E $(LIBDIR)/salvoportarmrv4.s $(CC_OPT2) $(CFLAGS) > $(LIBDIR)/salvoportarmrv4.out
	$(ASSEMBLER) $(AFLAGS) $(LIBDIR)/salvoportarmrv4.s -o salvoportarmrv4.o

salvoportarmrvcm3.o : $(LIBDIR)/salvoportarmrvcm3.s
	$(ASSEMBLER) $(AFLAGS) $(LIBDIR)/salvoportarmrvcm3.s -o salvoportarmrvcm3.o

	
###########################################################################################
#
#	Custom CC Rules
#
ifeq ($(CUSTOM_CC_RULES),1)

%.o: %.c $(INC)/salvo.h
	$(CC) $(CC_OPT1) $< -o $*.o $(CC_OPT2) $(CFLAGS)
	
endif


###########################################################################################
#
#	Build libraries.
#
#	See Makefile2	
	
			
###########################################################################################
#
#	Compiler Example
#
#	C:\Keil\ARM\BIN30\ArmCC 
#		-c --device DLM 
#		-g 
#		-O0 
#		-I..\..\..\..\..\Luminary_DK-LM3S1XX 
#		-I..\..\..\..\..\..\..\..\Inc 
#		-I..\Pro 
#		-I "C:\Keil\ARM\INC\Luminary" 
#		-o ".\Obj\salvobinsem.o" 
#		"..\..\..\..\..\..\..\..\Src\salvobinsem.c"
#
#	Assembler Example:
#
#  C:\Keil\ARM\BIN30\ArmAsm 
#		--device DLM 
#		-g 
#		--apcs=interwork 
#		-I "C:\Keil\ARM\INC\Luminary" 
#		--pd "FULL_VTABLE SETA 1" 
#		--list ".\Lst\salvoportarmrvcm3.lst" 
#		--xref 
#		-o .\Obj\salvoportarmrvcm3.o 
#		"..\..\..\..\..\..\..\..\Src\ARMRV\salvoportarmrvcm3.s"
#
#	Archiving Example:
#
#	C:\Keil\ARM\BIN30\ArmAr
#		--create ".\Obj\tut5pro.lib" 
#  		.\obj\startup.o
#		...
#		.\obj\salvohook_interrupt_cm3.o
#

###########################################################################################
#
# End of "Include Guard"
#
endif
