/*********************************************************************************
* File name:  crm_packet.c
*
* File description: 
*   - utility functions for interfacing with CRM packets 
*               
*********************************************************************************/ 

#include "stm32f2xx.h"
#include "crm_packet.h"
#include "cast.h"

/*********************************************************************************
* Function name:	CrmPacketBytesToAccel
*
* Description:
*
* Input parameters:
*
* Output parameters:
*
* Return value:
*********************************************************************************/
CrmPacketAccelType CrmPacketBytesToAccel (const uint8_t bytes [])
{   
	return ((CrmPacketAccelType)Int16ToInt32(((bytes[0] & 0xff) << 8) | (bytes[1] & 0xff)));
}
/* end CrmPacketBytesToAccel */

/*********************************************************************************
* Function name:	CrmPacketBytesToMag
*
* Description:
*
* Input parameters:
*
* Output parameters:
*
* Return value:
*********************************************************************************/
CrmPacketMagType CrmPacketBytesToMag (const uint8_t bytes [])
{  
	return ((CrmPacketMagType)Int16ToInt32(((bytes[0] & 0xff) << 8) | (bytes[1] & 0xff)));
}
/* end CrmPacketBytesToMag */

/*********************************************************************************
* Function name:	CrmPacketBytesToModelNo
*
* Description:
*
* Input parameters:
*
* Output parameters:
*
* Return value:
*********************************************************************************/
CrmPacketModelNoType CrmPacketBytesToModelNo (const uint8_t bytes [])
{
	return ((CrmPacketModelNoType)(((bytes[0] & 0xff) << 8) | (bytes[1] & 0xff)));
}
/* end CrmPacketBytesToModelNo */

/*********************************************************************************
* Function name:	CrmPacketBytesToBit
*
* Description:
*
* Input parameters:
*
* Output parameters:
*
* Return value:
*********************************************************************************/
CrmPacketBitType CrmPacketBytesToBit (const uint8_t bytes [])
{
	return ((CrmPacketBitType)(((bytes[0] & 0xff) << 8) | (bytes[1] & 0xff)));
}
/* end CrmPacketBytesToBit */

/*********************************************************************************
* Function name:	CrmPacketBytesToChecksum
*
* Description:
*
* Input parameters:
*
* Output parameters:
*
* Return value:
*********************************************************************************/
CrmPacketChecksumType CrmPacketBytesToChecksum (const uint8_t bytes [])
{
	return ((CrmPacketChecksumType)(((bytes[0] & 0xff) << 8) | (bytes[1] & 0xff)));
}
/* end CrmPacketBytesToChecksum */   

/*********************************************************************************
* Function name:	CrmPacketCalculateChecksum
*
* Description:
*
* Input parameters:
*
* Output parameters:
*
* Return value:
*********************************************************************************/
CrmPacketChecksumType CrmPacketCalculateChecksum (const uint8_t data [], uint16_t length, const CrmPacketChecksumType seed)
{   
	uint16_t byteCount;
    
    CrmPacketChecksumType checksum = seed;
    
    for (byteCount = 0; byteCount < length; ++byteCount) {
    	checksum += data[byteCount];
    }
    
	return checksum;		
}  
/* end UcbPacketCalculateCrc */ 
