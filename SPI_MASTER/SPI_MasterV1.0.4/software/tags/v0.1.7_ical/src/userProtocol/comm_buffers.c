/**********************************************************************************
* File name:  comm_buffers.c
*
* File description:
*   - This is a set of routines to move data in and out of the communication
*   - circular buffers.  They will also report back the byte used and bytes available
*   - in the buffer passed.  These are common routines used on both bootloader and DUP
*
* $Rev: 17420 $
* $Date: 2011-02-08 22:36:46 -0800 (Tue, 08 Feb 2011) $
* $Author: by-tdelong $
*********************************************************************************/
#include <stdint.h>
#include "dmu.h"
#include "extern_port_config.h"
#include "port_def.h"
#include "comm_buffers.h"


#define GOOD TRUE
#define BAD  FALSE

/**********************************************************************************
* Module name: COM_buffer_bytes
*
* Description: -        This routine will return the number of bytes in the
*                       circular buffer the buf_struc points to.
*
* Trace:
* [SDD_COM_BUFFER_BYTES_02 <-- SRC_COM_BUFFER_BYTES]
* [SDD_COM_BUFFER_BYTES_03 <-- SRC_COM_BUFFER_BYTES]
*
* Input parameters:     *buf_struc              pointer to the circular buffer structure
*
*
* Output parameters:
*
* Return value:         number of bytes in the buffer
*********************************************************************************/
unsigned int COM_buffer_bytes (cir_buf *buf_struc)
{
	unsigned int byteCnt;
	unsigned int tempIn;
    unsigned int tempOut;
    
    /* copy into temporary variables to keep atomic in case of alteration by interrupts*/
	tempIn=buf_struc->buf_inptr;
	tempOut= buf_struc->buf_outptr;
	
    if (tempIn >= tempOut) {
	byteCnt= tempIn - tempOut;
    } else {
	byteCnt= buf_struc->buf_size - (tempOut - tempIn);
    }
    return byteCnt;
}   /* end of COM_buffer_bytes */

/**********************************************************************************
* Module name: COM_buf_space
*
* Description: -          This routine will return the number of bytes available 
*                         in the circular buffer the buf_struc points
*
* Trace:
* [SDD_COM_BUF_SPACE_02 <-- SRC_COM_BUF_SPACE]
* [SDD_COM_BUF_SPACE_03 <-- SRC_COM_BUF_SPACE]
*
* Input parameters:     *buf_struc  pointer to the circular buffer structure
*
*
* Output parameters:    none
*
* Return value:         number of bytes available in the buffer.
*********************************************************************************/
unsigned int COM_buf_space (cir_buf *buf_struc)
{
     unsigned int byteCnt;
     unsigned int tempIn;
     unsigned int tempOut; 
     
     /* copy into temporary variables to keep the pointers stable in case of alteration by interrupts*/
	 tempIn=buf_struc->buf_inptr;
	 tempOut= buf_struc->buf_outptr;
	 
     if (tempIn >= tempOut) {
	  byteCnt= buf_struc->buf_size-(tempIn - tempOut);
     } else {
	  byteCnt= tempOut - tempIn;
     }
     return byteCnt-1;    /*Max number of bytes in buffer=buffer size -1 */
}   /* end of COM_buf_space*/

/**********************************************************************************
* Module name: COM_buf_pop
* Description: -  This routine will add the popCnt parameter to the output
*                 pointer of the circular buffer. If popCnt exceeds the number of 
*				  bytes in the buffer nothing will be removed.
*
* Trace:
* [SDD_COM_BUF_POP_01 <-- SRC_COM_BUF_POP]  
* [SDD_COM_BUF_POP_02 <-- SRC_COM_BUF_POP]
* [SDD_COM_BUF_POP_03 <-- SRC_COM_BUF_POP]
*
* Input parameters:     *buf_struc       pointer to the circular buffer structure
*                       popCnt			 number of bytes to remove from buffer
*
* Output parameters:    none
*
* Return value:         number of bytes popped.
*********************************************************************************/
unsigned int COM_buf_pop (cir_buf *buf_struc,unsigned int popCnt)
{
	unsigned int bytesPopped;
	unsigned int newOut; 
	
	if (COM_buffer_bytes (buf_struc)>=popCnt)  {
		bytesPopped= popCnt;
		newOut= buf_struc->buf_outptr + popCnt;
		if(newOut >= buf_struc->buf_size) {
			newOut-= buf_struc->buf_size;
		}
		buf_struc->buf_outptr=newOut;
    } else   bytesPopped=0;
	return bytesPopped;

}  /* end of COM_buf_pop */

/**********************************************************************************
* Module name: COM_buf_read
*
* Description: -        This routine will use the outptr and the bufIndex to point
*                       to the start of a string of charaters.  If asked for more
*                       characters than are in the buffer an error will be generated.
*                       The bytes read use the bufOut pointer for the starting add.
*
* Trace:
* [SDD_COM_BUF_READ_01 <-- SRC_COM_BUF_READ]  
* [SDD_COM_BUF_READ_02 <-- SRC_COM_BUF_READ]
* [SDD_COM_BUF_READ_03 <-- SRC_COM_BUF_READ]
* [SDD_COM_BUF_READ_04 <-- SRC_COM_BUF_READ]
* [SDD_COM_BUF_READ_05 <-- SRC_COM_BUF_READ]
*
* Input parameters:	cir_buf pointer buf_struc	pointer to the circular buffer structure
*                   unsigned int bufIndex
*					unsigned int cnt number of bytes output to bufOut pointer
*					
*
* Output parameters:  unsigned char pointer bufOut points to the output string of chars
*
* Return value:        status returns a BOOL either GOOD or BAD
*********************************************************************************/
BOOL COM_buf_read (cir_buf *buf_struc,unsigned int bufIndex,unsigned int cnt,unsigned char *bufOut)
{
	BOOL status;
	unsigned int strPtr;
	unsigned int i; 
	
	status=BAD;
	strPtr = buf_struc->buf_outptr + bufIndex;
	if(COM_buffer_bytes(buf_struc) >= cnt+bufIndex) {
		for (i=0; i<cnt; i++) {
			if (strPtr >= buf_struc->buf_size) {
				strPtr -= buf_struc->buf_size;
			}
			*bufOut= *(buf_struc->buf_add + strPtr++);
		 	bufOut++;
	    }
		status=GOOD;
	}
	return status;
}   /* end of COM_buf_read*/

/**********************************************************************************
* Module name: COM_buf_in
*
* Description: -        This routine will use cnt to check the available space in the
*                       circular buffer and if there is enough room, write it into
*                       the buffer using the pointer passed to it.  If there is not
*                       enough room in the buffer no data will be sent and an error
*                       will be returned
*
* Trace:
* [SDD_COM_BUF_IN_01 <-- SRC_COM_BUF_IN] 
* [SDD_COM_BUF_IN_02 <-- SRC_COM_BUF_IN]
* [SDD_COM_BUF_IN_03 <-- SRC_COM_BUF_IN]
* [SDD_COM_BUF_IN_04 <-- SRC_COM_BUF_IN]
*
* Input parameters:     cir_buf pointer buf_struc   pointer to the circular buffer structure
*                       unsigned char *buf pointer to input data
*                       unsigned int cnt number of bytes to put into the circular buffer
*
*
* Output parameters:    buf_struc.inptr in the buffer struct will be incremeted
*
* Return value:         status returns a BOOL either GOOD or BAD

*********************************************************************************/
BOOL COM_buf_in(cir_buf *buf_struc,unsigned char *buf,unsigned int cnt)
{   
	BOOL status;
	unsigned int i;
	
	status=BAD;
	if(COM_buf_space(buf_struc) >= cnt) {
		for (i=0; i<cnt; i++) {
			if(buf_struc->buf_inptr >= buf_struc->buf_size) {
				buf_struc->buf_inptr -= buf_struc->buf_size;
			}
			*(buf_struc->buf_add + (buf_struc->buf_inptr++)) = *buf;		
			buf++;
		}
		if(buf_struc->buf_inptr >= buf_struc->buf_size) {
			buf_struc->buf_inptr -= buf_struc->buf_size;   
		}
		status=GOOD;
	}
	return status;
}   /* end of COM_buf_in */
 /**********************************************************************************
* Module name: COM_buf_out
*
* Description: -  The routine will compare the number of bytes in cnt to
*                 the number of bytes in the buffer.  If the cnt is greater
*                 than the number of bytes in the buffer then no data will be
*                 moved and an error returned.  If there is enought data in the
*                 circular buffer then cnt number of bytes will be moved to the
*                 location pointed to by the buf pointer.
*
* Trace:
* [SDD_COM_BUF_OUT_01 <-- SRC_COM_BUF_OUT]  
* [SDD_COM_BUF_OUT_02 <-- SRC_COM_BUF_OUT]
* [SDD_COM_BUF_OUT_03 <-- SRC_COM_BUF_OUT]
* [SDD_COM_BUF_OUT_04 <-- SRC_COM_BUF_OUT]
*
* Input parameters:     *buf_struc      pointer to the circular buffer structure
*                       *buf 			pointer to the output location
*                       cnt             number of byte to pull from cir buffer.
*
*
* Output parameters:   outptr in the buffer will be incremented
*
* Return value:        status returns a BOOL either GOOD or BAD
*					   set if not enough bytes in circular buffer.
*********************************************************************************/
BOOL COM_buf_out(cir_buf *buf_struc,unsigned char *buf,unsigned int cnt)
{
	BOOL status;
	unsigned int i;
     
	status=BAD;
	if(COM_buffer_bytes(buf_struc) >= cnt) {
	    for (i=0; i<cnt; i++) {
			if(buf_struc->buf_outptr >= buf_struc->buf_size) buf_struc->buf_outptr -= buf_struc->buf_size;
			*buf= *(buf_struc->buf_add+(buf_struc->buf_outptr++)); 
		 	buf++;
	    }
	    if(buf_struc->buf_outptr>=buf_struc->buf_size) buf_struc->buf_outptr -= buf_struc->buf_size;
		status=GOOD;
	}
	return status;
}       /*end of COM_buf_out*/

/**********************************************************************************
* Module name: COM_buf_init
*
* Description: -  The routine shall initalize the buffer related pointers, indexs
*       -buffer size. it will also set datatype and tx_int_flg to zero. These items are
*       -the only items that are appropriate for this routine to initialize all others
*       -should be done by the subroutines which are related to them.  The data type and
*       -hw_int_flg are set here to inhibit functions from running if not initialize.
*
* Trace:
* [SDD_COM_BUF_INIT_01 <-- SRC_COM_BUF_INIT] 
* [SDD_COM_BUF_INIT_02 <-- SRC_COM_BUF_INIT]
* [SDD_COM_BUF_INIT_03 <-- SRC_COM_BUF_INIT]
*
* Input parameters:     *buf_struc              pointer to the circular buffer structure
*                                               *tx_buf                 pointer to the transmit buffer.
*                                               *rx_buf                 pointer to the receive buffer.
*
*
* Output parameters:   none
*
* Return value:        none
*********************************************************************************/
void COM_buf_init(port_struct *port,unsigned char *tx_buf,unsigned char *rx_buf,unsigned int rx_size,unsigned int tx_size)
{
	
	port->cdef.datatype=PORT_OFF;
	port->rec_buf.buf_add=rx_buf;
	port->rec_buf.buf_size=rx_size;
	port->rec_buf.buf_inptr=0;
	port->rec_buf.buf_outptr=0;

	port->xmit_buf.buf_add=tx_buf;
	port->xmit_buf.buf_size=tx_size;
	port->xmit_buf.buf_inptr=0;
	port->xmit_buf.buf_outptr=0;

}  /*end of COM_buf_init*/
