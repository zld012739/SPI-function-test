
#include <stm32f2xx.h>
#include "boardDefinition.h"

void InitTimer_TIM5( uint32_t TimerFreq_Hz );

extern uint8_t ToggleSyncPin;
extern uint16_t freqRatio;

extern uint32_t ElapsedTime_sec;
extern uint32_t ElapsedTime_usec;

