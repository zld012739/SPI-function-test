/*************************************************************
 * File name: xbowspBITStatus_priv.h
 *
 *  private flag and limit structures
 *  private initialization macros
 *  declarations of private function for prototype matching
 *  
 *
 * $Rev: 15645 $
* $Date: 2010-07-21 15:53:14 -0700 (Wed, 21 Jul 2010) $
* $Author: by-dmathis $
*
 ************************************************************/

#if ! defined BITSTATUS_PRIV_H
#define BITSTATUS_PRIV_H

/* 
 * Conversions to set default limits 
 */


/* volts at input to counts
 * 0.85625 to 4.14375  volts
 * 0       to  65535   counts
 */
#define A2D_CONV(mv) ( ( mv - 856/*.25*/) * ( 65536 / (4137 - 856) )  )

#define RATE_I_CONV(ma)
#define ACCEL_I_CONV(ma)

/* r2 connected to 2.5 v, r1 to measured */
#define RDIV(mv, r1,r2) ( ( 2500 * r1  / ( r1 + r2) ) + ( mv * r2 / ( r1 + r2 ) ) )

/* 100K & 2.8K divider */
#define VIN_CONV(mv)   A2D_CONV( RDIV( (mv), 1000, 28) )

/* 23.7K & 10.0K divider */
#define Vgen_CONV(mv)  A2D_CONV( RDIV ( (mv), 237, 100) )

/* accel current 2ma/1000 mV, 1700 microamp minimum */
#define Iaccel_CONV(ua) A2D_CONV( (ua) / 2 )

/* gyro current 20ma/1000 mV, 17 milliamp minimum  */
#define Gaccel_CONV(ma) A2D_CONV ( (ma) * 50 )

/* accel temp in K 1ua/K * 3.92K * gain of 2 */
#define Taccel_CONV(k) A2D_CONV((k) * 3920 / 1000 * 2)

/* gyro temp 2.5 V @ 25C 10mV/K */
#define Tgyro_CONV(k) A2D_CONV( 10 * (k) - (2982 - 2500) )

/* ================================================== */
/*
 * structs where the BIT and STATUS are kept
 */


struct BITerror {
     uint16_t flags;
     /* contains:
      *  masterFail
      *  hardwareError
      *  comError
      *  softwareError
	  */   

     struct hw {
          uint16_t flags;
          /* contains:
           *   powerError
           *   environmentalError
           *   sensorError
           *   internalComError 
           *   inertialPower
           *   inertialEnvironment
           *   airdataPower
           *   airdataEnvironemnt 
           */

          struct pwr {
               uint16_t  flags;
               /* contains:
                *  twoVoltIOUP  1.8V
                *  twoVoltDUP   1.8V
                *  twoFiveRef
                *  threeVolt
                *  fiveVoltA    analog
                *  fiveVoltAG   accels & gyros
                *  fiveVoltD    digital
                *  sevenVoltPwr */
          } voltage;
          struct sens {
               uint16_t flags;
               /* contains:
                *  acclCurrent
                *  rate1Current
                *  rate2Current
                *  rate3Current */
          } sens;
          struct env {
               uint16_t flags;
               /* contains:
                *  accelTemp1
                *  rateTemp1
                *  rateTemp2
                *  rateTemp3
                *  staticPressTemp
                *  dynamicPressTemp */
          } env;
          struct icom {
               uint16_t flags;
               /* contains:
                *  sensorDataError
                *  A2DError */
          } icom;         
 
     } hw;
     struct comm {
          uint16_t flags;
          /* contains:
           *  serialAError   user port
           *  serialBError   crm port */

          struct user {
               uint16_t flags;
               /* contains:
                *  transBufOverflow
                *  recBufOverflow
                *  framingError
                *  breakDetect
                *  parityError */
          } user;
          struct crm {
               uint16_t flags;
               /* contains:
                *  transBufOverflow
                *  recBufOverflow
                *  framingError
                *  breakDetect
                *  parityError */
          } crm;
          struct ports {
          	   uint16_t port0;
          	   uint16_t port1;
          	   uint16_t port2;
          	   uint16_t port3;  
          	   /* each contains:
                *  transBufOverflow
                *  recBufOverflow
                *  framingError
                *  breakDetect
                *  parityError */
            } ports;        
     } comm;
     
     struct sw {
          uint16_t flags;
          /* contains:
           *  algorithmError
           *  dataError 
	   */
          struct algo {
               uint16_t flags;
               /* contains:
                *  initialization
                *  overRange 
		*/
          } algo;
          struct data {
               uint16_t flags;
               /* contains:
                *  calibrationCRCError
                *  magAlignOutOfBounds
                *  oatOutOfRange 
		*/
          } data;
     } sw ;
  
} ;


struct BITstatus {
     uint16_t flags;
     /* contains:  
      *  masterFailKFD
      *  algoInit
      *  attitudeFail
      *  headingFail
      *  airdataFail
      *  magAlignInvalid
      *  oatFail
      *  VG mode 
      *
      *  masterStatus
      *  hardwareStatus
      *  comStatus
      *  softwareStatus
      *  sensorStatus */
   
     struct {
          uint16_t flags;
          /* contains:
           *  unlockedEEPROM
           *  oatFail
           *  extMagHardBitOn */
     } hw ;    /* hardware status */
     struct {
          uint16_t flags;
          /* contains:
           *  noExternalMag
           *  noExternalAirData */
     } comm ;  /* communication status with outside world */
     struct {
          uint16_t flags;
          /* contains:
           *  algorithmInit
           *  highGain
           *  attitudeOnlyAlgorithm
           *  turnSwitch
           *  noAirdataAiding
           *  noMagHeadingRef
           *  crmCalInProgress 0: normal; 1:hard/soft cal; 2:leveling; 3 :heading offset */
     } sw;      /* software status */
     struct {
          uint16_t flags;
          /* contains:
           *  overRange
           *  noCrmLeveling
           *  noHardSoftIronCal
           *  noCrmHeadingOffset */
     } sensor; /* sensor status      */
       
} ;


/*
 * construction of a table to split a BITFLAG
 */
struct bitCategoryTab {
     uint16_t major;
     uint16_t * flagP;
}  ;

/*
 * limit checker structure
 * 
 * Could be more graceful with 16 bit vs 32 bit limits
 * A union is less graceful, though
 *
 */
struct limits16 {
     const uint16_t * src;            /* value to check */
     const BITFLAG chkFlag;         /* flag to set if out of limits */
     uint16_t lower;                  /* lower bound */
     uint16_t upper;                  /* upper bound */
} ;


struct limits32 {

     const uint32_t * src;            /* value to check */
     const BITFLAG chkFlag;         /* flag to set if out of limits */
     uint32_t lower;                  /* lower bound */
     uint32_t upper;                  /* upper bound */
} ;




/*
 * error bit (low pass) filtering
 */

typedef struct  {
     const uint16_t  bitAdd; /* if flag (*list) is TRUE, add this to *currentF */
     const uint16_t  bitSub;            /* if flag is FALSE, subtrace this from *currentF */
     const uint16_t  bitThreshold;      /* limit for *currentF */
     BITFLAG tripped; /* set or reset this flag according to current > threshold */

     const uint16_t  nBits;                 /* count in list[] and currentF[]
                                     * since a sentinel is impractical */
     const BITFLAG * list;      /* list of flags to filter */
     uint16_t  * currentF;        /* list of filtered value for each flag in list[] */
} bitFilter;


/* quiet gcc */
extern uint16_t * bitCategory(uint32_t bit);
extern void setBITflags(uint16_t * fld, uint16_t off, uint16_t on) ;
extern int32_t getBITflags(BITFLAG err) ;
extern BOOL runBITfilter(const bitFilter * fP) ;
extern void runBITfilters(void) ;
extern void zeroFlagFilters(void);
extern void zeroBITflags(void) ;


extern void runLlBITlimits(void);
extern void runLlBIToverrange(void);
extern void runLlBITcopy(void);
extern void runLowBIT(void);

extern BOOL runMidFilterBIT(const bitFilter * fP);
extern void runMidFiltersBIT(void);
extern void restartOnOverrange(void);


extern void flowFlagsUp(BITFLAG src, BITFLAG dst);
extern void orFlagsUp(BITFLAG src, BITFLAG dst);
extern void flowStatusUp(BITFLAG src, BITFLAG dst, BOOL enabled );


extern void runMidBIT(void);
extern void runMidStatus(void);
extern void runTopBIT(void);
extern void runTopStatus(void);





#endif