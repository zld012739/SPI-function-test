#include <stdint.h>
#include "dmu.h"
#include "xbowsp_generaldrivers.h"
#include "xbowsp_algorithm.h"
#include "calc_airData.h"
#include "sensor.h"

uint16_t adahrsBitRawSensors[N_ADAHRS_BITSIGNAL];
uint32_t rawInertialSensors[N_RAW_SENS];

int magCalTerminateRequested = FALSE;

unsigned int rawAirDataSensors[SIZE_AIRDATA_SENSOR]; // this was defined in sensor.c
volatile unsigned char ioupDataStart=0; // this was defined in sensor.c
unsigned char gIOUPSync = NOSYNC; // this was defined in sensor.c

AlgorithmAirDataStruct algorithmAirData; // this was defined in calc_AirData.c
uint16_t lastLUTRstIndexC[N_TABLES_C + 1]; // this was defined in calc_AirData.c
uint16_t lastLUTRstIndexB[N_TABLES_B + 1];
uint16_t lastLUTRstIndexA[N_TABLES_A + 1];


void softwareReset(void) {} // this was in system.c


void initRPYAlignOffsetMat(void) {} // this was in sensor_cal.c
