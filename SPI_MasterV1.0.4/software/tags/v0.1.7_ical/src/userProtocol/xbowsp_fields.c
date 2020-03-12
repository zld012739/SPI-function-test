/*****************************************************************************
* File name:  xbowsp_fields.c
*
* File description:
*   - Checking field data per Crossbow Serial Protocol
*   - Providing validity checks on configuration field data.
*
* $Rev: 17482 $
* $Date: 2011-02-09 22:58:58 -0800 (Wed, 09 Feb 2011) $
* $Author: by-denglish $
***********************************************************************************/
#include <math.h>
#include <stdint.h>
#include "xbowsp_BITStatus.h"
#include "xbowsp_algorithm.h"
#include "xbowsp_version.h"
#include "xbowsp_generaldrivers.h"
#include "calc_airData.h"
#include "crc.h"
#include "ucb_packet.h"
#include "xbowsp_fields.h"
#include "s_eeprom.h"
#include "xbowsp_init.h"
#include "crm_packet.h"
#include "packet.h"
#include "extern_port.h"
#include "extern_port_config.h"
#include "handle_packet.h"
#include "send_packet.h"
#include "scaling.h"
#include "matrix.h"
#include "sensor.h"
#include "cast.h"

/* proposed configurations */
static ConfigurationStruct proposedRamConfiguration;
static ConfigurationStruct proposedEepromConfiguration;

/* indicates if any port settings are to be changed */
static BOOL portConfigurationChanged = FALSE;

/* for scaled sensor packet */
#define MAX_TEMP_4_SENSOR_PACKET		99.9

/*********************************************************************************
* Function name:	DefaultPortConfiguration
*
* Description:	 Set initaal ports:
* port 1 primary UCB, 57600 baud, AU packet, /1 packet divider
* port 2 CRM input, 38400 baud
* port 3 9600 baud, unassigned
* port 4 9600 baud, unassigned
*
* Trace: [SDD_CFG_PORT_DEF_01 <-- SRC_CFG_PORT_DEF]
*		 [SDD_CFG_PORT_DEF_02 <-- SRC_CFG_PORT_DEF]
*
* Input parameters:	none
*
* Output parameters:	none
*
* Return value:	 none
*
*********************************************************************************/
void DefaultPortConfiguration (void)
{
#define DEFAULT_CONFIG(field, value)	gConfiguration.field = value;
#include "extern_port_config.h"
#undef DEFAULT_CONFIG
}
/* end DefaultPortConfiguration */

/******************************************************
* Function name:    CheckPacketRateDivider
*
* Description:
*   -checks for valid data.
*
* Trace:
* [SDD_CHECK_PACKET_RATE_DIVIDER <-- SRC_CHECK_PACKET_RATE_DIVIDER]
*
* Input parameters:
*   uint16_t packetRateDivider: the divider for the requested packet rate.
*
* Output parameters:
* 	No output parameters
*
* Return value:
*   1 if available, zero otherwise.
*
*  Author: Darren Liccardo, Jan. 2004
*   	   Dong An, 2007, 2008
*******************************************************/
BOOL CheckPacketRateDivider (uint16_t packetRateDivider)
{
    switch (packetRateDivider) {
    case 0:
    case 1:
    case 2:
    case 4:
    case 5:
    case 10:
    case 20:
    case 25:
    case 50: return TRUE;
    default: return FALSE;
    }
}
/* end CheckPacketRateDivider */

/*********************************************************************************
* Function name:	CheckContPacketRate
*
* Description:	verify the packet can be 'comfortably' output at the
* baud rate and divider rate
*
* Trace: [SDD_PKT_CONT_RATE_CHK <-- SRC_PKT_CONT_RATE_CHK]
*
* Input parameters:	packet type, baud rate, divider
*
* Output parameters:	none
*
* Return value:	boolean, TRUE if the packet output is possible, FALSE if not
*
*********************************************************************************/
BOOL CheckContPacketRate (UcbPacketTypeEnum outputPacket, uint16_t baudRate, uint16_t packetRateDivider)
{
    BOOL valid = TRUE;

    uint16_t bytesPerPacket;
    uint16_t bytesPerSecond;

    if (packetRateDivider == 0) {
        valid = TRUE;
    } else {
        bytesPerPacket = UCB_SYNC_LENGTH + UCB_PACKET_TYPE_LENGTH + UCB_PAYLOAD_LENGTH_LENGTH + UCB_CRC_LENGTH;

        switch (outputPacket) {
        case UCB_IDENTIFICATION:    bytesPerPacket += UCB_IDENTIFICATION_LENGTH;    break;
        case UCB_TEST_0:            bytesPerPacket += UCB_TEST_0_LENGTH;            break;
        case UCB_TEST_1:            bytesPerPacket += UCB_TEST_1_LENGTH;            break;
        case UCB_FACTORY_1:         bytesPerPacket += UCB_FACTORY_1_LENGTH;         break;
        case UCB_FACTORY_2:         bytesPerPacket += UCB_FACTORY_2_LENGTH;         break;
        case UCB_FACTORY_5:         bytesPerPacket += UCB_FACTORY_5_LENGTH;         break;
        case UCB_FACTORY_6:         bytesPerPacket += UCB_FACTORY_6_LENGTH;         break;
        case UCB_FACTORY_7:         bytesPerPacket += UCB_FACTORY_7_LENGTH;         break;
        case UCB_SCALED_3:          bytesPerPacket += UCB_SCALED_3_LENGTH;          break;
        case UCB_ANGLE_3:           bytesPerPacket += UCB_ANGLE_3_LENGTH;           break;
        case UCB_ANGLE_5:           bytesPerPacket += UCB_ANGLE_5_LENGTH;           break;
        case UCB_ANGLE_U:           bytesPerPacket += UCB_ANGLE_U_LENGTH;           break;
        case UCB_VERSION_DATA:      bytesPerPacket += UCB_VERSION_DATA_LENGTH;      break;
        case UCB_VERSION_ALL_DATA:  bytesPerPacket += UCB_VERSION_ALL_DATA_LENGTH;  break;

        default:                    valid = FALSE;
        }

        bytesPerSecond = bytesPerPacket * (SERIAL_TX_ROUTINE_FREQUENCY / packetRateDivider);

        switch (baudRate) {
        case 0:     valid = (BOOL)(bytesPerSecond<950);     break;  /* 9600 BAUD with 10 byte/sec margin */
        case 1:     valid = (BOOL)(bytesPerSecond<1910);    break;  /* 19200 BAUD with 10 byte/sec margin */
        case 2:     valid = (BOOL)(bytesPerSecond<3820);    break;  /* 38400 BAUD with 20 byte/sec margin */
        case 3:     valid = (BOOL)(bytesPerSecond<5720);    break;  /* 57600 BAUD with 40 byte/sec margin */

        default:    valid = FALSE;
        }
    }

    return valid;
}
/* end CheckContPacketRate */

/*********************************************************************************
* Function name:	CheckPortUsage
*
* Description:	Assign no more than 1 port to primary UCB,
*  no more than 1 port to CRM and they cannot be the same port
*
* Trace: [SDD_PORT_USE_CHK <-- SRC_PORT_USE_CHK]
*
* Input parameters:	configuration array
*
* Output parameters:	none
*
* Return value:	boolean, TRUE if port assignment is OK, FALSE if not
*
*********************************************************************************/
BOOL CheckPortUsage (ConfigurationStruct *proposedConfiguration)
{
    BOOL valid = TRUE;

    BOOL primaryDefined = FALSE;
    BOOL crmDefined     = FALSE;

    /* check port 1 in range and usage */
    if (proposedConfiguration->port1Usage >= NUM_PORT_TYPES) {
        valid = FALSE;
    } else {
        if (proposedConfiguration->port1Usage == PRI_UCB_PORT) {
            primaryDefined = TRUE;
        } else if (proposedConfiguration->port1Usage == CRM_PORT) {
            crmDefined = TRUE;
        }
    }
#if DMU380_NOT_WORKING
    /* check port 2 in range and usage */
    if (proposedConfiguration->port2Usage >= NUM_PORT_TYPES) {
        valid = FALSE;
    } else {
        if (proposedConfiguration->port2Usage == PRI_UCB_PORT) {

            if (primaryDefined == FALSE) {
                primaryDefined = TRUE;
            } else {
                valid = FALSE;
            }
        } else if (proposedConfiguration->port2Usage == CRM_PORT) {

            if (crmDefined == FALSE) {
                crmDefined = TRUE;
            } else {
                valid = FALSE;
            }
        }
    }

    /* check port 3 in range and usage */
    if (proposedConfiguration->port3Usage >= NUM_PORT_TYPES) {
        valid = FALSE;
    } else {
        if (proposedConfiguration->port3Usage == PRI_UCB_PORT) {

            if (primaryDefined == FALSE) {
                primaryDefined = TRUE;
            } else {
                valid = FALSE;
            }
        } else if (proposedConfiguration->port3Usage == CRM_PORT) {

            if (crmDefined == FALSE) {
                crmDefined = TRUE;
            } else {
                valid = FALSE;
            }
        }
    }

    /* check port 4 in range and usage */
    if (proposedConfiguration->port4Usage >= NUM_PORT_TYPES) {
        valid = FALSE;
    } else {
        if (proposedConfiguration->port4Usage == PRI_UCB_PORT) {

            if (primaryDefined == FALSE) {
                primaryDefined = TRUE;
            } else {
                valid = FALSE;
            }
        } else if (proposedConfiguration->port4Usage == CRM_PORT) {

            if (crmDefined == FALSE) {
                crmDefined = TRUE;
            } else {
                valid = FALSE;
            }
        }
    }
#endif // DMU380_NOT_WORKING


   if (primaryDefined == FALSE) {
        valid = FALSE;
    }
#if DMU380_NOT_WORKING
    /* both must be specified for a valid configuration */
    if ((primaryDefined == FALSE) ||
        (crmDefined == FALSE)) {
        valid = FALSE;
    }
#endif  // DMU380_NOT_WORKING
    return valid;
}

/*********************************************************************************
* Function name:	CheckPortBaudRate
*
* Description:
*   all serial port baud rates must be 9600, 19200, 38400 or 57600
*
* Trace: [SDD_PORT_CFG_VALID_01 <-- SRC_CHECK_PORT_BAUD_RATE]
*
* Input parameters:	baud rate
*
* Output parameters:	none
*
* Return value:	boolean, TRUE if baud rate is valid, FALSE if not
*
*********************************************************************************/
BOOL CheckPortBaudRate (uint16_t portBaudRate)
{
    BOOL valid = TRUE;

    if (portBaudRate >= NUM_BAUD_RATES) {
        valid = FALSE;
    }

    return valid;
}

/*********************************************************************************
* Function name:	ValidPortConfiguration
*
* Description:
*   Check output packet configuration members for sanity
*
* Trace: [SDD_PORT_CFG_VALID_01 <-- SRC_PORT_CFG_VALID]
*        [SDD_PORT_CFG_VALID_02 <-- SRC_PORT_CFG_VALID]
*        [SDD_PORT_CFG_VALID_03 <-- SRC_PORT_CFG_VALID]
*        [SDD_PORT_CFG_VALID_04 <-- SRC_PORT_CFG_VALID]
*        [SDD_PORT_CFG_VALID_05 <-- SRC_PORT_CFG_VALID]
*
* Input parameters:	configuration
*
* Output parameters:	none
*
* Return value:	boolean, TRUE if OK, FALSE if not
*********************************************************************************/
BOOL ValidPortConfiguration (ConfigurationStruct *proposedConfiguration)
{
    /* working packet type byte buffer */
    uint8_t type [UCB_PACKET_TYPE_LENGTH];

    UcbPacketTypeEnum continuousPacketType;

    BOOL valid = TRUE;

    /* check packet rate divider */
    valid &= (BOOL)(CheckPacketRateDivider(proposedConfiguration->packetRateDivider));

    /* get enum for requested continuous packet type */
    type[0] = (uint8_t)((proposedConfiguration->packetType >> 8) & 0xff);
    type[1] = (uint8_t)(proposedConfiguration->packetType & 0xff);

    continuousPacketType = UcbPacketBytesToPacketType(type);

    /* check that a valid continuous output packet has been selected */
    valid &= UcbPacketIsAnOutputPacket(continuousPacketType);

    /* check continuous packet rate */
    if (proposedConfiguration->port1Usage == PRI_UCB_PORT) {
        valid &= CheckContPacketRate(continuousPacketType, proposedConfiguration->port1BaudRate, proposedConfiguration->packetRateDivider);
    } else if (proposedConfiguration->port2Usage == PRI_UCB_PORT) {
        valid &= CheckContPacketRate(continuousPacketType, proposedConfiguration->port2BaudRate, proposedConfiguration->packetRateDivider);
    } else if (proposedConfiguration->port3Usage == PRI_UCB_PORT) {
        valid &= CheckContPacketRate(continuousPacketType, proposedConfiguration->port2BaudRate, proposedConfiguration->packetRateDivider);
    } else if (proposedConfiguration->port4Usage == PRI_UCB_PORT) {
        valid &= CheckContPacketRate(continuousPacketType, proposedConfiguration->port2BaudRate, proposedConfiguration->packetRateDivider);
    } else {
        valid = FALSE;
    }

    /* check port usage - one primary port, one mag port */
    valid &= CheckPortUsage(proposedConfiguration);

    /* check port baud rates */
    valid &= CheckPortBaudRate(proposedConfiguration->port1BaudRate);
    valid &= CheckPortBaudRate(proposedConfiguration->port2BaudRate);
    valid &= CheckPortBaudRate(proposedConfiguration->port3BaudRate);
    valid &= CheckPortBaudRate(proposedConfiguration->port4BaudRate);

    return valid;
}
/* end ValidPortConfiguration */

/******************************************************
* Function name:    checkOrientation
*
* Description:
*   -varifies the integrity of a proposed orientation
*	field.
*
* Trace:
* [SDD_CHK_FIELD_ORIENT<-- SRC_CHECK_ORIENTATION]
* [SDD_INIT_CONFIGURATION_ORIENT_VALID <-- SRC_CHECK_ORIENTATION]
*
* Input parameters:
*   orientation: the value of the orientation field to verify.
*
* Output parameters:
* 	No output parameters
*
* Return value:
*   one for valid orientations, zero otherwise.
*
*  Author: Darren Liccardo, Jan. 2004
*   	   Dong An, 2007, 2008
*******************************************************/
BOOL CheckOrientation (uint16_t orientation)
{
    switch (orientation) {
    case 0:
    case 9:
    case 35:
    case 42:
    case 65:
    case 72:
    case 98:
    case 107:
    case 133:
    case 140:
    case 146:
    case 155:
    case 196:
    case 205:
    case 211:
    case 218:
    case 273:
    case 280:
    case 292:
    case 301:
    case 336:
    case 345:
    case 357:
    case 364:
        return TRUE;
    default:
        return FALSE;
    }
}  /*end CheckOrientation */

/******************************************************
* Function name:    CheckBaroCorrection
*
* Description:
*   -checks if the input baro correction is valid.
*
* Trace:
* [SDD_INIT_CONFIGURATION_DEFAULT_BARO <-- SRC_CHECK_BARO_CORRECTION]
* [SDD_CHK_FIELD_BARO <-- SRC_CHECK_BARO_CORRECTION]
*
* Input parameters:
*   int32_t baroCorrection: the input baro correction.
*
* Output parameters:
* 	None.
*
* Return value:
*   1: valid
*   0: invalid
*
*  Author:  Dong An, 2007, 2008
*******************************************************/
BOOL CheckBaroCorrection(int32_t baroCorrection)
{
    if (baroCorrection >=(BARO_CORRECTION_RANGE_LOW*BARO_CORRECTION_SCALING_IN_EEPROM)
        && baroCorrection <=(BARO_CORRECTION_RANGE_HIGH*BARO_CORRECTION_SCALING_IN_EEPROM))
        return TRUE;
    else return FALSE;
}  /* end CheckBaroCorrection */

/******************************************************
* Function name:    CheckFieldData
*
* Description:
*   - checks if field data has valid values.
*
* Trace:
* [SDD_CHECK_FIELD_DATA <-- SRC_CHECK_FIELD_DATA]
* [SDD_CHK_FIELD_ADDRESS <-- SRC_CHECK_FIELD_DATA]
* [SDD_CHK_FIELD_ORIENT  <-- SRC_CHECK_FIELD_DATA]
* [SDD_CHK_FIELD_BARO   <-- SRC_CHECK_FIELD_DATA]
* [SDD_CHK_FIELD_PKT_01  <-- SRC_CHECK_FIELD_DATA]
* [SDD_CHK_FIELD_PKT_02  <-- SRC_CHECK_FIELD_DATA]
* [SDD_CHK_FIELD_RATE_DIVIDER_01  <-- SRC_CHECK_FIELD_DATA]
* [SDD_CHK_FIELD_RATE_DIVIDER_02  <-- SRC_CHECK_FIELD_DATA]
* [SDD_CHK_FIELD_PORT_USE_01  <-- SRC_CHECK_FIELD_DATA]
* [SDD_CHK_FIELD_PORT_USE_02  <-- SRC_CHECK_FIELD_DATA]
* [SDD_CHK_FIELD_PORT_RATE_01  <-- SRC_CHECK_FIELD_DATA]
* [SDD_CHK_FIELD_PORT_RATE_02  <-- SRC_CHECK_FIELD_DATA]
*
* Input parameters:
*	ConfigurationStruct currentConfiguration:
*							current configuration to modify
*	uint8_t numFields:		number of fields to check
*   uint16_t fieldId []: 		array of field IDs.
*   uint16_t fieldData []:  	array of field data corresponding
*							index-wise to the array of field IDs
*
* Output parameters:
* 	validFields[]           array of fieldId that have been validated.
*
* Return value:
*   number of valid fields returned in validFields[]
*******************************************************/
static uint8_t CheckFieldData (ConfigurationStruct *currentConfiguration, uint8_t numFields, uint16_t fieldId [], uint16_t fieldData [], uint16_t validFields [])
{
    BOOL packetTypeChanged = FALSE;

    BOOL packetRateDividerChanged = FALSE;

    BOOL port1UsageChanged = FALSE;
    BOOL port2UsageChanged = FALSE;
    BOOL port3UsageChanged = FALSE;
    BOOL port4UsageChanged = FALSE;

    BOOL port1BaudChanged = FALSE;
    BOOL port2BaudChanged = FALSE;
    BOOL port3BaudChanged = FALSE;
    BOOL port4BaudChanged = FALSE;

    /* index for stepping through proposed configuration fields */
    uint8_t fieldIndex = 0;

    /* index for building valid return array */
    uint8_t validFieldIndex   = 0;

    /* working packet type byte buffer */
    uint8_t type [UCB_PACKET_TYPE_LENGTH];

    UcbPacketTypeEnum continuousPacketType;

    ConfigurationStruct proposedPortConfig;

    /* copy current configuration - for testing validity of port configuration only */
    proposedPortConfig = *currentConfiguration;

    /* update new field settings in proposed configuration */
    for (fieldIndex = 0; fieldIndex < numFields; ++fieldIndex) {
        if ((fieldId[fieldIndex] >= LOWER_CONFIG_ADDR_BOUND) &&
            (fieldId[fieldIndex] <= UPPER_CONFIG_ADDR_BOUND)) {
            /* parse field ID and, if applicable, check using respective function (not all fields require this) */
            switch (fieldId[fieldIndex]) {
            case PACKET_TYPE_FIELD_ID: {
                    /* get enum for requested continuous packet type */
                    type[0] = (uint8_t)((fieldData[fieldIndex] >> 8) & 0xff);
                    type[1] = (uint8_t)(fieldData[fieldIndex] & 0xff);

                    continuousPacketType = UcbPacketBytesToPacketType(type);

                    /* check that a valid continuous output packet has been selected */
                    if (UcbPacketIsAnOutputPacket(continuousPacketType) == TRUE) {
                        packetTypeChanged = TRUE;

                        proposedPortConfig.packetType = fieldData[fieldIndex];
                    }

                    break;
                }
            case PACKET_RATE_DIVIDER_FIELD_ID: {
                    packetRateDividerChanged = TRUE;

                    proposedPortConfig.packetRateDivider = fieldData[fieldIndex];

                    break;
                }
            case PORT_1_USAGE_FIELD_ID: {
                    port1UsageChanged = TRUE;

                    proposedPortConfig.port1Usage = fieldData[fieldIndex];

                    break;
                }
            case PORT_2_USAGE_FIELD_ID: {
                    port2UsageChanged = TRUE;

                    proposedPortConfig.port2Usage = fieldData[fieldIndex];

                    break;
                }
            case PORT_3_USAGE_FIELD_ID: {
                    port3UsageChanged = TRUE;

                    proposedPortConfig.port3Usage = fieldData[fieldIndex];

                    break;
                }
            case PORT_4_USAGE_FIELD_ID: {
                    port4UsageChanged = TRUE;

                    proposedPortConfig.port4Usage = fieldData[fieldIndex];

                    break;
                }
            case PORT_1_BAUD_RATE_FIELD_ID: {
                    port1BaudChanged = TRUE;

                    proposedPortConfig.port1BaudRate = fieldData[fieldIndex];

                    break;
                }
            case PORT_2_BAUD_RATE_FIELD_ID: {
                    port2BaudChanged = TRUE;

                    proposedPortConfig.port2BaudRate = fieldData[fieldIndex];

                    break;
                }
            case PORT_3_BAUD_RATE_FIELD_ID: {
                    port3BaudChanged = TRUE;

                    proposedPortConfig.port3BaudRate = fieldData[fieldIndex];

                    break;
                }
            case PORT_4_BAUD_RATE_FIELD_ID: {
                    port4BaudChanged = TRUE;

                    proposedPortConfig.port4BaudRate = fieldData[fieldIndex];

                    break;
                }
            case ORIENTATION_FIELD_ID: {
                    if (CheckOrientation(fieldData[fieldIndex]) == TRUE) {
                        /* update proposed configuration */
                        currentConfiguration->orientation.all = fieldData[fieldIndex];

                        /* add to valid list */
                        validFields[validFieldIndex++] = fieldId[fieldIndex];
                    }

                    break;
                }
            case BARO_CORRECTION_FIELD_ID: {
                    if (CheckBaroCorrection(Int16ToInt32((int16_t)fieldData[fieldIndex])) == TRUE) {
                        /* update proposed configuration */
                        currentConfiguration->baroCorrection = (int16_t)(fieldData[fieldIndex]);

                        /* add to valid list */
                        validFields[validFieldIndex++] = fieldId[fieldIndex];
                    }

                    break;
                }
            default: {
                    ((uint16_t *)currentConfiguration)[(fieldId[fieldIndex])] = fieldData[fieldIndex];

                    /* add to valid list */
                    validFields[validFieldIndex++] = fieldId[fieldIndex];

                    break;
                }
            }
        }
    }

    /* check proposed port configuration field settings (order/priority matters!) */
    if ((port1UsageChanged == TRUE) ||
        (port2UsageChanged == TRUE) ||
        (port3UsageChanged == TRUE) ||
        (port4UsageChanged == TRUE)) {
        if (ValidPortConfiguration(&proposedPortConfig) == TRUE) {
            portConfigurationChanged = TRUE;

            /* add all relevant fields to valid list */

            if (packetTypeChanged == TRUE) {
                currentConfiguration->packetType = proposedPortConfig.packetType;
                validFields[validFieldIndex++]   = PACKET_TYPE_FIELD_ID;
            }

            if (packetRateDividerChanged == TRUE) {
                currentConfiguration->packetRateDivider = proposedPortConfig.packetRateDivider;
                validFields[validFieldIndex++]          = PACKET_RATE_DIVIDER_FIELD_ID;
            }

            if (port1UsageChanged == TRUE) {
                currentConfiguration->port1Usage = proposedPortConfig.port1Usage;
                validFields[validFieldIndex++]   = PORT_1_USAGE_FIELD_ID;
            }

            if (port2UsageChanged == TRUE) {
                currentConfiguration->port2Usage = proposedPortConfig.port2Usage;
                validFields[validFieldIndex++]   = PORT_2_USAGE_FIELD_ID;
            }

            if (port3UsageChanged == TRUE) {
                currentConfiguration->port3Usage = proposedPortConfig.port3Usage;
                validFields[validFieldIndex++]   = PORT_3_USAGE_FIELD_ID;
            }


            if (port4UsageChanged == TRUE) {
                currentConfiguration->port4Usage = proposedPortConfig.port4Usage;
                validFields[validFieldIndex++]   = PORT_4_USAGE_FIELD_ID;
            }

            if (port1BaudChanged == TRUE) {
                currentConfiguration->port1BaudRate = proposedPortConfig.port1BaudRate;
                validFields[validFieldIndex++]      = PORT_1_BAUD_RATE_FIELD_ID;
            }

            if (port2BaudChanged == TRUE) {
                currentConfiguration->port2BaudRate = proposedPortConfig.port2BaudRate;
                validFields[validFieldIndex++]      = PORT_2_BAUD_RATE_FIELD_ID;
            }

            if (port3BaudChanged == TRUE) {
                currentConfiguration->port3BaudRate = proposedPortConfig.port3BaudRate;
                validFields[validFieldIndex++]      = PORT_3_BAUD_RATE_FIELD_ID;
            }

            if (port4BaudChanged == TRUE) {
                currentConfiguration->port4BaudRate = proposedPortConfig.port4BaudRate;
                validFields[validFieldIndex++]      = PORT_4_BAUD_RATE_FIELD_ID;
            }
        }
    } else if ((port1BaudChanged == TRUE) ||
               (port2BaudChanged == TRUE) ||
               (port3BaudChanged == TRUE) ||
               (port4BaudChanged == TRUE)) {
        if (ValidPortConfiguration(&proposedPortConfig) == TRUE) {
            portConfigurationChanged = TRUE;

            /* add configuration changes to proposed configuration and add all relevant fields to valid list */

            if (packetTypeChanged == TRUE) {
                currentConfiguration->packetType = proposedPortConfig.packetType;
                validFields[validFieldIndex++]   = PACKET_TYPE_FIELD_ID;
            }

            if (packetRateDividerChanged == TRUE) {
                currentConfiguration->packetRateDivider = proposedPortConfig.packetRateDivider;
                validFields[validFieldIndex++]          = PACKET_RATE_DIVIDER_FIELD_ID;
            }

            if (port1BaudChanged == TRUE) {
                currentConfiguration->port1BaudRate = proposedPortConfig.port1BaudRate;
                validFields[validFieldIndex++]      = PORT_1_BAUD_RATE_FIELD_ID;
            }

            if (port2BaudChanged == TRUE) {
                currentConfiguration->port2BaudRate = proposedPortConfig.port2BaudRate;
                validFields[validFieldIndex++]      = PORT_2_BAUD_RATE_FIELD_ID;
            }

            if (port3BaudChanged == TRUE) {
                currentConfiguration->port3BaudRate = proposedPortConfig.port3BaudRate;
                validFields[validFieldIndex++]      = PORT_3_BAUD_RATE_FIELD_ID;
            }

            if (port4BaudChanged == TRUE) {
                currentConfiguration->port4BaudRate = proposedPortConfig.port4BaudRate;
                validFields[validFieldIndex++]      = PORT_4_BAUD_RATE_FIELD_ID;
            }

        }
    } else if ((packetTypeChanged == TRUE) ||
               (packetRateDividerChanged == TRUE)) {
        /* port usage or baud settings haven't changed, DON'T indicate port configuration change */

        if (ValidPortConfiguration(&proposedPortConfig) == TRUE) {

            if (packetTypeChanged == TRUE) {
                currentConfiguration->packetType = proposedPortConfig.packetType;
                validFields[validFieldIndex++]   = PACKET_TYPE_FIELD_ID;
            }

            if (packetRateDividerChanged == TRUE) {
                currentConfiguration->packetRateDivider = proposedPortConfig.packetRateDivider;
                validFields[validFieldIndex++]          = PACKET_RATE_DIVIDER_FIELD_ID;
            }
        }
    }

    return validFieldIndex;
}
/* end CheckFieldData */

/******************************************************
* Function name:    CheckRamFieldData
*
* Description:
*   - checks if field data has valid values.
*
* Trace:
* [SDD_CHK_RAM_FIELDS_02 <-- SRC_CHECK_RAM_FIELD_DATA]
*
* Input parameters:
*	uint8_t numFields:		number of fields to check
*   uint16_t fieldId []: 		array of field IDs.
*   uint16_t fieldData []:  	array of field data corresponding
*							index-wise to the array of field IDs
*
* Output parameters:
* 	validFields[]           array of fieldId that have been validated.
*
* Return value:
*   number of valid fields returned in validFields[]
*******************************************************/
uint8_t CheckRamFieldData (uint8_t numFields, uint16_t fieldId [], uint16_t fieldData [], uint16_t validFields [])
{
    /* copy current RAM configuration */
    proposedRamConfiguration = gConfiguration;

    return CheckFieldData(&proposedRamConfiguration, numFields, fieldId, fieldData, validFields);
}

/******************************************************
* Function name:    CheckEepromFieldData
*
* Description:
*   - checks if field data has valid values.
*
* Trace:
* [SDD_CHK_EEPROM_FIELDS_02 <-- SRC_CHECK_EEPROM_FIELD_DATA]
*
* Input parameters:
*	uint8_t numFields:		number of fields to check
*   uint16_t fieldId []: 		array of field IDs.
*   uint16_t fieldData []:  	array of field data corresponding
*							index-wise to the array of field IDs
*
* Output parameters:
* 	validFields[]           array of fieldId that have been validated.
*
* Return value:
*   number of valid fields returned in validFields[]
*******************************************************/
uint8_t CheckEepromFieldData (uint8_t numFields, uint16_t fieldId [], uint16_t fieldData [], uint16_t validFields [])
{
    /* copy current EEPROM configuration */
    readEEPROMWords(0, sizeof(proposedEepromConfiguration), (uint16_t *)&proposedEepromConfiguration);

    return CheckFieldData(&proposedEepromConfiguration, numFields, fieldId, fieldData, validFields);
}

/*********************************************************************************
* Function name:	SetFieldData
*
* Description:  Perform config changes required by the SF command
*
* Trace:
*	[SDD_UCB_SET_FIELD_01 <-- SRC_SET_FIELD_DATA]
*	[SDD_UCB_SET_FIELD_02 <-- SRC_SET_FIELD_DATA]
*
* Input parameters:
*
* Output parameters:
*
* Return value:
*********************************************************************************/
void SetFieldData (void)
{
    /* special handling for changing port configuration */
    if (portConfigurationChanged == TRUE) {
        /* wait for all data in output buffers to be completely sent */
        ExternPortWaitOnTxIdle();

        /* assign proposed configuration to actual configuration */
        gConfiguration = proposedRamConfiguration;

        /* reset communication interface */
        ExternPortInit();

        /* reset to resync with IOUP  */
        gIOUPSync = NOSYNC;
        ioupDataStart = 0;
#if DMU380_NOT_WORKING
        /* turn disabled interrupts back on */
        interrupt_init();
#endif // DMU380_NOT_WORKING

        /* reset changed flag */
        portConfigurationChanged = FALSE;
    } else {  /* non-port related configuration change */
        /* assign proposed configuration to actual configuration */
        gConfiguration = proposedRamConfiguration;
    }
}
/* end SetFieldData */

/*********************************************************************************
* Function name:	WriteFieldData
*
* Description:	write the data from the WF command into eeprom
*
* Trace: [SDD_UCB_WRITE_FIELD <-- SRC_WRITE_FIELD_DATA]
*
* Input parameters:
*
* Output parameters:
*
* Return value:
*********************************************************************************/
BOOL WriteFieldData (void)
{
    BOOL success;

    /* write entire proposed configuration back to EEPROM */
    if (writeEEPROMWords(1+LOWER_CONFIG_ADDR_BOUND, NUM_CONFIG_FIELDS, (void *)(&(proposedEepromConfiguration.packetRateDivider))) == 0) {
        success = TRUE;
    } else {
        success = FALSE;
    }

    return success;
}
/* end WriteFieldData */

/******************************************************
* Function name:  appendCorrectedRates
*
* Description:
*   -calculates the algorithm corrected angular
*	rates and formats them for output.
*
* Trace:
* [SDD_APPEND_CORRECTED_RATES <-- SRC_APPEND_CORRECTED_RATES]
*
* Input parameters:
*   packetType: Packet type which should be GF or RF.
*	response: points to the beginning of the packet array.
*   index: response[index] is where data is added.
*
* Output parameters:
* 	No output parameters
*
* Return value:
*   modified index to next avaliable response buffer location.
*
* AUTHOR: Darren Liccardo, Aug. 2005
*         Dong An, 2007,2008
*******************************************************/
uint16_t appendCorrectedRates (char *response, uint16_t index)
{
    int i, tmp;

    for (i=0;i<3;i+=1) {
        tmp = (int)(SCALE_BY_2POW16_OVER_7PI(gAlgorithm.correctedRate[i]));
        response[index++] = (char)((tmp >> 8) & 0xff);
        response[index++] = (char)(tmp & 0xff);
    }

    return index;
}
/* end appendCorrectedRates */

/******************************************************
* Function name:  appendDownRate
*
* Description:
*   -scales down rate and formats it for output.
*
* Trace:
* [SDD_APPEND_DOWN_RATE <-- SRC_APPEND_DOWN_RATE]
*
* Input parameters:
*	response: points to the beginning of the packet array.
*   index: response[index] is where data is added.
*
* Output parameters:
* 	No output parameters
*
* Return value:
*   modified index to next avaliable response buffer location.
*
* AUTHOR: Dong An, 2007,2008
*******************************************************/
uint16_t appendDownRate (char *response, uint16_t index)
{
    int tmp;

    tmp = (int)(SCALE_BY_2POW16_OVER_7PI(gAlgorithm.downRate));
    response[index++] = (char)((tmp >> 8) & 0xff);
    response[index++] = (char)(tmp & 0xff);

    return index;
}
/* end appendDownRate */

/******************************************************
* Function name:  appendRates
*
* Description:
*   -calculates the algorithm corrected angular
*    rates and formats them for output.
*
* Trace:
* [SDD_APPEND_RATES <-- SRC_APPEND_RATES]
*
* Input parameters:
*	response: points to the beginning of the packet array.
*   index: response[index] is where data is added.
*
* Output parameters:
* 	No output parameters
*
* Return value:
*   modified index to next avaliable response buffer location.
*
* AUTHOR: Darren Liccardo, Aug. 2005
*         Dong An, 2007,2008
*******************************************************/
uint16_t appendRates (char *response, uint16_t index)
{
    int i, tmp;

    for (i=0;i<3;i+=1) {
        tmp = (int)(SCALE_BY_2POW16_OVER_7PI(gAlgorithm.scaledSensors[XRATE+i]));
        response[index++] = (char)((tmp >> 8) & 0xff);
        response[index++] = (char)(tmp & 0xff);
    }

    return index;
}
/* end appendRates */

/******************************************************
* Function name:  appendTangentRates
*
* Description:
*   - formats the local level frame angular rates for output.
*
* Trace:
* [SDD_APPEND_TANGENT_RATES <-- SRC_APPEND_TANGENT_RATES]
*
* Input parameters:
*	response: points to the beginning of the packet array.
*   index: response[index] is where data is added.
*
* Output parameters:
* 	No output parameters
*
* Return value:
*   modified index to next avaliable response buffer location.
*
* AUTHOR:
*******************************************************/
uint16_t appendTangentRates (char *response, uint16_t index)
{
    int i, tmp;

    for (i=0;i<3;i+=1) {
        tmp = (int)(SCALE_BY_2POW16_OVER_7PI(gAlgorithm.tangentRates[i]));
        response[index++] = (char)((tmp >> 8) & 0xff);
        response[index++] = (char)(tmp & 0xff);
    }

    return index;
}
/* end appendTangentRates */

/******************************************************
* Function name:  appendAccels
*
* Description:
*   -calculates the algorithm corrected
*	accelerations and formats them for output.
*
* Trace:
* [SDD_APPEND_ACCELS <-- SRC_APPEND_ACCELS]
*
* Input parameters:
*	response: points to the beginning of the packet array.
*   index: response[index] is where data is added.
*
* Output parameters:
* 	No output parameters
*
* Return value:
*   modified index to next avaliable response buffer location.
*
* AUTHOR: Darren Liccardo, Aug. 2005
*         Dong An, 2007,2008
*******************************************************/
uint16_t appendAccels (char *response, uint16_t index)
{
    int i, tmp;

    for (i=0;i<3;i+=1) {
        tmp = (int)(SCALE_BY_2POW16_OVER_20(gAlgorithm.scaledSensors[XACCEL+i]));
        response[index++] = (char)((tmp >> 8) & 0xff);
        response[index++] = (char)(tmp & 0xff);
    }

    return index;
}
/* end appendAccels */

/******************************************************
* Function name:  appendBodyLoLaNaccels
*
* Description:
*   -calculates Body Longitudinal Acceleration, Body Lateral Acceleration,
*    Body Normal Acceleration for ARINC705, and formats them for output.
*
* Trace:
* [SDD_APPEND_ACCELS <-- SRC_APPEND_ACCELS]
*
* Input parameters:
*	response: points to the beginning of the packet array.
*   index: response[index] is where data is added.
*
* Output parameters:
* 	No output parameters
*
* Return value:
*   modified index to next avaliable response buffer location.
*
* AUTHOR: Dong An, 2010
*******************************************************/
uint16_t appendBodyLoLaNaccels (char *response, uint16_t index)
{
    int i, tmp;
    double loLaNaccel[3];

    /* to comply with ARINC */
    for (i=0;i<2;i+=1) {
        loLaNaccel[i]=gAlgorithm.scaledSensors[XACCEL+i];
    }
    loLaNaccel[2]=-(gAlgorithm.scaledSensors[XACCEL+2]+1.0);

    for (i=0;i<3;i+=1) {
        tmp = (int)(SCALE_BY_2POW16_OVER_20(loLaNaccel[i]));
        response[index++] = (char)((tmp >> 8) & 0xff);
        response[index++] = (char)(tmp & 0xff);
    }

    return index;
}
/* end appendBodyLoLaNaccels */

/******************************************************
* Function name:  appendTangentAccels
*
* Description:
*   -calculates Along Heading Acceleration, Cross Heading Acceleration and Vertical Acceleration
*    for ARINC705, and formats them for output.
*
* Trace:
* [SDD_APPEND_ACCELS <-- SRC_APPEND_ACCELS]
*
* Input parameters:
*	response: points to the beginning of the packet array.
*   index: response[index] is where data is added.
*
* Output parameters:
* 	No output parameters
*
* Return value:
*   modified index to next avaliable response buffer location.
*
* AUTHOR:
*
*******************************************************/
uint16_t appendTangentAccels (char *response, uint16_t index)
{
    int i, tmp;
    double aHcHvAccel[3];

    for (i=0;i<3;i+=1) {
        aHcHvAccel[i]=gAlgorithm.tangentAccels[i];
    }
    aHcHvAccel[2]= -(gAlgorithm.tangentAccels[2]+1.0);

    for (i=0;i<3;i+=1) {
        tmp = (int)(SCALE_BY_2POW16_OVER_20(aHcHvAccel[i]));
        response[index++] = (char)((tmp >> 8) & 0xff);
        response[index++] = (char)(tmp & 0xff);
    }

    return index;
}
/* end appendTangentAccels */

/******************************************************
* Function name:  appendTemps
*
* Description:
*   -formats rate and board temperature data for output.
*
* Trace:
* [SDD_APPEND_TEMPS_01 <-- SRC_APPEND_TEMPS]
* [SDD_APPEND_TEMPS_02 <-- SRC_APPEND_TEMPS]
*
* Input parameters:
*	response: points to the beginning of the packet array.
*   index: response[index] is where data is added.
*
* Output parameters:
* 	No output parameters
*
* Return value:
*   modified index to next avaliable response buffer location.
*
* AUTHOR: Darren Liccardo, Dec. 2005
*         Dong An, 2007,2008
*******************************************************/
uint16_t appendTemps (char *response, uint16_t index)
{
    int i, tmp;
    double tmpD[3];

    for (i=0;i<3;i+=1) {
        tmpD[i] =gAlgorithm.scaledSensors[XRTEMP+i];
    }

    for (i=0;i<3;i+=1) {
        if (tmpD[i] >= MAX_TEMP_4_SENSOR_PACKET) {
            tmpD[i] =  MAX_TEMP_4_SENSOR_PACKET;
        }
        if (tmpD[i] <= -MAX_TEMP_4_SENSOR_PACKET) {
            tmpD[i] =  -MAX_TEMP_4_SENSOR_PACKET;
        }
    }

    for (i=0;i<3;i+=1) {
        tmp = (int)(SCALE_BY_2POW16_OVER_200(tmpD[i]));
        response[index++] = (char)((tmp >> 8) & 0xff);
        response[index++] = (char)(tmp & 0xff);
    }

    tmp = (int)(adahrsBitRawSensors[PCBTEMP]);
    response[index++] = (char)((tmp >> 8) & 0xff);
    response[index++] = (char)(tmp & 0xff);

    return index;

}
/*end appendTemps */

/******************************************************
* Function name:  appendAttitudeTrue
*
* Description:
*   -calculates roll, pitch, and true heading and
*	formats them for output.
*
* Trace:
* [SDD_APPEND_ATTITUDE_TRUE <-- SRC_APPEND_ATTITUDE_TRUE]
*
* Input parameters:
*	response: points to the beginning of the packet array.
*   index: response[index] is where data is added.
*
* Output parameters:
* 	No output parameters
*
* Return value:
*   modified index to next avaliable response buffer location.
*
* AUTHOR: Darren Liccardo, Aug. 2005
*         Dong An, 2007,2008
*******************************************************/
uint16_t appendAttitudeTrue (char *response, uint16_t index)
{
    int i,tmp;

    for (i=0;i<3;i+=1) {
        tmp = (int)(SCALE_BY_2POW16_OVER_2PI(gAlgorithm.attitude[i]));
        response[index++] = (char)((tmp >> 8) & 0xff);
        response[index++] = (char)(tmp & 0xff);
    }

    return index;
}
/* end appendAttitudeTrue */

/******************************************************
* Function name:  appendAirDataSolution
*
* Description:
*   -scales air data solution and
*	formats them for output.
*
* Trace:
* [SDD_APPEND_AIR_DATA_SOLUTION <-- SRC_APPEND_AIR_DATA_SOLUTION]
*
* Input parameters:
*	response: points to the beginning of the packet array.
*   index: response[index] is where data is added.
*
* Output parameters:
* 	No output parameters
*
* Return value:
*   modified index to next avaliable response buffer location.
*
* AUTHOR: Dong An, 2007,2008
*******************************************************/
uint16_t appendAirDataSolution (char *response, uint16_t index)
{
    unsigned int tmp=0;
    int tmp1=0;

    tmp =  (unsigned int)(SCALE_BY_2POW16_OVER_512(algorithmAirData.airDataSolution[TAS]));
    response[index++] = (char)((tmp >> 8) & 0xff);
    response[index++] = (char)(tmp & 0xff);

    tmp =  (unsigned int)(SCALE_BY_2POW16_OVER_512(algorithmAirData.airDataSolution[CAS]));
    response[index++] = (char)((tmp >> 8) & 0xff);
    response[index++] = (char)(tmp & 0xff);

    tmp1 =  (int)(MAXINT32_20BIT_OVER131072M * algorithmAirData.airDataSolution[CALTI]);
    response[index++] = (char)((tmp1 >> 24) & 0xff);
    response[index++] = (char)((tmp1 >> 16) & 0xff);
    response[index++] = (char)((tmp1 >> 8) & 0xff);
    response[index++] = (char)(tmp1 & 0xff);

    tmp1 =  (int)(MAXINT32_20BIT_OVER131072M *algorithmAirData.airDataSolution[PALTI]);
    response[index++] = (char)((tmp1 >> 24) & 0xff);
    response[index++] = (char)((tmp1 >> 16) & 0xff);
    response[index++] = (char)((tmp1 >> 8) & 0xff);
    response[index++] = (char)(tmp1 & 0xff);

    tmp1 =  (int)(MAXINT32_20BIT_OVER131072M*algorithmAirData.airDataSolution[DALTI]);
    response[index++] = (char)((tmp1 >> 24) & 0xff);
    response[index++] = (char)((tmp1 >> 16) & 0xff);
    response[index++] = (char)((tmp1 >> 8) & 0xff);
    response[index++] = (char)(tmp1 & 0xff);

    tmp =  (unsigned int)(SCALE_BY_2POW16_OVER_512(algorithmAirData.airDataSolution[ALTIRATE_F]));
    response[index++] = (char)((tmp >> 8) & 0xff);
    response[index++] = (char)(tmp & 0xff);

    tmp =  (unsigned int)(SCALE_BY_2POW16_OVER_2PI(algorithmAirData.airDataSolution[SLIPSKID]));
    response[index++] = (char)((tmp >> 8) & 0xff);
    response[index++] = (char)(tmp & 0xff);

    tmp =  (unsigned int)(SCALE_BY_2POW16_OVER_200(algorithmAirData.airDataSolution[OATC]));
    response[index++] = (char)((tmp >> 8) & 0xff);
    response[index++] = (char)(tmp & 0xff);

    return index;
}
/*end appendAirDataSolution */

/******************************************************
* Function name:  appendInertialCounts
*
* Description:
*   -formats accels and rates for output.
*
* Trace:
* [SDD_APPEND_INERTIAL_COUNTS <-- SRC_APPEND_INERTIAL_COUNTS]
*
* Input parameters:
*	response: points to the beginning of the packet array.
*   index: response[index] is where data is added.
*
* Output parameters:
* 	No output parameters
*
* Return value:
*   modified index to next avaliable response buffer location.
*
* AUTHOR: Dong An, 2007,2008
*******************************************************/
uint16_t appendInertialCounts (char *response, uint16_t index)
{
    int i;

    for (i=0;i<6;i+=1) {
        response[index++] = (char)((rawInertialSensors[i] >> 24) & 0xff);
        response[index++] = (char)((rawInertialSensors[i] >> 16) & 0xff);
        response[index++] = (char)((rawInertialSensors[i] >> 8) & 0xff);
        response[index++] = (char)(rawInertialSensors[i] & 0xff);
    }

    return index;
}
/* end appendInertialCounts */

/******************************************************
* Function name:  appendMagnetometerCounts
*
* Description:
*   -formats magnetometers for output.
*
* Trace:
* [SDD_APPEND_MAGNETOMETER_COUNTS <-- SRC_APPEND_MAGNETOMETER_COUNTS]
*
* Input parameters:
*	response: points to the beginning of the packet array.
*   index: response[index] is where data is added.
*
* Output parameters:
* 	No output parameters
*
* Return value:
*   modified index to next avaliable response buffer location.
*
* AUTHOR: Joe Motyka, 2013
*******************************************************/
uint16_t appendMagnetometerCounts (char *response, uint16_t index)
{
    for( int i=0; i<3; i++ )
    {
        response[index++] = (char)((rawInertialSensors[XMAG+i] >> 24) & 0xff);
        response[index++] = (char)((rawInertialSensors[XMAG+i] >> 16) & 0xff);
        response[index++] = (char)((rawInertialSensors[XMAG+i] >> 8) & 0xff);
        response[index++] = (char)(rawInertialSensors[XMAG+i] & 0xff);
    }

    return index;
}
/* end appendMagnetometerCounts */

/******************************************************
* Function name:  appendAllTempCounts
*
* Description:
*   -formats the measured temperature
*	 counts of gyro, accel and PCB for output.
*
* Trace:
* [SDD_APPEND_ALL_TEMP_COUNTS <-- SRC_APPEND_ALL_TEMP_COUNTS]
*
* Input parameters:
*	response: points to the beginning of the packet array.
*   index: response[index] is where data is added.
*
* Output parameters:
* 	No output parameters
*
* Return value:
*   modified index to next avaliable response buffer location.
*
* AUTHOR: Darren Liccardo, Oct. 2005
*         Dong An, 2007,2008
*******************************************************/
uint16_t appendAllTempCounts (char *response, uint16_t index)
{
    int i;

    for (i=XATEMP;i<XATEMP+6;i+=1) {
        response[index++] = (char)((rawInertialSensors[i] >> 24) & 0xff);
        response[index++] = (char)((rawInertialSensors[i] >> 16) & 0xff);
        response[index++] = (char)((rawInertialSensors[i] >> 8) & 0xff);
        response[index++] = (char)(rawInertialSensors[i] & 0xff);
    }

    response[index++] = 0;
    response[index++] = 0;

    response[index++] = (char)((adahrsBitRawSensors[PCBTEMP] >> 8) & 0xff);
    response[index++] = (char)(adahrsBitRawSensors[PCBTEMP] & 0xff);

    return index;
}
/* end appendAllTempCounts */

/******************************************************
* Function name:  appendAirDataSensorCounts
*
* Description:
*   -formats air data counts for output.
*
* Trace:
* [SDD_APPEND_AIR_DATA_SENSOR_COUNTS <-- SRC_APPEND_AIR_DATA_SENSOR_COUNTS]
*
* Input parameters:
*	response: points to the beginning of the packet array.
*   index: response[index] is where data is added.
*
* Output parameters:
* 	No output parameters
*
* Return value:
*   modified index to next avaliable response buffer location.
*
* AUTHOR: Dong An, 2007,2008
*******************************************************/
uint16_t appendAirDataSensorCounts (char *response, uint16_t index)
{
    int i;

    for (i=0;i<3;i+=1) {    /* static pressure, dynamic pressure, and OAT */
        response[index++] = (char)((rawAirDataSensors[i] >> 24) & 0xff);
        response[index++] = (char)((rawAirDataSensors[i] >> 16) & 0xff);
        response[index++] = (char)((rawAirDataSensors[i] >> 8) & 0xff);
        response[index++] = (char)(rawAirDataSensors[i] & 0xff);
    }

    return index;
}
/* end appendAirDataSensorCounts */

/******************************************************
* Function name:    appendAirDataSensor
*
* Description:
*   -formats scaled air data sensor data for output.
*
* Trace:
* [SDD_APPEND_AIR_DATA_SENSOR <-- SRC_APPEND_AIR_DATA_SENSOR]
*
* Input parameters:
*	response: points to the beginning of the packet array.
*   index: response[index] is where data is added.
*
* Output parameters:
* 	No output parameters
*
* Return value:
*   modified index to next avaliable response buffer location.
*
* AUTHOR: Dong An, 2007,2008
*******************************************************/
uint16_t appendAirDataSensor (char *response, uint16_t index)
{
    unsigned int tmp;
    double tmpDouble;

    tmpDouble = SCALE_BY_2POW16_OVER_128(algorithmAirData.scaledSensors[SPRESS]);
    tmp=(unsigned int)(floor(tmpDouble+0.5));  /*simple way to round to the nearest int. no negative pressure*/
    response[index++] = (char)((tmp >> 8) & 0xff);
    response[index++] = (char)(tmp & 0xff);
    tmpDouble = SCALE_BY_2POW16_OVER_64(algorithmAirData.scaledSensors[DPRESS]);
    tmp=(unsigned int)(floor(tmpDouble+0.5));  /*simple way to round to the nearest int. no negative pressure*/
    response[index++] = (char)((tmp >> 8) & 0xff);
    response[index++] = (char)(tmp & 0xff);

    return index;

}
/* end appendAirDataSensor */

/******************************************************
* Function name:    appendAirDataTemp
*
* Description:
*   -scales and formats the temperature data for
*	air data sensor data for output.
*
* Trace:
* [SDD_APPEND_AIR_DATA_TEMP <-- SRC_APPEND_AIR_DATA_TEMP]
*
* Input parameters:
*	response: points to the beginning of the packet array.
*   index: response[index] is where data is added.
*
* Output parameters:
* 	No output parameters
*
* Return value:
*   modified index to next avaliable response buffer location.
*
* AUTHOR: Dong An, 2007,2008
*******************************************************/
uint16_t appendAirDataTemp (char *response, uint16_t index)
{
    unsigned int tmp;

    tmp = (unsigned int)(SCALE_BY_2POW16_OVER_200(algorithmAirData.scaledSensors[SPRESSTEMP]));
    response[index++] = (char)((tmp >> 8) & 0xff);
    response[index++] = (char)(tmp & 0xff);
    tmp = (unsigned int)(SCALE_BY_2POW16_OVER_200(algorithmAirData.scaledSensors[DPRESSTEMP]));
    response[index++] = (char)((tmp >> 8) & 0xff);
    response[index++] = (char)(tmp & 0xff);
    tmp = (unsigned int)(SCALE_BY_2POW16_OVER_200(algorithmAirData.scaledSensors[OAT]));
    response[index++] = (char)((tmp >> 8) & 0xff);
    response[index++] = (char)(tmp & 0xff);

    return index;
}
/* end appendAirDataTemp */

/******************************************************
* Function name:  appendAirSpeedRate
*
* Description:
*   -formats airspeed rate for output.
*
* Trace:
* [SDD_APPEND_AIRSPEED_RATE <-- SRC_APPEND_AIRSPEED_RATE]
*
* Input parameters:
*	response: points to the beginning of the packet array.
*   index: response[index] is where data is added.
*
* Output parameters:
* 	No output parameters
*
* Return value:
*   modified index to next avaliable response buffer location.
*
* AUTHOR: Dong An, 2008
*******************************************************/
uint16_t appendAirSpeedRate (char *response, uint16_t index)
{
    int tmp;

    tmp = (int)(SCALE_BY_2POW16_OVER_200(algorithmAirData.airDataSolution[AIRSPEEDRATE]));
    response[index++] = (char)((tmp >> 8) & 0xff);
    response[index++] = (char)(tmp & 0xff);

    return index;
}
/*end appendAirSpeedRate */

/******************************************************
* Function name:  appendBaroCorrection
*
* Description:
*   -formats baro correction for output.
*
* Trace:
* [SDD_APPEND_BARO_CORRECTION <-- SRC_APPEND_BARO_CORRECTION]
*
* Input parameters:
*	response: points to the beginning of the packet array.
*   index: response[index] is where data is added.
*
* Output parameters:
* 	No output parameters
*
* Return value:
*   modified index to next avaliable response buffer location.
*
* AUTHOR: Dong An,2008
*******************************************************/
uint16_t appendBaroCorrection (char *response, uint16_t index)
{
    int tmp;

    tmp = (int)(algorithmAirData.baroCorrection);
    response[index++] = (char)((tmp >> 8) & 0xff);
    response[index++] = (char)(tmp & 0xff);

    return index;
}
/*end appendBaroCorrection */
