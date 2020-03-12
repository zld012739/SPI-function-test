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
# $Source: C:\\RCS\\D\\Pumpkin\\Salvo\\Src\\Make\\IARARM\\targets.mk,v $
# $Author: aek $
# $Revision: 3.3 $
# $Date: 2011-07-06 17:04:57-07 $
#
# targets: libraries for IAR ARM compiler (ARM7TDMI-S target). 
#
# libsalvo[f|l]iararm[4][b|l][a|t][-|i][-|i][a|d|e|m|t].a
# 
# [f|l]:              Salvo library type
# [4]:                ARM7TDMI
# [b|l]:              big-endian or little-endian
# [a|t]:              ARM or Thumb mode
# [-|i]:              intermixed CPU mode veneer disabled or enabled
# [-|i]:              debug information included
# [a|d|e|m|t]:        Salvo library configurations
#
IAR_ARM_STD_FAMILIES_V5  = iararm-v5
IAR_ARM_OPT_FAMILIES_V5  = iararm-v5-opt

IAR_ARM_STD_FAMILIES_V6  = iararm-v6
IAR_ARM_OPT_FAMILIES_V6  = iararm-v6-opt


###########################################################################################
#
# Included in Salvo distributions.
#
# With the wide range of different kinds of libraries (e.g., Thumb vs. ARM,
#  little-endian vs. big-endian), the standard libraries are only -t configs.
#
IAR_ARM_STD_LIBRARIES = $(IAR_ARM_STD_LIBRARIES_F_4_L_A_I_N) \
                        $(IAR_ARM_STD_LIBRARIES_F_4_B_A_I_N) \
                        $(IAR_ARM_STD_LIBRARIES_F_4_L_T_I_N) \
                        $(IAR_ARM_STD_LIBRARIES_F_4_B_T_I_N) \
                        $(IAR_ARM_STD_LIBRARIES_L_4_L_A_I_N) \
                        $(IAR_ARM_STD_LIBRARIES_L_4_B_A_I_N) \
                        $(IAR_ARM_STD_LIBRARIES_L_4_L_T_I_N) \
                        $(IAR_ARM_STD_LIBRARIES_L_4_B_T_I_N) \
                        $(IAR_ARM_STD_LIBRARIES_L_4_L_A_I_I) \
                        $(IAR_ARM_STD_LIBRARIES_L_4_B_A_I_I) \
                        $(IAR_ARM_STD_LIBRARIES_L_4_L_T_I_I) \
                        $(IAR_ARM_STD_LIBRARIES_L_4_B_T_I_I) 

IAR_ARM_STD_LIBRARIES_F_4_L_A_I_N = libsalvofiararm4lai-t.a
  
IAR_ARM_STD_LIBRARIES_F_4_B_A_I_N = libsalvofiararm4bai-t.a
  
IAR_ARM_STD_LIBRARIES_F_4_L_T_I_N = libsalvofiararm4lti-t.a

IAR_ARM_STD_LIBRARIES_F_4_B_T_I_N = libsalvofiararm4bti-t.a

IAR_ARM_STD_LIBRARIES_L_4_L_A_I_N = libsalvoliararm4lai-t.a
                    
IAR_ARM_STD_LIBRARIES_L_4_B_A_I_N = libsalvoliararm4bai-t.a
                    
IAR_ARM_STD_LIBRARIES_L_4_L_T_I_N = libsalvoliararm4lti-t.a
                    
IAR_ARM_STD_LIBRARIES_L_4_B_T_I_N = libsalvoliararm4bti-t.a
                    
IAR_ARM_STD_LIBRARIES_L_4_L_A_I_I = libsalvoliararm4laiit.a
                    
IAR_ARM_STD_LIBRARIES_L_4_B_A_I_I = libsalvoliararm4baiit.a
                    
IAR_ARM_STD_LIBRARIES_L_4_L_T_I_I = libsalvoliararm4ltiit.a
                    
IAR_ARM_STD_LIBRARIES_L_4_B_T_I_I = libsalvoliararm4btiit.a
                    

###########################################################################################
#
# Optional. Does not include libraries already part of the standard set.
#
IAR_ARM_OPT_LIBRARIES = $(IAR_ARM_OPT_LIBRARIES_L_4_L_A_I_N) \
                        $(IAR_ARM_OPT_LIBRARIES_L_4_B_A_I_N) \
                        $(IAR_ARM_OPT_LIBRARIES_L_4_L_T_I_N) \
                        $(IAR_ARM_OPT_LIBRARIES_L_4_B_T_I_N) \
                        $(IAR_ARM_OPT_LIBRARIES_L_4_L_A_I_I) \
                        $(IAR_ARM_OPT_LIBRARIES_L_4_B_A_I_I) \
                        $(IAR_ARM_OPT_LIBRARIES_L_4_L_T_I_I) \
                        $(IAR_ARM_OPT_LIBRARIES_L_4_B_T_I_I) \
                        $(IAR_ARM_OPT_LIBRARIES_F_4_L_A_N_N) \
                        $(IAR_ARM_OPT_LIBRARIES_F_4_B_A_N_N) \
                        $(IAR_ARM_OPT_LIBRARIES_F_4_L_A_I_N) \
                        $(IAR_ARM_OPT_LIBRARIES_F_4_B_A_I_N) \
                        $(IAR_ARM_OPT_LIBRARIES_F_4_L_T_N_N) \
                        $(IAR_ARM_OPT_LIBRARIES_F_4_B_T_N_N) \
                        $(IAR_ARM_OPT_LIBRARIES_F_4_L_T_I_N) \
                        $(IAR_ARM_OPT_LIBRARIES_F_4_B_T_I_N) \
                        $(IAR_ARM_OPT_LIBRARIES_L_4_L_A_N_N) \
                        $(IAR_ARM_OPT_LIBRARIES_L_4_B_A_N_N) \
                        $(IAR_ARM_OPT_LIBRARIES_L_4_L_A_N_I) \
                        $(IAR_ARM_OPT_LIBRARIES_L_4_B_A_N_I) \
                        $(IAR_ARM_OPT_LIBRARIES_L_4_L_T_N_N) \
                        $(IAR_ARM_OPT_LIBRARIES_L_4_B_T_N_N) \
                        $(IAR_ARM_OPT_LIBRARIES_L_4_L_T_N_I) \
                        $(IAR_ARM_OPT_LIBRARIES_L_4_B_T_N_I) 

IAR_ARM_OPT_LIBRARIES_L_4_L_A_I_N  = libsalvoliararm4lai-a.a \
                                     libsalvoliararm4lai-d.a \
                                     libsalvoliararm4lai-e.a \
                                     libsalvoliararm4lai-m.a 
                    
IAR_ARM_OPT_LIBRARIES_L_4_B_A_I_N  = libsalvoliararm4bai-a.a \
                                     libsalvoliararm4bai-d.a \
                                     libsalvoliararm4bai-e.a \
                                     libsalvoliararm4bai-m.a 
                    
IAR_ARM_OPT_LIBRARIES_L_4_L_T_I_N  = libsalvoliararm4lti-a.a \
                                     libsalvoliararm4lti-d.a \
                                     libsalvoliararm4lti-e.a \
                                     libsalvoliararm4lti-m.a 
                    
IAR_ARM_OPT_LIBRARIES_L_4_B_T_I_N  = libsalvoliararm4bti-a.a \
                                     libsalvoliararm4bti-d.a \
                                     libsalvoliararm4bti-e.a \
                                     libsalvoliararm4bti-m.a 
                    
IAR_ARM_OPT_LIBRARIES_L_4_L_A_I_I  = libsalvoliararm4laiia.a \
                                     libsalvoliararm4laiid.a \
                                     libsalvoliararm4laiie.a \
                                     libsalvoliararm4laiim.a 
                    
IAR_ARM_OPT_LIBRARIES_L_4_B_A_I_I  = libsalvoliararm4baiia.a \
                                     libsalvoliararm4baiid.a \
                                     libsalvoliararm4baiie.a \
                                     libsalvoliararm4baiim.a 
                    
IAR_ARM_OPT_LIBRARIES_L_4_L_T_I_I  = libsalvoliararm4ltiia.a \
                                     libsalvoliararm4ltiid.a \
                                     libsalvoliararm4ltiie.a \
                                     libsalvoliararm4ltiim.a 
                    
IAR_ARM_OPT_LIBRARIES_L_4_B_T_I_I  = libsalvoliararm4btiia.a \
                                     libsalvoliararm4btiid.a \
                                     libsalvoliararm4btiie.a \
                                     libsalvoliararm4btiim.a 

IAR_ARM_OPT_LIBRARIES_F_4_L_A_N_N  = libsalvofiararm4la--a.a \
                                     libsalvofiararm4la--d.a \
                                     libsalvofiararm4la--e.a \
                                     libsalvofiararm4la--m.a \
                                     libsalvofiararm4la--t.a 
                                
IAR_ARM_OPT_LIBRARIES_F_4_B_A_N_N  = libsalvofiararm4ba--a.a \
                                     libsalvofiararm4ba--d.a \
                                     libsalvofiararm4ba--e.a \
                                     libsalvofiararm4ba--m.a \
                                     libsalvofiararm4ba--t.a 
                                
IAR_ARM_OPT_LIBRARIES_F_4_L_A_I_N  = libsalvofiararm4lai-a.a \
                                     libsalvofiararm4lai-d.a \
                                     libsalvofiararm4lai-e.a \
                                     libsalvofiararm4lai-m.a 
                                
IAR_ARM_OPT_LIBRARIES_F_4_B_A_I_N  = libsalvofiararm4bai-a.a \
                                     libsalvofiararm4bai-d.a \
                                     libsalvofiararm4bai-e.a \
                                     libsalvofiararm4bai-m.a 
                                
IAR_ARM_OPT_LIBRARIES_F_4_L_T_N_N  = libsalvofiararm4lt--a.a \
                                     libsalvofiararm4lt--d.a \
                                     libsalvofiararm4lt--e.a \
                                     libsalvofiararm4lt--m.a \
                                     libsalvofiararm4lt--t.a 
                                
IAR_ARM_OPT_LIBRARIES_F_4_B_T_N_N  = libsalvofiararm4bt--a.a \
                                     libsalvofiararm4bt--d.a \
                                     libsalvofiararm4bt--e.a \
                                     libsalvofiararm4bt--m.a \
                                     libsalvofiararm4bt--t.a 
                                
IAR_ARM_OPT_LIBRARIES_F_4_L_T_I_N   = libsalvofiararm4lti-a.a \
                                libsalvofiararm4lti-d.a \
                                libsalvofiararm4lti-e.a \
                                libsalvofiararm4lti-m.a 
                                
IAR_ARM_OPT_LIBRARIES_F_4_B_T_I_N   = libsalvofiararm4bti-a.a \
                                libsalvofiararm4bti-d.a \
                                libsalvofiararm4bti-e.a \
                                libsalvofiararm4bti-m.a 
                                
IAR_ARM_OPT_LIBRARIES_L_4_L_A_N_N   = libsalvoliararm4la--a.a \
                                libsalvoliararm4la--d.a \
                                libsalvoliararm4la--e.a \
                                libsalvoliararm4la--m.a \
                                libsalvoliararm4la--t.a 
                                
IAR_ARM_OPT_LIBRARIES_L_4_B_A_N_N   = libsalvoliararm4ba--a.a \
                                libsalvoliararm4ba--d.a \
                                libsalvoliararm4ba--e.a \
                                libsalvoliararm4ba--m.a \
                                libsalvoliararm4ba--t.a 
                                
IAR_ARM_OPT_LIBRARIES_L_4_L_A_N_I   = libsalvoliararm4la-ia.a \
                                libsalvoliararm4la-id.a \
                                libsalvoliararm4la-ie.a \
                                libsalvoliararm4la-im.a \
                                libsalvoliararm4la-it.a 
                                
IAR_ARM_OPT_LIBRARIES_L_4_B_A_N_I   = libsalvoliararm4ba-ia.a \
                                libsalvoliararm4ba-id.a \
                                libsalvoliararm4ba-ie.a \
                                libsalvoliararm4ba-im.a \
                                libsalvoliararm4ba-it.a 
                                
IAR_ARM_OPT_LIBRARIES_L_4_L_T_N_N   = libsalvoliararm4lt--a.a \
                                libsalvoliararm4lt--d.a \
                                libsalvoliararm4lt--e.a \
                                libsalvoliararm4lt--m.a \
                                libsalvoliararm4lt--t.a 
                                
IAR_ARM_OPT_LIBRARIES_L_4_B_T_N_N   = libsalvoliararm4bt--a.a \
                                libsalvoliararm4bt--d.a \
                                libsalvoliararm4bt--e.a \
                                libsalvoliararm4bt--m.a \
                                libsalvoliararm4bt--t.a 
                                
IAR_ARM_OPT_LIBRARIES_L_4_L_T_N_I   = libsalvoliararm4lt-ia.a \
                                libsalvoliararm4lt-id.a \
                                libsalvoliararm4lt-ie.a \
                                libsalvoliararm4lt-im.a \
                                libsalvoliararm4lt-it.a 
                                
IAR_ARM_OPT_LIBRARIES_L_4_B_T_N_I   = libsalvoliararm4bt-ia.a \
                                libsalvoliararm4bt-id.a \
                                libsalvoliararm4bt-ie.a \
                                libsalvoliararm4bt-im.a \
                                libsalvoliararm4bt-it.a 
                                