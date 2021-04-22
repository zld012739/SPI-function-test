/*************************************************************
* File name: xbowsp_bit.c
*
* File desciption: 
*     private and public interface to Built In Test (BIT) recording
*
* $Rev: 17474 $
* $Date: 2011-02-09 21:27:04 -0800 (Wed, 09 Feb 2011) $
* $Author: by-denglish $
************************************************************/
#include <string.h>
#include <math.h>
#include <stdint.h>
#include "matrix.h"
#include "sensor.h"
#include "mag.h"
#include "scaling.h"
#include "crc.h"
#include "ucb_packet.h"
#include "crm_packet.h"
#include "packet.h"
#include "extern_port.h"
#include "s_eeprom.h"
#include "xbowsp_BITStatus.h"
#include "xbowsp_BITStatus_priv.h"   
#include "xbowsp_BITStatus.h"
#include "xbowsp_algorithm.h"
#include "xbowsp_version.h"
#include "xbowsp_generaldrivers.h"
#include "xbowsp_init.h"
// #include "airdata.h"
#include "calc_airData.h"



/* **************************************************/
/*                  low level errors                */

/*
*
*/
static struct BITerror  bitError;
static struct BITstatus bitStatus;

/***************************************************
* Map category name to flag address table
* Auto generated from list in xbowsp_bit.def which tracks
* the BIT struct in xbowsp_BITStatus_priv.h
*
* TRACE: [SDD_BIT_CAT_MAP <-- SRC_BIT_CAT_MAP]
************************************************************/
const struct bitCategoryTab   bitCategoryTab[] = {

#  define  BIT_CAT(category, mema)  {category, &(mema)},
#  include "xbowsp_bit.def"
#  undef BIT_CAT

} ;
#define N_BITCAT (N1_BITCAT -1)



/* ================================================== */

/*
* limits checker tables
*
*/

struct limits16 limits16[N_ROWS_BITLIMITS16] =  {

	/* voltages */
	{ &adahrsBitRawSensors[TWOVOLIOUP],    BITERR_TWOVOLTIOUP,
	Vgen_CONV(1600), Vgen_CONV(2000) },

	{ &adahrsBitRawSensors[TWOVOLDUP],     BITERR_TWOVOLTDUP,
	Vgen_CONV(1600), Vgen_CONV(2000) },  

	{ &adahrsBitRawSensors[TWOFIVEREFBUF], BITERR_TWOFIVEREF,
	Vgen_CONV(2400), Vgen_CONV(2600) },

	{ &adahrsBitRawSensors[THREEVOL],     BITERR_THREEVOLT,
	Vgen_CONV(3200), Vgen_CONV(3400) },

	{ &adahrsBitRawSensors[FIVEVOLA],     BITERR_FIVEVOLTA,
	Vgen_CONV(4700), Vgen_CONV(5300) },

	{ &adahrsBitRawSensors[FIVEVOLAG],      BITERR_FIVEVOLTAG,
	Vgen_CONV(4700), Vgen_CONV(5300) },

	{ &adahrsBitRawSensors[FIVEVOLD],      BITERR_FIVEVOLTD,
	Vgen_CONV(4700), Vgen_CONV(5300) },

	{ &adahrsBitRawSensors[SEVENVOLPWR],     BITERR_SEVENVOLTPWR,
	Vgen_CONV(5800), Vgen_CONV(7800) },

	/* sensor currents */
	{ &adahrsBitRawSensors[XACUR],  BITERR_ACCL1CURRENT,
	Iaccel_CONV(2000), Iaccel_CONV(25000) },

	{ &adahrsBitRawSensors[XRCUR],  BITERR_RATE1CURRENT,
	Gaccel_CONV(18), Gaccel_CONV(50) },

	{ &adahrsBitRawSensors[YRCUR],  BITERR_RATE2CURRENT,
	Gaccel_CONV(18), Gaccel_CONV(50) },

	{ &adahrsBitRawSensors[ZRCUR],  BITERR_RATE3CURRENT,
	Gaccel_CONV(18), Gaccel_CONV(50) }

};



/***********************************************************
*
*
***************************************************************/
struct limits32 limits32[N_ROWS_BITLIMITS32] = {

	/* sensor temperatures */
	{ &rawInertialSensors[XATEMP],  BITERR_ACCEL1TEMP,
	Taccel_CONV(223), Taccel_CONV(323) },

	{ &rawInertialSensors[XRTEMP],  BITERR_RATE1TEMP,
	Tgyro_CONV(223), Gaccel_CONV(323) },

	{ &rawInertialSensors[YRTEMP],  BITERR_RATE2TEMP,
	Tgyro_CONV(223), Gaccel_CONV(323) },

	{ &rawInertialSensors[ZRTEMP],  BITERR_RATE3TEMP,
	Tgyro_CONV(223), Gaccel_CONV(323) },

	/* air data pressure sensor temp/voltage */ 
	{ &rawAirDataSensors[SPRESSTEMP],  BITERR_STATICPRESSTEMP,
	Vgen_CONV(2000), Vgen_CONV(6000) },

	{ &rawAirDataSensors[DPRESSTEMP], BITERR_DYNAMICPRESSTEMP,
	Vgen_CONV(2000), Vgen_CONV(6000) },

	{(uint32_t *)&rawAirDataSensors[OAT], BITERR_OATCURRENT,
	Vgen_CONV(500), Vgen_CONV(4000) }

} ;



/* ================================================== */

/*
* error bit (low pass) filtering
*/
/* -------------------------------------------------- */
/*            tables for each group of filters
*
*  Each group has the same filter characteristics of weighting
*  time to respond and the mid level flag to set if any in the group
*  has exceeded the threshold
*/

/* --------------------------------------------------
* TRACE:
*      [SDD_BIT_FILTER_TAB_VOLT <-- SRC_BIT_FILTER_TAB_VOLT]
*/
const  BITFLAG bitFilterVolts [] = {
	BITERR_TWOVOLTIOUP,
	BITERR_TWOVOLTDUP,
	BITERR_TWOFIVEREF,
	BITERR_THREEVOLT,
	BITERR_FIVEVOLTA,
	BITERR_FIVEVOLTAG,
	BITERR_FIVEVOLTD,
	BITERR_SEVENVOLTPWR
} ;
#define N_FILTERVOLTS ( sizeof(bitFilterVolts) / sizeof(BITFLAG) )
#define THRESH_FILTERVOLTS 1000
uint16_t  bitFilterVoltsFlt[N_FILTERVOLTS];

/* --------------------------------------------------
* TRACE:
*      [SDD_BIT_FILTER_TAB_SENSI <-- SRC_BIT_FILTER_TAB_SENSI]
*/
const BITFLAG bitFilterSensI [] = {
	BITERR_ACCL1CURRENT,
	BITERR_RATE1CURRENT,
	BITERR_RATE2CURRENT,
	BITERR_RATE3CURRENT
} ;
#define N_FILTERSENSI ( sizeof(bitFilterSensI) / sizeof(BITFLAG) )
#define THRESH_FILTERSENSI 1000
uint16_t  bitFilterSensIFlt[N_FILTERSENSI];


/* --------------------------------------------------
* TRACE:
*      [SDD_BIT_FILTER_TAB_SENST <-- SRC_BIT_FILTER_TAB_SENST]
*/
const BITFLAG bitFilterSensT [] = {
	BITERR_ACCEL1TEMP,
	BITERR_RATE1TEMP,
	BITERR_RATE2TEMP,
	BITERR_RATE3TEMP,
} ;
#define N_FILTERSENST ( sizeof(bitFilterSensT) / sizeof(BITFLAG) )
#define THRESH_FILTERSENST 1000
uint16_t  bitFilterSensTFlt[N_FILTERSENST];

/* --------------------------------------------------
* TRACE:
*      [SDD_BIT_FILTER_TAB_ADT <-- SRC_BIT_FILTER_TAB_ADT]
*/
const BITFLAG bitFilterADT [] = {
	BITERR_STATICPRESSTEMP,
	BITERR_DYNAMICPRESSTEMP
} ;
#define N_FILTERADT ( sizeof(bitFilterADT) / sizeof(BITFLAG) )
#define THRESH_FILTERADT 1000
uint16_t  bitFilterADTFlt[N_FILTERADT];

/* --------------------------------------------------
* TRACE:
*      [SDD_BIT_FILTER_TAB_UCOMM <-- SRC_BIT_FILTER_TAB_UCOMM]
*/
const BITFLAG bitFilterUComm [] = {
	BITERR_USER_TRANSBUFOVERFLOW,
	BITERR_USER_RECBUFOVERFLOW,
	BITERR_USER_FRAMINGERROR,
	BITERR_USER_BREAKDETECT,
	BITERR_USER_PARITYERROR
} ;
#define N_FILTERUCOMM ( sizeof(bitFilterUComm) / sizeof(BITFLAG) )
#define THRESH_FILTERUCOMM 1000
uint16_t  bitFilterUCommFlt[N_FILTERUCOMM];

/* --------------------------------------------------
* TRACE:
*      [SDD_BIT_FILTER_TAB_MCOMM <-- SRC_BIT_FILTER_TAB_MCOMM]
*/
const BITFLAG bitFilterMComm [] = {
	BITERR_MAG_TRANSBUFOVERFLOW,
	BITERR_MAG_RECBUFOVERFLOW,
	BITERR_MAG_FRAMINGERROR,
	BITERR_MAG_BREAKDETECT,
	BITERR_MAG_PARITYERROR
} ;
#define N_FILTERMCOMM ( sizeof(bitFilterMComm) / sizeof(BITFLAG) )
#define THRESH_FILTERMCOMM 1000
uint16_t  bitFilterMCommFlt[N_FILTERMCOMM];

/* --------------------------------------------------
* TRACE:
*      [SDD_BIT_FILTER_TAB_ICOMM <-- SRC_BIT_FILTER_TAB_ICOMM]
*/
const BITFLAG bitFilterIComm [] = {
	BITERR_SENSORDATAERROR,
	BITERR_A2DERROR,
	BITERR_IOUP_NOCOMM

} ;
#define N_FILTERICOMM ( sizeof(bitFilterIComm) / sizeof(BITFLAG) )
#define THRESH_FILTERICOMM IOUP_MAX_COMM_ERR
uint16_t  bitFilterICommFlt[N_FILTERICOMM];

/* --------------------------------------------------
* TRACE:
*      [SDD_BIT_FILTER_TAB_OVR <-- SRC_BIT_FILTER_TAB_OVR]
*/
const BITFLAG bitFilterOverrange [] = {
	BITERR_OVERRANGE
} ;
#define N_FILTEROVERRANGE ( sizeof(bitFilterOverrange) / sizeof(BITFLAG) )
#define THRESH_FILTEROVERRANGE 10
uint16_t  bitFilterOverrangeFlt[N_FILTEROVERRANGE];


/* --------------------------------------------------
* TRACE:
*      [SDD_BIT_FILTER_TAB_OAT <-- SRC_BIT_FILTER_TAB_OAT]
*/
const BITFLAG bitFilterOAT [] = {
	BITERR_OATCURRENT
} ;
#define N_FILTEROAT ( sizeof(bitFilterOAT) / sizeof(BITFLAG) )
#define THRESH_FILTEROAT 1000
uint16_t  bitFilterOATFlt[N_FILTEROAT];

/* -------------------------------------------------- */
/*  TRACE:
*     [SDD_BIT_FILTERS <-- SRC_BIT_FILTER_LIST]
*/
const bitFilter  filters[] = {
	{ 2, 1,  THRESH_FILTERVOLTS,  BITERR_HWVOLT,
	N_FILTERVOLTS, bitFilterVolts, bitFilterVoltsFlt },

	{ 2, 1, THRESH_FILTERSENSI, BITERR_HWINERTIAL_SENS,
	N_FILTERSENSI, bitFilterSensI, bitFilterSensIFlt },

	{ 2, 1, THRESH_FILTERSENST, BITERR_HWINERTIAL_ENV,
	N_FILTERSENST, bitFilterSensT, bitFilterSensTFlt },

	{ 2, 1, THRESH_FILTERADT, BITERR_HWAD_ENV,
	N_FILTERADT, bitFilterADT, bitFilterADTFlt },

	{ 2, 1, THRESH_FILTERUCOMM, BITERR_UCOMM,
	N_FILTERUCOMM,  bitFilterUComm, bitFilterUCommFlt },

	{ 2, 1, THRESH_FILTERMCOMM, BITERR_MCOMM,
	N_FILTERMCOMM, bitFilterMComm, bitFilterMCommFlt },

	{ 2, 1, THRESH_FILTERICOMM, BITERR_HWICOMM,
	N_FILTERICOMM, bitFilterIComm, bitFilterICommFlt },

	{ 2, 1, THRESH_FILTEROVERRANGE, BITERR_OVR_FLT,
	N_FILTEROVERRANGE, bitFilterOverrange, bitFilterOverrangeFlt },

	{ 2, 1, THRESH_FILTEROAT, BITSTAT_OATFAIL,
	N_FILTEROAT, bitFilterOAT, bitFilterOATFlt },

} ;
#define N_FILTERS ( sizeof(filters) / sizeof(bitFilter) )



/************************************************************
* bitCategory()
*
* Find the address of the BIT field corresponding to the BIT code
*
* Parameters: BITflag
*
* Return: address of 'error category'.field or NULL if cant find the error category
*
* TRACE:
* [SDD_BIT_CAT_ADDR <-- SRC_BIT_CAT_ADDR]
*************************************************************/
uint16_t *bitCategory(uint32_t bit)
{
	CAT_BIT cat;
	uint16_t * bcat;                       /* flags pointer */
	const struct bitCategoryTab * cP;
	unsigned int i;


	bcat = NULL;
	cat = bit >> 16;
	for(i = 0, cP = bitCategoryTab; i < N_BITCAT; i++, cP++ ) {
		if (cP->major == cat) {
			bcat = cP->flagP;
			break;
		}
	}
	return bcat;
}


/****************************************************************
* setBITflags()
* manipulate the individual flags in a BIT field 16 bit word
* All individual bits can be turned on, turned off or ignored
*
* Parameters: Address of the 16 bit field, flags to turn off, flags to turn on
*
* Return: NONE
*
* TRACE:
* [SDD_BIT_SET_BITS <-- SRC_BIT_SET_BITS]
***************************************************************/
void setBITflags(uint16_t * fld, uint16_t off, uint16_t on)
{
	uint16_t tmp;

	tmp = *fld & ~off;
	tmp |= on;
	*fld = tmp;

}

/********************************************************************
* setBITflag()
* Set or reset an individual BIT flag
*
* Parameters: BIT code, ON/OFF indicator
*
* Return: TRUE if OK,  FALSE if BIT flag was unknown
*
* TRACE:
*      [SDD_BIT_SET_FLAG <-- SRC_BIT_SET_FLAG]
********************************************************************/
BOOL setBITflag(BITFLAG err, BOOL on) {
	uint16_t * bcat;
	uint16_t flag;
	BOOL ret;


	ret = FALSE;
	if ( (bcat = bitCategory(err)) != NULL) {
		flag = err & 0xFFFF;
		setBITflags(bcat, flag, ( on == 0 ? 0 : flag )  );
		ret = TRUE;
	}
	return ret;
}


/******************************************************************
* getBITflags()
* Return the raw field value of a BITcode or a psuedo BIT code.
* A psuedo BIT code may used to return several flags from a BIT field.
*
* Parameters: BIT code
*
* Return: flags requested by the BIT code, unjustified, or -1 if
*         the BIT code is invalid
*
* TRACE:
*      [SDD_BIT_GET_BITS <-- SRC_BIT_GET_BITS]
*******************************************************************/
int32_t getBITflags(BITFLAG err)
{
	uint16_t * bclass;
	int32_t flags;

	flags = -1;                                    /* default not found */

	if ( (bclass = bitCategory(err)) != NULL) {
		flags = (int32_t)(*bclass & err & 0xFFFF);
	}
	return flags;
}


/**********************************************************************
*  getBITflag()
*  return a truth value for a BIT code
*
*  Parameters: BIT code
*
*  Returns: 1  or 0  indicating the BIT code flag
*  is set or reset if the BIT code is valid.
*  Or -1 if the the BIT code is invalid
*
* TRACE:
*        [SDD_BIT_GET_FLAG <-- SRC_BIT_GET_FLAG]
***********************************************************************/
int16_t getBITflag(BITFLAG err) {
	int32_t bits;
	int16_t ret;


	ret = -1;
	if ( (bits = getBITflags(err) ) >=  0) {  /* < 0 is error in class */
		if (bits == 0) {
			ret = 0;
		} else {
			ret = 1;
		}
	}
	return ret;
}




/***********************************************************************
* loadBITlimits()
* Read the upper and lower limits for BIT checking from EEPROM
* into the limits checking struct.
* The order read from EEPROM is specified by the limits struct array 'limits'
*
* Parameters: none
*
* Return: none
*
* TRACE:
*        [SDD_BIT_LIMITS_EEPROM <-- SRC_BIT_LIMITS_EEPROM]
**********************************************************************/
void loadBITlimits(void)
{
	struct limits16 * l16P;
	struct limits32 * l32P;
	unsigned int i;
	unsigned int epromP;
	uint16_t slim[2];
	uint32_t blim[2];



	l16P = limits16;
	epromP = EEPROM_BITLIMITS_16bit;
	for (i = 0; i < N_ROWS_BITLIMITS16; i++, epromP += 2, l16P++) {
		readEEPROMWords(epromP, sizeof(slim), slim);
		l16P->lower = slim[0];
		l16P->upper = slim[1];
	}

	l32P = limits32;
	epromP = EEPROM_BITLIMITS_32bit;
	for (i = 0; i < N_ROWS_BITLIMITS32; i++, epromP += 4, l32P++) {
		readEEPROMTwoWords(epromP, sizeof(slim), blim);
		l32P->lower = blim[0];
		l32P->upper = blim[1];
	}
}


/************************************************************
* runBITlimits()
* Set the lowest level BIT flags by checking the limit list
*    for each entry, compare the upper and lower to the 'src' pointer
*    if out of bounds, set the 'chkFlag' flag
*
* Parameters: none
*
* Return: none
*
* TRACE:
*
*  [SDD_BIT_LOW_ERROR_HW_PWR <-- SRC_BIT_LL_LIMITS]
*  [SDD_BIT_LOW_ERROR_SENS_I  <-- SRC_BIT_LL_LIMITS]
*  [SDD_BIT_LOW_ERROR_SENS_T  <-- SRC_BIT_LL_LIMITS]
*  [SDD_BIT_LOW_ERROR_HW_AD_T  <-- SRC_BIT_LL_LIMITS]
*
**********************************************************/
void runLlBITlimits(void)
{
	unsigned int i;
	struct limits16 * l16P;
	struct limits32 * l32P;
	BOOL outLimit;
	uint16_t schk;
	uint32_t bchk;



	/* 16 bit sensors */
	for (i = 0, l16P = limits16;
		i < N_ROWS_BITLIMITS16;
		i++, l16P++) {

			outLimit = FALSE;
			schk = * (l16P->src);
			if ( (schk < l16P->lower) || (schk > l16P->upper) ) {
				outLimit =  TRUE;
			}
			setBITflag(l16P->chkFlag, outLimit);
	}

	/* 32 bit sensors */
	for (i = 0, l32P = limits32;
		i < N_ROWS_BITLIMITS32;
		i++, l32P++) {

			outLimit = FALSE;
			bchk = * (l32P->src);
			if ( (bchk < l32P->lower) || (bchk > l32P->upper) ) {
				outLimit =  TRUE;
			}
			setBITflag(l32P->chkFlag, outLimit);
	}

}


/*************************************************************************
* runBIToverrange()
*
* Set the lowest level BIT flags,  check  scaled sensors for overrange
*
* Parameters: none
*
* Return: none
*
* TRACE:
*
* [SDD_BIT_LL_A_OVERRANGE <-- SRC_BIT_LL_OVERRANGE]
* [SDD_BIT_LL_G_OVERRANGE <-- SRC_BIT_LL_OVERRANGE]
* [SDD_BIT_LL_T_OVERRANGE <-- SRC_BIT_LL_OVERRANGE]
*
********************************************************************/
void runLlBIToverrange(void)
{
	unsigned int i;
	BOOL outLimit;
	double * fvP;


	outLimit = FALSE;
	/* accels */
	for (i = XACCEL, fvP = &gAlgorithm.scaledSensors[XACCEL];
		i <= ZACCEL;  i++, fvP++) {
			if ( fabs(*fvP) > gCalibration.AccelSensorRange /* g */) {
				outLimit = TRUE;
			}
	}
	/* gyros */
	for (i = XRATE, fvP = &gAlgorithm.scaledSensors[XRATE];
		i <= ZRATE;  i++, fvP++) {
			if ( fabs(*fvP) > gCalibration.GyroSensorRange /* rad/sec */) {
				outLimit = TRUE;
			}
	}
	setBITflag(BITERR_OVERRANGE, outLimit);

	/* OAT */
	outLimit = toBool(
		(algorithmAirData.scaledSensors[OAT] < OAT_LOW_DEGREE_C_LIMIT) ||
		( algorithmAirData.scaledSensors[OAT] > OAT_HIGH_DEGREE_C_LIMIT) );
	setBITflag(BITERR_OATOUTOFRANGE, outLimit);

}


/************************************************************
* runLlBITcopy()
*
* copy in any needed other flags that were set from externalroutines
* assign physical uart errors to logical (user & crm) ports
*
* Parameters: none
*
* Return: none
*
* TRACE:
*  [SDD_BIT_LL_PORT <-- SRC_BIT_LL_COPY]
**************************************************************/
void runLlBITcopy(void)
{
	uint16_t port; 


	/*
	* convert COM0-3 errors to primary and CRM port
	*/

	/* primary */
	port = ExternPortToHWPort(PRIMARY_UCB_PORT);
	if ( port <= 3 ) {
		*bitCategory(BF(BITERR_LOWUCOMM, 0)) = *bitCategory(BF(BITERR_LOWPORT0 + port, 0));
	}
	/* CRM port */
	port = ExternPortToHWPort(MAG_DIAG_PORT);
	if ( port <= 3 ) {      
		*bitCategory(BF(BITERR_LOWMCOMM, 0)) = *bitCategory(BF(BITERR_LOWPORT0 + port, 0)); 
	}
}

/****************************************************************
* runLowBIT()
* Set the lowest level BIT flags
* FIRST:  Check the limit list
*    for each entry, compare the upper and lower to the 'src' pointer
*    if out of bounds, set the 'chkFlag' flag
* SECOND: check scaled sensors for overrange
* THIRD: copy in any needed other flags that were set from external
*    routines
*
* Parameters: none
*
* Return: none
*
* TRACE:
*  [SDD_BIT_LL <-- SRC_BIT_LL]
**********************************************************/
void runLowBIT(void)
{

	runLlBITcopy();
	runLlBITlimits();
	runLlBIToverrange();

	/* other low level bits are set directly by sofware checks */
}


/****************************************************************
* zeroFlagFilters()
* Initialize the BIT code filters used in runMidFilterBIT() and runMidFilterBIT()
*
* Parameters: none
*
* Return: none
*
* TRACE:
*  [SDD_BIT_INIT_FILTERS <-- SRC_BIT_INIT_FILTERS]
*****************************************************************/
void zeroFlagFilters(void)
{
	unsigned int i, j;
	const bitFilter * bfP;

	/* each filter */
	for ( i = 0, bfP = filters;
		i < N_FILTERS;
		i++, bfP++) {

			uint16_t n = bfP->nBits;            /* # elements to clear */
			uint16_t * fvP = bfP->currentF;     /* filter value list  */
			/* clear each element */
			for (j = 0; j < n; j++, fvP++) {
				*fvP = 0;
			}
	}
}


/* -------------------------------------------------- */
/*     filter low level bits into mid level bits      */


/**********************************************************
*  runMidFilterBIT
* some BIT codes should not propagate up unless they exist
* for a length of time. Place a low pass filter on those BIT codes.
*
* Parameters: pointer to a bitFilter struct that lists
*   the BIT code to filter, the filter rate, trip value and the current value
*
* Return TRUE if the BIT code tripped,  FALSE if not tripped
*
* TRACE:
* [SDD_BIT_LL_FILTER <-- SRC_BIT_LL_FILTER]  
* [SDD_BIT_LL_FILT_TRIP <-- SRC_BIT_LL_FILTER]
*
************************************************************/
BOOL runMidFilterBIT(const bitFilter * fP)
{
	unsigned int j;
	/* cache the list count, add, subtract & threshold */
	uint16_t jLim, addF, subF, thresh;
	const BITFLAG * flagP;
	uint16_t * currentFP;
	BOOL tripped;


	tripped = FALSE;

	addF = fP->bitAdd;
	subF = fP->bitSub;
	thresh = fP->bitThreshold;

	jLim = fP->nBits;

	/* run through each entry in the BITFLAG list
	* update the current filter value */
	flagP = fP->list;
	currentFP = fP->currentF;
	for( j = 0; j < jLim; j++, flagP++, currentFP++) {
		/* add/subtract to current filter value */
		if (getBITflag(*flagP) == TRUE) { /*  */
			if (*currentFP < thresh) {   /* clip at upper limit */
				*currentFP += addF;
			}
		} else {                          /* decay */
			if (*currentFP >= subF) {
				*currentFP -= subF;
			} else {
				*currentFP = 0;
			}
		}
		/* if at or over the threshold, trip the flag */
		if (*currentFP >= thresh) {
			tripped = TRUE;
		}
	}
	setBITflag(fP->tripped, tripped);
	return tripped;
}


/**********************************************************************
*  runMidFiltersBIT()
* some BIT codes should not propagate up unless they exist
* for a length of time. Place a low pass filter on those BIT codes.
* A list of BIT codes to filter is run is in the array 'filters'
*
* Parameters: none
*
* Returns: none
*
* TRACE:
*  [SDD_BIT_FILTERS <-- SRC_BIT_FILTERS]
**********************************************************************/
void runMidFiltersBIT(void)
{
	unsigned int i;
	const bitFilter * bfP;


	for ( i = 0, bfP = filters;
		i < N_FILTERS;
		i++, bfP++) {

			runMidFilterBIT(bfP);
	}
}




/**********************************************************************
* flowFlagsUp()
* BIT flags at higher levels are a logical OR of several lower level
* flags. 'OR' a BIT field into a BIT flag.
*
* Parameters: lower level BIT field, higher level BIT code
*
* Return: none
*
* TRACE:
*  [SDD_BIT_FLAGUP <-- SRC_BIT_FLAGUP]
***********************************************************************/
void flowFlagsUp(BITFLAG src, BITFLAG dst)
{
	int32_t fLo;


	if ( (fLo = getBITflags(src)) >= 0 ) { /* no reading error and some flags are set */
		setBITflag(dst, toBool(fLo));
	}
}

/*************************************************************
* orFlagsUp()
*
* conditional set flags
* Similar to flowFlagsUp() but only sets a bit, will not clear a bit.
* Used when source flags are from different categories.
* Is a 'sticky' operation.
*
* Parameters: lower level BIT field, higher level BIT code
*
* Return: none
*
* TRACE:
*  [SDD_BIT_ORUP <-- SRC_BIT_ORUP]
*************************************************************/
void orFlagsUp(BITFLAG src, BITFLAG dst)
{
	int32_t fLo;


	if ( (fLo = getBITflags(src)) > 0 ) { /* no reading error and some flags are set */
		setBITflag(dst, toBool(fLo));
	}
}

/********************************************************************
* flowStatusUp()
* Status flags are BIT flags that have an additional configuration enable bit
* Lower level fields are logically OR'd into an upper level flag if the enable
* bit is TRUE
*
* Parameters: BIT field, BIT flag, enable bit
*
* Return: none
*
* TRACE:
* [SDD_BIT_FLAGUP_STATUS <-- SRC_BIT_FLAGUP_STATUS]
****************************************************************/
void flowStatusUp(BITFLAG src, BITFLAG dst, BOOL enabled )
{
	int32_t fLo;

	if ( (fLo = getBITflags(src) ) >= 0) {
		setBITflag(dst, toBool(fLo & enabled) );
	}
}

/*************************************************************
* runMidBIT()
*
* Flow low level error flags to the mid level error flags
*
* Paramters: none
*
* Returns: none
*
* TRACE:
* [SDD_BIT_MID_SWDATA <-- SRC_BIT_MID]
* [SDD_BIT_MID_SWALG <-- SRC_BIT_MID]
*************************************************************/
void runMidBIT(void)
{
	orFlagsUp(BITERR_LOWICOMM_NF, BITERR_HWICOMM ); /* some are filtered */
	flowFlagsUp(BITERR_LOWALGO_ALL, BITERR_SWALGO); 
	flowFlagsUp(BITERR_LOWDATA_NF, BITERR_SWDATA); 

}


/***********************************************************************
* runMidStatus()
*
* Status bits are collection of various lower level flags, there is no
* hierarchy imposed in the status flags.
*
* Parameters: none
*
* Return: none
*
* TRACE:
*
* [SDD_BIT_STAT_AD <-- SRC_BIT_MID_STATUS]
* [SDD_BIT_STAT_HW <-- SRC_BIT_MID_STATUS]
* [SDD_BIT_STAT_SENS <-- SRC_BIT_MID_STATUS]
* [SDD_BIT_STAT_XCOMM <-- SRC_BIT_MID_STATUS]
* [SDD_BIT_STAT_SW <-- SRC_BIT_MID_STATUS]
*
************************************************************************/
void runMidStatus(void)
{

	/* copy filtered sensor ll overrange to status */
	flowFlagsUp(BITERR_OVR_FLT, BITSTAT_OVERRANGE);
	/* generate internal air data fail from ll temperature or current fail */
	flowFlagsUp(BITSTAT_INTAIRDATASRC1, BITSTAT_INTAIRDATA);
	orFlagsUp(BITSTAT_INTAIRDATASRC2, BITSTAT_INTAIRDATA);

	/* generate summary flags that will be enabled with configuration bits */
	flowFlagsUp(BITSTAT_HWMID_ALL, BITSTAT_HWMID);
	flowFlagsUp(BITSTAT_SENSMID_ALL, BITSTAT_SENSMID);
	flowFlagsUp(BITSTAT_COMMID_ALL, BITSTAT_COMMID);
	flowFlagsUp(BITSTAT_SWMID_ALL, BITSTAT_SWMID);

}

/*******************************************************************
* runTopBIT()
* Top level BIT flags are set from the mid level BIT flags.
* Top level error flags are sticky regardless of the mid
* level flag being sticky. Status  flags are not sticky.
*
* Paramters: none
*
* Return: none
*
* TRACE:
* [SDD_BIT_TOP_BIT <-- SRC_BIT_TOP_BIT]
* [SDD_BIT_MASTER <-- SRC_BIT_TOP_BIT]
* [SDD_DISCRETE_MASTER_FAIL <-- SRC_BIT_TOP_BIT]
***********************************************************************/
void runTopBIT(void)
{
	/* set sticky top level bits from mid level bits */
	orFlagsUp(BITERR_HWMID_ALL, BITERR_HW);
	orFlagsUp(BITERR_COMMID_ALL, BITERR_COMM);
	orFlagsUp(BITERR_SWMID_ALL, BITERR_SW);

	/* master fail is not sticky */
	flowFlagsUp(BITERR_TOPALL, BITERR_FAIL);

	if (getBITflag(BITERR_FAIL) == 0) {
		set_master_Bit(1);   /* set hardware line if no fail bits on*/
	}
	else {
		set_master_Bit(0);   /* clear hardware line if any fail bits on*/
	}
}


/*************************************************************************
* runStatusTop()
*
* Some Top level status flags are the ORing of lower level status flags.
* A configuration bit enables the lower level field to contribute to the
* upper level flags.
* Other top level status flags are combinations of lower level status or
*  error bits.  No hierarchy of flow applies.
*
* Parameters: none
*
* Return: none
*
* TRACE:
* [SDD_BIT_MODE_ALG <-- SRC_BIT_TOP_STATUS]
* [SDD_BIT_FAIL_ATT <-- SRC_BIT_TOP_STATUS]
* [SDD_BIT_FAIL_REMMAG <-- SRC_BIT_TOP_STATUS]
* [SDD_BIT_FAIL_MAGALIGN <-- SRC_BIT_TOP_STATUS]
* [SDD_BIT_FAIL_HEAD <-- SRC_BIT_TOP_STATUS]
* [SDD_BIT_FAIL_AD <-- SRC_BIT_TOP_STATUS]
* [SDD_BIT_FAIL_OAT <-- SRC_BIT_TOP_STATUS]
* [SDD_BIT_MODE_VG <-- SRC_BIT_TOP_STATUS]
* [SDD_BIT_MODE_OPMODE <-- SRC_BIT_TOP_STATUS]
*
* [SDD_BIT_TOP_STATUS_HW <-- SRC_BIT_TOP_STATUS]
* [SDD_BIT_TOP_STATUS_SENS <-- SRC_BIT_TOP_STATUS]
* [SDD_BIT_TOP_STATUS_XCOMM <-- SRC_BIT_TOP_STATUS]
* [SDD_BIT_TOP_STATUS_SW <-- SRC_BIT_TOP_STATUS]
* [SDD_BIT_TOP_STATUS <-- SRC_BIT_TOP_STATUS]

*************************************************************************/
void runTopStatus(void)
{
	int32_t f1, f2, f3;


	/* STATUS fails are random combinations and not sticky 
	* they need combinatorial logic, typically ORing */
	/* algo init is just a copy */
	flowFlagsUp(BITSTAT_ALGORITHMINIT, BITMODE_ALGOINIT);

	/* attitude fail, OR'd */
	f1 = getBITflags(BITERR_INERTIALCURRENT);
	f2 = getBITflags(BITERR_INERTIALTEMP);
	f3 = getBITflag(BITERR_OVR_FLT);
	setBITflag(BITFAIL_ATTITUDE, toBool( (f1 > 0) || (f2 > 0) || (f3 > 0) ) );

	/* remote mag fail, ORd */
	f1 = getBITflag(BITERR_MCOMM);
	f2 = getBITflag(BITSTAT_NOEXTERNALMAG);
	f3 = getBITflag(BITSTAT_EXTMAGFAIL);
	setBITflag(BITSTAT_REMOTEMAGFAIL, toBool( (f1 > 0) || (f2 > 0) || (f3 > 0) ) );

	/* magAlignInvalid */
	flowFlagsUp(BITSTAT_MAGALIGN, BITFAIL_MAGALIGN);

	/* heading fail */
	f1 = getBITflag(BITSTAT_REMOTEMAGFAIL);        /* NOTE chaining */
	f2 = getBITflag(BITFAIL_MAGALIGN);		   /* NOTE chaining */
	f3 = getBITflag(BITFAIL_ATTITUDE);		   /* NOTE chaining */
	setBITflag(BITFAIL_HEADING, toBool( (f1 > 0) || (f2 > 0) || (f3 > 0) ) );


	/* air data fail */
	f1 =  (gCalibration.productConfiguration.bit.isADAHRS == TRUE)  ?
		getBITflag(BITSTAT_INTAIRDATA) :
	getBITflag(BITSTAT_NOEXTERNALAIRDATA);
	setBITflag(BITFAIL_AIRDATA, toBool(f1 > 0)  ); /* won't set if invalid flag */

	/* OATfail raw counts filled in early at lowlevel error */
	flowFlagsUp(BITSTAT_OATFAIL, BITFAIL_OATFAIL);

	/* VG mode */
	flowFlagsUp(BITFAIL_HEADING,  BITMODE_VGMODE);
	orFlagsUp(BITSTAT_VGFORCE, BITMODE_VGMODE);
	/* and copy down to mid status bit for user packets */
	flowFlagsUp(BITMODE_VGMODE, BITSTAT_NOMAGHEADINGREF);

	/* operational mode, 1 == high accuracy */
	f1 = getBITflag(BITFAIL_AIRDATA);
	f2 = getBITflag(BITSTAT_NOAIRDATAAIDING);
	setBITflag(BITMODE_OPMODE, toBool( (f1 == 0) && (f2 == 0) ) );


	/* masterfail */
	flowFlagsUp(BITERR_FAIL, BITFAIL_MASTERFAIL);

	/* status summary bits are enabled by configuration */

	/* top status bits are not sticky */
	flowStatusUp(BITSTAT_HWMID,   BITSTAT_HARDWARESTATUS, toBool(gConfiguration.hardwareStatusEnable));
	flowStatusUp(BITSTAT_SENSMID, BITSTAT_SENSORSTATUS,   toBool(gConfiguration.sensorStatusEnable));
	flowStatusUp(BITSTAT_COMMID,  BITSTAT_COMSTATUS,      toBool(gConfiguration.comStatusEnable));
	flowStatusUp(BITSTAT_SWMID,   BITSTAT_SOFTWARESTATUS, toBool(gConfiguration.softwareStatusEnable));

	/* master status */
	flowFlagsUp(BITSTAT_TOPALL,  BITSTAT_MASTERSTATUS);
}

/**********************************************************************************
* Module name:  handleBITandStatus
*
* Description:
*       - processes BIT and Status word logic.
*
* Trace:
* [SDD_DUP_MAIN_HANDLE_BIT <-- SRC_HANDLE_BIT_AND_STATUS]
*
* Input parameters:
*   None.
*
* Output parameters:
*       None
*
* Return value:
*   None.
*

* AUTHOR: Darren Liccardo, Jan. 2006
*           Dong An, 2007,2008
*         D Mathis 2010
*********************************************************************************/
void handleBITandStatus(void)
{

	/* low level bits */
	runLowBIT();

	/* filter low level bits,
	* hardware, communication, sensor overrange
	* to set mid level flags */
	runMidFiltersBIT();
	/* finish the mid level flags (OR in if necessary) */
	runMidBIT();

	/* bring mid level bits up to 'sticky' top bits */
	runTopBIT();

	/* STATUS */
	runMidStatus();
	runTopStatus();
}


/**************************************************************
* zeroBITflags()
* initialization helper
* load the hardware check limits from the EEPROM
* prior to running BIT checks in the main loop
* 
* Parameters: none
*
* Returns: none
*
*  TRACE:
*  [SDD_BIT_INIT <-- SRC_BIT_INIT1]
***************************************************************/
void zeroBITflags(void) {

	memset((void *)&bitError, 0, sizeof(bitError));
	memset((void *)&bitStatus, 0, sizeof(bitStatus));

}

/**********************************************************************************
* BIT_init()

* Set the initial state of all BIT flags.  Set flag filters to zero
* allowing the initialization system to record errors
*
* Parameters: none
*
* Returns: none
*
* TRACE:
* [SDD_BIT_INIT <-- SRC_BIT_INIT]
* [SDD_BIT_INIT_ALG <-- SRC_BIT_INIT]
* [SDD_BIT_INIT_ATT_ONLY <-- SRC_BIT_INIT]
* [SDD_BIT_INIT_IOUP_NOCOMM <-- SRC_BIT_INIT]
* [SDD_BIT_INIT_MAGALIGN_INVALID <-- SRC_BIT_INIT]
* [SDD_BIT_INIT_NOEXTAIRDATA <-- SRC_BIT_INIT]
* [SDD_BIT_INIT_UNLOCKEEPROM <-- SRC_BIT_INIT]
* [SDD_BIT_HW_FAIL <-- SRC_BIT_INIT]
***********************************************************************************/
void BIT_init(void) {

	zeroBITflags();                /* all error and status flags clear */
	zeroFlagFilters();             /* clear filters */

	/* always set for this unit */
	setBITflag(BITSTAT_ALGORITHMINIT, TRUE);

	/* statii cleared when valid data is received */
	setBITflag(BITSTAT_ATTITUDEONLYALGORITHM, TRUE);
	setBITflag(BITERR_IOUP_NOCOMM, TRUE);
	setBITflag(BITSTAT_MAGALIGNINVALID, TRUE);
	setBITflag(BITSTAT_NOEXTERNALAIRDATA, TRUE);

	/* indicate EEPROM is locked (can only ulock using serial command) */
	setBITflag(BITSTAT_UNLOCKEDEEPROM, FALSE);	

	/* hardware line initialized to LOW */
	set_master_Bit(1);
}


/**********************************************************************************
* Module name: logUARTerrorBIT
*
* Description:
*
*     Set the uart port error bits.
*    Errors for each uart are reported as one argument,
*    in the orderdefined in xbowsp_BITstatus.h which is the mapping of the (current) uart
*
* Trace:
* [SDD_BIT_LOW_COMM_UART_ERROR <-- SRC_BIT_UART_ERROR]
*
* Parameters:
*     port, port number
*     comErrs, errors in this port
*
* Return value:
*   None.
*
* AUTHOR:  Dong An, 2007,2008
*********************************************************************************/
void logUARTerrorBIT(uint16_t port, uint16_t comErrs)
{
	uint16_t * bcat;

	if ( (bcat = bitCategory(BITERR_LOWPORT0 + port)) != NULL) {         
		setBITflags(bcat, 0xFF, comErrs);
	}
}


/*************************************************************
* getTopERROR()
* generate an 'old' packet Master error and status word
*
* Parameters: none
*
* Returns: 16 bit top word of heirarchical errors
*
* TRACE:
* [SDD_BIT_PKT_TOPBIT <-- SRC_BIT_GET_TOPBIT]
*
*************************************************************/
uint16_t getTopERROR(void)
{
	uint16_t master;

	master = (uint16_t)(getBITflags(BITERR_TOPPKT));
	master |= (uint16_t)(getBITflags(BITSTAT_TOPPKT));

	return master;
}

/*************************************************************
* getTopSTATUS()
* generate a packet status word
*
* Parameters: none
*
* Returns: 16 bit top word of status
*
* TRACE:
* [SDD_BIT_PKT_TOPSTATUS <-- SRC_BIT_GET_TOPSTATUS]
*************************************************************/
uint16_t getTopSTATUS(void)
{
	uint16_t master;

	master = 0;
	master |= getBITflags(BITSTAT_TOPPKT);
	return master;
}
