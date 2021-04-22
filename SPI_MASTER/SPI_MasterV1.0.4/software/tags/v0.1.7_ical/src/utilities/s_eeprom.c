/**********************************************************************************
* File name:  seprom.c
*
* File description:
*   This driver emulates a serial EEPROM by writing to internal flash.
*
*********************************************************************************/

#include <stdint.h>
#include <string.h> // memcpy 
#include "salvodefs.h"
#include "stm32f2xx.h"
#include "dmu.h"
#include "s_eeprom.h"
#include "port_def.h"
#include "uart.h"
#include "xbowsp_generaldrivers.h"
#include "boardDefinition.h"

typedef union  {
    uint8_t memory[sizeof(ConfigurationStruct) + sizeof(CalibrationStruct)];
    struct {
        ConfigurationStruct  configuration;
        uint8_t unusedMem[CALIBRATION_OFFSET - sizeof(ConfigurationStruct)];
        CalibrationStruct    calibration;
    } table;
} uEeprom; 
 
#pragma location="FLASH_BASED_EEPROM"  // defined in the linker's ICF file
__root const uEeprom gEepromInFlash  = {
    .table = {
        .configuration = {
            .port1Usage = 1, // PRI_UCB_PORT
            .calibrationCRC = 53088,
            .packetType = 0x4944, // id
        },
        .calibration = {
            .serialNumber = 0,
            .versionString = "HELLO",
        },
    },
} ;

static uEeprom gEepromShadow;

#define ERROR 1
#define NO_ERROR 0

/**********************************************************************************
* Module name:  sEEPROMtest
*
* Description: - initializes the serial eprom control lines,
*              - read status register to verify communication 
*              - check for correct data pattern
*
* Trace: 
* [SDD_SEPROM_INIT_01 <-- SRC_SEPROM_INIT] 
* [SDD_SEPROM_INIT_02 <-- SRC_SEPROM_INIT]
* [SDD_SEPROM_INIT_03 <-- SRC_SEPROM_INIT]
*
* Input parameters: none
*
* Output parameters: BIT structure set in case of error
*
* Return value: - TRUE   if no error
*               - FALSE  if errors occurred
*
*********************************************************************************/
BOOL sEEPROMtest(void)     
{
    return TRUE;
} /* end sEEPROMtest() */
 
/**********************************************************************************
 * Module name:  write8BitsBytes2seprom
 *
 * Description:
 * - write 8-bit byte stream into EEPROM
 *
 * Trace:
 * [SDD_WRITE_8BIT_BYTES_FROM_EEPROM_01  <-- SRC_WRITE_8BIT_BYTES_FROM_EEPROM ] 
 * [SDD_WRITE_8BIT_BYTES_FROM_EEPROM_02  <-- SRC_WRITE_8BIT_BYTES_FROM_EEPROM ]
 * [SDD_WRITE_8BIT_BYTES_FROM_EEPROM_03  <-- SRC_WRITE_8BIT_BYTES_FROM_EEPROM ]
 * [SDD_WRITE_8BIT_BYTES_FROM_EEPROM_04  <-- SRC_WRITE_8BIT_BYTES_FROM_EEPROM ]
 * [SDD_WRITE_8BIT_BYTES_FROM_EEPROM_05  <-- SRC_WRITE_8BIT_BYTES_FROM_EEPROM ]
 * [SDD_WRITE_8BIT_BYTES_FROM_EEPROM_06  <-- SRC_WRITE_8BIT_BYTES_FROM_EEPROM ]
 *
 * Input parameters:
 *   unsigned int startAdd: offset address to start writing;
 *       unsigned char *buffer: pointer to the 8-bit byte buffer to be written;
 *       unsigned int: number of the 8-bit bytes
 *
 * Output:
 *   None
 * Return values:
 *   error (1)
 *  no error (0)
 *********************************************************************************/
BOOL write8BitsBytes2seprom(unsigned int addr, unsigned char *source, int num)
{
    uint8_t *src = (uint8_t*) source;
    uint8_t *dst = (uint8_t*)&gEepromShadow;
    uint8_t changed = FALSE;
    
    if (addr + num > sizeof(gEepromShadow)) {
        return ERROR;
    }
    
    // read out of flash into the shadow,
    memcpy(&gEepromShadow, &gEepromInFlash, sizeof(gEepromInFlash));
    
    // set the ram shadow to new values
    while (num) {        
        if (dst[addr] != *src) {
            dst[addr] = *src;
            changed = TRUE;
        }
        addr++; src++;
        num--;
    }

    // if something changed, erase flash and write new data
    if (changed) {
        static volatile uint32_t  status = FLASH_COMPLETE;
        uint32_t start = (unsigned int) &gEepromInFlash;
        num = sizeof(gEepromInFlash);
        addr = 0;

        status = FLASH->SR;
        if ((status & 0xFE) != 0) {
            FLASH_ClearFlag(status & 0xFE);
        }
        OSDisableHook();
        FLASH_Unlock();
        status = FLASH_EraseSector(EEPROM_FLASH_SECTOR, EEPROM_FLASH_VOLTAGE);
        while (num > 0) {
            uint32_t *data = (uint32_t *) &gEepromShadow.memory[addr];
            status = FLASH_ProgramWord(start+addr, *data);
            addr+=4; num-=4;                
        }
        FLASH_Lock();
        OSEnableHook();
    }
    
    return NO_ERROR;
} /* end write8BitsBytes2seprom() */

/**********************************************************************************
 * Module name:  read8BitsBytesFromSeprom
 *
 * Description:
 * - read 8-bit byte stream from EEPROM
 *
 * Trace: 
 * [SDD_READ_8BIT_BYTES_FROM_EEPROM_1 <-- SRC_READ_8BIT_BYTES_FROM_EEPROM]
 * [SDD_READ_8BIT_BYTES_FROM_EEPROM_2 <-- SRC_READ_8BIT_BYTES_FROM_EEPROM]
 * [SDD_READ_8BIT_BYTES_FROM_EEPROM_3 <-- SRC_READ_8BIT_BYTES_FROM_EEPROM]
 *
 * Input parameters:
 *   unsigned int startAdd: offset address to start reading;
 *       unsigned char *buffer: pointer to the 8-bit byte buffer to be stored;
 *       unsigned int: number of the 8-bit bytes to be read
 *
 * Output:
 *   None
 * Return values:
 *       None
 *********************************************************************************/
void read8BitsBytesFromSeprom(uint16_t addr ,void* destination, uint16_t num)
{    
    uint8_t *dst = (uint8_t*) destination;
    if (addr + num > sizeof(gEepromInFlash)) {
        return;
    }
    while (num) {
        *dst = gEepromInFlash.memory[addr];
        addr++; dst++;
        num--;
    }
} /* end read8BitsBytesFromSeprom() */
 
 
/******************************************************
 * Function name:    writeEEPROMWords
 *
 * Description:
 *   -write multiple words into the EEPROM.
 *
 * Trace:
 * [SDD_WRITE_EEPROM_WORDS_01 <-- SRC_WRITE_EEPROM_WORDS]
 * [SDD_WRITE_EEPROM_WORDS_02 <-- SRC_WRITE_EEPROM_WORDS]
 * [SDD_WRITE_EEPROM_WORDS_03 <-- SRC_WRITE_EEPROM_WORDS]
 *
 * Input parameters:
 *       addr: the 16-bit word EEPROM address to write to.
 *       num: the number of 16-bit words to write.
 *       *source: points to the beginning of an array of data to be written.
 *
 * Output parameters:
 *       No output parameters
 *
 * Return value:
 *   FALSE: writing fails;
 *   TRUE: writing succeeds.
 *
 *******************************************************/
BOOL writeEEPROMWords(uint16_t addr, uint16_t num, void *source)  
{
    return write8BitsBytes2seprom(addr, source, num);
}  /* end writeEEPROMWords() */

/******************************************************
 * Function name:    readEEPROMWords
 *
 * Description:
 *   -read multiple-words from the EEPROM.
 *
 * Trace:
 * [SDD_READ_EEPROM_WORDS_01 <-- SRC_READ_EEPROM_WORDS]    
 * [SDD_READ_EEPROM_WORDS_02 <-- SRC_READ_EEPROM_WORDS]
 *
 * Input parameters:
 *       addr: the 16-bit word EEPROM address to read from.
 *       num: the number of 16-bit words to read.
 *       *destination: points to the beginning of an array where the EEPROM data
 *       will be put.
 *
 * Output parameters:
 *       No output parameters
 *
 * Return value:
 *   none
 *
*******************************************************/
void readEEPROMWords(uint16_t addr, uint16_t num, void *destination)
{
    read8BitsBytesFromSeprom(addr, destination, num );
} 
/* end readEEPROMWords() */

/******************************************************
 * Function name:    readEEPROMTwoWords
 *
 * Description:
 *   -read multiple two-word from the EEPROM.
 *
 * Trace:
 * [SDD_READ_EEPROM_TWO_WORDS_01 <-- SRC_READ_EEPROM_TWO_WORDS]
 * [SDD_READ_EEPROM_TWO_WORDS_02 <-- SRC_READ_EEPROM_TWO_WORDS]
 * [SDD_READ_EEPROM_TWO_WORDS_03 <-- SRC_READ_EEPROM_TWO_WORDS]
 *
 * Input parameters:
 *       addr: the 16-bit word EEPROM address to read from.
 *       num: the number of 32-bit words to read.
 *       *destination: points to the beginning of an array where the EEPROM data
 *       will be put.
 *
 * Output parameters:
 *       No output parameters
 *
 * Return value:
 *   None
 *
*******************************************************/
void readEEPROMTwoWords(uint16_t addr, uint16_t num, void *destination)
{
    // while a real EEPROM or even Flash might benefit from reading more than
    // a byte at a time, this one doesn't
    read8BitsBytesFromSeprom(addr, destination, num);

} /*end readEEPROMTwoWords() */

/******************************************************
 * Function name:    writeEEPROMtwoWords
 *
 * Description:
 *   -write multiple two-word to the EEPROM.
 *
 * Trace:
 * [SDD_WRITE_EEPROM_TWO_WORDS_01  <-- SRC_WRITE_EEPROM_TWO_WORDS]
 * [SDD_WRITE_EEPROM_TWO_WORDS_02  <-- SRC_WRITE_EEPROM_TWO_WORDS]
 * [SDD_WRITE_EEPROM_TWO_WORDS_03  <-- SRC_WRITE_EEPROM_TWO_WORDS]
 * [SDD_WRITE_EEPROM_TWO_WORDS_04  <-- SRC_WRITE_EEPROM_TWO_WORDS]
 *
 * Input parameters:
 *       addr: the 16-bit word EEPROM address to write to.
 *       num: the number of 32-bit words to write.
 *       *source: points to the beginning of an array of data to be written.
 *
 * Output parameters:
 *       No output parameters
 *
 * Return value:
 *   None
 *
*******************************************************/ 
BOOL writeEEPROMtwoWords(uint16_t addr, uint16_t num, void *source)  
{
    // while a real EEPROM or even Flash might benefit from reading more than
    // a byte at a time, this one doesn't
    return write8BitsBytes2seprom(addr, source, num);
}  /* end  writeEEPROMtwoWords() */



