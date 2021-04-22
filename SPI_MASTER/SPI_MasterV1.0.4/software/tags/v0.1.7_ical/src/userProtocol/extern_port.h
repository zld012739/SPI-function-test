/*********************************************************************************
* File name:  extern_port.h
*
* File description: 
*   - external communication port structures
*
*********************************************************************************/    

#ifndef EXTERN_PORT_H
#define EXTERN_PORT_H 

#include "port_def.h"

/* logical port parameters */
#define NUM_LOGICAL_PORTS 		4
                                 
/* physical port parameters */   
#define NUM_PHYSICAL_PORTS		4
  
typedef enum {  
#	define PORT_TYPE(constName, rxType, txType)		constName,
#	include "extern_port_types.def"
#	undef PORT_TYPE
	
	NUM_PORT_TYPE_ENUM
} ExternPortTypeEnum;      
     
extern void   	ExternPortInit         (void);
extern uint16_t	ExternPortToHWPort     (ExternPortTypeEnum port);
extern BOOL   	ExternPortEnabled      (ExternPortTypeEnum port);
extern BOOL   	ExternPortRx           (ExternPortTypeEnum port, PacketPtrType packetPtr);
extern void   	ExternPortTx           (ExternPortTypeEnum port, PacketPtrType packetPtr);
extern BOOL   	ExternPortAllTxIdle    (void);
extern void	 	ExternPortWaitOnTxIdle (void);

#endif  
