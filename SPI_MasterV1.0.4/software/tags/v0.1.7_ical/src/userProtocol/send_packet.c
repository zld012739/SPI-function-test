/*********************************************************************************
* File name:  			send_packet.c
*
* File description:		Functions for sending serial packets.
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
#include "send_packet.h"
#include "ucb_packet_types.def"
#include "xbowsp_version.h"
#include "xbowsp_init.h"
#include "xbowsp_fields.h"
#include "xbowsp_BITStatus.h"
#include "scaling.h"
#include "sensor_cal.h"
#include "calc_airData.h"

/* indices for IOUP data in adahrsBitRawSensors array*/
#define V_BIT_1P8V_IOUP 	TWOVOLIOUP
#define V_BIT_5VD			FIVEVOLD
#define V_BIT_5VA           FIVEVOLA
#define V_REF_2P5V        	TWOFIVEREFBUF
#define V_BIT_1P8V_DUP      TWOVOLDUP
#define V_IN_SUPPLY         VSUPPLY
#define V_OAT               OAT
#define V_BIT_3P3V          THREEVOL
#define V_BIT_5V_AG         FIVEVOLAG
#define V_STAT_PRESSURE     SPRESS
#define V_STAT_EXCITE       SPRESSTEMP
#define V_BIT_7V            SEVENVOLPWR
#define V_DYN_PRESSURE      DPRESS
#define V_DYN_EXCITE        DPRESSTEMP

/*  magnetometer data not used by algorithm */
static double extMagTemp;

/*********************************************************************************
* Function name:	SendUcbIdentification
*
* Description:	send ID packet
*
* Trace:
*	[SDD_UCB_TX_ID <-- SRC_UCB_TX_ID]
*
* Input parameters:
*
* Output parameters:
*
* Return value:
*********************************************************************************/
static void SendUcbIdentification (ExternPortTypeEnum port, PacketPtrType packetPtr)
{
    uint8_t packetIndex = 0;
    uint8_t stringIndex = 0;

    const char PART_NUMBER_STRING [] = SOFTWARE_PART;

    /* serial number */
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)(((gCalibration.serialNumber >> 24) & 0xff));
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)(((gCalibration.serialNumber >> 16) & 0xff));
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)(((gCalibration.serialNumber >> 8 ) & 0xff));
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)(( gCalibration.serialNumber        & 0xff));

	/* model string */
	while ((stringIndex < N_VERSION_STR) &&
		   (gCalibration.versionString[stringIndex] != 0))
	{
		packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)gCalibration.versionString[stringIndex++];
	}

	/* space between */
	packetPtr.ucbPacketPtr->payload[packetIndex++] = ' ';

	stringIndex = 0;

	/* software part number */
	while (stringIndex < SOFTWARE_PART_LEN) {
		packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)PART_NUMBER_STRING[stringIndex++];
	}

	/* zero delimiter */
	packetPtr.ucbPacketPtr->payload[packetIndex++] = 0;

	/* return packet length */
	packetPtr.ucbPacketPtr->payloadLength = packetIndex;

	/* send identification packet */
	ExternPortTx(port, packetPtr);
}

/*********************************************************************************
* Function name:	SendUcbVersionData
*
* Description: send VR packet
*
* Trace:
* 	[SDD_UCB_TX_VR <-- SRC_UCB_TX_VR]
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
static void SendUcbVersionData (ExternPortTypeEnum port, PacketPtrType packetPtr)
{
	/* return packet length */
	packetPtr.ucbPacketPtr->payloadLength = UCB_VERSION_DATA_LENGTH;

    /* DUP version data */
	packetPtr.ucbPacketPtr->payload[0] = (uint8_t)dupFMversion.major; 	/* append major version */
	packetPtr.ucbPacketPtr->payload[1] = (uint8_t)dupFMversion.minor;   	/* append minor version */
	packetPtr.ucbPacketPtr->payload[2] = (uint8_t)dupFMversion.patch;    	/* append patch */
	packetPtr.ucbPacketPtr->payload[3] = (uint8_t)dupFMversion.stage;    	/* append stage */
	packetPtr.ucbPacketPtr->payload[4] = (uint8_t)dupFMversion.build;    	/* append build number */

	/* send version data packet */
	ExternPortTx(port, packetPtr);
}

/*********************************************************************************
* Function name:	SendUcbVersionAllData
*
* Description:	Send VA packet
*
* Trace:
*	[SDD_UCB_TX_VA <-- SRC_UCB_TX_VA]
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
static void SendUcbVersionAllData (ExternPortTypeEnum port, PacketPtrType packetPtr)
{
    uint8_t packetIndex = 0;

	/* DUP version data */
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)dupFMversion.major; 	/* append major version */
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)dupFMversion.minor;    /* append minor version */
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)dupFMversion.patch;    /* append patch */
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)dupFMversion.stage;    /* append stage */
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)dupFMversion.build;    /* append build number */

    /* IOUP version data */
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)ioupFMversion.major;   /* append major version */
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)ioupFMversion.minor;   /* append minor version */
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)ioupFMversion.patch;   /* append patch */
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)ioupFMversion.stage;   /* append stage */
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)ioupFMversion.build;   /* append build number */

    /* boot version data */
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)bootFMversion.major;   /* append major version */
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)bootFMversion.minor;   /* append minor version */
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)bootFMversion.patch;   /* append patch */
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)bootFMversion.stage;   /* append stage */
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)bootFMversion.build;	/* append build number */

	/* return packet length */
	packetPtr.ucbPacketPtr->payloadLength = packetIndex;

	/* send version all data packet */
	ExternPortTx(port, packetPtr);
}

/*********************************************************************************
* Function name:	SendUcbAngle3
*
* Description:	Send A3 packet
*
* Trace:
*	[SDD_UCB_TX_A3 <-- SRC_UCB_TX_A3]
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
static void SendUcbAngle3(ExternPortTypeEnum port, PacketPtrType packetPtr)
{
	uint16_t packetIndex = 0;

	/* set packet length */
	packetPtr.ucbPacketPtr->payloadLength = UCB_ANGLE_3_LENGTH;

	/* add roll angle, pitch angle, magnetometer yaw angle */
	packetIndex = appendAttitudeTrue((char *)packetPtr.ucbPacketPtr->payload, packetIndex);

	/* add X-angular rate, Y-angular rate, Z-angular rate */
	packetIndex = appendCorrectedRates((char *)packetPtr.ucbPacketPtr->payload, packetIndex);

	/* add down rate */
	packetIndex = appendDownRate((char *)packetPtr.ucbPacketPtr->payload, packetIndex);

	/* add X-accelerometer, Y-accelerometer, Z-accelerometer */
	packetIndex = appendAccels((char *)packetPtr.ucbPacketPtr->payload, packetIndex);

	/* add air data solution */
	packetIndex = appendAirDataSolution((char *)packetPtr.ucbPacketPtr->payload, packetIndex);

	/* add air speed */
	packetIndex = appendAirSpeedRate((char *)packetPtr.ucbPacketPtr->payload, packetIndex);

	/* add barometer correction */
	packetIndex = appendBaroCorrection((char *)packetPtr.ucbPacketPtr->payload, packetIndex);

	/* add timer */
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((gAlgorithm.timer >> 24) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((gAlgorithm.timer >> 16) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((gAlgorithm.timer >> 8 ) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)( gAlgorithm.timer        & 0xff);

#define A3_TOP_BIT_STATUS	(BITFAIL_MASTERFAIL | BITMODE_ALGOINIT      | BITFAIL_ATTITUDE | BITFAIL_HEADING | \
						 	 BITFAIL_AIRDATA    | BITFAIL_MAGALIGN      | BITFAIL_OATFAIL  | BITMODE_VGMODE  | \
						 	 BITSTAT_TOPALL     | BITSTAT_REMOTEMAGFAIL | BITMODE_OPMODE)

	/* add BIT status */
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((getBITflags(A3_TOP_BIT_STATUS) >> 8) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)( getBITflags(A3_TOP_BIT_STATUS)       & 0xff);

	/* send Angle 3 packet */
	ExternPortTx(port, packetPtr);
}

/*********************************************************************************
* Function name:	SendUcbAngle5
*
* Description: send A5 packet
*
* Trace:
*	[SDD_UCB_TX_A5 <-- SRC_UCB_TX_A5]
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
static void SendUcbAngle5 (ExternPortTypeEnum port, PacketPtrType packetPtr)
{
#if DMU380_NOT_WORKING

	uint16_t packetIndex = 0;

	/* set packet length */
	packetPtr.ucbPacketPtr->payloadLength = UCB_ANGLE_5_LENGTH;

    /* add roll angle, pitch angle, magnetometer yaw angle */
	packetIndex = appendAttitudeTrue((char *)packetPtr.ucbPacketPtr->payload, packetIndex);

	/* add X-angular rate, Y-angular rate, Z-angular rate */
	packetIndex = appendRates((char *)packetPtr.ucbPacketPtr->payload, packetIndex);

	/* add X-accelerometer, Y-accelerometer, Z-accelerometer */
	packetIndex = appendAccels((char *)packetPtr.ucbPacketPtr->payload, packetIndex);

	/* add static and dynamic pressure */
	packetIndex = appendAirDataSensor((char *)packetPtr.ucbPacketPtr->payload, packetIndex);

	/* add pressure altitude */
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)(((int)(SCALE_BY_8(algorithmAirData.airDataSolution[PALTI])) >> 24) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)(((int)(SCALE_BY_8(algorithmAirData.airDataSolution[PALTI])) >> 16) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)(((int)(SCALE_BY_8(algorithmAirData.airDataSolution[PALTI])) >> 8 ) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)( (int)(SCALE_BY_8(algorithmAirData.airDataSolution[PALTI]))        & 0xff);

/*** "For flight test only!" ***/
#if 0
	/* add calibrated airspeed */
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)(((int)(SCALE_BY_2POW16_OVER_512(algorithmAirData.airDataSolution[CAS])) >> 8) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)( (int)(SCALE_BY_2POW16_OVER_512(algorithmAirData.airDataSolution[CAS]))       & 0xff);
#else
	/* add true airspeed */
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)(((int)(SCALE_BY_2POW16_OVER_512(algorithmAirData.airDataSolution[TAS])) >> 8) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)( (int)(SCALE_BY_2POW16_OVER_512(algorithmAirData.airDataSolution[TAS]))       & 0xff);
#endif

	/* add external X-mag data */
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)(((int)(SCALE_BY_2POW16_OVER_2(gAlgorithm.ExtMags[0])) >> 8) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)( (int)(SCALE_BY_2POW16_OVER_2(gAlgorithm.ExtMags[0]))       & 0xff);

    /* add external Y-mag data */
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)(((int)(SCALE_BY_2POW16_OVER_2(gAlgorithm.ExtMags[1])) >> 8) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)( (int)(SCALE_BY_2POW16_OVER_2(gAlgorithm.ExtMags[1]))       & 0xff);

	/* add external Z-mag data */
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)(((int)(SCALE_BY_2POW16_OVER_2(gAlgorithm.ExtMags[2])) >> 8) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)( (int)(SCALE_BY_2POW16_OVER_2(gAlgorithm.ExtMags[2]))       & 0xff);

	/* add X-rate temperature */
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)(((int)(SCALE_BY_2POW16_OVER_200(gAlgorithm.scaledSensors[XRTEMP])) >> 8) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)( (int)(SCALE_BY_2POW16_OVER_200(gAlgorithm.scaledSensors[XRTEMP]))       & 0xff);

	/* add Y-rate temperature */
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)(((int)(SCALE_BY_2POW16_OVER_200(gAlgorithm.scaledSensors[YRTEMP])) >> 8) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)( (int)(SCALE_BY_2POW16_OVER_200(gAlgorithm.scaledSensors[YRTEMP]))       & 0xff);

	/* add Z-rate temperature */
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)(((int)(SCALE_BY_2POW16_OVER_200(gAlgorithm.scaledSensors[ZRTEMP])) >> 8) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)( (int)(SCALE_BY_2POW16_OVER_200(gAlgorithm.scaledSensors[ZRTEMP]))       & 0xff);

	/* add X-accelerometer temperature */
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)(((int)(SCALE_BY_2POW16_OVER_200(gAlgorithm.scaledSensors[XATEMP])) >> 8) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)( (int)(SCALE_BY_2POW16_OVER_200(gAlgorithm.scaledSensors[XATEMP]))       & 0xff);

	/* add Y-accelerometer temperature */
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)(((int)(SCALE_BY_2POW16_OVER_200(gAlgorithm.scaledSensors[YATEMP])) >> 8) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)( (int)(SCALE_BY_2POW16_OVER_200(gAlgorithm.scaledSensors[YATEMP]))       & 0xff);

	/* add Z-accelerometer temperature */
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)(((int)(SCALE_BY_2POW16_OVER_200(gAlgorithm.scaledSensors[ZATEMP])) >> 8) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)( (int)(SCALE_BY_2POW16_OVER_200(gAlgorithm.scaledSensors[ZATEMP]))       & 0xff);

	/* add static pressure temperature counts */
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((rawAirDataSensors[SPRESSTEMP] >> 24) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((rawAirDataSensors[SPRESSTEMP] >> 16) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((rawAirDataSensors[SPRESSTEMP] >> 8 ) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)( rawAirDataSensors[SPRESSTEMP]        & 0xff);

	/* add dynamic pressure temperature counts */
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((rawAirDataSensors[DPRESSTEMP] >> 24) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((rawAirDataSensors[DPRESSTEMP] >> 16) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((rawAirDataSensors[DPRESSTEMP] >> 8 ) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)( rawAirDataSensors[DPRESSTEMP]        & 0xff);

	/* add outside air temperature */
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)(((int)(SCALE_BY_2POW16_OVER_200(algorithmAirData.scaledSensors[OAT])) >> 8) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)( (int)(SCALE_BY_2POW16_OVER_200(algorithmAirData.scaledSensors[OAT]))       & 0xff);

	/* add timer */
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((gAlgorithm.timer >> 24) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((gAlgorithm.timer >> 16) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((gAlgorithm.timer >> 8 ) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)( gAlgorithm.timer        & 0xff);

	/* add BIT status */
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((getTopERROR() >> 8) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)( getTopERROR()       & 0xff);

	/* send Angle 5 packet */
	ExternPortTx(port, packetPtr);
#endif // DMU380_NOT_WORKING

}

/*********************************************************************************
* Function name:	SendUcbAngleU
*
* Description:	send AU packet (typical output packet used in 525 configuration)
*
* Trace:
*	[SDD_UCB_TX_AU <-- SRC_UCB_TX_AU]
*
* Input parameters:
*
* Output parameters:
*
* Return value:
*********************************************************************************/
static void SendUcbAngleU (ExternPortTypeEnum port, PacketPtrType packetPtr)
{
	uint16_t packetIndex = 0;

	Crc32Type payloadCrc;

	/* set packet length */
	packetPtr.ucbPacketPtr->payloadLength = UCB_ANGLE_U_LENGTH;

	/* add roll angle, pitch angle, magnetometer yaw angle */
	packetIndex = appendAttitudeTrue((char *)packetPtr.ucbPacketPtr->payload, packetIndex);

	/* add X-angular rate, Y-angular rate, Z-angular rate */
	packetIndex = appendCorrectedRates((char *)packetPtr.ucbPacketPtr->payload, packetIndex);

	/* add X-accelerometer, Y-accelerometer, Z-accelerometer */
	packetIndex = appendAccels((char *)packetPtr.ucbPacketPtr->payload, packetIndex);

	/* add tangent X-rate, tangent Y-rate, tangent Z-rate */
	packetIndex = appendTangentRates((char *)packetPtr.ucbPacketPtr->payload, packetIndex);

	/* add tangent X-accelerometer, tangent Y-accelerometer, and tangent Z-accelerometer */
	packetIndex = appendTangentAccels((char *)packetPtr.ucbPacketPtr->payload, packetIndex);

	/* add compass heading */
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((((int)(SCALE_BY_2POW16_OVER_2PI(gAlgorithm.compassHeading))) >> 8) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)(( (int)(SCALE_BY_2POW16_OVER_2PI(gAlgorithm.compassHeading)))       & 0xff);

	/* add timer */
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((gAlgorithm.timer >> 24) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((gAlgorithm.timer >> 16) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((gAlgorithm.timer >> 8)  & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)( gAlgorithm.timer        & 0xff);

#define AU_TOP_BIT_STATUS	(BITFAIL_MASTERFAIL | BITMODE_ALGOINIT      | BITFAIL_ATTITUDE | BITFAIL_HEADING | \
						 	 BITFAIL_AIRDATA    | BITFAIL_MAGALIGN      | BITFAIL_OATFAIL  | BITMODE_VGMODE  | \
						 	 BITSTAT_TOPALL     | BITSTAT_REMOTEMAGFAIL | BITMODE_OPMODE)

	/* add BIT status */
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((getBITflags(AU_TOP_BIT_STATUS) >> 8) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)( getBITflags(AU_TOP_BIT_STATUS)       & 0xff);

	/* compute Universal payload CRC */
	payloadCrc = Crc32(packetPtr.ucbPacketPtr->payload, (UCB_ANGLE_U_LENGTH - CRC_32_LENGTH), CRC_32_INITIAL_SEED);

	/* add Universal payload CRC */
	Crc32TypeToBytes (payloadCrc, &(packetPtr.ucbPacketPtr->payload[packetIndex]));

	/* send Angle U packet */
	ExternPortTx(port, packetPtr);
}
/*********************************************************************************
* Function name:	SendUcbScaled3
*
* Description:	send S3 packet
*
* Trace:
*	[SDD_UCB_TX_S3 <-- SRC_UCB_TX_S3]
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
static void SendUcbScaled3 (ExternPortTypeEnum port, PacketPtrType packetPtr)
{
#if DMU380_NOT_WORKING
	uint16_t packetIndex = 0;

	/* set packet length */
	packetPtr.ucbPacketPtr->payloadLength = UCB_SCALED_3_LENGTH;

    /* add X-accelerometer, Y-accelerometer, Z-accelerometer */
	packetIndex = appendAccels((char *)packetPtr.ucbPacketPtr->payload, packetIndex);

	/* add X-angular rate, Y-angular rate, Z-angular rate */
	packetIndex = appendRates((char *)packetPtr.ucbPacketPtr->payload, packetIndex);

    /* add static and dynamic pressure */
	packetIndex = appendAirDataSensor((char *)packetPtr.ucbPacketPtr->payload, packetIndex);

    /* add pressure altitude */
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)(((int)(SCALE_BY_8(algorithmAirData.airDataSolution[PALTI])) >> 24) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)(((int)(SCALE_BY_8(algorithmAirData.airDataSolution[PALTI])) >> 16) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)(((int)(SCALE_BY_8(algorithmAirData.airDataSolution[PALTI])) >> 8 ) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)( (int)(SCALE_BY_8(algorithmAirData.airDataSolution[PALTI]))        & 0xff);

	/* add calibrated airspeed */
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)(((int)(SCALE_BY_2POW16_OVER_512(algorithmAirData.airDataSolution[CAS])) >> 8) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)( (int)(SCALE_BY_2POW16_OVER_512(algorithmAirData.airDataSolution[CAS]))       & 0xff);

   	/* add external X-accel data */
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)(((int)(SCALE_BY_2POW16_OVER_20(gAlgorithm.ExtAccels[0])) >> 8) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)( (int)(SCALE_BY_2POW16_OVER_20(gAlgorithm.ExtAccels[0]))       & 0xff);

    /* add external Y-accel data */
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)(((int)(SCALE_BY_2POW16_OVER_20(gAlgorithm.ExtAccels[1])) >> 8) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)( (int)(SCALE_BY_2POW16_OVER_20(gAlgorithm.ExtAccels[1]))       & 0xff);

	/* add external Z-accel data */
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)(((int)(SCALE_BY_2POW16_OVER_20(gAlgorithm.ExtAccels[2])) >> 8) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)( (int)(SCALE_BY_2POW16_OVER_20(gAlgorithm.ExtAccels[2]))       & 0xff);

	/* add external X-mag data */
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)(((int)(SCALE_BY_2POW16_OVER_2(gAlgorithm.ExtMags[0])) >> 8) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)( (int)(SCALE_BY_2POW16_OVER_2(gAlgorithm.ExtMags[0]))       & 0xff);

    /* add external Y-mag data */
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)(((int)(SCALE_BY_2POW16_OVER_2(gAlgorithm.ExtMags[1])) >> 8) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)( (int)(SCALE_BY_2POW16_OVER_2(gAlgorithm.ExtMags[1]))       & 0xff);

	/* add external Z-mag data */
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)(((int)(SCALE_BY_2POW16_OVER_2(gAlgorithm.ExtMags[2])) >> 8) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)( (int)(SCALE_BY_2POW16_OVER_2(gAlgorithm.ExtMags[2]))       & 0xff);

	/* add magnetometer temperature */
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)(((int)(SCALE_BY_2POW16_OVER_2(extMagTemp)) >> 8) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)( (int)(SCALE_BY_2POW16_OVER_2(extMagTemp))       & 0xff);

	/* add rate and board temperature */
	packetIndex = appendTemps((char *)packetPtr.ucbPacketPtr->payload, packetIndex);

	/* add static pressure, dynamic pressure, and outside air temperatures */
	packetIndex = appendAirDataTemp((char *)packetPtr.ucbPacketPtr->payload, packetIndex);

	/* add packet counter */
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((gAlgorithm.counter >> 8) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)( gAlgorithm.counter       & 0xff);

	/* increment packet counter */
	++gAlgorithm.counter;

	/* add BIT status */
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((getTopERROR() >> 8) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)( getTopERROR()       & 0xff);

	/* send Scaled 3 packet */
	ExternPortTx(port, packetPtr);
#endif // DMU380_NOT_WORKING
}

/*********************************************************************************
* Function name:	SendUcbTest0
*
* Description:	send T0 packet
*
* Trace:
*	[SDD_UCB_TX_T0 <-- SRC_UCB_TX_T0]
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
static void SendUcbTest0 (ExternPortTypeEnum port, PacketPtrType packetPtr)
{
	uint8_t packetIndex = 0;

	/* set packet length */
	packetPtr.ucbPacketPtr->payloadLength = UCB_TEST_0_LENGTH;

	/* add BIT */
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((getTopERROR() >> 8) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)( getTopERROR()       & 0xff);

	/* add hardware BIT */
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((hardwareBIT() >> 8 ) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)( hardwareBIT()        & 0xff);

	/* add hardware power BIT */
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((hardwarePowerBIT() >> 8 ) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)( hardwarePowerBIT()        & 0xff);

	/* add hardware environmental BIT */
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((hardwareEnvironmentalBIT() >> 8 ) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)( hardwareEnvironmentalBIT()        & 0xff);

	/* add com BIT */
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((comBIT() >> 8 ) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)( comBIT()        & 0xff);

	/* add com serial A BIT */
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((comSerialABIT() >> 8 ) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)( comSerialABIT()        & 0xff);

	/* add com serial B BIT */
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((comSerialBBIT() >> 8 ) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)( comSerialBBIT()        & 0xff);

	/* add software BIT */
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((softwareBIT() >> 8 ) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)( softwareBIT()        & 0xff);

	/* add software algorithm BIT */
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((softwareAlgorithmBIT() >> 8 ) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)( softwareAlgorithmBIT()        & 0xff);

	/* add software data BIT */
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((softwareDataBIT() >> 8 ) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)( softwareDataBIT()        & 0xff);

	/* add hardware status */
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((hardwareStatus() >> 8 ) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)( hardwareStatus()        & 0xff);

	/* add com status */
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((comStatus() >> 8 ) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)( comStatus()        & 0xff);

	/* add software status */
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((softwareStatus() >> 8 ) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)( softwareStatus()        & 0xff);

	/* add sensor status */
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((sensorStatus() >> 8 ) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)( sensorStatus()        & 0xff);

	/* send Test 0 packet */
	ExternPortTx(port, packetPtr);
}

/*********************************************************************************
* Function name:	SendUcbTest1
*
* Description:	send T1 packet
*
* Trace:
*	[SDD_UCB_TX_T1 <-- SRC_UCB_TX_T1]
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
static void SendUcbTest1 (ExternPortTypeEnum port, PacketPtrType packetPtr)
{
	uint8_t packetIndex = 0;

	/* set packet length */
	packetPtr.ucbPacketPtr->payloadLength = UCB_TEST_1_LENGTH;

	/* add BIT */
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((getTopERROR() >> 8 ) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)( getTopERROR()        & 0xff);

	/* add hardware BIT */
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((hardwareBIT() >> 8 ) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)( hardwareBIT()        & 0xff);

	/* add hardware power BIT */
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((hardwarePowerBIT() >> 8 ) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)( hardwarePowerBIT()        & 0xff);

	/* add hardware environmental BIT */
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((hardwareEnvironmentalBIT() >> 8 ) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)( hardwareEnvironmentalBIT()        & 0xff);

	/* add hardware sensor BIT */
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((hardwareSensorBIT() >> 8 ) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)( hardwareSensorBIT()        & 0xff);

	/* add hardware internal comm BIT */
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((hwInternalCommBIT() >> 8 ) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)( hwInternalCommBIT()        & 0xff);

	/* add com BIT */
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((comBIT() >> 8 ) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)( comBIT()        & 0xff);

	/* add com serial A BIT */
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((comSerialABIT() >> 8 ) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)( comSerialABIT()        & 0xff);

	/* add com serial B BIT */
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((comSerialBBIT() >> 8 ) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)( comSerialBBIT()        & 0xff);

	/* add software BIT */
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((softwareBIT() >> 8 ) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)( softwareBIT()        & 0xff);

	/* add software algorithm BIT */
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((softwareAlgorithmBIT() >> 8 ) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)( softwareAlgorithmBIT()        & 0xff);

	/* add software data BIT */
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((softwareDataBIT() >> 8 ) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)( softwareDataBIT()        & 0xff);

	/* add hardware status */
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((hardwareStatus() >> 8 ) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)( hardwareStatus()        & 0xff);

	/* add com status */
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((comStatus() >> 8 ) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)( comStatus()        & 0xff);

	/* add software status */
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((softwareStatus() >> 8 ) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)( softwareStatus()        & 0xff);

	/* add sensor status */
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((sensorStatus() >> 8 ) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)( sensorStatus()        & 0xff);

	/* send Test 1 packet */
	ExternPortTx(port, packetPtr);
}

/*********************************************************************************
* Function name:	SendUcbFactory1
*
* Description:	send F1 packet
*
* Trace:
*	[SDD_UCB_TX_F1 <-- SRC_UCB_TX_F1]
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
static void SendUcbFactory1 (ExternPortTypeEnum port, PacketPtrType packetPtr)
{
	uint16_t packetIndex = 0;

    /* set packet length */
	packetPtr.ucbPacketPtr->payloadLength = UCB_FACTORY_1_LENGTH;

    /* add inertial counts */
	packetIndex = appendInertialCounts((char *)packetPtr.ucbPacketPtr->payload, packetIndex);

	/* add temperature counts */
	packetIndex = appendAllTempCounts((char *)packetPtr.ucbPacketPtr->payload, packetIndex);

	/* add BIT status */
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((getTopERROR() >> 8) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)( getTopERROR()       & 0xff);

	/* send Factory 1 packet */
	ExternPortTx(port, packetPtr);
}

/*********************************************************************************
* Function name:	SendUcbFactory2
*
* Description:	send F2 packet
*
* Trace:
*	[SDD_UCB_TX_F2 <-- SRC_UCB_TX_F2]
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
static void SendUcbFactory2 (ExternPortTypeEnum port, PacketPtrType packetPtr)
{
	uint16_t packetIndex = 0;

    /* set packet length */
	packetPtr.ucbPacketPtr->payloadLength = UCB_FACTORY_2_LENGTH;

    /* add inertial counts */
	packetIndex = appendInertialCounts((char *)packetPtr.ucbPacketPtr->payload, packetIndex);

    /* add magnetometer counts */
	packetIndex = appendMagnetometerCounts((char *)packetPtr.ucbPacketPtr->payload, packetIndex);

	/* add temperature counts */
	packetIndex = appendAllTempCounts((char *)packetPtr.ucbPacketPtr->payload, packetIndex);

	/* add BIT status */
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((getTopERROR() >> 8) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)( getTopERROR()       & 0xff);

	/* send Factory 2 packet */
	ExternPortTx(port, packetPtr);
}

/*********************************************************************************
* Function name:	SendUcbFactory5
*
* Description:	send F5 packet
*
* Trace:
*	[SDD_UCB_TX_F5 <-- SRC_UCB_TX_F5]
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
static void SendUcbFactory5 (ExternPortTypeEnum port, PacketPtrType packetPtr)
{
	uint16_t packetIndex = 0;

    /* set packet length */
	packetPtr.ucbPacketPtr->payloadLength = UCB_FACTORY_5_LENGTH;

    /* add X-accelerometer sensor count */
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((rawInertialSensors[XACCEL] >> 24) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((rawInertialSensors[XACCEL] >> 16) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((rawInertialSensors[XACCEL] >> 8 ) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)( rawInertialSensors[XACCEL]        & 0xff);

    /* add Y-accelerometer sensor count */
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((rawInertialSensors[YACCEL] >> 24) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((rawInertialSensors[YACCEL] >> 16) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((rawInertialSensors[YACCEL] >> 8 ) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)( rawInertialSensors[YACCEL]        & 0xff);

	/* add Z-accelerometer sensor count */
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((rawInertialSensors[ZACCEL] >> 24) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((rawInertialSensors[ZACCEL] >> 16) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((rawInertialSensors[ZACCEL] >> 8 ) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)( rawInertialSensors[ZACCEL]        & 0xff);

    /* add X-rate sensor count */
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((rawInertialSensors[XRATE] >> 24) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((rawInertialSensors[XRATE] >> 16) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((rawInertialSensors[XRATE] >> 8 ) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)( rawInertialSensors[XRATE]        & 0xff);

    /* add Y-rate sensor count */
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((rawInertialSensors[YRATE] >> 24) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((rawInertialSensors[YRATE] >> 16) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((rawInertialSensors[YRATE] >> 8 ) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)( rawInertialSensors[YRATE]        & 0xff);

	/* add Z-rate sensor count */
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((rawInertialSensors[ZRATE] >> 24) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((rawInertialSensors[ZRATE] >> 16) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((rawInertialSensors[ZRATE] >> 8 ) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)( rawInertialSensors[ZRATE]        & 0xff);

    /* append static and dynamic pressure counts and outside air temperature counts */
    packetIndex = appendAirDataSensorCounts((char *)packetPtr.ucbPacketPtr->payload, packetIndex);

	/* add X-accel temperature sensor counts */
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((rawInertialSensors[XATEMP] >> 24) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((rawInertialSensors[XATEMP] >> 16) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((rawInertialSensors[XATEMP] >> 8 ) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)( rawInertialSensors[XATEMP]        & 0xff);

	/* add Y-accel temperature sensor counts */
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((rawInertialSensors[YATEMP] >> 24) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((rawInertialSensors[YATEMP] >> 16) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((rawInertialSensors[YATEMP] >> 8 ) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)( rawInertialSensors[YATEMP]        & 0xff);

	/* add Z-accel temperature sensor counts */
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((rawInertialSensors[ZATEMP] >> 24) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((rawInertialSensors[ZATEMP] >> 16) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((rawInertialSensors[ZATEMP] >> 8 ) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)( rawInertialSensors[ZATEMP]        & 0xff);

	/* add X-rate temperature sensor counts */
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((rawInertialSensors[XRTEMP] >> 24) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((rawInertialSensors[XRTEMP] >> 16) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((rawInertialSensors[XRTEMP] >> 8 ) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)( rawInertialSensors[XRTEMP]        & 0xff);

	/* add Y-rate temperature sensor counts */
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((rawInertialSensors[YRTEMP] >> 24) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((rawInertialSensors[YRTEMP] >> 16) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((rawInertialSensors[YRTEMP] >> 8 ) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)( rawInertialSensors[YRTEMP]        & 0xff);

	/* add Z-rate temperature sensor counts */
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((rawInertialSensors[ZRTEMP] >> 24) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((rawInertialSensors[ZRTEMP] >> 16) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((rawInertialSensors[ZRTEMP] >> 8 ) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)( rawInertialSensors[ZRTEMP]        & 0xff);

	/* add static pressure temperature sensor counts */
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((rawInertialSensors[V_STAT_EXCITE] >> 24) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((rawInertialSensors[V_STAT_EXCITE] >> 16) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((rawInertialSensors[V_STAT_EXCITE] >> 8 ) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)( rawInertialSensors[V_STAT_EXCITE]        & 0xff);

	/* add dynamic pressure temperature sensor counts */
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((rawInertialSensors[V_DYN_EXCITE] >> 24) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((rawInertialSensors[V_DYN_EXCITE] >> 16) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((rawInertialSensors[V_DYN_EXCITE] >> 8 ) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)( rawInertialSensors[V_DYN_EXCITE]        & 0xff);

	/* add BIT status */
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((getTopERROR() >> 8) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)( getTopERROR()       & 0xff);

	/* send Factory 5 packet */
	ExternPortTx(port, packetPtr);
}

/*********************************************************************************
* Function name:	SendUcbFactory6
*
* Description:	send F6 packet
*
* Trace:
*	[SDD_UCB_TX_F6 <-- SRC_UCB_TX_F6]
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
static void SendUcbFactory6 (ExternPortTypeEnum port, PacketPtrType packetPtr)
{
	uint8_t packetIndex = 0;

    /* set packet length */
	packetPtr.ucbPacketPtr->payloadLength = UCB_FACTORY_6_LENGTH;

    /* add 1.8-volt IOUP counts */
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((adahrsBitRawSensors[V_BIT_1P8V_IOUP] >> 24) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((adahrsBitRawSensors[V_BIT_1P8V_IOUP] >> 16) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((adahrsBitRawSensors[V_BIT_1P8V_IOUP] >> 8 ) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)( adahrsBitRawSensors[V_BIT_1P8V_IOUP]        & 0xff);

	/* add 1.8-volt DUP counts */
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((adahrsBitRawSensors[V_BIT_1P8V_DUP] >> 24) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((adahrsBitRawSensors[V_BIT_1P8V_DUP] >> 16) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((adahrsBitRawSensors[V_BIT_1P8V_DUP] >> 8 ) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)( adahrsBitRawSensors[V_BIT_1P8V_DUP]       & 0xff);

	/* add 2.5-volt reference counts */
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((adahrsBitRawSensors[V_REF_2P5V] >> 24) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((adahrsBitRawSensors[V_REF_2P5V] >> 16) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((adahrsBitRawSensors[V_REF_2P5V] >> 8 ) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)( adahrsBitRawSensors[V_REF_2P5V]        & 0xff);

	/* add 5-volt gyro analog counts */
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((adahrsBitRawSensors[V_BIT_5V_AG] >> 24) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((adahrsBitRawSensors[V_BIT_5V_AG] >> 16) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((adahrsBitRawSensors[V_BIT_5V_AG] >> 8)  & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)( adahrsBitRawSensors[V_BIT_5V_AG]        & 0xff);

	/* add 3.3-volt digital counts */
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((adahrsBitRawSensors[V_BIT_3P3V] >> 24) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((adahrsBitRawSensors[V_BIT_3P3V] >> 16) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((adahrsBitRawSensors[V_BIT_3P3V] >> 8 ) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)( adahrsBitRawSensors[V_BIT_3P3V]        & 0xff);

    /* add 5-volt digital counts */
    packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((adahrsBitRawSensors[V_BIT_5VD] >> 24) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((adahrsBitRawSensors[V_BIT_5VD] >> 16) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((adahrsBitRawSensors[V_BIT_5VD] >> 8 ) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)( adahrsBitRawSensors[V_BIT_5VD]        & 0xff);

    /* add 5-volt power analog counts */
    packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((adahrsBitRawSensors[V_BIT_5VA] >> 24) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((adahrsBitRawSensors[V_BIT_5VA] >> 16) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((adahrsBitRawSensors[V_BIT_5VA] >> 8 ) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)( adahrsBitRawSensors[V_BIT_5VA]        & 0xff);

    /* add 7-volt  */
    packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((adahrsBitRawSensors[V_BIT_7V] >> 24) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((adahrsBitRawSensors[V_BIT_7V] >> 16) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((adahrsBitRawSensors[V_BIT_7V] >> 8 ) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)( adahrsBitRawSensors[V_BIT_7V]        & 0xff);

	/* add X-accel current counts */
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((adahrsBitRawSensors[XACUR] >> 24) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((adahrsBitRawSensors[XACUR] >> 16) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((adahrsBitRawSensors[XACUR] >> 8 ) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)( adahrsBitRawSensors[XACUR]        & 0xff);

	/* add Y-accel current counts */
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((adahrsBitRawSensors[YACUR] >> 24) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((adahrsBitRawSensors[YACUR] >> 16) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((adahrsBitRawSensors[YACUR] >> 8 ) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)( adahrsBitRawSensors[YACUR]        & 0xff);

	/* add Z-accel current counts */
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((adahrsBitRawSensors[ZACUR] >> 24) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((adahrsBitRawSensors[ZACUR] >> 16) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((adahrsBitRawSensors[ZACUR] >> 8 ) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)( adahrsBitRawSensors[ZACUR]        & 0xff);

	/* add X-rate current counts */
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((adahrsBitRawSensors[XRCUR] >> 24) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((adahrsBitRawSensors[XRCUR] >> 16) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((adahrsBitRawSensors[XRCUR] >> 8 ) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)( adahrsBitRawSensors[XRCUR]        & 0xff);

	/* add Y-rate current counts */
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((adahrsBitRawSensors[YRCUR] >> 24) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((adahrsBitRawSensors[YRCUR] >> 16) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((adahrsBitRawSensors[YRCUR] >> 8 ) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)( adahrsBitRawSensors[YRCUR]        & 0xff);

	/* add Z-rate current counts */
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((adahrsBitRawSensors[ZRCUR] >> 24) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((adahrsBitRawSensors[ZRCUR] >> 16) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((adahrsBitRawSensors[ZRCUR] >> 8 ) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)( adahrsBitRawSensors[ZRCUR]        & 0xff);

	/* add static pressure current counts */
    packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((adahrsBitRawSensors[V_STAT_EXCITE] >> 24) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((adahrsBitRawSensors[V_STAT_EXCITE] >> 16) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((adahrsBitRawSensors[V_STAT_EXCITE] >> 8 ) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)( adahrsBitRawSensors[V_STAT_EXCITE]        & 0xff);

	/* add dynamic pressure current counts */
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((adahrsBitRawSensors[V_DYN_EXCITE] >> 24) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((adahrsBitRawSensors[V_DYN_EXCITE] >> 16) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((adahrsBitRawSensors[V_DYN_EXCITE] >> 8 ) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)( adahrsBitRawSensors[V_DYN_EXCITE]        & 0xff);

	/* add BIT status */
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((getTopERROR() >> 8) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)( getTopERROR()       & 0xff);

	/* send Factory 6 packet */
	ExternPortTx(port, packetPtr);
}

/*********************************************************************************
* Function name:	SendUcbFactory7
*
* Description:	send F7 packet
*
* Trace:
*	[SDD_UCB_TX_F7 <-- SRC_UCB_TX_F7]
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
static void SendUcbFactory7 (ExternPortTypeEnum port, PacketPtrType packetPtr)
{
	uint16_t packetIndex = 0;

    /* set packet length */
	packetPtr.ucbPacketPtr->payloadLength = UCB_FACTORY_7_LENGTH;

    /* add X-accelerometer sensor count */
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((rawInertialSensors[XACCEL] >> 24) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((rawInertialSensors[XACCEL] >> 16) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((rawInertialSensors[XACCEL] >> 8 ) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)( rawInertialSensors[XACCEL]        & 0xff);

    /* add Y-accelerometer sensor count */
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((rawInertialSensors[YACCEL] >> 24) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((rawInertialSensors[YACCEL] >> 16) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((rawInertialSensors[YACCEL] >> 8 ) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)( rawInertialSensors[YACCEL]        & 0xff);

	/* add Z-accelerometer sensor count */
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((rawInertialSensors[ZACCEL] >> 24) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((rawInertialSensors[ZACCEL] >> 16) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((rawInertialSensors[ZACCEL] >> 8 ) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)( rawInertialSensors[ZACCEL]        & 0xff);

    /* add X-rate sensor count */
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((rawInertialSensors[XRATE] >> 24) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((rawInertialSensors[XRATE] >> 16) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((rawInertialSensors[XRATE] >> 8 ) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)( rawInertialSensors[XRATE]        & 0xff);

    /* add Y-rate sensor count */
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((rawInertialSensors[YRATE] >> 24) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((rawInertialSensors[YRATE] >> 16) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((rawInertialSensors[YRATE] >> 8 ) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)( rawInertialSensors[YRATE]        & 0xff);

	/* add Z-rate sensor count */
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((rawInertialSensors[ZRATE] >> 24) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((rawInertialSensors[ZRATE] >> 16) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((rawInertialSensors[ZRATE] >> 8 ) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)( rawInertialSensors[ZRATE]        & 0xff);

    /* append static and dynamic pressure counts and outside air temperature counts */
    packetIndex = appendAirDataSensorCounts((char *)packetPtr.ucbPacketPtr->payload, packetIndex);

	/* add X-accel temperature sensor counts */
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((rawInertialSensors[XATEMP] >> 24) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((rawInertialSensors[XATEMP] >> 16) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((rawInertialSensors[XATEMP] >> 8 ) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)( rawInertialSensors[XATEMP]        & 0xff);

	/* add Y-accel temperature sensor counts */
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((rawInertialSensors[YATEMP] >> 24) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((rawInertialSensors[YATEMP] >> 16) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((rawInertialSensors[YATEMP] >> 8 ) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)( rawInertialSensors[YATEMP]        & 0xff);

	/* add Z-accel temperature sensor counts */
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((rawInertialSensors[ZATEMP] >> 24) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((rawInertialSensors[ZATEMP] >> 16) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((rawInertialSensors[ZATEMP] >> 8 ) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)( rawInertialSensors[ZATEMP]        & 0xff);

	/* add X-rate temperature sensor counts */
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((rawInertialSensors[XRTEMP] >> 24) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((rawInertialSensors[XRTEMP] >> 16) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((rawInertialSensors[XRTEMP] >> 8 ) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)( rawInertialSensors[XRTEMP]        & 0xff);

	/* add Y-rate temperature sensor counts */
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((rawInertialSensors[YRTEMP] >> 24) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((rawInertialSensors[YRTEMP] >> 16) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((rawInertialSensors[YRTEMP] >> 8 ) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)( rawInertialSensors[YRTEMP]        & 0xff);

	/* add Z-rate temperature sensor counts */
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((rawInertialSensors[ZRTEMP] >> 24) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((rawInertialSensors[ZRTEMP] >> 16) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((rawInertialSensors[ZRTEMP] >> 8 ) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)( rawInertialSensors[ZRTEMP]        & 0xff);

	/* add static pressure temperature sensor counts */
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((rawInertialSensors[V_STAT_EXCITE] >> 24) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((rawInertialSensors[V_STAT_EXCITE] >> 16) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((rawInertialSensors[V_STAT_EXCITE] >> 8 ) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)( rawInertialSensors[V_STAT_EXCITE]        & 0xff);

	/* add dynamic pressure temperature sensor counts */
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((rawInertialSensors[V_DYN_EXCITE] >> 24) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((rawInertialSensors[V_DYN_EXCITE] >> 16) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((rawInertialSensors[V_DYN_EXCITE] >> 8 ) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)( rawInertialSensors[V_DYN_EXCITE]        & 0xff);

	/* add 1.8-volt IOUP counts */
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((adahrsBitRawSensors[V_BIT_1P8V_IOUP] >> 24) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((adahrsBitRawSensors[V_BIT_1P8V_IOUP] >> 16) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((adahrsBitRawSensors[V_BIT_1P8V_IOUP] >> 8 ) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)( adahrsBitRawSensors[V_BIT_1P8V_IOUP]        & 0xff);

	/* add 1.8-volt DUP counts */
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((adahrsBitRawSensors[V_BIT_1P8V_DUP] >> 24) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((adahrsBitRawSensors[V_BIT_1P8V_DUP] >> 16) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((adahrsBitRawSensors[V_BIT_1P8V_DUP] >> 8 ) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)( adahrsBitRawSensors[V_BIT_1P8V_DUP]        & 0xff);

	/* add 2.5-volt reference counts */
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((adahrsBitRawSensors[V_REF_2P5V] >> 24) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((adahrsBitRawSensors[V_REF_2P5V] >> 16) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((adahrsBitRawSensors[V_REF_2P5V] >> 8 ) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)( adahrsBitRawSensors[V_REF_2P5V]        & 0xff);

	/* add 5-volt gyro analog counts */
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((adahrsBitRawSensors[V_BIT_5V_AG] >> 24) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((adahrsBitRawSensors[V_BIT_5V_AG] >> 16) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((adahrsBitRawSensors[V_BIT_5V_AG] >> 8 ) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)( adahrsBitRawSensors[V_BIT_5V_AG]        & 0xff);

	/* add 3.3-volt digital counts */
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((adahrsBitRawSensors[V_BIT_3P3V] >> 24) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((adahrsBitRawSensors[V_BIT_3P3V] >> 16) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((adahrsBitRawSensors[V_BIT_3P3V] >> 8 ) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)( adahrsBitRawSensors[V_BIT_3P3V]        & 0xff);

    /* add 5-volt digital counts */
    packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((adahrsBitRawSensors[V_BIT_5VD] >> 24) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((adahrsBitRawSensors[V_BIT_5VD] >> 16) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((adahrsBitRawSensors[V_BIT_5VD] >> 8 ) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)( adahrsBitRawSensors[V_BIT_5VD]        & 0xff);

    /* add 5-volt power analog counts */
    packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((adahrsBitRawSensors[V_BIT_5VA] >> 24) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((adahrsBitRawSensors[V_BIT_5VA] >> 16) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((adahrsBitRawSensors[V_BIT_5VA] >> 8 ) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)( adahrsBitRawSensors[V_BIT_5VA]        & 0xff);

    /* add 7-volt  */
    packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((adahrsBitRawSensors[V_BIT_7V] >> 24) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((adahrsBitRawSensors[V_BIT_7V] >> 16) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((adahrsBitRawSensors[V_BIT_7V] >> 8 ) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)( adahrsBitRawSensors[V_BIT_7V]        & 0xff);

	/* add X-accel current counts */
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((adahrsBitRawSensors[XACUR] >> 24) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((adahrsBitRawSensors[XACUR] >> 16) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((adahrsBitRawSensors[XACUR] >> 8 ) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)( adahrsBitRawSensors[XACUR]        & 0xff);

	/* add Y-accel current counts */
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((adahrsBitRawSensors[YACUR] >> 24) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((adahrsBitRawSensors[YACUR] >> 16) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((adahrsBitRawSensors[YACUR] >> 8 ) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)( adahrsBitRawSensors[YACUR]        & 0xff);

	/* add Z-accel current counts */
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((adahrsBitRawSensors[ZACUR] >> 24) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((adahrsBitRawSensors[ZACUR] >> 16) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((adahrsBitRawSensors[ZACUR] >> 8 ) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)( adahrsBitRawSensors[ZACUR]        & 0xff);

	/* add X-rate current counts */
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((adahrsBitRawSensors[XRCUR] >> 24) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((adahrsBitRawSensors[XRCUR] >> 16) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((adahrsBitRawSensors[XRCUR] >> 8 ) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)( adahrsBitRawSensors[XRCUR]        & 0xff);

	/* add Y-rate current counts */
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((adahrsBitRawSensors[YRCUR] >> 24) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((adahrsBitRawSensors[YRCUR] >> 16) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((adahrsBitRawSensors[YRCUR] >> 8 ) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)( adahrsBitRawSensors[YRCUR]        & 0xff);

	/* add Z-rate current counts */
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((adahrsBitRawSensors[ZRCUR] >> 24) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((adahrsBitRawSensors[ZRCUR] >> 16) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((adahrsBitRawSensors[ZRCUR] >> 8 ) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)( adahrsBitRawSensors[ZRCUR]        & 0xff);

	/* add static pressure current counts */
    packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((adahrsBitRawSensors[V_STAT_EXCITE] >> 24) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((adahrsBitRawSensors[V_STAT_EXCITE] >> 16) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((adahrsBitRawSensors[V_STAT_EXCITE] >> 8 ) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)( adahrsBitRawSensors[V_STAT_EXCITE]        & 0xff);

	/* add dynamic pressure current counts */
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((adahrsBitRawSensors[V_DYN_EXCITE] >> 24) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((adahrsBitRawSensors[V_DYN_EXCITE] >> 16) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((adahrsBitRawSensors[V_DYN_EXCITE] >> 8 ) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)( adahrsBitRawSensors[V_DYN_EXCITE]        & 0xff);

	/* add BIT status */
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((getTopERROR() >> 8) & 0xff);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)( getTopERROR()       & 0xff);

	/* send Factory 7 packet */
	ExternPortTx(port, packetPtr);
}

/*********************************************************************************
* Function name:	SendUcbMagCal1Complete
*
* Description:	send CB packet (cal phase 1 complete with data)
*
* Trace: [SDD_UCB_TX_CB <-- SRC_UCB_TX_CB]
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
static void SendUcbMagCal1Complete (ExternPortTypeEnum port, PacketPtrType packetPtr)
{
	uint8_t packetIndex = 0;

    /* set packet length */
	packetPtr.ucbPacketPtr->payloadLength = UCB_MAG_CAL_1_COMPLETE_LENGTH;

	/* add mag roll and pitch offsets */
    packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((gConfiguration.OffsetAnglesExtMag[0] >> 8) & 0xff);
    packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)( gConfiguration.OffsetAnglesExtMag[0]       & 0xff);
    packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)((gConfiguration.OffsetAnglesExtMag[1] >> 8) & 0xff);
    packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)( gConfiguration.OffsetAnglesExtMag[1]       & 0xff);

	/* send phase 1 complete response */
	ExternPortTx(port, packetPtr);
}

/*********************************************************************************
* Function name:	SendUcbMagCal3Complete
*
* Description:	send CD packet (cal phase 2 complete with data)
*
* Trace: [SDD_UCB_TX_CD <-- SRC_UCB_TX_CD]
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
static void SendUcbMagCal3Complete (ExternPortTypeEnum port, PacketPtrType packetPtr)
{
	uint8_t packetIndex = 0;

	/* set packet length */
	packetPtr.ucbPacketPtr->payloadLength = UCB_MAG_CAL_3_COMPLETE_LENGTH;

	/* add requested calibration task */
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)(0x00);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)(0x0B);

	/* add X hard-iron bias */
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)(gConfiguration.hardIronBiasExt[0] >> 8);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)(gConfiguration.hardIronBiasExt[0]);

	/* add Y hard-iron bias */
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)(gConfiguration.hardIronBiasExt[1] >> 8);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)(gConfiguration.hardIronBiasExt[1]);

	/* add soft iron scale ratio */
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)(softIronScaleRatio >> 8);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)(softIronScaleRatio);

	/* add soft iron angle */
    packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)(softIronAngle >> 8);
	packetPtr.ucbPacketPtr->payload[packetIndex++] = (uint8_t)(softIronAngle);

    /* send phase 2 complete response */
	ExternPortTx(port, packetPtr);
}

/*********************************************************************************
* Function name:	SendPacket
*
* Description:	top level send packet routine -
*				calls other send routines based on packet type
*
* Trace:
*	[SDD_OUTPUT_PACKET <-- SRC_DATA_PACKET_TYPES]
*
* Input parameters:
*         port               port number request came in on, the reply will go out
*                            this port
*         packet             data part of packet
*
* Output parameters:	none
*
* Return value: none
*********************************************************************************/
void SendPacket (ExternPortTypeEnum port, PacketPtrType packetPtr)
{
#if DMU380_NOT_WORKING

	/* valid port and non-null packet pointer? */
    if ( ( (port == PRIMARY_UCB_PORT) 	   ||
		   (port == SECONDARY_UCB_PORT_0) ||
		   (port == SECONDARY_UCB_PORT_1) ||
           (port == MAG_DIAG_PORT)  )      &&
        (packetPtr.ucbPacketPtr != 0))
#endif  // DMU380_NOT_WORKING
    if ( ( (port == PRIMARY_UCB_PORT) &&  (packetPtr.ucbPacketPtr != 0)) )
    {
		switch (packetPtr.ucbPacketPtr->packetType) {
#			define OUTPUT_PACKET_TYPES
#			define UPDATE_PACKET_TYPES
#			define PACKET_TYPE(constName, varName, code)	case constName: Send ## varName(port, packetPtr); break;
#			include "ucb_packet_types.def"
#			undef OUTPUT_PACKET_TYPES
#			undef UPDATE_PACKET_TYPES
#			undef PACKET_TYPE

			default:	break;	/* default handler? */
		}
	}
}
