/*****************************************************************************
* File name:  sensor_cal1.c
*
* File description: 
*   -functions for magnetometer alignment 
*
* $Rev: 17364 $
* $Date: 2011-02-02 15:23:33 -0800 (Wed, 02 Feb 2011) $
* $Author: by-denglish $
*
***********************************************************************************/
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include "cast.h"
#include "scaling.h"
#include "matrix.h"
#include "sensor.h"
#include "dmu.h"

#include "xbowsp_BITStatus.h"
#include "mag.h"			
#include "xbowsp_algorithm.h" 
#include "xbowsp_version.h"
#include "xbowsp_generaldrivers.h"
 #include "sensor_cal.h"
#include "xbowsp_init.h"

/* mag cal */
#define TOTAL_POINT  380
int16_t magRollOffset,magPitchOffset;
int16_t hardIronBias[2];
uint16_t softIronScaleRatio;
int16_t softIronAngle; 
double sMagx1[TOTAL_POINT],sMagy1[TOTAL_POINT];
double sfMat[4];



/***************************************************************************** 
* Function name:  remotemag_preProcessing
*
* Description:  apply roll-pitch-yaw offset compensation and 
*               perform filtering for mag data only
*                                   
* Trace:
* [SDD_REMOTE_MAG_PRE_PROCESSING_01 <-- SRC_REMOTE_MAG_PRE_PROCESSING]
*
* Input parameters:  
*                   None
*
* Output parameters:  
*                   None
* 
* Return value:  None
******************************************************************************/ 
void remotemag_preProcessing(void)
{
	rpyAlignOffsetComp(&gAlgorithm.ExtAccels[0]);
	rpyAlignOffsetComp(&gAlgorithm.ExtMags[0]);
	filterRemotemag(&gAlgorithm.ExtMags[0], &resetMagFilter);     
 
}  


/***************************************************************************** 
* Function name:  calcExtMagInstallAngles
*
* Description:  calculate the incidence angles between
*          an external mag and AHC525, so that AHC525 can resolve 
*          the measured body-axis external mags into a local level frame
*          in order to calculate the corresponding mag. heading.
*                                   
* Trace:
* [SDD_CALC_LEVEL_INIT <-- SRC_CALSC_EXT_INSTALL_ANGLES]
* [SDD_CALC_LEVEL_AVG <-- SRC_CALSC_EXT_INSTALL_ANGLES]
* [SDD_CALC_LEVEL_1 <-- SRC_CALSC_EXT_INSTALL_ANGLES]
* [SDD_CALC_LEVEL_2 <-- SRC_CALSC_EXT_INSTALL_ANGLES]
* [SDD_CALC_LEVEL_DONE <-- SRC_CALSC_EXT_INSTALL_ANGLES]
*
* Input parameters:  double  *ExtAccels - remote mag's accel
*                    double  *AHRSattitude-  AHRS's roll and pitch
*  
* Output parameters: double *angleOfi-the leveling results
* 
* Return value:  TRUE: the process is complete
*                FALSE the process is in progress
******************************************************************************/
BOOL calcExtMagInstallAngles(double  *ExtAccels, double  *AHRSattitude, double *angleOfi)
{
	int i;
	static unsigned int NumofSamples = 1;
	static double AveAccels[3],prevAveAccels[3]={0,0,0};
	static double AveAHRSAngles[2],prevAveAHRSAngles[2]={0,0};  
	BOOL returnedValue=FALSE;
	double roll, pitch; /* ext. mag pitch angle  */
	
	/*clear mag-align results*/
	magRollOffset=0;
	magPitchOffset=0;
	hardIronBias[0]=0;
	hardIronBias[1]=0;
	softIronScaleRatio = 1*IRON_SCALE;
	softIronAngle = 0;
	
	/*************************/	  
	/* recursive averaging of external (quasi-)static accel measurements.  */
	for(i=0;i<3;i++) {
		AveAccels[i] =  prevAveAccels[i]*(NumofSamples-1)/NumofSamples
						+  ExtAccels[i]/NumofSamples;
		prevAveAccels[i] = AveAccels[i];
	}
	
	/* recursive averaging of AHRS roll and pitch angles */
	for(i=0;i<2;i++) {
		AveAHRSAngles[i] = prevAveAHRSAngles[i]*(NumofSamples-1)/NumofSamples
						+ AHRSattitude[i]/NumofSamples;
		prevAveAHRSAngles[i] = AveAHRSAngles[i];
	}
	
	if (NumofSamples++ > 3000)  {    /* 30 sec @ 100Hz */
		/* calculate ext. mag angles and the corresponding  */
		/* incidence angles between ext. mag and AHC525     */
		
		roll =atan2(AveAccels[1], AveAccels[2]);
		pitch    = asin(-AveAccels[0]);
		
		/* calculate the incidence angles  */
		angleOfi[0] = roll- AveAHRSAngles[0];
		angleOfi[1]=  pitch- AveAHRSAngles[1];
		
		/* reset */
		NumofSamples = 1;
		prevAveAccels[0]=prevAveAccels[1]=prevAveAccels[2]=0;
		prevAveAHRSAngles[0]=prevAveAHRSAngles[1]=0;    
		
		returnedValue=TRUE;
	
	}
	
	return(returnedValue);
	
} 

/***************************************************************************** 
* Function name:  magCal
*
* Description:  gather data during the 380-degree turn for hard/soft iron alignment
*                                   
* Trace:
* [SDD_CALC_MAG_CAL_INIT_01 <-- SRC_MAG_CAL]
* [SDD_CALC_MAG_CAL_01 <-- SRC_MAG_CAL]
* [SDD_CALC_MAG_CAL_02 <-- SRC_MAG_CAL]
* [SDD_CALC_MAG_CAL_03 <-- SRC_MAG_CAL]
*
* Input parameters:  int state - state indicator of mag-align steps
*  
* Output parameters: 
*                   int state - state indicator of mag-align steps
*                   double *hardIron - hard iron
*                   double *softIron - soft iron
* Return value:  TRUE: the process is complete
*                FALSE the process is in progress
******************************************************************************/
BOOL magCal(int state, double *hardIron, double *softIron) 
{
	static double minMag[2], maxMag[2];
	static double filteredMag[3];
	static double integratedZRate;
	static unsigned char initForExtMag = 1;
	int i;
	static double integratedZRate_prev=0;
	static unsigned short numofpts = 0;
	double   dPsi;
	static unsigned int  fullDataSet=0;
	static BOOL resetMagCalFilter=TRUE;
	BOOL returnedValuse=FALSE;
	
	/* initialize a mag cal procedure */
	if(initForExtMag==1) {
		initForExtMag = 0;
		integratedZRate =0;
		for(i=0;i<2;i+=1) {
			minMag[i] = 0.0;
			maxMag[i] = 0.0;
		}
		numofpts = 0;
		integratedZRate_prev = gAlgorithm.attitude[2];
		fullDataSet=0;
		resetMagCalFilter=TRUE; 
	}
	
	calcLeveledMags(&gAlgorithm.attitude[0],&gAlgorithm.ExtMags[0]);
	
	/* low pass leveled mags */
	
	for(i=0;i<3;i++) {
		filteredMag[i] = gAlgorithm.leveledMags[i];
	}
	
	filterRemotemagForCal(filteredMag,&resetMagCalFilter);
	
	/* track min and max bounds  */
	for(i=0;i<2;i+=1) {
		if(minMag[i] > filteredMag[i]) minMag[i] = filteredMag[i];
		if(maxMag[i] < filteredMag[i]) maxMag[i] = filteredMag[i];
	}
	
	
	dPsi = gAlgorithm.attitude[2] - integratedZRate_prev;
	if (dPsi > PI)  {
		dPsi -= TWOPI;
	} else if(dPsi < -PI) {
		dPsi += TWOPI;
	}
	
	if (fabs(dPsi) >= DEG2RAD(1.0)) {
		sMagx1[numofpts] = filteredMag[0];
		sMagy1[numofpts] = filteredMag[1];
		numofpts++;  
		if (numofpts==TOTAL_POINT) {
			numofpts=0; 
			fullDataSet=1;
		}
		integratedZRate_prev = gAlgorithm.attitude[2]; 
		integratedZRate += dPsi;
	}
	
	
	if((state==MAG_ALIGN_STATUS_START_CAL_WITH_AUTOEND && 
		fabs(integratedZRate) > (TWOPI+DEG2RAD(20.0))) || 
		state==MAG_ALIGN_STATUS_TERMINATION) {
	
		if (fullDataSet) {
			numofpts=TOTAL_POINT;
		}
		
		stopMagCal(minMag, maxMag, &numofpts,hardIron, softIron);
		initForExtMag = 1;    /* for next time */
		returnedValuse=TRUE;
	
	} 
	
	return(returnedValuse);
}   

/***************************************************************************** 
* Function name:  stopMagCal
*
* Description:  compute hard and soft iron at the completion of the 380-degree turn for hard/soft iron alignment
*                                   
* Trace:
* [SDD_CALC_MAG_CAL_02_BIAS <-- SRC_STOP_MAG_CAL]
* [SDD_CALC_MAG_CAL_02_RATIO1 <-- SRC_STOP_MAG_CAL]
* [SDD_CALC_MAG_CAL_02_RATIO2 <-- SRC_STOP_MAG_CAL]
* [SDD_CALC_MAG_CAL_02_ANGLE <-- SRC_STOP_MAG_CAL]
*
* Input parameters:  double *minMag - minimal XY mag during the turn
*                    double *maxMag - maximal XY mag during the turn 
*                    unsigned short *numofpts - number of the data sample
*  
* Output parameters: 
*                   double *hardIron - hard iron
*                   double *softIron - soft iron
* Return value:  None
******************************************************************************/
void stopMagCal(double *minMag, double *maxMag, unsigned short *numofpts,double *hardIron, 
				double *softIron)
{  
	unsigned short i,idx;
	double tmp[2];
	double rTmp;
	double rmin,rmax;

	for(i = 0; i < 2; ++i) {
		tmp[i] = (minMag[i]+maxMag[i]) / 2.0;
	}

	for(i = 0; i < 2; ++i) {
		hardIron[i] = tmp[i];
	}
	
	/* find soft-iron axis and scale ratio  */
	rTmp = sqrt((sMagx1[0] - tmp[0]) * (sMagx1[0] - tmp[0]) 
	   + (sMagy1[0] - tmp[1]) * (sMagy1[0] - tmp[1]));
	rmin = rTmp;
	rmax = rTmp; idx = 0; 
	for (i=1;i<*numofpts;i++) {
		 rTmp = sqrt((sMagx1[i] - tmp[0]) * (sMagx1[i] - tmp[0]) 
		   + (sMagy1[i] - tmp[1]) * (sMagy1[i] - tmp[1]));
		
		/* track min and max bounds */
		if(rTmp < rmin) { 
			rmin = rTmp;          
		}
		if(rTmp > rmax) { 
			rmax = rTmp; 
			idx = i;
		}
	}

	softIron[0]=rmin/rmax;
	softIron[1]=atan2(sMagy1[idx]-tmp[1],sMagx1[idx]-tmp[0]);

}

 
/***************************************************************************** 
* Function name: calcMagScaleParameter 
*
* Description:  calculate scale factor matrix for accommodating soft iron angle effect
*                                   
* Trace:
* [SDD_CALC_SOFT_IRON_MATRIX <-- SRC_CALC_MAG_SCALE_PARAMETER]
*
* Input parameters:  
*                None
* Output parameters: 
*                None
* Return value:  None
******************************************************************************/
void calcMagScaleParameter(void)
{
	double cAlpha,sAlpha,scale;
	double softIangle;
	
	softIangle=softIronAngle/((double)MAXUINT16_OVER_2PI);
	
	cAlpha = cos(softIangle);
	sAlpha = sin(softIangle);
	scale  =softIronScaleRatio/((double)IRON_SCALE); 
	
	sfMat[0] = cAlpha*cAlpha*scale + sAlpha*sAlpha;
	sfMat[1] = sfMat[2] = cAlpha*sAlpha*scale - cAlpha*sAlpha;
	sfMat[3] = sAlpha*sAlpha*scale + cAlpha*cAlpha;
	
} 

/***************************************************************************** 
* Function name: calcLeveledMags 
*
* Description:  project the Eath's magnetometer field from CRM body frame to level frame
*                                   
* Trace:
* [SDD_CALC_LEVELED_MAG1 <-- SRC_CALC_LEVELED_MAG]
* [SDD_CALC_LEVELED_MAG2 <-- SRC_CALC_LEVELED_MAG]
*
* Input parameters:  double *AHRSrp - AHRS's roll and pitch
*                    double *remoteMag -  the Eath's magnetometer field
*  
* Output parameters: 
*                None
* Return value:  None
******************************************************************************/
void calcLeveledMags(double *AHRSrp,double *remoteMag) 
{
	int i;
	double cosP, sinP, cosR, sinR, mag[3];
    double levelMagswrtAHRS[3]; 
    double extRoll,extPitch;
	
	extRoll=magRollOffset/((double)MAXUINT16_OVER_2PI);
	extPitch=magPitchOffset/((double)MAXUINT16_OVER_2PI);
	
	for(i=0;i<3;i+=1) mag[i]= remoteMag[i];
     

	cosR = cos(extRoll);
	sinR = sin(extRoll);
	cosP = cos(extPitch);
	sinP = sin(extPitch);
		
	levelMagswrtAHRS[0] = cosP*mag[0] + sinP*(cosR*mag[2] + sinR*mag[1]);
	levelMagswrtAHRS[1] = cosR*mag[1] - sinR*mag[2];
	levelMagswrtAHRS[2] =-sinP*mag[0] + cosP*(cosR*mag[2] + sinR*mag[1]);
	

	/* Level mag wrt AHRS frame to Level frame */
	cosR = cos(AHRSrp[0]);
	sinR = sin(AHRSrp[0]);
	cosP = cos(AHRSrp[1]);
	sinP = sin(AHRSrp[1]);
	
	gAlgorithm.leveledMags[0] = cosP*levelMagswrtAHRS[0] + sinP*(cosR*levelMagswrtAHRS[2] +
							 sinR*levelMagswrtAHRS[1]);
	gAlgorithm.leveledMags[1] = cosR*levelMagswrtAHRS[1] - sinR*levelMagswrtAHRS[2];

 
}

/***************************************************************************** 
* Function name: calcMagHeading 
*
* Description:  calculate magnetic heading
*                                   
* Trace:
* [SDD_CALC_HEADING1 <-- SRC_CALC_MAG_HEADING]
* [SDD_CALC_HEADING2 <-- SRC_CALC_MAG_HEADING]
* [SDD_CALC_HEADING3 <-- SRC_CALC_MAG_HEADING]
* [SDD_CALC_HEADING4 <-- SRC_CALC_MAG_HEADING]
* [SDD_CALC_HEADING5 <-- SRC_CALC_MAG_HEADING]
*
* Input parameters:  
*                double *AHRSrp:  AHRS's roll and pitch
*                double *remoteMag:  magnetic field measurement from remote mag
* Output parameters: 
*                double*magHeading: magnetic heading
* Return value:  None
******************************************************************************/
void calcMagHeading(double *AHRSrp,double *remoteMag, double*magHeading) 
{
	double xMag, yMag,tmpX,tmpY, tmpH;
    
	calcLeveledMags(AHRSrp,remoteMag);

	tmpX = hardIronBias[0]/((double)IRON_SCALE);
    tmpY = hardIronBias[1]/((double)IRON_SCALE);

	tmpX = gAlgorithm.leveledMags[0] - tmpX ;
    tmpY = gAlgorithm.leveledMags[1] - tmpY;
	
    xMag = sfMat[0]*tmpX + sfMat[1]*tmpY;
	yMag = sfMat[2]*tmpX + sfMat[3]*tmpY;

	tmpH=atan2(-yMag, xMag); 
	*magHeading=tmpH;
	gAlgorithm.compassHeading=*magHeading;
}

/***************************************************************************** 
* Function name: initMagAlignResult 
*
* Description:  initialize the parameters for magnetic heading calculation
*                                   
* Trace:
* [SDD_INIT_EXT_MAG_CONFIG <-- SRC_INIT_MAGALIGN_RESULT]
*
* Input parameters:  
*                double *AHRSrp:  AHRS's roll and pitch
*                double *remoteMag:  magnetic field measurement from remote mag
* Output parameters: 
*                double*magHeading: magnetic heading
* Return value:  None
******************************************************************************/
void initMagAlignResult(void) 
{

	magRollOffset=gConfiguration.OffsetAnglesExtMag[0];
	magPitchOffset=gConfiguration.OffsetAnglesExtMag[1];
	hardIronBias[0]=gConfiguration.hardIronBiasExt[0];
	hardIronBias[1]=gConfiguration.hardIronBiasExt[1];
	softIronScaleRatio=gConfiguration.softIronScaleRatioExt ;
	softIronAngle=gConfiguration.softIronAngleExt;
    
    calcMagScaleParameter();
	
}

/*********************************************************************************
* Function name:	MagCalOutOfBounds	
*
* Description:	 check if the magnitude of the mag-align results are within the thretholds
*
* Trace: [SDD_CHECK_MAG_OUT_OF_BOUNDS <-- SRC_MAGCAL_IN_BOUNDS]
*
* Input parameters:	
*                   None
* Output parameters:	
*                   None
* Return value:     TRUE: the magnitude is outside the bounds
*                   FALSE: the magnitude is within the bounds
*********************************************************************************/ 
BOOL
MagCalOutOfBounds (void)
{
	BOOL outOfBounds = FALSE;
	uint16_t tmp;
	
	tmp =(uint16_t)(abs((int32_t)gConfiguration.softIronScaleRatioExt - IRON_SCALE));
	
	if ( ( (uint16_t)(abs(gConfiguration.hardIronBiasExt[0])) > gCalibration.HardIronLimit ) ||
	     ( (uint16_t)(abs(gConfiguration.hardIronBiasExt[1])) > gCalibration.HardIronLimit ) ||
	     (  tmp > gCalibration.SoftIronLimit ) ||
	     ( gConfiguration.softIronScaleRatioExt == 0) )  /* work around because abs(-32768) 
	                                                    returns zero! */
	{
		outOfBounds = TRUE;
	} 
	
	if (((uint16_t)(abs(gConfiguration.OffsetAnglesExtMag[0])) > gCalibration.RollIncidenceLimit ) ||
	     ((uint16_t)(abs(gConfiguration.OffsetAnglesExtMag[1])) > gCalibration.PitchIncidenceLimit ))
	{
		outOfBounds = TRUE;
	} 
	
	return outOfBounds;
}
