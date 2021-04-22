/*****************************************************************************
** Temperature sensors interface for the Texas instrument TMP102 thermistor
** Note, the sensor should implement the interface described in 
** temperatureSensor.h. This file just provides the specifics, for use by the
** associated C file only.
*****************************************************************************/
#ifndef TMP102_H
#define TMP102_H

// I2C address with Addr lines pulled high is 1001001
// shifted over to the left by 1, the write address is  1001 0010 
#define TMP102_I2C_ADDR       0x92


#define TMP102_TEMPERATURE_REG    0
#define TMP102_CONFIG_REG         1

#define TMP102_CONFIG_SHUTDOWN    0x01
#define TMP102_CONFIG_FAULT_MASK  0x18
#define TMP102_CONFIG_FAULT_SHIFT 3
#define TMP102_CONFIG_RES_MASK    0xA0
#define TMP102_CONFIG_RES_SHIFT   5
#define TMP102_CONFIG_RESOLUTION  3 // 12 bits (0.0625°C), 320ms for conv
#define TMP102_CONV_TIME          600 // ms

#define TMP102_CONFIG_START_CONV  0xE1

#define TMP102_CONFIG1_DEFAULT    0xA0




#endif /* TMP102_H */