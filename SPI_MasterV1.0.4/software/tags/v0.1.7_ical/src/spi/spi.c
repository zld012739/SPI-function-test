/*******************************************************************************
* File name: 
* 		spi.c
*
* Description:
*       This SPI driver sets up a DMA interrupt to transfer data back and forth
*	
*******************************************************************************/

#include <stdint.h>
#include <salvo.h>
#include "dmu.h"
#include "spi.h"
#include "boardDefinition.h"
#include "stm32f2xx_dma.h"

void (*gSPICallbackOnDMAComplete)(void) = NULL;
int gSPIComplete = FALSE;

uint8_t _SPI1_Init(uint8_t cpolAndCphaHigh)
{
	SPI_InitTypeDef SPI_InitStructure;
    GPIO_InitTypeDef GPIO_InitStructure;
    DMA_InitTypeDef  DMA_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;

    SPI_DeInit(SPI1);
    SPI_Cmd(SPI1, DISABLE);

    RCC_AHB1PeriphClockCmd(SPI1_GPIO_MOSI_CLK, ENABLE);
    RCC_AHB1PeriphClockCmd(SPI1_GPIO_MOSI_CLK, ENABLE);
    RCC_AHB1PeriphClockCmd(SPI1_GPIO_SCK_CLK, ENABLE);
    RCC_AHB1PeriphClockCmd(SPI1_DMA_CLK, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, ENABLE);

    GPIO_PinAFConfig(SPI1_MOSI_PORT, SPI1_MOSI_SOURCE, GPIO_AF_SPI1);
    GPIO_PinAFConfig(SPI1_MISO_PORT, SPI1_MISO_SOURCE, GPIO_AF_SPI1);
    GPIO_PinAFConfig(SPI1_SCK_PORT,  SPI1_SCK_SOURCE,  GPIO_AF_SPI1);

    GPIO_InitStructure.GPIO_Pin = SPI1_MOSI_PIN;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(SPI1_MOSI_PORT, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = SPI1_MISO_PIN;
    GPIO_Init(SPI1_MISO_PORT, &GPIO_InitStructure);
    GPIO_InitStructure.GPIO_Pin = SPI1_SCK_PIN;
    GPIO_Init(SPI1_SCK_PORT, &GPIO_InitStructure);

    SPI_StructInit(&SPI_InitStructure);
	SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
    
    if (cpolAndCphaHigh) {
        SPI_InitStructure.SPI_CPOL = SPI_CPOL_High;
    	SPI_InitStructure.SPI_CPHA = SPI_CPHA_2Edge;    
    } else {
        SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;
        SPI_InitStructure.SPI_CPHA = SPI_CPHA_1Edge;
    }
	SPI_InitStructure.SPI_NSS = SPI_NSS_Soft; // software manages the NSS
    
    // fPCLK / baudrate
    // fPCLK is the APB1 clock frequency
    // that is currenly SystemCoreClock which is 120MHz
    // the MAX21000 can run at 10Mzh
    // but SPI mode starts slow and speeds up.
    SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_128; 

	SPI_Init(SPI1, &SPI_InitStructure);

    DMA_StructInit(&DMA_InitStructure);
    DMA_InitStructure.DMA_Channel = SPI1_DMA_CHANNEL;
    DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)(&(SPI1->DR));
    DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
    DMA_InitStructure.DMA_Priority = DMA_Priority_VeryHigh;
    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralToMemory;
    DMA_InitStructure.DMA_Memory0BaseAddr = 0;
    DMA_InitStructure.DMA_BufferSize = 0;
    DMA_Init(SPI1_DMA_RX_STREAM, &DMA_InitStructure);

    DMA_InitStructure.DMA_DIR = DMA_DIR_MemoryToPeripheral;
    DMA_Init(SPI1_DMA_TX_STREAM, &DMA_InitStructure);

    NVIC_InitStructure.NVIC_IRQChannel = SPI1_DMA_RX_STREAM_IRQ;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);     

    
    SPI_Cmd(SPI1, ENABLE);
    
    return SPI_NO_ERROR; 
}

uint8_t spi_configure(SPI_TypeDef* SPIx, uint8_t cpolAndCphaHigh, void (*callback)(void))
{
    gSPICallbackOnDMAComplete = callback;
    if (SPIx == SPI1) {
        return _SPI1_Init(cpolAndCphaHigh);
    } 
    return SPI_ERROR_GENERIC;
}

void SPI1_DMA_RX_STREAM_IRQHANDLER(void)
{
    OSDisableHook();
    if(DMA_GetITStatus(SPI1_DMA_RX_STREAM, SPI1_DMA_RX_INT_DONE))
    {
        if (SPI_I2S_GetFlagStatus(kGyroSPI, SPI_I2S_FLAG_RXNE) == RESET) {
            gSPIComplete = TRUE;
            if (gSPICallbackOnDMAComplete) {
                gSPICallbackOnDMAComplete();
            }
            DMA_ITConfig(SPI1_DMA_RX_STREAM, DMA_IT_TC, DISABLE);
        }
    }
    DMA_ClearITPendingBit(SPI1_DMA_RX_STREAM, SPI1_DMA_RX_INT_DONE); 
    OSEnableHook();
}


uint8_t spi_done(SPI_TypeDef* SPIx) 
{
    return gSPIComplete;
}
/*********************************************************************************
* Function name: 
*		spi_transfer(spiChan,mode,slaveCntrl,enCRC,inptr,outptr,data_number)
*
* Description: 
*		This routine is used for bidirectional transfer of data.
*
* Trace: 
*
* Input parameters:	
*		spiChan	    uint
*		in		uint - pointer to rx buffer, must be length long
*		out	    uint - pointer to tx buffer, should be length long
*		length	uint - number of bytes to send
*
* Output parameters: 
*		NONE
*	
* Return value: 
*		return 0 if success
************************************************************************************/
uint8_t spi_transfer(SPI_TypeDef* SPIx,uint8_t *in , uint8_t *out, uint16_t length)
{

    DMA_ClearFlag(SPI1_DMA_TX_STREAM, SPI1_DMA_TX_FLAGS);
    DMA_ClearFlag(SPI1_DMA_RX_STREAM, SPI1_DMA_RX_FLAGS);
    
    DMA_MemoryTargetConfig(SPI1_DMA_RX_STREAM, (uint32_t)in, DMA_Memory_0);
    DMA_MemoryTargetConfig(SPI1_DMA_TX_STREAM, (uint32_t)out, DMA_Memory_0);
    DMA_SetCurrDataCounter(SPI1_DMA_RX_STREAM, length);
    DMA_SetCurrDataCounter(SPI1_DMA_TX_STREAM, length);
    
    DMA_Cmd(SPI1_DMA_RX_STREAM, ENABLE);
    DMA_Cmd(SPI1_DMA_TX_STREAM, ENABLE);
    gSPIComplete = FALSE;
    DMA_ITConfig(SPI1_DMA_RX_STREAM, DMA_IT_TC, ENABLE);
    SPI_I2S_DMACmd(SPIx, SPI_I2S_DMAReq_Rx | SPI_I2S_DMAReq_Tx, ENABLE);
    
    return 0;
}

void _spi_set_baud(SPI_TypeDef* SPIx, uint16_t baudRate) 
{
    uint16_t tmpreg = 0;
    const uint16_t baudRateMask = SPI_BaudRatePrescaler_256;

    __disable_irq();
    SPI_Cmd(SPI1, DISABLE);

    tmpreg = SPIx->CR1;
    tmpreg &= ~baudRateMask;
    tmpreg |= baudRate;
    SPIx->CR1 = tmpreg;
    
    SPI_Cmd(SPI1, ENABLE);
    __enable_irq();
}
void spi_go_fast(SPI_TypeDef* SPIx)
{
    // with clock at 120Hz, this is ~8Mhz 
    // baud rate is clock/(2*prescaler)
    _spi_set_baud(SPIx, SPI_BaudRatePrescaler_8);
}

void  spi_go_slow(SPI_TypeDef* SPIx)
{
    // with clock at 120Hz, this is ~0.5Mhz 
    // baud rate is clock/(2*prescaler)
    _spi_set_baud(SPIx, SPI_BaudRatePrescaler_128);
}
