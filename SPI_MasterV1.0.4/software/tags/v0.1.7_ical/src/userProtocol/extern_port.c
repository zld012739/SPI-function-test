/*********************************************************************************
* File name:  extern_port.c
*
* File description:
*   - functions for general external port interface
*
*********************************************************************************/

#include "stm32f2xx.h"
#include "crc.h"
#include "dmu.h"
#include "ucb_packet.h"
#include "crm_packet.h"
#include "packet.h"
#include "extern_port.h"
#include "extern_port_config.h"
#include "comm_buffers.h"
#include "xbowsp_generaldrivers.h"
#include "uart.h"

/* size of port-associated software circular buffers */
#define PORT_0_RX_BUF_SIZE	COM_BUF_SIZE
#define PORT_0_TX_BUF_SIZE  COM_BUF_SIZE
#define PORT_1_RX_BUF_SIZE	COM_BUF_SIZE
#define PORT_1_TX_BUF_SIZE  COM_BUF_SIZE
#define PORT_2_RX_BUF_SIZE	COM_BUF_SIZE
#define PORT_2_TX_BUF_SIZE  COM_BUF_SIZE
#define PORT_3_RX_BUF_SIZE	COM_BUF_SIZE
#define PORT_3_TX_BUF_SIZE  COM_BUF_SIZE

/* packet-type receive working buffer sizes */
#define UCB_RX_WORKING_BUFFER_SIZE	128
#define CRM_RX_WORKING_BUFFER_SIZE	128

/* main loop period */
extern const uint16_t MAIN_LOOP_PERIOD;		/* milliseconds (approximately) */

/* port behavior parameters */
#define RX_POLL_PERIOD	MAIN_LOOP_PERIOD	/* milliseconds (approximately) */
#define UCB_RX_TIMEOUT 	1000				/* milliseconds (approximately) */
#define CRM_RX_TIMEOUT	1000				/* milliseconds (approximately) */

/* baud rate config field to hardware baud rate lookup table, initialized at runtime */
static uint16_t BAUD_TABLE [NUM_BAUD_RATES];

/* array of physical port pointers */
static port_struct *physicalPort [NUM_PHYSICAL_PORTS];

/* logical to physical port mapping lookup table */
static port_struct *portMap [NUM_LOGICAL_PORTS];

/* logical to physical port number mapping lookup table */
static uint16_t portNumMap [NUM_LOGICAL_PORTS];

/* initial CRC seed to use for UCB packet CRC */
static const UcbPacketCrcType UCB_PACKET_CRC_INITIAL_SEED = CRC_CCITT_INITIAL_SEED;

/* for forcing state machine reset whenever initialization has been performed */
static BOOL stateReset = FALSE;
port_struct     gPort0, gPort1, gPort2, gPort3; /* reference to physical port structures */

/*********************************************************************************
* Function name:	ExternPortInit
*
* Description:    Initializes the serial port and maps logical ports.
*
* Trace:
* [SDD_EXT_SERIAL_INIT_USAGE <-- SRC_EXT_PORT_INIT]
* [SDD_EXT_SERIAL_INIT_MAP <-- SRC_EXT_PORT_INIT]
* [SDD_EXT_SERIAL_INIT_OFF <-- SRC_EXT_PORT_INIT]
*
* Input parameters: none
*
* Output parameters: none
*
* Static variables affected:
*     BAUD_TABLE[]
*     physicalPort[]
*     portMap[]
*     portNumMap[]
*
*
* Return value: none
*********************************************************************************/
void ExternPortInit (void)
{
    /* reference to configuration fields */
    //    extern ConfigurationStruct configuration;
    /* array of physical port configuration settings for easy indexing */
    uint16_t *physicalPortConfig [NUM_PHYSICAL_PORTS];

    /* port-associated software rx and tx circular buffers */
	static uint8_t port0_rx [PORT_0_RX_BUF_SIZE], port0_tx [PORT_0_TX_BUF_SIZE], \
			  	 port1_rx [PORT_1_RX_BUF_SIZE], port1_tx [PORT_1_TX_BUF_SIZE], \
		 		 port2_rx [PORT_2_RX_BUF_SIZE], port2_tx [PORT_2_TX_BUF_SIZE], \
		 		 port3_rx [PORT_3_RX_BUF_SIZE], port3_tx [PORT_3_TX_BUF_SIZE];

    uint16_t portNum;

    /* initialize baud rate config field to hardware baud rate lookup table */
#	define PORT_BAUD_MAP(configBaud, hwBaud)	BAUD_TABLE[ configBaud ] = hwBaud;
#	include "extern_port_config.h"
#	undef PORT_BAUD_MAP
    
    /* reset port mapping and numbering */
    for (portNum = 0; portNum < NUM_LOGICAL_PORTS; ++portNum) {
    	portMap[portNum]    = 0;
    	portNumMap[portNum] = 0;
    }

    /* assign physical port pointers */
	physicalPort[0] = &gPort0;
	physicalPort[1] = &gPort1;
	physicalPort[2] = &gPort2;
	physicalPort[3] = &gPort3;

    /* assign physical port configuration settings */
	physicalPortConfig[0] = &gConfiguration.port1Usage;
	physicalPortConfig[1] = &gConfiguration.port2Usage;
	physicalPortConfig[2] = &gConfiguration.port3Usage;
	physicalPortConfig[3] = &gConfiguration.port4Usage;

    /* set baud rates from configuration */
    gPort0.hw.baud = BAUD_TABLE[gConfiguration.port1BaudRate];
    gPort1.hw.baud = BAUD_TABLE[gConfiguration.port2BaudRate];
    gPort2.hw.baud = BAUD_TABLE[gConfiguration.port3BaudRate];
    gPort3.hw.baud = BAUD_TABLE[gConfiguration.port4BaudRate];

    /* initialize software circular buffers */
    COM_buf_init(&gPort0, port0_tx, port0_rx, PORT_0_RX_BUF_SIZE, PORT_0_TX_BUF_SIZE);
    COM_buf_init(&gPort1, port1_tx, port1_rx, PORT_1_RX_BUF_SIZE, PORT_1_TX_BUF_SIZE);
    COM_buf_init(&gPort2, port2_tx, port2_rx, PORT_2_RX_BUF_SIZE, PORT_2_TX_BUF_SIZE);
    COM_buf_init(&gPort3, port3_tx, port3_rx, PORT_3_RX_BUF_SIZE, PORT_3_TX_BUF_SIZE);

	/* initialize the physical UART ports */
	uart_init(0, &(gPort0.hw));
	uart_init(1, &(gPort1.hw));
#if DMU380_NOT_WORKING
    uart_init(QUART_CHANNEL2, &(gPort2.hw));
    uart_init(QUART_CHANNEL3, &(gPort3.hw));
#endif // DMU380_NOT_WORKING

	/* map physical ports to logical ports */
	for (portNum = 0; portNum < NUM_PHYSICAL_PORTS; ++portNum) {
		if (*physicalPortConfig[portNum] != PORT_OFF) {
			switch (*physicalPortConfig[portNum]) {

				case CRM_PORT: {
			 		portMap[MAG_DIAG_PORT]    = physicalPort[portNum];
			 		portNumMap[MAG_DIAG_PORT] = portNum;

			 		break;
			 	}
				case PRI_UCB_PORT: {
				    portMap[PRIMARY_UCB_PORT]    = physicalPort[portNum];
					portNumMap[PRIMARY_UCB_PORT] = portNum;

					break;
				}

				case GPS_PORT: {

				}

			    default: {	/* bad, means the configuration is corrupted */
				break;
			    }
		 	}
		}
	}

	/* reset all state machines in case any aren't reset (used for re-initialization) */
	stateReset = TRUE;
}
/* end ExternPortInit */

/*********************************************************************************
* Function name:	ExternPortMapToHWPort
*
* Description:      maps an external port to a logical hardware port.
*
* Trace: [SDD_BIT_LL_PORT <-- SRC_EXT_PORT_HW]
*
* Input parameters: port -- the port type requesting physical port mapping
*
* Output parameters:none
*
* Return value: physical port number
*********************************************************************************/
inline uint16_t ExternPortToHWPort (ExternPortTypeEnum port)
{
	return portNumMap[port];
}
/* end ExternPortToHWPort */

/*********************************************************************************
* Function name:	ExternPortEnabled
*
* Description:     Test a port number to see if its enabled.
*
* Trace: 
* [SDD_PROCESS_USER_PORTS_02 <-- SRC_EXT_SERIAL_PORT_ENABLED]
* [SDD_PROCESS_USER_PORTS_03 <-- SRC_EXT_SERIAL_PORT_ENABLED]
*
* Input parameters: port -- the port type requested info if enabled
*
* Output parameters: none
*
* Return value: TRUE if the port is enabled, FALSE if not
*********************************************************************************/
inline BOOL ExternPortEnabled (ExternPortTypeEnum port)
{
	BOOL enabled = FALSE;

    if (portMap[port] != 0) {
    	enabled = TRUE;
    }

	return enabled;
}
/* end ExternPortEnabled */

/*********************************************************************************
* Function name:	HwPortRx
*
* Description:
*   Read input for a logical port type, place the data into the logical ports buffer
*
* Trace: [SDD_UART_READ_01 <-- SRC_EXT_SERIAL_HWRX]
*
* Input parameters:
*
* Output parameters:
*
* Return value:
*********************************************************************************/
static void HwPortRx (ExternPortTypeEnum port)
{
	/* FIXME ECW code for receiving on UART on given port */
}
/* end HwPortRx */

/*********************************************************************************
* Function name:	HwPortTx
*
* Description:
* write output into the buffers for a logical port type
*
* Trace: [SDD_UART_WRITE_01 <-- SRC_EXT_SERIAL_HWTX]
*
* Input parameters:
*
* Output parameters:
*
* Return value:
*********************************************************************************/
static void HwPortTx (ExternPortTypeEnum port)
{
    uart_write(portNumMap[port], portMap[port]);
} /* end HwPortTx */

/*********************************************************************************
* Function name:	HandleUcbRx
*
* Description:   handles received ucb packets
*
* Trace: 
*	[SDD_UCB_TIMEOUT_01 <-- SRC_HANDLE_UCB_RX]   
*	[SDD_UCB_PACKET_CRC <-- SRC_HANDLE_UCB_RX]
*	[SDD_UCB_CONVERT_DATA <-- SRC_HANDLE_UCB_RX]
*	[SDD_UCB_STORE_DATA <-- SRC_HANDLE_UCB_RX]
*	[SDD_UCB_UNKNOWN_01 <-- SRC_HANDLE_UCB_RX]
*	[SDD_UCB_CRC_FAIL_01 <-- SRC_HANDLE_UCB_RX]     
*	[SDD_UCB_VALID_PACKET <-- SRC_HANDLE_UCB_RX]
*
* Input parameters:
*             port      --  logical port type

*
* Output parameters:
*             packetPtr --  UCB packet to read the packet into
*
* Return value:
*      TRUE if a full packet has been seen (can fail CRC)
*      FALSE if needing more to fill in a packet
*********************************************************************************/
static BOOL HandleUcbRx (ExternPortTypeEnum port, PacketPtrType packetPtr)
{
	/* receive states, only referenced in this function so kept to local scope */
	typedef enum {
		RESET_STATE,
		SYNC_STATE,
		PACKET_TYPE_STATE,
		PAYLOAD_LENGTH_STATE,
		DATA_STATE,
		CRC_STATE,
		CHECK_CRC_STATE,
		PACKET_DONE_STATE
	} StateType;

	static StateType state [NUM_LOGICAL_PORTS] = { RESET_STATE, RESET_STATE, RESET_STATE, RESET_STATE };

    /* receive timeout timer */
    static BOOL		timeoutRunning [NUM_LOGICAL_PORTS] = { FALSE, FALSE, FALSE, FALSE };
    static uint16_t	timeoutCount [NUM_LOGICAL_PORTS]   = { 0, 0, 0, 0 };

    /* working buffer for assembling packets */
    static uint8_t 	workingBuffer [NUM_LOGICAL_PORTS][UCB_RX_WORKING_BUFFER_SIZE];
    	   uint16_t	bufferIndex;

    /* bytes to read from each port buffer */
    static uint16_t bytesToRead [NUM_LOGICAL_PORTS] = { 0, 0, 0, 0 };

    /* indicates how many sync bytes were found in current working buffer */
    static uint16_t syncBytesFound [NUM_LOGICAL_PORTS] = { 0, 0, 0, 0 };

    /* general use byte counter */
    uint8_t byteCount;

    /* number of data words left to read from port buffer */
    static UcbPacketPayloadLengthType dataCount [NUM_LOGICAL_PORTS];

    /* received packet CRC */
    static UcbPacketCrcType packetCrc [NUM_LOGICAL_PORTS];

    /* complete packet received? */
    BOOL packetDone = FALSE;


	  	/* increment timeout timer if neccessary, check if timeout limit was reached */
	 	if (timeoutRunning[port] == TRUE) {
	 		if (++timeoutCount[port] >= (UCB_RX_TIMEOUT / RX_POLL_PERIOD))	{
	 			timeoutCount[port] = 0;

	 			packetPtr.ucbPacketPtr->packetType = UCB_ERROR_TIMEOUT;

	 			state[port] = PACKET_DONE_STATE;
	 		}
	 	}


    /* handle current state */
    if ((state[port] == RESET_STATE) ||
    	(stateReset == TRUE)) {
    	/* reset timeout timer */
    	timeoutCount[port] = 0;

    	/* reset bytes to read counts */
    	bytesToRead[port] = 0;

    	/* reset number of synchronization bytes found */
    	syncBytesFound[port] = 0;

    	/* initialize state */
    	state[port] = SYNC_STATE;

    	/* clear state reset */
    	stateReset = FALSE;
    } /* end if */

	/* move any newly received data from hardware to software buffers and/or continue processing data
	   already in software buffers */
	HwPortRx(port);

    if (state[port] == SYNC_STATE) {
    	/* available bytes to read? */
	bytesToRead[port] = COM_buffer_bytes(&(portMap[port]->rec_buf));

	if (bytesToRead[port] > 0) {
		/* bytes to read doesn't exceed allocated buffer size? */
		if (bytesToRead[port] < UCB_RX_WORKING_BUFFER_SIZE) {
			/* read in all available bytes */
				COM_buf_read(&(portMap[port]->rec_buf),0,bytesToRead[port],
														&(workingBuffer[port][0]));
			} else {
			/* read in maximum allocated to buffer */
			COM_buf_read(&(portMap[port]->rec_buf), 0, UCB_RX_WORKING_BUFFER_SIZE, &(workingBuffer[port][0]));
		}

	    /* loop through all bytes until preamble is found, stop looping if found */
	    for (bufferIndex = 0; (bufferIndex < bytesToRead[port]) && (syncBytesFound[port] < UCB_SYNC_LENGTH); ++bufferIndex) {

		if (syncBytesFound[port] < UCB_SYNC_LENGTH)	{	/* not all preamble sync bytes have been found */
			if (workingBuffer[port][bufferIndex] == UCB_SYNC[syncBytesFound[port]]) {
				timeoutRunning[port] = TRUE;
				++syncBytesFound[port];
			} else {	/* not all preamble bytes appeared in a contiguous sequence, 
													not a preamble sync sequence */
				timeoutRunning[port] = FALSE;
				timeoutCount[port]   = 0;
				syncBytesFound[port] = 0;
			}
		}
		}

		if (syncBytesFound[port] == UCB_SYNC_LENGTH) {	/* all preamble sync bytes have been found */
			syncBytesFound[port] = 0;
		state[port] = PACKET_TYPE_STATE;
	    }

		/* remove all bytes from buffer up through current byte */
		COM_buf_pop(&(portMap[port]->rec_buf), bufferIndex);
	}
	}

    if (state[port] == PACKET_TYPE_STATE) {
    	/* available bytes to read? */
	bytesToRead[port] = COM_buffer_bytes(&(portMap[port]->rec_buf));

    	/* greater than size of packet type field? */
    	if (bytesToRead[port] >= UCB_PACKET_TYPE_LENGTH) {
		/* read in available bytes */
		COM_buf_out(&(portMap[port]->rec_buf), &(workingBuffer[port][0]), UCB_PACKET_TYPE_LENGTH);

		/* place packet type in packet structure */
		packetPtr.ucbPacketPtr->packetType = UcbPacketBytesToPacketType(&(workingBuffer[port][0]));

		/* check for invalid or non-input packet type */
		if ((packetPtr.ucbPacketPtr->packetType == UCB_ERROR_INVALID_TYPE) ||
				(UcbPacketIsAnInputPacket(packetPtr.ucbPacketPtr->packetType) == FALSE)) {

		/* place invalid packet type at beginning of data field */
		for (byteCount = 0; byteCount < UCB_PACKET_TYPE_LENGTH; ++byteCount) {
			(packetPtr.ucbPacketPtr)->payload[byteCount] = workingBuffer[port][byteCount];
		}

			state[port] = PACKET_DONE_STATE;
			} else {
			/* compute running CRC */
			packetCrc[port] = UcbPacketCalculateCrc(&(workingBuffer[port][0]), UCB_PACKET_TYPE_LENGTH, UCB_PACKET_CRC_INITIAL_SEED);

			state[port] = PAYLOAD_LENGTH_STATE;
		}
	}
    }

    if (state[port] == PAYLOAD_LENGTH_STATE) {
    	/* available bytes to read? */
	bytesToRead[port] = COM_buffer_bytes(&(portMap[port]->rec_buf));

    	/* greater than size of packet type field? */
    	if (bytesToRead[port] >= UCB_PAYLOAD_LENGTH_LENGTH) {
		/* read in available bytes */
		COM_buf_out(&(portMap[port]->rec_buf), &(workingBuffer[port][0]), UCB_PAYLOAD_LENGTH_LENGTH);

		/* place packet length in packet structure */
		packetPtr.ucbPacketPtr->payloadLength = UcbPacketBytesToPayloadLength(&(workingBuffer[port][0]));

		/* place payload length in data byte counter */
		dataCount[port] = UcbPacketBytesToPayloadLength(&(workingBuffer[port][0]));

		/* compute running CRC */
		packetCrc[port] = UcbPacketCalculateCrc(&(workingBuffer[port][0]), UCB_PAYLOAD_LENGTH_LENGTH, packetCrc[port]);

		state[port] = DATA_STATE;
	}
    }

    if (state[port] == DATA_STATE) {
    	if (dataCount[port] > 0) {	/* some packets have no data fields, skip this state if so */
    		/* available bytes to read? */
		bytesToRead[port] = COM_buffer_bytes(&(portMap[port]->rec_buf));

    		/* greater than number of remaining payload field bytes? */
    		if (bytesToRead[port] >= dataCount[port]) {
			/* place data bytes in packet structure */
			COM_buf_out(&(portMap[port]->rec_buf), packetPtr.ucbPacketPtr->payload, dataCount[port]);

		    /* compute running CRC */
			packetCrc[port] = UcbPacketCalculateCrc(packetPtr.ucbPacketPtr->payload, dataCount[port], packetCrc[port]);

    			state[port] = CRC_STATE;
    		}
    	} else {
    		state[port] = CRC_STATE;
    	}
    }

    if (state[port] == CRC_STATE) {
    	/* available bytes to read? */
	bytesToRead[port] = COM_buffer_bytes(&(portMap[port]->rec_buf));

    	/* greater than number of remaining payload field bytes? */
    	if (bytesToRead[port] >= UCB_CRC_LENGTH) {
		/* read in available bytes */
		COM_buf_out(&(portMap[port]->rec_buf), &(workingBuffer[port][0]), UCB_CRC_LENGTH);

		/* compute running CRC */
		packetCrc[port] = UcbPacketCalculateCrc(&(workingBuffer[port][0]), UCB_CRC_LENGTH, packetCrc[port]);

		state[port] = CHECK_CRC_STATE;
	}
    }

    if (state[port] == CHECK_CRC_STATE) {
    	if (packetCrc[port] == 0) {	/* CRC pass? */
    		state[port] = PACKET_DONE_STATE;
    	} else {	/* CRC fail */
    		/* copy packet type to payload */
    		UcbPacketPacketTypeToBytes(packetPtr.ucbPacketPtr->packetType, (packetPtr.ucbPacketPtr)->payload);

    		packetPtr.ucbPacketPtr->packetType = UCB_ERROR_CRC_FAIL;

    		state[port] = PACKET_DONE_STATE;
    	}

    	/* prepare for next time ExternPortRx is called in case there are bytes in the buffer
    	   but none in hardware */
	    bytesToRead[port] = COM_buffer_bytes(&(portMap[port]->rec_buf));
    }

    if (state[port] == PACKET_DONE_STATE) {
    	timeoutRunning[port] = FALSE;
    	packetDone  		 = TRUE;
    	state[port] 		 = RESET_STATE;
    }

	return packetDone;
}
/* end HandleUcbRx */

/*********************************************************************************
* Function name:	HandleCrmRx
*
* Description:     Handles recieved CRM data packets
*
* Trace: 
*    [SDD_CRM_PKT_TIMEOUT <-- SRC_HANDLE_CRM_RX]
*    [SDD_CRM_PKT_RESET <-- SRC_HANDLE_CRM_RX]
*    [SDD_CRM_PKT_SYNC <-- SRC_HANDLE_CRM_RX]
*    [SDD_CRM_PKT_XACCEL <-- SRC_HANDLE_CRM_RX]
*    [SDD_CRM_PKT_YACCEL <-- SRC_HANDLE_CRM_RX]
*    [SDD_CRM_PKT_ZACCEL <-- SRC_HANDLE_CRM_RX]
*    [SDD_CRM_PKT_XMAG <-- SRC_HANDLE_CRM_RX]
*    [SDD_CRM_PKT_YMAG <-- SRC_HANDLE_CRM_RX]
*    [SDD_CRM_PKT_ZMAG <-- SRC_HANDLE_CRM_RX]
*    [SDD_CRM_PKT_BIT <-- SRC_HANDLE_CRM_RX]
*    [SDD_CRM_PKT_MODELNUMBER <-- SRC_HANDLE_CRM_RX]
*    [SDD_CRM_PKT_CHKSUM <-- SRC_HANDLE_CRM_RX]
*    [SDD_CRM_PKT_CALC_CHKSUM_01 <-- SRC_HANDLE_CRM_RX]
*
* Input parameters:
*             port --  logical port type
*
* Output parameters:
*             packetPtr --- completed CRM packet
*
* Return value:
*    TRUE if a full packet has been seen, may not be valid
*    FALSE needing data for a full packet
*********************************************************************************/
static BOOL HandleCrmRx (ExternPortTypeEnum port, PacketPtrType packetPtr)
{
     /* receive states, only referenced in this function so kept to local scope */
     typedef enum {
	  RESET_STATE,
	  SYNC_STATE,
	  X_ACCEL_STATE,
	  Y_ACCEL_STATE,
	  Z_ACCEL_STATE,
	  X_MAG_STATE,
	  Y_MAG_STATE,
	  Z_MAG_STATE,
	  MODEL_NO_STATE,
	  BIT_STATE,
	  CHECKSUM_STATE,
	  CHECK_CHECKSUM_STATE,
	  PACKET_DONE_STATE
     } StateType;

     static StateType state = RESET_STATE;

     /* receive timeout timer */
     static BOOL         timeoutRunning = FALSE;
     static uint16_t       timeoutCount   = 0;

     /* working buffer for assembling packets */
     static uint8_t        workingBuffer [CRM_RX_WORKING_BUFFER_SIZE];
     uint16_t       bufferIndex;

     /* bytes to read from port buffer */
     static uint16_t bytesToRead;

     /* indicates how many sync bytes were found in current working buffer */
     static uint16_t syncBytesFound;

     /* received packet checksum */
     static CrmPacketChecksumType packetChecksum;

     /* complete packet received? */
     BOOL packetDone = FALSE;

     /* increment timeout timer if neccessary, check if timeout limit was reached */
     if (timeoutRunning == TRUE) {
	  if (++timeoutCount >= (CRM_RX_TIMEOUT / RX_POLL_PERIOD))        {
	       timeoutCount = 0;

	       packetPtr.ucbPacketPtr->packetType = CRM_ERROR_TIMEOUT;

	       state = PACKET_DONE_STATE;
	  }
     }

     /* handle current state */
	if 	((state == RESET_STATE) || (stateReset == TRUE)) {
	  /* reset timeout timer */
	  timeoutCount = 0;

	  /* reset bytes to read counts */
	  bytesToRead = 0;

	  /* reset number of synchronization bytes found */
	  syncBytesFound = 0;

	  /* reset checksum */
	  packetChecksum = 0;

	  /* initialize state */
	  state = SYNC_STATE;

	  /* clear state reset */
	  stateReset = FALSE;
     }

     /* move any newly received data from hardware to software buffers and/or continue processing data
	already in software buffers */
     HwPortRx(port);

     if (state == SYNC_STATE) {
	  /* available bytes to read? */
	  bytesToRead = COM_buffer_bytes(&(portMap[port]->rec_buf));

	  if (bytesToRead > 0) {
	       /* bytes to read doesn't exceed allocated buffer size? */
	       if (bytesToRead < CRM_RX_WORKING_BUFFER_SIZE) {
		    /* read in all available bytes */
		    COM_buf_read(&(portMap[port]->rec_buf), 0, bytesToRead, &(workingBuffer[0]));
			} else {
		    /* read in maximum allocated to buffer */
		    COM_buf_read(&(portMap[port]->rec_buf), 0, CRM_RX_WORKING_BUFFER_SIZE, &(workingBuffer[0]));
	       }

	       /* loop through all bytes until preamble is found, stop looping if found */
	       for (bufferIndex = 0; (bufferIndex < bytesToRead) && (syncBytesFound < CRM_SYNC_LENGTH); ++bufferIndex) {

		    if (syncBytesFound < CRM_SYNC_LENGTH)   {       /* not all preamble sync bytes have been found */
			 if (workingBuffer[bufferIndex] == CRM_SYNC[syncBytesFound]) {
			      timeoutRunning = TRUE;
			      ++syncBytesFound;
			 }
			 else {  /* not all preamble bytes appeared in a contiguous sequence, not a preamble sync sequence */
			      timeoutRunning = FALSE;
			      timeoutCount   = 0;
			      syncBytesFound = 0;
			 }
		    }
	       }

	       if (syncBytesFound == CRM_SYNC_LENGTH) {        /* all preamble sync bytes have been found */
		    syncBytesFound = 0;
		    state = X_ACCEL_STATE;
	       }

	       /* remove all bytes from buffer up through current byte */
	       COM_buf_pop(&(portMap[port]->rec_buf), bufferIndex);
	  }
     }

     if (state == X_ACCEL_STATE) {
	  /* available bytes to read? */
	  bytesToRead = COM_buffer_bytes(&(portMap[port]->rec_buf));

	  /* greater than size of X acceleration field? */
	  if (bytesToRead >= CRM_X_ACCEL_LENGTH) {
	       /* read in available bytes */
	       COM_buf_out(&(portMap[port]->rec_buf), &(workingBuffer[0]), CRM_X_ACCEL_LENGTH);

	       /* place X acceleration in packet structure */
	       packetPtr.crmPacketPtr->xAccel = CrmPacketBytesToAccel(&(workingBuffer[0]));

	       /* compute running checksum */
	       packetChecksum = CrmPacketCalculateChecksum(&(workingBuffer[0]), CRM_X_ACCEL_LENGTH, packetChecksum);

	       state = Y_ACCEL_STATE;
	  }
     }

     if (state == Y_ACCEL_STATE) {
	  /* available bytes to read? */
	  bytesToRead = COM_buffer_bytes(&(portMap[port]->rec_buf));

	  /* greater than size of Y acceleration field? */
	  if (bytesToRead >= CRM_Y_ACCEL_LENGTH) {
	       /* read in available bytes */
	       COM_buf_out(&(portMap[port]->rec_buf), &(workingBuffer[0]), CRM_Y_ACCEL_LENGTH);

	       /* place Y acceleration in packet structure */
	       packetPtr.crmPacketPtr->yAccel = CrmPacketBytesToAccel(&(workingBuffer[0]));

	       /* compute running checksum */
	       packetChecksum = CrmPacketCalculateChecksum(&(workingBuffer[0]), CRM_Y_ACCEL_LENGTH, packetChecksum);

	       state = Z_ACCEL_STATE;
	  }
     }

     if (state == Z_ACCEL_STATE) {
	  /* available bytes to read? */
	  bytesToRead = COM_buffer_bytes(&(portMap[port]->rec_buf));

	  /* greater than size of Z acceleration field? */
	  if (bytesToRead >= CRM_Z_ACCEL_LENGTH) {
	       /* read in available bytes */
	       COM_buf_out(&(portMap[port]->rec_buf), &(workingBuffer[0]), CRM_Z_ACCEL_LENGTH);

	       /* place Y acceleration in packet structure */
	       packetPtr.crmPacketPtr->zAccel = CrmPacketBytesToAccel(&(workingBuffer[0]));

	       /* compute running checksum */
	       packetChecksum = CrmPacketCalculateChecksum(&(workingBuffer[0]), CRM_Z_ACCEL_LENGTH, packetChecksum);

	       state = X_MAG_STATE;
	  }
     }

     if (state == X_MAG_STATE) {
	  /* available bytes to read? */
	  bytesToRead = COM_buffer_bytes(&(portMap[port]->rec_buf));

	  /* greater than size of X mag field? */
	  if (bytesToRead >= CRM_X_MAG_LENGTH) {
	       /* read in available bytes */
	       COM_buf_out(&(portMap[port]->rec_buf), &(workingBuffer[0]), CRM_X_MAG_LENGTH);

	       /* place X mag in packet structure */
	       packetPtr.crmPacketPtr->xMag = CrmPacketBytesToMag(&(workingBuffer[0]));

	       /* compute running checksum */
	       packetChecksum = CrmPacketCalculateChecksum(&(workingBuffer[0]), CRM_X_MAG_LENGTH, packetChecksum);

	       state = Y_MAG_STATE;
	  }
     }

     if (state == Y_MAG_STATE) {
	  /* available bytes to read? */
	  bytesToRead = COM_buffer_bytes(&(portMap[port]->rec_buf));

	  /* greater than size of Y mag field? */
	  if (bytesToRead >= CRM_Y_MAG_LENGTH) {
	       /* read in available bytes */
	       COM_buf_out(&(portMap[port]->rec_buf), &(workingBuffer[0]), CRM_Y_MAG_LENGTH);

	       /* place Y mag in packet structure */
	       packetPtr.crmPacketPtr->yMag = CrmPacketBytesToMag(&(workingBuffer[0]));

	       /* compute running checksum */
	       packetChecksum = CrmPacketCalculateChecksum(&(workingBuffer[0]), CRM_Y_MAG_LENGTH, packetChecksum);

	       state = Z_MAG_STATE;
	  }
     }

     if (state == Z_MAG_STATE) {
	  /* available bytes to read? */
	  bytesToRead = COM_buffer_bytes(&(portMap[port]->rec_buf));

	  /* greater than size of Z mag field? */
	  if (bytesToRead >= CRM_Z_MAG_LENGTH) {
	       /* read in available bytes */
	       COM_buf_out(&(portMap[port]->rec_buf), &(workingBuffer[0]), CRM_Z_MAG_LENGTH);

	       /* place Y mag in packet structure */
	       packetPtr.crmPacketPtr->zMag = CrmPacketBytesToMag(&(workingBuffer[0]));

	       /* compute running checksum */
	       packetChecksum = CrmPacketCalculateChecksum(&(workingBuffer[0]), CRM_Z_MAG_LENGTH, packetChecksum);

	       state = MODEL_NO_STATE;
	  }
     }

     if (state == MODEL_NO_STATE) {
	  /* available bytes to read? */
	  bytesToRead = COM_buffer_bytes(&(portMap[port]->rec_buf));

	  /* greater than size of model number field? */
	  if (bytesToRead >= CRM_MODEL_NO_LENGTH) {
	       /* read in available bytes */
	       COM_buf_out(&(portMap[port]->rec_buf), &(workingBuffer[0]), CRM_MODEL_NO_LENGTH);

	       /* place model number in packet structure */
	       packetPtr.crmPacketPtr->modelNo = CrmPacketBytesToModelNo(&(workingBuffer[0]));

	       /* compute running checksum */
	       packetChecksum = CrmPacketCalculateChecksum(&(workingBuffer[0]), CRM_MODEL_NO_LENGTH, packetChecksum);

	       state = BIT_STATE;
	  }
     }

     if (state == BIT_STATE) {
	  /* available bytes to read? */
	  bytesToRead = COM_buffer_bytes(&(portMap[port]->rec_buf));

	  /* greater than size of BIT field? */
	  if (bytesToRead >= CRM_BIT_LENGTH) {
	       /* read in available bytes */
	       COM_buf_out(&(portMap[port]->rec_buf), &(workingBuffer[0]), CRM_BIT_LENGTH);

	       /* place BIT in packet structure */
	       packetPtr.crmPacketPtr->bit = CrmPacketBytesToBit(&(workingBuffer[0]));

	       /* compute running checksum */
	       packetChecksum = CrmPacketCalculateChecksum(&(workingBuffer[0]), CRM_BIT_LENGTH, packetChecksum);

	       state = CHECKSUM_STATE;
	  }
     }

     if (state == CHECKSUM_STATE) {
	  /* available bytes to read? */
	  bytesToRead = COM_buffer_bytes(&(portMap[port]->rec_buf));

	  /* greater than number of remaining payload field bytes? */
	  if (bytesToRead >= CRM_CHECKSUM_LENGTH) {
	       /* read in available bytes */
	       COM_buf_out(&(portMap[port]->rec_buf), &(workingBuffer[0]), CRM_CHECKSUM_LENGTH);

	       /* subtract off received checksum from computed checksum, if correct it should be 0 */
	       packetChecksum -= CrmPacketBytesToChecksum(&(workingBuffer[0]));

	       state = CHECK_CHECKSUM_STATE;
	  }
     }

     if (state == CHECK_CHECKSUM_STATE) {
	  if (packetChecksum == 0) {      /* checksum pass? */
	       packetPtr.ucbPacketPtr->packetType = CRM;

	       state = PACKET_DONE_STATE;
		} else {  /* checksum fail */
	       packetPtr.crmPacketPtr->packetType = CRM_ERROR_CHECKSUM_FAIL;

	       state = PACKET_DONE_STATE;
	  }

	  /* prepare for next time ExternPortRx is called in case there are bytes in the buffer
	     but none in hardware */
	  bytesToRead = COM_buffer_bytes(&(portMap[port]->rec_buf));
     }

     if (state == PACKET_DONE_STATE) {
	  timeoutRunning = FALSE;
	  packetDone     = TRUE;
	  state              = RESET_STATE;
     }

     return packetDone;
}
/* end HandleCrmRx */

/*********************************************************************************
* Function name:	ExternPortRx
*
* Description:     Processes available data by calling the correct handler for the 
* data packet type, ucb or CRM.
*
* Trace: [SDD_PROCESS_EXT_PORT_RX <-- SRC_EXTERN_PORT_RX]
*
* Input parameters:
*                  port -- port type UCB or CRM
*
* Output parameters:
*                  packetPtr -- filled in packet from the mapped physical port
*
* Return value:
*            valid packet in packetPtr TRUE
*********************************************************************************/
BOOL ExternPortRx (ExternPortTypeEnum port, PacketPtrType packetPtr)
{
	BOOL packetDone = FALSE;

	switch (port) {
#		define PORT_TYPE(constName, rxType, txType)	case constName: packetDone = Handle ## rxType ## Rx(port, packetPtr); break;
#		include "extern_port_types.def"
#		undef PORT_TYPE

		default:	break;	/* no handler defined - do nothing */
	}

	return packetDone;
}
/* end ExternPortRx */

/*********************************************************************************
* Function name:	HandleUcbTx
*
* Description:     builds a UCB packet and then triggers transmission of it.
*
* Trace: [SDD_UCB_PROCESS_OUT <-- SRC_UCB_OUT_PKT]
*
* Input parameters:	none
*
* Output parameters:	none
*
* Return value:	none
*********************************************************************************/
static void HandleUcbTx (ExternPortTypeEnum port, PacketPtrType packetPtr)
{
#if ((UCB_PACKET_TYPE_LENGTH + UCB_PAYLOAD_LENGTH_LENGTH) > UCB_CRC_LENGTH)
	uint8_t data [UCB_PACKET_TYPE_LENGTH + UCB_PAYLOAD_LENGTH_LENGTH];
#else
    uint8_t data [UCB_CRC_LENGTH];
#endif

	UcbPacketCrcType crc;

	/* get byte representation of packet type, index adjust required since sync isn't placed in data array */
	UcbPacketPacketTypeToBytes(packetPtr.ucbPacketPtr->packetType, &(data[UCB_PACKET_TYPE_INDEX - UCB_SYNC_LENGTH]));

	/* get byte representation of packet length, index adjust required since sync isn't placed in data array */
	UcbPacketPayloadLengthToBytes(packetPtr.ucbPacketPtr->payloadLength, &(data[UCB_PAYLOAD_LENGTH_INDEX - UCB_SYNC_LENGTH]));

	/* place sync bytes in buffer */
	COM_buf_in(&(portMap[port]->xmit_buf), (uint8_t *)UCB_SYNC, UCB_SYNC_LENGTH);

	/* place packet type and payload length bytes in buffer */
    COM_buf_in(&(portMap[port]->xmit_buf), data, (UCB_PACKET_TYPE_LENGTH + UCB_PAYLOAD_LENGTH_LENGTH));

	/* place payload bytes into buffer */
	COM_buf_in(&(portMap[port]->xmit_buf), (uint8_t *)packetPtr.ucbPacketPtr->payload, packetPtr.ucbPacketPtr->payloadLength);

	/* calculate running CRC on packet type and payload length bytes */
	crc = UcbPacketCalculateCrc(data, (UCB_PACKET_TYPE_LENGTH + UCB_PAYLOAD_LENGTH_LENGTH), UCB_PACKET_CRC_INITIAL_SEED);

	/* calculate running CRC on payload bytes */
	if (packetPtr.ucbPacketPtr->payloadLength > 0) {
		crc = UcbPacketCalculateCrc(packetPtr.ucbPacketPtr->payload, packetPtr.ucbPacketPtr->payloadLength, crc);
	}

	/* get byte representation of CRC */
	UcbPacketCrcToBytes(crc, data);

	/* place CRC bytes into buffer */
	COM_buf_in(&(portMap[port]->xmit_buf), data, UCB_CRC_LENGTH);

	/* trigger low-level driver transmit */
	HwPortTx(port);
}
/* end HandleUcbTx */

/*********************************************************************************
* Function name:	ExternPortTx
*
* Description:    Calls the UCB handler to build and send a packet.  This will map
* the port to a logical hardware uart.
*
* Trace: 
* [SDD_PROCESS_EXT_PORT_TX <-- SRC_EXTERN_PORT_TX]
* [SDD_PROCESS_EXT_PORT_PKT_TYPE <-- SRC_EXTERN_PORT_TX]
*
* Input parameters:	none
*
* Output parameters:	none
*
* Return value:	none
*********************************************************************************/
void ExternPortTx (ExternPortTypeEnum port, PacketPtrType packetPtr)
{
	switch (port) {
#		define PORT_TYPE(constName, rxType, txType)	case constName: Handle ## txType ## Tx(port, packetPtr); break;
#		include "extern_port_types.def"
#		undef PORT_TYPE

		default:	break;	/* default handler? */
	}
}

/*********************************************************************************
* Function name:	ExternPortWaitOnTxIdle
*
* Description:
*  Drain all pending output on all port types
*
* Trace: [SDD_EXT_PORT_DRAIN <-- SRC_EXT_PORT_DRAIN]
*        [SDD_WATCHDOG <-- SRC_EXT_PORT_DRAIN]
*
* Input parameters:	none
*
* Output parameters:	none
*
* Return value:	none
*********************************************************************************/
void ExternPortWaitOnTxIdle (void)
{

    /* disable IOUP related interrupts - prevents timer interrupt from turning off
       communication interrupts */
#if DMU380_NOT_WORKING
    disable_ioup_ints();
#endif // DMU380_NOT_WORKING

    /* prevent watchdog reset */
    kick_dog();

	/* primary UCB port */
	if (ExternPortEnabled(PRIMARY_UCB_PORT) == TRUE) {
		while (bytes_remaining(portNumMap[PRIMARY_UCB_PORT], physicalPort[PRIMARY_UCB_PORT]) > 0) {
			/* run transmit cycle on port */
			HwPortTx(PRIMARY_UCB_PORT);
		}
	}

	/* mag/diag port */
	if (ExternPortEnabled(MAG_DIAG_PORT) == TRUE) {
		while (bytes_remaining(portNumMap[MAG_DIAG_PORT], physicalPort[MAG_DIAG_PORT]) > 0) {
			/* run transmit cycle on port */
			HwPortTx(MAG_DIAG_PORT);
		}
	}

	/* prevent watchdog reset */
    kick_dog();
}
