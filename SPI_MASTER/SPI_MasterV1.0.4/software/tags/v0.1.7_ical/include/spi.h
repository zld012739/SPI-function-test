/******************************************************************************
* File name:	
*	spi.h
*
* File description:
*	spi head file
*
* $Revision: 15866 $
*
* $Date: 2011-02-10 11:42:26 -0800 (Thu, 10 Feb 2011) $
*
* $Author: jsun $
 ******************************************************************************/
 
#ifndef __SPI_H
#define __SPI_H

#include "stdint.h"
#include "stm32f2xx.h"
#include "stm32f2xx_spi.h"

#define SPI_CPOL_AND_CPHA_HIGH  1
#define SPI_CPOL_AND_CPHA_LOW  0
uint8_t spi_configure(SPI_TypeDef* SPIx, uint8_t cpolAndCphaHigh, void (*callback)(void));
uint8_t spi_transfer(SPI_TypeDef* SPIx, uint8_t *in , uint8_t *out, uint16_t length);
uint8_t spi_done(SPI_TypeDef* SPIx) ;
void spi_go_slow(SPI_TypeDef* SPIx);
void spi_go_fast(SPI_TypeDef* SPIx);


#define SPI_ERROR_GENERIC 1
#define SPI_NO_ERROR 0
#endif
