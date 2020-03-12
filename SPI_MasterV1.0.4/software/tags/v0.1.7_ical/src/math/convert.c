#include <stdint.h>
#include "xbowsp_IQmath.h"

/**********************************************************************************
* Module name: convert_ieee_float_to_c33
*
* Description: 
*  Converts ieee single precision float format programmed into EEPROM into C33 
*  special floating point format. Taken from section 5.4.1 of the C3x users 
*  manual
* 
* Trace: 	[SDD_CONVERT_IEEE_FLOAT_TO_C33_01 <-- SRC_CONVERT_IEEE_FLOAT_TO_C33]
* 			[SDD_CONVERT_IEEE_FLOAT_TO_C33_02 <-- SRC_CONVERT_IEEE_FLOAT_TO_C33] 
* 			[SDD_CONVERT_IEEE_FLOAT_TO_C33_03 <-- SRC_CONVERT_IEEE_FLOAT_TO_C33]
* 			[SDD_CONVERT_IEEE_FLOAT_TO_C33_04 <-- SRC_CONVERT_IEEE_FLOAT_TO_C33]
* 			[SDD_CONVERT_IEEE_FLOAT_TO_C33_05 <-- SRC_CONVERT_IEEE_FLOAT_TO_C33]
* 			[SDD_CONVERT_IEEE_FLOAT_TO_C33_06 <-- SRC_CONVERT_IEEE_FLOAT_TO_C33]
* 			[SDD_CONVERT_IEEE_FLOAT_TO_C33_07 <-- SRC_CONVERT_IEEE_FLOAT_TO_C33]
* Input parameters: ieee  32bit floating point number read as a uint32_t
*
* Output parameters: none
*
* Return value: C33 32 bit unsigned int
*********************************************************************************/
#define MAX_E_IEEE 	0x7f800000
#define MASK_E_IEEE 0x7f800000
#define MASK_M_IEEE 0x007FFFFF
#define NEG_IEEE  	0x80000000
#define MAX_NEG_C33	0x7F800000
#define MAX_POS_C33 0x7F7FFFFF
#define ZERO_C33	0x80000000
#define NORM_E_C33  0x7F000000
#define NEG_C33		0x00800000


uint32_t convert_ieee_float_to_c33(uint32_t ieee) 
{
	uint32_t tmp;
	
	/*case 1 and 2 */
	if ((ieee & MASK_E_IEEE)==MAX_E_IEEE) {
		if (ieee & NEG_IEEE) {
			return MAX_NEG_C33;		/*case 1*/
		} else {
			return MAX_POS_C33;		/*case 2*/
		}                    
	}  
	
	if (!(ieee & MASK_E_IEEE)) {
		return ZERO_C33;      		/*case 6*/
	}    
	
	if (!(ieee & NEG_IEEE)) { 						/*case 3*/ /* positive in range number */  
		tmp = (ieee & MASK_E_IEEE) <<1; 			/* exp only */
		tmp-= NORM_E_C33;
		tmp+= (ieee & MASK_M_IEEE); 
		return tmp;
	}  
	
	if(!(ieee & MASK_M_IEEE)) {                     /*case 5*/
		tmp = (ieee & MASK_E_IEEE) <<1; 			/* exp only */
		tmp-= ZERO_C33; 							
		tmp|= NEG_C33;
		return tmp;                         
	}	
													/*case 4*/
	tmp = (ieee & MASK_E_IEEE) <<1; 				/* exp only */
	tmp-= NORM_E_C33;
	tmp|= NEG_C33;
	tmp+= ((~ieee) & MASK_M_IEEE) +1;
	return tmp;

} /* end convert_ieee_float_to_c33 */   
/**********************************************************************************
* Function name:  IQ27toF
*
* Description: 
*	- scales  C2000's IQ27 type into TI VC33's double type
*               
* Trace:
* [SDD_IQ27TOF <-- SRC_IQ27TOF]
*
* Input parameters: 
*       tmp: the number to be scaled.
* Output parameters: none
*
* Return value: 
*       The double type data.
* Author Dong An 
*********************************************************************************/
double IQ27toF (IQ27 tmp)	{
  	return((double)(tmp*IQ27_TO_DOUBLE));
}  /* End  double IQ27toF (iq27 tmp)  */

