/**
 * @section DESCRIPTION
 *
 * header file for all other files to include, basic information about
 * system configuration
 * associated C file only.
 */  

#ifndef DMU_H
#define DMU_H

// QMATH defines, look in MATH_TYPE == FLOAT_MATH of IQMathCpp.h file 
// to get floating point equivalents
typedef double iq27;
typedef double iq30;
typedef double iq23;
typedef double iq29;

typedef int bool;
typedef int BOOL;


#define   IQ27rsmpy(A,B)     (A * B)
#define IQ27(x) (x)


typedef uint8_t tBoolean;

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif

#ifndef NULL
#define NULL 0
#endif

#define NUM_AXIS 3

//raw sensor order
enum eSensorOrder { 
      ACCEL_START=0,
      RATE_START=3, 
      MAG_START=6,  
      NUM_SENSOR_IN_AXIS = 9, 
      TEMP_START = 9,
      GYRO_TEMP = 9,
      TEMP_SENSOR = 10,
      NUM_SENSOR_READINGS = 11
    } ;

#define NUM_SENSORS 3

enum eGainOrder {
     ACCEL_GAIN = 0,
     GYRO_GAIN = 1,
     MAG_GAIN = 2,
     TEMP_GAIN = 3,
     NUM_GAIN_ENTRIES = 4
} ;


#define MAXUINT32 4294967295 	/* max unsigned 32 bit int=> ((2^32)-1) */


#define MAXUINT16 65535			/* max unsigned 16 bit int=> ((2^16)-1) */

#define MAXINT16 32767	  /* max signed 16 bit int=> ((2^15)) */              
           

#define D2R        (PI/180.0)
#define DEG2RAD(d) ( (d) * D2R ) 

#define SIGMA      1.0e-8


#define KNOT2MPSEC		5.144444444e-1

#define SQUARE(x) ((x)*(x))


#define OUTPUT_DATA_RATE 400 // ASAP

typedef struct {
    uint32_t outputDataRate;
    
    uint32_t accelRange; 
    uint32_t gyroRange; 
    uint32_t magRange; 
    
    int useGyro2 : 1;
    int outputTime : 1;
    int outputInUnits : 1;
    int outputAutoStart : 1;
    
} tSystemConfiguration;
extern tSystemConfiguration gSystemConfiguration;
extern void  DataAquisitionStart(void);
extern void DataAquisitionStop(void);

#define kick_dog() // to nothing. Seriously, who kicks a dog anymore? You have to pet watchdogs. And only in the main loop.

#endif /* DMU_H */