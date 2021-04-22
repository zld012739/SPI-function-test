/******************************************************************************
* File name:  taskDataAcquisition.c
*
* File description:  sensor data acquisition task runs at 400Hz, gets the 
*   data for each sensor and applies available calibration
*
******************************************************************************/

/* Includes */
// #include <stddef.h>
#include "salvodefs.h"
#include "stm32f2xx.h"
#include "bsp.h"
#include "dmu.h"

#include "debug_usart.h"
#include "debug.h"

#include "taskDataAcquisition.h"
#include "temperatureSensor.h"
#include "magnetometer.h"
#include "accelerometer.h"
#include "gyroscope.h"

#include "calibrate.h"

tSystemConfiguration gSystemConfiguration = {
    .outputDataRate =  OUTPUT_DATA_RATE,
    .accelRange = ACCEL_RANGE_4G,
    .gyroRange  = DEFAULT_GYRO_RANGE,
    .magRange   = MAG_RANGE_4000_MILLI_GA,
    .useGyro2   = FALSE,
    .outputTime = TRUE,
    .outputInUnits = TRUE,
    .outputAutoStart = FALSE,
};

void InitDataAcquisitionTimer(uint32_t outputDataRate)
{
    // Using TIM5 to do the timer because it has 32 bit
    // In the long term, want to get a sync signal from GPS or the user on 
    // TIM2 and use that to trim the TIM5 reload value. But for now, the
    // goal is to have a timer that is at the same rate as the goal 
    // outputDataRate
    TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
    NVIC_InitTypeDef NVIC_InitStructure;
    uint32_t period;
    
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM5, ENABLE);
    
    period = SystemCoreClock >> 1;
    period /= (outputDataRate - (outputDataRate >> 3)); // ODR is 350 instead of 400... at 400 it doesn't quite make the accels
    
    /* Time base configuration */
    TIM_TimeBaseStructInit(&TIM_TimeBaseStructure);
    TIM_TimeBaseStructure.TIM_Period = period;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM5, &TIM_TimeBaseStructure);
    TIM_ARRPreloadConfig(TIM5, ENABLE);

    /* Enable the TIM3 gloabal Interrupt */
    NVIC_InitStructure.NVIC_IRQChannel = TIM5_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);   
    
    /* TIM Interrupts enable */
    TIM_ITConfig(TIM5, TIM_IT_Update , ENABLE);
    
    /* TIM3 enable counter */
    TIM_Cmd(TIM5, ENABLE);
    
}

void TIM5_IRQHandler(void) 
{
    static int timerCounts = 0;
    OStypeEFlag eFlag;
    
    OSDisableHook();
    timerCounts++;
    eFlag = OSReadEFlag(EFLAGS_DATA_DONE);
       
    // each sensor will set a bit in the flags when its data is ready for 
    // being read
    AccelerometerStartReading();
    if (eFlag & EF_DONE_MAG_READ) {
        MagnetometerStartReading(); 
        OSClrEFlag(EFLAGS_DATA_DONE, EF_DONE_MAG_READ);
        
    }
//    if (eFlag & EF_DONE_TEMP_READ) {
//        TemperatureStartReading(); 
//        OSClrEFlag(EFLAGS_DATA_DONE, EF_DONE_TEMP_READ);
//    }

    if (gSystemConfiguration.useGyro2) {
        Gyro2StartReading();
    } else { 
        GyroStartReading();
    }
        
    TIM_ClearITPendingBit(TIM5, TIM_IT_Update);
    OSEnableHook();
}


void DataAquisitionStart(void)
{
    if (gSystemConfiguration.outputTime) {
        DEBUG_STRING("\tt(ms)");
    }
    DEBUG_STRING("\tAX\tAY\tAZ");
    DEBUG_STRING("\tGX\tGY\tGZ");
    DEBUG_STRING("\tMX\tMY\tMZ");
    DEBUG_STRING("\ttemp");
    DEBUG_ENDLINE();
    OSClrEFlag(EFLAGS_DATA_READY, EF_DATA_ALL);
    OSClrEFlag(EFLAGS_DATA_DONE, EF_DONE_TEMP_READ | EF_DONE_MAG_READ);
    
    InitDataAcquisitionTimer(gSystemConfiguration.outputDataRate);
    OSSetEFlag(EFLAGS_DATA_READY, EF_DATA_RUN_AQU);
}

 void DataAquisitionStop(void)
 {
    TIM_Cmd(TIM5, DISABLE);
    OSClrEFlag(EFLAGS_DATA_READY, EF_DATA_RUN_AQU);
 }

void TaskDataAcquisition(void) 
{
    // variables that remain through each OS_ call
    static uint16_t gain[NUM_GAIN_ENTRIES];
    static tTime startTime;
    static int16_t reading[NUM_SENSOR_READINGS];
    static double scaledOuput[NUM_SENSOR_READINGS];

    // short term variables
    OStypeEFlag eFlag;
    uint32_t range;
    uint32_t outputDataRate;
    int sensor;
    
    range = gSystemConfiguration.accelRange;
    outputDataRate = gSystemConfiguration.outputDataRate;
    if (!InitAccelerometer()) { ERROR_STRING("Failed to init accel.\r\n"); }
    AccelerometerConfig(&range, &outputDataRate);
    gain[ACCEL_GAIN] = AccelerometerGetGain(); 

    range = gSystemConfiguration.gyroRange; 
    outputDataRate = gSystemConfiguration.outputDataRate;;
    if (gSystemConfiguration.useGyro2) {
        if (!InitGyro2()) { ERROR_STRING("Failed to init gyro2.\r\n");}
        Gyro2Config(&range);
         gain[GYRO_GAIN] = Gyro2GetGain();
    } else {
        if (!InitGyro()) { ERROR_STRING("Failed to init gyro.\r\n");}
        GyroConfig(&range, &outputDataRate);
        gain[GYRO_GAIN] = GyroGetGain();
    }    
    if (!InitMagnetometer()) { ERROR_STRING("Failed to init mag.\r\n"); }
    range = gSystemConfiguration.magRange;
    MagnetometerConfig(&range);
    gain[MAG_GAIN] = MagnetometerGetGain();
    
    if (!InitTemperatureSensor()) { ERROR_STRING("Failed to init temp sensor.\r\n");}
    gain[TEMP_GAIN] = TemperatureGetGain();

    InitCalibration();
    for (sensor = 0; sensor < NUM_SENSOR_READINGS; sensor++) {
        reading[sensor] = 0;
    }

    if (gSystemConfiguration.outputAutoStart) {
        DataAquisitionStart();
    }
    startTime = TimeNow();
    
    while (1) {
        OS_WaitEFlag(EFLAGS_DATA_READY, EF_DATA_GYRO_ACCEL_READY | EF_DATA_RUN_AQU , OSALL_BITS, 0);
        eFlag = OSReadEFlag(EFLAGS_DATA_READY);
        
        // get whatever sensor data is available
        AccelerometerGetLastReading(&reading[ACCEL_START]); 
        GyroGetLastReading(&reading[RATE_START]);
        OSClrEFlag(EFLAGS_DATA_READY, EF_DATA_GYRO_ACCEL_READY);
        
        if (eFlag & EF_DATA_MAG_READY) {
            MagnetometerGetLastReading(&reading[MAG_START]);
            OSClrEFlag(EFLAGS_DATA_READY, EF_DATA_MAG_READY);  
            OSSetEFlag(EFLAGS_DATA_DONE, EF_DONE_MAG_READ);
        }
        if (eFlag & EF_DATA_TEMP_READY){
             TemperatureGetLastReading(&reading[TEMP_START]);
             OSClrEFlag(EFLAGS_DATA_READY, EF_DATA_TEMP_READY);
             OSSetEFlag(EFLAGS_DATA_DONE, EF_DONE_TEMP_READ);
        }
        
        if (gSystemConfiguration.outputTime) {
            DEBUG_INT("\t", TimeNow() - startTime);
        }
        
        CalibrateSensors(reading, gain, scaledOuput);       
        // FIXME: determine output method and output there... options are
        // debug UART, user UART, user SPI (stored for next read)
        // also, what output units
        if (gSystemConfiguration.outputInUnits) {
            float tmp;
            for (sensor = 0; sensor < NUM_SENSOR_READINGS; sensor++) {
                DEBUG_FLOAT("\t", scaledOuput[sensor], 2);
            }
            tmp = (float) reading[TEMP_START] / gain[TEMP_GAIN];
            DEBUG_FLOAT("\t", tmp, 2);
        } else {
            for (sensor = 0; sensor < NUM_SENSOR_READINGS; sensor++) {
                    DEBUG_INT("\t", reading[sensor]);
            }
        }
        DEBUG_ENDLINE();
        
    }
}