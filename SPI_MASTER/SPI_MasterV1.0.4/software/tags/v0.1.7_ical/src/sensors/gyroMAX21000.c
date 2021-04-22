/*****************************************************************************
** Gyroscope interface for the Maxim 21000 gyro
** Some features of interest for implementation:
**  * SPI, up to 10MHz
*****************************************************************************/
#include <stdint.h>
#include "salvodefs.h"
#include "gyroscope.h"
#include "gyroMAX21000.h"
#include "spi.h"
//#define LOGGING_LEVEL LEVEL_INFO
#include "debug.h"
#include "dmu.h"
#include "boardDefinition.h"
#include "timer.h"

// readings are 2 bytes per axis, plus one for read command, plus two for temp
#define GYRO_BUFFER_SIZE ((NUM_AXIS *sizeof(uint16_t) + 1) +2)

static uint8_t gGyroDataBuffer[GYRO_BUFFER_SIZE]; 

static uint8_t _ReadMAX21000Register(uint8_t address)
{   
    uint8_t rx[2];
    uint8_t error;
    
    address |= READ_INDICATION | SINGLE_ADD_BURST;
    
    rx[0] = address;
    GPIO_ResetBits(MAXGYRO_SELECT_PORT, MAXGYRO_SELECT_PIN);
    error = spi_transfer(kGyroSPI, rx , rx, sizeof(rx));
    while (!spi_done(kGyroSPI)) {/* spin */;} 
    GPIO_SetBits(MAXGYRO_SELECT_PORT, MAXGYRO_SELECT_PIN);   
    
    INFO_HEX("\trxa  ", address);
    INFO_HEX(" rx0 ", rx[1]);
    INFO_ENDLINE();
    
    if (error) { ERROR_STRING("Error in _ReadMAX21000Register\r\n"); }
    
    return rx[1];  
}
static uint8_t _WriteMAX21000Register(uint8_t address, uint8_t data)
{    
    uint8_t error;
    uint8_t tx[2];
    tx[0] = address;
    tx[1] = data;

    GPIO_ResetBits(MAXGYRO_SELECT_PORT, MAXGYRO_SELECT_PIN);    
    error = spi_transfer(kGyroSPI, tx , tx, sizeof(tx));
    while (!spi_done(kGyroSPI)) {/* spin */;} 
    GPIO_SetBits(MAXGYRO_SELECT_PORT, MAXGYRO_SELECT_PIN);
    
    INFO_HEX("\n\tWrote ", data);
    INFO_HEX("  to  ", address);
    INFO_ENDLINE();
    INFO_HEX("\tRead  ", _ReadMAX21000Register(address));
    INFO_HEX(" from ", address);
    INFO_ENDLINE();

    return error;
}

void _GyroDataReadyInt(FunctionalState enable)
{
    NVIC_InitTypeDef NVIC_InitStructure;

    /* Enable and set EXTI Interrupt to the lowest priority */
    NVIC_InitStructure.NVIC_IRQChannel = MAXGYRO_DATA_READY_EXTI_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x0F;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x0F;
    NVIC_InitStructure.NVIC_IRQChannelCmd = enable;
    NVIC_Init(&NVIC_InitStructure);     
}

static void GyroDataReadyIRQ(void)
{
    // need to start read as one command, let SPI and DMA finish it
    // currently, this does the read here since spi_transfer finishes
	GPIO_ResetBits(MAXGYRO_SELECT_PORT, MAXGYRO_SELECT_PIN);
    gGyroDataBuffer[0] = GYRO_X_MSB_REG | READ_INDICATION;
    spi_transfer(kGyroSPI, gGyroDataBuffer, gGyroDataBuffer, sizeof(gGyroDataBuffer));
}

void EXTI1_IRQHandler(void)
{
    OSEnterCritical();
    GyroDataReadyIRQ();
    EXTI_ClearITPendingBit(MAXGYRO_DATA_READY_EXTI_LINE);
    OSLeaveCritical();    
}

uint8_t GyroStartReading()
{
    int i;
    // clear the buffer so we don't send anything untoward
    for (i = 0; i < sizeof(gGyroDataBuffer); i++) {
        gGyroDataBuffer[i] = 0;
    }
    GPIO_SetBits(MAXGYRO_SYNC_PORT, MAXGYRO_SYNC_PIN);    
    _GyroDataReadyInt(ENABLE);
    // set sync line
    return TRUE;
}

uint8_t IsGyroDoneReading()
{
    OStypeEFlag eFlag = OSReadEFlag(EFLAGS_DATA_READY);
    return eFlag & EF_DATA_GYRO_READY;
}

static void _GyroDMADoneCallback()
{
    GPIO_SetBits(MAXGYRO_SELECT_PORT, MAXGYRO_SELECT_PIN);    
    OSSetEFlag(EFLAGS_DATA_READY, EF_DATA_GYRO_READY);
}

uint8_t GyroGetLastReading(int16_t *readings)
{
    uint8_t *data = &(gGyroDataBuffer[1]);
    int i;

    _GyroDataReadyInt(DISABLE);
    
    GPIO_ResetBits(MAXGYRO_SYNC_PORT, MAXGYRO_SYNC_PIN);    
    if (readings) {
        for (i = 0; i < 6; i++) {
            INFO_HEX(" ", data[i]);
        }
        readings[0] = (data[4] << 8) | data[5];
        readings[1] = (data[0] << 8) | data[1];
        readings[2] = (data[2] << 8) | data[3];
    }   
    return 0;
}

uint8_t GyroWhoAmI(uint32_t *whoami)
{
    uint8_t rx = 0;
    uint8_t retries = 3;
    spi_go_slow(kGyroSPI);

    while (retries && rx !=  WHO_AM_I) {
        rx  = _ReadMAX21000Register(WHO_AM_I_REG);
        retries--;
    }
    
    if (whoami) {
        *whoami  = rx;
    }
    
    spi_go_fast(kGyroSPI);
    return rx == WHO_AM_I;
}
 
uint16_t GyroGetGain()
{
    uint8_t config;
    spi_go_slow(kGyroSPI);
	_WriteMAX21000Register(BANK_SEL_REG, BANK_00);
	config = _ReadMAX21000Register(SENSE_CFG0);
	config &= FS_MASK;
    spi_go_fast(kGyroSPI);
	
	switch (config) {
	case FS_2000_DPS:
		return 15;
	case FS_1000_DPS:
		return 30;
	case FS_500_DPS:
		return 60;
	case FS_250_DPS:
		return 120;
	}
    spi_go_fast(kGyroSPI);
	return TRUE;
}

uint8_t  GyroSelfTest()
{
    int i;
    uint16_t serialNumberWord;
    uint8_t error;
   
    error = _WriteMAX21000Register(BANK_SEL_REG, BANK_01);
    DEBUG_STRING("\r\n\tMAX21000 Serial number: ");
    for (i = 0; i < 6; i+=2) {
        serialNumberWord = _ReadMAX21000Register(SERIAL_0_REG + i);
        serialNumberWord = serialNumberWord << 8;
        serialNumberWord |= _ReadMAX21000Register(SERIAL_0_REG + i + 1);
        DEBUG_HEX(" ", serialNumberWord);
    }
    DEBUG_STRING("\r\n\t");

    // return back to normal
	error = _WriteMAX21000Register(BANK_SEL_REG, BANK_00);

    if (error) {return FALSE;}
    
    return GyroWhoAmI(NULL);
}

uint8_t GyroTempExists() 
{
    return TRUE; // MPU-3300 has temperature sensing
}
uint8_t GyroTempGetLastReading(int16_t* reading)
{
    
    uint8_t *data = &(gGyroDataBuffer[7]);
    _GyroDataReadyInt(DISABLE);
    
    if (reading) {
        reading[0] = (data[0] << 8) | data[1];
    }   
    return 0;
}
uint8_t GyroTempGetTemperature(int16_t reading, float *out)
{
    float temperature;
    // Temperature in degrees 
    // C = (TEMP_OUT Register Value as a signed quantity)/340 + 36.53
    temperature = (float) reading / (float) 256;
    *out = temperature;
    return 0; // no error
}


void _InitGyroDataReadyInt()
{
    GPIO_InitTypeDef GPIO_InitStructure;
    EXTI_InitTypeDef EXTI_InitStructure;
    
    RCC_AHB1PeriphClockCmd(MAXGYRO_DATA_READY_CLK, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE); // for interrupts
    
    /* Configure data ready pin as input */
    GPIO_StructInit(&GPIO_InitStructure);
    GPIO_InitStructure.GPIO_Pin = MAXGYRO_DATA_READY_PIN; 
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_Init(MAXGYRO_DATA_READY_PORT, &GPIO_InitStructure);    
    
    /* Configure EXTI line */
    EXTI_StructInit(&EXTI_InitStructure);
    EXTI_InitStructure.EXTI_Line = MAXGYRO_DATA_READY_EXTI_LINE;
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;  
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    EXTI_Init(&EXTI_InitStructure);

    /* Connect EXTI Line to GPIO Pin */
    SYSCFG_EXTILineConfig(MAXGYRO_DATA_READY_EXTI_PORT_SOURCE, MAXGYRO_DATA_READY_EXTI_PIN_SOURCE);
    
    _GyroDataReadyInt(DISABLE);
        
}


uint8_t GyroConfig(uint32_t *rangeInDps, uint32_t *outputDataRate)
{
    uint8_t config;
    uint8_t error;
    uint16_t odr;

    if (*rangeInDps >= 2000) {
        config = FS_2000_DPS;
        *rangeInDps = 2000;
    } else if (*rangeInDps >= 1000) {
        config = FS_1000_DPS;
        *rangeInDps = 1000;
    } else if (*rangeInDps >= 500) {
        config = FS_500_DPS;
        *rangeInDps = 500;
    } else {
        config = FS_250_DPS;
        *rangeInDps = 250;
    }
    
    spi_go_slow(kGyroSPI);
	error = _WriteMAX21000Register(BANK_SEL_REG, BANK_00);
    config |= EN_ALL_RATE | PW_SL_TO_NM ;       
    error += _WriteMAX21000Register(SENSE_CFG0, config);
    
    if (*outputDataRate >= 100) { // 100 Hz
        // ODR = 10kHz/(n+1) where n is the config value
        odr = (10000 / *outputDataRate) - 1;
        *outputDataRate = 10000 / (odr+1) ;
    } else if (*outputDataRate >= 20) { // 20 Hz
        // ODR = 10kHz/(100+5*(n-99)) where n is the config value
        odr = (2000 / *outputDataRate) + 79;
        *outputDataRate = 10000 /(100 + 5*(odr-99)) ;
    } else { // < 20 Hz
       //  ODR = 10kHz/(100+20*(n-179))
        odr = (500 / *outputDataRate) + 154;
        if (odr > 0xFF) { odr = 0xFF; } // max value
        *outputDataRate = 10000 /(100 + 20*(odr-179)) ;
    }
    
    config = odr & 0xFF;
    error += _WriteMAX21000Register(SENSE_CFG2, config);   

    spi_go_fast(kGyroSPI);
    return (error == 0);
}


uint8_t InitGyro()
{
    uint8_t config;
    uint8_t error;
    GPIO_InitTypeDef GPIO_InitStructure;
  
    error = spi_configure(kGyroSPI, SPI_CPOL_AND_CPHA_HIGH, &_GyroDMADoneCallback);
    spi_go_slow(kGyroSPI);
    if (error) { return error; }

    _InitGyroDataReadyInt();
        
    RCC_AHB1PeriphClockCmd(MAXGYRO_SELECT_CLK, ENABLE);
    RCC_AHB1PeriphClockCmd(MAXGYRO_SYNC_CLK, ENABLE);
    
    GPIO_InitStructure.GPIO_Pin = MAXGYRO_SYNC_PIN;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(MAXGYRO_SELECT_PORT, &GPIO_InitStructure);
    GPIO_ResetBits(MAXGYRO_SYNC_PORT, MAXGYRO_SYNC_PIN);    

    GPIO_InitStructure.GPIO_Pin = MAXGYRO_SELECT_PIN;
    GPIO_Init(MAXGYRO_SELECT_PORT, &GPIO_InitStructure);

    GPIO_SetBits(MAXGYRO_SELECT_PORT, MAXGYRO_SELECT_PIN);    
    DelayMs(2);
    
	// select register bank 0
	error = _WriteMAX21000Register(BANK_SEL_REG, BANK_00);
    config = EN_ALL_RATE | PW_SL_TO_NM | FS_250_DPS ;       
    error += _WriteMAX21000Register(SENSE_CFG0, config);
    config = SNS_BW_75;
    error += _WriteMAX21000Register(SENSE_CFG1, config);
    error += _WriteMAX21000Register(I2C_CFG, I2C_OFF );    

    if (error) {
        ERROR_INT("GyroMax Init Errors ", error);
        ERROR_ENDLINE();
    }    
    return GyroWhoAmI(NULL);
}
