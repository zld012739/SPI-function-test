/** ***************************************************************************
 * @file   commandTable.c
 * @Author
 * @date   September, 2008
 * @brief  Copyright (c) 2013, 2014 All Rights Reserved.
 *
 * THIS CODE AND INFORMATION ARE PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
 * KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
 * PARTICULAR PURPOSE.
 *
 * Table of serial Debug console input Commands with callback function pointers
 * and help text
 ******************************************************************************/
#include "commandLine.h" // tCommand
#include "commands.h"

#define COMMAND_TABLE_END {"",0,0,""}

// SPI Master Command Table
const tCommand gCommands[] =
{ // Command,  callback, data, help text
  { "blink", &CmdBlinkLED, 0, "Usage: blink\r\nBlink LED4 <numBlinks, 3> <ms between blinks, 250>" },

  { "output", &CmdUserUsart, 0,
  "Usage: output which string\r\nPrint characters to the user uart <which> <character string>"},

  { "boardSpeedTest", &CmdUserSpi_SpeedTest, 0, "Usage: boardSpeedTest regAddr NumOfBytes" \
                                    "\r\n       Read from 'regAddr' the number of bytes specified by 'NumOfBytes'" \
                                    "\r\n       <default -- regAddr: 65 (0x41), NumOfBytes: 4>" },

  // SPI communication commands
  { "spiCmd01", &CmdUserSpiTestSequence01, 0, "Usage: spiCmd01\r\nWrite a test sequence to the slave and display the response" },

  { "initSpi", &CmdInitSPIPeripheral, 0, "Usage: initSpi\r\nInitialize the SPI peripheral" },

  { "ver",    &CmdVersion, 0, "Usage: ver \r\nDisplay firmware version of SPI Master"},

  // Useful commands:
  { "pin", &CmdGpioPin, 0, "Usage: pin port pin to state"},

  // Finalized versions of SPI interface code:
  { "whoami", &CmdUserSpi_WhoAmI, 0, "Usage: whoami" \
                                    "\r\n       Read from register 0x54 to get the product ID" },

  { "atp", &CmdUserSpi_ATP_Process, 0, "Usage: atp" \
                                    "\r\n       Send a series of writes to configure the system followed by reads of the" \
                                    "\r\n         register to verify that the data was written.  This is echoed to the user" \
                                    "\r\n         to confirm the write was correct.  A 'Test Successful' statement is written" \
                                    "\r\n         if all writes were successful." },

  { "spitest", &CmdUserSpi_ATP_Process, 0, "Usage: spitest" \
                                    "\r\n       Send a series of writes to configure the system followed by reads of the" \
                                    "\r\n         register to verify that the data was written.  This is echoed to the user" \
                                    "\r\n         to confirm the write was correct.  A 'Test Successful' statement is written" \
                                    "\r\n         if all writes were successful." },

  { "jdInit", &CmdUserSpi_jdInitProcess, 0, "Usage: jdInit numOfTests numOfToggles_nRst numOfToggles_nSS" \
                                    "\r\n       Initiate the JD initialization sequence 'numOfTests' times in a row." \
                                    "\r\n       Note: the number of tests performed is actually 2x the number specified as" \
                                    "\r\n       each test consists of an nSS toggle sequence followed by nRst toggle." \
                                    "\r\n       <default -- numOfTests: 3, numOfToggles_nRst: 5, numOfToggles_nSS: 5>" },

  { "j", &CmdUserSpi_jdInitProcess, 0, "Usage: j numOfTests numOfToggles_nRst numOfToggles_nSS" \
                                    "\r\n       Initiate the JD initialization sequence 'numOfTests' times in a row." \
                                    "\r\n       Note: the number of tests performed is actually 2x the number specified as" \
                                    "\r\n       each test consists of an nSS toggle sequence followed by nRst toggle." \
                                    "\r\n       <default -- numOfTests: 3, numOfToggles_nRst: 5, numOfToggles_nSS: 5>" },

  { "indraTest", &CmdUserSpi_IndraTesting, 0, "Usage: indraTest numOfTests numOfToggles_nRst numOfToggles_nSS" \
                                    "\r\n       Initiate the JD initialization sequence 'numOfTests' times in a row." \
                                    "\r\n       Note: the number of tests performed is actually 2x the number specified as" \
                                    "\r\n       each test consists of an nSS toggle sequence followed by nRst toggle." \
                                    "\r\n       <default -- numOfTests: 3, numOfToggles_nRst: 5, numOfToggles_nSS: 5>" },

  { "i", &CmdUserSpi_IndraTesting, 0, "Usage: i numOfTests numOfToggles_nRst numOfToggles_nSS" \
                                    "\r\n       Initiate the JD initialization sequence 'numOfTests' times in a row." \
                                    "\r\n       Note: the number of tests performed is actually 2x the number specified as" \
                                    "\r\n       each test consists of an nSS toggle sequence followed by nRst toggle." \
                                    "\r\n       <default -- numOfTests: 3, numOfToggles_nRst: 5, numOfToggles_nSS: 5>" },

  { "burst_F1", &CmdUserSpi_BurstRead_F1, 0, "Usage: burst_f1 numberOfSamples delayBetweenSamples" \
                                    "\r\n       Perform a burst-read to get sensor information (status/rates/accels/temp)" \
                                    "\r\n       <default -- numberOfSamples: 10, delayBetweenSamples: 500 msec>" },

  { "burst_F2", &CmdUserSpi_BurstRead_F2, 0, "Usage: burst_f2 numberOfSamples delayBetweenSamples" \
                                    "\r\n       Perform a burst-read to get sensor information (status/rates/accels/temp)" \
                                    "\r\n       <default -- numberOfSamples: 10, delayBetweenSamples: 500 msec>" },

  { "burst_S0", &CmdUserSpi_BurstRead_S0, 0, "Usage: burst_s0 numberOfSamples delayBetweenSamples" \
                                    "\r\n       Perform a burst-read to get sensor information (status/rates/accels/temp)" \
                                    "\r\n       <default -- numberOfSamples: 10, delayBetweenSamples: 500 msec>" },

  { "ahrsMode", &CmdUserSpi_BurstForLowGainAHRS, 0, "Usage: ahrsMode" \
                                    "\r\n       Perform a burst-read to get AHRS mode information (HG/LG)" \
                                    "\r\n       <default -- numberOfSamples: 10, delayBetweenSamples: 500 msec>" },

  { "burstTest", &CmdUserSpi_burstReadSwitch, 0, "Usage: burstTest" \
                                    "\r\n       Perform a series of burst-reads to test switching between various" \
                                    "\r\n       burst commands (JD Burst, S0 Burst, S1 Burst, A1 Burst, F1 Burst)" \
                                    "\r\n       then repeats the test" \
                                    "\r\n       <default -- 5 tests, 2 MHz SPI clock" },

  { "magAlign", &CmdUserSpi_MagAlign, 0, "Usage: magAlign" \
                                    "\r\n       Perform a magnetic alignment" \
                                    "\r\n       <default -- 5 tests, 2 MHz SPI clock" },

  { "temp", &CmdUserSpi_GetBoardTemp, 0, "Usage: temp numberOfSamples delayBetweenSamples" \
                                    "\r\n       Perform a read of temperature-sensor information" \
                                    "\r\n       <default -- numberOfSamples: 10, delayBetweenSamples: 500 msec>" },

  { "accels", &CmdUserSpi_GetAccels, 0, "Usage: accels numberOfSamples delayBetweenSamples" \
                                    "\r\n       Perform a read of accelerometer information" \
                                    "\r\n       <default -- numberOfSamples: 10, delayBetweenSamples: 500 msec>" },

  { "sensors", &CmdUserSpi_GetSensorValues, 0, "Usage: sensors numberOfSamples delayBetweenSamples" \
                                    "\r\n       Perform a non-burst-mode read of sensor information (rates/accels/temp)" \
                                    "\r\n       <default -- numberOfSamples: 10, delayBetweenSamples: 500 msec>" },

  { "read", &CmdUserSpi_ReadRegister, 0, "Usage: read regAddr NumOfBytes" \
                                    "\r\n       Read from 'regAddr' the number of bytes specified by 'NumOfBytes'" \
                                    "\r\n       <default -- regAddr: 65 (0x41), NumOfBytes: 4>" },

  { "r", &CmdUserSpi_ReadRegister, 0, "Usage: r regAddr NumOfBytes" \
                                    "\r\n       Read from 'regAddr' the number of bytes specified by 'NumOfBytes'" \
                                    "\r\n       <default -- regAddr: 65 (0x41), NumOfBytes: 4>" },

  { "rHex", &CmdUserSpi_ReadRegisterHex, 0, "Usage: rHex regAddr NumOfBytes" \
                                    "\r\n       Read from 'regAddr' the number of bytes specified by 'NumOfBytes'" \
                                    "\r\n       <default -- regAddr: 65 (0x41), NumOfBytes: 4>" },

  { "rHex2", &CmdUserSpi_ReadRegisterHex2, 0, "Usage: rHex2 regAddr NumOfBytes" \
                                    "\r\n       Read from 'regAddr' the number of bytes specified by 'NumOfBytes'" \
                                    "\r\n       <default -- regAddr: 65 (0x41), NumOfBytes: 4>" },



  // Updated functions and descriptions follow
  { "selfTest", &CmdUserSpi_SelfTest, 0, "Usage: selfTest numberOfTests" \
                                    "\r\n       Perform a self-test of the system" \
                                    "\r\n       <default -- numberOfTests: 10, delay between checks: 5 msec>" },

  { "burst", &CmdUserSpi_BurstRead, 0, "Usage: burst numberOfSamples OutputDataRate" \
                                    "\r\n       Perform a burst-read to get sensor information (status/rates/accels/temp)" \
                                    "\r\n       <default -- numberOfSamples: 10, OutputDataRate: 2 Hz>" },

  { "burst_A1", &CmdUserSpi_BurstRead_A1, 0, "Usage: burst_a1 numberOfSamples OutputDataRate" \
                                    "\r\n       Perform a burst-read of the A1 packet to get AHRS and sensor information" \
                                    "\r\n       <default -- numberOfSamples: 10, OutputDataRate: 2 Hz>" },

  { "burst_S1", &CmdUserSpi_BurstRead_S1, 0, "Usage: burst_s1 numberOfSamples OutputDataRate" \
                                    "\r\n       Perform a burst-read of the S1 packet to get sensor information (status/rates/accels/temp)" \
                                    "\r\n       <default -- numberOfSamples: 10, OutputDataRate: 2 Hz>" },

  { "rates", &CmdUserSpi_GetRates, 0, "Usage: rates numberOfSamples OutputDataRate" \
                                    "\r\n       Perform a read of rate-sensor registers" \
                                    "\r\n       <default -- numberOfSamples: 10, OutputDataRate: 2 Hz>" },

  { "readHex", &CmdUserSpi_ReadRegisterHex, 0, "Usage: readHex regAddr NumOfBytes NumberOfReads OutputDataRate" \
                                    "\r\n       Read from 'regAddr' the number of bytes specified by 'NumOfBytes'" \
                                    "\r\n       <default -- regAddr: 4 (0x04), NumOfWords: 1, NumOfReadsd: 10, ODR: 2 Hz>" },

  { "write", &CmdUserSpi_WriteRegister, 0, "Usage: write regAddr msg" \
                                     "\r\n       Write a single byte, 'msg', to 'regAddr'" \
                                     "\r\n       <default -- regAddr: 0 (0x00), msg: 15 (0x0F)>" },

  // functions to modify how the reads operate
  { "toggleCS", &CmdUserSpi_ToggleChipSelect, 0, "Usage: toggleCS" \
                                     "\r\n       Commands the nSS to toggle between reads of burst-" \
                                     "\r\n       message words" },

  { "holdCS", &CmdUserSpi_HoldChipSelect, 0, "Usage: holdCS" \
                                     "\r\n       Commands the nSS to hold a constant state between" \
                                     "\r\n       reads of burst-message words" },

  { "displayConverted", &CmdUserSpi_DisplayConverted, 0, "Usage: displayConverted" \
                                     "\r\n       Commands the output message to be converted to" \
                                     "\r\n       floating-point values (if possible)" },

  { "displayHex", &CmdUserSpi_DisplayHex, 0, "Usage: displayHex" \
                                     "\r\n       Commands the output message to be displayed as" \
                                     "\r\n       hex values" },

  { "waitDRDY", &CmdUserSpi_WaitForDRDY, 0, "Usage: waitDRDY" \
                                     "\r\n       Commands the master to read data when the DRDY" \
                                     "\r\n       line is set" },

  { "dontWaitDRDY", &CmdUserSpi_DontWaitForDRDY, 0, "Usage: dontWaitDRDY" \
                                     "\r\n       Commands the master to read data at the desired" \
                                     "\r\n       rate regardless of how the DRDY-line is set" },

  { "speed", &CmdSpeed, 0, "Usage: speed value" \
                           "\r\n       Select SPI clock rate as specified by 'value'" \
                           "\r\n       <default -- speed: 2.0 MHaz" \
                             "\r\n       Values: 0.5, 1.0, 2.0, 4.0 MHz" },
                             
//<WangQing20181116+++>  for ATP2 Test       
  { "1PPSADJ", &CmdUser_OnePPSAdjust, 0, "Usage: 1PPS Test"},
  { "1PPS", &CmdUser_OnePPSTest, 0, "Usage: 1PPS Test"},
  
  { "poweron", &CmdUser_PowerOn, 0, "Usage: 1PPS Test"},
  { "poweroff", &CmdUser_PowerOff, 0, "Usage: 1PPS Test"},
  
  { "uart", &CmdUser_UARTMode, 0, "Usage: Switch to UART Test"},
  { "spi", &CmdUser_SPIMode, 0, "Usage: Switch to SPI Test"},
    
  { "ping", &CmdUser_Ping, 0, "Usage: Ping SPI Master"},
//<WangQing20181116--->
  { "readFDE", &CmdUser_readFDE, 0, "Usage: read fault detected enable"},
  { "writeFDE", &CmdUser_writeFDE, 0, "Usage: write fault detected enable"},
  { "writeANOISPER", &CmdUser_writeANOISPER, 0, "Usage:write ACCEL_NOISE_DETECT_PERIOD"},
  { "readANOISPER", &CmdUser_readANOISPER, 0, "Usage:read ACCEL_NOISE_DETECT_PERIOD"},
  { "readRNOISPER", &CmdUser_readRNOISPER, 0, "Usage:read RATE_NOISE_DETECT_PERIOD"},
  { "readINOISPER", &CmdUser_readINOISPER, 0, "Usage:read ACCEL_IMPROP_DETECT_PERIOD"},
  { "readACDP", &CmdUser_readACDP, 0, "Usage:read ACCEL_CONSISTENCY_DETECT_PERIOD"},
  { "readRCDP", &CmdUser_readRCDP, 0, "Usage:read RATE_CONSISTENCY_DETECT_PERIOD"},
  { "readANDT", &CmdUser_readANDT, 0, "Usage:read ACCEL_NOISE_DETECT_THRESHOLD"},
  { "readRNDT", &CmdUser_readRNDT, 0, "Usage:read RATE_NOISE_DETECT_THRESHOLD"},
  { "readAIDT", &CmdUser_readAIDT, 0, "Usage:read ACCEL_IMPROP_DETECT_THRESHOLD"},
  { "readACDT", &CmdUser_readACDT, 0, "Usage:read ACCEL_CONSISTENCY_DETECT_THRESHOLD"},
  { "readRCDT", &CmdUser_readRCDT, 0, "Usage:read RATE_CONSISTENCY_DEDECT_THRESHOLD"},
  { "readDSR", &CmdUser_readDSR, 0, "Usage:read Diagnostic Status Register"},
  { "Ulock_FDS", &CmdUser_Ulock_FDS, 0, "Usage:unlock Fault Detection interface"},
  { "read_write_37", &read_write_37, 0, "Usage:read_write_37"},
  { "SRTE", &CmdUser_saveconfigure, 0, "Usage:save configure"},
  { "writeRNOISPER", &CmdUser_writeRNOISPER, 0, "Usage:write RATE_NOISE_DETECT_PERIOD"},
  
  { "writeINOISPER", &CmdUser_writeINOISPER, 0, "Usage:write ACCEL_IMPROP_DETECT_PERIOD"},
  { "writeACDP", &CmdUser_writeACDP, 0, "Usage:write ACCEL_CONSISTENCY_DETECT_PERIOD"},
  { "writeRCDP", &CmdUser_writeRCDP, 0, "Usage:write RATE_CONSISTENCY_DETECT_PERIOD"},
  { "writeANDT", &CmdUser_writeANDT, 0, "Usage:write ACCEL_NOISE_DETECT_THRESHOLD"},
  { "writeRNDT", &CmdUser_writeRNDT, 0, "Usage:write RATE_NOISE_DETECT_THRESHOLD"},
  { "writeAIDT", &CmdUser_writeAIDT, 0, "Usage:write ACCEL_IMPROP_DETECT_THRESHOLD"},
  { "writeACDT", &CmdUser_writeACDT, 0, "Usage:write ACCEL_CONSISTENCY_DETECT_THRESHOLD"},
  { "writeRCDT", &CmdUser_writeRCDT, 0, "Usage:write RATE_CONSISTENCY_DEDECT_THRESHOLD"},
  { "clearFDE", &CmdUser_clearFDE, 0, "Usage:clearFDE"},
  COMMAND_TABLE_END  //MUST BE LAST!!!
};

