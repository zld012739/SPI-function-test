/*****************************************************************************
** Magnetometer interface for the Honeywell HMC5883L magnetometer
** Some features of interest for implementation:
**  * Fast 160 Hz Maximum Output Rate
**  * Built-In Self Test
**  * I2C should run at 400 kHz
**
** The normal method for reading is:
**  * Call MagnetometerStartReading, probably from a timer interrupt
**  * Receive data ready interrupt, this triggers an I2C read
**  * Get I2C read complete flag set in OS (or poll IsMagnetometerDoneReading()
**  * Call MagnetometerGetLastReading to get the data out
** Repeat
*****************************************************************************/
#include <stdint.h>
#include "salvodefs.h"
#include "magnetometer.h"
#include "magHMC5883L.h"
#include "i2c.h"
#include "debug.h"
#include "dmu.h"
#include "boardDefinition.h"

#define I2C_TIMEOUT 100000

static uint8_t gMagnetometerDataBuffer[NUM_AXIS *2]; // readings are 2 bytes per axis

/*************************************************************************
* Function Name: _MagWriteReg : blocking write via I2C to magnetometer
* Parameters: address and value
* Return: 0 if success, error otherwise
**************************************************************************/
static uint8_t _MagWriteReg(uint8_t addr, uint8_t value)
{
    uint8_t data[2];
    uint32_t error;
    uint32_t timeout = I2C_TIMEOUT;
    
    data[0] = addr;
    data[1] = value;
    i2c_open(kMagnetometerI2C, NULL);
    i2c_data_send(kMagnetometerI2C, HMC5883L_I2C_ADDR, 
                  data, sizeof(data));
    while (!i2c_is_done(kMagnetometerI2C) && timeout) {timeout--;}
    error = i2c_has_error(kMagnetometerI2C);
    if (!timeout) { error = TRUE; }
    i2c_close(kMagnetometerI2C);
    return error;
}    

/*************************************************************************
* Function Name: _MagWriteReg : blocking read via I2C to magnetometer
* Parameters: address and pointer value
* Return: 0 if success, error otherwise
**************************************************************************/
static uint8_t _MagReadReg(uint8_t addr, uint8_t *value)
{
    uint32_t error;
    uint32_t timeout = I2C_TIMEOUT;
    i2c_open(kMagnetometerI2C, NULL);
    i2c_data_request( kMagnetometerI2C, HMC5883L_I2C_ADDR, addr,
                     value, sizeof(*value));
    while (!i2c_is_done(kMagnetometerI2C) && timeout) {timeout--;}
    error = i2c_has_error(kMagnetometerI2C);
    if (!timeout) {error = TRUE; }
    i2c_close(kMagnetometerI2C);
    return error;
}    
/*************************************************************************
* Function Name: _MagnetometerDataReceivedCallback : 
* Description: Called from the I2C interrupt when the data has been
*   obtained, this is the last step in getting a sample
* Parameters: none
* Return: none
**************************************************************************/
static void _MagnetometerDataReceivedCallback(void)
{
    OSSetEFlag(EFLAGS_DATA_READY, EF_DATA_MAG_READY);
    i2c_close(kMagnetometerI2C);
}
    
/*************************************************************************
* Function Name: MagnetomterDataReadyIRQ
* Description: This interrupt indicates the data is ready on the 
*  magenetomter. Kicks off reading data.
* Parameters: none
* Return: 0 if success, error otherwise
**************************************************************************/
void MagnetomterDataReadyIRQ(void)
{
    i2c_open(kMagnetometerI2C, _MagnetometerDataReceivedCallback);
    i2c_data_request( kMagnetometerI2C, HMC5883L_I2C_ADDR, HMC5883L_X_MSB,
                     gMagnetometerDataBuffer, sizeof(gMagnetometerDataBuffer));
}

/*************************************************************************
* Function Name: MagnetometerStartReading
* Description: This kicks off conversion of magnetometers
* Parameters: none
* Return: 0 if success, error otherwise
**************************************************************************/
uint8_t MagnetometerStartReading()
{
    static uint8_t data[2]; // static so the memory stays available to I2C
    
    data[0] = HMC5883L_MODE;
    data[1] = HMC5883L_MODE_READ_SINGLE;

    i2c_open(kMagnetometerI2C, NULL);
    i2c_data_send(kMagnetometerI2C, HMC5883L_I2C_ADDR, 
                  data, sizeof(data));
    i2c_close(kMagnetometerI2C);    
    return 0;
}

/*************************************************************************
* Function Name: IsMagnetometerDoneReading
* Description: Checks to see if magnetometer transaction is complete
* Parameters: none
* Return: 0 if success, error otherwise
**************************************************************************/
uint8_t IsMagnetometerDoneReading()
{
    OStypeEFlag eFlag = OSReadEFlag(EFLAGS_DATA_READY);
    return eFlag & EF_DATA_MAG_READY;
}

/*************************************************************************
* Function Name: MagnetometerGetLastReading
* Description: Gets the data that was last read by I2C process
* Parameters: array of reading (NUM_AXIS)
* Return: 0 if success, error otherwise
**************************************************************************/
uint8_t MagnetometerGetLastReading(int16_t *readings)
{
    uint8_t *data = gMagnetometerDataBuffer;
    uint32_t error;
            
    error = i2c_has_error(kMagnetometerI2C);
        
    if (!error && readings) {
        readings[0] = data[0] << 8 | data[1];
        readings[1] = data[2] << 8 | data[3];
        readings[2] = data[4] << 8 | data[5];
    }
    return error;
}

/*************************************************************************
* Function Name: MagnetometerWhoAmI
* Description: Verify magnetometer attached is one we know how to talk to
* Parameters: pointer to whoami value, data will be returned if this is not NULL
* Return: TRUE if successful, FALSE if error
**************************************************************************/
uint8_t MagnetometerWhoAmI(uint32_t *whoami)
{
    uint8_t data[3];
    uint32_t error;
    uint32_t timeout = I2C_TIMEOUT;

    i2c_open(kMagnetometerI2C, NULL);
    i2c_data_request( kMagnetometerI2C, HMC5883L_I2C_ADDR, HMC5883L_ID_A,
                     data, sizeof(data));
    while (!i2c_is_done(kMagnetometerI2C) && timeout) {timeout--;}
    error = i2c_has_error(kMagnetometerI2C);
    if (!timeout) { error = TRUE; }
    i2c_close(kMagnetometerI2C);
    
    if (!error && whoami) {
        *whoami  = data[0] << 16;
        *whoami |= data[1] << 8;
        *whoami |= data[2];
    }
        
    if (error ||
        data[0] != HMC5883L_ID_A_EXPECTED ||
        data[1] != HMC5883L_ID_B_EXPECTED ||
        data[2] != HMC5883L_ID_C_EXPECTED) {
            // not as expected
            return FALSE;
    }
    return TRUE;
}
 
/*************************************************************************
* Function Name: MagnetometerGetGain
* Description: Get gain of magnetometer in LSB/gauss
* Parameters: none
* Return: 0 if error, gain if succesful
**************************************************************************/
uint16_t MagnetometerGetGain()
{
    // for HMC5883L_CONFIGURATION_B which sets the gain 
    const uint16_t HMC5883L_GAIN_SETTINGS[] = { 
        1370, // 000 +/- 0.88 Ga
        1090, // 001 +/- 1.3  Ga, default
        820,  // 010 +/- 1.9  Ga
        660,  // 011 +/- 2.5  Ga
        440,  // 100 +/- 4.0  Ga
        390,  // 101 +/- 4.7  Ga
        330,  // 110 +/- 5.6  Ga
        230,  // 111 +/- 8.1  Ga
    };
    uint8_t data;
    uint8_t gain;
    _MagReadReg(HMC5883L_CONFIGURATION_B, &data);    

    gain = data >> 5;
    return (HMC5883L_GAIN_SETTINGS[gain]);   
}


/*************************************************************************
* Function Name: _MagnetometerClearBuffer
* Description: Clear the magnetometer's buffer, only used in swtest
**************************************************************************/
static void _MagnetometerClearBuffer() 
{
    int16_t numReads = 6; // this may be excessive but better safe
    int16_t readings[NUM_AXIS];
    int i;
    INFO_ENDLINE();
    while (numReads) {
        MagnetometerStartReading();
        while (!IsMagnetometerDoneReading()) { /* spin */ ; }
        MagnetometerGetLastReading(readings);
        numReads--;
        for (i = 0; i < NUM_AXIS; i++) {
            INFO_INT("\t", reading[i]);
        }
        INFO_ENDLINE();
    }
}
/*************************************************************************
* Function Name: MagnetometerSelfTest
* Description: Run the magnetometer self-test. This only prints results, 
*  it does not assess if the test was successful.
* Return: TRUE if magnetomter's existence if verified, FALSE if there was
*  an error talkign to the mag (or the mag's whoami is wrong)
**************************************************************************/
uint8_t  MagnetometerSelfTest()
{
    int16_t readings[NUM_AXIS];
    uint8_t originalGainSetting;
    uint16_t gain;
    const uint8_t numSigDigits = 2;
    int i;
    
    // going to reset gain to something that won't be overwhelmed
    // with 1.1 induced gausss but we'll need to store the original
    // value for later
    _MagReadReg(HMC5883L_CONFIGURATION_B, &originalGainSetting);    
    _MagWriteReg(HMC5883L_CONFIGURATION_B, HMC5883L_B_GAIN_4_0_GA);    

    gain = MagnetometerGetGain();
    
    // now set to go into test mode
    _MagWriteReg(HMC5883L_CONFIGURATION_A, HMC5883L_A_TEST_POSITIVE);    

    _MagnetometerClearBuffer();

    // now in test mode, need two data acquitiions in single sample mode
    MagnetometerStartReading();
    while (!IsMagnetometerDoneReading()) { /* spin */ ; }
    MagnetometerGetLastReading(readings);
    DEBUG_ENDLINE();
    DEBUG_STRING("\tPOS:");
    for ( i = 0; i < NUM_AXIS; i++ ) {    
        DEBUG_FLOAT(" ", (float) readings[i] / gain, numSigDigits);
    }
    DEBUG_STRING(" (ex  1.16,  1.16,  1.08)\r\n");

    _MagWriteReg(HMC5883L_CONFIGURATION_A, HMC5883L_A_TEST_NEGATIVE);    
    
    MagnetometerStartReading();
    while (!IsMagnetometerDoneReading()) { /* spin */ ; }
    MagnetometerGetLastReading(readings);
    DEBUG_STRING("\tNEG:");
    for ( i = 0; i < NUM_AXIS; i++ ) {    
        DEBUG_FLOAT(" ", (float) readings[i] / gain, numSigDigits);
    }
    DEBUG_STRING(" (ex -1.16, -1.16, -1.08)\r\n\t");

    // return things to normal
    i2c_open(kMagnetometerI2C, NULL);
    _MagWriteReg(HMC5883L_CONFIGURATION_A, HMC5883L_A_TEST_NORMAL);    
    _MagWriteReg(HMC5883L_CONFIGURATION_B, originalGainSetting);    

    _MagnetometerClearBuffer();
    
    return TRUE;
}

/*************************************************************************
* Function Name: MagnetometerConfig
* Description: Set up config of the magnetometer
* Parameters: range in milligauss, 
* Return: none
**************************************************************************/
uint8_t MagnetometerConfig(uint32_t *rangeInMilliGauss)
{
    uint8_t config;
    
    if (*rangeInMilliGauss >= 8100) {
        config = HMC5883L_B_GAIN_8_1_GA;
        *rangeInMilliGauss = 8100;        
    } else if (*rangeInMilliGauss >= 5600) {
        config = HMC5883L_B_GAIN_5_6_GA;
        *rangeInMilliGauss = 5600;        
    } else if (*rangeInMilliGauss >= 4700) {
        config = HMC5883L_B_GAIN_4_7_GA;
        *rangeInMilliGauss = 4700;        
    } else if (*rangeInMilliGauss >= 4000) {
        config = HMC5883L_B_GAIN_4_0_GA;
        *rangeInMilliGauss = 4000;        
    } else if (*rangeInMilliGauss >= 2500) {
        config = HMC5883L_B_GAIN_2_5_GA;
        *rangeInMilliGauss = 2500;        
    } else if (*rangeInMilliGauss >= 1900) {
        config = HMC5883L_B_GAIN_1_9_GA;
        *rangeInMilliGauss = 1900;        
    } else if (*rangeInMilliGauss >= 1300) {
        config = HMC5883L_B_GAIN_1_3_GA;
        *rangeInMilliGauss = 4000;        
    } else {
        config = HMC5883L_B_GAIN_0_88_GA;
        *rangeInMilliGauss = 880;        
    }
     config <<= HMC5883L_GAIN_SHIFT;
    _MagWriteReg(HMC5883L_CONFIGURATION_B, config);   

    return TRUE;

}


/*************************************************************************
* Function Name: _InitMagDataReadyInt
* Description: Set up data ready GPIO and make it an interrupt
* Parameters: none
* Return: none
**************************************************************************/
void _InitMagDataReadyInt(void) 
{
    GPIO_InitTypeDef GPIO_InitStructure;
    EXTI_InitTypeDef EXTI_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;
    
    RCC_AHB1PeriphClockCmd(MAG_DATA_READY_GPIO_CLK, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE); // for interrupts
    
    /* Configure data ready pin as input */
    GPIO_StructInit(&GPIO_InitStructure);
    GPIO_InitStructure.GPIO_Pin = MAG_DATA_READY_GPIO_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_Init(MAG_DATA_READY_GPIO_PORT, &GPIO_InitStructure);
        
    /* Configure EXTI line */
    EXTI_StructInit(&EXTI_InitStructure);
    EXTI_InitStructure.EXTI_Line = MAG_DATA_READY_EXTI_LINE;
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;  
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    EXTI_Init(&EXTI_InitStructure);
 
    /* Connect EXTI Line to GPIO Pin */
    SYSCFG_EXTILineConfig(MAG_DATA_READY_EXTI_PORT_SOURCE, MAG_DATA_READY_EXTI_PIN_SOURCE);

    /* Enable and set EXTI Interrupt to the lowest priority */
    NVIC_InitStructure.NVIC_IRQChannel = MAG_DATA_READY_EXTI_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x0F;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x0F;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);     
}

/*************************************************************************
* Function Name: InitMagnetometer
* Description: Initialize magnetometer, no assumptions made about rest of 
*  system's initialization. This will call i2c_configure and set up the 
*  data ready line up as an interrupt. Finally, it will put the magnetomter
*  in the normal running mode
* Parameters: none
* Return: FALSE if error
**************************************************************************/
uint8_t InitMagnetometer()
{
    uint8_t status, config;
    uint32_t error;
    
    _InitMagDataReadyInt();
    i2c_configure(kMagnetometerI2C);
    
    // HMC5883L_CONFIGURATION_A default is 
    //      MS0:1 0 bias
    //      D00:2 15Hz output in continuous mode
    //      MA0:1 8 sample ave
    // leave that for now, especially since we won't use continuous mode
    
    // HMC5883L_CONFIGURATION_B default is 
    //      GN0:2 gain is set 001, +/1 1.3Ga, 1090 lsb/gauss
    // see MagnetometerGetGain for more info
    
    // HMC5883L_MODE defaults to single measurement mode
    
    // check status is good    
    error = _MagReadReg(HMC5883L_STATUS, &status);
    if (error) { 
        INFO_HEX("ERR can't read mag status ", 
                  i2c_has_error(kMagnetometerI2C)); 
    } else { 
        INFO_HEX("Mag status: ", status); 
    }
    INFO_ENDLINE();
    
    config = 0x1C;
    _MagWriteReg(HMC5883L_CONFIGURATION_A, config);

    return MagnetometerWhoAmI(NULL);
}
