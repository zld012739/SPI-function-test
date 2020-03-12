/*******************************************************************************
* File name:	comm.h
*
* File description:
*		Head file for comm.c.
*
* $Rev: 15953 $
* $Date: 2011-02-17 16:26:00 -0800 (Thu, 17 Feb 2011) $
* $Author: dwang $
*******************************************************************************/

#ifndef COMM_H_
#define COMM_H_

void comm_init(void);
int  comm_test(void);
void comm_put(char);
void comm_puts(const char*);
char comm_get(void);

#endif

