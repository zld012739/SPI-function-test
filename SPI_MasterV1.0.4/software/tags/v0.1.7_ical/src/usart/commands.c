/***************************************************************************
** commands.c
** Description:
** Commands available to the commandLine.c shell
**
**************************************************************************/

#include "commandline.h"
#include "commands.h"

#include "salvodefs.h"

#define LOGGING_LEVEL LEVEL_INFO
#include "debug.h"
#include "dmu380.h"
#include "dmu.h"
#include "temperatureSensor.h"
#include "magnetometer.h"
#include "accelerometer.h"
#include "gyroscope.h"

#include "timer.h"

#include <math.h> // for floating point sqrt, FIXME XXXOPT: could take this out

// Display firmware version
void CmdVersion(uint32_t data)
{
    DEBUG_STRING(FIRMWARE_ID_STRING);
    DEBUG_INT(" ", VERSION_MAJOR);
    DEBUG_INT(".", VERSION_MINOR);
    DEBUG_INT(".", VERSION_REV);
    DEBUG_ENDLINE();
}


// Read accelerometer <num reads, def !ault 10> <ms between reads, default asap>
void CmdReadSensors(uint32_t intputParameters)
{
    int16_t reading[NUM_AXIS*NUM_SENSORS];
    uint32_t numReads = 10;
    uint32_t msApart = 2;
    uint16_t gain[NUM_SENSORS];
    uint16_t magStart = TRUE;
    tTime startTime;
    int axis, sensor;
    int readInUnits = intputParameters & 0xF;

    CmdLineGetArgUInt(&numReads);
    CmdLineGetArgUInt(&msApart);

    INFO_INT("Reading sensors ", numReads);
    INFO_INT(" times, ", msApart);
    INFO_STRING(" ms apart\r\n");

    if (readInUnits) {
        gain[ACCEL_GAIN] = AccelerometerGetGain();
        gain[MAG_GAIN] = MagnetometerGetGain();

        if (gSystemConfiguration.useGyro2) { gain[GYRO_GAIN] = Gyro2GetGain();}
        else                               { gain[GYRO_GAIN] = GyroGetGain();}
    }

    DEBUG_STRING("\tt(ms)");
    DEBUG_STRING("\tAX\tAY\tAZ");
    DEBUG_STRING("\tGX\tGY\tGZ");
    DEBUG_STRING("\tMX\tMY\tMZ");
    DEBUG_ENDLINE();
    startTime = TimeNow();
    while (numReads) {
        OSClrEFlag(EFLAGS_DATA_READY, EF_DATA_GYRO_ACCEL_READY);
        // start everybody off
        AccelerometerStartReading();
        if (magStart) {
            OSClrEFlag(EFLAGS_DATA_READY, EF_DATA_MAG_READY);
            MagnetometerStartReading();
            magStart = FALSE;
        }

        if (gSystemConfiguration.useGyro2) { Gyro2StartReading();}
        else                               { GyroStartReading();}

        DelayMs(msApart);
        DEBUG_INT("\t", TimeNow() - startTime);

        // accels and gyros should all be outputting at the
        // same rate so wait for each in turn
        if (! gSystemConfiguration.useGyro2) {
            while (!IsGyroDoneReading()) { /*spin*/;}
            GyroGetLastReading(&reading[RATE_START]);
        }
        while (!IsAccelerometerDoneReading()) { /*spin*/;}
        AccelerometerGetLastReading(&reading[ACCEL_START]);

        if (gSystemConfiguration.useGyro2) {
            // G2 doesn't have to wait, it averages while the
            // accel is converting... need to make sure we
            // have at least one
            while (!IsGyro2DoneReading()) { /*spin*/;}
            Gyro2GetLastReading(&reading[RATE_START]);
        }
        if (IsMagnetometerDoneReading()) {
            MagnetometerGetLastReading(&reading[MAG_START]);
            magStart = TRUE;
        }

        if (readInUnits) {
            for (sensor = 0; sensor < NUM_SENSORS; sensor++) {
                for (axis = 0; axis < NUM_AXIS; axis++) {
                    float tmp;
                    tmp = (float) reading[axis + (sensor*NUM_AXIS)] / (float) gain[sensor];
                    DEBUG_FLOAT("\t", tmp, 2);
                }
            }
        } else {
            for (sensor = 0; sensor < NUM_SENSORS; sensor++) {
                for (axis = 0; axis < NUM_AXIS; axis++) {
                    DEBUG_INT("\t", reading[axis + (sensor*NUM_AXIS)]);
                }
            }
        }
        DEBUG_ENDLINE();
        numReads--;
    }
}

// Read accelerometer <num reads, default 10> <ms between reads, default asap>
void CmdReadAccelerometer(uint32_t readInGs)
{
    int16_t readings[NUM_AXIS];
    uint32_t numReads = 10;
    uint32_t msApart = 2;
    uint8_t error;
    uint16_t gain = 0;
    int i;

    CmdLineGetArgUInt(&numReads);
    CmdLineGetArgUInt(&msApart);

    INFO_INT("Reading accel ", numReads);
    INFO_INT(" times, ", msApart);
    INFO_STRING(" ms apart\r\n");

    if (readInGs) {
        gain = AccelerometerGetGain();
    }

    DEBUG_STRING("\tX\tY\tZ\tsuccess\r\n");
    while (numReads) {
        AccelerometerStartReading();
        OSClrEFlag(EFLAGS_DATA_READY, EF_DATA_ACCEL_READY);
        DelayMs(msApart);
        while (!IsAccelerometerDoneReading()) { /*spin*/;}
        error = AccelerometerGetLastReading(readings);
        if (error) {
            InitAccelerometer();
            DEBUG_INT(" ERROR: ", error);
            DelayMs(msApart);
        } else {
            if (readInGs) {
                for (i = 0; i < NUM_AXIS; i++) {
                    float tmp;
                    tmp = (float) readings[i] / (float) gain;
                    DEBUG_FLOAT("\t", tmp, 2);
                }
            } else {
                for (i = 0; i < NUM_AXIS; i++) {
                    DEBUG_INT("\t", readings[i]);
                }
            }
            DEBUG_INT("\t", error);
        }
        DEBUG_ENDLINE();
        numReads--;
    }
}
// Read temperature <num reads, default 10> <ms between reads, default asap>
void CmdReadTemperature(uint32_t readInC)
{
    int16_t reading;
    float temperature;
    uint32_t numReads = 10;
    uint32_t msApart = 0;
    uint16_t gain;
    uint32_t timeout;

    CmdLineGetArgUInt(&numReads);
    CmdLineGetArgUInt(&msApart);

    INFO_INT("Reading temp ", numReads);
    INFO_INT(" times, ", msApart);
    INFO_STRING(" ms apart (each conversion is about 0.75s anyway) \r\n");

    if (!InitTemperatureSensor()) {
        /* Initialization fault */
        ERROR_STRING("Temp sensor init failed\r\n");
        return;
    }

    if (readInC) {
        gain = TemperatureGetGain();
    }

    DEBUG_STRING("\ttemp\r\n");

    while (numReads) {
        if (!TemperatureStartReading()) {
            ERROR_STRING("Temp sensor read failed\r\n");
            continue;
        }
        timeout = 300;
        while (timeout && ! IsTemperatureDoneReading()) {timeout--; DelayMs(5);}
        if (TemperatureGetLastReading(&reading)) {
            if (readInC) {
                temperature = (float) reading / (float) gain;
                DEBUG_FLOAT("\t", temperature, 2);
                DEBUG_STRING(" C\r\n");
            } else {
                DEBUG_INT("\t", reading);
                DEBUG_ENDLINE();
            }
        } else {
            ERROR_STRING("Temp sensor read failed\r\n");
        }

        DelayMs(msApart);
        numReads--;
    }
}

// Read magnetometer <num reads, default 10> <ms between reads, default asap>
void CmdReadMagnetometer(uint32_t readInGauss)
{
    int16_t readings[NUM_AXIS];
    uint32_t numReads = 10;
    uint32_t msApart = 0;
    uint16_t gain = 0;
    int i;

    CmdLineGetArgUInt(&numReads);
    CmdLineGetArgUInt(&msApart);

    INFO_INT("Reading magnetometer ", numReads);
    INFO_INT(" times, ", msApart);
    INFO_STRING(" ms apart\r\n");

    if (readInGauss) {
        gain = MagnetometerGetGain();
    }

    while (numReads) {
        MagnetometerStartReading();
        while (!IsMagnetometerDoneReading()) { /* spin */ ; }
        MagnetometerGetLastReading(readings);
        if (readInGauss) {
            double mag = 0.0;
            float tmp;
            for ( i = 0; i < NUM_AXIS; i++) {
                tmp = (float) readings[i] / (float) gain;
                mag += tmp * tmp;
                DEBUG_FLOAT("\t", tmp, 2);
            }
            tmp = sqrt(mag);
            DEBUG_FLOAT("\t ( ", tmp, 2);
            DEBUG_STRING(" )");
        } else {
            for ( i = 0; i < NUM_AXIS; i++) {
                DEBUG_INT("\t", readings[i]);
            }
        }
        DEBUG_ENDLINE();
        DelayMs(msApart);
        numReads--;
    }
}

// Read gyro <num reads, default 10> <ms between reads, default asap>
void CmdReadGyro(uint32_t readInDegPerSec)
{
    int16_t readings[NUM_AXIS];
    uint32_t numReads = 10;
    uint32_t msApart = 0;
    uint16_t gain;
    int i;

    CmdLineGetArgUInt(&numReads);
    CmdLineGetArgUInt(&msApart);

   // if going between gyro and gyro2, need to init in between
    if (gSystemConfiguration.useGyro2) {
        InitGyro();
    }

    INFO_INT("Reading gyro ", numReads);
    INFO_INT(" times, ", msApart);
    INFO_STRING(" ms apart\r\n");

    if (readInDegPerSec) {
        gain = GyroGetGain();
    }

    while (numReads) {
        OSClrEFlag(EFLAGS_DATA_READY, EF_DATA_GYRO_READY);
        GyroStartReading();
        while (!IsGyroDoneReading()) { /* spin */ ; }
        GyroGetLastReading(readings);
        if (readInDegPerSec) {
            for ( i = 0; i < NUM_AXIS; i++ ) {
                float tmp;
                tmp = (float) (readings[i])/ (float) gain;
                DEBUG_FLOAT("\t", tmp, 2);
            }
        } else {
            for ( i = 0; i < NUM_AXIS; i++ ) {
                DEBUG_INT("\t", readings[i]);
            }
        }
        DEBUG_ENDLINE();
        DelayMs(msApart);
        numReads--;
    }
}

void CmdReadGyro2(uint32_t readInDegPerSec)
{
    int16_t readings[NUM_AXIS];
    uint32_t numReads = 10;
    uint32_t msApart = 2; // this can read at 8kHz, don't trash the serial output
    uint16_t gain;
    int i;
    tTime startTime;

    CmdLineGetArgUInt(&numReads);
    CmdLineGetArgUInt(&msApart);

   // if going between gyro and gyro2, need to init in between
    if (! gSystemConfiguration.useGyro2) {
        InitGyro2();
    }

    INFO_INT("Reading INV gyro ", numReads);
    INFO_INT(" times, ", msApart);
    INFO_STRING(" ms apart\r\n");

    if (readInDegPerSec) {
        gain = Gyro2GetGain();
    }
    INFO_STRING("\ttime\tX\tY\tZ\r\n");
    startTime = TimeNow();
    while (numReads) {
        Gyro2StartReading();
        DelayMs(msApart);
        while (!IsGyro2DoneReading()) { /* spin */ ; }
        Gyro2GetLastReading(readings);
        DEBUG_INT("\t", TimeNow() - startTime);
        if (readInDegPerSec) {
            for (i = 0; i < NUM_AXIS; i++ ) {
                float tmp;
                tmp = (float) (readings[i] *10)/ (float) gain;
                DEBUG_FLOAT("\t", tmp, 2);
            }
        } else {
            for (i = 0; i < NUM_AXIS; i++ ) {
             DEBUG_INT("\t", readings[i]);
            }
        }
        DEBUG_ENDLINE();
        numReads--;
    }
}

// Read temperature <num reads, default 10> <ms between reads, default asap>
void CmdReadGyroTemp(uint32_t readInC)
{
    int16_t reading;
    float temperature;
    uint32_t numReads = 10;
    uint32_t msApart = 0;

    CmdLineGetArgUInt(&numReads);
    CmdLineGetArgUInt(&msApart);

    INFO_INT("Reading gryo temp ", numReads);
    INFO_INT(" times, ", msApart);
    INFO_STRING(" ms apart\r\n");

    DEBUG_STRING("\ttemp\r\n");

    while (numReads) {
        OSClrEFlag(EFLAGS_DATA_READY, EF_DATA_GYRO_READY);
        GyroStartReading();
        while (!IsGyroDoneReading()) { /* spin */ ; }
        GyroGetLastReading(NULL);
        if (! GyroTempGetLastReading(&reading)) {
            if (readInC) {
                GyroTempGetTemperature(reading, &temperature);
                DEBUG_FLOAT("\t", temperature, 2);
                DEBUG_STRING(" C\r\n");
            } else {
                DEBUG_INT("\t", reading);
                DEBUG_ENDLINE();
            }
        } else {
            ERROR_STRING("Temp sensor read failed\r\n");
        }

        DelayMs(msApart);
        numReads--;
    }
}

// Read temperature <num reads, default 10> <ms between reads, default asap>
void CmdReadGyro2Temp(uint32_t readInC)
{
    int16_t reading;
    float temperature;
    uint32_t numReads = 10;
    uint32_t msApart = 0;


    CmdLineGetArgUInt(&numReads);
    CmdLineGetArgUInt(&msApart);

    INFO_INT("Reading gryo2 temp ", numReads);
    INFO_INT(" times, ", msApart);
    INFO_STRING(" ms apart\r\n");

    DEBUG_STRING("\ttemp\r\n");
    while (numReads) {
        if (! Gyro2TempGetLastReading(&reading)) {
            if (readInC) {
                Gyro2TempGetTemperature(reading, &temperature);
                DEBUG_FLOAT("\t", temperature, 2);
                DEBUG_STRING(" C\r\n");
            } else {
                DEBUG_INT("\t", reading);
                DEBUG_ENDLINE();
            }
        } else {
            ERROR_STRING("Temp sensor read failed\r\n");
        }

        DelayMs(msApart);
        numReads--;
    }
}

//
// Read the accelerometer, rate, mags, and temperature sensors and display the information to the serial
//   terminal.  Arguments to the function dictate whether the function displays floating-point or
//   integer values, the number of data points, and the time between samples.
//
void CmdInertialCalib(uint32_t intputParameters)
{
    // Read in data from accelerometer (3 axes), rate sensor (3 axes),  
    //    gyro temperature, and temperature sensor
    //   (1 measurement) - 8 total readings.
    int16_t reading[NUM_SENSOR_READINGS] = {0};
    float gyroTemp;
    float temperature;
    uint16_t magStart = TRUE;
    uint32_t numReads;
    uint32_t msApart;
    uint16_t dontOutputMags;

    // Only two sensors (accelerometer and rate) have gains associated with them.
    uint16_t gain[NUM_GAIN_ENTRIES];

    tTime startTime;

    // Counting variables
    int axis, sensor;

    // "readInUnits" controls the format of the terminal output.  When set to one, floating point
    //   values are displayed on the serial terminal, if zero then raw counts are displayed.
    int readInUnits = intputParameters & 0xF;

    if (intputParameters & 0xF0)  // tcal is a long, slow process
    {        
        // Default values of reads (1 Hz for 8 hours = 28,800 samples)
        numReads = 28800;
        msApart = 1000;
        dontOutputMags = 0;
        
    } else { // ical is shorter, faster
        // Default values of reads (400 Hz for 10 seconds = 4000 samples)
        numReads = 4000;
        msApart = 0;
        dontOutputMags = 1; // goes too fast to be able to output everything 
    }
    
    // Parse the function arguments into appropriate variables (if there are more arguments than the
    //   "readInUnits" arguments).
    CmdLineGetArgUInt(&numReads);
    CmdLineGetArgUInt(&msApart);

    // Display sensor commands on the serial terminal
    INFO_INT("Reading accelerometer and rate sensors (with temperature) ", numReads);
    INFO_INT(" times, ", msApart);
    INFO_STRING(" ms apart\r\n");

    // If "readInUnits" is one, display floating-point representations of the sensor data.  This
    //   logic sets the scale factors of the sensors.
    if (readInUnits)
    {
        gain[ACCEL_GAIN] = AccelerometerGetGain();
        gain[MAG_GAIN] = MagnetometerGetGain();

        // Set gyro gain based on the sensor used
        if ( gSystemConfiguration.useGyro2 ) {
            gain[GYRO_GAIN] = Gyro2GetGain();
            if (msApart == 0) {
                ERROR_STRING("You have to use a delay when using Gyro2\r\n");
            }
        } else {
            gain[GYRO_GAIN] = GyroGetGain();
        }
        
        gain[TEMP_GAIN] = TemperatureGetGain();
    }

    // Generate a header (accels, rates, mag, & temp)
    DEBUG_STRING("\tt(ms)");
    DEBUG_STRING("\tAX\tAY\tAZ");
    DEBUG_STRING("\tGX\tGY\tGZ");
    if (dontOutputMags == 0) {
        DEBUG_STRING("\tMX\tMY\tMZ");
    }
    DEBUG_STRING("\tTG\tT");
    DEBUG_ENDLINE();

    // Begin sensor reads
    startTime = TimeNow();
    TemperatureStartReading();

    // Loop until the counter, "numReads", decrements to zero
    while (numReads)
    {
        // RTOS command - don't know what this does....
        OSClrEFlag(EFLAGS_DATA_READY, EF_DATA_GYRO_ACCEL_READY);

        // Start the sensor reads...
        AccelerometerStartReading();
        
        if (magStart) {
            OSClrEFlag(EFLAGS_DATA_READY, EF_DATA_MAG_READY);
            MagnetometerStartReading();
            magStart = FALSE;
        }


        // Read in the correct rate sensor data based on the system configuration
        if (gSystemConfiguration.useGyro2) {
            Gyro2StartReading();
        } else {
            GyroStartReading();
        }

        // Wait for the appropriate number of msec before continuing and display the time
        DelayMs(msApart);
        DEBUG_INT("\t", TimeNow() - startTime);

        // accels and gyros should all be outputting at the
        // same rate so wait for each in turn
        if( !gSystemConfiguration.useGyro2 )
        {
            while (!IsGyroDoneReading())
            {
                /*spin*/;
            }
            GyroGetLastReading( &reading[RATE_START] );
            if( GyroTempGetLastReading( &reading[GYRO_TEMP] ) )
            {
                ERROR_STRING( "Temp sensor read failed\r\n" );
            }
        }
        while (!IsAccelerometerDoneReading())
        {
            /*spin*/;
        }
        AccelerometerGetLastReading(&reading[ACCEL_START]);

        if (gSystemConfiguration.useGyro2)
        {
            // G2 doesn't have to wait, it averages while the
            // accel is converting... need to make sure we
            // have at least one
            while (!IsGyro2DoneReading()) { /*spin*/;}
            Gyro2GetLastReading(&reading[RATE_START]);
            if( Gyro2TempGetLastReading( &reading[GYRO_TEMP] ) )
            {
                ERROR_STRING( "Temp sensor read failed\r\n" );
            }
        }

        if (IsMagnetometerDoneReading()) {
            MagnetometerGetLastReading(&reading[MAG_START]);
            magStart = TRUE;
        }
        if (IsTemperatureDoneReading()) {
            TemperatureGetLastReading(&reading[TEMP_SENSOR]);
            TemperatureStartReading();
        } // else, don't worry about it
        
        // 'readInUnits' controls the format of the output (float or integer) - set by the argument
        //   to this function.
        if (readInUnits)
        {
            for (sensor = 0; sensor < NUM_SENSORS-dontOutputMags; sensor++) {
                for (axis = 0; axis < NUM_AXIS; axis++)
                {
                    float tmp;
                    // Convert to a float and display results
                    tmp = (float) reading[axis + (sensor*NUM_AXIS)] / (float) gain[sensor];
                    DEBUG_FLOAT("\t", tmp, 2);
                }
            }            

            // Display the temperature (floating-point values)
            if( !gSystemConfiguration.useGyro2 )
            {
                GyroTempGetTemperature( reading[GYRO_TEMP], &gyroTemp );
            }
            else
            {
                Gyro2TempGetTemperature( reading[GYRO_TEMP], &gyroTemp );
            }
                       
            DEBUG_FLOAT("\t", gyroTemp, 2);
            
            temperature = reading[TEMP_SENSOR];
            temperature = temperature/ (float) gain[TEMP_GAIN];
            DEBUG_FLOAT("\t", temperature, 2);                        
        }
        else
        {
            // Loop through all the sensors
            for (sensor = 0; sensor < NUM_SENSORS-dontOutputMags; sensor++)
            {
                for (axis = 0; axis < NUM_AXIS; axis++)
                {
                    // Display raw counts
                    DEBUG_INT("\t", reading[axis + (sensor*NUM_AXIS)]);
                }
            }

            // Display raw counts
            DEBUG_INT("\t", reading[GYRO_TEMP]);
            DEBUG_INT("\t", reading[TEMP_SENSOR]);
        }
        DEBUG_ENDLINE();

        // Decrement the counter
        numReads--;
    }
}



// Run self test, verify existence of each sensor.
void CmdSelfTest(uint32_t data)
{
    uint32_t whoami;
    uint32_t timeout = 300;

    DEBUG_STRING("Checking temperature sensor:\t");
    if (InitTemperatureSensor()) {
        int16_t reading;
        float temperature;
        uint16_t gain = TemperatureGetGain();
        TemperatureStartReading();
        while (timeout && ! IsTemperatureDoneReading()) {timeout--; DelayMs(5);}
        TemperatureGetLastReading(&reading);
        temperature = (float) reading / (float) gain;
        DEBUG_FLOAT("SUCCESS (", temperature, 2);
        DEBUG_STRING(" C)\r\n");
    } else {
        DEBUG_STRING("FAIL - check I2C open \r\n");
    }

    DEBUG_STRING("Checking magnetometer:\t");
    if (MagnetometerWhoAmI(&whoami)) {
        if (MagnetometerSelfTest()) {
            DEBUG_HEX("SUCCESS (", whoami);
            DEBUG_STRING(")\r\n");
        } else {
            DEBUG_HEX("FAIL (", whoami);
            DEBUG_STRING(") - mag self test failed\r\n");
        }
    } else {
        DEBUG_HEX("FAIL (", whoami);
        DEBUG_STRING(") - whoami failed, check mag inited and I2C open \r\n");
    }

    DEBUG_STRING("Checking accelerometer:\t");
    if (AccelerometerWhoAmI(&whoami)) {
        DEBUG_HEX("SUCCESS (", whoami);
        DEBUG_STRING(")\r\n");
    } else {
        DEBUG_HEX("FAIL (", whoami);
        DEBUG_STRING(") - check accel inited and I2C open \r\n");
    }

    DEBUG_STRING("Checking gryoscope:\t");
    if (GyroWhoAmI(&whoami)) {
        if (GyroSelfTest()) {
            DEBUG_HEX("SUCCESS (", whoami);
            DEBUG_STRING(")\r\n");
        } else {
            DEBUG_HEX("FAIL (", whoami);
            DEBUG_STRING(") - gyro self test failed \r\n");
        }
    } else {
        DEBUG_HEX("FAIL (", whoami);
        DEBUG_STRING(") - check gyro inited and SPI open \r\n");
    }
    DEBUG_STRING("Checking gryo2 (invensense):\t");
    if (Gyro2WhoAmI(&whoami)) {
        if (Gyro2SelfTest()) {
            DEBUG_HEX("SUCCESS (", whoami);
            DEBUG_STRING(")\r\n");
        } else {
            DEBUG_HEX("FAIL (", whoami);
            DEBUG_STRING(") - gyro self test failed \r\n");
        }
    } else {
        DEBUG_HEX("FAIL (", whoami);
        DEBUG_STRING(") - check gyro inited and SPI open \r\n");
    }
}

void CmdAccelInit(uint32_t data)
{
    uint32_t rangeInGs = ACCEL_RANGE_4G;
    uint32_t outputDataRate = 800; // fixed to allow for accumulation

    CmdLineGetArgUInt(&rangeInGs);

    if (InitAccelerometer()) {
        AccelerometerConfig(&rangeInGs, &outputDataRate);
        DEBUG_INT("Accel init'd ok, ", rangeInGs);
        DEBUG_STRING(" G range and (fixed) 800 Hz output\r\n");
    } else {
        DEBUG_STRING("Accel error on init\r\n");
    }
}

void CmdGyroInit(uint32_t data)
{
    uint32_t rangeInDps = DEFAULT_GYRO_RANGE;
    uint32_t outputDataRate = OUTPUT_DATA_RATE;

    CmdLineGetArgUInt(&rangeInDps);
    CmdLineGetArgUInt(&outputDataRate);

    if (InitGyro()) {
        GyroConfig(&rangeInDps, &outputDataRate);
        DEBUG_INT("Gyro init'd ok, ", rangeInDps);
        DEBUG_INT(" dps range and  ", outputDataRate);
        DEBUG_STRING(" Hz output\r\n");
    } else {
        DEBUG_STRING("Gyro error on init\r\n");
    }
    gSystemConfiguration.useGyro2 = FALSE;
}
void CmdGyro2Init(uint32_t data)
{
    uint32_t rangeInDps = DEFAULT_GYRO_RANGE;

    CmdLineGetArgUInt(&rangeInDps);

    if (InitGyro2()) {
        Gyro2Config(&rangeInDps);
        DEBUG_INT("Gyro2 init'd ok, ", rangeInDps);
        DEBUG_STRING(" dps range and (fixed) 8kHz output\r\n");
    } else {
        DEBUG_STRING("Gyro error on init\r\n");
    }
    gSystemConfiguration.useGyro2 = TRUE;
}

void CmdMagnetometerInit(uint32_t data)
{
    uint32_t rangeInMilliGauss = 4000;

    CmdLineGetArgUInt(&rangeInMilliGauss);

    if (InitMagnetometer()) {
        MagnetometerConfig(&rangeInMilliGauss);
        DEBUG_INT("Mag init'd ok, ", rangeInMilliGauss);
        DEBUG_STRING(" mGa\r\n");
    } else {
        DEBUG_STRING("Mag error on init\r\n");
    }
}
void CmdAutoDataAquisitionMode(uint32_t data)
{
    uint32_t on = TRUE;
    CmdLineGetArgUInt(&on);

    if (on) {
        DataAquisitionStart();
    } else {
        DataAquisitionStop();
    }
}
#include "port_def.h"
#include "comm_buffers.h"
#include "uart.h"
extern port_struct     gPort0, gPort1, gPort2, gPort3; /* reference to physical port structures */

void CmdUserUsart(uint32_t data)
{
    uint32_t uart = 0;
    unsigned char *buffer;
    port_struct *port;
    int ok;

    CmdLineGetArgUInt(&uart);
    CmdLineGetArgString(&buffer);

    if (uart == 0) {port = &gPort0;}
    else if (uart == 1) {port = &gPort1;}
    else {ERROR_STRING("Unknown port\r\n"); return;}

    ok = COM_buf_in(&(port->xmit_buf), buffer, strlen((const char*)buffer));
    uart_write(uart, port);
    DEBUG_STRING("Wrote (");
    DEBUG_STRING((const char*)buffer);
    DEBUG_INT(") to ", uart);
    DEBUG_INT(", ok= ", ok);
    DEBUG_ENDLINE();
}

void CmdGpioPin(uint32_t data)
{
    uint32_t pin = 1;
    uint32_t state = 1;
    uint8_t *port;
    GPIO_TypeDef* GPIO;
    GPIO_InitTypeDef GPIO_InitStructure;

    CmdLineGetArgString(&port);
    CmdLineGetArgUInt(&pin);
    CmdLineGetArgUInt(&state);

    switch(port[0]) {
    case 'A': GPIO = GPIOA;   break;
    case 'B': GPIO = GPIOB;   break;
    case 'C': GPIO = GPIOC;   break;
    case 'D': GPIO = GPIOD;   break;
    case 'F': GPIO = GPIOF;   break;
    default: ERROR_STRING("Unknown port\r\n"); return;
    }

    pin = 1 << pin;

    GPIO_InitStructure.GPIO_Pin = pin;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIO, &GPIO_InitStructure);

    if (state) {
        GPIO_SetBits(GPIO, pin);
    } else {
        GPIO_ResetBits(GPIO, pin);
    }
}
