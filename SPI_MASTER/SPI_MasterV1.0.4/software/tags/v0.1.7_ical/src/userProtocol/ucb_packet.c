/*********************************************************************************
* File name: 	ucb_packet.c
*
* File description:
* 	- utility functions for interfacing with UCB packets
*               
*********************************************************************************/ 

#include "stm32f2xx.h"
#include "dmu.h"
#include "crc.h"        
#include "ucb_packet.h"
 
static const UcbPacketCodeType UCB_PACKET_CODE [] = {
#	define INPUT_PACKET_TYPES 
#	define OUTPUT_PACKET_TYPES 
#	define UPDATE_PACKET_TYPES
#	define OUTPUT_ERROR_PACKET_TYPES
#	define PACKET_TYPE(constName, varName, code)	code,
#	include "ucb_packet_types.def"
#	undef INPUT_PACKET_TYPES 
#	undef OUTPUT_PACKET_TYPES 
#	undef UPDATE_PACKET_TYPES
#	undef OUTPUT_ERROR_PACKET_TYPES
#	undef PACKET_TYPE             
};
 
#define NUM_ARRAY_ITEMS(a)	(sizeof(a) / sizeof(a[0])) 
  
static const uint16_t NUM_PACKET_CODES = NUM_ARRAY_ITEMS(UCB_PACKET_CODE);
 

/*********************************************************************************
* Function name:	UcbPacketBytesToPacketType
*
* Description:	Convert the packet bytes into the packet type enum
*
* Trace: 
* [SDD_UCB_UNKNOWN_01 <-- SRC_UCB_PKT_ENUM]
* [SDD_HANDLE_PKT  <-- SRC_UCB_PKT_ENUM]
* [SDD_UCB_VALID_PACKET <-- SRC_UCB_PKT_ENUM] 
*
* Input parameters:	byte array containing 2 bytes of packet type
*
* Output parameters:	none
*
* Return value: packet type enum
*********************************************************************************/
UcbPacketTypeEnum UcbPacketBytesToPacketType (const uint8_t bytes [])
{   
	UcbPacketTypeEnum packetType = 0;
 
	UcbPacketCodeType receivedCode = (UcbPacketCodeType)(((bytes[0] & 0xff) << 8) | (bytes[1] & 0xff));
    
    BOOL valid = FALSE;
    
    /* check type against valid types */
	while ((packetType < NUM_PACKET_CODES) && (valid == FALSE)) {
	
        if ( (receivedCode == UCB_PACKET_CODE[packetType]) && (packetType != UCB_NAK) ) {
			valid = TRUE;
        } else {
			++packetType;
		}	
	}
	
	if (valid == FALSE) {
		packetType = UCB_ERROR_INVALID_TYPE;
	} 
	
	return packetType;			
}              
/* end UcbPacketBytesToPacketType */

/*********************************************************************************
* Function name:	UcbPacketBytesToPacketType
*
* Description:	Convert the packet type enum into bytes
*
* Trace: 
* [SDD_UCB_UNKNOWN_01 <-- SRC_UCB_PKT_STR]
* [SDD_HANDLE_PKT  <-- SRC_UCB_PKT_STR]
* [SDD_UCB_VALID_PACKET <-- SRC_UCB_PKT_STR] 
*
* Input parameters:	packet type enum
*
* Output parameters: byte array containing 2 bytes
*
* Return value: none
*********************************************************************************/
void UcbPacketPacketTypeToBytes (UcbPacketTypeEnum type, uint8_t bytes [])
{   
    if ((type >= 0) && (type < NUM_PACKET_CODES)) {     
        bytes[0] = (uint8_t)((UCB_PACKET_CODE[type] >> 8) & 0xff);
        bytes[1] = (uint8_t)(UCB_PACKET_CODE[type] & 0xff);
	} else {
		bytes[0] = 0;
		bytes[1] = 0;
	}
}
/* end UcbPacketPacketTypeToBytes */ 
 
/*********************************************************************************
* Function name:	UcbPacketBytesToPayloadLength
*
* Description:	Convert the packet bytes into the packet payload length
*
* Trace: 
* [SDD_UCB_STORE_DATA <-- SRC_UCB_PAYLEN] 
* [SDD_UCB_PKT_PAYLEN <-- SRC_UCB_PAYLEN]
*
* Input parameters:	byte array containing one byte
*
* Output parameters: none
*
* Return value: payload length
*********************************************************************************/
UcbPacketPayloadLengthType UcbPacketBytesToPayloadLength (const uint8_t bytes [])
{         
	return ((UcbPacketPayloadLengthType)(bytes[0] & 0xff));	
}              
/* end UcbPacketBytesToPayloadLength */  

/*********************************************************************************
* Function name:	UcbPacketPayloadLengthToBytes
*
* Description:	Convert the payload length into bytes 
*
* Trace: 
* [SDD_UCB_PROCESS_OUT <-- SRC_UCB_PKT_LENBYT]   
* [SDD_UCB_PKT_LENBYT <-- SRC_UCB_PKT_LENBYT]
*
* Input parameters:	payload type
*
* Output parameters: byte array, containing one byte
*
* Return value: none
*********************************************************************************/
void	UcbPacketPayloadLengthToBytes (UcbPacketPayloadLengthType type, uint8_t bytes [])
{
	bytes[0] = (uint8_t)(type & 0xff);
}  
/* end UcbPacketBytesToPayloadLength */

/*********************************************************************************
* Function name:	UcbPacketBytesToCrc
*
* Description:	
*
* Input parameters:	
*
* Output parameters:	
*
* Return value:
*********************************************************************************/
UcbPacketCrcType UcbPacketBytesToCrc (const uint8_t bytes [])
{                           
	return ((UcbPacketCrcType)(((bytes[0] & 0xff) << 8) | (bytes[1] & 0xff)));
}              
/* end UcbPacketBytesToCrc */    

/*********************************************************************************
* Function name:	UcbPacketCrcToBytes
*
* Description:	This function converts a value into a 2 byte string.
*
* Trace: [SDD_UCB_PROCESS_OUT <-- SRC_UCB_PKT_CRCSTR]
*
* Input parameters:	16-bit value (crc value)
*
* Output parameters: byte array with byte[0] containing the upper byte of input value
*
* Return value:
*********************************************************************************/
void UcbPacketCrcToBytes (const UcbPacketCrcType crc, uint8_t bytes [])
{
	bytes[0] = (uint8_t)((crc >> 8) & 0xff);
	bytes[1] = (uint8_t)(crc & 0xff);
}
/* end UcbPacketCrcToBytes */

/*********************************************************************************
* Function name:	UcbPacketCalculateCrc
*
* Description:	This function returns the CCITT-16 CRC on the passed in data of 
*               length given with an initial CRC value.
*
* Trace: [SDD_UCB_CRC_FAIL_01  <-- SRC_CRC_UCB_PKT]
*        [SDD_UCB_VALID_PACKET <-- SRC_CRC_UCB_PKT]
*
* Input parameters:	
*      data[]     byte array to CRC
*      length     number of bytes in byte array
*      seed       initial or continuing value of CRC
*
* Output parameters: none	
*
* Return value:
*     16 bit CRC
*********************************************************************************/
UcbPacketCrcType UcbPacketCalculateCrc (const uint8_t data [], uint16_t length, const UcbPacketCrcType seed)
{    
	return (CrcCcitt(data, length, seed));		
}  
/* end UcbPacketCalculateCrc */  

/*********************************************************************************
* Function name:	UcbPacketIsAnInputPacket
*
* Description:	Returns TRUE if given packet type is an input packet type
*
* Trace: 
* [SDD_UCB_UNKNOWN_01 <-- SRC_UCB_PKT_INTYPE]
* [SDD_UCB_UNKNOWN_02 <-- SRC_UCB_PKT_INTYPE]
* [SDD_UCB_VALID_PACKET <-- SRC_UCB_PKT_INTYPE] 

*
* Input parameters:	UCB packet type enum
*
* Output parameters: none
*
* Return value: TRUE if output packet type, FALSE otherwise 
*********************************************************************************/
BOOL UcbPacketIsAnInputPacket (UcbPacketTypeEnum type)
{    
	BOOL isAnInputPacket;

	switch (type) {
#		define INPUT_PACKET_TYPES
#		define PACKET_TYPE(constName, varName, code)	case constName:	isAnInputPacket = TRUE;	break;
#		include "ucb_packet_types.def"
#		undef INPUT_PACKET_TYPES
#		undef PACKET_TYPE	 
	 
		default:	isAnInputPacket = FALSE; 
	} 
	
	return isAnInputPacket;
}
/* end UcbPacketIsAnInputPacket */

/*********************************************************************************
* Function name:	UcbPacketIsAnOutputPacket
*
* Description:	Returns TRUE if given packet type is an output packet type
*
* Trace: [SDD_PORT_CFG_VALID_03 <-- SRC_UCB_PKT_OUTTYPE]
*
* Input parameters:	UCB packet type
*
* Output parameters: none	
*
* Return value: TRUE if input packet type, FALSE otherwise 
*********************************************************************************/
BOOL UcbPacketIsAnOutputPacket (UcbPacketTypeEnum type)
{
	BOOL isAnOutputPacket;

	switch (type) {
#		define OUTPUT_PACKET_TYPES
#		define PACKET_TYPE(constName, varName, code)	case constName:	isAnOutputPacket = TRUE;	break;
#		include "ucb_packet_types.def"
#		undef OUTPUT_PACKET_TYPES	 
#		undef PACKET_TYPE
	 
		default:	isAnOutputPacket = FALSE; 
	} 
	
	return isAnOutputPacket;
}
/* end UcbPacketIsAnOutputPacket */
                       
