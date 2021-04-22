/*****************************************************************************
** Gyroscope interface for the Invensense MPU3300 Gyro
** Some features of interest for implementation:
**  * 1Mhz SPI for most communication, up to 20MHz SPI for reading
**    interrupt registers and output data
**  * User-programmable gyroscope range of ±225 and ±450°/sec, 16 bit
**  * Built in self test
*****************************************************************************/
#include <stdint.h>
#include "salvodefs.h"
#include "gyroscope.h"
#include "gyroMPU3300.h"
#include "spi.h"
//#define LOGGING_LEVEL LEVEL_INFO
#include "debug.h"
#include "dmu.h"
#include "boardDefinition.h"
#include "timer.h"
static uint8_t gGyro2DataBuffer[NUM_AXIS *2 + 1]; // readings are 2 bytes per axis, plus one for read command
static int32_t gGyro2AccumulationBuffer[NUM_AXIS];
static uint16_t gGyro2AccumulationCount;
static uint16_t gGyro2DataGathering = FALSE;

static uint8_t _ReadMPU3300Register(uint8_t address, uint8_t waitForFinish)
{   
    uint8_t rx[2];
    uint8_t error;

    GPIO_ResetBits(INVGYRO_SELECT_PORT, INVGYRO_SELECT_PIN);
    address |= READ_INDICATION;
    rx[0] = address;
    rx[1] = 0;
    error = spi_transfer(kGyroSPI, rx , rx, sizeof(rx));
    if (waitForFinish) {while (!spi_done(kGyroSPI)) {/* spin */;} }
    // when spi finishes, DMA int will set the select pin to deselect the chip

    INFO_HEX("\trxa  ", address);
    INFO_HEX(" rx0 ", rx[0]);
    INFO_HEX(" rx1 ", rx[1]);
    INFO_ENDLINE();

    if (error) { ERROR_STRING("Error in _ReadMPU3300Register\r\n"); }
   
   return rx[1];  
}
static uint8_t _WriteMPU3300Register(uint8_t address, uint8_t data)
{    
    uint8_t error;
    uint8_t tx[2];
    tx[0] = address;
    tx[1] = data;

    GPIO_ResetBits(INVGYRO_SELECT_PORT, INVGYRO_SELECT_PIN);    
    error = spi_transfer(kGyroSPI, tx , tx, sizeof(tx));
    while (!spi_done(kGyroSPI)) {/* spin */;} 
    GPIO_SetBits(INVGYRO_SELECT_PORT, INVGYRO_SELECT_PIN);
    
    INFO_HEX("\n\tWrote ", data);
    INFO_HEX("  to  ", address);
    INFO_ENDLINE();
    INFO_HEX("\tRead  ", _ReadMPU3300Register(address, TRUE));
    INFO_HEX(" from ", address);
    INFO_ENDLINE();

    return error;
}


void Gyro2DataReadyIRQ(void)
{
    if (gGyro2DataGathering) {
        int16_t val; // need to make positives and negaitves work out
        int i;
        uint8_t *data = &gGyro2DataBuffer[1]; 
     
       // clear old data, ready for new
        for (i = 0; i < NUM_AXIS; i++ ) {
            val = (data[2*i] << 8)  | data[ (2*i) + 1];
            data[2*i] = data[ (2*i) + 1] = 0; // clear buffer for next read
            gGyro2AccumulationBuffer[i] = val;
        }
        gGyro2AccumulationCount++;
        GPIO_ResetBits(INVGYRO_SELECT_PORT, INVGYRO_SELECT_PIN);
        gGyro2DataBuffer[0] = GYRO_XOUT_H | READ_INDICATION;      
        spi_transfer(kGyroSPI, gGyro2DataBuffer , gGyro2DataBuffer, sizeof(gGyro2DataBuffer));
    }    
}
void _Gyro2DMACallback()
{
    if (gGyro2DataGathering) {
        GPIO_SetBits(INVGYRO_SELECT_PORT, INVGYRO_SELECT_PIN);
        OSSetEFlag(EFLAGS_DATA_READY, EF_DATA_GYRO_READY);
    }
}    

// This doesn't kick off a conversion cycle so the sampling for the 
// Invensense gyro is asynchronous to the system, it just makes it so 
// the data ready line goes (but it goes whenever it was ready). A 
// better fix for this would be to set the system at 8kHz and set up the FIFO.
// On StartReading, clear the FIFO. When the other sensors have completed 
// conversion, read out the FIFO and average the signals. (This is kind of how
// the 440 worked.)
uint8_t Gyro2StartReading()
{
    int i;
    
    for (i = 0; i < NUM_AXIS; i++) {
        gGyro2AccumulationBuffer[i] = 0;
    }
    gGyro2AccumulationCount = 0;

// FIXME: this shouldn't be here as this is called from an interrupt
// in normal data aquisition
// Need to turn off data ready line
    while (!spi_done(kGyroSPI)) {;}

    OSEnterCritical();
// FIXME: There is a timing issue here that makes gyro2 not happy on some 
// starts, particularly those that are out of sync (i.e. from the command line)
// The order of operations can be bad where a data ready interrupt happens
// as we are reading the register to allow the next data ready
    // To fix: diagram out how it happens, figure out what order needs to change
    // I think that gGyro2DataGathering needs a state that is "I'm waiting for 
    // the dummy read to complete before accepting data ready interrrupts". That
    // would be the state set here and the _Gyro2DMACallback would see it and
    // go on to the "data gathering" state that would allow the Gyro2DataReadyIRQ
    // to accumulate data.
    EXTI_ClearITPendingBit(INVGYRO_DATA_READY_EXTI_LINE);    
    gGyro2DataGathering = TRUE;
    _ReadMPU3300Register(0, FALSE); // clear latched int, any read will do it
    OSLeaveCritical();

    return TRUE;
}

uint8_t IsGyro2DoneReading()
{
    return gGyro2AccumulationCount > 1; 
}

uint8_t Gyro2GetLastReading(int16_t *readings)
{
    uint8_t error = 0;
    int i;

    gGyro2DataGathering = FALSE;
    INFO_INT("cnt  ",gGyro2AccumulationCount);
    for (i = 0; i < NUM_AXIS; i++) {
        gGyro2AccumulationBuffer[i] /=  gGyro2AccumulationCount; // ave
        INFO_HEX(" ", gGyro2AccumulationBuffer[i]);
    }
    INFO_STRING("   ");
    
    
    if (!error && readings) {
        for (i = 0; i < NUM_AXIS; i++) {
            readings[i] =  gGyro2AccumulationBuffer[i];
        }   
    }
    return error;
}

uint8_t Gyro2WhoAmI(uint32_t *whoami)
{
    uint8_t rx = 0;
    uint8_t retries = 3;
    spi_go_slow(kGyroSPI);
    
    while (retries && rx !=  WHO_AM_I_DEFAULT) {
        rx  = _ReadMPU3300Register(WHO_AM_I, TRUE);
        retries--;
    }
    
    if (whoami) {
        *whoami  = rx;
    }
    
    spi_go_fast(kGyroSPI);
    return rx == WHO_AM_I_DEFAULT;
}
 
// return LSB*10/degrees per second
uint16_t Gyro2GetGain()
{
    uint8_t config;
    spi_go_slow(kGyroSPI);
    config = _ReadMPU3300Register(GYRO_CONFIG, TRUE);
    spi_go_fast(kGyroSPI);

    if (config & GYRO_CONFIG_450DPS_FS) {
        // range is ± 450 °/s. That means 72.8 LSB/°/s
        return 728; // 72.8 LSB/°/s
    } else {
        // range is ± 225 °/s. That means 72.8 LSB/°/s
        return 1456; // 145.6LSB/°/s
    }    
}

uint8_t  Gyro2SelfTest()
{
    uint8_t testX, testY, testZ;
    uint8_t config, origConfig;
    uint8_t error = 0;

    spi_go_slow(kGyroSPI);

    origConfig = _ReadMPU3300Register(GYRO_CONFIG, TRUE);
    
    config = GYRO_CONFIG_225DPS_FS;
    config |= GYRO_CONFIG_X_TEST;
    config |= GYRO_CONFIG_Y_TEST;
    config |= GYRO_CONFIG_Z_TEST;
    
    error = _WriteMPU3300Register(GYRO_CONFIG, config);

    testX = _ReadMPU3300Register(SELF_TEST_X, TRUE);
    testY = _ReadMPU3300Register(SELF_TEST_Y, TRUE);
    testZ = _ReadMPU3300Register(SELF_TEST_Z, TRUE);
    
    DEBUG_INT("\r\n\tTest x = ", testX);
    DEBUG_INT("\r\n\tTest y = ", testY);
    DEBUG_INT("\r\n\tTest Z = ", testZ);
    DEBUG_INT("\r\n\terrors = ", error);
    DEBUG_ENDLINE();

    // return to normal
    _WriteMPU3300Register(GYRO_CONFIG, origConfig);
    _WriteMPU3300Register(SIGNAL_PATH_RESET, 
                          SIGNAL_PATH_RESET_TEMP | 
                          SIGNAL_PATH_RESET_GYRO);
    
        
    return Gyro2WhoAmI(NULL);
}

uint8_t Gyro2TempExists() 
{
    return TRUE; // MPU-3300 has temperature sensing
}
uint8_t Gyro2TempGetLastReading(int16_t* reading)
{
    uint8_t data[2];
    uint8_t address = TEMP_OUT_H | READ_INDICATION;
    uint8_t error;
    GPIO_ResetBits(INVGYRO_SELECT_PORT, INVGYRO_SELECT_PIN);
    error = spi_transfer(kGyroSPI, NULL , &address, 1);
    while (!spi_done(kGyroSPI)) {/* spin */;} 
    error += spi_transfer(kGyroSPI, NULL , NULL, 1); // dummy read
    while (!spi_done(kGyroSPI)) {/* spin */;} 
    error += spi_transfer(kGyroSPI, data , NULL, sizeof(data));
    while (!spi_done(kGyroSPI)) {/* spin */;} 
    GPIO_SetBits(INVGYRO_SELECT_PORT, INVGYRO_SELECT_PIN);
    
    if (error) ERROR_STRING(" Error in Gyro2TempGetLastReading\r\n");
    
    if (!error && reading) {
        *reading = (data[0] << 8) | data[1];
    }   
    return error;
}
uint8_t Gyro2TempGetTemperature(int16_t reading, float *out)
{
    float temperature;
    // Temperature in degrees 
    // C = (TEMP_OUT Register Value as a signed quantity)/340 + 36.53
    temperature = (float) reading / (float) 340;
    temperature += 36.53;
    *out = temperature;
    return 0; // no error
}

uint8_t Gyro2Config(uint32_t *rangeInDps)
{
    uint8_t config, readConfig;
    uint8_t error;

    spi_go_slow(kGyroSPI);
 
    if (*rangeInDps > 250) {
        config = GYRO_CONFIG_450DPS_FS;
        *rangeInDps = 450;
    } else {
        config = GYRO_CONFIG_225DPS_FS;
        *rangeInDps = 225;
    }    
    error = _WriteMPU3300Register(GYRO_CONFIG, config);
    DelayMs(5);
    readConfig = _ReadMPU3300Register(GYRO_CONFIG, TRUE);
    
    if (readConfig != config) {
        DEBUG_HEX("readConfig ", readConfig);
        DEBUG_HEX(" != intended ", config);
        DEBUG_ENDLINE();
        error += 1;
    }
        
    error += _WriteMPU3300Register(SIGNAL_PATH_RESET, 
                          SIGNAL_PATH_RESET_TEMP | 
                          SIGNAL_PATH_RESET_GYRO);

    DelayMs(5);
    _WriteMPU3300Register(INT_PIN_CFG, INT_CFG_READ_CLEAR);
    DelayMs(10);
    _WriteMPU3300Register(INT_ENABLE, INT_DATA_RDY);
    DelayMs(5);
    spi_go_fast(kGyroSPI);

    return (error == 0);
}

void _InitGyro2DataReadyInt()
{
    GPIO_InitTypeDef GPIO_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;
    EXTI_InitTypeDef EXTI_InitStructure;
    
    RCC_AHB1PeriphClockCmd(INVGYRO_DATA_READY_CLK, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE); // for interrupts
    
    /* Configure data ready pin as input */
    GPIO_StructInit(&GPIO_InitStructure);
    GPIO_InitStructure.GPIO_Pin = INVGYRO_DATA_READY_PIN; 
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;    
    GPIO_Init(INVGYRO_DATA_READY_PORT, &GPIO_InitStructure);

    /* Configure EXTI line */
    EXTI_StructInit(&EXTI_InitStructure);
    EXTI_InitStructure.EXTI_Line = INVGYRO_DATA_READY_EXTI_LINE;
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;  
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    EXTI_Init(&EXTI_InitStructure);    

    /* Connect EXTI Line to GPIO Pin */
    SYSCFG_EXTILineConfig(INVGYRO_DATA_READY_EXTI_PORT_SOURCE, INVGYRO_DATA_READY_EXTI_PIN_SOURCE);


    // Unlike with the MAX21000, this can't use NVIC to turn on
    // and off the interrupt because it is shared with other 
    // data ready lines. Instead, use gGyro2DataGathering boolean
    NVIC_InitStructure.NVIC_IRQChannel = INVGYRO_DATA_READY_EXTI_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x0F;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x0F;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);     
    
    gGyro2DataGathering = FALSE;
    
}
uint8_t InitGyro2()
{
    uint8_t error;
    GPIO_InitTypeDef GPIO_InitStructure;
  
    error = spi_configure(kGyroSPI, SPI_CPOL_AND_CPHA_LOW, _Gyro2DMACallback);
    spi_go_slow(kGyroSPI);
    if (error) { return error; }
    _InitGyro2DataReadyInt();
    
    RCC_AHB1PeriphClockCmd(INVGYRO_SELECT_CLK, ENABLE);
    
    GPIO_SetBits(INVGYRO_SELECT_PORT, INVGYRO_SELECT_PIN);
    GPIO_InitStructure.GPIO_Pin = INVGYRO_SELECT_PIN;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(INVGYRO_SELECT_PORT, &GPIO_InitStructure);
    
    
    _WriteMPU3300Register(PWR_MGMT_1, PWR_MGMT_1_DEVICE_RESET);
    DelayMs(10);
    // device resets into sleep mode, need to clear that
    _WriteMPU3300Register(PWR_MGMT_1, 0);

    // set I2C_IF_DIS in reg so Gyro doesn't go to I2C mode when CS released
    _WriteMPU3300Register(USER_CTRL, USER_CTRL_DISABLE_I2C);
    DelayMs(5);
      
    _WriteMPU3300Register(SIGNAL_PATH_RESET, 
                          SIGNAL_PATH_RESET_TEMP | 
                          SIGNAL_PATH_RESET_GYRO);
    _WriteMPU3300Register(INT_ENABLE, 0);
    DelayMs(5);
    _WriteMPU3300Register(INT_PIN_CFG, INT_CFG_READ_CLEAR);
    DelayMs(10);
    _WriteMPU3300Register(INT_ENABLE, INT_DATA_RDY);
    DelayMs(5);
    
    return Gyro2WhoAmI(NULL);
}
