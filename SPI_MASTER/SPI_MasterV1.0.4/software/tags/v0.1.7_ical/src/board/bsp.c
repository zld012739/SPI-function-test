/******************************************************************************
* File name: bsp.c
*
* File description:
*		Use the file to Configure Cortex M3
*
* $Rev: 16166 $
* $Date: 2011-03-09 11:53:45 -0800 (Wed, 09 Mar 2011) $
* $Author: whpeng $
******************************************************************************/  

/* Including bsp head file */
#include "bsp.h"

/**********************************************************************
* Function name:  led_on
*
* Description:   
*		brief  Turns selected LED On.
*
* Trace:
*
* Input parameters: 
*		Led_TypeDef 	led
* Output parameters:  
*
* Return value:  
**********************************************************************/
void led_on(Led_TypeDef led)
{
	STM_EVAL_LEDOn(led);     
}

/**********************************************************************
* Function name:  led_off
*
* Description:   
*		brief  Turns selected LED off.
*
* Trace:
*
* Input parameters: 
*		Led_TypeDef 	led
* Output parameters:  
*
* Return value:  
**********************************************************************/
void led_off(Led_TypeDef led)
{
	STM_EVAL_LEDOff(led);     
}

/**********************************************************************
* Function name:  led_toggle
*
* Description:   
*		brief  Toggles the selected LED.
*
* Trace:
*
* Input parameters: 
*		Led_TypeDef 	led
* Output parameters:  
*
* Return value:  
**********************************************************************/
void led_toggle(Led_TypeDef led)
{
	STM_EVAL_LEDToggle(led);     
}

/**********************************************************************
* Function name:  BSP_init
*
* Description:   
*		This function should be called by your application code before you make use of any of the
*               functions found in this module
*
* Trace:
*
* Input parameters: 
*		
* Output parameters:  
*
* Return value:  
**********************************************************************/
void BSP_init(void)
{
    STM_EVAL_LEDInit(HEARTBEAT_LED);
    STM_EVAL_LEDInit(LED2);

	/* System clocks configuration */
	RCC_config();	
	//NVIC_config();
	GPIO_config();
	InitSystemTimer();

}

/**********************************************************************
* Function name:  GPIO_config
*
* Description:   
*		configuration GPIO
*
* Trace:
*
* Input parameters: 
*		
* Output parameters:  
*
* Return value:  
**********************************************************************/
void GPIO_config(void)
{
  STM_EVAL_LEDInit(HEARTBEAT_LED);
}

/**********************************************************************
* Function name:  RCC_config
*
* Description:   
*		Configures the different system clocks and Enable peripherals.
*
* Trace:
*
* Input parameters: 
*		
* Output parameters:  
*
* Return value:  
**********************************************************************/
void RCC_config(void)
{
#if defined(SYSTICK_RCC_CONF) /* temporary reserves */
	ErrorStatus HSEStartUpStatus;   

	/* RCC system reset(for debug purpose) */
	RCC_DeInit();

	/* Enable HSE */
	RCC_HSEConfig(RCC_HSE_ON);

	/* Wait till HSE is ready */
	HSEStartUpStatus = RCC_WaitForHSEStartUp();

	if(HSEStartUpStatus == SUCCESS)
	{
		/* Enable Prefetch Buffer */
		FLASH_PrefetchBufferCmd(FLASH_PrefetchBuffer_Enable);

		/* Flash 2 wait state */
		FLASH_SetLatency(FLASH_Latency_2);

		/* HCLK = SYSCLK */
		RCC_HCLKConfig(RCC_SYSCLK_Div1); 

		/* PCLK2 = HCLK */
		RCC_PCLK2Config(RCC_HCLK_Div1); 

		/* PCLK1 = HCLK/2 */
		RCC_PCLK1Config(RCC_HCLK_Div2);

		/* PLLCLK = 8MHz * 9 = 72 MHz */
		RCC_PLLConfig(RCC_PLLSource_HSE_Div1, RCC_PLLMul_16);

		/* Enable PLL */ 
		RCC_PLLCmd(ENABLE);

		/* Wait till PLL is ready */
		while(RCC_GetFlagStatus(RCC_FLAG_PLLRDY) == RESET)
		{
		}

		/* Select PLL as system clock source */
		RCC_SYSCLKConfig(RCC_SYSCLKSource_PLLCLK);

		/* Wait till PLL is used as system clock source */
		while(RCC_GetSYSCLKSource() != 0x08)
		{
		}
	}
#elif defined(SYSTICK_STM32_CONF)
	SystemInit();/* temporary reserves */
#endif

}

/**********************************************************************
* Function name:  NVIC_config
*
* Description:   
*		 Configures Vector Table base location.
*
* Trace:
*
* Input parameters: 
*		
* Output parameters:  
*
* Return value:  
**********************************************************************/
void NVIC_config(void)
{
#if defined (VECT_TAB_RAM)
	/* Set the Vector Table base location at 0x20000000 */ 
	NVIC_SetVectorTable(NVIC_VectTab_RAM, 0x0); 
#elif defined(VECT_TAB_FLASH_IAP)
	NVIC_SetVectorTable(NVIC_VectTab_FLASH, NVIC_FLASH_IAP);
#else  /* VECT_TAB_FLASH  */
	/* Set the Vector Table base location at 0x08000000 */ 
	NVIC_SetVectorTable(NVIC_VectTab_FLASH, 0x0);   
#endif 
	/* Configure the NVIC Preemption Priority Bits */  
    // FIXME ECW: main.c sets this to _2
	//NVIC_PriorityGroupConfig(NVIC_PriorityGroup_0);
  
}

