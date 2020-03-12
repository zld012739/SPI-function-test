/**	\file calibrate.c
 * 	\brief Sensor Calibration Algorithms. 
 *
 *
 */

#include <stdint.h>
#include "dmu.h"
#include "calibrate.h"
#include "configuration.h"

#include "algorithm.h"


//temp comp lookup index... first six values are for bias, second for scale factor
int16_t gCurrentTableLookupIndex[15];


/**	\fn 	uint32_t _searchTempTable(int sensor)
 *
 * 	given inertial sensor index 0:5 (accels,gyros), this function looks at the appropriate temp
 * 	sensor and searches the inertial sensor's temp comp table to find the correct
 * 	table entry.  The search begins from where the last search ended.  
 *	
 *	\param	sensor inertial sensor index 0:5 (accels,gyros)
 *	\return The interpolated temp bias is calculated and returned.
 *	
 *	
 */
static uint32_t _searchTempTable(int sensor, uint32_t tempReading ) 
{
	
	int16_t B_Index, E_Index;
	uint32_t BCount, ECount;
	int32_t BValue, EValue;
     double valOverCount;

	B_Index = gCurrentTableLookupIndex[sensor];
	E_Index = B_Index+1;
	BCount = gCalibration.calibrationTablesA[B_Index][0];
	ECount = gCalibration.calibrationTablesA[E_Index][0];

	if (tempReading < BCount)  {
		do { 
			if (B_Index == gCalibration.calibrationTableIndexA[sensor]) {break;}
			B_Index--;
			BCount = gCalibration.calibrationTablesA[B_Index][0];
			if (tempReading >= BCount) { break; }
		} while (1);

		gCurrentTableLookupIndex[sensor] = B_Index;
		E_Index = B_Index+1;
		ECount = gCalibration.calibrationTablesA[E_Index][0];
	
	} else if (tempReading > ECount)  {
		do {
			if (E_Index == (gCalibration.calibrationTableIndexA[sensor+1]-1)) { break;}
			E_Index++;
			ECount = gCalibration.calibrationTablesA[E_Index][0];
			if (tempReading <= ECount) { break; }
		} while (1);

		B_Index = E_Index-1;
		gCurrentTableLookupIndex[sensor] = B_Index;
		BCount = gCalibration.calibrationTablesA[B_Index][0];
	}

/////////////////////////////////////////

	BValue = gCalibration.calibrationTablesA[B_Index][1];
	EValue = gCalibration.calibrationTablesA[E_Index][1];
    valOverCount = ((double)(EValue-BValue)/(double)(ECount-BCount));
       
	return (uint32_t)(BValue + ((int32_t)(tempReading-BCount)) * valOverCount);

////////////////////////////////////////
}

/**	\fn 	double searchScaleTable(int sensor, int32_t sensorCounts)
 *
 * 	given sensor index 0:8 (accels,gyros,mags), and the temp compensated sensor
 * 	counts, this function searches the sensor's scale table to find the correct
 * 	table entry.  The search begins from where the last search ended.  
 *
 *	\param	sensor sensor index 0:8 (accels,gyros,mags)
 *	\param	sensorCounts temp compensated sensor counts
 *
 *	\return The interpolated engineering unit value is calculated and returned.
 *	 *	
 */
double _searchScaleTable(int sensor, int32_t sensorCounts) 
{
	int16_t B_Index, E_Index;
	int32_t BCount, ECount;
	float BValue, EValue;
    double valOverCount;

	
	B_Index = gCurrentTableLookupIndex[6+sensor];
	E_Index = B_Index+1;
	BCount = gCalibration.calibrationTablesA[B_Index][0];
	ECount = gCalibration.calibrationTablesA[E_Index][0];

	if (sensorCounts < BCount)  {
		do {
			if (B_Index == gCalibration.calibrationTableIndexA[6+sensor]) { break; }
			B_Index--;
			BCount=gCalibration.calibrationTablesA[B_Index][0];
			if (sensorCounts >= BCount) { break; }
		} while (1);

		gCurrentTableLookupIndex[6+sensor] = B_Index;
		E_Index = B_Index+1;
		ECount = gCalibration.calibrationTablesA[E_Index][0];
	
	} else if (sensorCounts > ECount)  {
		do {
			if (E_Index==(gCalibration.calibrationTableIndexA[6+sensor+1]-1)) { break; }
			E_Index++;
			ECount = gCalibration.calibrationTablesA[E_Index][0];
			if (sensorCounts<=ECount) { break; }
		} while (1);

		B_Index = E_Index - 1;
		gCurrentTableLookupIndex[6+sensor] = B_Index;
		BCount = gCalibration.calibrationTablesA[B_Index][0];
	}

/////////////////////////////////////////
  //force the bits into a double!
	*((uint32_t*)&BValue) = gCalibration.calibrationTablesA[B_Index][1];
	*((uint32_t*)&EValue) = gCalibration.calibrationTablesA[E_Index][1];
    valOverCount = ((double)(EValue-BValue)/(double)(ECount-BCount));
	return (BValue + ((double)(sensorCounts-BCount))*valOverCount);
////////////////////////////////////////
}


/**	\fn void InitCalibration(void)
 * 
 *	Initialize the calibration global variables (gCurrentTableLookupIndex)
 *
 */
void InitCalibration(void) 
{
    int i, j, calStart, calIndex;
    const int tempStep = 100; // mCel for each entry in the table
    const int sensorStep = 500;
    float scaleValueFloat;
    int scaleValueInt;

    int sizeofArray = sizeof(gCurrentTableLookupIndex)/sizeof(gCurrentTableLookupIndex[0]);
    if (gCalibration.serialNumber == 0) { // unit not calibrated, use default settings   
        for (i = 0; i < NUM_AXIS; i++) {
            // build temp tables that don't do anything
            calStart = ACCEL_CAL_TABLE_OFFSET + (i * ACCEL_CAL_TABLE_SIZE);
            calIndex = ACCEL_START + i + TEMP_BIAS_OFFSET;
            gCalibration.calibrationTableIndexA[calIndex] = calStart;
            for (j = 0; j < ACCEL_CAL_TABLE_SIZE / 2; j++) {
                calIndex = j + calStart;
                gCalibration.calibrationTablesA[calIndex][TEMP_COUNT] = tempStep * j;  
                gCalibration.calibrationTablesA[calIndex][BIAS_VALUE] = 0;      
            }
            calIndex = ACCEL_START + i + TEMP_SCALE_OFFSET;
            gCalibration.calibrationTableIndexA[calIndex] = calStart +  ACCEL_CAL_TABLE_SIZE / 2;
            for (j = j; j < ACCEL_CAL_TABLE_SIZE; j++) {
                calIndex = j + calStart;                 
                // if count == step, then bias ends up at 1.0
                scaleValueFloat =  sensorStep * j;
                scaleValueInt = *(uint32_t *)&scaleValueFloat;
                gCalibration.calibrationTablesA[calIndex][SENSOR_COUNT] = sensorStep * j;  
                gCalibration.calibrationTablesA[calIndex][SCALE_VALUE] = scaleValueInt; 
            }
            
            calStart = RATE_CAL_TABLE_OFFSET + (i * RATE_CAL_TABLE_SIZE);
            calIndex = RATE_START + i + TEMP_BIAS_OFFSET;
            gCalibration.calibrationTableIndexA[calIndex] = calStart;
            for (j = 0; j < RATE_CAL_TABLE_SIZE / 2; j++) {
                calIndex = j + calStart;
                gCalibration.calibrationTablesA[calIndex][TEMP_COUNT] = tempStep * j;  
                gCalibration.calibrationTablesA[calIndex][BIAS_VALUE] = 0; 
            }
            calIndex = RATE_START + i + TEMP_SCALE_OFFSET;
            gCalibration.calibrationTableIndexA[calIndex] = calStart +  RATE_CAL_TABLE_SIZE / 2;
            for (j = j; j < RATE_CAL_TABLE_SIZE; j++) {
                calIndex = j + calStart;
                scaleValueFloat =  sensorStep * j;
                scaleValueInt = *(uint32_t *)&scaleValueFloat;
                gCalibration.calibrationTablesA[calIndex][SENSOR_COUNT] = sensorStep * j;  
                gCalibration.calibrationTablesA[calIndex][SCALE_VALUE] = scaleValueInt; 
            }
            
            calStart = MAG_CAL_TABLE_OFFSET + (i * MAG_CAL_TABLE_SIZE);
            calIndex = MAG_START + i + TEMP_SCALE_OFFSET; // no temp bias for mag
            gCalibration.calibrationTableIndexA[calIndex] = calStart;
            for (j = 0; j < MAG_CAL_TABLE_SIZE ; j++) {
                calIndex = j + calStart;
                scaleValueFloat =  sensorStep * j;
                scaleValueInt = *(uint32_t *)&scaleValueFloat;
                gCalibration.calibrationTablesA[calIndex][SENSOR_COUNT] = sensorStep * j;  
                gCalibration.calibrationTablesA[calIndex][SCALE_VALUE] = scaleValueInt; 
            }
            
        }
 
        // misalignment table
        for ( i = 0; i < NUM_AXIS*2; i++ ) {
           gCalibration.misalign[i] = 0; // no off axis influences
        }

    } // end if no serial numer so fake a calibration table    
        
    
    for (i = 0; i < sizeofArray; i++) {
        gCurrentTableLookupIndex[i] = gCalibration.calibrationTableIndexA[i];
    }
}

/**	\fn void calibrateSensors(int16_t *rawReadings, uint16_t *gain, iq27 *scaledSensors)
 * 
 *	calibrateSensors calculates calibrated sensor variables.
 *	The input is the raw readings (accels, gyros, mags, temp as described in 
 *  eSensorOrder. The gain for each sensor is also passed in.
 *
 *  The output is in scaledSensors, the results are in g's, rad/s, G, deg C, (rotated to user body frame)
 */
void CalibrateSensors(int16_t *rawReadings, uint16_t *gain, iq27 *scaledSensors) 
{
    int i, si, sensor;
    uint32_t tempCompBias[6];     //the current temp comp inertial sensor bias
    uint16_t gainValue;
    
	uint32_t temperature;
    temperature = rawReadings[TEMP_START] * 1000;
    temperature /= gain[TEMP_GAIN]; // degrees mC
    
	// apply temperature correction bias and scale factor to rates and accels
    for (sensor = 0; sensor < 2; sensor++) { // accel and gyro
        gainValue = gain[sensor]; 
        for( i=0; i < NUM_AXIS; i++ ) {
            int16_t rawReadingMinusBias;
            si = (sensor * NUM_AXIS) + i;
            // FIXME: this doesn't take into account the gain which may vary 
            // depending on sensor configuration, need to apply this before
            // bias or scale factor
            tempCompBias[si] = _searchTempTable(si, temperature); // in raw sensor counts
            rawReadingMinusBias = rawReadings[si] - tempCompBias[si];        
            scaledSensors[si] = IQ27(_searchScaleTable(i, rawReadingMinusBias)); 
            scaledSensors[si]  /= (float) gainValue;
        }
    }    
	
	//scale magnetometers (no temp comp)
    gainValue = gain[MAG_GAIN];
    for( i=0; i < NUM_AXIS; i++ ) {
        si = MAG_START + i;
        scaledSensors[si] = IQ27(_searchScaleTable(si,  rawReadings[si])); 
        scaledSensors[si]  /= (float) gainValue;
    }    



    //misalignment
    for(i=0; i<NUM_SENSOR_IN_AXIS; i+=NUM_AXIS) {
        iq27 tempX, tempY, tempZ;
        iq27 *misalign = &(gCalibration.misalign[2*i]);
        tempX = scaledSensors[i];
        tempY = scaledSensors[i+1];
        tempZ = scaledSensors[i+2];
        scaledSensors[i]   -= (IQ27rsmpy(misalign[0], tempY) + IQ27rsmpy(misalign[1], tempZ));
        scaledSensors[i+1] -= (IQ27rsmpy(misalign[2], tempX) + IQ27rsmpy(misalign[3], tempZ));
        scaledSensors[i+2] -= (IQ27rsmpy(misalign[4], tempX) + IQ27rsmpy(misalign[5], tempY));
    }

#if 0    // FIXME: still to do
	orientSensors();
#endif


}

/**	\fn		void orientSensors(void)
 *
 *	orientSensors rotates the accels, rates, mags, and temps to the user
 *	defined orientation frame
 *
 *	\author Darren Liccardo, Aug. 2005
 */
void orientSensors(void)
{
   int i,j;
   iq27 temp[3];

   for( i=0;i<15;i+=3 )
   {
      for(j=0;j<3;j+=1)
      {
         // Store sensor data for next interation of swapping
         temp[j] = gAlgorithm.scaledSensors[i+j];
      }

      // Adjust the sign of the sensor data if the configuration bit is set
      if( gConfiguration.orientation.bit.forwardAxisSign )
      {
         gAlgorithm.scaledSensors[i] = -temp[gConfiguration.orientation.bit.forwardAxis];
      }
      else
      {
         gAlgorithm.scaledSensors[i] = temp[gConfiguration.orientation.bit.forwardAxis];
      }

      //
      if(gConfiguration.orientation.bit.rightAxisSign)
      {
         gAlgorithm.scaledSensors[i+1] = -temp[(gConfiguration.orientation.bit.rightAxis+1)%3];
      }
      else
      {
         gAlgorithm.scaledSensors[i+1] = temp[(gConfiguration.orientation.bit.rightAxis+1)%3];
      }

      //
      if(gConfiguration.orientation.bit.downAxisSign)
      {
         gAlgorithm.scaledSensors[i+2] = -temp[(gConfiguration.orientation.bit.downAxis+2)%3];
      }
      else
      {
         gAlgorithm.scaledSensors[i+2] = temp[(gConfiguration.orientation.bit.downAxis+2)%3];
      }
   }
}