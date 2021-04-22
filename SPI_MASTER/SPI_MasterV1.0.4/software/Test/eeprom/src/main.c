/****************************************************************************
**
**  File        : main.c
**
**  Abstract    : main function.
**
**  Functions   : main
**
**  Environment : Atollic TrueSTUDIO/STM32
**                STMicroelectronics STM32F10x Standard Peripherals Library
**
**  Distribution: This file is used to test the eeprom driver.
**
*****************************************************************************/

#include <stddef.h>
#include "stm32f10x.h"
#include "i2c_eeprom.h"

/* Private define */
#define EEPROM_WriteAddress1    0x05
#define EEPROM_ReadAddress1     0x05
#define BufferSize1             (countof(Tx1_Buffer)-1)
#define BufferSize2             (countof(Tx2_Buffer)-1)
#define EEPROM_WriteAddress2    (EEPROM_WriteAddress1 + BufferSize1)
#define EEPROM_ReadAddress2     (EEPROM_ReadAddress1 + BufferSize1)
#define countof(a) (sizeof(a) / sizeof(*(a)))

typedef enum {FAILED = 0, PASSED = !FAILED} TestStatus;
uint8_t Tx1_Buffer[] = "/* STM32F10x I2C Firmware ";
uint8_t Tx2_Buffer[] = "Library Example */";
uint8_t Rx1_Buffer[BufferSize1], Rx2_Buffer[BufferSize2];
volatile TestStatus TransferStatus1 = FAILED, TransferStatus2 = FAILED;

void RCC_Configuration(void);
TestStatus Buffercmp(uint8_t* pBuffer1, uint8_t* pBuffer2, uint16_t BufferLength);

/*******************************************************************************
  Main program
*******************************************************************************/
int main(void)
{
  /* System clocks configuration */
  RCC_Configuration();

  /* Initialize the I2C EEPROM driver */
  I2C_EE_Init();

  /* First write in the memory followed by a read of the written data */
  /* Write on I2C EEPROM from EEPROM_WriteAddress1 */
  I2C_EE_BufferWrite(Tx1_Buffer, EEPROM_WriteAddress1, BufferSize1);

  /* Read from I2C EEPROM from EEPROM_ReadAddress1 */
  I2C_EE_BufferRead(Rx1_Buffer, EEPROM_ReadAddress1, BufferSize1);

  /* Check if the data written to the memory is read correctly */
  TransferStatus1 = Buffercmp(Tx1_Buffer, Rx1_Buffer, BufferSize1);
  /* TransferStatus1 = PASSED, if the transmitted and received data
     to/from the EEPROM are the same */
  /* TransferStatus1 = FAILED, if the transmitted and received data
     to/from the EEPROM are different */

  /* Wait for EEPROM standby state */
  I2C_EE_WaitEepromStandbyState();

  /* Second write in the memory followed by a read of the written data */
  /* Write on I2C EEPROM from EEPROM_WriteAddress2 */
  I2C_EE_BufferWrite(Tx2_Buffer, EEPROM_WriteAddress2, BufferSize2);

  /* Read from I2C EEPROM from EEPROM_ReadAddress2 */
  I2C_EE_BufferRead(Rx2_Buffer, EEPROM_ReadAddress2, BufferSize2);

  /* Check if the data written to the memory is read correctly */
  TransferStatus2 = Buffercmp(Tx2_Buffer, Rx2_Buffer, BufferSize2);
  /* TransferStatus2 = PASSED, if the transmitted and received data
     to/from the EEPROM are the same */
  /* TransferStatus2 = FAILED, if the transmitted and received data
     to/from the EEPROM are different */

  while (1)
  {
  }
}

/*******************************************************************************
   Distribution: Configures the different system clocks.
   Input: None
   Output: None
*******************************************************************************/
void RCC_Configuration(void)
{
  /* Setup the microcontroller system. Initialize the Embedded Flash Interface,
     initialize the PLL and update the SystemFrequency variable. */
  SystemInit();

  /* Enable peripheral clocks */
  /* GPIOB Periph clock enable */
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
  /* I2C1 Periph clock enable */
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1, ENABLE);
}

TestStatus Buffercmp(uint8_t* pBuffer1, uint8_t* pBuffer2, uint16_t BufferLength)
{
  while(BufferLength--)
  {
    if(*pBuffer1 != *pBuffer2)
    {
      return FAILED;
    }
    pBuffer1++;
    pBuffer2++;
  }
  return PASSED;
}

/**************************************************************************************
* Minimal __assert_func used by the assert() macro
**************************************************************************************/
void __assert_func(const char *file, int line, const char *func, const char *failedexpr)
{
	while(1)
	{}
}

/**************************************************************************************
* Minimal __assert() uses __assert__func()
**************************************************************************************/
void __assert(const char *file, int line, const char *failedexpr)
{
	__assert_func (file, line, NULL, failedexpr);
}
