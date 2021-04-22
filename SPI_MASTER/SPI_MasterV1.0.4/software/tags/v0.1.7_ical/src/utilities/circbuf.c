/*
 * circbuf.c
 *
 */

#include <stdint.h>
#include "circbuf.h"


/*****************************************************************************
** Function name:   CircBufInit
** Descriptions:    Initializes a circular buffer structure
** Parameters:      cb - pointer to circular buffer structure
**                  buf - a pointer to a pre-allocated buffer used to store the data
**                  buf_size - size of the pre-allocated buffer
** Returned value:  TRUE if successful, FALSE otherwise
*****************************************************************************/
tBoolean CircBufInit(tCircularBuffer *cb, uint8_t volatile *buf, uint32_t buf_size) {
	if (!cb || !buf || (0 == buf_size)) {
		return FALSE;
	}
	cb->data = buf;
	cb->size = buf_size;
	cb->rd_idx = 0;
	cb->wr_idx = 0;
	return TRUE;
}

/*****************************************************************************
** Function name:   CircBufPut
** Descriptions:    Puts a byte in the circular buffer, this only 
**                  accesses rd_idx once and it only modifies wr_idx
** parameters:      cb - pointer to circular buffer structure
**                  c - byte to add
** Returned value:  None
*****************************************************************************/
void CircBufPut(tCircularBuffer *cb, uint8_t c) {
    
    int32_t len = cb->rd_idx; // atomic read to eliminate chance of 
                              // interrupts messing this up
    len -= cb->wr_idx;
    if (len < 0) { len += cb->size; }
	if (len < cb->size) {
		cb->data[cb->wr_idx++] = c;
		if (cb->wr_idx >= cb->size) {
			cb->wr_idx = 0;
		}
	}
}

/*****************************************************************************
** Function name:   CircBufGet
** Descriptions:    Gets a byte from the circular buffer,, this only 
**                  accesses wr_idx once and it only modifies rd_idx
** parameters:      cb - pointer to circular buffer structure
** Returned value:  Byte removed from circular buffer
*****************************************************************************/
uint8_t CircBufGet(tCircularBuffer *cb) {
	uint8_t c = 0;
    int32_t len = cb->wr_idx; // atomic read to eliminate chance of 
                              // interrupts messing this up
    len -= cb->rd_idx;
    if (len < 0) { len += cb->size; }
    
	if (len > 0) {
		c = cb->data[cb->rd_idx++];
		if (cb->rd_idx >= cb->size) {
			cb->rd_idx = 0;
		}
	}
	return c;
}

/*****************************************************************************
** Function name:   CircBufFlush
** Descriptions:    Empties a circular buffer
** parameters:      cb - pointer to circular buffer structure
** Returned value:  None
*****************************************************************************/
void CircBufFlush(tCircularBuffer *cb) {
	cb->rd_idx = cb->wr_idx  = 0;
}

/*****************************************************************************
** Function name:   CircBufLen
** Descriptions:    Returns the lenght of a circular buffer
** parameters:      cb - pointer to circular buffer structure
** Returned value:  Length of the circular buffer
*****************************************************************************/
uint32_t CircBufLen(tCircularBuffer *cb) {
    int32_t len = cb->wr_idx; 
    len -= cb->rd_idx;
    if (len < 0) { len += cb->size; }
    return len;
}

/*****************************************************************************
** Function name:   CircBufFull
** Descriptions:    Returns TRUE if the circular buffer is full
** parameters:      cb - pointer to circular buffer structure
** Returned value:  TRUE if the circular buffer is full, FALSE otherwise
*****************************************************************************/
tBoolean CircBufFull(tCircularBuffer *cb) {
	return (CircBufLen(cb) == cb->size);
}

/*****************************************************************************
** Function name:   CircBufEmpty
** Descriptions:    Returns TRUE if the circular buffer is empty
** parameters:      cb - pointer to circular buffer structure
** Returned value:  TRUE if the circular buffer is empty, FALSE otherwise
*****************************************************************************/
tBoolean CircBufEmpty(tCircularBuffer *cb) {
    uint32_t wr_idx = cb->wr_idx;
    uint32_t rd_idx =  cb->rd_idx;   
	return (wr_idx == rd_idx);
}


