
#ifndef COMMANDS_H
#define _COMMANDS_H

void CmdVersion(uint32_t data);

void CmdBlinkLED( uint32_t data );

void CmdInitSPIPeripheral( uint32_t data );

void CmdUserSpi_ReadTest( uint32_t data );
void CmdUserSpi_SpeedTest( uint32_t data );

void CmdUserSpi_adiReadTest( uint32_t data );
void CmdUserSpi_jdBurstTest( uint32_t data );

void CmdUserSpi_WhoAmI( uint32_t data );

void CmdUserSpi_GetBoardTemp( uint32_t data );
void CmdUserSpi_GetRates( uint32_t data );
void CmdUserSpi_GetAccels( uint32_t data );
void CmdUserSpi_GetSensorValues( uint32_t data );

void CmdUserSpi_ReadRegister_TwoBytes( uint32_t data );
void CmdUserSpi_ReadRegisterHex( uint32_t data );
void CmdUserSpi_ReadRegisterHex2( uint32_t data );
void CmdUserSpi_ReadRegister( uint32_t data );
void CmdUserSpi_ReadRegister2( uint32_t data );
void CmdUserSpi_WriteRegister( uint32_t data );

void CmdUserSpi_SelfTest( uint32_t data );

void CmdUserSpi_MagAlign( uint32_t data );

void CmdUserSpi_ATP_Process( uint32_t data );
void CmdUserSpi_jdInitProcess( uint32_t data );

void CmdUserSpi_IndraTesting( uint32_t data );

void CmdUserSpi_BurstRead( uint32_t data );

void CmdUserSpi_BurstRead_F1( uint32_t data );
void CmdUserSpi_BurstRead_F2( uint32_t data );

void CmdUserSpi_BurstRead_S0( uint32_t data );
void CmdUserSpi_BurstRead_S1( uint32_t data );

void CmdUserSpi_BurstRead_A1( uint32_t data );

void CmdUserSpi_BurstForLowGainAHRS( uint32_t data );

void CmdUserSpi_burstReadSwitch( uint32_t data );

void CmdUserSpi_BurstTest( uint32_t data );
void CmdUserSpi_BurstTest01( uint32_t data );

void CmdSpeed( uint32_t data );

void CmdUserSpiTestSequence01( uint32_t data );

void CmdSelfTest(uint32_t data);

void CmdUserUsart(uint32_t data);
void CmdGpioPin(uint32_t data);

void CmdUserSpi_HoldChipSelect( uint32_t data );
void CmdUserSpi_ToggleChipSelect( uint32_t data );

void CmdUserSpi_DisplayConverted( uint32_t data );
void CmdUserSpi_DisplayHex( uint32_t data );

void CmdUserSpi_WaitForDRDY( uint32_t data );
void CmdUserSpi_DontWaitForDRDY( uint32_t data );

void CmdUser_OnePPSTest( uint32_t data );
void CmdUser_OnePPSAdjust(uint32_t data);

void CmdUser_PowerOn( uint32_t data );
void CmdUser_PowerOff(uint32_t data);

void CmdUser_UARTMode(uint32_t data);
void CmdUser_SPIMode(uint32_t data);

void CmdUser_Ping(uint32_t data);

void CmdUser_readFDE(uint32_t data);
void CmdUser_writeFDE(uint32_t data);
void CmdUser_writeANOISPER(uint32_t data);
void CmdUser_readANOISPER(uint32_t data);
void CmdUser_readRNOISPER(uint32_t data);
void CmdUser_readINOISPER(uint32_t data);
void CmdUser_readACDP(uint32_t data);
void CmdUser_readRCDP(uint32_t data);
void CmdUser_readANDT(uint32_t data);
void CmdUser_readRNDT(uint32_t data);
void CmdUser_readAIDT(uint32_t data);
void CmdUser_readACDT(uint32_t data);
void CmdUser_readRCDT(uint32_t data);
void CmdUser_readDSR(uint32_t data);
void CmdUser_Ulock_FDS(uint32_t data);
void read_write_37(uint32_t data);
void CmdUser_saveconfigure(uint32_t data);
void CmdUser_writeRNOISPER(uint32_t data);
void CmdUser_writeINOISPER(uint32_t data);
void CmdUser_writeACDP(uint32_t data);
void CmdUser_writeRCDP(uint32_t data);
void CmdUser_writeANDT(uint32_t data);
void CmdUser_writeRNDT(uint32_t data);
void CmdUser_writeAIDT(uint32_t data);
void CmdUser_writeACDT(uint32_t data);
void CmdUser_writeRCDT(uint32_t data);
void CmdUser_clearFDE(uint32_t data);
void _SetSPIClockSpeed( uint8_t clockSpeed );
void DRDY_interval();
#endif /* COMMANDS_H */