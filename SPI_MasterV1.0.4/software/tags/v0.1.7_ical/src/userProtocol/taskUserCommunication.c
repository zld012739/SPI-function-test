/******************************************************************************
* File name:  taskUserCommunication.c
*
* File description:  user communication
*
******************************************************************************/

/* Includes */
#include "salvodefs.h"
#include "stm32f2xx.h"
#include "dmu.h"

#include "debug_usart.h"
#include "debug.h"

#include "crc.h"
#include "ucb_packet.h"
#include "crm_packet.h"
#include "packet.h"

#include "extern_port.h"
#include "send_packet.h"
#include "handle_packet.h"
#include "xbowsp_BITStatus.h"
#include "xbowsp_BITStatus_priv.h"   
#include "xbowsp_generaldrivers.h"
#include "xbowsp_init.h"
#include "ucb_packet.h"
#include "taskUserCommunication.h"

static void ProcessUcbCommands(void);
static void ProcessCrmPort(void);
static void ProcessContUcbPkt(void);
static void ProcessDiagOutput(void);

/* CRM port inactivity timeout (in ms) */
static const uint16_t CRM_PORT_COMM_TIMEOUT	= 2000;	/* ms */

/* main loop period (in ms) */
const uint16_t MAIN_LOOP_PERIOD = 10; /* ms */

/* continuous packet rate divider */
uint8_t divideCount = 1;

/* logical port (e.g. primary UCB, mag, etc.) packet structures, one for each logical port */
static UcbPacketStruct	continuousUcbPacket;	/* continuous primary UCB port output packet data*/
static UcbPacketStruct	primaryUcbPacket;		/* all other primary UCB port packet data */
static CrmPacketStruct	magCrmPacket;			/* CRM packet data */
static UcbPacketStruct 	diagUcbPacket;			/* diagnostic UCB port output packet data */
static UcbPacketStruct 	secondaryUcbPacket0;	/* secondary UCB port 0 packet data */
static UcbPacketStruct 	secondaryUcbPacket1;	/* secondary UCB port 1 packet data */

/* external port interface packet pointers, one for each logical port packet structure */
static PacketPtrType	continuousUcbPacketPtr;
static PacketPtrType 	primaryUcbPacketPtr;
static PacketPtrType 	magCrmPacketPtr;
static PacketPtrType 	diagUcbPacketPtr;
static PacketPtrType 	secondaryUcbPacket0Ptr;
static PacketPtrType 	secondaryUcbPacket1Ptr;



void TaskUserCommunication(void) 
{
    
    initConfigureUnit(); /* read config and cal globals from memory */
    ExternPortInit(); /* initialize the external port interface including the underlying hardware interface */
    
    /* assignment for abstraction of logical port packet structures */
	continuousUcbPacketPtr.ucbPacketPtr	= &continuousUcbPacket;
	primaryUcbPacketPtr.ucbPacketPtr	= &primaryUcbPacket;
	magCrmPacketPtr.crmPacketPtr        = &magCrmPacket;
	diagUcbPacketPtr.ucbPacketPtr	  	= &diagUcbPacket;
	secondaryUcbPacket0Ptr.ucbPacketPtr	= &secondaryUcbPacket0;
	secondaryUcbPacket1Ptr.ucbPacketPtr	= &secondaryUcbPacket1;

    while (1) {
        OS_WaitEFlag(EFLAGS_USER, EF_USER_ALL, OSALL_BITS, MAIN_LOOP_PERIOD/SALVO_TIMER_PRESCALE);

// OS_Wait on user rx but also on timeout... ticks are 10ms so this is only 100Hz output, will have to    
// change ticks or get a signal from something else, such as the data acq task
        
        /* process all incoming (UCB, CRM) communication */
        ProcessUcbCommands();
		ProcessCrmPort();
        
        ProcessContUcbPkt();
	   	ProcessDiagOutput();

                
    }
} 

/**********************************************************************************
* Module name:  ProcessUcbCommands
*
* Description:  This routine will test for the a port to be assigned to the UCB 
* function and test that port for a received packet.  If the packet is received it
* will call a handler.
*
* Trace:
* [SDD_PROCESS_USER_PORTS_01 <-- SRC_PROCESS_UCB_COMMANDS]
* [SDD_PROCESS_USER_PORTS_02 <-- SRC_PROCESS_UCB_COMMANDS]
* [SDD_PROCESS_USER_PORTS_03 <-- SRC_PROCESS_UCB_COMMANDS]
* [SDD_PROCESS_COMMANDS_SEQ <-- SRC_PROCESS_UCB_COMMANDS] 
*
* Input parameters: none
*
* Output parameters: none
*
* Return value: none
*
*********************************************************************************/
void ProcessUcbCommands (void)
{
    /* check all ports for received packets and handle appropriately */
	if (ExternPortRx(PRIMARY_UCB_PORT, primaryUcbPacketPtr) == TRUE) {
		HandlePacket(PRIMARY_UCB_PORT, primaryUcbPacketPtr);
	}

}
/* end ProcessExternPorts() */

/**********************************************************************************
* Module name:  ProcessCrmPort
*
* Description:  This function handles the high level crm communications including 
* the timeout for loss of communications.
*
* Trace:
* [SDD_PROCESS_CRM_01 <-- SRC_PROCESS_CRM]
* [SDD_PROCESS_CRM_02 <-- SRC_PROCESS_CRM]
* [SDD_PROCESS_CRM_03 <-- SRC_PROCESS_CRM]
* [SDD_PROCESS_CRM_04 <-- SRC_PROCESS_CRM]
* [SDD_PROCESS_CRM_05 <-- SRC_PROCESS_CRM]
* [SDD_PROCESS_CRM_06 <-- SRC_PROCESS_CRM]
*
* Input parameters: none
*
* Output parameters: none
*
* Return value: none
*
*********************************************************************************/
void ProcessCrmPort (void)
{
	static uint16_t timeoutCount = 0;

	if (ExternPortEnabled(MAG_DIAG_PORT) == TRUE) {
		if (ExternPortRx(MAG_DIAG_PORT, magCrmPacketPtr) == TRUE) {
			/* reset timeout */
			timeoutCount = 0;

			/* clear mag comm failure status  */
			setBITflag(BITSTAT_NOEXTERNALMAG, FALSE);

			/* send CRM packet to handler */
			HandlePacket(MAG_DIAG_PORT, magCrmPacketPtr);
		}
		else {
			/* CRM inactivity exceeds timeout? */
			if (timeoutCount >= (CRM_PORT_COMM_TIMEOUT / MAIN_LOOP_PERIOD)) {
				/* flag mag comm failure status - 
						will continue flagging every MAIN_LOOP_PERIOD milliseconds */
				/* flag error - will continue flagging every MAIN */		
				setBITflag(BITSTAT_NOEXTERNALMAG, TRUE);	
			}
		    else {
				++timeoutCount;	/* elapse timeout */
			}
		}
	}
}
/* end ProcessCrmCommands()*/

/**********************************************************************************
* Module name:  ProcessContUcbPkt
*
* Description: this generates the automatic transmission of UCB packets.  The specified
* packet type will be sent at some multiple of the 10 mSec acquistion rate.  This allows
* large packets that require more time to be sent.
*
* Trace:
* [SDD_PROCESS_PRIMARY_01 <-- SRC_PROCESS_PRIMARY]
* [SDD_PROCESS_PRIMARY_02 <-- SRC_PROCESS_PRIMARY]
*
* Input parameters: none
*
* Output parameters: none
*
* Return value: none
*
*********************************************************************************/
void ProcessContUcbPkt (void)
{
    uint8_t type [UCB_PACKET_TYPE_LENGTH];

    if (gConfiguration.packetRateDivider != 0) {	/* check for quiet mode */
		if (divideCount == 1) {
			/* get enum for requested continuous packet type */
			type[0] = (uint8_t)((gConfiguration.packetType >> 8) & 0xff);
			type[1] = (uint8_t)(gConfiguration.packetType & 0xff);

			/* set continuous output packet type based on configuration */
			continuousUcbPacketPtr.ucbPacketPtr->packetType = UcbPacketBytesToPacketType(type);

			/* send primary output port data */
		    SendPacket(PRIMARY_UCB_PORT, continuousUcbPacketPtr);

			/* reset divide count */
		    divideCount = (uint8_t)(gConfiguration.packetRateDivider);
		}
		else {
			--divideCount;
		}
	}
}
/* end ProcessContUcbPkt() */

/**********************************************************************************
* Module name:  ProcessDiagOuput
*
* Description:  This sends out a rotating packet and is timed so that the packet 
* transmission rate does not exceed the serial port bandwidth.  There are currently 
* eight packets that are sent out.
*
* Trace:
* [SDD_DIAG_OUTPUT <-- SRC_PROCESS_DIAG_OUTPUT]
*
* Input parameters: none
*
* Output parameters: none
*
* Return value: none
*
*********************************************************************************/
void ProcessDiagOutput (void)
{
     const UcbPacketTypeEnum ROTATING_DIAG_PACKET [] = { UCB_IDENTIFICATION,
							 UCB_VERSION_ALL_DATA,
							 UCB_ANGLE_3,
							 UCB_SCALED_3,
							 UCB_TEST_1,
							 UCB_FACTORY_7,
							 UCB_ANGLE_5,
							 UCB_ANGLE_U };

#	define NUM_DIAG_PACKET_TYPES	(sizeof(ROTATING_DIAG_PACKET)/sizeof(UcbPacketTypeEnum))
  
     static uint8_t currentPacketTypeIndex = 0;

     /* current iteration packet type */  
#ifndef DEBUG_CRM_TX_DEBUG_PORT_ALWAYS_A5
     diagUcbPacketPtr.ucbPacketPtr->packetType = (ROTATING_DIAG_PACKET[currentPacketTypeIndex]);  
#else
     diagUcbPacketPtr.ucbPacketPtr->packetType = UCB_ANGLE_5;
#endif

	 ++currentPacketTypeIndex;

     currentPacketTypeIndex = (uint8_t)(currentPacketTypeIndex % NUM_DIAG_PACKET_TYPES);
          
#ifndef DEBUG_CRM_TX_DEBUG_PORT_ALWAYS_A5          
     /* send diagnostic output port data */
     SendPacket(MAG_DIAG_PORT, diagUcbPacketPtr); 
#else
     if (currentPacketTypeIndex % 2 == 0) {
     	/* send diagnostic output port data */
     	SendPacket(MAG_DIAG_PORT, diagUcbPacketPtr);
     }
#endif

}
/* end ProcessDiagOutput() */
