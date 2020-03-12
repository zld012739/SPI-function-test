###########################################################################################
# The contents of this file are subject to the Pumpkin Salvo
# License (the "License").  You may not use this file except
# in compliance with the License. You may obtain a copy of
# the License at http://www.pumpkininc.com, or from
# warranty@pumpkininc.com.
# 
# Software distributed under the License is distributed on an
# "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either express
# or implied. See the License for specific language governing
# the warranty and the rights and limitations under the
# License.
# 
# The Original Code is Salvo - The RTOS that runs in tiny
# places(TM). Copyright (C) 1995-2011 Pumpkin, Inc. and its
# Licensor(s). All Rights Reserved.
# 
# $Source: C:\\RCS\\D\\Pumpkin\\Salvo\\Src\\Make\\IARARM\\Makefile,v $
# $Author: aek $
# $Revision: 3.5 $
# $Date: 2011-07-06 17:04:57-07 $
#
# Salvo Library Makefiles for IAR ARM
#
# Use of '""' in pathnames enables pathnames with spaces in them.
#
# ARM:
#   EWARM 5 in e.g. C:\Program Files\IAR Systems\Embedded Workbench 5.4_0
#   EWARM 6 in e.g. C:\Program Files\IAR Systems\Embedded Workbench 6.0
#


###########################################################################################
#
# "Include Guard"
#
BUILD_FOR_IARARM = FALSE

ifeq ($(F),iararm)
  BUILD_FOR_IARARM = TRUE
endif

ifeq ($(BUILD_FOR_IARARM),TRUE)

###########################################################################################
#
# Home directory & executables used. Yours may vary based on the install location for
#  IAR Embedded Workbench.
#
IAR-ICCARM-V5-HOME     ="c:/Program Files/IAR Systems/Embedded Workbench 5.4_0/arm"
IAR-ICCARM-V6-HOME     ="c:/Program Files/IAR Systems/Embedded Workbench 6.0/arm"

IAR-ICCARM-V5          =$(IAR-ICCARM-V5-HOME)/bin/iccarm.exe --silent
IAR-IASMARM-V5         =$(IAR-ICCARM-V5-HOME)/bin/iasmarm.exe -S
IAR-IARCHIVE-V5        =$(IAR-ICCARM-V5-HOME)/bin/iarchive.exe
IAR-ICCARM-V6          =$(IAR-ICCARM-V6-HOME)/bin/iccarm.exe --silent
IAR-IASMARM-V6         =$(IAR-ICCARM-V6-HOME)/bin/iasmarm.exe -S
IAR-IARCHIVE-V6        =$(IAR-ICCARM-V6-HOME)/bin/iarchive.exe


###########################################################################################
#
#  Default command-line options
#
IAR-ARM-V5-OPTS=$(LIBRARY-OPTS-D) -I$(INC)/ -I$(INC2)/ -I$(IAR-ICCARM-V5-HOME)/inc/ -I$(IAR-ICCARM-V5-HOME)/inc/clib/
IAR-ARM-V6-OPTS=$(LIBRARY-OPTS-D) -I$(INC)/ -I$(INC2)/ -I$(IAR-ICCARM-V6-HOME)/inc/ -I$(IAR-ICCARM-V6-HOME)/inc/clib/


###########################################################################################
#
# Compile & Assemble Each Salvo Source-code Module (C & Assembly)
#
# IAR ARM C Compiler for ARM7 TDMI 
#
# libsalvo[f|l]iararm[4][b|l][a|t][-|i][-|i][a|d|e|m|t].a
# 
# SUFFIX:           4 - ARM7TDMI
# ENDIANNESS:       b - big-endian
#                   l - little-endian
# INSTRUCTION SET:  a - ARM mode
#                   t - Thumb mode
# INTERWORKING      - - intermixed CPU mode veneer disabled
#                   i - intermixed CPU mode veneer enabled
# OPTION            - - no debugging information included
#                   i - debugging information included
# No variants.
#
ifeq ($(F),iararm)

  V         =OSNONE
  
  ifeq ($(CVER),5)
    CC        =$(IAR-ICCARM-V5)
    CFLAGS    =$(IAR-ARM-V5-OPTS)
    ASSEMBLER =$(IAR-IASMARM-V5)
    AFLAGS    =-o $@
    LIBDIR    =iararm-v5
    LIBRARIAN =$(IAR-IARCHIVE-V5)
  endif    
  
  ifeq ($(CVER),6)
    CC        =$(IAR-ICCARM-V6)
    CFLAGS    =$(IAR-ARM-V6-OPTS)
    ASSEMBLER =$(IAR-IASMARM-V6)
    AFLAGS    =-o $@
    LIBDIR    =iararm-v6
    LIBRARIAN =$(IAR-IARCHIVE-V6)
  endif    
  
  CC_OPT1   =-e
  CC_OPT2   =-o $@ $<
  
  LIB_NAME  =salvoiararm.r79
  CTXSW_SRC =IARARM/salvoportiararm.s79
  CTXSW_OBJ =IARARM/salvoportiararm.o
  EXTRA_OBJS=IARARM/salvohook_idle IARARM/salvohook_interrupt IARARM/salvohook_wdt
  RMOBJS_LIST=*.o IARARM/*.o
  OBJ_SUFFIX=.o
  LIB_SUFFIX=.a

  
  ##############################################
  #
  # ARM family / architecture
  #
  ifeq ($(SU),4)
    CFLAGS +=--cpu ARM7TDMI-S
  endif
  
  
  ##############################################
  #
  # little- or big-endian
  #
  ifeq ($(END),OSB)
    CFLAGS +=--endian big
    AFLAGS += -e
  endif
  ifeq ($(END),OSL)
    CFLAGS +=--endian little
  endif
  
  
  ##############################################
  #
  # ARM or Thumb mode
  #
  ifeq ($(ISET),OSA)
    CFLAGS +=--cpu_mode arm
  endif
  ifeq ($(ISET),OST)
    CFLAGS +=--cpu_mode thumb
  endif
  
  
  ##############################################
  #
  # Interworking support
  #
  ifeq ($(IW),OSI)
    CFLAGS +=--interwork
  endif


  ##############################################
  #
  # Include debugging support
  #
  ifeq ($(O),OSI)
    CC_OPT1 +=--debug
    AFLAGS  +=-r
  endif
  
endif  


###########################################################################################
#
#  Rules for Assembly-language Context Switcher 
# 
#
$(CTXSW_OBJ) : $(CTXSW_SRC)
  $(ASSEMBLER) $(AFLAGS) $(CTXSW_SRC)

%.o : %.c $(INC)/salvo.h
  $(CC) $(CC_OPT1) $(CPPFLAGS) $(CFLAGS) $(CC_OPT2)
  
%.o : $(LIBDIR)/%.c $(INC)/salvo.h
  $(CC) $(CC_OPT1) $(CPPFLAGS) $(CFLAGS) $(CC_OPT2)


###########################################################################################
#
#  Compiler Example
#

###########################################################################################
#
# End of "Include Guard"
#
endif
