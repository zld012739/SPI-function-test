/************************************************************
** RTOS related defintions, these don't change the RTOS directly,
** only the way that the DMU application runs on the RTOS
************************************************************/
#ifndef SALVODEFS_H
#define SALVODEFS_H

#include <salvo.h>

#define SALVO_TIMER_PRESCALE 10
#define OS_TICKS_PER_SECOND (1000/SALVO_TIMER_PRESCALE)

#define DATA_ACQ_TASK               OSTCBP(1)
#define DATA_ACQ_PRIORITY           3          // highest priority

#define USER_COMMUNICATION_TASK     OSTCBP(2)
#define USER_COMMUNICATION_PRIORITY 8

#define COMMAND_LINE_TASK           OSTCBP(3)
#define COMMAND_LINE_PRIORITY       10

#define HEARTBEAT_TASK              OSTCBP(4)
#define HEARTBEAT_PRIORITY          15         // lowest priority

#define BINSEM_SERIAL_RX          OSECBP(1)     // Serial receive
#define BINSEM_DATA_ACQ_TIMER     OSECBP(2)     // Time to get data
#define EFLAGS_DATA_READY         OSECBP(3) 
#define EFLAGS_DATA_READY_CB_P    OSEFCBP(1)    // this is a control block for data ready mailbox
#define EFLAGS_DATA_DONE          OSECBP(4) 
#define EFLAGS_DATA_DONE_CB_P     OSEFCBP(2) 
#define EFLAGS_USER               OSECBP(5) 
#define EFLAGS_USER_CB_P          OSEFCBP(3) 

// fixme: add a data time to start message for temp and mag

typedef enum {
    EF_DATA_GYRO_READY   = 0x01,
    EF_DATA_ACCEL_READY  = 0x02,
    EF_DATA_MAG_READY    = 0x04,
    EF_DATA_TEMP_READY   = 0x08,
    
    EF_DATA_RUN_AQU      = 0x80, // run the acq loop or not
} eEFlag_Data_Ready;
#define EF_DATA_GYRO_ACCEL_READY (EF_DATA_GYRO_READY | EF_DATA_ACCEL_READY)
#define EF_DATA_ALL   0xFF

typedef enum {
    EF_DONE_GYRO_READ    = 0x01,
    EF_DONE_ACCEL_READ   = 0x02,
    EF_DONE_MAG_READ     = 0x04, 
    EF_DONE_TEMP_READ    = 0x08, 
} eEFlag_Data_Done;

typedef enum {
    EF_USER_RX_A    = 0x01,
    EF_USER_RX_B    = 0x01,
    EF_USER_TX    = 0x02,
} eEFlag_User;
#define EF_USER_ALL 0xFF


#endif /* SALVODEFS_H */