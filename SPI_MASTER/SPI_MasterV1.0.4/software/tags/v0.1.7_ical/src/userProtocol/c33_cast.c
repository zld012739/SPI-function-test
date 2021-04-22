/*********************************************************************************
* File name:  c33_cast.c
*
* File description: 
*   - functions for sign extending  
*               
* $Rev: 17458 $
* $Date: 2011-02-09 19:24:05 -0800 (Wed, 09 Feb 2011) $
* $Author: by-tdelong $
*********************************************************************************/ 
 
#include <stdint.h> 
#include "cast.h"

#define INT8_SIGN_BIT	0x80
#define INT16_SIGN_BIT	0x8000 
#define MAX_UINT8		0xFF
#define MAX_UINT16		0xFFFF

/*********************************************************************************
* Function name:	Int8ToInt32
*
* Description:	  This function will cast a int8 to a int32 to fix a TI compiler bug.
*
* Trace: [SDD_CAST_8TO32 <-- SRC_CAST_8TO32]
*
* Input parameters:	      int8_t int8Value
*
* Output parameters:	  None
*
* Return value:           int32_t int32Value
*********************************************************************************/ 
int32_t Int8ToInt32 (int8_t int8Value)
{   
	int32_t int32Value = int8Value;
    
    /* this code is a consequence of the TI compiler not treating chars, unsigned chars, or 
       shorts as 8/16-bit types: sign extension must be done manually out to the default type 
	   width of 32 bits */
    
	/* sign bit set? */
	if ((int32Value & INT8_SIGN_BIT) == INT8_SIGN_BIT) {
		/* extend the sign bit */
		int32Value -= (MAX_UINT8 + 1);	/* for example: 
										   -1 (8 bits) = 0xFF 
										   --> (0xFF - 0x100) = 0xFFFFFFFF = -1 (32 bits) */
	}
	
	return int32Value;
} 
 
/*********************************************************************************
* Function name:	Int16ToInt32
*
* Description:	   This function will cast a int16 to a int32 to fix a ti compiler bug
*
* Trace: [SDD_CAST_16TO32 <-- SRC_CAST_16TO32]
*
* Input parameters:	  int16_t int16Value
*
* Output parameters:  None 	
*
* Return value:       int32_t int32Value 
*********************************************************************************/
int32_t Int16ToInt32 (int16_t int16Value)
{
	int32_t int32Value = int16Value;
    
    /* this code is a consequence of the TI compiler not treating chars, unsigned chars, or 
       shorts as 8/16-bit types: sign extension must be done manually out to the default type 
	   width of 32 bits */
    
	/* sign bit set? */
	if ((int32Value & INT16_SIGN_BIT) == INT16_SIGN_BIT) {
		/* extend the sign bit */
		int32Value -= (MAX_UINT16 + 1);	/* for example: 
										   -1 (16 bits) = 0xFFFF 
										   --> (0xFFFF - 0x10000) = 0xFFFFFFFF = -1 (32 bits) */
	}
	
	return int32Value;
}
