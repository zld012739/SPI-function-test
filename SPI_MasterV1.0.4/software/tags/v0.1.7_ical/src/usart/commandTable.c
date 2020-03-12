
#include "commandLine.h"
#include "commands.h"

#define COMMAND_TABLE_END {"",0,0,""}

//Command table
const tCommand gCommands[] =
{
  {"ver",    &CmdVersion, 0, "Display firmware version"},

  { "auto", &CmdAutoDataAquisitionMode, 0,
    "System will go into constant output mode, no parameters" },


  {"agm",    &CmdReadSensors, 0x00,
    "Read accel, gyro, mag sensor counts <num reads,  10> <ms between reads, asap> " \
        "\r\n\t\tNote: mags only updated as they are available and gyro used is " \
        "\r\n\t the one that was last initialized."},
  {"agmf",    &CmdReadSensors, 0x01,
    "Read accel (Gs),  gyro (dps), mag (Ga) <num reads,  10> <ms between reads, asap> "
        "\r\n\t\tNote: mags only updated as they are available and gyro used is " \
        "\r\n\t the one that was last initialized."},

  {"ai",    &CmdAccelInit, 0,
     "Initialize the accel, <output range in Gs> "},
  {"a",    &CmdReadAccelerometer, 0,
    "Read accelerometer <num reads,  10> <ms between reads, asap>"},
  {"af",    &CmdReadAccelerometer, 1,
    "Read accelerometer in Gs <num reads,  10> <ms between reads, asap>"},

  {"t",    &CmdReadTemperature, 0,
    "Read temperature <num reads, 10> <ms between reads, asap>"},
  {"tf",    &CmdReadTemperature, 1,
    "Read temperature in C <num reads, 10> <ms between reads, asap>"},

  {"gi",    &CmdGyroInit, 0,
     "Initialize the gyro, <output range in dps> <output data rate>"},
  {"g",    &CmdReadGyro, 0,
    "Read gryo <num reads, 10> <ms between reads, asap>"},
  {"gf",    &CmdReadGyro, 1,
    "Read gyro in degrees per sec <num reads, 10> <ms between reads, asap>"},
  {"gtemp", &CmdReadGyroTemp, 0,
    "Read gryo temperature <num reads, 10> <ms between reads, asap>"},
  {"gtempf", &CmdReadGyroTemp, 1,
    "Read gyro temp in C <num reads, 10> <ms between reads, asap>"},

  {"g2i",    &CmdGyro2Init, 0,
     "Initialize the gyro2 <output range in dps>"},
  {"g2",    &CmdReadGyro2, 0,
    "Read gryo <num reads, 10> <ms between reads, asap>"},
  {"g2f",    &CmdReadGyro2, 1,
    "Read gyro in degrees per sec <num reads, 10> <ms between reads, asap>"},
  {"g2temp",    &CmdReadGyro2Temp, 0,
    "Read gryo temperature <num reads, 10> <ms between reads, asap>"},
  {"g2tempf",    &CmdReadGyro2Temp, 1,
    "Read gyro temp in C <num reads, 10> <ms between reads, asap>"},


  {"mi",    &CmdMagnetometerInit, 0,
  "Initialize the mag, <output range in milligauss> (Note: no odr, it goes as fast as possible.)"},
  {"m",    &CmdReadMagnetometer, 0,
    "Read magnetometer <num reads, default 10> <ms between reads, default asap>"},
  {"mf",    &CmdReadMagnetometer, 1,
    "Read magnetometer in Gauss <num reads, default 10> <ms between reads, default asap>"},

  { "iCal",  &CmdInertialCalib, 0x00,
    "Inertial calibration: Read accel, gyro, gyro temperature, temperature sensor counts <num reads,  10> <ms between reads, asap> " \
        "\r\n\t\tNote: the gyro used is the one that was last initialized."},
  { "iCalf",  &CmdInertialCalib, 0x01,
    "Inertial calibration: Read accel, gyro, gyro temperature, temperature sensor counts <num reads,  10> <ms between reads, asap> " \
        "\r\n\t\tNote: the gyro used is the one that was last initialized."},

  { "tCal",  &CmdInertialCalib, 0x10,
    "Temperature calibration: Read accel, gyro, magnetometer, gyro temperature sensor, " \
    "\r\n\t\tand board temperature sensor counts <num reads,  10> <ms between reads, asap> " \
        "\r\n\t\tNote: mags only updated as they are available and gyro used is " \
        "\r\n\t the one that was last initialized."},
   { "tCalf",  &CmdInertialCalib, 0x11,
    "Temperature calibration: Read accel, gyro, magnetometer, gyro temperature sensor, " \
    "\r\n\t\tand board temperature sensor counts <num reads,  10> <ms between reads, asap> " \
        "\r\n\t\tNote: mags only updated as they are available and gyro used is " \
        "\r\n\t the one that was last initialized."},
     
        

  { "output", &CmdUserUsart, 0,
    "Print characters to the user uart <which> <character string>"},

  { "pin", &CmdGpioPin, 0,
    "Set pin <port> <pin> to <state>"},
#if 0
  {"g",    &CmdReadGyro, 0,
    "Read gyro <num reads, default 10> <ms between reads, default asap>"},

#endif // 0
  // mfg test style functions
  {"swtest", &CmdSelfTest, 0, "Run self test, verify existence of each sensor."},
  COMMAND_TABLE_END  //MUST BE LAST!!!
};

