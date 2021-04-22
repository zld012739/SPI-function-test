/*****************************************************************************
** Magnetometer interface for the Honeywell HMC5883L magnetometer
** Note, the magnetometer should implement the interface described in 
** magnetometer.h. This file just provides the specifics, for use by the
** associated C file only.
*****************************************************************************/
#ifndef HMC5883L_H
#define HMC5883L_H

#define HMC5883L_I2C_ADDR      0x3C


#define HMC5883L_CONFIGURATION_A 0
#define HMC5883L_CONFIGURATION_B 1
#define HMC5883L_MODE			 2
#define HMC5883L_X_MSB			 3
#define HMC5883L_X_LSB			 4
#define HMC5883L_Z_MSB			 5
#define HMC5883L_Z_LSB			 6
#define HMC5883L_Y_MSB			 7
#define HMC5883L_Y_LSB			 8
#define HMC5883L_STATUS			 9
#define HMC5883L_ID_A			 10
#define HMC5883L_ID_B			 11
#define HMC5883L_ID_C			 12

#define HMC5883L_ID_A_EXPECTED  'H'
#define HMC5883L_ID_B_EXPECTED  '4'
#define HMC5883L_ID_C_EXPECTED  '3'

#define HMC5883L_STATUS_DRDY      0x01
#define HMC5883L_MODE_READ_SINGLE 0x01 // Single-Measurement Mode, other bits
                                       // must be clear for correct operation

#define HMC5883L_A_TEST_POSITIVE   0x71 // MS0 changed for self test mode
#define HMC5883L_A_TEST_NEGATIVE   0x72
#define HMC5883L_A_TEST_NORMAL     0x70

#define HMC5883L_GAIN_SHIFT         5
#define HMC5883L_B_GAIN_0_88_GA    0
#define HMC5883L_B_GAIN_1_3_GA     1
#define HMC5883L_B_GAIN_1_9_GA     2
#define HMC5883L_B_GAIN_2_5_GA     3
#define HMC5883L_B_GAIN_4_0_GA     4
#define HMC5883L_B_GAIN_4_7_GA     5
#define HMC5883L_B_GAIN_5_6_GA     6
#define HMC5883L_B_GAIN_8_1_GA     7

#endif /* HMC5883L_H */