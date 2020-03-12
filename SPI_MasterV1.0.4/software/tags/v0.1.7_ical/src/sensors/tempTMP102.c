/*****************************************************************************
** Temperature sensor interface for the TI TMP102 temperature sensor
** Some features of interest for implementation:
**  * I2C should run at 400 kHz
** http://www.ti.com/lit/gpn/tmp102
*****************************************************************************/
#include <stdint.h>
#include "temperatureSensor.h"
#include "tempTMP102.h"
#include "i2c.h"
// #define LOGGING_LEVEL LEVEL_INFO
#include "debug.h"
#include "dmu.h"
#include "boardDefinition.h"
#include "timer.h"

#define I2C_TIMEOUT 10000
tTime gTemperatureSensorConvStart; 

uint8_t InitTemperatureSensor()
{
    uint8_t config[2];
    uint8_t buffer[3];
    uint32_t error;
    uint32_t timeout = I2C_TIMEOUT;

    i2c_configure(kTemperatureSensorI2C);
    
    // this is a dummy read   
    i2c_open(kTemperatureSensorI2C, NULL);
    i2c_data_request( kTemperatureSensorI2C, TMP102_I2C_ADDR, TMP102_CONFIG_REG,
                     config, sizeof(config));
    while (!i2c_is_done(kTemperatureSensorI2C) && timeout) {timeout--;}

    config[0]  = TMP102_CONFIG_SHUTDOWN;
    config[0] |= TMP102_CONFIG_RESOLUTION << TMP102_CONFIG_RES_SHIFT;
    config[1] = TMP102_CONFIG1_DEFAULT;
    
    buffer[0] = TMP102_CONFIG_REG;
    buffer[1] = config[0];
    buffer[2] = config[1]; 
    
    i2c_data_send(kTemperatureSensorI2C, TMP102_I2C_ADDR, buffer, sizeof(buffer));
    timeout = I2C_TIMEOUT;    
    while (!i2c_is_done(kTemperatureSensorI2C) && timeout) {timeout--;}
    error = i2c_has_error(kTemperatureSensorI2C);
    if (timeout > 0 && !error) {
        INFO_STRING("Wrote config to temp sensor.\r\n")
    } else {
        ERROR_INT("Timeout writing to temperature sensor config ", error);
        ERROR_ENDLINE();       
    }
    i2c_data_request( kTemperatureSensorI2C, TMP102_I2C_ADDR, TMP102_CONFIG_REG,
                     config, sizeof(config));
    timeout = I2C_TIMEOUT;    
    while (!i2c_is_done(kTemperatureSensorI2C) && timeout) {timeout--;}
    error = i2c_has_error(kTemperatureSensorI2C);
    if (timeout > 0) {
        INFO_HEX("Temp config reg re-read ", config[0]);
        INFO_HEX(" ", config[1]);
        INFO_ENDLINE();
    } else {
        ERROR_INT("Timeout reading to temp sensor config (2) ", error);
        ERROR_ENDLINE();
    }
    i2c_close(kTemperatureSensorI2C);
    if (!timeout) { return FALSE; }
    
    return TRUE;
}


/*************************************************************************
* Function Name: _TemperatureRegWriteDoneCallback : 
* Description: Called from the I2C interrupt 
* Parameters: none
* Return: none
**************************************************************************/
static void _TemperatureRegWriteDoneCallback(void)
{
    i2c_close(kTemperatureSensorI2C);
}
uint8_t TemperatureStartReading()
{
    uint8_t buffer[3];
    uint32_t timeout = I2C_TIMEOUT;
    uint32_t error;

    // When the device is in Shutdown Mode, 
    // writing a 1 to the OS/ALERT bit will start
    // a single temperature conversion. The device will return to
    // the shutdown state at the completion of the single conversion

    buffer[0] = TMP102_CONFIG_REG;
    buffer[1] = TMP102_CONFIG_START_CONV;
    buffer[2] = TMP102_CONFIG1_DEFAULT;
    i2c_open(kTemperatureSensorI2C, _TemperatureRegWriteDoneCallback);
    i2c_data_send(kTemperatureSensorI2C, TMP102_I2C_ADDR, buffer, sizeof(buffer));
    while (!i2c_is_done(kTemperatureSensorI2C) && timeout) {timeout--;}
    error = i2c_has_error(kTemperatureSensorI2C);
    i2c_close(kTemperatureSensorI2C);
    
    if (error) { ERROR_INT("Failed to start reading ", error); ERROR_ENDLINE(); }

    gTemperatureSensorConvStart =  TimeNow();
        
    return TRUE;
}

// purely time based
uint8_t IsTemperatureDoneReading()
{
    return (TMP102_CONV_TIME <= TimePassed(gTemperatureSensorConvStart));
}

// FIXME: this takes the time to read the sensor data but there are more
// efficient ways than to wait for completion here
uint8_t TemperatureGetLastReading(int16_t *t)
{
    uint8_t data[2];
    uint16_t temperature;
    uint32_t timeout = I2C_TIMEOUT;
    uint32_t error;
    
    i2c_open(kTemperatureSensorI2C, NULL);
    i2c_data_request( kTemperatureSensorI2C, TMP102_I2C_ADDR, 
                     TMP102_TEMPERATURE_REG,
                     data, sizeof(data));
    while (!i2c_is_done(kTemperatureSensorI2C) && timeout) {timeout--;}
    error = i2c_has_error(kTemperatureSensorI2C);
    i2c_close(kTemperatureSensorI2C);
    if (timeout > 0 && !error) {    
        temperature = data[0] << 8 | data[1];        
        if (t) *t = temperature;
        return TRUE;
    } else {
        if (t) *t = 0;
        return FALSE;
    }
}

// return LSB/Gauss
uint16_t TemperatureGetGain()
{
 
    /*******************************
    ** TEMPERATURE      DIGITAL OUTPUT
    **    (°C)        (BINARY)          HEX                   
    **   128        0111 1111 1111      7FF
    **   127.9375   0111 1111 1111      7FF
    **   100        0110 0100 0000      640
    **   80         0101 0000 0000      500
    **   75         0100 1011 0000      4B0
    **   50         0011 0010 0000      320
    **   25         0001 1001 0000      190
    **   0.25       0000 0000 0100      004
    **   0.0        0000 0000 0000      000
    **   -0.25      1111 1111 1100      FFC
    **   -25        1110 0111 0000      E70
    **   -55        1100 1001 0000      C90
    **   -128       1000 0000 0000      800
    *******************************/
   return 16 << 4; // shift is because ADC is returning 12 bits 
                    // but it is in top bits of a uint16_t
}


uint8_t  TemperatureSelfTest()
{    
    return TRUE;
}
