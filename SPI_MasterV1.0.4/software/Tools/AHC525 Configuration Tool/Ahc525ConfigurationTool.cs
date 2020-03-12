using System;
using System.Collections.Generic;
using System.Text;
using System.IO.Ports;
using System.Threading;

namespace Ahc525ConfigurationTool
{
    class Ahc525ConfigurationTool
    {
        static SerialPort serialPort;

        #region Configuration tool parameters

        const string SOFTWARE_PART_NUMBER = "5325-0675-01";

        static string SOFTWARE_VERSION = System.Reflection.Assembly.GetExecutingAssembly().GetName().Version.ToString();

        #endregion

        #region Product configuration

        #region Version string setting

        static char[] VERSION_STRING = "AHC525GA".ToCharArray();

        #endregion

        #region Product configuration settings

        const UInt16 HAS_INTERNAL_MAGNETOMETER = (0x0001 << 0 );
        const UInt16 HAS_GPS                   = (0x0001 << 1 );
        const UInt16 ALGORITHM_ENABLED         = (0x0001 << 2 );
        const UInt16 EXTERNAL_AIDING           = (0x0001 << 3 );
        const UInt16 AHC525_ARCHITECTURE       = (0x0006 << 4 );
        const UInt16 IS_ADAHRS                 = (0x0001 << 15);

        const UInt16 PRODUCT_CONFIGURATION = ( ALGORITHM_ENABLED   |
                                               EXTERNAL_AIDING     |
                                               AHC525_ARCHITECTURE );

        #endregion

        #region Packet type settings

        const UInt16 UCB_IDENTIFICATION   = 0x4944;
        const UInt16 UCB_VERSION_DATA     = 0x5652;
        const UInt16 UCB_VERSION_ALL_DATA = 0x5641;
        const UInt16 UCB_ANGLE_3          = 0x4133;
        const UInt16 UCB_ANGLE_5          = 0x4135;
        const UInt16 UCB_ANGLE_U          = 0x4155;
        const UInt16 UCB_SCALED_3         = 0x5333;
        const UInt16 UCB_TEST_0           = 0x5430;
        const UInt16 UCB_TEST_1           = 0x5431;
        const UInt16 UCB_FACTORY_1        = 0x4631;
        const UInt16 UCB_FACTORY_5        = 0x4635;
        const UInt16 UCB_FACTORY_6        = 0x4636;
        const UInt16 UCB_FACTORY_7        = 0x4637;

        const UInt16 PACKET_TYPE = UCB_ANGLE_U;

        #endregion

        #region Packet rate settings

        const UInt16 RATE_QUIET = 0;
        const UInt16 RATE_2HZ   = 50;
        const UInt16 RATE_4HZ   = 25;
        const UInt16 RATE_5HZ   = 20;
        const UInt16 RATE_10HZ  = 10;
        const UInt16 RATE_20HZ  = 5;
        const UInt16 RATE_25HZ  = 4;
        const UInt16 RATE_50HZ  = 2;
        const UInt16 RATE_100HZ = 1;

        const UInt16 PACKET_RATE = RATE_100HZ;

        #endregion

        #region Port usage and baud rate settings

        const UInt16 PORT_OFF           = 0;
        const UInt16 CRM_PORT           = 1;
        const UInt16 PRIMARY_UCB_PORT   = 2;
        const UInt16 SECONDARY_UCB_PORT = 3;

        const UInt16 BAUD_9600  = 0;
        const UInt16 BAUD_19200 = 1;
        const UInt16 BAUD_38400 = 2;
        const UInt16 BAUD_57600 = 3;

        const UInt16 PORT_1_USAGE     = PRIMARY_UCB_PORT;
        const UInt16 PORT_1_BAUD_RATE = BAUD_57600;
        const UInt16 PORT_2_USAGE     = CRM_PORT;
        const UInt16 PORT_2_BAUD_RATE = BAUD_38400;
        const UInt16 PORT_3_USAGE     = PORT_OFF;
        const UInt16 PORT_3_BAUD_RATE = BAUD_57600;
        const UInt16 PORT_4_USAGE     = PORT_OFF;
        const UInt16 PORT_4_BAUD_RATE = BAUD_57600;

        #endregion

        #region Unit orientation settings

        const UInt16 X_AXIS_POS_USER_X = (0x0000 << 0);
        const UInt16 X_AXIS_NEG_USER_X = (0x0001 << 0);
        const UInt16 X_AXIS_POS_USER_Y = (0x0002 << 0);
        const UInt16 X_AXIS_NEG_USER_Y = (0x0003 << 0);
        const UInt16 X_AXIS_POS_USER_Z = (0x0004 << 0);
        const UInt16 X_AXIS_NEG_USER_Z = (0x0005 << 0);

        const UInt16 Y_AXIS_POS_USER_Y = (0x0000 << 3);
        const UInt16 Y_AXIS_NEG_USER_Y = (0x0001 << 3);
        const UInt16 Y_AXIS_POS_USER_Z = (0x0002 << 3);
        const UInt16 Y_AXIS_NEG_USER_Z = (0x0003 << 3);
        const UInt16 Y_AXIS_POS_USER_X = (0x0004 << 3);
        const UInt16 Y_AXIS_NEG_USER_X = (0x0005 << 3);

        const UInt16 Z_AXIS_POS_USER_Z = (0x0000 << 0);
        const UInt16 Z_AXIS_NEG_USER_Z = (0x0001 << 0);
        const UInt16 Z_AXIS_POS_USER_X = (0x0002 << 0);
        const UInt16 Z_AXIS_NEG_USER_X = (0x0003 << 0);
        const UInt16 Z_AXIS_POS_USER_Y = (0x0004 << 0);
        const UInt16 Z_AXIS_NEG_USER_Y = (0x0005 << 0);

        const UInt16 UNIT_ORIENTATION = ( X_AXIS_POS_USER_X |
                                          Y_AXIS_POS_USER_Y |
                                          Z_AXIS_POS_USER_Z );

        #endregion

        #region Hardware status enable settings

        const UInt16 UNLOCKED_EEPROM            = (0x0001 << 3);
        const UInt16 OAT_FAIL                   = (0x0001 << 5);
        const UInt16 EXTERNAL_MAGNETOMETER_FAIL = (0x0001 << 6);
        const UInt16 INTERNAL_AIR_DATA          = (0x0001 << 7);
        
        const UInt16 HARDWARE_STATUS_ENABLE = ( UNLOCKED_EEPROM            |
                                                EXTERNAL_MAGNETOMETER_FAIL );

        #endregion

        #region Software status enable settings

        const UInt16 ALGORITHM_INIT              = (0x0001 << 0);
        const UInt16 ATTITUDE_ONLY_ALGORITHM     = (0x0001 << 2);
        const UInt16 TURN_SWITCH                 = (0x0001 << 3);
        const UInt16 NO_MAGNETOMETER_HEADING_REF = (0x0001 << 5);
        const UInt16 VERTICAL_GYRO_FORCE         = (0x0001 << 9);

        const UInt16 SOFTWARE_STATUS_ENABLE = ( ALGORITHM_INIT              |
                                                NO_MAGNETOMETER_HEADING_REF );

        #endregion

        #region Sensor status enable settings

        const UInt16 SENSOR_OVERRANGE                      = (0x0001 << 0);
        const UInt16 MAGNETOMETER_ALIGNMENT_INVALID        = (0x0001 << 2);
        const UInt16 MAGNETOMETER_SERIAL_NUMBER_MATCH_FAIL = (0x0001 << 4);
        const UInt16 NO_AIR_DATA_AIDING                    = (0x0001 << 5);

        const UInt16 SENSOR_STATUS_ENABLE = ( NO_AIR_DATA_AIDING );

        #endregion

        #region Communication status enable settings

        const UInt16 NO_EXTERNAL_MAGNETOMETER = (0x0001 << 1);
        const UInt16 NO_EXTERNAL_AIR_DATA     = (0x0001 << 2);

        const UInt16 COMMUNICATION_STATUS_ENABLE = ( NO_EXTERNAL_MAGNETOMETER |
                                                     NO_EXTERNAL_AIR_DATA     );

        #endregion

        #region Barometer settings

        const double BAROMETER_SETTING = 29.92;

        #endregion

        #region Roll, pitch, and yaw offset settings

        const double ROLL_OFFSET  = 0;
        const double PITCH_OFFSET = 0;
        const double YAW_OFFSET   = 0;

        #endregion

///////////////////////////
        #region Roll/pitch offset external Mag, X/y Hard Iron Bias Ext,Soft Iron Scale Ratio Ext,Soft Iron Angle Ext
        const double ROLL_OFFSET_EXT_MAG = 0;
        const double PITCH_OFFSET_EXT_MAG = 0;
        const double X_HARD_IRON = 0;
        const double Y_HARD_IRON = 0;
        const double SOFT_IRON_SCALE_RATIO = 0.99;
        const double SOFT_IRON_ANGLE_EXT = 0;

        #endregion

/////////////////////////
        #region INERTIAL SENSOR RANGE
        const UInt16 INERTIAL_SENSOR_RANGE_ACCEL = 0x0004;
        const UInt16 INERTIAL_SENSOR_RANGE_GYRO = 0x00C8;   //0x00C8;---200deg/sec; 0x012C---300 deg/s

        #endregion
/////////////////////////

        #region BIT limits
         
        const UInt16 BITERR_TWOVOLTIOUP_LOWER      = 0x6CD3;
        const UInt16 BITERR_TWOVOLTIOUP_UPPER      = 0x7247;
        const UInt16 BITERR_TWOVOLTDUP_LOWER       = 0x6CD3;
        const UInt16 BITERR_TWOVOLTDUP_UPPER       = 0x7247;
        const UInt16 BITERR_TWOFIVEREF_LOWER       = 0x7D6E;
        const UInt16 BITERR_TWOFIVEREF_UPPER       = 0x8500;
        const UInt16 BITERR_THREEVOLT_LOWER        = 0x9068;
        const UInt16 BITERR_THREEVOLT_UPPER        = 0x9A66;
        const UInt16 BITERR_FIVEVOLTA_LOWER        = 0xB8BC;
        const UInt16 BITERR_FIVEVOLTA_UPPER        = 0xC7E0;
        const UInt16 BITERR_FIVEVOLTAG_LOWER       = 0xB8BC;
        const UInt16 BITERR_FIVEVOLTAG_UPPER       = 0xC7E0;
        const UInt16 BITERR_FIVEVOLTD_LOWER        = 0xB8BC;
        const UInt16 BITERR_FIVEVOLTD_UPPER        = 0xC7E0;
        const UInt16 BITERR_SEVENVOLTPWR_LOWER     = 0xE82D;
        const UInt16 BITERR_SEVENVOLTPWR_UPPER     = 0xFD5F;
        const UInt16 BITERR_ACCL1CURRENT_LOWER     = 0x3ABF;
        const UInt16 BITERR_ACCL1CURRENT_UPPER     = 0x4BC1;
        const UInt16 BITERR_RATE1CURRENT_LOWER     = 0x29BD;
        const UInt16 BITERR_RATE1CURRENT_UPPER     = 0x4340;
        const UInt16 BITERR_RATE2CURRENT_LOWER     = 0x29BD;
        const UInt16 BITERR_RATE2CURRENT_UPPER     = 0x4340;
        const UInt16 BITERR_RATE3CURRENT_LOWER     = 0x29BD;
        const UInt16 BITERR_RATE3CURRENT_UPPER     = 0x4340;

        const UInt32 BITERR_ACCEL1TEMP_LOWER        = 0x18030392;
        const UInt32 BITERR_ACCEL1TEMP_UPPER        = 0x2FE90c83;
        const UInt32 BITERR_RATE1TEMP_LOWER         = 0x10D00000;
        const UInt32 BITERR_RATE1TEMP_UPPER         = 0x34F4C000;
        const UInt32 BITERR_RATE2TEMP_LOWER         = 0x10D00000;
        const UInt32 BITERR_RATE2TEMP_UPPER         = 0x34F4C000;
        const UInt32 BITERR_RATE3TEMP_LOWER         = 0x109FF8F2;
        const UInt32 BITERR_RATE3TEMP_UPPER         = 0x34F4C000;
        const UInt32 BITERR_STATICPRESSTEMP_LOWER = 0x00000000;
        const UInt32 BITERR_STATICPRESSTEMP_UPPER  = 0xFFFFFFFF;
        const UInt32 BITERR_DYNAMICPRESSTEMP_LOWER = 0x00000000;
        const UInt32 BITERR_DYNAMICPRESSTEMP_UPPER = 0xFFFFFFFF;
        const UInt32 BITERR_OATCURRENT_LOWER       = 0x00000000;
        const UInt32 BITERR_OATCURRENT_UPPER       = 0xFFFFFFFF;

        #endregion

        #endregion
//////////////////////////////////////////////////////
        #region Roll/Pitch Incidence limit
        const UInt16 ROLL_INCIDENCE_LIMIT = 0x1000;
        const UInt16 PITCH_INCIDENCE_LIMIT = 0x1000;

        #endregion

        #region Hard/Soft iron limit
        const UInt16 HARD_IRON_LIMIT = 0x6666;
        const UInt16 SOFT_IRON_LIMIT = 0x4000;

        #endregion

/////////////////////////////////////////////////////


        #region EEPROM parameters

        // EEPROM constants
        const int LOWER_CALIBRATION_ADDRESS = 0x0100;
        const int UPPER_CALIBRATION_ADDRESS = 0x153A;

        #endregion

        #region UCB commands

        // communication constants
        const int READ_TIMEOUT = 200;
        const int SHORT_READ_TIMEOUT = 80;
        const int WRITE_TIMEOUT = 5000;

        // sync header
        static byte[] SYNC_HEADER = { 0x55, 0x55 };

        // set to quiet mode
        static byte[] commandQuietMode = { 0x53, 0x46, 0x05, 0x01, 0x00, 0x01, 0x00, 0x00 };

        // ping
        static byte[] commandPing = { 0x50, 0x4b, 0x00 };
        static byte[] responsePing = { 0x50, 0x4b, 0x00 };

        // write unit serial number
        static byte[] commandWriteUnitSerialNumber = { 0x57, 0x45, 0x07, 0x01, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00 };
        static byte[] responseWriteUnitSerialNumber = { 0x57, 0x45, 0x03, 0x01, 0x00, 0x02 };

        // read unit serial number
        static byte[] commandReadUnitSerialNumber = { 0x52, 0x45, 0x03, 0x01, 0x00, 0x02 };
        static byte[] responseReadUnitSerialNumber = { 0x52, 0x45, 0x07, 0x01, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00 };

        // unlock EEPROM
        static byte[] commandUnlockEeprom = { 0x55, 0x45, 0x02, 0x00, 0x00 };
        static byte[] responseUnlockEeprom = { 0x55, 0x45, 0x00 };

        // write version string
        static byte[] commandWriteVersionString =  { 0x57, 0x45, Convert.ToByte(3 + (VERSION_STRING.Length + 1) * 2), 0x01, 0x04, Convert.ToByte(VERSION_STRING.Length + 1) }  ;
        static byte[] responseWriteVersionString = { 0x57, 0x45, 0x03, 0x01, 0x04, Convert.ToByte(VERSION_STRING.Length + 1) };

        // read version string
        static byte[] commandReadVersionString = { 0x52, 0x45, 0x03, 0x01, 0x04, Convert.ToByte(VERSION_STRING.Length + 1) };
        static byte[] responseReadVersionString = { 0x52, 0x45, Convert.ToByte(3 + (VERSION_STRING.Length + 1) * 2), 0x01, 0x04, Convert.ToByte(VERSION_STRING.Length + 1) };

        // write product configuration 
        static byte[] commandWriteProductConfiguration = { 0x57, 0x45, 0x05, 0x0D, 0x34, 0x01, Convert.ToByte((PRODUCT_CONFIGURATION >> 8) & 0xff), Convert.ToByte((PRODUCT_CONFIGURATION) & 0xff) };
        static byte[] responseWriteProductConfiguration = { 0x57, 0x45, 0x03, 0x0D, 0x34, 0x01 };

        // read product configuration
        static byte[] commandReadProductConfiguration = { 0x52, 0x45, 0x03, 0x0D, 0x34, 0x01 };
        static byte[] responseReadProductConfiguration = { 0x52, 0x45, 0x05, 0x0D, 0x34, 0x01, Convert.ToByte((PRODUCT_CONFIGURATION >> 8) & 0xff), Convert.ToByte((PRODUCT_CONFIGURATION) & 0xff) };

        // write packet type 
        static byte[] commandWritePacketType = { 0x57, 0x45, 0x05, 0x00, 0x06, 0x01, Convert.ToByte((PACKET_TYPE >> 8) & 0xff), Convert.ToByte((PACKET_TYPE) & 0xff) };
        static byte[] responseWritePacketType = { 0x57, 0x45, 0x03, 0x00, 0x06, 0x01 };

        // read packet type
        static byte[] commandReadPacketType = { 0x52, 0x45, 0x03, 0x00, 0x06, 0x01 };
        static byte[] responseReadPacketType = { 0x52, 0x45, 0x05, 0x00, 0x06, 0x01, Convert.ToByte((PACKET_TYPE >> 8) & 0xff), Convert.ToByte((PACKET_TYPE) & 0xff) };

        // write packet rate
        static byte[] commandWritePacketRate = { 0x57, 0x45, 0x05, 0x00, 0x02, 0x01, Convert.ToByte((PACKET_RATE >> 8) & 0xff), Convert.ToByte((PACKET_RATE) & 0xff) };
        static byte[] responseWritePacketRate = { 0x57, 0x45, 0x03, 0x00, 0x02, 0x01 };

        // read packet rate
        static byte[] commandReadPacketRate = { 0x52, 0x45, 0x03, 0x00, 0x02, 0x01 };
        static byte[] responseReadPacketRate = { 0x52, 0x45, 0x05, 0x00, 0x02, 0x01, Convert.ToByte((PACKET_RATE >> 8) & 0xff), Convert.ToByte((PACKET_RATE) & 0xff) };

        // write port 1 baud rate
        static byte[] commandWritePort1BaudRate = { 0x57, 0x45, 0x05, 0x00, 0x4A, 0x01, Convert.ToByte((PORT_1_BAUD_RATE >> 8) & 0xff), Convert.ToByte((PORT_1_BAUD_RATE) & 0xff) };
        static byte[] responseWritePort1BaudRate = { 0x57, 0x45, 0x03, 0x00, 0x4A, 0x01 };

        // read port 1 baud rate
        static byte[] commandReadPort1BaudRate = { 0x52, 0x45, 0x03, 0x00, 0x4A, 0x01 };
        static byte[] responseReadPort1BaudRate = { 0x52, 0x45, 0x05, 0x00, 0x4A, 0x01, Convert.ToByte((PORT_1_BAUD_RATE >> 8) & 0xff), Convert.ToByte((PORT_1_BAUD_RATE) & 0xff) };

        // write port 2 baud rate
        static byte[] commandWritePort2BaudRate = { 0x57, 0x45, 0x05, 0x00, 0x4C, 0x01, Convert.ToByte((PORT_2_BAUD_RATE >> 8) & 0xff), Convert.ToByte((PORT_2_BAUD_RATE) & 0xff) };
        static byte[] responseWritePort2BaudRate = { 0x57, 0x45, 0x03, 0x00, 0x4C, 0x01 };

        // read port 2 baud rate
        static byte[] commandReadPort2BaudRate = { 0x52, 0x45, 0x03, 0x00, 0x4C, 0x01 };
        static byte[] responseReadPort2BaudRate = { 0x52, 0x45, 0x05, 0x00, 0x4C, 0x01, Convert.ToByte((PORT_2_BAUD_RATE >> 8) & 0xff), Convert.ToByte((PORT_2_BAUD_RATE) & 0xff) };

        // write port 3 baud rate
        static byte[] commandWritePort3BaudRate = { 0x57, 0x45, 0x05, 0x00, 0x4E, 0x01, Convert.ToByte((PORT_3_BAUD_RATE >> 8) & 0xff), Convert.ToByte((PORT_3_BAUD_RATE) & 0xff) };
        static byte[] responseWritePort3BaudRate = { 0x57, 0x45, 0x03, 0x00, 0x4E, 0x01 };

        // read port 3 baud rate
        static byte[] commandReadPort3BaudRate = { 0x52, 0x45, 0x03, 0x00, 0x4E, 0x01 };
        static byte[] responseReadPort3BaudRate = { 0x52, 0x45, 0x05, 0x00, 0x4E, 0x01, Convert.ToByte((PORT_3_BAUD_RATE >> 8) & 0xff), Convert.ToByte((PORT_3_BAUD_RATE) & 0xff) };

        // write port 4 baud rate
        static byte[] commandWritePort4BaudRate = { 0x57, 0x45, 0x05, 0x00, 0x50, 0x01, Convert.ToByte((PORT_4_BAUD_RATE >> 8) & 0xff), Convert.ToByte((PORT_4_BAUD_RATE) & 0xff) };
        static byte[] responseWritePort4BaudRate = { 0x57, 0x45, 0x03, 0x00, 0x50, 0x01 };

        // read port 4 baud rate
        static byte[] commandReadPort4BaudRate = { 0x52, 0x45, 0x03, 0x00, 0x50, 0x01 };
        static byte[] responseReadPort4BaudRate = { 0x52, 0x45, 0x05, 0x00, 0x50, 0x01, Convert.ToByte((PORT_4_BAUD_RATE >> 8) & 0xff), Convert.ToByte((PORT_4_BAUD_RATE) & 0xff) };

        // write port 1 usage
        static byte[] commandWritePort1Usage = { 0x57, 0x45, 0x05, 0x00, 0x42, 0x01, Convert.ToByte((PORT_1_USAGE >> 8) & 0xff), Convert.ToByte((PORT_1_USAGE) & 0xff) };
        static byte[] responseWritePort1Usage = { 0x57, 0x45, 0x03, 0x00, 0x42, 0x01 };

        // read port 1 usage
        static byte[] commandReadPort1Usage = { 0x52, 0x45, 0x03, 0x00, 0x42, 0x01 };
        static byte[] responseReadPort1Usage = { 0x52, 0x45, 0x05, 0x00, 0x42, 0x01, Convert.ToByte((PORT_1_USAGE >> 8) & 0xff), Convert.ToByte((PORT_1_USAGE) & 0xff) };

        // write port 2 usage
        static byte[] commandWritePort2Usage = { 0x57, 0x45, 0x05, 0x00, 0x44, 0x01, Convert.ToByte((PORT_2_USAGE >> 8) & 0xff), Convert.ToByte((PORT_2_USAGE) & 0xff) };
        static byte[] responseWritePort2Usage = { 0x57, 0x45, 0x03, 0x00, 0x44, 0x01 };

        // read port 2 usage
        static byte[] commandReadPort2Usage = { 0x52, 0x45, 0x03, 0x00, 0x44, 0x01 };
        static byte[] responseReadPort2Usage = { 0x52, 0x45, 0x05, 0x00, 0x44, 0x01, Convert.ToByte((PORT_2_USAGE >> 8) & 0xff), Convert.ToByte((PORT_2_USAGE) & 0xff) };

        // write port 3 usage
        static byte[] commandWritePort3Usage = { 0x57, 0x45, 0x05, 0x00, 0x46, 0x01, Convert.ToByte((PORT_3_USAGE >> 8) & 0xff), Convert.ToByte((PORT_3_USAGE) & 0xff) };
        static byte[] responseWritePort3Usage = { 0x57, 0x45, 0x03, 0x00, 0x46, 0x01 };

        // read port 3 usage
        static byte[] commandReadPort3Usage = { 0x52, 0x45, 0x03, 0x00, 0x46, 0x01 };
        static byte[] responseReadPort3Usage = { 0x52, 0x45, 0x05, 0x00, 0x46, 0x01, Convert.ToByte((PORT_3_USAGE >> 8) & 0xff), Convert.ToByte((PORT_3_USAGE) & 0xff) };

        // write port 4 usage
        static byte[] commandWritePort4Usage = { 0x57, 0x45, 0x05, 0x00, 0x48, 0x01, Convert.ToByte((PORT_4_USAGE >> 8) & 0xff), Convert.ToByte((PORT_4_USAGE) & 0xff) };
        static byte[] responseWritePort4Usage = { 0x57, 0x45, 0x03, 0x00, 0x48, 0x01 };

        // read port 4 usage
        static byte[] commandReadPort4Usage = { 0x52, 0x45, 0x03, 0x00, 0x48, 0x01 };
        static byte[] responseReadPort4Usage = { 0x52, 0x45, 0x05, 0x00, 0x48, 0x01, Convert.ToByte((PORT_4_USAGE >> 8) & 0xff), Convert.ToByte((PORT_4_USAGE) & 0xff) };

        // write orientation
        static byte[] commandWriteOrientation = { 0x57, 0x45, 0x05, 0x00, 0x0E, 0x01, Convert.ToByte((UNIT_ORIENTATION >> 8) & 0xff), Convert.ToByte((UNIT_ORIENTATION) & 0xff) };
        static byte[] responseWriteOrientation = { 0x57, 0x45, 0x03, 0x00, 0x0E, 0x01 };

        // read orientation
        static byte[] commandReadOrientation = { 0x52, 0x45, 0x03, 0x00, 0x0E, 0x01 };
        static byte[] responseReadOrientation = { 0x52, 0x45, 0x05, 0x00, 0x0E, 0x01, Convert.ToByte((UNIT_ORIENTATION >> 8) & 0xff), Convert.ToByte((UNIT_ORIENTATION) & 0xff) };

        // write hardware status enable
        static byte[] commandWriteHardwareStatusEn = { 0x57, 0x45, 0x05, 0x00, 0x20, 0x01, Convert.ToByte((HARDWARE_STATUS_ENABLE >> 8) & 0xff), Convert.ToByte((HARDWARE_STATUS_ENABLE) & 0xff) };
        static byte[] responseWriteHardwareStatusEn = { 0x57, 0x45, 0x03, 0x00, 0x20, 0x01 };

        // read hardware status enable
        static byte[] commandReadHardwareStatusEn = { 0x52, 0x45, 0x03, 0x00, 0x20, 0x01 };
        static byte[] responseReadHardwareStatusEn = { 0x52, 0x45, 0x05, 0x00, 0x20, 0x01, Convert.ToByte((HARDWARE_STATUS_ENABLE >> 8) & 0xff), Convert.ToByte((HARDWARE_STATUS_ENABLE) & 0xff) };

        // write software status enable
        static byte[] commandWriteSoftwareStatusEn = { 0x57, 0x45, 0x05, 0x00, 0x24, 0x01, Convert.ToByte((SOFTWARE_STATUS_ENABLE >> 8) & 0xff), Convert.ToByte((SOFTWARE_STATUS_ENABLE) & 0xff) };
        static byte[] responseWriteSoftwareStatusEn = { 0x57, 0x45, 0x03, 0x00, 0x24, 0x01 };

        // read software status enable
        static byte[] commandReadSoftwareStatusEn = { 0x52, 0x45, 0x03, 0x00, 0x24, 0x01 };
        static byte[] responseReadSoftwareStatusEn = { 0x52, 0x45, 0x05, 0x00, 0x24, 0x01, Convert.ToByte((SOFTWARE_STATUS_ENABLE >> 8) & 0xff), Convert.ToByte((SOFTWARE_STATUS_ENABLE) & 0xff) };

        // write sensor status enable
        static byte[] commandWriteSensorStatusEn = { 0x57, 0x45, 0x05, 0x00, 0x26, 0x01, Convert.ToByte((SENSOR_STATUS_ENABLE >> 8) & 0xff), Convert.ToByte((SENSOR_STATUS_ENABLE) & 0xff) };
        static byte[] responseWriteSensorStatusEn = { 0x57, 0x45, 0x03, 0x00, 0x26, 0x01 };

        // read sensor status enable
        static byte[] commandReadSensorStatusEn = { 0x52, 0x45, 0x03, 0x00, 0x26, 0x01 };
        static byte[] responseReadSensorStatusEn = { 0x52, 0x45, 0x05, 0x00, 0x26, 0x01, Convert.ToByte((SENSOR_STATUS_ENABLE >> 8) & 0xff), Convert.ToByte((SENSOR_STATUS_ENABLE) & 0xff) };

        // write com status enable
        static byte[] commandWriteComStatusEn = { 0x57, 0x45, 0x05, 0x00, 0x22, 0x01, Convert.ToByte((COMMUNICATION_STATUS_ENABLE >> 8) & 0xff), Convert.ToByte((COMMUNICATION_STATUS_ENABLE) & 0xff) };
        static byte[] responseWriteComStatusEn = { 0x57, 0x45, 0x03, 0x00, 0x22, 0x01 };

        // read communication status enable
        static byte[] commandReadComStatusEn = { 0x52, 0x45, 0x03, 0x00, 0x22, 0x01 };
        static byte[] responseReadComStatusEn = { 0x52, 0x45, 0x05, 0x00, 0x22, 0x01, Convert.ToByte((COMMUNICATION_STATUS_ENABLE >> 8) & 0xff), Convert.ToByte((COMMUNICATION_STATUS_ENABLE) & 0xff) };

        // write baro setting
        static byte[] commandWriteBaroSetting = { 0x57, 0x45, 0x05, 0x00, 0x2C, 0x01, Convert.ToByte((Convert.ToInt16(BAROMETER_SETTING * 1000) >> 8) & 0xff), Convert.ToByte(Convert.ToInt16(BAROMETER_SETTING * 1000) & 0xff) };
        static byte[] responseWriteBaroSetting = { 0x57, 0x45, 0x03, 0x00, 0x2C, 0x01 };

        // read baro setting
        static byte[] commandReadBaroSetting = { 0x52, 0x45, 0x03, 0x00, 0x2C, 0x01 };
        static byte[] responseReadBaroSetting = { 0x52, 0x45, 0x05, 0x00, 0x2C, 0x01, Convert.ToByte((Convert.ToInt16(BAROMETER_SETTING * 1000) >> 8) & 0xff), Convert.ToByte(Convert.ToInt16(BAROMETER_SETTING * 1000) & 0xff) };

        // write roll offset
        static byte[] commandWriteRollOffset = { 0x57, 0x45, 0x05, 0x00, 0x32, 0x01, Convert.ToByte((Convert.ToInt16(ROLL_OFFSET * 65536 / (2 * Math.PI)) >> 8) & 0xff), Convert.ToByte(Convert.ToInt16(ROLL_OFFSET * 65536 / (2 * Math.PI)) & 0xff) };
        static byte[] responseWriteRollOffset = { 0x57, 0x45, 0x03, 0x00, 0x32, 0x01 };

        // read roll offset
        static byte[] commandReadRollOffset = { 0x52, 0x45, 0x03, 0x00, 0x32, 0x01 };
        static byte[] responseReadRollOffset = { 0x52, 0x45, 0x05, 0x00, 0x32, 0x01, Convert.ToByte((Convert.ToInt16(ROLL_OFFSET * 65536 / (2 * Math.PI)) >> 8) & 0xff), Convert.ToByte(Convert.ToInt16(ROLL_OFFSET * 65536 / (2 * Math.PI)) & 0xff) };

        // write pitch offset
        static byte[] commandWritePitchOffset = { 0x57, 0x45, 0x05, 0x00, 0x34, 0x01, Convert.ToByte((Convert.ToInt16(PITCH_OFFSET * 65536 / (2 * Math.PI)) >> 8) & 0xff), Convert.ToByte(Convert.ToInt16(PITCH_OFFSET * 65536 / (2 * Math.PI)) & 0xff) };
        static byte[] responseWritePitchOffset = { 0x57, 0x45, 0x03, 0x00, 0x34, 0x01 };

        // read pitch offset
        static byte[] commandReadPitchOffset = { 0x52, 0x45, 0x03, 0x00, 0x34, 0x01 };
        static byte[] responseReadPitchOffset = { 0x52, 0x45, 0x05, 0x00, 0x34, 0x01, Convert.ToByte((Convert.ToInt16(PITCH_OFFSET * 65536 / (2 * Math.PI)) >> 8) & 0xff), Convert.ToByte(Convert.ToInt16(PITCH_OFFSET * 65536 / (2 * Math.PI)) & 0xff) };

        // write yaw offset
        static byte[] commandWriteYawOffset = { 0x57, 0x45, 0x05, 0x00, 0x36, 0x01, Convert.ToByte((Convert.ToInt16(YAW_OFFSET * 65536 / (2 * Math.PI)) >> 8) & 0xff), Convert.ToByte(Convert.ToInt16(YAW_OFFSET * 65536 / (2 * Math.PI)) & 0xff) };
        static byte[] responseWriteYawOffset = { 0x57, 0x45, 0x03, 0x00, 0x36, 0x01 };

        // read yaw offset
        static byte[] commandReadYawOffset = { 0x52, 0x45, 0x03, 0x00, 0x36, 0x01 };
        static byte[] responseReadYawOffset = { 0x52, 0x45, 0x05, 0x00, 0x36, 0x01, Convert.ToByte((Convert.ToInt16(YAW_OFFSET * 65536 / (2 * Math.PI)) >> 8) & 0xff), Convert.ToByte(Convert.ToInt16(YAW_OFFSET * 65536 / (2 * Math.PI)) & 0xff) };

///////////////////////////////////////////
        // write ROLL_OFFSET_EXT_MAG
        static byte[] commandWriteRollOffsetExtMag = { 0x57, 0x45, 0x05, 0x00, 0x2E, 0x01, Convert.ToByte((Convert.ToInt16(ROLL_OFFSET_EXT_MAG * 65536 / (2 * Math.PI)) >> 8) & 0xff), Convert.ToByte(Convert.ToInt16(ROLL_OFFSET_EXT_MAG * 65536 / (2 * Math.PI)) & 0xff) };
        static byte[] responseWriteRollOffsetExtMag = { 0x57, 0x45, 0x03, 0x00, 0x2E, 0x01 };

        // read ROLL_OFFSET_EXT_MAG
        static byte[] commandReadRollOffsetExtMag = { 0x52, 0x45, 0x03, 0x00, 0x2E, 0x01 };
        static byte[] responseReadRollOffsetExtMag = { 0x52, 0x45, 0x05, 0x00, 0x2E, 0x01, Convert.ToByte((Convert.ToInt16(ROLL_OFFSET_EXT_MAG * 65536 / (2 * Math.PI)) >> 8) & 0xff), Convert.ToByte(Convert.ToInt16(ROLL_OFFSET_EXT_MAG * 65536 / (2 * Math.PI)) & 0xff) };


        // write PITCH_OFFSET_EXT_MAG
        static byte[] commandWritePitchOffsetExtMag = { 0x57, 0x45, 0x05, 0x00, 0x30, 0x01, Convert.ToByte((Convert.ToInt16(PITCH_OFFSET_EXT_MAG * 65536 / (2 * Math.PI)) >> 8) & 0xff), Convert.ToByte(Convert.ToInt16(PITCH_OFFSET_EXT_MAG * 65536 / (2 * Math.PI)) & 0xff) };
        static byte[] responseWritePitchOffsetExtMag = { 0x57, 0x45, 0x03, 0x00, 0x30, 0x01 };

        // read PITCH_OFFSET_EXT_MAG
        static byte[] commandReadPitchOffsetExtMag = { 0x52, 0x45, 0x03, 0x00, 0x30, 0x01 };
        static byte[] responseReadPitchOffsetExtMag = { 0x52, 0x45, 0x05, 0x00, 0x30, 0x01, Convert.ToByte((Convert.ToInt16(PITCH_OFFSET_EXT_MAG * 65536 / (2 * Math.PI)) >> 8) & 0xff), Convert.ToByte(Convert.ToInt16(PITCH_OFFSET_EXT_MAG * 65536 / (2 * Math.PI)) & 0xff) };

        // write X_HARD_IRON
        static byte[] commandWriteXhardIron = { 0x57, 0x45, 0x05, 0x00, 0x38, 0x01, Convert.ToByte((Convert.ToInt16(X_HARD_IRON * 65536 / (2 * Math.PI)) >> 8) & 0xff), Convert.ToByte(Convert.ToInt16(X_HARD_IRON * 65536 / (2 * Math.PI)) & 0xff) };
        static byte[] responseWriteXhardIron = { 0x57, 0x45, 0x03, 0x00, 0x38, 0x01 };

        // read X_HARD_IRON
        static byte[] commandReadXhardIron = { 0x52, 0x45, 0x03, 0x00, 0x38, 0x01 };
        static byte[] responseReadXhardIron = { 0x52, 0x45, 0x05, 0x00, 0x38, 0x01, Convert.ToByte((Convert.ToInt16(X_HARD_IRON * 65536 / (2 * Math.PI)) >> 8) & 0xff), Convert.ToByte(Convert.ToInt16(X_HARD_IRON * 65536 / (2 * Math.PI)) & 0xff) };


        // write Y_HARD_IRON
        static byte[] commandWriteYhardIron = { 0x57, 0x45, 0x05, 0x00, 0x3A, 0x01, Convert.ToByte((Convert.ToInt16(Y_HARD_IRON * 65536 / (2 * Math.PI)) >> 8) & 0xff), Convert.ToByte(Convert.ToInt16(Y_HARD_IRON * 65536 / (2 * Math.PI)) & 0xff) };
        static byte[] responseWriteYhardIron = { 0x57, 0x45, 0x03, 0x00, 0x3A, 0x01 };

        // read Y_HARD_IRON
        static byte[] commandReadYhardIron = { 0x52, 0x45, 0x03, 0x00, 0x3A, 0x01 };
        static byte[] responseReadYhardIron = { 0x52, 0x45, 0x05, 0x00, 0x3A, 0x01, Convert.ToByte((Convert.ToInt16(Y_HARD_IRON * 65536 / (2 * Math.PI)) >> 8) & 0xff), Convert.ToByte(Convert.ToInt16(Y_HARD_IRON * 65536 / (2 * Math.PI)) & 0xff) };


        // write SOFT_IRON_ANGLE_EXT
        static byte[] commandWriteSoftIronAngleExt = { 0x57, 0x45, 0x05, 0x00, 0x3E, 0x01, Convert.ToByte((Convert.ToInt16(SOFT_IRON_ANGLE_EXT * 65536 / (2 * Math.PI)) >> 8) & 0xff), Convert.ToByte(Convert.ToInt16(SOFT_IRON_ANGLE_EXT * 65536 / (2 * Math.PI)) & 0xff) };
        static byte[] responseWriteSoftIronAngleExt = { 0x57, 0x45, 0x03, 0x00, 0x3E, 0x01 };

        // read SOFT_IRON_ANGLE_EXT
        static byte[] commandReadSoftIronAngleExt = { 0x52, 0x45, 0x03, 0x00, 0x3E, 0x01 };
        static byte[] responseReadSoftIronAngleExt = { 0x52, 0x45, 0x05, 0x00, 0x3E, 0x01, Convert.ToByte((Convert.ToInt16(SOFT_IRON_ANGLE_EXT * 65536 / (2 * Math.PI)) >> 8) & 0xff), Convert.ToByte(Convert.ToInt16(SOFT_IRON_ANGLE_EXT * 65536 / (2 * Math.PI)) & 0xff) };


        // write SOFT_IRON_SCALE_RATIO
        static byte[] commandWriteSoftIronSF = { 0x57, 0x45, 0x05, 0x00, 0x3C, 0x01, Convert.ToByte((Convert.ToInt16(SOFT_IRON_SCALE_RATIO * 32768) >> 8) & 0xff), Convert.ToByte(Convert.ToInt16(SOFT_IRON_SCALE_RATIO * 32768) & 0xff) };
        static byte[] responseWriteSoftIronSF = { 0x57, 0x45, 0x03, 0x00, 0x3C, 0x01 };

        // read SOFT_IRON_SCALE_RATIO
        static byte[] commandReadSoftIronSF = { 0x52, 0x45, 0x03, 0x00, 0x3C, 0x01 };
        static byte[] responseReadSoftIronSF = { 0x52, 0x45, 0x05, 0x00, 0x3C, 0x01, Convert.ToByte((Convert.ToInt16(SOFT_IRON_SCALE_RATIO * 32768) >> 8) & 0xff), Convert.ToByte(Convert.ToInt16(SOFT_IRON_SCALE_RATIO * 32768) & 0xff) };

/////////////////////////////////////////
        // write Inertial sensors range
        static byte[] commandWriteInertialSensorRange = { 0x57, 0x45, 0x07, 0x14, 0xBA, 0x02, Convert.ToByte((INERTIAL_SENSOR_RANGE_ACCEL >> 8) & 0xff), Convert.ToByte((INERTIAL_SENSOR_RANGE_ACCEL) & 0xff), Convert.ToByte((INERTIAL_SENSOR_RANGE_GYRO >> 8) & 0xff), Convert.ToByte((INERTIAL_SENSOR_RANGE_GYRO) & 0xff) };
        static byte[] responseWriteInertialSensorRange = { 0x57, 0x45, 0x03, 0x14, 0xBA, 0x02 };

        // read Inertial sensors range
        static byte[] commandReadInertialSensorRange = { 0x52, 0x45, 0x03, 0x14, 0xBA, 0x02 };
        static byte[] responseReadInertialSensorRange = { 0x52, 0x45, 0x07, 0x14, 0xBA, 0x02, Convert.ToByte((INERTIAL_SENSOR_RANGE_ACCEL >> 8) & 0xff), Convert.ToByte((INERTIAL_SENSOR_RANGE_ACCEL) & 0xff), Convert.ToByte((INERTIAL_SENSOR_RANGE_GYRO >> 8) & 0xff), Convert.ToByte((INERTIAL_SENSOR_RANGE_GYRO) & 0xff) };

////////////////////////////////////////

        // write Roll/Pitch Incidence Limit
        static byte[] commandWriteRollPitchIncLimit = { 0x57, 0x45, 0x07, 0x14, 0xD0, 0x02, Convert.ToByte((ROLL_INCIDENCE_LIMIT >> 8) & 0xff), Convert.ToByte((ROLL_INCIDENCE_LIMIT) & 0xff), Convert.ToByte((PITCH_INCIDENCE_LIMIT >> 8) & 0xff), Convert.ToByte((PITCH_INCIDENCE_LIMIT) & 0xff) };
        static byte[] responseWriteRollPitchIncLimit = { 0x57, 0x45, 0x03, 0x14, 0xD0, 0x02 };

        // read write Roll/Pitch Incidence Limit
        static byte[] commandReadRollPitchIncLimit = { 0x52, 0x45, 0x03, 0x14, 0xD0, 0x02 };
        static byte[] responseReadRollPitchIncLimit = { 0x52, 0x45, 0x07, 0x14, 0xD0, 0x02, Convert.ToByte((ROLL_INCIDENCE_LIMIT >> 8) & 0xff), Convert.ToByte((ROLL_INCIDENCE_LIMIT) & 0xff), Convert.ToByte((PITCH_INCIDENCE_LIMIT >> 8) & 0xff), Convert.ToByte((PITCH_INCIDENCE_LIMIT) & 0xff) };

        // write Hard/soft Iron Limit
        static byte[] commandWriteHardSoftIronLimit = { 0x57, 0x45, 0x07, 0x14, 0xD4, 0x02, Convert.ToByte((HARD_IRON_LIMIT >> 8) & 0xff), Convert.ToByte((HARD_IRON_LIMIT) & 0xff), Convert.ToByte((SOFT_IRON_LIMIT >> 8) & 0xff), Convert.ToByte((SOFT_IRON_LIMIT) & 0xff) };
        static byte[] responseWriteHardSoftIronLimit = { 0x57, 0x45, 0x03, 0x14, 0xD4, 0x02 };

        // read Hard/soft Iron Limit
        static byte[] commandReadHardSoftIronLimit = { 0x52, 0x45, 0x03, 0x14, 0xD4, 0x02 };
        static byte[] responseReadHardSoftIronLimit = { 0x52, 0x45, 0x07, 0x14, 0xD4, 0x02, Convert.ToByte((HARD_IRON_LIMIT >> 8) & 0xff), Convert.ToByte((HARD_IRON_LIMIT) & 0xff), Convert.ToByte((SOFT_IRON_LIMIT >> 8) & 0xff), Convert.ToByte((SOFT_IRON_LIMIT) & 0xff) };

  /////////////////////////////


        // read EEPROM location
        static byte[] commandReadEepromLocation = { 0x52, 0x45, 0x03, 0x00, 0x00, 0x01 };
        static byte[] responseReadEepromLocation = { 0x52, 0x45, 0x07, 0x00, 0x00, 0x01, 0x00, 0x00 };

        // write EEPROM CRC
        static byte[] commandWriteEepromCrc = { 0x57, 0x45, 0x05, 0x00, 0x00, 0x01, 0x00, 0x00 };
        static byte[] responseWriteEepromCrc = { 0x57, 0x45, 0x03, 0x00, 0x00, 0x01 };

        #endregion

        static void Main(string[] args)
        {
            bool unitPresent   = true;
            bool configSuccess = true;
            bool crcSuccess    = true;

            byte[] serialNumber = new byte[4];

            string serialNumberString;

            #region Emit configuration tool details

            Console.Write("AHC525 Configuration Tool\n");
            Console.Write("Part Number: \t{0}\n", SOFTWARE_PART_NUMBER);
            Console.Write("Version: \t{0}\n\n", SOFTWARE_VERSION);

            #endregion

            #region Get serial number

            bool useEnteredSerialNumber = false;

            do
            {
                Console.Write("\n");
                Console.Write("Enter unit serial number: ");

                serialNumberString = Console.ReadLine();

                Console.Write("Use {0} for serial number? (Y/N): ", serialNumberString);

                string confirm = Console.ReadLine();

                if ((confirm == "Y") || (confirm == "y"))
                {
                    useEnteredSerialNumber = true;
                }

            } while (useEnteredSerialNumber == false);

            #endregion

            #region Setup serial port

            StringComparer stringComparer = StringComparer.OrdinalIgnoreCase;

            // Create a new SerialPort object with default settings.
            serialPort = new SerialPort();

            // Allow the user to set the appropriate properties.
            serialPort.PortName = "COM18";   // SetPortName(serialPort.PortName);
            serialPort.BaudRate = 9600;  // SetPortBaudRate(serialPort.BaudRate);

            // Set the read/write timeouts
            serialPort.ReadTimeout = 2000;
            serialPort.WriteTimeout = 2000;

            serialPort.Open();

            #endregion

            #region Place unit in quiet mode

            SendCommand(commandQuietMode);
            SendCommand(commandQuietMode);
            SendCommand(commandQuietMode);

            // wait for all incoming serial data to be received
            Thread.Sleep(2000);

            // flush serial port
            serialPort.ReadExisting();

            #endregion

            #region Ping unit

            // send ping
            Console.Write("\n");
            Console.Write("Send ping: \t\t\t\t\t\t");

            byte[] receivedPing = new byte[SYNC_HEADER.Length + responsePing.Length + 2];

            SendCommand(commandPing);
            ReceiveResponse(ref receivedPing, (SYNC_HEADER.Length + responsePing.Length + 2), READ_TIMEOUT);
            unitPresent = CheckResponse(responsePing, receivedPing);

            #endregion

            if (unitPresent == true)
            {
                #region Unlock EEPROM

                byte[] receivedReadUnitSerialNumber = new byte[SYNC_HEADER.Length + responseReadUnitSerialNumber.Length + 2];

                SendCommand(commandReadUnitSerialNumber);
                ReceiveResponse(ref receivedReadUnitSerialNumber, (SYNC_HEADER.Length + responseReadUnitSerialNumber.Length + 2), READ_TIMEOUT);

                Array.Copy(receivedReadUnitSerialNumber, 8, serialNumber, 0, 4);

                // unlock EEPROM with serial number CRC
                UInt16 serialNumberCrc = Crc(serialNumber, Convert.ToUInt32(serialNumber.Length), 0x1d0f);

                commandUnlockEeprom[3] = Convert.ToByte((serialNumberCrc >> 8) & 0xff);
                commandUnlockEeprom[4] = Convert.ToByte(serialNumberCrc & 0xff);

                Console.Write("\n");
                Console.Write("Send unlock EEPROM command: \t\t\t\t");

                byte[] receivedUnlockEeprom = new byte[SYNC_HEADER.Length + responseUnlockEeprom.Length + 2];

                SendCommand(commandUnlockEeprom);
                ReceiveResponse(ref receivedUnlockEeprom, (SYNC_HEADER.Length + responseUnlockEeprom.Length + 2), READ_TIMEOUT);

                if (CheckResponse(responseUnlockEeprom, receivedUnlockEeprom) != true)
                {
                    configSuccess = false;
                }

                #endregion

                #region Write serial number

                Console.Write("\n");
                Console.Write("Write unit serial number: \t\t\t\t");

                byte[] receivedWriteUnitSerialNumber = new byte[SYNC_HEADER.Length + responseWriteUnitSerialNumber.Length + 2];

                commandWriteUnitSerialNumber[6] = Convert.ToByte((Convert.ToUInt32(serialNumberString) >> 8 ) & 0xff);
                commandWriteUnitSerialNumber[7] = Convert.ToByte((Convert.ToUInt32(serialNumberString) >> 0 ) & 0xff);
                commandWriteUnitSerialNumber[8] = Convert.ToByte((Convert.ToUInt32(serialNumberString) >> 24 ) & 0xff);
                commandWriteUnitSerialNumber[9] = Convert.ToByte((Convert.ToUInt32(serialNumberString) >> 16) & 0xff);

                SendCommand(commandWriteUnitSerialNumber);
                ReceiveResponse(ref receivedWriteUnitSerialNumber, (SYNC_HEADER.Length + responseWriteUnitSerialNumber.Length + 2), WRITE_TIMEOUT);

                if (CheckResponse(responseWriteUnitSerialNumber, receivedWriteUnitSerialNumber) != true)
                {
                    configSuccess = false;
                }

                // flush serial port
                serialPort.ReadExisting();

                #endregion

                #region Verify serial number

                Console.Write("\n");
                Console.Write("Verify serial number: \t\t\t\t\t");

                receivedReadUnitSerialNumber = new byte[SYNC_HEADER.Length + responseReadUnitSerialNumber.Length + 2];

                SendCommand(commandReadUnitSerialNumber);
                ReceiveResponse(ref receivedReadUnitSerialNumber, (SYNC_HEADER.Length + responseReadUnitSerialNumber.Length + 2), READ_TIMEOUT);

                responseReadUnitSerialNumber[6] = Convert.ToByte((Convert.ToUInt32(serialNumberString) >> 8 ) & 0xff);
                responseReadUnitSerialNumber[7] = Convert.ToByte((Convert.ToUInt32(serialNumberString) >> 0 ) & 0xff);
                responseReadUnitSerialNumber[8] = Convert.ToByte((Convert.ToUInt32(serialNumberString) >> 24) & 0xff);
                responseReadUnitSerialNumber[9] = Convert.ToByte((Convert.ToUInt32(serialNumberString) >> 16) & 0xff);

                if (CheckResponse(responseReadUnitSerialNumber, receivedReadUnitSerialNumber) != true)
                {
                    configSuccess = false;
                }

                // flush serial port
                serialPort.ReadExisting();

                #endregion

                #region Write version string

                Console.Write("\n");
                Console.Write("Write version string: \t\t\t\t");

                byte[] receivedWriteVersionString = new byte[SYNC_HEADER.Length + responseWriteVersionString.Length + 2];

                byte[] appendedCommandWriteVersionString = new byte[commandWriteVersionString.Length + (VERSION_STRING.Length + 1) * 2];

                commandWriteVersionString.CopyTo(appendedCommandWriteVersionString, 0);

                for (uint byteIndex = 0; byteIndex < VERSION_STRING.Length; ++byteIndex)
                {
                    appendedCommandWriteVersionString[(byteIndex) + commandWriteVersionString.Length] = Convert.ToByte(VERSION_STRING[byteIndex]);
                }

                SendCommand(appendedCommandWriteVersionString);
                ReceiveResponse(ref receivedWriteVersionString, (SYNC_HEADER.Length + responseWriteVersionString.Length + 2), WRITE_TIMEOUT);

                if (CheckResponse(responseWriteVersionString, receivedWriteVersionString) != true)
                {
                    configSuccess = false;
                }

                // flush serial port
                serialPort.ReadExisting();

                #endregion

                #region Verify version string

                Console.Write("\n");
                Console.Write("Verify version string: \t\t\t\t\t");

                byte[] receivedReadVersionString = new byte[SYNC_HEADER.Length + responseReadVersionString.Length + 2];

                byte[] appendedResponseReadVersionString = new byte[responseReadVersionString.Length + (VERSION_STRING.Length + 1) * 2];

                responseReadVersionString.CopyTo(appendedResponseReadVersionString, 0);

                for (uint byteIndex = 0; byteIndex < VERSION_STRING.Length; ++byteIndex)
                {
                    appendedResponseReadVersionString[(byteIndex) + responseReadVersionString.Length] = Convert.ToByte(VERSION_STRING[byteIndex]);
                }

                SendCommand(commandReadVersionString);
                ReceiveResponse(ref receivedReadVersionString, (SYNC_HEADER.Length + responseReadVersionString.Length + (VERSION_STRING.Length + 1) * 2 + 2), READ_TIMEOUT);

                if (CheckResponse(appendedResponseReadVersionString, receivedReadVersionString) != true)
                {
                    configSuccess = false;
                }

                // flush serial port
                serialPort.ReadExisting();

                #endregion

                #region Write product configuration

                Console.Write("\n");
                Console.Write("Write product configuration: \t\t\t\t");

                byte[] receivedWriteProductConfiguration = new byte[SYNC_HEADER.Length + responseWriteProductConfiguration.Length + 2];

                SendCommand(commandWriteProductConfiguration);
                ReceiveResponse(ref receivedWriteProductConfiguration, (SYNC_HEADER.Length + responseWriteProductConfiguration.Length + 2), WRITE_TIMEOUT);

                if (CheckResponse(responseWriteProductConfiguration, receivedWriteProductConfiguration) != true)
                {
                    configSuccess = false;
                }

                // flush serial port
                serialPort.ReadExisting();

                #endregion

                #region Verify product configuration

                Console.Write("\n");
                Console.Write("Verify product configuration: \t\t\t\t");

                byte[] receivedReadProductConfiguration = new byte[SYNC_HEADER.Length + responseReadProductConfiguration.Length + 2];

                SendCommand(commandReadProductConfiguration);
                ReceiveResponse(ref receivedReadProductConfiguration, (SYNC_HEADER.Length + responseReadProductConfiguration.Length + 2), READ_TIMEOUT);

                if (CheckResponse(responseReadProductConfiguration, receivedReadProductConfiguration) != true)
                {
                    configSuccess = false;
                }

                // flush serial port
                serialPort.ReadExisting();

                #endregion

                #region Write packet type

                Console.Write("\n");
                Console.Write("Write packet type: \t\t\t\t\t");

                byte[] receivedWritePacketType = new byte[SYNC_HEADER.Length + responseWritePacketType.Length + 2];

                SendCommand(commandWritePacketType);
                ReceiveResponse(ref receivedWritePacketType, (SYNC_HEADER.Length + responseWritePacketType.Length + 2), WRITE_TIMEOUT);

                if (CheckResponse(responseWritePacketType, receivedWritePacketType) != true)
                {
                    configSuccess = false;
                }

                // flush serial port
                serialPort.ReadExisting();

                #endregion

                #region Verify packet type

                Console.Write("\n");
                Console.Write("Verify packet type: \t\t\t\t\t");

                byte[] receivedReadPacketType = new byte[SYNC_HEADER.Length + responseReadPacketType.Length + 2];

                SendCommand(commandReadPacketType);
                ReceiveResponse(ref receivedReadPacketType, (SYNC_HEADER.Length + responseReadPacketType.Length + 2), READ_TIMEOUT);

                if (CheckResponse(responseReadPacketType, receivedReadPacketType) != true)
                {
                    configSuccess = false;
                }

                // flush serial port
                serialPort.ReadExisting();

                #endregion

                #region Write packet rate

                Console.Write("\n");
                Console.Write("Write packet rate: \t\t\t\t\t");

                byte[] receivedWritePacketRate = new byte[SYNC_HEADER.Length + responseWritePacketRate.Length + 2];

                SendCommand(commandWritePacketRate);
                ReceiveResponse(ref receivedWritePacketRate, (SYNC_HEADER.Length + responseWritePacketRate.Length + 2), WRITE_TIMEOUT);

                if (CheckResponse(responseWritePacketRate, receivedWritePacketRate) != true)
                {
                    configSuccess = false;
                }

                // flush serial port
                serialPort.ReadExisting();

                #endregion

                #region Verify packet rate

                Console.Write("\n");
                Console.Write("Verify packet rate: \t\t\t\t\t");

                byte[] receivedReadPacketRate = new byte[SYNC_HEADER.Length + responseReadPacketRate.Length + 2];

                SendCommand(commandReadPacketRate);
                ReceiveResponse(ref receivedReadPacketRate, (SYNC_HEADER.Length + responseReadPacketRate.Length + 2), READ_TIMEOUT);

                if (CheckResponse(responseReadPacketRate, receivedReadPacketRate) != true)
                {
                    configSuccess = false;
                }

                // flush serial port
                serialPort.ReadExisting();

                #endregion

                #region Write port 1 baud rate

                Console.Write("\n");
                Console.Write("Write port 1 baud rate: \t\t\t\t");

                byte[] receivedWritePort1BaudRate = new byte[SYNC_HEADER.Length + responseWritePort1BaudRate.Length + 2];

                SendCommand(commandWritePort1BaudRate);
                ReceiveResponse(ref receivedWritePort1BaudRate, (SYNC_HEADER.Length + responseWritePort1BaudRate.Length + 2), WRITE_TIMEOUT);

                if (CheckResponse(responseWritePort1BaudRate, receivedWritePort1BaudRate) != true)
                {
                    configSuccess = false;
                }

                // flush serial port
                serialPort.ReadExisting();

                #endregion

                #region Verify port 1 baud rate

                Console.Write("\n");
                Console.Write("Verify port 1 rate: \t\t\t\t\t");

                byte[] receivedReadPort1BaudRate = new byte[SYNC_HEADER.Length + responseReadPort1BaudRate.Length + 2];

                SendCommand(commandReadPort1BaudRate);
                ReceiveResponse(ref receivedReadPort1BaudRate, (SYNC_HEADER.Length + responseReadPort1BaudRate.Length + 2), READ_TIMEOUT);

                if (CheckResponse(responseReadPort1BaudRate, receivedReadPort1BaudRate) != true)
                {
                    configSuccess = false;
                }

                // flush serial port
                serialPort.ReadExisting();

                #endregion

                #region Write port 2 baud rate

                Console.Write("\n");
                Console.Write("Write port 2 baud rate: \t\t\t\t");

                byte[] receivedWritePort2BaudRate = new byte[SYNC_HEADER.Length + responseWritePort2BaudRate.Length + 2];

                SendCommand(commandWritePort2BaudRate);
                ReceiveResponse(ref receivedWritePort2BaudRate, (SYNC_HEADER.Length + responseWritePort2BaudRate.Length + 2), WRITE_TIMEOUT);

                if (CheckResponse(responseWritePort2BaudRate, receivedWritePort2BaudRate) != true)
                {
                    configSuccess = false;
                }

                // flush serial port
                serialPort.ReadExisting();

                #endregion

                #region Verify port 2 baud rate

                Console.Write("\n");
                Console.Write("Verify port 2 rate: \t\t\t\t\t");

                byte[] receivedReadPort2BaudRate = new byte[SYNC_HEADER.Length + responseReadPort2BaudRate.Length + 2];

                SendCommand(commandReadPort2BaudRate);
                ReceiveResponse(ref receivedReadPort2BaudRate, (SYNC_HEADER.Length + responseReadPort2BaudRate.Length + 2), READ_TIMEOUT);

                if (CheckResponse(responseReadPort2BaudRate, receivedReadPort2BaudRate) != true)
                {
                    configSuccess = false;
                }

                // flush serial port
                serialPort.ReadExisting();

                #endregion

                #region Write port 3 baud rate

                Console.Write("\n");
                Console.Write("Write port 3 baud rate: \t\t\t\t");

                byte[] receivedWritePort3BaudRate = new byte[SYNC_HEADER.Length + responseWritePort3BaudRate.Length + 2];

                SendCommand(commandWritePort3BaudRate);
                ReceiveResponse(ref receivedWritePort3BaudRate, (SYNC_HEADER.Length + responseWritePort3BaudRate.Length + 2), WRITE_TIMEOUT);

                if (CheckResponse(responseWritePort3BaudRate, receivedWritePort3BaudRate) != true)
                {
                    configSuccess = false;
                }

                // flush serial port
                serialPort.ReadExisting();

                #endregion

                #region Verify port 3 baud rate

                Console.Write("\n");
                Console.Write("Verify port 3 rate: \t\t\t\t\t");

                byte[] receivedReadPort3BaudRate = new byte[SYNC_HEADER.Length + responseReadPort3BaudRate.Length + 2];

                SendCommand(commandReadPort3BaudRate);
                ReceiveResponse(ref receivedReadPort3BaudRate, (SYNC_HEADER.Length + responseReadPort3BaudRate.Length + 2), READ_TIMEOUT);

                if (CheckResponse(responseReadPort3BaudRate, receivedReadPort3BaudRate) != true)
                {
                    configSuccess = false;
                }

                // flush serial port
                serialPort.ReadExisting();

                #endregion

                #region Write port 4 baud rate

                Console.Write("\n");
                Console.Write("Write port 4 baud rate: \t\t\t\t");

                byte[] receivedWritePort4BaudRate = new byte[SYNC_HEADER.Length + responseWritePort4BaudRate.Length + 2];

                SendCommand(commandWritePort4BaudRate);
                ReceiveResponse(ref receivedWritePort4BaudRate, (SYNC_HEADER.Length + responseWritePort4BaudRate.Length + 2), WRITE_TIMEOUT);

                if (CheckResponse(responseWritePort4BaudRate, receivedWritePort4BaudRate) != true)
                {
                    configSuccess = false;
                }

                // flush serial port
                serialPort.ReadExisting();

                #endregion

                #region Verify port 4 baud rate

                Console.Write("\n");
                Console.Write("Verify port 4 rate: \t\t\t\t\t");

                byte[] receivedReadPort4BaudRate = new byte[SYNC_HEADER.Length + responseReadPort4BaudRate.Length + 2];

                SendCommand(commandReadPort4BaudRate);
                ReceiveResponse(ref receivedReadPort4BaudRate, (SYNC_HEADER.Length + responseReadPort4BaudRate.Length + 2), READ_TIMEOUT);

                if (CheckResponse(responseReadPort4BaudRate, receivedReadPort4BaudRate) != true)
                {
                    configSuccess = false;
                }

                // flush serial port
                serialPort.ReadExisting();

                #endregion

                #region Write port 1 usage

                Console.Write("\n");
                Console.Write("Write port 1 usage: \t\t\t\t\t");

                byte[] receivedWritePort1Usage = new byte[SYNC_HEADER.Length + responseWritePort1Usage.Length + 2];

                SendCommand(commandWritePort1Usage);
                ReceiveResponse(ref receivedWritePort1Usage, (SYNC_HEADER.Length + responseWritePort1Usage.Length + 2), WRITE_TIMEOUT);

                if (CheckResponse(responseWritePort1Usage, receivedWritePort1Usage) != true)
                {
                    configSuccess = false;
                }

                // flush serial port
                serialPort.ReadExisting();

                #endregion

                #region Verify port 1 usage

                Console.Write("\n");
                Console.Write("Verify port 1 usage: \t\t\t\t\t");

                byte[] receivedReadPort1Usage = new byte[SYNC_HEADER.Length + responseReadPort1Usage.Length + 2];

                SendCommand(commandReadPort1Usage);
                ReceiveResponse(ref receivedReadPort1Usage, (SYNC_HEADER.Length + responseReadPort1Usage.Length + 2), READ_TIMEOUT);

                if (CheckResponse(responseReadPort1Usage, receivedReadPort1Usage) != true)
                {
                    configSuccess = false;
                }

                // flush serial port
                serialPort.ReadExisting();

                #endregion

                #region Write port 2 usage

                Console.Write("\n");
                Console.Write("Write port 2 usage: \t\t\t\t\t");

                byte[] receivedWritePort2Usage = new byte[SYNC_HEADER.Length + responseWritePort2Usage.Length + 2];

                SendCommand(commandWritePort2Usage);
                ReceiveResponse(ref receivedWritePort2Usage, (SYNC_HEADER.Length + responseWritePort2Usage.Length + 2), WRITE_TIMEOUT);

                if (CheckResponse(responseWritePort2Usage, receivedWritePort2Usage) != true)
                {
                    configSuccess = false;
                }

                // flush serial port
                serialPort.ReadExisting();

                #endregion

                #region Verify port 2 usage

                Console.Write("\n");
                Console.Write("Verify port 2 usage: \t\t\t\t\t");

                byte[] receivedReadPort2Usage = new byte[SYNC_HEADER.Length + responseReadPort2Usage.Length + 2];

                SendCommand(commandReadPort2Usage);
                ReceiveResponse(ref receivedReadPort2Usage, (SYNC_HEADER.Length + responseReadPort2Usage.Length + 2), READ_TIMEOUT);

                if (CheckResponse(responseReadPort2Usage, receivedReadPort2Usage) != true)
                {
                    configSuccess = false;
                }

                // flush serial port
                serialPort.ReadExisting();

                #endregion

                #region Write port 3 usage

                Console.Write("\n");
                Console.Write("Write port 3 usage: \t\t\t\t\t");

                byte[] receivedWritePort3Usage = new byte[SYNC_HEADER.Length + responseWritePort3Usage.Length + 2];

                SendCommand(commandWritePort3Usage);
                ReceiveResponse(ref receivedWritePort3Usage, (SYNC_HEADER.Length + responseWritePort3Usage.Length + 2), WRITE_TIMEOUT);

                if (CheckResponse(responseWritePort3Usage, receivedWritePort3Usage) != true)
                {
                    configSuccess = false;
                }

                // flush serial port
                serialPort.ReadExisting();

                #endregion

                #region Verify port 3 usage

                Console.Write("\n");
                Console.Write("Verify port 3 usage: \t\t\t\t\t");

                byte[] receivedReadPort3Usage = new byte[SYNC_HEADER.Length + responseReadPort3Usage.Length + 2];

                SendCommand(commandReadPort3Usage);
                ReceiveResponse(ref receivedReadPort3Usage, (SYNC_HEADER.Length + responseReadPort3Usage.Length + 2), READ_TIMEOUT);

                if (CheckResponse(responseReadPort3Usage, receivedReadPort3Usage) != true)
                {
                    configSuccess = false;
                }

                // flush serial port
                serialPort.ReadExisting();

                #endregion

                #region Write port 4 usage

                Console.Write("\n");
                Console.Write("Write port 4 usage: \t\t\t\t\t");

                byte[] receivedWritePort4Usage = new byte[SYNC_HEADER.Length + responseWritePort4Usage.Length + 2];

                SendCommand(commandWritePort4Usage);
                ReceiveResponse(ref receivedWritePort4Usage, (SYNC_HEADER.Length + responseWritePort4Usage.Length + 2), WRITE_TIMEOUT);

                if (CheckResponse(responseWritePort4Usage, receivedWritePort4Usage) != true)
                {
                    configSuccess = false;
                }

                // flush serial port
                serialPort.ReadExisting();

                #endregion

                #region Verify port 4 usage

                Console.Write("\n");
                Console.Write("Verify port 4 usage: \t\t\t\t\t");

                byte[] receivedReadPort4Usage = new byte[SYNC_HEADER.Length + responseReadPort4Usage.Length + 2];

                SendCommand(commandReadPort4Usage);
                ReceiveResponse(ref receivedReadPort4Usage, (SYNC_HEADER.Length + responseReadPort4Usage.Length + 2), READ_TIMEOUT);

                if (CheckResponse(responseReadPort4Usage, receivedReadPort4Usage) != true)
                {
                    configSuccess = false;
                }

                // flush serial port
                serialPort.ReadExisting();

                #endregion

                #region Write orientation

                Console.Write("\n");
                Console.Write("Write orientation: \t\t\t\t\t");

                byte[] receivedWriteOrientation = new byte[SYNC_HEADER.Length + responseWriteOrientation.Length + 2];

                SendCommand(commandWriteOrientation);
                ReceiveResponse(ref receivedWriteOrientation, (SYNC_HEADER.Length + responseWriteOrientation.Length + 2), WRITE_TIMEOUT);

                if (CheckResponse(responseWriteOrientation, receivedWriteOrientation) != true)
                {
                    configSuccess = false;
                }

                // flush serial port
                serialPort.ReadExisting();

                #endregion

                #region Verify orientation

                Console.Write("\n");
                Console.Write("Verify orientation: \t\t\t\t\t");

                byte[] receivedReadOrientation = new byte[SYNC_HEADER.Length + responseReadOrientation.Length + 2];

                SendCommand(commandReadOrientation);
                ReceiveResponse(ref receivedReadOrientation, (SYNC_HEADER.Length + responseReadOrientation.Length + 2), READ_TIMEOUT);

                if (CheckResponse(responseReadOrientation, receivedReadOrientation) != true)
                {
                    configSuccess = false;
                }

                // flush serial port
                serialPort.ReadExisting();

                #endregion

                #region Write hardware status enable

                Console.Write("\n");
                Console.Write("Write hardware status enable: \t\t\t\t");

                byte[] receivedWriteHardwareStatusEn = new byte[SYNC_HEADER.Length + responseWriteHardwareStatusEn.Length + 2];

                SendCommand(commandWriteHardwareStatusEn);
                ReceiveResponse(ref receivedWriteHardwareStatusEn, (SYNC_HEADER.Length + responseWriteHardwareStatusEn.Length + 2), WRITE_TIMEOUT);

                if (CheckResponse(responseWriteHardwareStatusEn, receivedWriteHardwareStatusEn) != true)
                {
                    configSuccess = false;
                }

                // flush serial port
                serialPort.ReadExisting();

                #endregion

                #region Verify hardware status enable

                Console.Write("\n");
                Console.Write("Verify hardware status enable: \t\t\t\t");

                byte[] receivedReadHardwareStatusEn = new byte[SYNC_HEADER.Length + responseReadHardwareStatusEn.Length + 2];

                SendCommand(commandReadHardwareStatusEn);
                ReceiveResponse(ref receivedReadHardwareStatusEn, (SYNC_HEADER.Length + responseReadHardwareStatusEn.Length + 2), READ_TIMEOUT);

                if (CheckResponse(responseReadHardwareStatusEn, receivedReadHardwareStatusEn) != true)
                {
                    configSuccess = false;
                }

                // flush serial port
                serialPort.ReadExisting();

                #endregion

                #region Write software status enable

                Console.Write("\n");
                Console.Write("Write software status enable: \t\t\t\t");

                byte[] receivedWriteSoftwareStatusEn = new byte[SYNC_HEADER.Length + responseWriteSoftwareStatusEn.Length + 2];

                SendCommand(commandWriteSoftwareStatusEn);
                ReceiveResponse(ref receivedWriteSoftwareStatusEn, (SYNC_HEADER.Length + responseWriteSoftwareStatusEn.Length + 2), WRITE_TIMEOUT);

                if (CheckResponse(responseWriteSoftwareStatusEn, receivedWriteSoftwareStatusEn) != true)
                {
                    configSuccess = false;
                }

                // flush serial port
                serialPort.ReadExisting();

                #endregion

                #region Verify software status enable

                Console.Write("\n");
                Console.Write("Verify software status enable: \t\t\t\t");

                byte[] receivedReadSoftwareStatusEn = new byte[SYNC_HEADER.Length + responseReadSoftwareStatusEn.Length + 2];

                SendCommand(commandReadSoftwareStatusEn);
                ReceiveResponse(ref receivedReadSoftwareStatusEn, (SYNC_HEADER.Length + responseReadSoftwareStatusEn.Length + 2), READ_TIMEOUT);

                if (CheckResponse(responseReadSoftwareStatusEn, receivedReadSoftwareStatusEn) != true)
                {
                    configSuccess = false;
                }

                // flush serial port
                serialPort.ReadExisting();

                #endregion

                #region Write sensor status enable

                Console.Write("\n");
                Console.Write("Write sensor status enable: \t\t\t\t");

                byte[] receivedWriteSensorStatusEn = new byte[SYNC_HEADER.Length + responseWriteSensorStatusEn.Length + 2];

                SendCommand(commandWriteSensorStatusEn);
                ReceiveResponse(ref receivedWriteSensorStatusEn, (SYNC_HEADER.Length + responseWriteSensorStatusEn.Length + 2), WRITE_TIMEOUT);

                if (CheckResponse(responseWriteSensorStatusEn, receivedWriteSensorStatusEn) != true)
                {
                    configSuccess = false;
                }

                // flush serial port
                serialPort.ReadExisting();

                #endregion

                #region Verify sensor status enable

                Console.Write("\n");
                Console.Write("Verify sensor status enable: \t\t\t\t");

                byte[] receivedReadSensorStatusEn = new byte[SYNC_HEADER.Length + responseReadSensorStatusEn.Length + 2];

                SendCommand(commandReadSensorStatusEn);
                ReceiveResponse(ref receivedReadSensorStatusEn, (SYNC_HEADER.Length + responseReadSensorStatusEn.Length + 2), READ_TIMEOUT);

                if (CheckResponse(responseReadSensorStatusEn, receivedReadSensorStatusEn) != true)
                {
                    configSuccess = false;
                }

                // flush serial port
                serialPort.ReadExisting();

                #endregion

                #region Write communication status enable

                Console.Write("\n");
                Console.Write("Write communication status enable: \t\t\t");

                byte[] receivedWriteComStatusEn = new byte[SYNC_HEADER.Length + responseWriteComStatusEn.Length + 2];

                SendCommand(commandWriteComStatusEn);
                ReceiveResponse(ref receivedWriteComStatusEn, (SYNC_HEADER.Length + responseWriteComStatusEn.Length + 2), WRITE_TIMEOUT);

                if (CheckResponse(responseWriteComStatusEn, receivedWriteComStatusEn) != true)
                {
                    configSuccess = false;
                }

                // flush serial port
                serialPort.ReadExisting();

                #endregion

                #region Verify communication status enable

                Console.Write("\n");
                Console.Write("Verify communication status enable: \t\t\t");

                byte[] receivedReadComStatusEn = new byte[SYNC_HEADER.Length + responseReadComStatusEn.Length + 2];

                SendCommand(commandReadComStatusEn);
                ReceiveResponse(ref receivedReadComStatusEn, (SYNC_HEADER.Length + responseReadComStatusEn.Length + 2), READ_TIMEOUT);

                if (CheckResponse(responseReadComStatusEn, receivedReadComStatusEn) != true)
                {
                    configSuccess = false;
                }

                // flush serial port
                serialPort.ReadExisting();

                #endregion

                #region Write barometer setting

                Console.Write("\n");
                Console.Write("Write barometer setting: \t\t\t\t");

                byte[] receivedWriteBaroSetting = new byte[SYNC_HEADER.Length + responseWriteBaroSetting.Length + 2];

                SendCommand(commandWriteBaroSetting);
                ReceiveResponse(ref receivedWriteBaroSetting, (SYNC_HEADER.Length + responseWriteBaroSetting.Length + 2), WRITE_TIMEOUT);

                if (CheckResponse(responseWriteBaroSetting, receivedWriteBaroSetting) != true)
                {
                    configSuccess = false;
                }

                // flush serial port
                serialPort.ReadExisting();

                #endregion

                #region Verify barometer setting

                Console.Write("\n");
                Console.Write("Verify barometer setting: \t\t\t\t");

                byte[] receivedReadBaroSetting = new byte[SYNC_HEADER.Length + responseReadBaroSetting.Length + 2];

                SendCommand(commandReadBaroSetting);
                ReceiveResponse(ref receivedReadBaroSetting, (SYNC_HEADER.Length + responseReadBaroSetting.Length + 2), READ_TIMEOUT);

                if (CheckResponse(responseReadBaroSetting, receivedReadBaroSetting) != true)
                {
                    configSuccess = false;
                }

                // flush serial port
                serialPort.ReadExisting();

                #endregion

                #region Write roll offset

                Console.Write("\n");
                Console.Write("Write roll offset: \t\t\t\t\t");

                byte[] receivedWriteRollOffset = new byte[SYNC_HEADER.Length + responseWriteRollOffset.Length + 2];

                SendCommand(commandWriteRollOffset);
                ReceiveResponse(ref receivedWriteRollOffset, (SYNC_HEADER.Length + responseWriteRollOffset.Length + 2), WRITE_TIMEOUT);

                if (CheckResponse(responseWriteRollOffset, receivedWriteRollOffset) != true)
                {
                    configSuccess = false;
                }

                // flush serial port
                serialPort.ReadExisting();

                #endregion

                #region Verify roll offset

                Console.Write("\n");
                Console.Write("Verify roll offset: \t\t\t\t\t");

                byte[] receivedReadRollOffset = new byte[SYNC_HEADER.Length + responseReadRollOffset.Length + 2];

                SendCommand(commandReadRollOffset);
                ReceiveResponse(ref receivedReadRollOffset, (SYNC_HEADER.Length + responseReadRollOffset.Length + 2), READ_TIMEOUT);

                if (CheckResponse(responseReadRollOffset, receivedReadRollOffset) != true)
                {
                    configSuccess = false;
                }

                // flush serial port
                serialPort.ReadExisting();

                #endregion

                #region Write pitch offset

                Console.Write("\n");
                Console.Write("Write pitch offset: \t\t\t\t\t");

                byte[] receivedWritePitchOffset = new byte[SYNC_HEADER.Length + responseWritePitchOffset.Length + 2];

                SendCommand(commandWritePitchOffset);
                ReceiveResponse(ref receivedWritePitchOffset, (SYNC_HEADER.Length + responseWritePitchOffset.Length + 2), WRITE_TIMEOUT);

                if (CheckResponse(responseWritePitchOffset, receivedWritePitchOffset) != true)
                {
                    configSuccess = false;
                }

                // flush serial port
                serialPort.ReadExisting();

                #endregion

                #region Verify pitch offset

                Console.Write("\n");
                Console.Write("Verify pitch offset: \t\t\t\t\t");

                byte[] receivedReadPitchOffset = new byte[SYNC_HEADER.Length + responseReadPitchOffset.Length + 2];

                SendCommand(commandReadPitchOffset);
                ReceiveResponse(ref receivedReadPitchOffset, (SYNC_HEADER.Length + responseReadPitchOffset.Length + 2), READ_TIMEOUT);

                if (CheckResponse(responseReadPitchOffset, receivedReadPitchOffset) != true)
                {
                    configSuccess = false;
                }

                // flush serial port
                serialPort.ReadExisting();

                #endregion

                #region Write yaw offset

                Console.Write("\n");
                Console.Write("Write yaw offset: \t\t\t\t\t");

                byte[] receivedWriteYawOffset = new byte[SYNC_HEADER.Length + responseWriteYawOffset.Length + 2];

                SendCommand(commandWriteYawOffset);
                ReceiveResponse(ref receivedWriteYawOffset, (SYNC_HEADER.Length + responseWriteYawOffset.Length + 2), WRITE_TIMEOUT);

                if (CheckResponse(responseWriteYawOffset, receivedWriteYawOffset) != true)
                {
                    configSuccess = false;
                }

                // flush serial port
                serialPort.ReadExisting();

                #endregion

                #region Verify yaw offset

                Console.Write("\n");
                Console.Write("Verify yaw offset: \t\t\t\t\t");

                byte[] receivedReadYawOffset = new byte[SYNC_HEADER.Length + responseReadYawOffset.Length + 2];

                SendCommand(commandReadYawOffset);
                ReceiveResponse(ref receivedReadYawOffset, (SYNC_HEADER.Length + responseReadYawOffset.Length + 2), READ_TIMEOUT);

                if (CheckResponse(responseReadYawOffset, receivedReadYawOffset) != true)
                {
                    configSuccess = false;
                }

                // flush serial port
                serialPort.ReadExisting();

                #endregion

////////////////////////////////////////////////////////////////////////////
                /////////////////////////////////////////////////////////////////////////////
                #region write ROLL_OFFSET_EXT_MAG

                Console.Write("\n");
                Console.Write("Write ROLL_OFFSET_EXT_MAG: \t\t\t\t\t");

                byte[] receivedWriteRollOffsetExtMag = new byte[SYNC_HEADER.Length + responseWriteRollOffsetExtMag.Length + 2];

                SendCommand(commandWriteRollOffsetExtMag);
                ReceiveResponse(ref receivedWriteRollOffsetExtMag, (SYNC_HEADER.Length + responseWriteRollOffsetExtMag.Length + 2), WRITE_TIMEOUT);

                if (CheckResponse(responseWriteRollOffsetExtMag, receivedWriteRollOffsetExtMag) != true)
                {
                    configSuccess = false;
                }

                // flush serial port
                serialPort.ReadExisting();

                #endregion

                #region Verify ROLL_OFFSET_EXT_MAG

                Console.Write("\n");
                Console.Write("Verify ROLL_OFFSET_EXT_MAG: \t\t\t\t\t");

                byte[] receivedReadRollOffsetExtMag = new byte[SYNC_HEADER.Length + responseReadRollOffsetExtMag.Length + 2];

                SendCommand(commandReadRollOffsetExtMag);
                ReceiveResponse(ref receivedReadRollOffsetExtMag, (SYNC_HEADER.Length + responseReadRollOffsetExtMag.Length + 2), READ_TIMEOUT);

                if (CheckResponse(responseReadRollOffsetExtMag, receivedReadRollOffsetExtMag) != true)
                {
                    configSuccess = false;
                }

                // flush serial port
                serialPort.ReadExisting();

                #endregion


                #region write PITCH_OFFSET_EXT_MAG

                Console.Write("\n");
                Console.Write("Write PITCH_OFFSET_EXT_MAG: \t\t\t\t\t");

                byte[] receivedWritePitchOffsetExtMag = new byte[SYNC_HEADER.Length + responseWritePitchOffsetExtMag.Length + 2];

                SendCommand(commandWritePitchOffsetExtMag);
                ReceiveResponse(ref receivedWritePitchOffsetExtMag, (SYNC_HEADER.Length + responseWritePitchOffsetExtMag.Length + 2), WRITE_TIMEOUT);

                if (CheckResponse(responseWritePitchOffsetExtMag, receivedWritePitchOffsetExtMag) != true)
                {
                    configSuccess = false;
                }

                // flush serial port
                serialPort.ReadExisting();

                #endregion

                #region Verify PITCH_OFFSET_EXT_MAG

                Console.Write("\n");
                Console.Write("Verify PITCH_OFFSET_EXT_MAG: \t\t\t\t\t");

                byte[] receivedReadPitchOffsetExtMag = new byte[SYNC_HEADER.Length + responseReadPitchOffsetExtMag.Length + 2];

                SendCommand(commandReadPitchOffsetExtMag);
                ReceiveResponse(ref receivedReadPitchOffsetExtMag, (SYNC_HEADER.Length + responseReadPitchOffsetExtMag.Length + 2), READ_TIMEOUT);

                if (CheckResponse(responseReadPitchOffsetExtMag, receivedReadPitchOffsetExtMag) != true)
                {
                    configSuccess = false;
                }

                // flush serial port
                serialPort.ReadExisting();

                #endregion


                #region write X_HARD_IRON

                Console.Write("\n");
                Console.Write("Write X_HARD_IRON: \t\t\t\t\t");

                byte[] receivedWriteXhardIron = new byte[SYNC_HEADER.Length + responseWriteXhardIron.Length + 2];

                SendCommand(commandWriteXhardIron);
                ReceiveResponse(ref receivedWriteXhardIron, (SYNC_HEADER.Length + responseWriteXhardIron.Length + 2), WRITE_TIMEOUT);

                if (CheckResponse(responseWriteXhardIron, receivedWriteXhardIron) != true)
                {
                    configSuccess = false;
                }

                // flush serial port
                serialPort.ReadExisting();

                #endregion

                #region Verify X_HARD_IRON

                Console.Write("\n");
                Console.Write("Verify X_HARD_IRON: \t\t\t\t\t");

                byte[] receivedReadXhardIron = new byte[SYNC_HEADER.Length + responseReadXhardIron.Length + 2];

                SendCommand(commandReadXhardIron);
                ReceiveResponse(ref receivedReadXhardIron, (SYNC_HEADER.Length + responseReadXhardIron.Length + 2), READ_TIMEOUT);

                if (CheckResponse(responseReadXhardIron, receivedReadXhardIron) != true)
                {
                    configSuccess = false;
                }

                // flush serial port
                serialPort.ReadExisting();

                #endregion


                #region write Y_HARD_IRON

                Console.Write("\n");
                Console.Write("Write Y_HARD_IRON: \t\t\t\t\t");

                byte[] receivedWriteYhardIron = new byte[SYNC_HEADER.Length + responseWriteYhardIron.Length + 2];

                SendCommand(commandWriteYhardIron);
                ReceiveResponse(ref receivedWriteYhardIron, (SYNC_HEADER.Length + responseWriteYhardIron.Length + 2), WRITE_TIMEOUT);

                if (CheckResponse(responseWriteYhardIron, receivedWriteYhardIron) != true)
                {
                    configSuccess = false;
                }

                // flush serial port
                serialPort.ReadExisting();

                #endregion

                #region Verify Y_HARD_IRON

                Console.Write("\n");
                Console.Write("Verify Y_HARD_IRON: \t\t\t\t\t");

                byte[] receivedReadYhardIron = new byte[SYNC_HEADER.Length + responseReadYhardIron.Length + 2];

                SendCommand(commandReadYhardIron);
                ReceiveResponse(ref receivedReadYhardIron, (SYNC_HEADER.Length + responseReadYhardIron.Length + 2), READ_TIMEOUT);

                if (CheckResponse(responseReadYhardIron, receivedReadYhardIron) != true)
                {
                    configSuccess = false;
                }

                // flush serial port
                serialPort.ReadExisting();

                #endregion


                #region write SOFT_IRON_ANGLE_EXT

                Console.Write("\n");
                Console.Write("Write SOFT_IRON_ANGLE_EXT: \t\t\t\t\t");

                byte[] receivedWriteSoftIronAngleExt = new byte[SYNC_HEADER.Length + responseWriteSoftIronAngleExt.Length + 2];

                SendCommand(commandWriteSoftIronAngleExt);
                ReceiveResponse(ref receivedWriteSoftIronAngleExt, (SYNC_HEADER.Length + responseWriteSoftIronAngleExt.Length + 2), WRITE_TIMEOUT);

                if (CheckResponse(responseWriteSoftIronAngleExt, receivedWriteSoftIronAngleExt) != true)
                {
                    configSuccess = false;
                }

                // flush serial port
                serialPort.ReadExisting();

                #endregion

                #region Verify SOFT_IRON_ANGLE_EXT

                Console.Write("\n");
                Console.Write("Verify SOFT_IRON_ANGLE_EXT: \t\t\t\t\t");

                byte[] receivedReadSoftIronAngleExt = new byte[SYNC_HEADER.Length + responseReadSoftIronAngleExt.Length + 2];

                SendCommand(commandReadSoftIronAngleExt);
                ReceiveResponse(ref receivedReadSoftIronAngleExt, (SYNC_HEADER.Length + responseReadSoftIronAngleExt.Length + 2), READ_TIMEOUT);

                if (CheckResponse(responseReadSoftIronAngleExt, receivedReadSoftIronAngleExt) != true)
                {
                    configSuccess = false;
                }

                // flush serial port
                serialPort.ReadExisting();

                #endregion


                #region write SOFT_IRON_SCALE_RATIO

                Console.Write("\n");
                Console.Write("Write SOFT_IRON_SCALE_RATIO: \t\t\t\t\t");

                byte[] receivedWriteSoftIronSF = new byte[SYNC_HEADER.Length + responseWriteSoftIronSF.Length + 2];

                SendCommand(commandWriteSoftIronSF);
                ReceiveResponse(ref receivedWriteSoftIronSF, (SYNC_HEADER.Length + responseWriteSoftIronSF.Length + 2), WRITE_TIMEOUT);

                if (CheckResponse(responseWriteSoftIronSF, receivedWriteSoftIronSF) != true)
                {
                    configSuccess = false;
                }

                // flush serial port
                serialPort.ReadExisting();

                #endregion

                #region Verify SOFT_IRON_SCALE_RATIO

                Console.Write("\n");
                Console.Write("Verify SOFT_IRON_SCALE_RATIO: \t\t\t\t\t");

                byte[] receivedReadSoftIronSF = new byte[SYNC_HEADER.Length + responseReadSoftIronSF.Length + 2];

                SendCommand(commandReadSoftIronSF);
                ReceiveResponse(ref receivedReadSoftIronSF, (SYNC_HEADER.Length + responseReadSoftIronSF.Length + 2), READ_TIMEOUT);

                if (CheckResponse(responseReadSoftIronSF, receivedReadSoftIronSF) != true)
                {
                    configSuccess = false;
                }

                // flush serial port
                serialPort.ReadExisting();

                #endregion



/////////////////////////////////////////////////////////////////////////////
                #region Write inertial sensor range

                Console.Write("\n");
                Console.Write("Write inertial sensor range: \t\t\t");

                byte[] receivedWriteInertialSensorRange = new byte[SYNC_HEADER.Length + responseWriteInertialSensorRange.Length + 2];

                SendCommand(commandWriteInertialSensorRange);
                ReceiveResponse(ref receivedWriteInertialSensorRange, (SYNC_HEADER.Length + responseWriteInertialSensorRange.Length + 2), WRITE_TIMEOUT);

                if (CheckResponse(responseWriteInertialSensorRange, receivedWriteInertialSensorRange) != true)
                {
                    configSuccess = false;
                }

                // flush serial port
                serialPort.ReadExisting();

                #endregion

                #region Verify inertial sensor range

                Console.Write("\n");
                Console.Write("Verify inertial sensor range: \t\t\t");

                byte[] receivedReadInertialSensorRange = new byte[SYNC_HEADER.Length + responseReadInertialSensorRange.Length + 2];

                SendCommand(commandReadInertialSensorRange);
                ReceiveResponse(ref receivedReadInertialSensorRange, (SYNC_HEADER.Length + responseReadInertialSensorRange.Length + 2), READ_TIMEOUT);

                if (CheckResponse(responseReadInertialSensorRange, receivedReadInertialSensorRange) != true)
                {
                    configSuccess = false;
                }

                // flush serial port
                serialPort.ReadExisting();

                #endregion

////////////////////////////////////////////////////////////////////////////


                /////////////////////////////////////////////////////////////////////////////
                #region Write roll/pitch incidence limit

                Console.Write("\n");
                Console.Write("roll/pitch incidence limit: \t\t\t");

                byte[] receivedWriteRollPitchIncLimit = new byte[SYNC_HEADER.Length + responseWriteRollPitchIncLimit.Length + 2];

                SendCommand(commandWriteRollPitchIncLimit);
                ReceiveResponse(ref receivedWriteRollPitchIncLimit, (SYNC_HEADER.Length + responseWriteRollPitchIncLimit.Length + 2), WRITE_TIMEOUT);

                if (CheckResponse(responseWriteRollPitchIncLimit, receivedWriteRollPitchIncLimit) != true)
                {
                    configSuccess = false;
                }

                // flush serial port
                serialPort.ReadExisting();

                #endregion

                #region Verify roll/pitch incidence limit

                Console.Write("\n");
                Console.Write("Verify roll/pitch incidence limit: \t\t\t");

                byte[] receivedReadRollPitchIncLimit = new byte[SYNC_HEADER.Length + responseReadRollPitchIncLimit.Length + 2];

                SendCommand(commandReadRollPitchIncLimit);
                ReceiveResponse(ref receivedReadRollPitchIncLimit, (SYNC_HEADER.Length + responseReadRollPitchIncLimit.Length + 2), READ_TIMEOUT);

                if (CheckResponse(responseReadRollPitchIncLimit, receivedReadRollPitchIncLimit) != true)
                {
                    configSuccess = false;
                }

                // flush serial port
                serialPort.ReadExisting();

                #endregion

                #region Write hard/softiron limit

                Console.Write("\n");
                Console.Write("Write hard/softiron limit: \t\t\t");

                byte[] receivedWriteHardSoftIronLimit = new byte[SYNC_HEADER.Length + responseWriteHardSoftIronLimit.Length + 2];

                SendCommand(commandWriteHardSoftIronLimit);
                ReceiveResponse(ref receivedWriteHardSoftIronLimit, (SYNC_HEADER.Length + responseWriteHardSoftIronLimit.Length + 2), WRITE_TIMEOUT);

                if (CheckResponse(responseWriteHardSoftIronLimit, receivedWriteHardSoftIronLimit) != true)
                {
                    configSuccess = false;
                }

                // flush serial port
                serialPort.ReadExisting();

                #endregion

                #region Verify hard/softiron limit

                Console.Write("\n");
                Console.Write("Verify hard/softiron limit: \t\t\t");

                byte[] receivedReadHardSoftIronLimit = new byte[SYNC_HEADER.Length + responseReadHardSoftIronLimit.Length + 2];

                SendCommand(commandReadHardSoftIronLimit);
                ReceiveResponse(ref receivedReadHardSoftIronLimit, (SYNC_HEADER.Length + responseReadHardSoftIronLimit.Length + 2), READ_TIMEOUT);

                if (CheckResponse(responseReadHardSoftIronLimit, receivedReadHardSoftIronLimit) != true)
                {
                    configSuccess = false;
                }

                // flush serial port
                serialPort.ReadExisting();

                #endregion


////////////////////////////////////////////////////////////////////////////


                #region Compute new calibration CRC and write to EEPROM

                Console.Write("\n");

                UInt16 eepromCrc = 0x1d0f;

                // loop through all EEPROM locations, computing running CRC
                for (int eepromIndex = LOWER_CALIBRATION_ADDRESS; ((eepromIndex <= UPPER_CALIBRATION_ADDRESS) && (crcSuccess == true)); eepromIndex+=2)
                {
                    Console.Write("\rCompute CRC on location {0:X2} of {1:X2}: \t\t\t", new object[] { eepromIndex, UPPER_CALIBRATION_ADDRESS });

                    byte[] receivedReadEepromLocation = new byte[SYNC_HEADER.Length + responseReadEepromLocation.Length + 2];

                    commandReadEepromLocation[3] = Convert.ToByte((eepromIndex >> 8) & 0xff);
                    commandReadEepromLocation[4] = Convert.ToByte(eepromIndex & 0xff);

                    SendCommand(commandReadEepromLocation);
                    ReceiveResponse(ref receivedReadEepromLocation, (SYNC_HEADER.Length + responseReadEepromLocation.Length + 2), SHORT_READ_TIMEOUT);

                    if ((receivedReadEepromLocation[2] == commandReadEepromLocation[0]) &&
                        (receivedReadEepromLocation[3] == commandReadEepromLocation[1]) &&
                        (receivedReadEepromLocation[5] == Convert.ToByte((eepromIndex >> 8) & 0xff)) &&
                        (receivedReadEepromLocation[6] == Convert.ToByte(eepromIndex & 0xff)))
                    {
                        Console.Write("<PASS>");

                        byte[] eepromValue = new byte[2];

                        Array.Copy(receivedReadEepromLocation, 8, eepromValue, 0, 2);

                        eepromCrc = Crc(eepromValue, 2, eepromCrc);
                    }
                    else
                    {
                        Console.Write("<FAIL>");

                        crcSuccess = false;
                    }
                }

                Console.Write("\rCompute CRC on locations {0:X2} - {1:X2}: \t\t\t", new object[] { LOWER_CALIBRATION_ADDRESS, UPPER_CALIBRATION_ADDRESS });

                if (crcSuccess == true)
                {
                    Console.Write("<PASS>");
                }
                else
                {
                    Console.Write("<FAIL>");
                }

                // flush serial port
                serialPort.ReadExisting();

                // write new EEPROM CRC to EEPROM
                Console.Write("\n");
                Console.Write("Write EEPROM CRC: \t\t\t\t\t");

                byte[] receivedWriteEepromCrc = new byte[SYNC_HEADER.Length + responseWriteEepromCrc.Length + 2];

                commandWriteEepromCrc[6] = Convert.ToByte((eepromCrc >> 8) & 0xff);
                commandWriteEepromCrc[7] = Convert.ToByte(eepromCrc & 0xff);

                SendCommand(commandWriteEepromCrc);
                ReceiveResponse(ref receivedWriteEepromCrc, (SYNC_HEADER.Length + responseWriteEepromCrc.Length + 2), WRITE_TIMEOUT);

                if (CheckResponse(responseWriteEepromCrc, receivedWriteEepromCrc) != true)
                {
                    configSuccess = false;
                }

                // flush serial port
                serialPort.ReadExisting();

                #endregion
            }
            else
            {
                Console.Write("\n\n");

                Console.Write("ERROR: Can't establish communication with unit.\n\n");
            }

            #region Cleanup

            serialPort.Close();

            #endregion

            #region Emit pass or fail message

            if ( (unitPresent   == false) ||
                 (configSuccess == false) ||
                 (crcSuccess    == false) )
            {
                Console.Write("\n\n");

                Console.Write("Configuration FAILED\n\n");
            }
            else
            {
                Console.Write("\n\n");

                Console.Write("Configuration PASSED\n\n");
            }

            #endregion

            #region Wait for user input before exiting

            Console.Write("Press any key to exit.");
            Console.Read();

            #endregion
        }

        public static void DisplayCommand(byte[] command)
        {
            // display sync header
            Console.Write("{0:X2}", SYNC_HEADER[0]);
            Console.Write("{0:X2}", SYNC_HEADER[1]);

            // display packet contents
            foreach (byte byteVal in command)
            {
                Console.Write("{0:X2}", byteVal);
            }

            // compute CRC
            UInt16 crc = Crc(command, Convert.ToUInt32(command.Length), 0x1d0f);

            // display CRC
            Console.Write("{0:X2}", Convert.ToByte((crc >> 8) & 0xff));
            Console.Write("{0:X2}", Convert.ToByte(crc & 0xff));

            Console.Write("\n\n");
        }

        public static void SendCommand(byte[] command)
        {
            // compute CRC
            UInt16 crc = Crc(command, Convert.ToUInt32(command.Length), 0x1d0f);

            // send command to DUP
            Write(SYNC_HEADER);
            Write(command);
            Write(new byte[] { Convert.ToByte((crc >> 8) & 0xff), Convert.ToByte(crc & 0xff) });
        }

        public static void ReceiveResponse(ref byte[] response, int length, int timeout)
        {
            // sleep
            Thread.Sleep(timeout);

            response = Read(length);
        }

        public static bool CheckResponse(byte[] expectedResponse, byte[] receivedResponse)
        {
            UInt16 crc = Crc(expectedResponse, Convert.ToUInt32(expectedResponse.Length), 0x1d0f);    // CRC seed value

            // check for match
            bool match = true;

            // check for sync header match
            if ((receivedResponse[0] != SYNC_HEADER[0]) ||
                (receivedResponse[1] != SYNC_HEADER[1]))
            {
                match = false;
            }

            if (match != false)
            {
                for (int index = 0; ((index < expectedResponse.Length) && (index < receivedResponse.Length)); ++index)
                {
                    if (receivedResponse[index + SYNC_HEADER.Length] != expectedResponse[index])
                    {
                        match = false;
                    }
                }
            }

            if (match != false)
            {
                // check CRC match
                if ((receivedResponse[SYNC_HEADER.Length + expectedResponse.Length + 0] != Convert.ToByte((crc >> 8) & 0xff)) ||
                    (receivedResponse[SYNC_HEADER.Length + expectedResponse.Length + 1] != Convert.ToByte(crc & 0xff)))
                {
                    match = false;
                }
            }

            if (match == true)
            {
                Console.Write("<PASS>");
            }
            else
            {
                Console.Write("<FAIL>");
            }

            return match;
        }

        public static byte[] Read(int count)
        {
            byte[] message = new byte[100];

            try
            {
                serialPort.Read(message, 0, count);
            }
            catch (TimeoutException) { }

            return message;
        }

        public static void Write(byte[] message)
        {
            try
            {
                serialPort.Write(message, 0, message.Length);
            }
            catch (TimeoutException) { }    
        }

        public static string SetPortName(string defaultPortName)
        {
            string portName;

            Console.WriteLine("Available Ports:");
            foreach (string s in SerialPort.GetPortNames())
            {
                Console.WriteLine("   {0}", s);
            }

            Console.Write("COM port({0}): ", defaultPortName);
            portName = Console.ReadLine();

            if (portName == "")
            {
                portName = defaultPortName;
            }
            return portName;
        }

        public static int SetPortBaudRate(int defaultPortBaudRate)
        {
            string baudRate;

            Console.Write("Baud Rate({0}): ", defaultPortBaudRate);
            baudRate = Console.ReadLine();

            if (baudRate == "")
            {
                baudRate = defaultPortBaudRate.ToString();
            }

            return int.Parse(baudRate);
        }

        public static Parity SetPortParity(Parity defaultPortParity)
        {
            string parity;

            Console.WriteLine("Available Parity options:");
            foreach (string s in Enum.GetNames(typeof(Parity)))
            {
                Console.WriteLine("   {0}", s);
            }

            Console.Write("Parity({0}):", defaultPortParity.ToString());
            parity = Console.ReadLine();

            if (parity == "")
            {
                parity = defaultPortParity.ToString();
            }

            return (Parity)Enum.Parse(typeof(Parity), parity);
        }

        public static int SetPortDataBits(int defaultPortDataBits)
        {
            string dataBits;

            Console.Write("Data Bits({0}): ", defaultPortDataBits);
            dataBits = Console.ReadLine();

            if (dataBits == "")
            {
                dataBits = defaultPortDataBits.ToString();
            }

            return int.Parse(dataBits);
        }

        public static StopBits SetPortStopBits(StopBits defaultPortStopBits)
        {
            string stopBits;

            Console.WriteLine("Available Stop Bits options:");
            foreach (string s in Enum.GetNames(typeof(StopBits)))
            {
                Console.WriteLine("   {0}", s);
            }

            Console.Write("Stop Bits({0}):", defaultPortStopBits.ToString());
            stopBits = Console.ReadLine();

            if (stopBits == "")
            {
                stopBits = defaultPortStopBits.ToString();
            }

            return (StopBits)Enum.Parse(typeof(StopBits), stopBits);
        }

        public static Handshake SetPortHandshake(Handshake defaultPortHandshake)
        {
            string handshake;

            Console.WriteLine("Available Handshake options:");
            foreach (string s in Enum.GetNames(typeof(Handshake)))
            {
                Console.WriteLine("   {0}", s);
            }

            Console.Write("Handshake({0}):", defaultPortHandshake.ToString());
            handshake = Console.ReadLine();

            if (handshake == "")
            {
                handshake = defaultPortHandshake.ToString();
            }

            return (Handshake)Enum.Parse(typeof(Handshake), handshake);
        }

        public static UInt16 Crc(byte[] data, uint length, UInt16 seed) 
		{
			uint i,j;

			UInt16 crc = seed;  // initial CRC value  

			for (i = 0; i < length; ++i) 
			{
				crc ^= (ushort)(data[i] << 8);

				for (j = 0; j < 8; ++j) 
				{
					if ((crc & 0x8000) != 0)
                    {
						crc = (ushort)((crc << 1) ^ 0x1021);
                    }
					else 
                    {
                        crc = (ushort)(crc << 1);
                    }
				}
			}
			return crc;
		}
    }
}
