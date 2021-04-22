/*****************************************************************************
** Accelerometer interface for the Freescale MMA8451Q accelerometer
** Some features of interest for implementation:
**  * Fast 160 Hz Maximum Output Rate
**  * Built-In Self Test
**  * I2C should run at 400 kHz
** 
** The normal method for reading is:
**  * Call AccelerometerStartReading, probably from a timer interrupt
**  * Receive data ready interrupt, this triggers an I2C read
**  * Get I2C read complete flag set in OS (or poll IsAccelerometerDoneReading()
**  * Call AccelerometerGetLastReading to get the data out
** Repeat
*****************************************************************************/
#include <stdint.h>
#include "salvodefs.h"
#include "dmu.h"
#include "accelerometer.h"
#include "accelMMA8451Q.h"
#include "boardDefinition.h"
#include "i2c.h"

//#define LOGGING_LEVEL LEVEL_INFO
#include "debug.h"

#define I2C_TIMEOUT 10000
static struct {
    uint8_t dataBuffer[NUM_AXIS*2 + 1 ]; // readings are 2 bytes per axis + 1 for status
    int32_t accumulationBuffer[NUM_AXIS];
    int32_t accumulationCount;
} gAccel;

/*************************************************************************
* Function Name: _AccReadReg : blocking read via I2C to accelerometer
* Parameters: address 
* Return: value read
**************************************************************************/
static uint8_t _AccReadReg(uint8_t address)
{   
    uint8_t rx = 0xFF; 
    uint32_t timeout = I2C_TIMEOUT;
    i2c_open(kAccelerometerI2C, NULL);
    i2c_data_request(kAccelerometerI2C, MMA8451_I2C_ADDR, address,
                     &rx, sizeof(rx)); 
    while (!i2c_is_done(kAccelerometerI2C) && timeout) {timeout--;}
    i2c_close(kAccelerometerI2C);
    return rx;  
}

/*************************************************************************
* Function Name: _AccWriteReg : blocking write via I2C to accelerometer
* Parameters: address and value
* Return: value read
**************************************************************************/
static int _AccWriteReg(uint8_t address, uint8_t data, int waitForDone)
{   
    static uint8_t buffer[2];
    uint32_t error;
    uint32_t timeout = I2C_TIMEOUT;
    buffer[0] = address;
    buffer[1] = data;
    i2c_open(kAccelerometerI2C, NULL);
    i2c_data_send(kAccelerometerI2C, MMA8451_I2C_ADDR, 
                         buffer, sizeof(buffer));
    while (waitForDone && !i2c_is_done(kAccelerometerI2C) && timeout) {timeout--;}
    error = i2c_has_error(kAccelerometerI2C);  
    if (!timeout) error = TRUE;
    i2c_close(kAccelerometerI2C);
    return !error;
}

/*************************************************************************
* Function Name: _SetStandby : Sets the MMA845x to standby mode.
*   It must be in standby to change most register settings 
* Parameters: none
* Return: none
**************************************************************************/
static void _SetStandby()
{
    uint8_t reg;
    reg = _AccReadReg(MMA8451_CTRL_REG1);
    reg &= ~(MMA8451_CTRL_REG1_ACTIVE);
    _AccWriteReg(MMA8451_CTRL_REG1, reg, TRUE);
}

/*************************************************************************
* Function Name: _SetActive : Sets the MMA845x to active mode.
*   It must be in active to read sensors
* Parameters: none
* Return: none
**************************************************************************/
static void _SetActive()
{ 
    uint8_t reg;
    reg = _AccReadReg(MMA8451_CTRL_REG1);
    reg |= (MMA8451_CTRL_REG1_ACTIVE);
    _AccWriteReg(MMA8451_CTRL_REG1, reg, TRUE);
}

/*************************************************************************
* Function Name: _AccelerometerDataReceivedCallback : 
* Description: Called from the I2C interrupt when the data has been
*   obtained, this is the last step in getting a sample
* Parameters: none
* Return: none
**************************************************************************/
void _AccelerometerDataReceivedCallback(void)
{
    int16_t val; // need int16_t to make positives and negaitves work out
    int i;
    uint32_t error = i2c_has_error(kAccelerometerI2C);
    uint8_t *data = &gAccel.dataBuffer[1];
    uint8_t status = gAccel.dataBuffer[0];            
        
    if (!error && (status & MMA8451_STATUS_READY) && ! (status & MMA8451_STATUS_ERROR)  ) {
        // sum this data into the accumulation buffer
        for (i = 0; i < NUM_AXIS; i++ ) {
            val = (data[2*i] << 8)  | data[ (2*i) + 1];
            gAccel.accumulationBuffer[i] += val;
        }
        gAccel.accumulationCount++;
    } else {
        gAccel.accumulationBuffer[0] = status;
    }
    OSSetEFlag(EFLAGS_DATA_READY, EF_DATA_ACCEL_READY);
    i2c_close(kAccelerometerI2C);
}

/*************************************************************************
* Function Name: AccelerometerDataReadyIRQ : 
* Description: This interrupt indicates the data is ready on the 
*  accelerometer. Kicks off reading data.
* Parameters: none
* Return: none
**************************************************************************/
void AccelerometerDataReadyIRQ(void)
{
    uint32_t error = i2c_has_error(kAccelerometerI2C);
    if (!error) {
        if (i2c_open(kAccelerometerI2C, _AccelerometerDataReceivedCallback)) {
            i2c_data_request(kAccelerometerI2C, MMA8451_I2C_ADDR,
                               MMA8451_STATUS,
                               gAccel.dataBuffer, sizeof(gAccel.dataBuffer));     
        } else {
            error = TRUE;
        }
    }
    if (error) {
        OSSetEFlag(EFLAGS_DATA_READY, EF_DATA_ACCEL_READY);
        i2c_close(kAccelerometerI2C);
    }
}


/*************************************************************************
* Function Name: AccelerometerStartReading
* Description:  This doesn't kick off a conversion cycle as the sampling
*   asynchronous to the system. It just makes it so the data ready line 
*   goes (but it goes whenever it was ready). 
* Parameters: none
* Return: 0 if success, error otherwise
**************************************************************************/
uint8_t AccelerometerStartReading()
{
    int i;
    
    OSEnterCritical();
    for (i = 0; i < NUM_AXIS; i++) {
        gAccel.accumulationBuffer[i] = 0;
    }
    gAccel.accumulationCount = 0;

    // clear buffer
    for (i = 0; i < sizeof(gAccel.dataBuffer); i++) {
        gAccel.dataBuffer[i] = 0;    
    }
    
    OSLeaveCritical();

    return 0;
}
/*************************************************************************
* Function Name: IsAccelerometerDoneReading
* Description: Checks to see if  transaction is complete
* Parameters: none
* Return: 0 if success, error otherwise
**************************************************************************/
uint8_t IsAccelerometerDoneReading()
{
    OStypeEFlag eFlag = OSReadEFlag(EFLAGS_DATA_READY);
    return eFlag & EF_DATA_ACCEL_READY;
}

/*************************************************************************
* Function Name: _AccelComputeAverages
* Description: Mildly optimized method to calculate accumulated 800Hz samples
* Parameters: number of readings and the accumulated data
* Return: accumulationBuffer has updated data
**************************************************************************/
void _AccelComputeAverages(int32_t count, int32_t *accumulationBuffer) 
{
    int i;
    switch (count) {
    case 0:
    case 1:
        // do nothing
        break;
    case 2:
        for (i = 0; i < NUM_AXIS; i++) {
             accumulationBuffer[i] >>= 1;
        }
        break;
    case 4:
        for (i = 0; i < NUM_AXIS; i++) {
             accumulationBuffer[i] >>= 2;
        }
        break;
    case 8: // max likely
        for (i = 0; i < NUM_AXIS; i++) {
             accumulationBuffer[i] >>= 3;
        }
        break;
    default:
        for (i = 0; i < NUM_AXIS; i++) {
             accumulationBuffer[i] = accumulationBuffer[i]  / count;
        }
        break;
    } // end switch
            
}

/*************************************************************************
* Function Name: AccelerometerGetLastReading
* Description: Gets the data that has been accumulated, averages it and returns it
* Parameters: array of reading (NUM_AXIS)
* Return: 0 if success, error otherwise
**************************************************************************/
uint8_t AccelerometerGetLastReading(int16_t *readings)
{    
    int i;
    uint32_t error = i2c_has_error(kAccelerometerI2C);
    int32_t count;
    int32_t accumulationBuffer[NUM_AXIS];

    OSDisableHook();
    // copy the data before averaging so we don't take the time for a
    // divide in a no-interrupt zone
    for (i = 0; i < NUM_AXIS; i++) {
        accumulationBuffer[i] = gAccel.accumulationBuffer[i];
    }
    count = gAccel.accumulationCount;
    OSEnableHook();
 
    _AccelComputeAverages(count, accumulationBuffer); 

    if (!error && readings) {
        for (i = 0; i < NUM_AXIS; i++) {
            readings[i] = accumulationBuffer[i];
        }
    }    
    
    INFO_INT("\t", count);
    return error;
}
/*************************************************************************
* Function Name: AccelerometerWhoAmI
* Description: Verify accelerometer attached is one we know how to talk to
* Parameters: pointer to whoami value, data will be returned if this is not NULL
* Return: TRUE if successful, FALSE if error
**************************************************************************/
uint8_t  AccelerometerWhoAmI(uint32_t *whoami)
{
    uint8_t read;
    read = _AccReadReg(MMA8451_WHO_AM_I);
    
    if (whoami) { *whoami = read; }
    
    if (read == MMA8451_EXPECTED_WHO_AM_I) { return TRUE; }
    return FALSE;
}

/*************************************************************************
* Function Name: AccelerometerGetGain
* Description: Get gain of accelerometer in LSB/G
* Parameters: none
* Return: 0 if error, gain if succesful
**************************************************************************/
uint16_t AccelerometerGetGain()
{
    uint16_t gain;

    _SetStandby();
    
    gain = _AccReadReg(MMA8451_XYZ_DATA_CFG);
    gain =  (gain & MMA8451_CFG_xG_MASK) >> MMA8451_CFG_xG_SHIFT;

    _SetActive();

    switch (gain) { 
        
    case MMA8451_CFG_2G: return 4096 << 2; // the shift up is because
    case MMA8451_CFG_4G: return 2048 << 2; // the data is 14 bit but
    case MMA8451_CFG_8G: return 1024 << 2; // we treat it as 16
    default:
        ERROR_INT("Accelerometer gain is not in range! ", gain);
        ERROR_ENDLINE();
        return 0;
    }
}

uint8_t  AccelerometerSelfTest()
{
    return FALSE;
}

uint8_t AccelerometerConfig(uint32_t *rangeInGs, uint32_t *outputDataRate)
{
    uint8_t config, reg;

    _SetStandby();
    
    if (*rangeInGs >= ACCEL_RANGE_8G) {
        config = MMA8451_CFG_8G;
        *rangeInGs = 8;
    } else if (*rangeInGs >= ACCEL_RANGE_4G) {
        config = MMA8451_CFG_4G;
        *rangeInGs = 4;
    } else { // two G is the default
        config = MMA8451_CFG_2G;
        *rangeInGs = 2;
    }
     config <<=  MMA8451_CFG_xG_SHIFT;
        
    reg = _AccReadReg(MMA8451_XYZ_DATA_CFG);
    reg &= ~MMA8451_CFG_xG_MASK;
    reg |= config;
    _AccWriteReg(MMA8451_XYZ_DATA_CFG, reg, TRUE);
    
    config = MMA8451_DATARATE_800HZ; // there is no way to sync the  accelerometer
        // so output at the fastest rate and use a timer to determine when complete
    *outputDataRate = 800;

    config <<= MMA8451_DATARATE_SHIFT;
    
    if (*rangeInGs <= ACCEL_RANGE_4G) {
        config |= MMA8451_CTRL_REG1_LOW_NOISE;
    }
    
    _AccWriteReg(MMA8451_CTRL_REG1, config, TRUE);

    _SetActive();    
    return TRUE;
}


/*************************************************************************
* Function Name: _InitAccelDataReadyInt
* Description: Set up data ready GPIO and make it an interrupt
* Parameters: address and value
* Return: FALSE if error
**************************************************************************/
void _InitAccelDataReadyInt(void) 
{
    GPIO_InitTypeDef GPIO_InitStructure;
    EXTI_InitTypeDef EXTI_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;
    
    RCC_AHB1PeriphClockCmd(ACCEL_DATA_READY_GPIO_CLK, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE); // for interrupts
    
    /* Configure data ready pin as input */
    GPIO_StructInit(&GPIO_InitStructure);
    GPIO_InitStructure.GPIO_Pin = ACCEL_DATA_READY_GPIO_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_Init(ACCEL_DATA_READY_GPIO_PORT, &GPIO_InitStructure);
       
    /* Configure EXTI line */
    EXTI_StructInit(&EXTI_InitStructure);
    EXTI_InitStructure.EXTI_Line = ACCEL_DATA_READY_EXTI_LINE;
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;  
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    EXTI_Init(&EXTI_InitStructure);   

    /* Connect EXTI Line to GPIO Pin */
    SYSCFG_EXTILineConfig(ACCEL_DATA_READY_EXTI_PORT_SOURCE, ACCEL_DATA_READY_EXTI_PIN_SOURCE);
    
    /* Enable and set EXTI Interrupt to the lowest priority */
    NVIC_InitStructure.NVIC_IRQChannel = ACCEL_DATA_READY_EXTI_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x0F;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x0F;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);     
}

uint8_t InitAccelerometer()
{
    _InitAccelDataReadyInt();   
    
    i2c_configure(kAccelerometerI2C);

    _AccWriteReg(MMA8451_CTRL_REG1, 0, TRUE);
    _AccWriteReg(MMA8451_CTRL_REG4, MMA8451_CFG4_INT_EN_DRDY, TRUE);
    
    return AccelerometerWhoAmI(NULL);    
}
