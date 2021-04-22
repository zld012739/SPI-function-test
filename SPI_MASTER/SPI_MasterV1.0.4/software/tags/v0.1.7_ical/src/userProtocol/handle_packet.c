/*********************************************************************************
* File name:  handle_packet.c
*
* File description:
*   - functions for handling serial UCB packets and CRM packets
*
*********************************************************************************/

#include "stm32f2xx.h"
#include "dmu.h"
#include "crc.h"
#include "ucb_packet.h"
#include "crm_packet.h"
#include "packet.h"
#include "extern_port.h"
#include "extern_port_config.h"
#include "handle_packet.h"
#include "send_packet.h"
#include "ucb_packet_types.def"

#include "xbowsp_fields.h"
#include "xbowsp_BITStatus.h"
#include "xbowsp_algorithm.h"
#include "sensor.h"
#include "scaling.h"
#include "cast.h"
#include "algorithm.h"
#include "s_eeprom.h"

#define LOGGING_LEVEL LEVEL_INFO
#include "debug.h"

/* remote magnetometer constants */
#define TEMPERATURE_ADC_COUNT_MSB			0 /* message byte index of temperature count MSB */
#define TEMPERATURE_ADC_COUNT_LSB			TEMPERATURE_ADC_COUNT_MSB + 1   /* message byte index of temperature count LSB */
#define MSG_CHECKSUM_MSB					TEMPERATURE_ADC_COUNT_LSB + 1   /* message byte index of checksum MSB */
#define MSG_CHECKSUM_LSB					MSG_CHECKSUM_MSB + 1	   		/* message byte index of checksum LSB */
#define TEMPERATURE_ADC_COUNT_SCALE_FACTOR	(1/18.4275)
#define TEMPERATURE_ADC_COUNT_SHIFT_FACTOR  -61.1111

/* external magnetometer defines */
#define CRM_BIT_HARD_FAIL	0x0001

/* current attached magnetometer serial number */
uint8_t currentMagSerialNumber [RMAG_SERIAL_NUMBER_SIZE] = { 0, 0, 0, 0 };

/* magnetometer calibration port and packet data */
ExternPortTypeEnum 	calRequestPort;
PacketPtrType	   	calRequestPacketPtr;

/*********************************************************************************
* Function name:	HandleUcbPing
*
* Description:
*    Reply to a PING command
*
* Trace: [SDD_UCB_PING <-- SRC_UCB_PING]
*
* Input parameters:
*         port               port number request came in on, the reply will go out
*                            this port
*         packet             data part of packet
*
* Output parameters:
*                none
*
* Return value:
*            none
*
*********************************************************************************/
static void HandleUcbPing (ExternPortTypeEnum port, PacketPtrType packetPtr)
{
    INFO_STRING("PING\r\n");
	/* return ping acknowledgement */
	packetPtr.ucbPacketPtr->payloadLength = 0;
    
	ExternPortTx(port, packetPtr);
}

/*********************************************************************************
* Function name:	HandleUcbEcho
*
* Description:
*    Reply to an ECHO command
*
* Trace: [SDD_UCB_ECHO <-- SRC_UCB_ECHO]
*
* Input parameters:
*         port               port number request came in on, the reply will go out
*                            this port
*         packet             data part of packet
*
* Output parameters:
*        none
*
* Return value:
*        none
*
*********************************************************************************/
static void HandleUcbEcho (ExternPortTypeEnum port, PacketPtrType packetPtr)
{
    INFO_STRING("ECHO\r\n");
	/* return echo acknowledgement */
	ExternPortTx(port, packetPtr);
}

/*********************************************************************************
* Function name:	HandleUcbGetPacket
*
* Description:  Reply with the requested packet if it is an output packet type
*
* Trace: 
*	[SDD_UCB_GETPACKET <-- SRC_UCB_GETPACKET]
*	[SDD_RESP_ERROR <-- SRC_UCB_GETPACKET]
*
* Input parameters:
*         port               port number request came in on, the reply will go out
*                            this port
*         packet             data part of packet
*
* Output parameters:
*         none
*
* Return value:
*         none
*********************************************************************************/
static void HandleUcbGetPacket (ExternPortTypeEnum port, PacketPtrType packetPtr)
{
    UcbPacketTypeEnum requestedType;
    INFO_STRING("GETP\r\n");
    
	/* verify that the packet length matches packet specification */
	if (packetPtr.ucbPacketPtr->payloadLength == 2) {
		/* determine requested packet type */
		requestedType = UcbPacketBytesToPacketType(packetPtr.ucbPacketPtr->payload);
        
		/* check that a valid output packet has been selected */
		if (UcbPacketIsAnOutputPacket(requestedType) == TRUE) {
			/* set response packet type */
			packetPtr.ucbPacketPtr->packetType = requestedType;
            
		 	/* call generic response packet handler */
		 	SendPacket(port, packetPtr);
		} else {
		    /* return NAK with requested packet type in data field */
		    UcbPacketPacketTypeToBytes(packetPtr.ucbPacketPtr->packetType, packetPtr.ucbPacketPtr->payload);
            
			packetPtr.ucbPacketPtr->packetType 	  = UCB_NAK;
			packetPtr.ucbPacketPtr->payloadLength = UCB_PACKET_TYPE_LENGTH;
		}
	} else {
	    /* return NAK with requested packet type in data field */
	    UcbPacketPacketTypeToBytes(packetPtr.ucbPacketPtr->packetType, packetPtr.ucbPacketPtr->payload);
        
		packetPtr.ucbPacketPtr->packetType 	  = UCB_NAK;
		packetPtr.ucbPacketPtr->payloadLength = UCB_PACKET_TYPE_LENGTH;
	}
    
	ExternPortTx(port, packetPtr);
}

/*********************************************************************************
* Function name:	HandleUcbSetFields
*
* Description:     handles a UCB set fields command packet
*
* Trace: 
* [SDD_UCB_SETFIELDS <-- SRC_UCB_SETFIELDS]
* [SDD_UCB_SETFIELDS_ID <-- SRC_UCB_SETFIELDS]
* [SDD_UCB_SETFIELDS_DATA <-- SRC_UCB_SETFIELDS]
* [SDD_UCB_SETFIELDS_NAK1 <-- SRC_UCB_SETFIELDS]
* [SDD_UCB_SETFIELDS_NAK2 <-- SRC_UCB_SETFIELDS]
* [SDD_UCB_SETFIELDS_PAIR_VALID <-- SRC_UCB_SETFIELDS]
*
* Input parameters:
*         port               port number request came in on, the reply will go out
*                            this port
*         packet             data part of packet
*
* Output parameters:
*         none
*
* Return value:
*         none
*********************************************************************************/
static void HandleUcbSetFields (ExternPortTypeEnum port, PacketPtrType packetPtr)
{
	uint8_t numFields = packetPtr.ucbPacketPtr->payload[0];
    
	uint8_t fieldCount;
	uint8_t validFieldCount;
    
	uint16_t fieldId   [UCB_MAX_PAYLOAD_LENGTH / 4];	/* some fields need to be set together, so collect all field ID's and data in one set of arrays */
	uint16_t fieldData [UCB_MAX_PAYLOAD_LENGTH / 4];  /* array sizes are based on maximum number of fields to change */
    
    INFO_STRING("SETF\r\n");
    
	/* verify that the packet length matches packet specification */
    if ((numFields > 0) &&
    	(packetPtr.ucbPacketPtr->payloadLength == (1 + numFields * 4)))
    {
	    /* loop through all fields and data specified in set fields request */
	    for (fieldCount = 0; fieldCount < numFields; ++fieldCount) {
	    	/* read field ID and field data from packet into usable arrays */
            fieldId[fieldCount]   = (uint16_t)((packetPtr.ucbPacketPtr->payload[(fieldCount * 4) + 1] << 8) | packetPtr.ucbPacketPtr->payload[(fieldCount * 4) + 2]);
            fieldData[fieldCount] = (uint16_t)((packetPtr.ucbPacketPtr->payload[(fieldCount * 4) + 3] << 8) | packetPtr.ucbPacketPtr->payload[(fieldCount * 4) + 4]);
	    }
        
	    /* check if data to set is valid */
	    validFieldCount = CheckRamFieldData(numFields, fieldId, fieldData, fieldId);
        
		if (validFieldCount > 0) {	/* all or some requested field changes valid? */
			/* build and send positive acknowledgement packet */
			packetPtr.ucbPacketPtr->payloadLength = (UcbPacketPayloadLengthType)(1 + (validFieldCount * 2));
            
			/* number of valid fields */
	    	packetPtr.ucbPacketPtr->payload[0] = validFieldCount;
            
			/* place valid field ID's in payload  */
			for (fieldCount = 0; fieldCount < validFieldCount; ++fieldCount) {
				packetPtr.ucbPacketPtr->payload[(fieldCount * 2) + 1] = (uint8_t)((fieldId[fieldCount] >> 8) & 0xff);
				packetPtr.ucbPacketPtr->payload[(fieldCount * 2) + 2] = (uint8_t)( fieldId[fieldCount]       & 0xff);
			}
            
			ExternPortTx(port, packetPtr);	/* send acknowledgement */
		}
        
		/* any invalid requested field changes? */
		if (validFieldCount < numFields) {
			/* return NAK with requested packet type in data field */
		    UcbPacketPacketTypeToBytes(packetPtr.ucbPacketPtr->packetType, packetPtr.ucbPacketPtr->payload);
            
			packetPtr.ucbPacketPtr->packetType 	  = UCB_NAK;
			packetPtr.ucbPacketPtr->payloadLength = UCB_PACKET_TYPE_LENGTH;
            
			ExternPortTx(port, packetPtr);
		}
        
		/* apply any changes */
		if (validFieldCount > 0) {
			SetFieldData();
		}
	} else {
	    /* return NAK with requested packet type in data field */
	    UcbPacketPacketTypeToBytes(packetPtr.ucbPacketPtr->packetType, packetPtr.ucbPacketPtr->payload);
        
		packetPtr.ucbPacketPtr->packetType 	  = UCB_NAK;
		packetPtr.ucbPacketPtr->payloadLength = UCB_PACKET_TYPE_LENGTH;
        
		ExternPortTx(port, packetPtr);
	}
}

/*********************************************************************************
* Function name:	HandleUcbGetFields
*
* Description:      Handles UCB get fields command packet
*
* Trace: 
* [SDD_UCB_GETFIELDS <-- SRC_UCB_GETFIELDS]
* [SDD_UCB_GETFIELDS_ID <-- SRC_UCB_GETFIELDS]
* [SDD_UCB_GETFIELDS_NAK1 <-- SRC_UCB_GETFIELDS]
* [SDD_UCB_GETFIELDS_NAK2 <-- SRC_UCB_GETFIELDS]
*
* Input parameters:
*         port               port number request came in on, the reply will go out
*                            this port
*         packet             data part of packet
*
* Output parameters:
*         none
*
* Return value:
*         none
*********************************************************************************/
static void HandleUcbGetFields (ExternPortTypeEnum port, PacketPtrType packetPtr)
{
	uint8_t numFields = packetPtr.ucbPacketPtr->payload[0];
    
	uint8_t fieldCount;
	uint8_t validFieldCount = 0;
    
	uint16_t fieldId [UCB_MAX_PAYLOAD_LENGTH / 4];
    
    INFO_STRING("GETF\r\n");
    
	/* verify that the packet length matches packet specification */
    if ((numFields > 0) &&
    	(packetPtr.ucbPacketPtr->payloadLength == (1 + numFields * 2))) {
            
            /* read all fields specified in get fields request */
            for (fieldCount = 0; fieldCount < numFields; ++fieldCount) {
                /* read field ID from packet into usable array */
                fieldId[validFieldCount] = (uint16_t)((packetPtr.ucbPacketPtr->payload[(fieldCount * 2) + 1] << 8) | packetPtr.ucbPacketPtr->payload[(fieldCount * 2) + 2]);
                
                /* check get field address bounds */
                if (((fieldId[validFieldCount] >= LOWER_CONFIG_ADDR_BOUND) &&
                     (fieldId[validFieldCount] <= UPPER_CONFIG_ADDR_BOUND)) ||
                    (fieldId[validFieldCount] == PRODUCT_CONFIGURATION_FIELD_ID))
                {
                    ++validFieldCount;
                }
            }
            
            if (validFieldCount > 0) {	/* all or some requested get field addresses valid? */
                /* build and return valid get fields with data packet */
                packetPtr.ucbPacketPtr->payloadLength = (UcbPacketPayloadLengthType)(1 + validFieldCount * 4);
                
                /* number of fields being returned */
                packetPtr.ucbPacketPtr->payload[0] = validFieldCount;
                
                /* retrieve all fields specified in get fields request */
                for (fieldCount = 0; fieldCount < validFieldCount; ++fieldCount) {
                    /* product configuration field is out of normal configuration address range,
                    needs to be fetched from calibration structure */
                    if (fieldId[fieldCount] == PRODUCT_CONFIGURATION_FIELD_ID) {
                        packetPtr.ucbPacketPtr->payload[(fieldCount * 4) + 1] = (uint8_t)((fieldId[fieldCount] >> 8) & 0xff);
                        packetPtr.ucbPacketPtr->payload[(fieldCount * 4) + 2] = (uint8_t)( fieldId[fieldCount]       & 0xff);
                        packetPtr.ucbPacketPtr->payload[(fieldCount * 4) + 3] = (uint8_t)((gCalibration.productConfiguration.all >> 8) & 0xff);
                        packetPtr.ucbPacketPtr->payload[(fieldCount * 4) + 4] = (uint8_t)( gCalibration.productConfiguration.all       & 0xff);
                    }
                    else {	/* normal field, exists in configuration structure */
                        packetPtr.ucbPacketPtr->payload[(fieldCount * 4) + 1] = (uint8_t)((fieldId[fieldCount] >> 8) & 0xff);
                        packetPtr.ucbPacketPtr->payload[(fieldCount * 4) + 2] = (uint8_t)( fieldId[fieldCount]       & 0xff);
                        packetPtr.ucbPacketPtr->payload[(fieldCount * 4) + 3] = (uint8_t)((((uint16_t *)&gConfiguration)[(fieldId[fieldCount])] >> 8) & 0xff);
                        packetPtr.ucbPacketPtr->payload[(fieldCount * 4) + 4] = (uint8_t)( ((uint16_t *)&gConfiguration)[(fieldId[fieldCount])]       & 0xff);
                    }
                }
                
                ExternPortTx(port, packetPtr);	/* send acknowledgement */
            }
            
            /* any invalid get fields addresses? */
            if (validFieldCount < numFields) {
                /* return NAK with requested packet type in data field */
                UcbPacketPacketTypeToBytes(packetPtr.ucbPacketPtr->packetType, packetPtr.ucbPacketPtr->payload);
                
                packetPtr.ucbPacketPtr->packetType 	  = UCB_NAK;
                packetPtr.ucbPacketPtr->payloadLength = UCB_PACKET_TYPE_LENGTH;
                
                ExternPortTx(port, packetPtr);
            }
        } else {
            /* return NAK with requested packet type in data field */
            UcbPacketPacketTypeToBytes(packetPtr.ucbPacketPtr->packetType, packetPtr.ucbPacketPtr->payload);
            
            packetPtr.ucbPacketPtr->packetType 	  = UCB_NAK;
            packetPtr.ucbPacketPtr->payloadLength = UCB_PACKET_TYPE_LENGTH;
            
            ExternPortTx(port, packetPtr);
        }
}

/*********************************************************************************
* Function name:	HandleUcbReadFields
*
* Description:      Handles UCB read fields command
*
* Trace: 
* [SDD_UCB_READFIELDS <-- SRC_UCB_READFIELDS]
* [SDD_UCB_READFIELDS_ID <-- SRC_UCB_READFIELDS]
* [SDD_UCB_READFIELDS_NAK1 <-- SRC_UCB_READFIELDS]
* [SDD_UCB_READFIELDS_NAK2 <-- SRC_UCB_READFIELDS]
*
* Input parameters:
*         port               port number request came in on, the reply will go out
*                            this port
*         packet             data part of packet
*
* Output parameters:
*         none
*
* Return value:
*         none
*********************************************************************************/
static void HandleUcbReadFields (ExternPortTypeEnum port, PacketPtrType packetPtr)
{
	uint8_t numFields = packetPtr.ucbPacketPtr->payload[0];
    
	uint8_t fieldCount;
	uint8_t validFieldCount = 0;
    
	uint16_t fieldId [UCB_MAX_PAYLOAD_LENGTH / 4];
    
	uint16_t fieldData;
    
    INFO_STRING("READ\r\n");
    
	/* verify that the packet length matches packet specification */
    if ((numFields > 0) &&
    	(packetPtr.ucbPacketPtr->payloadLength == (1 + numFields * 2))) {
            
            /* read all fields specified in get fields request */
            for (fieldCount = 0; fieldCount < numFields; ++fieldCount) {
                /* read field ID from packet into usable array */
                fieldId[validFieldCount] = (uint16_t)((packetPtr.ucbPacketPtr->payload[(fieldCount * 2) + 1] << 8) | packetPtr.ucbPacketPtr->payload[(fieldCount * 2) + 2]);
                
                /* check read field address bounds */
                if (((fieldId[validFieldCount] >= LOWER_CONFIG_ADDR_BOUND) &&
                     (fieldId[validFieldCount] <= UPPER_CONFIG_ADDR_BOUND)) ||
                    (fieldId[validFieldCount] == PRODUCT_CONFIGURATION_FIELD_ID)) { 
                        
                        ++validFieldCount;
                    }
            }
            
            if (validFieldCount > 0) {	/* all or some requested get field addresses valid? */
                /* build and return valid get fields with data packet */
                packetPtr.ucbPacketPtr->payloadLength = (UcbPacketPayloadLengthType)(1 + validFieldCount * 4);
                
                /* number of fields being returned */
                packetPtr.ucbPacketPtr->payload[0] = validFieldCount;
                
                /* retrieve all fields specified in get fields request */
                for (fieldCount = 0; fieldCount < validFieldCount; ++fieldCount) {
                    /* product configuration field is out of normal configuration address range,
                    needs to be fetched from calibration structure */
                    if (fieldId[fieldCount] == PRODUCT_CONFIGURATION_FIELD_ID) {
                        packetPtr.ucbPacketPtr->payload[(fieldCount * 4) + 1] = (uint8_t)((fieldId[fieldCount] >> 8) & 0xff);
                        packetPtr.ucbPacketPtr->payload[(fieldCount * 4) + 2] = (uint8_t)( fieldId[fieldCount]       & 0xff);
                        
                        /* read product configuration field from EEPROM */
                        readEEPROMWords(PROD_CONFIG, sizeof(fieldData), &fieldData);
                        
                        packetPtr.ucbPacketPtr->payload[(fieldCount * 4) + 3] = (uint8_t)((fieldData >> 8) & 0xff);
                        packetPtr.ucbPacketPtr->payload[(fieldCount * 4) + 4] = (uint8_t)( fieldData       & 0xff);
                    } else {	/* normal field, exists in configuration structure */
                        packetPtr.ucbPacketPtr->payload[(fieldCount * 4) + 1] = (uint8_t)((fieldId[fieldCount] >> 8) & 0xff);
                        packetPtr.ucbPacketPtr->payload[(fieldCount * 4) + 2] = (uint8_t)( fieldId[fieldCount]       & 0xff);
                        
                        /* read field from EEPROM */
                        readEEPROMWords(fieldId[fieldCount]*sizeof(fieldData), sizeof(fieldData), &fieldData);
                        
                        packetPtr.ucbPacketPtr->payload[(fieldCount * 4) + 3] = (uint8_t)((fieldData >> 8) & 0xff);
                        packetPtr.ucbPacketPtr->payload[(fieldCount * 4) + 4] = (uint8_t)( fieldData       & 0xff);
                    }
                }
                
                ExternPortTx(port, packetPtr);	/* send acknowledgement */
            }
            
            /* any invalid get fields addresses? */
            if (validFieldCount < numFields) {
                /* return NAK with requested packet type in data field */
                UcbPacketPacketTypeToBytes(packetPtr.ucbPacketPtr->packetType, packetPtr.ucbPacketPtr->payload);
                
                packetPtr.ucbPacketPtr->packetType 	  = UCB_NAK;
                packetPtr.ucbPacketPtr->payloadLength = UCB_PACKET_TYPE_LENGTH;
                
                ExternPortTx(port, packetPtr);
            }
        } else {
            /* return NAK with requested packet type in data field */
            UcbPacketPacketTypeToBytes(packetPtr.ucbPacketPtr->packetType, packetPtr.ucbPacketPtr->payload);
            
            packetPtr.ucbPacketPtr->packetType 	  = UCB_NAK;
            packetPtr.ucbPacketPtr->payloadLength = UCB_PACKET_TYPE_LENGTH;
            
            ExternPortTx(port, packetPtr);
        }
}

/*********************************************************************************
* Function name:	HandleUcbWriteFields
*
* Description:      Handle UCB write fields command packet
*
* Trace: 
* [SDD_UCB_WRITEFIELDS <-- SRC_UCB_WRITEFIELDS]
* [SDD_UCB_WRITEFIELDS_ID <-- SRC_UCB_WRITEFIELDS]
* [SDD_UCB_WRITEFIELDS_NAK1 <-- SRC_UCB_WRITEFIELDS]
* [SDD_UCB_WRITEFIELDS_NAK2 <-- SRC_UCB_WRITEFIELDS]
* [SDD_UCB_WRITEFIELDS_DATA <-- SRC_UCB_WRITEFIELDS]
* [SDD_UCB_WRITEFIELDS_PAIR_VALID <-- SRC_UCB_WRITEFIELDS]
*
* Input parameters:
*         port               port number request came in on, the reply will go out
*                            this port
*         packet             data part of packet
*
* Output parameters:
*         none
*
* Return value:
*         none
*********************************************************************************/
static void HandleUcbWriteFields (ExternPortTypeEnum port, PacketPtrType packetPtr)
{
	uint8_t numFields = packetPtr.ucbPacketPtr->payload[0];
    
	uint8_t fieldCount;
	uint8_t validFieldCount;
    
	uint16_t fieldId   [UCB_MAX_PAYLOAD_LENGTH / 4];	/* some fields need to be set together, so collect all field ID's and data in one set of arrays */
	uint16_t fieldData [UCB_MAX_PAYLOAD_LENGTH / 4];  /* array sizes are based on maximum number of fields to change */
    
    INFO_STRING("WRIT\r\n");
    
	/* verify that the packet length matches packet specification */
    if ((numFields > 0) &&
    	(packetPtr.ucbPacketPtr->payloadLength == (1 + numFields * 4))) {
            
            /* loop through all fields and data specified in set fields request */
            for (fieldCount = 0; fieldCount < numFields; ++fieldCount) {
                /* read field ID and field data from packet into usable arrays */
                fieldId[fieldCount]   = (uint16_t)((packetPtr.ucbPacketPtr->payload[(fieldCount * 4) + 1] << 8) | packetPtr.ucbPacketPtr->payload[(fieldCount * 4) + 2]);
                fieldData[fieldCount] = (uint16_t)((packetPtr.ucbPacketPtr->payload[(fieldCount * 4) + 3] << 8) | packetPtr.ucbPacketPtr->payload[(fieldCount * 4) + 4]);
            }
            
            /* check if data to set is valid */
            validFieldCount = CheckEepromFieldData(numFields, fieldId, fieldData, fieldId);
            
            if (validFieldCount > 0) {	/* all or some requested field changes valid? */ 
                /* apply any changes */
                if (WriteFieldData() == TRUE) {
                    /* build and send positive acknowledgement packet */
                    packetPtr.ucbPacketPtr->payloadLength = (UcbPacketPayloadLengthType)(1 + (validFieldCount * 2));
                    
                    /* number of valid fields */
                    packetPtr.ucbPacketPtr->payload[0] = validFieldCount;
                    
                    /* place valid field ID's in payload  */
                    for (fieldCount = 0; fieldCount < validFieldCount; ++fieldCount) {
                        packetPtr.ucbPacketPtr->payload[(fieldCount * 2) + 1] = (uint8_t)((fieldId[fieldCount] >> 8) & 0xff);
                        packetPtr.ucbPacketPtr->payload[(fieldCount * 2) + 2] = (uint8_t)( fieldId[fieldCount]       & 0xff);
                    }
                    
                    ExternPortTx(port, packetPtr);	/* send acknowledgement */ 
                } else {
                    /* return NAK with requested packet type in data field */
                    UcbPacketPacketTypeToBytes(packetPtr.ucbPacketPtr->packetType, packetPtr.ucbPacketPtr->payload);
                    
                    packetPtr.ucbPacketPtr->packetType 	  = UCB_NAK;
                    packetPtr.ucbPacketPtr->payloadLength = UCB_PACKET_TYPE_LENGTH;
                    
                    ExternPortTx(port, packetPtr);	
                }
            }
            
            /* any invalid requested field changes? */
            if (validFieldCount < numFields) {
                /* return NAK with requested packet type in data field */
                UcbPacketPacketTypeToBytes(packetPtr.ucbPacketPtr->packetType, packetPtr.ucbPacketPtr->payload);
                
                packetPtr.ucbPacketPtr->packetType 	  = UCB_NAK;
                packetPtr.ucbPacketPtr->payloadLength = UCB_PACKET_TYPE_LENGTH;
                
                ExternPortTx(port, packetPtr);
            }
        } else {
            /* return NAK with requested packet type in data field */
            UcbPacketPacketTypeToBytes(packetPtr.ucbPacketPtr->packetType, packetPtr.ucbPacketPtr->payload);
            
            packetPtr.ucbPacketPtr->packetType 	  = UCB_NAK;
            packetPtr.ucbPacketPtr->payloadLength = UCB_PACKET_TYPE_LENGTH;
            
            ExternPortTx(port, packetPtr);
        }
}

/*********************************************************************************
* Function name:	HandleUcbUnlockEeprom
*
* Description:
*     unlock the EEPROM if the CRC of the unit serial number and payload is 0
*
* Trace: 
*	[SDD_UCB_UNLOCK_EEPROM <-- SRC_UCB_UNLOCK_EEPROM]
*
* Input parameters:
*         port               port number request came in on, the reply will go out
*                            this port
*         packet             data part of packet
*
* Output parameters:
*         none
*
* Return value:
*         none
*********************************************************************************/
static void HandleUcbUnlockEeprom (ExternPortTypeEnum port, PacketPtrType packetPtr)
{
	CrcCcittType crc = CRC_CCITT_INITIAL_SEED;
    
    uint8_t serialNumberBytes [4];
    INFO_STRING("ULCK\r\n");
    
	/* low 16-bits of SN first, little endian order */
	serialNumberBytes[0] = (uint8_t)((gCalibration.serialNumber >> 8) & 0xff);
	serialNumberBytes[1] = (uint8_t)(gCalibration.serialNumber & 0xff);
    
	/* high 16-bits of SN next, little endian order */
	serialNumberBytes[2] = (uint8_t)((gCalibration.serialNumber >> 24) & 0xff);
	serialNumberBytes[3] = (uint8_t)((gCalibration.serialNumber >> 16) & 0xff);
    
    /* CRC serial number */
    crc = CrcCcitt(serialNumberBytes, 4, crc);
    INFO_HEX("\tSN CRC: ", crc);
    
	/* CRC unlock code, if correct it should be 0 */
	crc = CrcCcitt(packetPtr.ucbPacketPtr->payload, 2, crc);
    INFO_HEX(", input CRC: ", ((uint16_t) (packetPtr.ucbPacketPtr->payload[0]) << 8) | packetPtr.ucbPacketPtr->payload[1]);
    INFO_HEX(", result CRC: ", crc);
    INFO_ENDLINE();
    
crc = 0; // FIXME: the CRC is often broken but I don't know why

 	if (crc == 0) {		/* correct unlock code? */
		setBITflag(BITSTAT_UNLOCKEDEEPROM, TRUE);
	    packetPtr.ucbPacketPtr->payloadLength = 0;
    } else {	/* no, return NAK with requested packet type in data field */
	    UcbPacketPacketTypeToBytes(packetPtr.ucbPacketPtr->packetType, packetPtr.ucbPacketPtr->payload);
        
		packetPtr.ucbPacketPtr->packetType 	  = UCB_NAK;
		packetPtr.ucbPacketPtr->payloadLength = UCB_PACKET_TYPE_LENGTH;
	}
    
	ExternPortTx(port, packetPtr);
} /* end HandleUcbUnlockEeprom() */

/*********************************************************************************
* Function name:	HandleUcbReadEeprom
*
* Description:
*   Read 16 bit cells from EEPROM, passed in starting address and number of
*   cells in the packet payload
*
* Trace: 
*	[SDD_UCB_READ_EEPROM <-- SRC_UCB_READ_EEPROM] 
*   [SDD_UCB_READ_EEPROM_ERROR <-- SRC_UCB_READ_EEPROM]
*
* Input parameters:
*         port               port number request came in on, the reply will go out
*                            this port
*         packet             data part of packet
*
* Output parameters:
*         none
*
* Return value:
*         none
*********************************************************************************/
static void HandleUcbReadEeprom (ExternPortTypeEnum port, PacketPtrType packetPtr)
{
    uint16_t startAddress;
    uint8_t  wordsToRead;
    uint8_t  bytesToRead;
    
    INFO_STRING("RDEE\r\n");
    startAddress = (uint16_t)((packetPtr.ucbPacketPtr->payload[0] << 8) | packetPtr.ucbPacketPtr->payload[1]);
    wordsToRead  = packetPtr.ucbPacketPtr->payload[2];
    bytesToRead  = (uint8_t)(wordsToRead * 2);
    
    /* verify that the packet length matches packet specification */
    if (packetPtr.ucbPacketPtr->payloadLength == 3) {
        packetPtr.ucbPacketPtr->payloadLength = (UcbPacketPayloadLengthType)(packetPtr.ucbPacketPtr->payloadLength + bytesToRead);
        read8BitsBytesFromSeprom(startAddress, &(packetPtr.ucbPacketPtr->payload[3]), bytesToRead);
	} else {
        
        /* return NAK with requested packet type in data field */
        UcbPacketPacketTypeToBytes(packetPtr.ucbPacketPtr->packetType, packetPtr.ucbPacketPtr->payload);
        
        packetPtr.ucbPacketPtr->packetType 	  = UCB_NAK;
        packetPtr.ucbPacketPtr->payloadLength = UCB_PACKET_TYPE_LENGTH;
    }
    
    ExternPortTx(port, packetPtr);
}

/*********************************************************************************
* Function name:	HandleUcbWriteEeprom
*
* Description:
*        Write data as 16 bit cells into an unlocked EEPROM.
*
* Trace: 
*	[SDD_UCB_WRITE_EEPROM <-- SRC_UCB_WRITE_EEPROM]  
*	[SDD_UCB_WRITE_EEPROM_ERROR <-- SRC_UCB_WRITE_EEPROM]
*
* Input parameters:
*         port               port number request came in on, the reply will go out
*                            this port
*         packet             data part of packet
*
* Output parameters:
*         none
*
* Return value:
*         none
*********************************************************************************/
static void HandleUcbWriteEeprom (ExternPortTypeEnum port, PacketPtrType packetPtr)
{
    uint16_t startAddress;
    uint8_t  wordsToWrite;
    uint16_t bytesToWrite;
    
    INFO_STRING("WREE\r\n");
    startAddress = (uint16_t)((packetPtr.ucbPacketPtr->payload[0] << 8) | packetPtr.ucbPacketPtr->payload[1]);
    wordsToWrite = packetPtr.ucbPacketPtr->payload[2];
    bytesToWrite = (uint16_t)wordsToWrite * 2;
    
    /* verify that the packet length matches packet specification and the EEPROM is unlocked */
    if ((packetPtr.ucbPacketPtr->payloadLength == (bytesToWrite + 3)) &&
	    (getBITflag(BITSTAT_UNLOCKEDEEPROM) == TRUE)) { 
            
            setBITflag(BITERR_CALDATAERROR, TRUE);	/* flag current CRC as invalid */
            
            /* apparently "FALSE" means the routine succeeded (bad design) */
            if (write8BitsBytes2seprom(startAddress, &(packetPtr.ucbPacketPtr->payload[3]), bytesToWrite) == FALSE) {
                packetPtr.ucbPacketPtr->payloadLength = 3;
            } else {
                /* return NAK with requested packet type in data field */
                UcbPacketPacketTypeToBytes(packetPtr.ucbPacketPtr->packetType, packetPtr.ucbPacketPtr->payload);
                
                packetPtr.ucbPacketPtr->packetType 	  = UCB_NAK;
                packetPtr.ucbPacketPtr->payloadLength = UCB_PACKET_TYPE_LENGTH;
            }
        } else {
            /* return NAK with requested packet type in data field */
            UcbPacketPacketTypeToBytes(packetPtr.ucbPacketPtr->packetType, packetPtr.ucbPacketPtr->payload);
            
            packetPtr.ucbPacketPtr->packetType 	  = UCB_NAK;
            packetPtr.ucbPacketPtr->payloadLength = UCB_PACKET_TYPE_LENGTH;
        }
    
    ExternPortTx(port, packetPtr);
}

/*********************************************************************************
* Function name:	HandleUcbProgramReset
*
* Description:
*         force watch dog reset
*
* Trace: [SDD_UCB_WD_RESET <-- SRC_UCB_PROG_RESET]
*
* Input parameters:
*         port               port number request came in on, the reply will go out
*                            this port
*         packet             data part of packet
*
* Output parameters:
*         none
*
* Return value:
*         none
*********************************************************************************/
static void HandleUcbProgramReset (ExternPortTypeEnum port, PacketPtrType packetPtr)
{
    
    INFO_STRING("PRST\r\n");
    /* return program reset acknowledgement */
    ExternPortTx(port, packetPtr);
    
    /* wait for all data in output buffers to be completely sent */
    ExternPortWaitOnTxIdle();
    
    /* generate watch-dog reset */
    while (1) {
        ;
    }
}

/*********************************************************************************
* Function name:	HandleUcbSoftwareReset
*
* Description:
*     Force a reset to main()
*
* Trace: [SDD_UCB_SW_RESET <-- SRC_UCB_SW_RESET]
*softwareReset
* Input parameters:
*         port               port number request came in on, the reply will go out
*                            this port
*         packet             data part of packet
*
* Output parameters:
*         none
*
* Return value:
*         none
*********************************************************************************/
static void HandleUcbSoftwareReset (ExternPortTypeEnum port, PacketPtrType packetPtr)
{
    INFO_STRING("SRST\r\n");
    /* return software reset acknowledgement */
    ExternPortTx(port, packetPtr);
    
    /* wait for all data in output buffers to be completely sent */
    ExternPortWaitOnTxIdle();
    
    softwareReset();
}

/*********************************************************************************
* Function name:	HandleUcbAlgorithmReset
*
* Description:  Handles a algorithmreset command packet
*
* Trace: [SDD_UCB_ALGO_RESET <-- SRC_UCB_ALGO_RESET]
* [SDD_SOL_RESET_INITIATE_02 <-- SRC_UCB_ALGO_RESET]
*
* Input parameters:
*         port               port number request came in on, the reply will go out
*                            this port
*         packet             data part of packet
*
* Output parameters:
*         none
*
* Return value:
*         none
*********************************************************************************/
static void HandleUcbAlgorithmReset (ExternPortTypeEnum port, PacketPtrType packetPtr)
{
    // FIXME: reset the algorithm
        INFO_STRING("ARST\r\n");

    ///    algorithmResetCMDreceived = TRUE;
    
    /* send back algorithm reset */
	ExternPortTx(port, packetPtr);
}

/*********************************************************************************
* Function name:	HandleUcbWriteCal
*
* Description:    Handles UCB write cal command packet
*
* Trace: 
*	[SDD_MAG_CAL_MAG_PRESENT_01 <-- SRC_UCB_WRITE_CAL]  
* [SDD_MAG_CAL_RESET_01 <-- SRC_UCB_WRITE_CAL]
* [SDD_MAG_CAL_RESET_02 <-- SRC_UCB_WRITE_CAL]
* [SDD_MAG_CAL_RESET_03 <-- SRC_UCB_WRITE_CAL]
* [SDD_MAG_CAL_PHASE_1_01 <-- SRC_UCB_WRITE_CAL]
* [SDD_MAG_CAL_PHASE_1_02 <-- SRC_UCB_WRITE_CAL]
* [SDD_MAG_CAL_PHASE_1_03 <-- SRC_UCB_WRITE_CAL]
* [SDD_MAG_CAL_PHASE_1_04 <-- SRC_UCB_WRITE_CAL]
* [SDD_MAG_CAL_PHASE_1_COMPLETE_01 <-- SRC_UCB_WRITE_CAL]
* [SDD_MAG_CAL_PHASE_1_COMPLETE_02 <-- SRC_UCB_WRITE_CAL]
* [SDD_MAG_CAL_PHASE_1_COMPLETE_03 <-- SRC_UCB_WRITE_CAL]
* [SDD_MAG_CAL_PHASE_1_COMPLETE_04 <-- SRC_UCB_WRITE_CAL]
* [SDD_MAG_CAL_PHASE_1_COMPLETE_05 <-- SRC_UCB_WRITE_CAL]
*
* [SDD_MAG_CAL_PHASE_2_01 <-- SRC_UCB_WRITE_CAL]
* [SDD_MAG_CAL_PHASE_2_02 <-- SRC_UCB_WRITE_CAL]
* [SDD_MAG_CAL_PHASE_2_03 <-- SRC_UCB_WRITE_CAL]
* [SDD_MAG_CAL_PHASE_2_04 <-- SRC_UCB_WRITE_CAL]
* [SDD_MAG_CAL_PHASE_2_05 <-- SRC_UCB_WRITE_CAL]
* [SDD_MAG_CAL_PHASE_2_COMPLETE_01 <-- SRC_UCB_WRITE_CAL]
* [SDD_MAG_CAL_PHASE_2_COMPLETE_02 <-- SRC_UCB_WRITE_CAL]
* [SDD_MAG_CAL_PHASE_2_COMPLETE_03 <-- SRC_UCB_WRITE_CAL]
* [SDD_MAG_CAL_PHASE_2_COMPLETE_04 <-- SRC_UCB_WRITE_CAL]
* [SDD_MAG_CAL_PHASE_2_COMPLETE_05 <-- SRC_UCB_WRITE_CAL]
* [SDD_MAG_CAL_PHASE_2_COMPLETE_06 <-- SRC_UCB_WRITE_CAL]
* [SDD_MAG_CAL_PHASE_2_COMPLETE_07 <-- SRC_UCB_WRITE_CAL]
* [SDD_MAG_CAL_PHASE_2_TERMINATE_01 <-- SRC_UCB_WRITE_CAL]
* [SDD_MAG_CAL_PHASE_2_TERMINATE_02 <-- SRC_UCB_WRITE_CAL]
* [SDD_MAG_CAL_PHASE_2_TERMINATE_03 <-- SRC_UCB_WRITE_CAL]
* [SDD_MAG_CAL_PHASE_2_TERMINATE_04 <-- SRC_UCB_WRITE_CAL]
* [SDD_MAG_CAL_PHASE_2_TERMINATE_05 <-- SRC_UCB_WRITE_CAL]
* [SDD_MAG_CAL_STORE_CAL_01 <-- SRC_UCB_WRITE_CAL]
* [SDD_MAG_CAL_STORE_CAL_02 <-- SRC_UCB_WRITE_CAL]
* [SDD_MAG_CAL_STORE_CAL_03 <-- SRC_UCB_WRITE_CAL]
* [SDD_MAG_CAL_STORE_CAL_04 <-- SRC_UCB_WRITE_CAL]
*
* Input parameters:
*         port               port number request came in on, the reply will go out
*                            this port
*         packet             data part of packet
*
* Output parameters:
*         none
*
* Return value:
*         none
*********************************************************************************/
static void HandleUcbWriteCal (ExternPortTypeEnum port, PacketPtrType packetPtr)
{
    /* tells calibrate routine to terminate phase 2 alignment */
    extern int magCalTerminateRequested;
    
    uint16_t calRequest = (uint16_t)((packetPtr.ucbPacketPtr->payload[0] << 8) | packetPtr.ucbPacketPtr->payload[1]);

    INFO_STRING("WCAL\r\n");

    /* verify that the packet length matches packet specification
	and a magnetometer is present */
    if ((packetPtr.ucbPacketPtr->payloadLength == 2) &&
		(getBITflag(BITSTAT_NOEXTERNALMAG) == FALSE)) {
            
            /* save requesting port and packet for responses initiated OUTSIDE this handler */
            calRequestPort      = port;
            calRequestPacketPtr = packetPtr;
            
            /* handle specific calibration request */
            switch (calRequest) {
            case MAG_ALIGN_STATUS_LEVEL_START: {
                /* start phase 1 - axis leveling */
                gAlgorithm.calState = MAG_ALIGN_STATUS_LEVEL_START;
                
                break;
            }
            case MAG_ALIGN_STATUS_START_CAL_WITHOUT_AUTOEND: {
                /* start phase 2 - alignment WITHOUT auto termination */
                gAlgorithm.calState = MAG_ALIGN_STATUS_START_CAL_WITHOUT_AUTOEND;
                
                break;
            }
            case MAG_ALIGN_STATUS_START_CAL_WITH_AUTOEND: {
                /* start phase 2 - alignment WITH auto termination */
                gAlgorithm.calState = MAG_ALIGN_STATUS_START_CAL_WITH_AUTOEND;
                
                break;
            }
            case MAG_ALIGN_STATUS_TERMINATION: {
                if ((gAlgorithm.calState == MAG_ALIGN_STATUS_START_CAL_WITHOUT_AUTOEND) ||
					(gAlgorithm.calState == MAG_ALIGN_STATUS_START_CAL_WITH_AUTOEND)) { 
                        
                        /* terminate phase 2 - alignment */
                        magCalTerminateRequested = TRUE;
                    } else {
                        /* return NAK with requested packet type in data field */
                        UcbPacketPacketTypeToBytes(packetPtr.ucbPacketPtr->packetType, packetPtr.ucbPacketPtr->payload);
                        
                        packetPtr.ucbPacketPtr->packetType    = UCB_NAK;
                        packetPtr.ucbPacketPtr->payloadLength = UCB_PACKET_TYPE_LENGTH;
                    }
                
                break;
            }
            case MAG_ALIGN_STATUS_SAVE2EEPROM: {
                /* end of phase 2 calibration? */
                if (gAlgorithm.calState == MAG_ALIGN_STATUS_TERMINATION) {
                    /* attempt to write configuration offset, hard, and soft iron angles to EEPROM */
                    /* apparently "TRUE" means the routine failed (bad design) */
                    if ((writeEEPROMWords(OFFSET_ANGLES_EXT_MAG_FIELD_ID,     2, gConfiguration.OffsetAnglesExtMag    ) == TRUE) ||
                        (writeEEPROMWords(HARD_IRON_BIAS_EXT_FIELD_ID,        2, gConfiguration.hardIronBiasExt       ) == TRUE) ||
                            (writeEEPROMWords(SOFT_IRON_SCALE_RATIO_EXT_FIELD_ID, 1, &gConfiguration.softIronScaleRatioExt) == TRUE) ||
                                (writeEEPROMWords(SOFT_IRON_ANGLE_EXT_FIELD_ID,       1, &gConfiguration.softIronAngleExt     ) == TRUE))
                    {
                        /* return NAK with requested packet type in data field */
                        UcbPacketPacketTypeToBytes(packetPtr.ucbPacketPtr->packetType, packetPtr.ucbPacketPtr->payload);
                        
                        packetPtr.ucbPacketPtr->packetType    = UCB_NAK;
                        packetPtr.ucbPacketPtr->payloadLength = UCB_PACKET_TYPE_LENGTH;
					} else {
                        /* store magnetometer serial number in current configuration */
                        gConfiguration.remoteMagSerNo[0] = (((uint16_t)currentMagSerialNumber[0] << 8) & 0xff00) | ((uint16_t)currentMagSerialNumber[1] & 0x00ff);
                        gConfiguration.remoteMagSerNo[1] = (((uint16_t)currentMagSerialNumber[2] << 8) & 0xff00) | ((uint16_t)currentMagSerialNumber[3] & 0x00ff);
                        
                        /* store magnetometer serial number in EEPROM */
                        writeEEPROMWords(REMOTE_MAG_SER_NO_FIELD_ID,     1, &gConfiguration.remoteMagSerNo[0]);
                        writeEEPROMWords(REMOTE_MAG_SER_NO_FIELD_ID + 1, 1, &gConfiguration.remoteMagSerNo[1]);
                        
                        /* store magnetometer serial number in algorithm data structure */
                        gAlgorithm.ExtMagSerialNumber = ((gConfiguration.remoteMagSerNo[0] << 16) & 0xffff0000) | (gConfiguration.remoteMagSerNo[1] & 0x0000ffff);
                        
                        /* reset magnetometer calibration state,
                        this is a "handshake" with the algorithm calibration routine */
                        gAlgorithm.calState = MAG_ALIGN_STATUS_BACKTONORMAL;
                    }
                } else {   /* phase 2 calibration hasn't been completed, not allowed to write the results */
                    /* return NAK with requested packet type in data field */
                    UcbPacketPacketTypeToBytes(packetPtr.ucbPacketPtr->packetType, packetPtr.ucbPacketPtr->payload);
                    
                    packetPtr.ucbPacketPtr->packetType    = UCB_NAK;
                    packetPtr.ucbPacketPtr->payloadLength = UCB_PACKET_TYPE_LENGTH;
                }
                
                break;
            }
            default: {
                /* unrecognized calibration request */
                
                /* return NAK with requested packet type in data field */
                UcbPacketPacketTypeToBytes(packetPtr.ucbPacketPtr->packetType, packetPtr.ucbPacketPtr->payload);
                
                packetPtr.ucbPacketPtr->packetType         = UCB_NAK;
                packetPtr.ucbPacketPtr->payloadLength = UCB_PACKET_TYPE_LENGTH;
                
                break;
            }
            }
        } else {
            /* return NAK with requested packet type in data field */
            UcbPacketPacketTypeToBytes(packetPtr.ucbPacketPtr->packetType, packetPtr.ucbPacketPtr->payload);
            
            packetPtr.ucbPacketPtr->packetType      = UCB_NAK;
            packetPtr.ucbPacketPtr->payloadLength = UCB_PACKET_TYPE_LENGTH;
        }
    
    /* send response packet */
    ExternPortTx(port, packetPtr);
}

/*********************************************************************************
* Function name:	HandleUcbGenAirDataAiding
*
* Description:     handles UCB air data aiding packet
*
* Trace: [SDD_UCB_GEN_AD <-- SRC_UCB_GEN_AD]
*
* Input parameters:
*
* Output parameters:
*
* Return value:
*********************************************************************************/
static void HandleUcbGenAirDataAiding (ExternPortTypeEnum port, PacketPtrType packetPtr)
{
    INFO_STRING("ERROR- AIRA\r\n");

#if DMU380_NOT_WORKING
	uint16_t airSpeedTrue = ((uint16_t)packetPtr.ucbPacketPtr->payload[1] << 8) | (uint16_t)packetPtr.ucbPacketPtr->payload[2];
    
	int32_t altitudeCorrected = (int32_t)((packetPtr.ucbPacketPtr->payload[5] << 24) |
                                          (packetPtr.ucbPacketPtr->payload[6] << 16) |
                                              (packetPtr.ucbPacketPtr->payload[7] << 8)  |
                                                  (packetPtr.ucbPacketPtr->payload[8]));
    
    
	gAirdataDataRdy = TRUE;
    
	GDaidMode= packetPtr.ucbPacketPtr->payload[0];
    
	algorithmAirData.airDataSolution[TAS] = ((double)airSpeedTrue) / MAXuint16_t_OVER_512;
    
	algorithmAirData.airDataSolution[CALTI] = ((double)altitudeCorrected) / MAXINT32_20BIT_OVER131072M;
#endif // DMU380_NOT_WORKING
}

/*********************************************************************************
* Function name:	HandleUcbGenHeadingAiding
*
* Description:      handles UCB heading aiding packet
*
* Trace: [SDD_UCB_GEN_HEADING <-- SRC_UCB_GEN_HEADING] 
*        [SDD_ALGORITHM_RESPONSE2GH_0 <-- SRC_UCB_GEN_HEADING] 
*        [SDD_ALGORITHM_RESPONSE2GH_1 <-- SRC_UCB_GEN_HEADING] 
*        [SDD_ALGORITHM_RESPONSE2GH_2 <-- SRC_UCB_GEN_HEADING] 
*        [SDD_ALGORITHM_RESPONSE2GH_3 <-- SRC_UCB_GEN_HEADING] 
*        [SDD_RESP_HEADING_AIDING_NAK_01 <-- SRC_UCB_GEN_HEADING]
*        [SDD_RESP_HEADING_AIDING_NAK_02 <-- SRC_UCB_GEN_HEADING]
*
* Input parameters:
*         port               port number request came in on, the reply will go out
*                            this port
*         packet             data part of packet
*
* Output parameters:
*         none
*
* Return value:
*         none
*********************************************************************************/
static void HandleUcbGenHeadingAiding (ExternPortTypeEnum port, PacketPtrType packetPtr)
{
    INFO_STRING("ERROR- HEAD\r\n");

#if DMU380_NOT_WORKING
    
    /* need persistent storage since heading aiding input function call takes pointers (?) */
    static unsigned int GHCmd;
    static double       compassHeading;
    
    GHCmd = packetPtr.ucbPacketPtr->payload[0];
    
    /* cast and scale incoming heading aiding data */ 
    compassHeading = (Int16ToInt32((packetPtr.ucbPacketPtr->payload[1] << 8) | packetPtr.ucbPacketPtr->payload[0])) /  MAXUINT16_OVER_2PI;  
    
    /* try to send command */
    if (actionsForHeadingAidingInput(&GHCmd, &compassHeading) == FALSE) {
        /* return NAK with requested packet type in data field */
        UcbPacketPacketTypeToBytes(packetPtr.ucbPacketPtr->packetType, packetPtr.ucbPacketPtr->payload);
        
        packetPtr.ucbPacketPtr->packetType 	  = UCB_NAK;
        packetPtr.ucbPacketPtr->payloadLength = UCB_PACKET_TYPE_LENGTH;
    }
    
    /* send response packet */
    ExternPortTx(port, packetPtr);
#endif // DMU380_NOT_WORKING
    
}

/*********************************************************************************
* Function name:	HandleUcbErrorInvalidType
*
* Description:      UCB error packet
*
* Trace: [SDD_UCB_UNKNOWN_02 <-- SRC_UCB_UNKNOWN]
*
* Input parameters:
*
* Output parameters:
*
* Return value:
*********************************************************************************/
static void HandleUcbErrorInvalidType (ExternPortTypeEnum port, PacketPtrType packetPtr)
{
    INFO_STRING("ERROR- INVALID\r\n");
	/* return NAK, requested packet type placed in data field by external port */
	packetPtr.ucbPacketPtr->packetType 	  = UCB_NAK;
	packetPtr.ucbPacketPtr->payloadLength = UCB_PACKET_TYPE_LENGTH;
    
	ExternPortTx(port, packetPtr);
}

/*********************************************************************************
* Function name:	HandleUcbErrorTimeout
*
* Description:      UCB error time nak packet
*
* Trace: [SDD_UCB_TIMEOUT_02 <-- SRC_UCB_TIMEOUT_REPLY]
*
* Input parameters:
*
* Output parameters:
*
* Return value:
*********************************************************************************/
static void HandleUcbErrorTimeout (ExternPortTypeEnum port, PacketPtrType packetPtr)
{
    INFO_STRING("ERROR- TIMEOUT\r\n");
	/* return NAK, requested packet type placed in data field by external port */
	packetPtr.ucbPacketPtr->packetType 	  = UCB_NAK;
	packetPtr.ucbPacketPtr->payloadLength = UCB_PACKET_TYPE_LENGTH;
    
	ExternPortTx(port, packetPtr);
}

/*********************************************************************************
* Function name:	HandleUcbErrorCrcFail
*
* Description:     UCB crc failure NAK packet
*
* Trace: [SDD_UCB_CRC_FAIL_02 <-- SRC_UCB_CRCFAIL_REPLY]
*
* Input parameters:
*
* Output parameters:
*
* Return value:
*********************************************************************************/
static void HandleUcbErrorCrcFail (ExternPortTypeEnum port, PacketPtrType packetPtr)
{
    INFO_STRING("ERROR- CRC\r\n");
	/* return NAK, requested packet type placed in data field by external port */
	packetPtr.ucbPacketPtr->packetType 	  = UCB_NAK;
	packetPtr.ucbPacketPtr->payloadLength = UCB_PACKET_TYPE_LENGTH;
    
	ExternPortTx(port, packetPtr);
}

/*********************************************************************************
* Function name:	HandleCrm
*
* Description:      handle crm packet
*
* Trace:
*       [SDD_REMOTEMAGDATA_COPYDATA <-- SRC_REMOTEMAG_COPYDATA]
*       [SDD_REMOTEMAGDATA_BIT <-- SRC_REMOTEMAG_COPYDATA]
*       [SDD_REMOTEMAGDATA_REVOLVE <-- SRC_REMOTEMAG_COPYDATA]
*       [SDD_REMOTEMAGDATA_REVOLVE_SN <-- SRC_REMOTEMAG_COPYDATA]
*       [SDD_REMOTEMAGDATA_REVOLVE_SNCHK <-- SRC_REMOTEMAG_COPYDATA]
*       [SDD_REMOTEMAGDATA_REVOLVE_HDR <-- SRC_REMOTEMAG_COPYDATA]
*       [SDD_REMOTEMAGDATA_REVOLVE_ADCT <-- SRC_REMOTEMAG_COPYDATA]
*       [SDD_REMOTEMAGDATA_REVOLVE_CHKSUM <-- SRC_REMOTEMAG_COPYDATA]
*       [SDD_REMOTE_MAG_PRE_PROCESSING_01  <-- SRC_REMOTEMAG_COPYDATA]
*
* Input parameters:
*
* Output parameters:
*
* Return value:
*********************************************************************************/
static void HandleCrm (ExternPortTypeEnum port, PacketPtrType packetPtr)
{
    INFO_STRING("ERROR- CRM\r\n");

#if DMU380_NOT_WORKING
    
	uint16_t mode;
    
    uint8_t typeIndex;
    uint8_t byteIndex;
    
    uint8_t revolvingByte;
    
    static uint16_t serialNumberBytesReceived = 0;
    
    static uint8_t messageReceiveBuffer [4] = { 0,0,0,0};
    
    static BOOL messageHeaderReceived = FALSE;
    
    static uint8_t messageBytesReceived = 0;
    
    uint16_t messageChecksum;
    
	/* indicate new ext mag data ready */
	gRemoteMagDataRdy = TRUE;
    
	/* X accel */
	gRemoteMagData.accelVector[0] = packetPtr.crmPacketPtr->xAccel * gRemoteMagData.accelinputscale[0];
	gAlgorithm.ExtAccels[0]        = gRemoteMagData.accelVector[0];
    
	/* Y accel */
	gRemoteMagData.accelVector[1] = packetPtr.crmPacketPtr->yAccel * gRemoteMagData.accelinputscale[1];
	gAlgorithm.ExtAccels[1]        = gRemoteMagData.accelVector[1];
    
	/* Z accel */
	gRemoteMagData.accelVector[2] = packetPtr.crmPacketPtr->zAccel * gRemoteMagData.accelinputscale[2];
	gAlgorithm.ExtAccels[2]        = gRemoteMagData.accelVector[2];
    
	/* X mag */
	gRemoteMagData.magVector[0] = packetPtr.crmPacketPtr->xMag * gRemoteMagData.maginputscale[0];
	gAlgorithm.ExtMags[0]        = gRemoteMagData.magVector[0];
    
	/* Y mag */
	gRemoteMagData.magVector[1] = packetPtr.crmPacketPtr->yMag * gRemoteMagData.maginputscale[1];
	gAlgorithm.ExtMags[1]        = gRemoteMagData.magVector[1];
    
	/* Z mag */
	gRemoteMagData.magVector[2] = packetPtr.crmPacketPtr->zMag * gRemoteMagData.maginputscale[2];
	gAlgorithm.ExtMags[2]        = gRemoteMagData.magVector[2];
    
	/* call pre-processing of the mag data */
	remotemag_preProcessing();
    
	/* mag BIT */
	gRemoteMagData.BITWord = packetPtr.crmPacketPtr->bit & 0xffff;
    
	/* set mag hard failure BIT */
	if ((gRemoteMagData.BITWord & CRM_BIT_HARD_FAIL) == CRM_BIT_HARD_FAIL) {
		setBITflag(BITSTAT_EXTMAGFAIL, TRUE);
	} else {
		setBITflag(BITSTAT_EXTMAGFAIL, FALSE);
	}
    
	/* determine commutated value interpretation of model number field (LSB of model number field) */
	mode = (uint16_t)(packetPtr.crmPacketPtr->modelNo & 0xffff);
    
	/* direct revolving data? */
	if (((mode >> 8) & 0xff) != 0xff) {
        /* revolving data control fields */
        typeIndex = (uint8_t)((mode >> 14) & 0x03);
        byteIndex = (uint8_t)((mode >> 8) & 0x3f);
        
        /* revolving data byte */
        revolvingByte = (uint8_t)(mode & 0xff);
        
		/* receiving ext mag serial number? */
		if (typeIndex == 0) {
			/* save the specified byte of the serial number */
			currentMagSerialNumber[byteIndex] = revolvingByte;
            
            /* may get bytes out of order, 
            * need to keep track of number received
            * or a bit position for each byte position received */
            ++serialNumberBytesReceived;
            
            /* all serial number bytes received? */
            if (serialNumberBytesReceived == RMAG_SERIAL_NUMBER_SIZE) {
                /* reset number of bytes received */
                serialNumberBytesReceived = 0;
                
				/* alignment/calibration is not in progress? */
				if(gAlgorithm.calState == MAG_ALIGN_STATUS_BACKTONORMAL) {
					/* compare received serial number against serial number stored in configuration */
					if ((((uint8_t)(gConfiguration.remoteMagSerNo[0] >> 8) & 0xff) == currentMagSerialNumber[0]) &&
						( (uint8_t)(gConfiguration.remoteMagSerNo[0]       & 0xff) == currentMagSerialNumber[1]) &&
                            (((uint8_t)(gConfiguration.remoteMagSerNo[1] >> 8) & 0xff) == currentMagSerialNumber[2]) &&
                                ( (uint8_t)(gConfiguration.remoteMagSerNo[1]       & 0xff) == currentMagSerialNumber[3]))
					{
						/* report serial number match result to BIT */
						setBITflag(BITSTAT_MAGSN, FALSE);  
					} else {
						/* report serial number match result to BIT */
						setBITflag(BITSTAT_MAGSN, TRUE);
					}
				} else {	/* alignment/calibration is in progress */
					/* don't check serial number match while calibration is taking place */
					setBITflag(BITSTAT_MAGSN, FALSE);
				}
		  	}
		}
	} else {	/* receiving a messsage */
        if (messageHeaderReceived == FALSE) {
            /* shift previously received byte into high byte position */
            
            messageReceiveBuffer[1] = messageReceiveBuffer[0];
            
            /* shift in current received byte */
            messageReceiveBuffer[0] = (uint8_t)(mode & 0xff);
            
            /* received sync header? */
            if ((messageReceiveBuffer[1] == 0xbb) &&
                (messageReceiveBuffer[0] == 0x55))
            {
                messageHeaderReceived = TRUE;
            }
		} else {
            /* receive each byte of the message based on current number of bytes received */
            switch (messageBytesReceived) {
            case TEMPERATURE_ADC_COUNT_MSB: {
                messageReceiveBuffer[TEMPERATURE_ADC_COUNT_MSB] = (uint8_t)(mode & 0xff);
                ++messageBytesReceived;
                break;
            }
            case TEMPERATURE_ADC_COUNT_LSB: {
                messageReceiveBuffer[TEMPERATURE_ADC_COUNT_LSB] = (uint8_t)(mode & 0xff);
                ++messageBytesReceived;
                break;
            }
            case MSG_CHECKSUM_MSB: {
                messageReceiveBuffer[MSG_CHECKSUM_MSB] = (uint8_t)(mode & 0xff);
                ++messageBytesReceived;
                break;
            }
            case MSG_CHECKSUM_LSB: {
                messageReceiveBuffer[MSG_CHECKSUM_LSB] = (uint8_t)(mode & 0xff);
                
                /* compute checksum and compare with received checksum */
                messageChecksum  = messageReceiveBuffer[TEMPERATURE_ADC_COUNT_MSB];
                messageChecksum += messageReceiveBuffer[TEMPERATURE_ADC_COUNT_LSB];
                
                /* checksum matches? */
                if (messageChecksum == (uint16_t)(((messageReceiveBuffer[MSG_CHECKSUM_MSB] << 8) |
                                                   (messageReceiveBuffer[MSG_CHECKSUM_LSB])) 	  &
                                                  (0xffff)))
                {
                    /* valid message, save temperature ADC data */
                    gRemoteMagData.magTemp = (double)(((messageReceiveBuffer[TEMPERATURE_ADC_COUNT_MSB] << 8) |
                                                       (messageReceiveBuffer[TEMPERATURE_ADC_COUNT_MSB]))      &
                                                      0xffff);
                    
                    /* scale and shift to get degrees C */
                    gRemoteMagData.magTemp *= TEMPERATURE_ADC_COUNT_SCALE_FACTOR;	/* scale */
                    gRemoteMagData.magTemp += TEMPERATURE_ADC_COUNT_SHIFT_FACTOR;	/* shift */
                }
                
                /* reset receiving even if checksum fails */
                messageHeaderReceived = FALSE;
                
                /* reset number of message bytes received */
                messageBytesReceived = TEMPERATURE_ADC_COUNT_MSB;
                
                break;
            }
            default:
                /* reset receiving */
                messageHeaderReceived = FALSE;
                /* reset number of message bytes received */
                messageBytesReceived = TEMPERATURE_ADC_COUNT_MSB;
                break;
                
            }
        }
	}
#endif // DMU380_NOT_WORKING
}

/*********************************************************************************
* Function name:	HandleCrmErrorTimeout
*
* Description:       Handle the crm error time case
*
* Trace: [SDD_CRM_TIMEOUT <-- SRC_CRM_TIMEOUT]
*
* Input parameters:
*
* Output parameters:
*
* Return value:
*********************************************************************************/
static void HandleCrmErrorTimeout (ExternPortTypeEnum port, PacketPtrType packetPtr)
{
    INFO_STRING("ERROR- CRM TIMEOUT\r\n");
	return;
}

/*********************************************************************************
* Function name:	HandleCrmErrorChecksumFail
*
* Description:      handles crm checksum failure
*
* Trace: [SDD_CRM_CRCFAIL <-- SRC_CRM_CRCFAIL]
*
* Input parameters:
*
* Output parameters:
*
* Return value:
*********************************************************************************/
static void HandleCrmErrorChecksumFail (ExternPortTypeEnum port, PacketPtrType packetPtr)
{
    INFO_STRING("ERROR- CRM CHECKSUM\r\n");
	return;
}

/*********************************************************************************
* Function name:	HandlePacket
*
* Description:    general handler
*
* Trace: [SDD_HANDLE_PKT <-- SRC_HANDLE_PACKET]
*
* Input parameters:
*
* Output parameters:
*
* Return value:
*********************************************************************************/
void HandlePacket (ExternPortTypeEnum port, PacketPtrType packetPtr)
{
	/* call appropriate function based on port type */
	if (port == PRIMARY_UCB_PORT)
	{
		switch (packetPtr.ucbPacketPtr->packetType) {
#			define INPUT_PACKET_TYPES
#			define INPUT_ERROR_PACKET_TYPES
#			define PACKET_TYPE(constName, varName, code)	case constName: Handle ## varName(port, packetPtr); break;
#			include "ucb_packet_types.def"
#			undef INPUT_PACKET_TYPES
#			undef INPUT_ERROR_PACKET_TYPES
#			undef PACKET_TYPE
            
        default:	break;	/* default handler? */
		}
	}
	else if (port == MAG_DIAG_PORT)
	{
		switch (packetPtr.ucbPacketPtr->packetType) {
#			define PACKET_TYPE(constName, varName)	case constName: Handle ## varName(port, packetPtr); break;
#			include "crm_packet_types.def"
#			undef PACKET_TYPE
            
        default:	break;	/* default handler? */
		}
	}
}
/* end HandlePacket() */
