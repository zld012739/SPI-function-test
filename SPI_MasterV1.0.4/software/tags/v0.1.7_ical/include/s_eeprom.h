/**********************************************************************************
* File name:  s_eeprom.h
*
* File description: 
*   -Header file for s_eeprom.c
*               
* $Rev: 16090 $
* $Date: 2010-08-19 14:53:01 -0700 (Thu, 19 Aug 2010) $
* $Author: by-tdelong $
*
*************************************/
#ifndef S_EEPROM_H
#define S_EEPROM_H


/*----------------------------------------------------------------------------
* SERIAL EPROM REGISTERS:
*  SEPROM_WREN 	0x06	- 0xEEPROM write enabled
*  SEPROM_RDSR 	0x05	- read status register
*  SEPROM_WPROT	0x04	- set EEPROM write protect
*  SEPROM_READ 	0x03	- read data from memory  
*  SEPROM_WRITE	0x02	- write data to memory 
*  SEPROM_WRSR 	0x01	- write status register  
-----------------------------------------------------------------------------*/
#define  SEPROM_WREN        0x6
#define  SEPROM_RDSR        0x5
#define  SEPROM_WPROT		0x4
#define  SEPROM_READ        0x3
#define  SEPROM_WRITE       0x2
/*----------------------------------------------------------------------------
* SERIAL EPROM REGISTER BITS:
*  	SEPROM_RDY  		0x01	 Ready bit in RDSR reg    : 0 => ready
*	SEPROM_WEN		  	0x02
*	SEPROM_NOPROT      	0x00     no area protect BP1,BP0=00
*	SEPROM_1QTR        	0x04     0x1800 to 0x1fff protected BP1,BP0=01
*	SEPROM_1HALF       	0x08     0x1000 to 0x1fff protected BP1,BP0=10
*	SEPROM_ALL         	0x0C     0x0000 to 0x1fff protected BP1,BP0=11
*	SEPROM_WPEN			0x80     enables hardware write protect
-----------------------------------------------------------------------------*/
#define SEPROM_RDY         0x1 

/*----------------------------------------------------------------------------
* SERIAL WRITE PARAMETERS:
* SEPROM_WRITE_MSEC- msec to do a write to serial eprom
*-----------------------------------------------------------------------------*/
#define  SEPROM_WRITE_MSEC  15  

#define SEPROM_PATTERN       0xf0f0aa55   /* 32 bit pattern word */
#define SEPROM_NMB_PATT      7            /* # of 32 bit patterns stored */

 
#define XBOW_SP_EEPROM_BASE   0x0  
#define S_EEPROM_PATT_ADD     0xE00 


extern BOOL sEEPROMtest(void);  
extern void readEEPROMWords(uint16_t addr, uint16_t num, void *destination) ;
extern BOOL writeEEPROMWords(uint16_t addr, uint16_t num, void *source) ;
extern void readEEPROMTwoWords(uint16_t addr, uint16_t num, void *destination) ;
extern BOOL writeEEPROMtwoWords(uint16_t addr, uint16_t num, void *source);  
extern void read8BitsBytesFromSeprom(uint16_t addr ,void* destination, uint16_t num);
extern BOOL write8BitsBytes2seprom(unsigned int addr, unsigned char *source, int num);

#endif /* S_EEPROM_H */ 


