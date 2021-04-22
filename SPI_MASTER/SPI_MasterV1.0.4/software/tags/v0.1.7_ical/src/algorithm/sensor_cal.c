/*****************************************************************************
* File name:  sensor_cal.c
*
* File description: 
*   -calibrate inertial sensors and magnetometers 
*
* $Rev: 17465 $
* $Date: 2011-02-09 20:20:16 -0800 (Wed, 09 Feb 2011) $
* $Author: by-denglish $
*
***********************************************************************************/
#include <stdint.h>
#include <string.h>
#include <math.h>


#include <stdint.h>
#include <string.h>
#include <math.h>
#include "cast.h"
#include "scaling.h"
#include "matrix.h"
// #include "calc_est.h"
#include "sensor.h"
// #include "calc_sol.h" 
// #include "comm.h"
// #include "flash_memory.h"
#include "dmu.h"

#include "xbowsp_BITStatus.h"
#include "mag.h"			
// #include "flightdata.h"		
#include "xbowsp_algorithm.h" 
#include "xbowsp_version.h"
#include "xbowsp_generaldrivers.h"
// #include "calc_digitalFilter.h"
 #include "sensor_cal.h"
#include "xbowsp_init.h"
// #include "calc_airData.h"
// #include "sensor_cal.h" 
// #include "xbowsp_IQmath.h"
// #include "airdata.h"

/* temp comp lookup index */
uint16_t lastLUTRstIndexA[N_TABLES_A + 1];


/*the mapping of temperature sensor scale factor to temp sensors for each architecture used in converting temperatures to deg C */
/* AHC525 temperature scale values */
static const double AHC525_TEMP_SCALE[6] = {4.387861700215672e-007,		/* X-accel */ 
											4.387861700215672e-007,		/* Y-accel */ 
											4.387861700215672e-007, 	/* Z-accel */
											4.387861700215672e-007, 	/* X-rate */
											4.387861700215672e-007,     /* Y-rate */
											4.387861700215672e-007};    /* Z-rate */

/* AHC525 temperature bias values */ 
static const long AHC525_TEMP_BIAS[6] = {615096320,		/* X-accel */ 
										 615096320,     /* Y-accel */
										 615096320,     /* Z-accel */
										 615096320,     /* X-rate */
										 615096320,     /* Y-rate */
										 615096320};    /* Z-rate */


/* memory allocation */
static double gLevelMatM[3][3]; /* memory allocation */

double CB2userBM[3][3];
matrixStruct CB2userB = { 3, 3, NULL };

/* local functions */
extern void gSensComp(void);


/***************************************************************************** 
* Function name:  calibrate
*
* Description:  perform gyro and accel calibration.
*               Place calibrated sensor data into the appropriate vectors in the 
*               sensor structure.
*               Perform sensor data filtering and algorithm data filtering
*                                   
* Trace:
* [SDD_TOP_CAL_FLOW <-- SRC_CALIBRATE]
* [SDD_INERTIAL_SENSOR_CAL_01 <-- SRC_CALIBRATE]
* [SDD_ACC_FILTER <-- SRC_CALIBRATE]
* [SDD_GYRO_FILTER <-- SRC_CALIBRATE]
* [SDD_GYRO_ACCEL_MAG_UNAVAIL <-- SRC_CALIBRATE]
* [SDD_GYRO_ACCEL_VALID <-- SRC_CALIBRATE]
* [SDD_MAG_VALID <-- SRC_CALIBRATE]
* [SDD_INERTIAL_SENSOR_CAL_TRANSFER <-- SRC_CALIBRATE]
* [SDD_INERTIAL_SENSOR_CAL_TRANSFORM <-- SRC_CALIBRATE]
* [SDD_CAL_GYRO_UNFILT <-- SRC_CALIBRATE]
* [SDD_CONFIG_RPY_OFFSETS_ACCEL <-- SRC_CALIBRATE]
* [SDD_CONFIG_RPY_OFFSETS_GYRO <-- SRC_CALIBRATE]
*
* Input parameters:  sensorStruct *s - sensor structure containing all pertinent
*                    sensor data and control flags
*  
* Output parameters:  output calibrated sensor data inside sensorStruct *s 
* 
* Return value:  None
******************************************************************************/
void calibrate(sensorStruct *s)
{
    int i;
  
	calibrateSensors();   

	orientSensors(); 

	rpyAlignOffsetComp(&algorithm.scaledSensors[XACCEL]);
	rpyAlignOffsetComp(&algorithm.scaledSensors[XRATE]);

	/* Place calibrated sensor data into the appropriate vectors in the */
	/* sensor structure for the algorithm*/
	
	for (i=0; i<NUM_AXIS; i++){ 
		s->accel[i] = -algorithm.scaledSensors[XACCEL+i];
		s->gyro[i] = algorithm.scaledSensors[XRATE+i];
		s->accelNonFiltered[i] = s->accel[i];
		s->gyroNonFiltered[i] = s->gyro[i];
	}
    
    /* Perform sensor data filtering */
    
	filter_accel(s);
	filter_rate(s);

	/* Output data only when the wait until Stable Sensor Counts is achieved */
	/* By controlling s->state, the data will show that the sensors are calibrated, */
	/* or not.  If not then the rest of the attitude solution will not be run */
	
	if (s->countToStableSensor)    {
		s->countToStableSensor--;
	} else {	
		s->state |= M_ACCEL|M_GYRO;
		if ((s->algoState & M_FORCED_VG)==M_FORCED_VG) {
			 if ((s->state & M_REMOTEMAG) && s->HeadingReset ==TRUE_AND_USE_REMOTE_MAG ) { 
			     s->state |= M_MAG;	
			 }	
		} else {
			if (s->state & M_REMOTEMAG) {
				s->state |= M_MAG; 
			}
		}

	}
	
} /* end function calibrate */



/***************************************************************************** 
* Function name:  gSensComp
*
* Description:  perform g-sensitivity compensation for gyros
*                                   
* Trace:
* [SDD_GYROS_G_SEN <-- SRC_G_SENSITIVITY_COMPENSATION]
*
* Input parameters:  
*       none
*  
* Output parameters:  
*       none 
* 
* Return value:  None
******************************************************************************/
void gSensComp(void) 
{
     long tmpL[3] ;
	
	/* g-sensitivity count/g*/ 		       
	tmpL[0]=(long)(calibration.gSenForGyros[0]* algorithm.scaledSensors[XACCEL] + 
	       calibration.gSenForGyros[1]* algorithm.scaledSensors[YACCEL] +
	       calibration.gSenForGyros[2]* algorithm.scaledSensors[ZACCEL]);
	tmpL[1]=(long)(calibration.gSenForGyros[3]* algorithm.scaledSensors[XACCEL] +
	       calibration.gSenForGyros[4]* algorithm.scaledSensors[YACCEL] + 
	       calibration.gSenForGyros[5]* algorithm.scaledSensors[ZACCEL]);
	tmpL[2]=(long)(calibration.gSenForGyros[6]* algorithm.scaledSensors[XACCEL] +
	       calibration.gSenForGyros[7]* algorithm.scaledSensors[YACCEL] +
	       calibration.gSenForGyros[8]* algorithm.scaledSensors[ZACCEL]);
	algorithm.rawSensors[XRATE] = algorithm.rawSensors[XRATE] - tmpL[0];
	algorithm.rawSensors[YRATE] = algorithm.rawSensors[YRATE] - tmpL[1];
	algorithm.rawSensors[ZRATE] = algorithm.rawSensors[ZRATE] - tmpL[2];
	
	
}   /* end of void gSensComp(void) */

/******************************************************
* Function name:    calibrateSensors
*
* Description: 
*   -calculates calibrated sensor variables.
*
* Trace:
* [SDD_TEMP_SENSOR_SCALING <-- SRC_CALIBRATE_SENSORS]
* [SDD_TEMP_CAL_TABLES_01 <-- SRC_CALIBRATE_SENSORS]
* [SDD_TEMP_CAL_DATA_TYPES_01 <-- SRC_CALIBRATE_SENSORS]
* [SDD_LUT_SENSOR_TEMP_COMP_TAB_ENTRY_01 <-- SRC_CALIBRATE_SENSORS]
* [SDD_LUT_SENSOR_TEMP_COMP_LINEAR_01 <-- SRC_CALIBRATE_SENSORS]
* [SDD_TEMP_COMP_01 <-- SRC_CALIBRATE_SENSORS]
* [SDD_INERTIAL_CAL_TABLES_01 <-- SRC_CALIBRATE_SENSORS]
* [SDD_LUT_SENSOR_CAL_TAB_ENTRY_01 <-- SRC_CALIBRATE_SENSORS]
* [SDD_INERTIAL_CAL_DATA_TYPES_01 <-- SRC_CALIBRATE_SENSORS]
* [SDD_LUT_SENSOR_CAL_LINEAR_01 <-- SRC_CALIBRATE_SENSORS]
* 
* [SDD_MISALIGN_ACCEL_ORDER <-- SRC_CALIBRATE_SENSORS]
* [SDD_MISALIGN_GYRO_ORDER <-- SRC_CALIBRATE_SENSORS]
* [SDD_MISALIGN_ACCEL <-- SRC_CALIBRATE_SENSORS]
* [SDD_MISALIGN_GYRO <-- SRC_CALIBRATE_SENSORS]

* [SDD_GYROS_G_SEN <-- SRC_CALIBRATE_SENSORS]
*
* Input parameters:
*   None.
* 
* Output parameters:
* 	No output parameters
* 
* Return value: 
*   None.
*
*  Author: Darren Liccardo, Jan. 2004
*   	   Dong An, 2007, 2008
*******************************************************/
void calibrateSensors(void) 
{
     unsigned int i,j;
     double tmp, tmpX, tmpY; 
     LUTinOutStruct myTableInOut;
     unsigned int tableFirstRow;
     unsigned int tableNo;

     /*scaled tmperature output */
     for(i=0;i<6;i+=1) { 
        tmp =(INT32)algorithm.rawSensors[XATEMP+i]-AHC525_TEMP_BIAS[i];
		algorithm.scaledSensors[XATEMP+i] = tmp*AHC525_TEMP_SCALE[i];
     } 
     

     /*apply temp bias and lookup scaling to rates and accels   */
     for(i=0;i<6;i+=1) { 
		  /*temperature compensation*/
		  /*get the table and last results*/
		  tableNo=findTable(1,1,i);    /* determine the table to be used*/
		  myTableInOut.numberToBeSearched=algorithm.rawSensors[XATEMP+i]; 
		  myTableInOut.firstColumnUnsigned=TRUE; 
		  tableFirstRow= calibration.calibrationTableIndexA[tableNo];
		  myTableInOut.bIndexLastSearch = lastLUTRstIndexA[tableNo]-tableFirstRow;
		  myTableInOut.TableLength=calibration.calibrationTableIndexA[tableNo+1]
		  							-calibration.calibrationTableIndexA[tableNo]; 
		  /*get search result*/
		  search1DTable(&myTableInOut,
		  			(genericLUTStruct *)&calibration.calibrationTablesA[tableFirstRow][0]);
		  /*save the results for next time*/
		  lastLUTRstIndexA[tableNo]=myTableInOut.bIndexLastSearch+tableFirstRow;
		  /*linear interpolation*/
		  algorithm.tempCompBias[i]=fixedPtInterpolation(&myTableInOut);
	        
		  /* scaling*/
		  /*get temprature compensated count*/ 
		  myTableInOut.numberToBeSearched = algorithm.rawSensors[i] - algorithm.tempCompBias[i];
		  /*get the table and last results*/
		  tableNo=findTable(1,2,i); /* determine the table to be used*/
		  myTableInOut.firstColumnUnsigned=FALSE;
		  tableFirstRow= calibration.calibrationTableIndexA[tableNo];
		  myTableInOut.bIndexLastSearch = lastLUTRstIndexA[tableNo]-tableFirstRow;
		  myTableInOut.TableLength=calibration.calibrationTableIndexA[tableNo+1]
		  						   -calibration.calibrationTableIndexA[tableNo]; 
		  /*get search result*/
		  search1DTable(&myTableInOut,(genericLUTStruct *)&calibration.calibrationTablesA[tableFirstRow][0]);
		  /*save the results for next time*/
		  lastLUTRstIndexA[tableNo]=myTableInOut.bIndexLastSearch+tableFirstRow;
		  /*linear interpolation*/
		  algorithm.scaledSensors[i] = floatingPtInterpolation(&myTableInOut);
	
		  /* when accel cal is done*/
		  if (i==2) {
               /*misalignment for accel */
               j=0;
		       tmpX = algorithm.scaledSensors[j];
		       tmpY = algorithm.scaledSensors[j+1];
		       algorithm.scaledSensors[j]   -= calibration.misalign[2*j]*tmpY + 
		       						calibration.misalign[2*j+1]*algorithm.scaledSensors[j+2];
		       algorithm.scaledSensors[j+1] -= calibration.misalign[2*j+2]*tmpX + 
		       						calibration.misalign[2*j+3]*algorithm.scaledSensors[j+2];
		       algorithm.scaledSensors[j+2] -= calibration.misalign[2*j+4]*tmpX + 
		       						calibration.misalign[2*j+5]*tmpY;
		  	   gSensComp();
		  }
		  /*****/
     }

     /* misalignment for gyros    */
	for(i=3;i<6;i+=3) {
		tmpX = algorithm.scaledSensors[i];
		tmpY = algorithm.scaledSensors[i+1];
		algorithm.scaledSensors[i]   -= calibration.misalign[2*i]*tmpY + 
								calibration.misalign[2*i+1]*algorithm.scaledSensors[i+2];
		algorithm.scaledSensors[i+1] -= calibration.misalign[2*i+2]*tmpX + 
								calibration.misalign[2*i+3]*algorithm.scaledSensors[i+2];
		algorithm.scaledSensors[i+2] -= calibration.misalign[2*i+4]*tmpX + 
								calibration.misalign[2*i+5]*tmpY;
	}

}  /* end void calibrateSensors(void) */

/******************************************************
* Function name:    search1DTable
*
* Description: 
*   -This function looks up the right column value, given a left column value of a lookup table
*     and last search results (index). The search begins from where the last search ended. 
* 
* Trace:
* [SDD_LUT_SENSOR_TEMP_COMP_TAB_ENTRY_01 <-- SRC_SEARCH_1D_TABLE]
* [SDD_LUT_SENSOR_CAL_TAB_ENTRY_01 <-- SRC_SEARCH_1D_TABLE]
* [SDD_LUT_SENSOR_TEMP_COMP_TAB_ENTRY_02 <-- SRC_SEARCH_1D_TABLE]
* [SDD_LUT_SENSOR_CAL_TAB_ENTRY_02 <-- SRC_SEARCH_1D_TABLE]
* [SDD_LUT_SENSOR_TEMP_COMP_TAB_ENTRY_03 <-- SRC_SEARCH_1D_TABLE]
* [SDD_LUT_SENSOR_CAL_TAB_ENTRY_03 <-- SRC_SEARCH_1D_TABLE]
*
* Input parameters:
*   LUTinOutStruct *myTableInOut: pointer to input/output structure;
*   genericLUTStruct *myLUT, pointer to the lookup table;
* 
* Output parameters:
* 	LUTinOutStruct *myTableInOut: pointer to input/output structure.
* 
* Return value: 
*   None.
*
*  History:
* 	06/24/2004	DA	    All Modules		    Initial revision
 * 	7/1/04		DL							Merged into 420 code base
 * 	11/7/05		DL							updated for new temp mapping
 *	02/2008     DA                          Merged temp comp tablble and scaling table into one generic 
 *  \n                                      look-up table routine.
 *
 *******************************************************/
void search1DTable(LUTinOutStruct *myTableInOut, genericLUTStruct *myLUT) 
{
	unsigned int inputEntry;
	unsigned int bIndex, eIndex;
	unsigned int bLeft, eLeft;
	int bRight, eRight;
    int checkSearchDir1,checkSearchDir2;
    
	/*left colum number users want to look up*/
	inputEntry = myTableInOut->numberToBeSearched; 
	
	/*begining and end index, left and right column of last search result*/
	bIndex = myTableInOut->bIndexLastSearch ;
	eIndex = bIndex+1;
	bLeft  = myLUT->lookupTable[bIndex][0];
	eLeft  = myLUT->lookupTable[eIndex][0];
    
    /* determine broad direction of search (first column could be signed)*/
    if (myTableInOut->firstColumnUnsigned)     {
		checkSearchDir1 = inputEntry - bLeft;  
		checkSearchDir2 = inputEntry - eLeft;   
	}        
    else    {
    	checkSearchDir1 = (int)inputEntry - (int)bLeft;
    	checkSearchDir2 = (int)inputEntry - (int)eLeft;
    }
    
    /*searching either down or up direction */       
	if (checkSearchDir1<0)  {
		do {
			if (bIndex==0) break;
			bIndex--;
			bLeft = myLUT->lookupTable[bIndex][0];

		    if (myTableInOut->firstColumnUnsigned)  {
		            checkSearchDir2 = inputEntry - bLeft;
		    } else    {
		    	checkSearchDir2 = (int)inputEntry - (int)bLeft;	
		    }		
			if (checkSearchDir2>=0) {
				break;
			}
		} while (1);

		myTableInOut->bIndexLastSearch=bIndex;  /*put it back for next time*/
		eIndex = bIndex+1;
		eLeft  = myLUT->lookupTable[eIndex][0];
	
	} else if (checkSearchDir2>0)  {
		do {
			if (eIndex==(myTableInOut->TableLength-1)) break;
			eIndex++;
			eLeft = myLUT->lookupTable[eIndex][0];

		    if (myTableInOut->firstColumnUnsigned) {
		            checkSearchDir2 = inputEntry - eLeft;
		    } else {    
		    	checkSearchDir2 = (int)inputEntry - (int)eLeft;  
		    }
			if (checkSearchDir2<=0) {
				break;  
			} 
			
		} while (1);

		bIndex = eIndex-1;
		myTableInOut->bIndexLastSearch=bIndex; /*put it back for next time*/
		bLeft = myLUT->lookupTable[bIndex][0];
	}
    /* end of search  */
    
    /*get right column values of search results */
	bRight = myLUT->lookupTable[bIndex][1];
	eRight = myLUT->lookupTable[eIndex][1];
    
    /*put the search result into the structure*/
	myTableInOut->bLeftValue  = bLeft;
	myTableInOut->eLeftValue  = eLeft;
	myTableInOut->bRightValue = bRight;
	myTableInOut->eRightValue = eRight;
	
} /* end void search1DTable ()  */ 



/******************************************************
* Function name:    findTable
*
* Description: 
*   -find the location of the specific cal table in the groups of cal tables for a given sensor 
*	 to be calibrated.
*
* Trace:
* [SDD_INERTIAL_CAL_TABLES_01 <-- SRC_FIND_TABLE]
* [SDD_INERTIAL_CAL_TABLES_02 <-- SRC_FIND_TABLE]
* [SDD_INERTIAL_CAL_TABLES_03 <-- SRC_FIND_TABLE]
* [SDD_TEMP_CAL_TABLES_01 <-- SRC_FIND_TABLE]
* [SDD_TEMP_CAL_TABLES_02 <-- SRC_FIND_TABLE]
* [SDD_TEMP_CAL_TABLES_03 <-- SRC_FIND_TABLE]
*
* Input parameters:
*   unsigned int tableGroup: the cal table group that the cal table belongs to.
*   unsigned int sensorType: temperature or temperature compensated sensor data.
*   unsigned int sensorInde: sensor index.
*
* Output parameters:
* 	None.
* 
* Return value: 
*   The index of the cal table location
*
*  Author: Darren Liccardo and Dong An, 2007, 2008
*******************************************************/
unsigned int findTable(unsigned int tableGroup,unsigned int sensorType, unsigned int sensorIndex) 
{
	unsigned int tableLocationIndex;
	
 	switch (tableGroup) {
	 	case TABLE_GROUP_A_INDEX:   /* table group A  */
	 	        switch(sensorType) {
		 	        case SENSOR_TYPE_INDEX_TEMP:    /* temprature   sensor  */
		 	        	  tableLocationIndex=sensorIndex;
		 	        break;
		 	        case SENSOR_TYPE_INDEX_INERTIAL:  /* inertial */
		 	        	 tableLocationIndex=sensorIndex+6;
		 	        break;
	 	        }
	 	break;
	 	case TABLE_GROUP_B_INDEX: /* table group B */
			 	 switch(sensorType) {  
		 	        case SENSOR_TYPE_INDEX_TEMP: /* temprature sensor */ 
		 	        		tableLocationIndex=sensorIndex+3;  
		 	        break;
		 	        case SENSOR_TYPE_INDEX_PRESSURE:  /* pressure sensor  */
		 	        		tableLocationIndex=sensorIndex+5;  
		 	        break; 
		 	     }
	 	break;

	 	case TABLE_GROUP_C_INDEX: /* table group C */
			 	 switch(sensorType) {  
		 	        case SENSOR_TYPE_INDEX_TEMP: /* temprature sensor */
		 	              tableLocationIndex=sensorIndex;
		 	        break;
		 	        case SENSOR_TYPE_INDEX_OAT:  /* OAT sensor  */
		 	             tableLocationIndex=sensorIndex+1;  
		 	        break;
	 	        }	 	
	 	break;
  	}

	return(tableLocationIndex);

}  /* end unsigned int findTable(unsigned int tableGroup,unsigned int sensorType, unsigned int sensorIndex) */
  
/******************************************************
* Function name:    fixedPtInterpolation
*
* Description: 
*   -interpolate the corresponding final fixed point search result based on the input for the LUT and 
*    the search results of four-point boundaries. 
*
* Trace:
* [SDD_TEMP_CAL_DATA_TYPES_01 <-- SRC_FIXED_POINT_INTERPOLATION]
* [SDD_TEMP_CAL_DATA_TYPES_02 <-- SRC_FIXED_POINT_INTERPOLATION]
* [SDD_TEMP_CAL_DATA_TYPES_03 <-- SRC_FIXED_POINT_INTERPOLATION]
* [SDD_FIXED_POINT_INTERPOLATION <-- SRC_FIXED_POINT_INTERPOLATION]
* [SDD_INTERPOLATION_ERROR <-- SRC_FIXED_POINT_INTERPOLATION]
*
* Input parameters:
*   LUTinOutStruct *myTableInOut: input/output data structure for operating a table search
*
* Output parameters:
* 	None.
* 
* Return value: 
*   the corresponding final fixed point search result 
*
*  Author: Darren Liccardo and Dong An, 2007, 2008
*******************************************************/
int fixedPtInterpolation(LUTinOutStruct *myTableInOut)	{
	int tmp; 
	double tmpDouble1;
	int divisor;

	divisor = (int)(myTableInOut->eLeftValue-myTableInOut->bLeftValue);
	
	if (divisor == 0) {     
	   /* degenerate case (error case)*/
		tmp= (int)((myTableInOut->eRightValue + myTableInOut->bRightValue)/2);
	} else {
		/* linear interpolation  */
		/* when this function is called, the left column is always "unsigned" */
		tmp=myTableInOut->bRightValue;
		tmpDouble1=((double)(myTableInOut->eRightValue-myTableInOut->bRightValue))/divisor;
	    tmp+= (int)(((int)(myTableInOut->numberToBeSearched-myTableInOut->bLeftValue))*tmpDouble1);
    }

    return(tmp);   
    
} /* end int fixedPtInterpolation(LUTinOutStruct *myTableInOut) */
 
/******************************************************
* Function name:    floatingPtInterpolation
*
* Description: 
*   -interpolate the corresponding final floating point search result based on the input for the LUT and the search results 
*    of four-point boundaries.
*
* Trace:
* [SDD_INERTIAL_CAL_DATA_TYPES_01 <-- SRC_FLOATING_POINT_INTERPOLATION]
* [SDD_INERTIAL_CAL_DATA_TYPES_02 <-- SRC_FLOATING_POINT_INTERPOLATION]
* [SDD_INERTIAL_CAL_DATA_TYPES_03 <-- SRC_FLOATING_POINT_INTERPOLATION]
* [SDD_FLOATING_POINT_INTERPOLATION <-- SRC_FLOATING_POINT_INTERPOLATION]
* [SDD_INTERPOLATION_ERROR <-- SRC_FLOATING_POINT_INTERPOLATION]
*
* Input parameters:
*   LUTinOutStruct *myTableInOut: input/output data structure for operating a table search
*
* Output parameters:
* 	None.
* 
* Return value: 
*   the corresponding final floating point search result. 
*
*  Author: Darren Liccardo and Dong An, 2007, 2008
*******************************************************/ 
double floatingPtInterpolation(LUTinOutStruct *myTableInOut)	{
	double bRight, eRight;
	double tmp,tmp1;
	int divisor;
    
    /*  convert back to floating numbers   */
	*((INT32*)&bRight) = myTableInOut->bRightValue;
	*((INT32*)&eRight) = myTableInOut->eRightValue;  
	
	/* linear interpolation
	 * when this function is called, the leftcolumn is always "signed"  */
	tmp=bRight;

	divisor = (int)myTableInOut->eLeftValue-(int)myTableInOut->bLeftValue;

	if (divisor == 0) {   
    	/* degenerate case (error case)  */
		tmp= (eRight+ bRight)/2.0;
	} else {
		tmp1=(eRight-bRight)/divisor;
		tmp+=((int)myTableInOut->numberToBeSearched-(int)myTableInOut->bLeftValue)*tmp1;
    }

    return(tmp);
    
}  /*end double floatingPtInterpolation(LUTinOutStruct *myTableInOut)*/

/******************************************************
* Function name:    orientSensors
*
* Description: 
*   -rotates the accels, rates, mags, and temps to the user 
 *	defined orientation frame
*
* Trace:
* [SDD_CONFIG_ORIENTATION_02 <-- SRC_ORIENT_SENSORS]
* [SDD_CONFIG_ORIENTATION_03 <-- SRC_ORIENT_SENSORS]
* [SDD_CONFIG_ORIENTATION_04 <-- SRC_ORIENT_SENSORS]
*
* Input parameters:
*   None.
*
* Output parameters:
* 	None.
* 
* Return value: 
*   None.
*
*  Author: Darren Liccardo, Jan. 2004
*   	   Dong An, 2007, 2008
*******************************************************/
void orientSensors(void) { 

     int i,j;
     double tmp[3];
	 
	 /*Accel and Gyro*/
     for(i=0;i<6;i+=3) {	
		for(j=0;j<3;j+=1) {
			tmp[j] = algorithm.scaledSensors[i+j]; /* store sensor data for next interation of 
			                                       swapping */
		}
		if(configuration.orientation.bit.forwardAxisSign) {
			algorithm.scaledSensors[i] = -tmp[configuration.orientation.bit.forwardAxis];
		}		
		else {
			algorithm.scaledSensors[i] = tmp[configuration.orientation.bit.forwardAxis];
		}
		if(configuration.orientation.bit.rightAxisSign) {
			algorithm.scaledSensors[i+1] = -tmp[(configuration.orientation.bit.rightAxis+1)%3];	
		}	
		else {
			algorithm.scaledSensors[i+1] = tmp[(configuration.orientation.bit.rightAxis+1)%3]; 
		}
		if(configuration.orientation.bit.downAxisSign) {
			algorithm.scaledSensors[i+2] = -tmp[(configuration.orientation.bit.downAxis+2)%3];		
		} else {
			algorithm.scaledSensors[i+2] = tmp[(configuration.orientation.bit.downAxis+2)%3];
		}
     }
     
     /*tmperature*/
     for(i=9;i<15;i+=3) {	
		  for(j=0;j<3;j+=1) {
		       tmp[j] = algorithm.scaledSensors[i+j]; /* store sensor data for next interation of 
		                                              swapping  */
		  }
		  algorithm.scaledSensors[i] = tmp[configuration.orientation.bit.forwardAxis];		
		  algorithm.scaledSensors[i+1] = tmp[(configuration.orientation.bit.rightAxis+1)%3];		
		  algorithm.scaledSensors[i+2] = tmp[(configuration.orientation.bit.downAxis+2)%3];		
     }

}  /* end void orientSensors(void) */


/******************************************************
* Function name:    initRPYAlignOffsetMat
*
* Description: 
*   - generate a roll-pitch-yaw alignment offset compensation matrix (CB2userBM).
*
* Trace:
* [SDD_INIT_RPY_OFFSETS <-- SRC_INIT_RPY_ALIGN_OFFSET_COMP_MATRIX]
* [SDD_INIT_RPY_OFFSETS_EXTEND <-- SRC_INIT_RPY_ALIGN_OFFSET_COMP_MATRIX]
*
* Input parameters:
*   None
*
* Output parameters:
* 	None.
* 
* Return value: 
*   None.
*  
*******************************************************/  
void initRPYAlignOffsetMat(void)
{
	double rollOffset,pitchOffset,yawOffset;
	double  tmp331M[3][3],tmp332M[3][3], attitudeM[3];
	matrixStruct tmp331 = { 3, 3, NULL }, tmp332 = { 3, 3, NULL }, attitude = { 3, 1, NULL }; 

	/* casting twice neccessary in order to sign extend value out to underlying C33 data width 
	 of 32 bits for all C types */
	configuration.OffsetAnglesAlign[0] = (INT16)(Int16ToInt32(configuration.OffsetAnglesAlign[0]));

    /* casting twice neccessary in order to sign extend value out to underlying C33 data width of
     32 bits for all C types */
	configuration.OffsetAnglesAlign[1] = (INT16)(Int16ToInt32(configuration.OffsetAnglesAlign[1]));

	/* casting twice neccessary in order to sign extend value out to underlying C33 data width of 
	32 bits for all C types */
	configuration.OffsetAnglesAlign[2] = (INT16)(Int16ToInt32(configuration.OffsetAnglesAlign[2]));

    CB2userB.Ptr=&CB2userBM[0][0];
	attitude.Ptr = &attitudeM[0];
	tmp331.Ptr = &tmp331M[0][0];
	tmp332.Ptr = &tmp332M[0][0];	
	
	rollOffset=((double)configuration.OffsetAnglesAlign[0])/MAXuint16_t_OVER_2PI;
	pitchOffset=((double)configuration.OffsetAnglesAlign[1])/MAXuint16_t_OVER_2PI;
	yawOffset=((double)configuration.OffsetAnglesAlign[2])/MAXuint16_t_OVER_2PI;
	
	*(attitude.Ptr) = yawOffset;
	*(attitude.Ptr+1) = pitchOffset;
	*(attitude.Ptr+2) = rollOffset;

	mat_make_identity(tmp331);
	rotate_xyz_matrix(3,*(attitude.Ptr),tmp331,tmp332);
	rotate_xyz_matrix(2,*(attitude.Ptr+1),tmp332,tmp331);
	rotate_xyz_matrix(1,*(attitude.Ptr+2),tmp331,tmp332);
    mat_move(tmp332,&CB2userB);           

}  /*void initRPYAlignOffsetMat(void) */

/******************************************************
* Function name:    rpyAlignOffsetComp
*
* Description: 
*   -transform a vector in AHRS body frame to the user's body frame.
*
* Trace:
* [SDD_CONFIG_RPY_OFFSETS_ACCEL <-- SRC_RPY_ALIGN_OFFSET_COMP]
* [SDD_CONFIG_RPY_OFFSETS_GYRO <-- SRC_RPY_ALIGN_OFFSET_COMP]
*
* Input parameters:
*   double *vector:   the pointer to the vecotr that needs to be transformed.
*
* Output parameters:
* 	None.
* 
* Return value: 
*   None. 
*******************************************************/ 
void rpyAlignOffsetComp(double *vector)
{
	int key = 0;
	double tmpVectorM1[3], tmpVectorM2[3];
 	matrixStruct tmpVector1 = { 3, 1, NULL }, tmpVector2 = { 3, 1, NULL }; 

    CB2userB.Ptr=&CB2userBM[0][0];
 	tmpVector1.Ptr = &tmpVectorM1[0];
 	tmpVector2.Ptr = &tmpVectorM2[0];

	*(tmpVector1.Ptr) = vector[0];
	*(tmpVector1.Ptr+1) = vector[1];
	*(tmpVector1.Ptr+2) = vector[2];
	
	mat_multiply(CB2userB,tmpVector1,&tmpVector2,&key);	
	
	vector[0]=*(tmpVector2.Ptr);
	vector[1]=*(tmpVector2.Ptr+1);
	vector[2]=*(tmpVector2.Ptr+2);

} /*void rpyAlignOffsetComp(double *vector)*/

/**********************************************************************************
* Function name:  IQ27toF
*
* Description: 
*	- scales  C2000's IQ27 type into TI VC33's double type
*               
* Trace:
* [SDD_IQ27TOF <-- SRC_IQ27TOF]
*
* Input parameters: 
*       tmp: the number to be scaled.
* Output parameters: none
*
* Return value: 
*       The double type data.
* Author Dong An 
*********************************************************************************/
double IQ27toF (IQ27 tmp)	{
  	return((double)(tmp*IQ27_TO_DOUBLE));
}  /* End  double IQ27toF (iq27 tmp)  */




/**********************************************************************************
 * Function name:  init_sensor_variable
 *
 * Description:
 *  -Init sensor structure
 *  -Set UART port as RS233 or RS422 depending on flash memory configuration
 *  -Set UART baud rate
 *  -Set output data rate
 *
 * Trace:
 * [SDD_INIT_SENSOR_VAR <-- SRC_INIT_SENSOR_VARIABLE]
 * [SDD_GYRO_ACCEL_MAG_UNAVAIL <-- SRC_INIT_SENSOR_VARIABLE]
 * [SDD_SOL_RESET_INITIATE_01 <-- SRC_INIT_SENSOR_VARIABLE]
 * [SDD_INIT_SENSOR_VAR_INIT <-- SRC_INIT_SENSOR_VARIABLE]
 *
 * Input parameters:
 *   sensor:  structure to init
 *
 * Output parameters:
 *   sensor : structure initialized
 *
 * Return value: none
 *********************************************************************************/
void init_sensor_variable(sensorStruct *sensor)
{
	int i;
	
	sensor->state = 0;
	sensor->algoState = M_INIT;
	for (i=0; i<NUM_AXIS; i++) {
		sensor->accel[i] = 0.0;
		sensor->gyro[i] = 0.0;
		sensor->gyroNonFiltered[i] = 0.0;
		sensor->bodyRate[i] = 0.0;
		sensor->bodyRateUnfilt[i] = 0.0;
		sensor->bodyAccel[i] = 0.0;
		sensor->tangentAccel[i] = 0.0;
		sensor->tangentRate[i] = 0.0;	  
		sensor->accTanCalc[i] = 0.0;
		sensor->accMeasGrav[i] = 0.0;
	}
	
	sensor->roll = 0.0;
	sensor->pitch = 0.0;
	sensor->yaw = 0.0;
	sensor->filteredroll = 0.0;
	sensor->filteredpitch = 0.0;
	sensor->filteredyaw = 0.0;
	sensor->centripetalAccFilt = 0.0;
	
	sensor->LevelMat.m = 3;
	sensor->LevelMat.n = 3;
	sensor->LevelMat.Ptr = &gLevelMatM[0][0];
	mat_make_identity(sensor->LevelMat);
	
	sensor->dt = 0.01023865;
	
	for(i=0; i<MAX_COVAR; i++)  {
		sensor->covar.a[i] = 0;
	}
	sensor->covar.matNorm = 0;
	sensor->covar.size = MAX_COVAR;
	
	sensor->countToStableSensor = 50;
	sensor->gyroSaturated = FALSE;
	sensor->solutionReset = TRUE;
	sensor->HeadingReset =FALSE;
	
	/* the next 3 sub-initializations need to be done after the sensor struct is initialized */	
	airdata_init(sensor);
	remotemag_init(sensor);
	flightdata_init(sensor);
	
} /* end function init_sensor_variable */

/**********************************************************************************
* Function name: getADAHRSsolution
*
* Description: 
*	-fills ADAHRS solution into the ADAHRS protocol's algorithms structure.
*               
* Trace:
* [SDD_GET_ADAHRS_SOLUTION <-- SRC_GET_ADAHRS_SOLUTION]
*
* Input parameters: 
*   sensorStruct *sensor: points to the senosr structure.
*
* Output parameters: 	
*	None
*
* Return value: 
*   None.
* 
* AUTHOR:  Dong An, 2007,2008  
*********************************************************************************/
void getADAHRSsolution(sensorStruct *sensor)  {
  	int i;  

	algorithm.attitude[0]=sensor->filteredroll;
	algorithm.attitude[1]=sensor->filteredpitch;
	algorithm.attitude[2]=sensor->filteredyaw;
    
    for (i=0;i<3;i++) {
        algorithm.correctedRate[i]=sensor->bodyRateUnfilt[i];
        algorithm.tangentAccels[i]=-sensor->tangentAccel[i];
        algorithm.tangentRates[i]=sensor->tangentRate[i];
    } 
    
    algorithmAirData.airDataSolution[SLIPSKID]=sensor->flight->slipSkidAngle;  
	algorithm.downRate=sensor->yawRateAve;
    
} /* end void getADAHRSsolution(sensorStruct *sensor) */


