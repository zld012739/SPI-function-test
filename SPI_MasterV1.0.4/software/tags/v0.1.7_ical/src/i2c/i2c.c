/*******************************************************************************
* File name: 
* i2c.c
*
* Description:
*	
* $Revision: 15960 $
*
* $Date: 2011-02-18 16:56:57 -0800 (Fri, 18 Feb 2011) $
*
* $Author: jsun $
*******************************************************************************/

#include "stdint.h"
#include "stm32f2xx.h"
#include "salvo.h"
#include "i2c.h"
#include "boardDefinition.h"
#include "debug.h"
#include "dmu.h"

#define kI2C1 0
#define kI2C3 1
#define NUM_I2C 2

#define I2C_TIMEOUT 2000
#define ERROR_TIMEOUT 0x100

enum eI2CState{ 
    i2cStateAddrSend,            /* Start already sent, now send address. */
    i2cStateAddrWaitAck,         /* Wait for ACK/NACK on address sent. */
    i2cStateReStart,             /* Repeat start. */
    i2cStateDataSend,            /* Send data. */
    i2cStateDataWaitAck,         /* Wait for ACK/NACK on data sent. */
    i2cStateRxStart,             /* Wait for data. */
    i2cStateRxData,              /* Wait for data. */
    i2cStateRxWaitStop,          /* Wait for STOP to have been transmitted. */
    i2cStateError,
    i2cStateDone                 /* Transfer completed successfully. */
} ;

static  struct {
    I2C_TypeDef* I2Cx;
    enum eI2CState state;
    uint32_t lastEvent;
    uint32_t thisEvent;
    uint32_t timeout;

    volatile uint32_t error;
    uint32_t errorCount;
    uint8_t notInUse:1; 
    
    uint8_t slaveAddr;
    uint8_t txSize;
    uint8_t rxSize;
    
    uint8_t txDataMem[2]; // two bytes of memory to hold things 

    uint8_t *txData;
    uint8_t *rxData;
    
    void (*callback)(void);
}  gI2C[NUM_I2C];


/*************************************************************************
* Function Name: _i2cx_to_index
* Parameters: base address
* Return: index into gI2C global
**************************************************************************/
static int _i2cx_to_index(I2C_TypeDef *I2Cx) 
{
    if (I2C1 == I2Cx)  {
        return kI2C1;
    }
    if (I2C3 == I2Cx) {
        return kI2C3;
    }
    return 0;
}

/*************************************************************************
* Function Name: _i2c_error_irq
* Description: Generic error interrrupt handler
* Parameters: index into gI2C global
* Return: none
**************************************************************************/
static void _i2c_error_irq(int i)
{  
    I2C_TypeDef* I2Cx = gI2C[i].I2Cx;
    gI2C[i].error = I2C_ReadRegister(I2Cx, I2C_Register_SR1) & 0xFF00;
    if(I2C_EVENT_SLAVE_ACK_FAILURE & I2C_GetLastEvent(I2Cx)) {
        // Generate Stop condition 
        I2C_GenerateSTOP(I2Cx, ENABLE);
        I2C_ClearFlag(I2Cx,I2C_FLAG_AF);
    }
    gI2C[i].state = i2cStateError;
}

/*************************************************************************
* Function Name: _i2c_error_irq
* Description: Interrupt handlers, must be so named to receive interrupt,
*   these get passed off to the generic handler
* Parameters: none
* Return: none
**************************************************************************/
void I2C1_ER_IRQHandler(void) { OSEnterCritical(); _i2c_error_irq(kI2C1);  OSLeaveCritical();}
void I2C3_ER_IRQHandler(void) { OSEnterCritical(); _i2c_error_irq(kI2C3);  OSLeaveCritical();}


/*************************************************************************
* Function Name: _i2c_event_irq
* Description: Generic I2C state machine interrrupt handler
* Parameters: index into gI2C global. Start should be sent before this is 
*   called. State machine is modelled on the one described in 
*   http://www.st.com/st-web-ui/static/active/en/resource/technical/document/application_note/CD00209826.pdf
*
* Return: none
**************************************************************************/
static void _i2c_event_irq(int i)
{  
    I2C_TypeDef* I2Cx = gI2C[i].I2Cx;
    uint32_t lastEvent = I2C_GetLastEvent(I2Cx);
    enum eI2CState prevState = gI2C[i].state;
    
    if (gI2C[i].timeout) { gI2C[i].timeout--; }
    
    if (lastEvent & I2C_IT_SB)
    {
        gI2C[i].state = i2cStateAddrWaitAck;
        if (gI2C[i].txSize) {
            I2C_Send7bitAddress(I2Cx, gI2C[i].slaveAddr, I2C_Direction_Transmitter);
        } else if (gI2C[i].rxSize) {
            I2C_Send7bitAddress(I2Cx,  gI2C[i].slaveAddr, I2C_Direction_Receiver);
        } else {
            gI2C[i].state = i2cStateError;
        }
        
    } else if ( (gI2C[i].state == i2cStateAddrWaitAck) && 
               (lastEvent & I2C_IT_ADDR)) 
    {
        if (gI2C[i].txSize) {
            I2C_SendData(I2Cx, *gI2C[i].txData);
            gI2C[i].txData++;
            gI2C[i].txSize--;
            gI2C[i].state = i2cStateDataSend;
        } else if (gI2C[i].rxSize) {
            if (1 == gI2C[i].rxSize ) {
                I2C_GenerateSTOP(I2Cx,ENABLE);
                I2C_AcknowledgeConfig(I2Cx,DISABLE);    
                gI2C[i].state = i2cStateRxData;
            } else { // gI2C[i].rxSize >= 2
                gI2C[i].state = i2cStateRxData;
            }
        } else {
            gI2C[i].state = i2cStateError;
        }
        
    } else if ((gI2C[i].state == i2cStateDataSend) && 
               (lastEvent & I2C_IT_TXE)) 
    { 
        if(gI2C[i].txSize) {
            I2C_SendData(I2Cx, *gI2C[i].txData);
            gI2C[i].txData++;
            gI2C[i].txSize--;
            gI2C[i].state = i2cStateDataSend; // same again
        } else if (gI2C[i].rxSize) {
            I2C_GenerateSTART(I2Cx, ENABLE);
            gI2C[i].state = i2cStateAddrSend;
        } else {
            // all done with tx and there is no rx
            gI2C[i].state = i2cStateDataWaitAck;
        }
    } else if ((gI2C[i].state == i2cStateDataWaitAck)  &&
               (lastEvent & I2C_IT_BTF)) 
    {
        I2C_GenerateSTOP(I2Cx,ENABLE);
        gI2C[i].state = i2cStateDone; // transmit complete            
        
    } else if ((gI2C[i].state == i2cStateRxData) &&
               ((lastEvent & I2C_IT_BTF) || (lastEvent & I2C_IT_RXNE))) 
    {
        gI2C[i].rxSize--;
        if (1 ==  gI2C[i].rxSize) {
            I2C_GenerateSTOP(I2Cx,ENABLE);
            I2C_AcknowledgeConfig(I2Cx,DISABLE);
        }
        *gI2C[i].rxData = I2C_ReceiveData(I2Cx);
        gI2C[i].rxData++;
        if (0 == gI2C[i].rxSize) {
            gI2C[i].state = i2cStateDone;
        }
    } else if (gI2C[i].state == i2cStateRxWaitStop) {
        gI2C[i].state = i2cStateDone;           
    } else {
        gI2C[i].thisEvent = lastEvent;
    }
    
    if (gI2C[i].timeout == 0) {
        gI2C[i].state = i2cStateDone;
        gI2C[i].error = ERROR_TIMEOUT;
        I2C_Cmd(gI2C[i].I2Cx, DISABLE);
        I2C_ITConfig(gI2C[i].I2Cx, I2C_IT_BUF | I2C_IT_EVT | I2C_IT_ERR, DISABLE);

    }

    if ((gI2C[i].state == i2cStateDone) && prevState != i2cStateDone) {
        if (gI2C[i].callback != NULL) {
            gI2C[i].callback();
        }
    }

    gI2C[i].lastEvent = lastEvent;
}

/*************************************************************************
* Function Name: _i2c_event_irq
* Description: Interrupt handlers, must be so named to receive interrupt,
*   these get passed off to the generic handler
* Parameters: none
* Return: none
**************************************************************************/
void I2C1_EV_IRQHandler(void) {     
    OSEnterCritical();
    _i2c_event_irq(kI2C1); 
    OSLeaveCritical();
}
void I2C3_EV_IRQHandler(void) { 
    OSEnterCritical();
    _i2c_event_irq(kI2C3); 
    OSLeaveCritical();
}


/*************************************************************************
* Function Name: i2c_open
* Parameters: which I2C channel to open (mutex) and an
*  optional callback to call when I2C transfer is complete
*
* Return: Boolean, TRUE = good
*
* Description: Init I2C interface open
*
*************************************************************************/
uint8_t i2c_open (I2C_TypeDef* I2Cx, void (*callback)())
{
    int i = _i2cx_to_index(I2Cx);
    
    if ( (gI2C[i].state != i2cStateDone) || !gI2C[i].notInUse) { return FALSE; }

    gI2C[i].timeout = I2C_TIMEOUT;
    gI2C[i].callback = callback;
    gI2C[i].notInUse = FALSE; // FIXME: needs to be atomic
    return TRUE;
}

/*************************************************************************
* Function Name: i2c_close
* Parameters: none
*
* Return: none
*
* Description: Init I2C interface release
*
*************************************************************************/
void i2c_close (I2C_TypeDef* I2Cx)
{
    int i = _i2cx_to_index(I2Cx);
    if (gI2C[i].error && !gI2C[i].notInUse) {
        // open and with error
        
    }
    gI2C[i].notInUse = TRUE; // FIXME: needs to be atomic
}

/*************************************************************************
* Function Name: I2C1_Init
* Parameters: none
*
* Return: none
*
* Description: Init I2C1 interface
*
*************************************************************************/
static  void _i2c1_init(void) 
 {
    I2C_InitTypeDef  I2C_InitStructure;
    GPIO_InitTypeDef GPIO_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;

    RCC_AHB1PeriphClockCmd(I2C1_GPIO_CLK, ENABLE);

    GPIO_PinAFConfig(I2C1_SCL_GPIO_PORT, I2C1_SCL_SOURCE, GPIO_AF_I2C1);
    GPIO_PinAFConfig(I2C1_SDA_GPIO_PORT, I2C1_SDA_SOURCE, GPIO_AF_I2C1);
    
    GPIO_InitStructure.GPIO_Pin = I2C1_SCL_PIN;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(I2C1_SCL_GPIO_PORT, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = I2C1_SDA_PIN;
    GPIO_Init(I2C1_SCL_GPIO_PORT, &GPIO_InitStructure);
            
    RCC_APB1PeriphClockCmd( RCC_APB1Periph_I2C1, ENABLE);
    
    // I2C configuration
    I2C_StructInit(&I2C_InitStructure);
    I2C_InitStructure.I2C_DutyCycle = I2C_DutyCycle_16_9;
    I2C_InitStructure.I2C_Ack = I2C_Ack_Enable;
    I2C_InitStructure.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
    I2C_InitStructure.I2C_ClockSpeed = I2C1_SPEED;
    I2C_Init(I2C1, &I2C_InitStructure);
 
   // Enable the I2C Events Interrupt
    NVIC_InitStructure.NVIC_IRQChannel = I2C1_EV_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = I2C1_INTR_PRIO;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = I2C1_INTR_SUBPRIO;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
    
    // Enable the I2C Errors Interrupt
    NVIC_InitStructure.NVIC_IRQChannel = I2C1_ER_IRQn;
    NVIC_Init(&NVIC_InitStructure);
    
 }


/*************************************************************************
* Function Name: I2C3_Init
* Parameters: none
*
* Return: none
*
* Description: Init I2C3 interface
*
*************************************************************************/
static  void _i2c3_init(void) 
 {
    I2C_InitTypeDef  I2C_InitStructure;
    GPIO_InitTypeDef GPIO_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;

    RCC_AHB1PeriphClockCmd(I2C3_GPIO_SCL_CLK, ENABLE);
    RCC_AHB1PeriphClockCmd(I2C3_GPIO_SDA_CLK, ENABLE);
        
    GPIO_PinAFConfig(I2C3_SCL_GPIO_PORT, I2C3_SCL_SOURCE, GPIO_AF_I2C3);
    GPIO_PinAFConfig(I2C3_SDA_GPIO_PORT, I2C3_SDA_SOURCE, GPIO_AF_I2C3);

    GPIO_InitStructure.GPIO_Pin = I2C3_SCL_PIN;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(I2C3_SCL_GPIO_PORT, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = I2C3_SDA_PIN;
    GPIO_Init(I2C3_SDA_GPIO_PORT, &GPIO_InitStructure);
    
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C3, ENABLE);
    
    /* I2C configuration */
    I2C_StructInit(&I2C_InitStructure);
    I2C_InitStructure.I2C_DutyCycle = I2C_DutyCycle_16_9;
    I2C_InitStructure.I2C_Ack = I2C_Ack_Enable;
    I2C_InitStructure.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
    I2C_InitStructure.I2C_ClockSpeed = I2C3_SPEED;
    I2C_Init(I2C3, &I2C_InitStructure);        

    
    // Enable the I2C Events Interrupt
    NVIC_InitStructure.NVIC_IRQChannel = I2C3_EV_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = I2C3_INTR_PRIO;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = I2C3_INTR_SUBPRIO;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
    
    // Enable the I2C Errors Interrupt
    NVIC_InitStructure.NVIC_IRQChannel = I2C3_ER_IRQn;
    NVIC_Init(&NVIC_InitStructure);
}



/*************************************************************************
* Function Name: I2C_Configure
* Parameters: which I2C to init
*
* Return: none
*
* Description: Initializes specific I2C interface with knowledge of the board
*
*************************************************************************/
void i2c_configure (I2C_TypeDef* I2Cx)
{
    int i = _i2cx_to_index(I2Cx);

    gI2C[i].I2Cx = I2Cx;
    gI2C[i].notInUse = TRUE;
    gI2C[i].state = i2cStateDone;
    
    I2C_Cmd(gI2C[i].I2Cx, DISABLE);
    I2C_DeInit(gI2C[i].I2Cx);

    if (I2C1 == I2Cx)  {
        _i2c1_init();
    } else if (I2C3 == I2Cx) {
        _i2c3_init();
    }

    /* Enable interrupts from module */
    I2C_ITConfig(gI2C[i].I2Cx, I2C_IT_BUF | I2C_IT_EVT | I2C_IT_ERR, ENABLE);
    I2C_GenerateSTOP(I2Cx,ENABLE);

    /* I2C Peripheral Enable */
    I2C_Cmd(gI2C[i].I2Cx, ENABLE);
}

/*********************************************************************************
* Function name:
*		i2c_data_request(void)
*
* Description:
*
* Trace:
*
* Input parameters:
*
* Output parameters:
*		NONE
*
* Return value:
*		NONE
************************************************************************************/
uint8_t i2c_data_request(I2C_TypeDef *I2Cx,uint16_t slave_addr,	uint8_t read_addr,
                         uint8_t* rec_buffer,  uint8_t num_bytes)
{
    int i = _i2cx_to_index(I2Cx);
    __disable_irq();
    gI2C[i].error = FALSE;
    gI2C[i].slaveAddr = slave_addr;
    gI2C[i].rxSize = num_bytes;
    gI2C[i].rxData = rec_buffer;

    // to rx bytes we also need to tx one byte to the unit, the 
    // address to read from 
    gI2C[i].txSize = 1;
    gI2C[i].txDataMem[0] = read_addr;
    gI2C[i].txData = gI2C[i].txDataMem;
    gI2C[i].state = i2cStateAddrSend;
      
    I2C_AcknowledgeConfig(I2Cx, ENABLE);
    I2C_GenerateSTART(I2Cx, ENABLE);
    __enable_irq();

    return TRUE;

}

/*********************************************************************************
* Function name:
*		i2c_data_send(void)
*
* Description:
*
* Trace:
*
* Input parameters:
*
* Output parameters:
*		NONE
*
* Return value:
*		NONE
************************************************************************************/
uint8_t i2c_data_send(I2C_TypeDef *I2Cx,uint16_t slave_addr, 
                      uint8_t* send_buffer, uint8_t num_bytes)
{
    int i = _i2cx_to_index(I2Cx);
    __disable_irq();
    gI2C[i].error = FALSE;
    gI2C[i].slaveAddr = slave_addr;
    gI2C[i].rxSize = 0;

    // to rx bytes we also need to tx one byte to the unit, the 
    // address to read from 
    gI2C[i].txSize = num_bytes;
    gI2C[i].txData = send_buffer;
    gI2C[i].state = i2cStateAddrSend;
       
    I2C_AcknowledgeConfig(I2Cx, ENABLE);
    I2C_GenerateSTART(I2Cx, ENABLE);

    __enable_irq();

    return TRUE;
}

/*********************************************************************************
* Function name:
*		i2c_is_done(void)
*
* Output parameters:
*		TRUE if i2c transaction is complete
*
* Return value:
*		TRUE or FALSE
************************************************************************************/
uint8_t i2c_is_done(I2C_TypeDef *I2Cx)
{
    int i = _i2cx_to_index(I2Cx);
    
    return (gI2C[i].state ==  i2cStateDone);
}

/*********************************************************************************
* Function name:
*		i2c_has_error(void)
*
* Output parameters:
*		FALSE if no error
*
* Return value:
*		error value
************************************************************************************/
uint32_t i2c_has_error(I2C_TypeDef *I2Cx)
{
    int i = _i2cx_to_index(I2Cx);
    return (gI2C[i].error);
}
