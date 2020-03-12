/*****************************************************************************
* File name:  xbowsp_init.c
*
* File description:
*   - Initialization for UCB's Comm. and Cal.
*
* $Rev: 17479 $
* $Date: 2011-02-09 22:02:39 -0800 (Wed, 09 Feb 2011) $
* $Author: by-dan $
***********************************************************************************/
#include <string.h>
#include <stdint.h>
#include "cast.h"
#include "scaling.h"
#include "matrix.h"
#include "sensor.h"
#include "crc.h"
#include "s_eeprom.h"
#include "xbowsp_IQmath.h"
#include "xbowsp_BITStatus.h"
#include "xbowsp_algorithm.h"
#include "xbowsp_version.h"
#include "xbowsp_generaldrivers.h"
#include "sensor_cal.h"
#include "xbowsp_init.h"
#include "sensor_cal.h"
#include "calc_airData.h"
#include "ucb_packet.h"
#include "extern_port_config.h"
#include "xbowsp_fields.h"
//#include "calc_sol.h"


// FIXME: gCalibration will need to be fixed in memory, to be modified with
// special code that can change the internal flash without messing up the code
CalibrationStruct gCalibration;
ConfigurationStruct gConfiguration;
AlgorithmStruct gAlgorithm;

/* DUP code base */
softwareVersionStruct dupFMversion;
/*IOUP code base */
softwareVersionStruct ioupFMversion;
/* bootloader code base */
softwareVersionStruct bootFMversion;

/***************************************************
* Function Name; CrcCcittType
*
* Description:
* helper for CRCing 16 bit values
* 
* TRACE:
*      [SDD_EEPROM_CRC_METHOD <-- SRC_CRC_LOAD_EE]
* INPUT PARAMETERS:
*
* OUTPUT PARAMETERS
*
* RETURN VALUE
*
***************************************************/
static CrcCcittType
initCRC_16bit(uint16_t v, CrcCcittType seed)
{
    uint8_t c[2];

    /* unpack 16 bit into array of 8's */
    c[0] = (uint8_t)((v >> 8) & 0xFF);
    c[1] = (uint8_t)(v & 0x00FF);

    return CrcCcitt(c, 2, seed);
}

/*********************************************************************************
* Function name:	initAlgStruct
*
* Description:
*    Initialize the algorithm structure
*
* Trace:  
*	[SDD_INIT_ALG_STRUCT <-- SRC_INIT_ALG_STRUCT]
*
* Input parameters:   
*				none
*
* Output parameters:
*               none
*
* Return value:
*            	none
*
*********************************************************************************/
void
initAlgStruct(void) {

    memset(&gAlgorithm, 0, sizeof(AlgorithmStruct)); 
    gAlgorithm.calState = MAG_ALIGN_STATUS_BACKTONORMAL; 

}

/*********************************************************************************
* Function name:	readConfigAndCalIntoMem
*
* Description:
*    Read configuration from EEPROM into RAM
*
* Trace:  
*	[SDD_INIT_CONFIGURATION_ADAHRS <-- SRC_READ_CONFIGURATION_AND_CALIBRATION_INTO_MEMORY]
*	[SDD_EEPROM_INIT_READ <-- SRC_READ_CONFIGURATION_AND_CALIBRATION_INTO_MEMORY]
* 	[SDD_INIT_MISALIGN_ADAHRS <-- SRC_READ_CONFIGURATION_AND_CALIBRATION_INTO_MEMORY]
*
Input parameters:   
*				none
*
* Output parameters:
*               none
*
* Return value:
*            	none
*
*********************************************************************************/
static void readConfigAndCalIntoMem (void)
{   
    unsigned int i;

    int16_t sensorLimit;  /* convert to floating point */ 

    union {
        IQ27 misalign[N_MISALIGN];              /* convert to floating point */
        uint16_t reserved[N_RESERVED_CAL];          /* CRC only */
        uint16_t lim16[N_ROWS_BITLIMITS16][N_COLS_BITLIMITS16];   /* CRC, read into struct later */
        uint32_t lim32[N_ROWS_BITLIMITS32][N_COLS_BITLIMITS32];   /* CRC, read into struct later */
    } eepromWordsStruct;

    /* read EEPROM for configuration fields and calibration table A */
    readEEPROMWords(0, sizeof(gConfiguration), (uint16_t *)&gConfiguration);

    /* calibration struct */
    readEEPROMTwoWords(CAL_SERIAL_NUMBER, sizeof(gCalibration.serialNumber), &gCalibration.serialNumber);

    readEEPROMWords(CAL_VERSION, sizeof(gCalibration.versionString), (uint16_t *)gCalibration.versionString);

    /* read table A & index */
    readEEPROMWords(CAL_INDEXA, sizeof(gCalibration.calibrationTableIndexA), &gCalibration.calibrationTableIndexA[0]);
    readEEPROMTwoWords(CAL_TABLEA, sizeof(gCalibration.calibrationTablesA), &gCalibration.calibrationTablesA[0][0]);

    /* read misalignment array
    * the EEPROM data must be converted to floating point */
    readEEPROMTwoWords(CAL_MISALIGN, sizeof(eepromWordsStruct.misalign), (uint16_t *)&eepromWordsStruct.misalign[0]);

    for (i=0; i<N_MISALIGN; i++) {
        gCalibration.misalign[i]=IQ27toF(eepromWordsStruct.misalign[i]);  /*covert to floating one time*/
    }

    /* unused data */
    readEEPROMWords(CAL_RESERVED, sizeof(eepromWordsStruct.reserved), &eepromWordsStruct.reserved[0]);

    /* dangerous if productConfiguration were to ever change */
    readEEPROMWords(PROD_CONFIG, sizeof(gCalibration.productConfiguration), &gCalibration.productConfiguration.all);   

    /* read table B & index */
    readEEPROMWords(CAL_INDEXB, sizeof(gCalibration.calibrationTableIndexB), &gCalibration.calibrationTableIndexB[0]);

    readEEPROMTwoWords(CAL_TABLEB,sizeof(gCalibration.calibrationTablesB), &gCalibration.calibrationTablesB[0][0]);

    /* read table C & index */
    readEEPROMWords(CAL_INDEXC, sizeof(gCalibration.calibrationTableIndexC), &gCalibration.calibrationTableIndexC[0]);

    readEEPROMTwoWords(CAL_TABLEC, sizeof(gCalibration.calibrationTablesC), &gCalibration.calibrationTablesC[0][0]);

    /* g Sensitivity */
    readEEPROMTwoWords(CAL_GSENS, G_SENS_MATRIX_SIZE, (uint16_t *)&gCalibration.gSenForGyros[0]);

    /* Accel & Gyro  sensor limits */
    readEEPROMWords(ACCEL_MAX, sizeof(sensorLimit), (uint16_t *)&sensorLimit); 
    gCalibration.AccelSensorRange = Int16ToInt32(sensorLimit);
    readEEPROMWords(GYRO_MAX, sizeof(sensorLimit), (uint16_t *)&sensorLimit);
    gCalibration.GyroSensorRange = Int16ToInt32(sensorLimit);

    /* BIT limits */
    readEEPROMWords(EEPROM_BITLIMITS_16bit, N_BITLIMITS16, (uint16_t *)&eepromWordsStruct.lim16);
    readEEPROMWords(EEPROM_BITLIMITS_32bit, N_BITLIMITS32, (uint16_t *)&eepromWordsStruct.lim32);

    /* magnetometer calibration limits */
    readEEPROMWords(ROLL_INCIDENCE_LIMIT, sizeof(gCalibration.RollIncidenceLimit), &gCalibration.RollIncidenceLimit);
    readEEPROMWords(PITCH_INCIDENCE_LIMIT, sizeof(gCalibration.PitchIncidenceLimit), &gCalibration.PitchIncidenceLimit);
    readEEPROMWords(HARD_IRON_LIMIT, sizeof(gCalibration.HardIronLimit), &gCalibration.HardIronLimit);
    readEEPROMWords(SOFT_IRON_LIMIT, sizeof(gCalibration.SoftIronLimit), &gCalibration.SoftIronLimit);
}

/*********************************************************************************
* Function name:	CheckCalibrationCrc
*
* Description:
*    Verifies the stored calibration CRC matches the computed CRC
*
* Trace:  
* 	    [SDD_EEPROM_CRC_METHOD <-- SRC_CHECK_CALIBRATION_CRC]
*       [SDD_EEPROM_CRC_DATA <-- SRC_CHECK_CALIBRATION_CRC]
*
* Input parameters:   
*				none
*
* Output parameters:
*               none
*
* Return value:
*            	boolean indicating success or failure of CRC of calibration
*		area
*
*********************************************************************************/
static BOOL CheckCalibrationCrc (void)
{
    unsigned int i;
    uint16_t eepromWord;

    CrcCcittType testCRC = CRC_CCITT_INITIAL_SEED;

    /* CRC valid EEPROM range to check */
    for (i = CALIBRATION_OFFSET; i < EEPROM_CALIBRATION_END; i+= sizeof(eepromWord)) {
        /* read next word from EEPROM */
        readEEPROMWords(i, sizeof(eepromWord), &eepromWord);
        /* CRC it */
        testCRC = initCRC_16bit(eepromWord, testCRC);
    }   

    if (testCRC == gConfiguration.calibrationCRC) {
        return TRUE;
    } else {
        return FALSE;
    }
}

/*********************************************************************************
* Function name:	ProcessCalibrationData
*
* Description:
*    Calibration data processing
*
* Trace:  
*	[SDD_INIT_CALTABLE_DEFAULT_SIZE_ADAHRS <-- SRC_PROCESS_CALIBRATION_DATA]
*   [SDD_INIT_CALTABLE_INVALID <-- SRC_PROCESS_CALIBRATION_DATA]
*	[SDD_INIT_IF_ADAHRS_01 <-- SRC_PROCESS_CALIBRATION_DATA]
*	[SDD_INIT_IF_ADAHRS_02 <-- SRC_PROCESS_CALIBRATION_DATA]
*	[SDD_INIT_CALTABLE_IEEE2TI_ADAHRS <-- SRC_PROCESS_CALIBRATION_DATA]
*   [SDD_INIT_SWAP_16_BITS <-- SRC_PROCESS_CALIBRATION_DATA]
*   [SDD_INIT_G_SENS_CHECK <-- SRC_PROCESS_CALIBRATION_DATA]
*   [SDD_WATCHDOG <-- SRC_PROCESS_CALIBRATION_DATA]
*
* Input parameters:   
*				none
*
* Output parameters:
*               none
*
* Return value:
*            	none
*
*********************************************************************************/
static void ProcessCalibrationData (void)
{ 
    unsigned int i;
    int invalidCalTableA, invalidCalTableB, invalidCalTableC;   

    /* set initial lookup index */
    for (i=0; i<(N_TABLES_A + 1); i++) {
        lastLUTRstIndexA[i]=gCalibration.calibrationTableIndexA[i];
    }
    /* simple check if the EEPROM is blank or cal table index is invalid */
    invalidCalTableA=0;
    for (i=0;i<(N_TABLES_A + 1); i++) {
        if (lastLUTRstIndexA[i]>N_ROWS_TABLE_A) {
            invalidCalTableA=1;
            break;
        }
    }
    if (invalidCalTableA) {
        /* flag cal data error */
        setBITflag(BITERR_CALDATAERROR, TRUE);

        for (i=0;i<(N_TABLES_A + 1); i++) {
            lastLUTRstIndexA[i]=2*i;
            gCalibration.calibrationTableIndexA[i]=lastLUTRstIndexA[i];
        }
    }

    /* cal tables B and C needed only if receiving air-data aiding */
    if ( gCalibration.productConfiguration.bit.isADAHRS == 1 ) {
        /* SAME for B tables */
        for (i=0; i<(N_TABLES_B + 1); i++) {
            lastLUTRstIndexB[i]=gCalibration.calibrationTableIndexB[i];
        }
        /* simple check if the EEPROM is blank or cal table index is invalid */
        invalidCalTableB=0;
        for (i=0; i < (N_TABLES_B + 1); i++) {
            if (lastLUTRstIndexB[i]>N_ROWS_TABLE_B) {
                invalidCalTableB=1;
                break;
            }
        }
        if (invalidCalTableB) {
            /* flag cal data error */
            setBITflag(BITERR_CALDATAERROR, TRUE);

            for (i=0; i < (N_TABLES_B + 1); i++) {
                lastLUTRstIndexB[i]=2*i;
                gCalibration.calibrationTableIndexB[i]=lastLUTRstIndexB[i];
            }
        }

        /* and Tables C */
        for (i=0; i<(N_TABLES_C + 1); i++) {
            lastLUTRstIndexC[i]=gCalibration.calibrationTableIndexC[i];
        }
        /* simple check if the EEPROM is blank or cal table index is invalid */
        for (i=0; i<(N_TABLES_C + 1); i++) {
            if (lastLUTRstIndexC[i]>N_ROWS_TABLE_C) {
                invalidCalTableC=1;
                break;
            }
        }
        if (invalidCalTableC) {
            /* flag cal data error */
            setBITflag(BITERR_CALDATAERROR, TRUE);

            for (i=0;i < (N_TABLES_C + 1); i++) {
                lastLUTRstIndexC[i]=2*i;
                gCalibration.calibrationTableIndexC[i]=lastLUTRstIndexC[i];
            }
        }
    }

    for (i=0;i<G_SENS_MATRIX_SIZE;i++) {
        gCalibration.gSenForGyros[i] = (((gCalibration.gSenForGyros[i] >> 16) & 0xFFFF ) |
                                       ((gCalibration.gSenForGyros[i] & 0xFFFF ) << 16));
    }

    /* simple check*/
    if ((uint32_t)gCalibration.gSenForGyros[0] == 0xFFFFFFFF &&
        (uint32_t)gCalibration.gSenForGyros[1] == 0xFFFFFFFF &&
        (uint32_t)gCalibration.gSenForGyros[2] == 0xFFFFFFFF) {
        for (i=0;i<G_SENS_MATRIX_SIZE;i++) {
            gCalibration.gSenForGyros[i]=0;
        }
    }

    /* change the second column of scaling table into c33 type */
    for (i=0; i < (gCalibration.calibrationTableIndexA[12]-gCalibration.calibrationTableIndexA[6]); i++) {
        gCalibration.calibrationTablesA[gCalibration.calibrationTableIndexA[6]+i][1]=
        convert_ieee_float_to_c33(gCalibration.calibrationTablesA[gCalibration.calibrationTableIndexA[6]+i][1]);
    }

    kick_dog();  /* in case it takes too long */

    /* cal tables B and C needed only if receiving air-data aiding */
    if ( gCalibration.productConfiguration.bit.isADAHRS == 1 ) {
        for (i=0;i<(gCalibration.calibrationTableIndexB[7]-gCalibration.calibrationTableIndexB[5]); i++) {
            gCalibration.calibrationTablesB[gCalibration.calibrationTableIndexB[5]+i][1] =
            convert_ieee_float_to_c33(gCalibration.calibrationTablesB[gCalibration.calibrationTableIndexB[5]+i][1]);
        }

        for (i=0;i<(gCalibration.calibrationTableIndexC[3]-gCalibration.calibrationTableIndexC[1]); i++) {
            gCalibration.calibrationTablesC[gCalibration.calibrationTableIndexC[1]+i][1] =
            convert_ieee_float_to_c33(gCalibration.calibrationTablesC[gCalibration.calibrationTableIndexC[1]+i][1]);
        }    
    }
}

/*********************************************************************************
* Function name:	InitAirDataSensor
*
* Description:
*    Resets the air data algorithm sensor 
*
* Trace:  
*	[SDD_AIR_DATA_DEFAULT_COND <-- SRC_INIT_AIRDATA_SENSOR]
*
* Input parameters:   
*				none
*
* Output parameters:
*               none
*
* Return value:
*            	none
*
*********************************************************************************/
void InitAirDataSensor (void)
{
    /* for air data process */
    algorithmAirData.airDataSensorFlag=DO_NOTHING;
}

/*********************************************************************************
* Function name:	GetDupSoftwareVersion
*
* Description:
*    Reads the DUP sotware version into memory 
*
* Trace:  
*	[SDD_INIT_DUP_SW_VERSION_ADAHRS <-- SRC_GET_DUP_SOFTWARE_VERSION]
*
* Input parameters:   
*				none
*
* Output parameters:
*               none
*
* Return value:
*            	none
*
*********************************************************************************/
void GetDupSoftwareVersion (void)
{
    /* get DUP software version */
    dupFMversion.major=VERSION_MAJOR;
    dupFMversion.minor=VERSION_MINOR;
    dupFMversion.patch=VERSION_PATCH;
    dupFMversion.stage=VERSION_STAGE;
    dupFMversion.build=VERSION_BUILD;   
}

/*********************************************************************************
* Function name:	initExtMagnetometer
*
* Description:
*    Reads the magnetometer configuration from EEPROM into memory.  Checks the
*    configuration values and flags if invalid.  Initializes other BIT values. 
*
* Trace:  
*	[SDD_INIT_EXT_MAG_SIGN_EXTEND <-- SRC_INIT_EXTERNAL_MAGNETOMETER]
*   [SDD_MAG_ALIGN_CHECK_BIT <--  SRC_INIT_EXTERNAL_MAGNETOMETER]
*	[SDD_MAG_INIT_RESULT <-- SRC_INIT_EXTERNAL_MAGNETOMETER]
*	[SDD_MAG_INIT_COMM_FAIL <-- SRC_INIT_EXTERNAL_MAGNETOMETER]
*	[SDD_MAG_INIT_SN_FAIL <-- SRC_INIT_EXTERNAL_MAGNETOMETER]
*
* Input parameters:   
*				none
*
* Output parameters:
*               none
*
* Return value:
*            	none
*
*********************************************************************************/
static void initExtMagnetometer (void)
{ 
    unsigned int i;

    /* for external Mag cal */
    for (i=0;i<2;i++) {
        /* casting twice neccessary in order to sign extend value out to underlying C33 data width of 32 bits for all C types */
        gConfiguration.OffsetAnglesExtMag[i] = (int16_t)(Int16ToInt32(gConfiguration.OffsetAnglesExtMag[i]));
    }
    for (i=0;i<2;i++) {
        /* casting twice neccessary in order to sign extend value out to underlying C33 data width of 32 bits for all C types */
        gConfiguration.hardIronBiasExt[i] = (int16_t)(Int16ToInt32(gConfiguration.hardIronBiasExt[i]));
    }

    /* casting twice neccessary in order to sign extend value out to underlying C33 data width of 32 bits for all C types */
    gConfiguration.softIronAngleExt = (int16_t)(Int16ToInt32(gConfiguration.softIronAngleExt));

    /* initially set external mag comm failure */
    setBITflag(BITSTAT_NOEXTERNALMAG, TRUE);

    initMagAlignResult();

    /* check the current mag calibration and set result in BIT status */
    setBITflag(BITSTAT_MAGALIGNINVALID, MagCalOutOfBounds());

    /* initially fail magnetometer serial number matching until verified by magnetometer packet */
    setBITflag(BITSTAT_MAGSN, TRUE);
}

/*********************************************************************************
* Function name:	readDupBootloaderVer
*
* Description:
*    Reads the DUP bootloader software version
*
* Trace:  
*	[SDD_INIT_BOOTLOADER_SW_VERSION <-- SRC_BOOTLOADER_VERSION]
*
* Input parameters:   
*				none
*
* Output parameters:
*               none
*
* Return value:
*            	none
*
*********************************************************************************/
void readDupBootloaderVer(void)
{
#if DMU380_NOT_WORKING

    unsigned int addr, junk ;

    /* read bootloader signature for VA packet*/
    addr = BOOTLOADER_SIGNATURE_ADDR;
    flash_read_uint(addr, &junk); 
    addr+=2; /*hardware archtecture*/
    flash_read_uint(addr, &junk); 
    addr+=2;
    bootFMversion.major=junk;

    flash_read_uint(addr, &junk); 
    addr+=2;
    bootFMversion.minor=junk;

    flash_read_uint(addr, &junk); 
    addr+=2;
    bootFMversion.patch=junk;

    flash_read_uint(addr, &junk); 
    addr+=2;
    bootFMversion.stage=junk;

    flash_read_uint(addr, &junk); 
    addr+=2;
    bootFMversion.build=junk;
#endif // DMU380_NOT_WORKING

} /* void readDupBootloaderVer(void) */

/**********************************************************************************
* Module name:  initConfigureUnit
*
* Description:
*	- initializes the data structures and configurations
*     that are read from EEPROM for DUP software to run on ADAHRS platform
*
* Trace:
* [SDD_INIT_CONFIGURATION_ADAHRS <-- SRC_INIT_CONFIGURE_UNIT]
* [SDD_EEPROM_INIT_READ <-- SRC_INIT_CONFIGURE_UNIT]
* [SDD_INIT_MISALIGN_ADAHRS <-- SRC_INIT_CONFIGURE_UNIT]
* [SDD_EEPROM_CRC_DATA <-- SRC_INIT_CONFIGURE_UNIT]
* [SDD_INIT_EEPROM_CRC_ADAHRS <-- SRC_INIT_CONFIGURE_UNIT]
* [SDD_BIT_LIMITS_EEPROM <-- SRC_INIT_CONFIGURE_UNIT]

* [SDD_CAL_G_DATA_CHECK <-- SRC_INIT_CONFIGURE_UNIT]   
* [SDD_INIT_CONFIGURATION_ORIENT_VALID <-- SRC_INIT_CONFIGURE_UNIT]
* [SDD_CFG_PORT_DEF_01 <-- SRC_INIT_CONFIGURE_UNIT]
* [SDD_CFG_PORT_DEF_02 <-- SRC_INIT_CONFIGURE_UNIT]
* [SDD_INIT_CONFIGURATION_DEFAULT_BARO <-- SRC_INIT_CONFIGURE_UNIT]

* [SDD_INIT_RPY_OFFSETS_EXTEND <-- SRC_INIT_CONFIGURE_UNIT]
* [SDD_INIT_RPY_OFFSETS <-- SRC_INIT_CONFIGURE_UNIT]
* [SDD_INIT_EXT_MAG_CONFIG <-- SRC_INIT_CONFIGURE_UNIT]
* [SDD_INIT_DUP_SW_VERSION_ADAHRS <-- SRC_INIT_CONFIGURE_UNIT]
* [SDD_INIT_BOOTLOADER_SW_VERSION <-- SRC_INIT_CONFIGURE_UNIT]
* [SDD_WATCHDOG <-- SRC_INIT_CONFIGURE_UNIT]
*
* Input parameters: none
*
* Output parameters: none
*
* Return value: none
*
*********************************************************************************/
void initConfigureUnit(void) 
{
    /* read configuration and calibration data from EEPROM into configuration structure in RAM */
    readConfigAndCalIntoMem();

    /* CRC the calibration area and set BIT appropriately */
    if (CheckCalibrationCrc() == TRUE) {
        setBITflag(BITERR_CALDATAERROR, FALSE);
    } else {
        setBITflag(BITERR_CALDATAERROR, TRUE);
    }

    /* load BIT limits */                             
    loadBITlimits();                             

    /* process calibration table indices and data */
    ProcessCalibrationData();

    /* check user orientation field for validity and default to zero */
    if (CheckOrientation(gConfiguration.orientation.all) == FALSE) {
        gConfiguration.orientation.all = 0;
    }

    /* check port configuration fields against rules */
    if (ValidPortConfiguration(&gConfiguration) == FALSE) {
        /* set to defaults */
        DefaultPortConfiguration();
    }

#if DMU380_NOT_WORKING
    /* check baro correction: set to standard if the reading from EEPROM is invalid */
    if (CheckBaroCorrection(Int16ToInt32(gConfiguration.baroCorrection)) == FALSE) {
        gConfiguration.baroCorrection = 29.92 * BARO_CORRECTION_SCALING_IN_EEPROM;
    }
    /* initialize external magnetometer configuration and BIT values */
    initExtMagnetometer(); 

#endif // DMU380_NOT_WORKING

    /*init RPY offset alignment compensation matrix*/
    initRPYAlignOffsetMat();

    kick_dog();

} /*end void initConfigureUnit(void) */
