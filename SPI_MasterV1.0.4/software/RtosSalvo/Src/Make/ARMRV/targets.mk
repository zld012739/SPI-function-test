###########################################################################################
#
#   targets: libraries for ARM RealView C compiler. 
#
#	for ARM7TDMI and Cortex-M3
#
#	salvo[f|l]armrv4[b|l][a|t][-|i][-|i][a|d|e|m|t].lib
#
#		[f|l]:				freeware or standard library
#       [b|l]:              big- or little-endian
#       [a|t]:              ARM or Thumb mode
#		[-|i]:              CPU mode intermixing (veneer)
#       [-|i]:          	debug information included
#		[a|d|e|m|t]:		Salvo library types
#
#	salvo[f|l]armrvcm3[-|i][a|d|e|m|t].lib
#
#		[f|l]:				freeware or standard library
#       [-|i]:          	debug information included
#		[a|d|e|m|t]:		Salvo library types
#
#
ARM_RV4_STD_FAMILIES				=	armrv4
ARM_RV4_OPT_FAMILIES				=	armrv4-opt

ARM_RVCM3_STD_FAMILIES        		= 	armrvcm3
ARM_RVCM3_OPT_FAMILIES        		= 	armrvcm3-opt


###########################################################################################
#
# For ARM7TDMI
#
# Included in Salvo distributions.
#
ARM_RV4_STD_LIBRARIES	 			=	$(ARM_RV4_STD_LIBRARIES_F_L_A_I_N) \
										$(ARM_RV4_STD_LIBRARIES_F_L_T_I_N) \
									   	$(ARM_RV4_STD_LIBRARIES_L_L_A_I_N) \
									   	$(ARM_RV4_STD_LIBRARIES_L_L_T_I_N) \
									   	$(ARM_RV4_STD_LIBRARIES_L_L_A_I_I) \
									   	$(ARM_RV4_STD_LIBRARIES_L_L_T_I_I) \ 

# Lite
ARM_RV4_STD_LIBRARIES_F_L_A_I_N 	= 	salvofarmrv4lai-t.lib 

ARM_RV4_STD_LIBRARIES_F_L_T_I_N 	= 	salvofarmrv4lti-t.lib 


# LE
ARM_RV4_STD_LIBRARIES_L_L_A_I_N		= 	salvolarmrv4lai-a.lib \
                       				  	salvolarmrv4lai-d.lib \
                       				  	salvolarmrv4lai-e.lib \
                       			      	salvolarmrv4lai-m.lib \
                       			      	salvolarmrv4lai-t.lib 
 	
ARM_RV4_STD_LIBRARIES_L_L_T_I_N		= 	salvolarmrv4lti-a.lib \
                       				  	salvolarmrv4lti-d.lib \
                       				  	salvolarmrv4lti-e.lib \
                       			      	salvolarmrv4lti-m.lib \
                       			      	salvolarmrv4lti-t.lib 


# Pro 	
ARM_RV4_STD_LIBRARIES_L_L_A_I_I		= 	salvolarmrv4laiia.lib \
                       				  	salvolarmrv4laiid.lib \
                       				  	salvolarmrv4laiie.lib \
                       			      	salvolarmrv4laiim.lib \
                       			      	salvolarmrv4laiit.lib 
 	
ARM_RV4_STD_LIBRARIES_L_L_T_I_I		= 	salvolarmrv4ltiia.lib \
                       				  	salvolarmrv4ltiid.lib \
                       				  	salvolarmrv4ltiie.lib \
                       			      	salvolarmrv4ltiim.lib \
                       			      	salvolarmrv4ltiit.lib 
 	


###########################################################################################
#
# Optional. Does not include libraries already part of the standard set.
#
ARM_RV4_OPT_LIBRARIES				= 	$(ARM_RV4_OPT_LIBRARIES_F_N) 

ARM_RV4_OPT_LIBRARIES_F_L_A_I_N		= 	salvofarmrv4lai-a.lib \
                       				  	salvofarmrv4lai-d.lib \
                       				  	salvofarmrv4lai-e.lib \
                       			      	salvofarmrv4lai-m.lib 


                       			
                                              
###########################################################################################
#
# For Cortex-M3
#
# Included in Salvo distributions.
#
ARM_RVCM3_STD_LIBRARIES	 			=	$(ARM_RVCM3_STD_LIBRARIES_F_N) \
									   	$(ARM_RVCM3_STD_LIBRARIES_L_N) \
									   	$(ARM_RVCM3_STD_LIBRARIES_L_I) 

ARM_RVCM3_STD_LIBRARIES_F_N 		= 	salvofarmrvcm3-t.lib 

ARM_RVCM3_STD_LIBRARIES_L_N		 	= 	salvolarmrvcm3-a.lib \
                       				  	salvolarmrvcm3-d.lib \
                       				  	salvolarmrvcm3-e.lib \
                       			      	salvolarmrvcm3-m.lib \
                       			      	salvolarmrvcm3-t.lib 
 	
ARM_RVCM3_STD_LIBRARIES_L_I		 	= 	salvolarmrvcm3ia.lib \
                       				  	salvolarmrvcm3id.lib \
                       				  	salvolarmrvcm3ie.lib \
                       			      	salvolarmrvcm3im.lib \
                       			      	salvolarmrvcm3it.lib 
 	

###########################################################################################
#
# Optional. Does not include libraries already part of the standard set.
#
ARM_RVCM3_OPT_LIBRARIES				= 	$(ARM_RVCM3_OPT_LIBRARIES_F_N) 

ARM_RVCM3_OPT_LIBRARIES_F_N		 	= 	salvofarmrvcm3-a.lib \
                       				  	salvofarmrvcm3-d.lib \
                       				  	salvofarmrvcm3-e.lib \
                       			      	salvofarmrvcm3-m.lib 
                       			      	