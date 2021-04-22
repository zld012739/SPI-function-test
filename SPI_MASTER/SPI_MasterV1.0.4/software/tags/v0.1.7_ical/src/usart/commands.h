
#ifndef COMMANDS_H
#define _COMMANDS_H

void CmdVersion(uint32_t data);

void CmdReadSensors(uint32_t data);

void CmdAccelInit(uint32_t data);
void CmdReadAccelerometer(uint32_t data);

void CmdReadTemperature(uint32_t data);
void CmdMagnetometerInit(uint32_t data);
void CmdReadMagnetometer(uint32_t data);

void CmdGyroInit(uint32_t data);
void CmdReadGyro(uint32_t data);
void CmdReadGyroTemp(uint32_t data);

void CmdGyro2Init(uint32_t data);
void CmdReadGyro2(uint32_t data);
void CmdReadGyro2Temp(uint32_t data);

void CmdSelfTest(uint32_t data);

void CmdInertialCalib(uint32_t data);
void CmdTemperatureCalib(uint32_t data);

void CmdAutoDataAquisitionMode(uint32_t data);

void CmdUserUsart(uint32_t data);
void CmdGpioPin(uint32_t data);

#endif /* COMMANDS_H */