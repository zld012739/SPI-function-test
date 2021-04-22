/*********************************************************************************
* File name:  extern_port_config.h
*
* File description: 
*   - extern port configuration
*
*********************************************************************************/

/* constants for port type configuration fields (see DMU specification)  */
#ifndef PORT_TYPE_CONFIG
#define PORT_TYPE_CONFIG
#define PORT_OFF		0
#define PRI_UCB_PORT    1
#define CRM_PORT		2
#define GPS_PORT		3

#endif

/* total number of port types */ 
#ifndef NUM_PORT_TYPES	
#define NUM_PORT_TYPES	4
#endif 

/* constants for port type configuration fields (see DMU specification)  */
#ifndef PORT_BAUD_CONFIG
#define PORT_BAUD_CONFIG
#define BAUD_9600	0
#define BAUD_19200	1
#define BAUD_38400	2
#define BAUD_57600	3
#endif

/* total number of port baud rates */ 
#ifndef NUM_BAUD_RATES	
#define NUM_BAUD_RATES	4
#endif
          

/* configuration baud rate to hardware baud rate mapping */ 
/* PORT_BAUD_CONFIG(<configuration baud rate>, <hardware baud rate>) */
#ifdef PORT_BAUD_MAP
PORT_BAUD_MAP(BAUD_9600, 	9600)
PORT_BAUD_MAP(BAUD_19200,	19200)
PORT_BAUD_MAP(BAUD_38400,	38400)
PORT_BAUD_MAP(BAUD_57600,	57600)
#endif

/* default configuration settings */
#ifdef DEFAULT_CONFIG 
DEFAULT_CONFIG(packetRateDivider, 	1)
DEFAULT_CONFIG(packetType,			0x4155)			/* Angle U (AU) = 0x4155 */ 
DEFAULT_CONFIG(port1BaudRate,		BAUD_57600)
DEFAULT_CONFIG(port2BaudRate,		BAUD_38400)
DEFAULT_CONFIG(port3BaudRate,		BAUD_9600) 
DEFAULT_CONFIG(port4BaudRate,		BAUD_9600)
DEFAULT_CONFIG(port1Usage,			PRI_UCB_PORT)
DEFAULT_CONFIG(port2Usage,			CRM_PORT)
DEFAULT_CONFIG(port3Usage,			PORT_OFF)			
DEFAULT_CONFIG(port4Usage,			PORT_OFF)
#endif 

