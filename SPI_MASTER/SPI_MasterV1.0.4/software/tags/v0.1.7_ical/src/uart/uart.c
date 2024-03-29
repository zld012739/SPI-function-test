/**********************************************************************************
* File name:  uart.c
*
* File description: 
*   - UART driver for the DMU380's two user serial ports
*	- transmitting and receive of serial data and then number of bytes remaining in 
*	- the circular buffer. There is no FIFO on the uart as there is in the 525
*********************************************************************************/  
#include <stdint.h> 
#include "salvo.h"
#include "stm32f2xx.h"
#include "dmu.h"
#include "port_def.h"
#include "uart.h"
#include "comm_buffers.h" 
#include "xbowsp_BITStatus.h"   
#include "boardDefinition.h"

extern port_struct gPort0, gPort1, gPort2, gPort3; 

#define INT_DISABLED 0
#define INT_ENABLED 1

    struct sPinConfig {
        GPIO_TypeDef *port;
        uint16_t pin;
        uint16_t source;
        uint8_t altFun;
    };
        
    struct sUartConfig {
        USART_TypeDef *uart;
        uint32_t       ahb1ClockEnable;
        uint32_t       apb1ClockEnable;
        uint32_t       ahb2ClockEnable;
        struct sPinConfig tx;
        struct sPinConfig rx;
        uint16_t irqChannel;
    };
    const struct sUartConfig gUartConfig[2] = {
    {
        .uart = USER_A_UART,
        .ahb1ClockEnable = USER_A_UART_TX_GPIO_CLK | USER_A_UART_TX_GPIO_CLK,
        .apb1ClockEnable = USER_A_UART_CLK,
        .ahb2ClockEnable = 0,
        .tx = {
             .pin = USER_A_UART_TX_PIN,
            .port = USER_A_UART_TX_GPIO_PORT,
            .source = USER_A_UART_TX_SOURCE,
            .altFun = USER_A_UART_TX_AF
        },
        .rx = {
             .pin = USER_A_UART_RX_PIN,
            .port = USER_A_UART_RX_GPIO_PORT,
            .source = USER_A_UART_RX_SOURCE,
            .altFun = USER_A_UART_RX_AF
        },
        .irqChannel = USER_A_UART_IRQn,
                 
    }, {
        .uart = USER_B_UART,
        .ahb1ClockEnable = USER_B_UART_TX_GPIO_CLK | USER_B_UART_TX_GPIO_CLK,
        .apb1ClockEnable = USER_B_UART_CLK,
        .ahb2ClockEnable = 0,
        .tx = {
             .pin = USER_B_UART_TX_PIN,
            .port = USER_B_UART_TX_GPIO_PORT,
            .source = USER_B_UART_TX_SOURCE,
            .altFun = USER_B_UART_TX_AF
        },
        .rx = {
             .pin = USER_B_UART_RX_PIN,
            .port = USER_B_UART_RX_GPIO_PORT,
            .source = USER_B_UART_RX_SOURCE,
            .altFun = USER_B_UART_RX_AF
        },
        .irqChannel = USER_B_UART_IRQn,
    }
};



/**********************************************************************************
* Module name: uart_init  
*
* Description: - intializes all channels of the UART peripheral
*
* Input parameters: 
*    unsigned int	uartChannel     0-MAX_UART
*    uart_hw *port_hw   uart_hw data structure containing fifo trigger levels
*						and the baudrate for this port
*				
* Output parameters: none
*
* Return value: none
*********************************************************************************/
void uart_init(unsigned int uartChannel, uart_hw *port_hw)
{

    const struct sUartConfig *uartConfig = &(gUartConfig[uartChannel]);      
    GPIO_InitTypeDef GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;
    
    RCC_AHB1PeriphClockCmd(uartConfig->ahb1ClockEnable, ENABLE);
    RCC_APB1PeriphClockCmd(uartConfig->apb1ClockEnable, ENABLE);
    RCC_AHB2PeriphClockCmd(uartConfig->ahb2ClockEnable, ENABLE);
    
    /* configure COM TX Pins */  
    GPIO_InitStructure.GPIO_Pin = uartConfig->tx.pin;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(uartConfig->tx.port, &GPIO_InitStructure);
    GPIO_PinAFConfig(uartConfig->tx.port, uartConfig->tx.source, uartConfig->tx.altFun);
    
    /* configure COM RX Pins */    
    GPIO_InitStructure.GPIO_Pin = uartConfig->rx.pin;
    GPIO_Init(uartConfig->rx.port, &GPIO_InitStructure);
    GPIO_PinAFConfig(uartConfig->rx.port, uartConfig->rx.source, uartConfig->rx.altFun);
    
    /* USARTx configured as follow:
    - BaudRate = from port_hw
    - Word Length = 8 Bits, one stop bit, o parity, no flow control
    - Receive and transmit enabled
    */
    USART_InitStructure.USART_BaudRate = port_hw->baud;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    
    USART_Init(uartConfig->uart, &USART_InitStructure);
    
    USART_Cmd(uartConfig->uart, ENABLE);
    // clear anything in the rx  
    if (USART_GetFlagStatus(uartConfig->uart, USART_FLAG_RXNE)) {
        USART_ReceiveData(uartConfig->uart);
    }
    
    // now for interrupts, only turn rx interrupts on
    USART_ITConfig( uartConfig->uart, USART_IT_RXNE, ENABLE );
	NVIC_InitStructure.NVIC_IRQChannel = uartConfig->irqChannel;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x0C;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init( &NVIC_InitStructure );    
       
    port_hw->tx_int_flg = INT_DISABLED;
    
} /* end function uart_init */
/**********************************************************************************
* Module name: uart_read  
*
* Description: - This routine will read the designated channel receive data and put
*				all valid data into the receive circular buffer in the passed data
*				structure.  
*
* Input parameters: 
*   unsigned int channel  - 	selects UART channel to read from.  
*   port_struct	 port     - 	structure contains buffer information to be used 
*				 				to pass incomming data to higher level routines.
*
* Output parameters: None
*          
* Return value: 	None
*********************************************************************************/
void uart_read(unsigned int channel, port_struct *port)
{  
    // do nothing, the uart rx is handled in the irq
} /* end function quad_uart_read */

/**********************************************************************************
* Module name: uart_write  
*
* Description:
*  - Write the bytes in the circular buffer to the serial channel until the buffer
*  - is empty or the UART FIFO is full.  If there is more data than will fit in the
*  - FIFO the transmit interrupt will be enabled and the rest of the data will be sent
*  - by the interrupt routine.  A flag will be sent so that all further call to this
*  - routine will be ignored until the interrupt routine has completly emptied the 
*  - transmit circular buffer.  In the future consider letting the interrupt routine
*  - move all the data to the uart FIFO.
*
*  - Not the FIFO size for this processor is one (as in, there is no FIFO buffer.)
*
* Input parameters: 
*   unsigned int	channel- selects channel to read from, must be 0-3.   
*   port_struct		*port - address of the data structure that contain the  
*           		circular buffer for transmit data and the interrupt flag
*					to handshake with this routine.
*
* Output parameters:   none
*             
* Return value:  none
* 
*********************************************************************************/
void uart_write(unsigned int channel, port_struct *port)
{
    // enable tx interrupt
    USART_ITConfig( gUartConfig[channel].uart, USART_IT_TXE, ENABLE );
    port->hw.tx_int_flg = INT_ENABLED;
    
} /* end function uart_write */
/**********************************************************************************
* Module name: uart_isr  
*
* Description:  this routine will read the UART status register and if there was an
*  - interrupt generated from this UART port it will check for a transmit fifo level 
*  - interrupt. If no interrupt was generated this routine will exit.  If there was
*  - another interrupt source the interrupt enable register will be cleared except
*  - for the FIFO level interrupt.  If a valid transmit fifo level interrupt has 
*  - occured the fifo level will be read from the UART and compared to the number
*  - of bytes in the transmit buffer.  If there is more than will fit in the UART
*  - FIFO then only what will fit is move to a temporary buffer and then put in the
*  - FIFO, the interrupt remains active.  If there is less bytes in the buffer than
*  - space in the FIFO the buffer is emptied and the interrupt disabled.

* Input parameters: 
*   unsigned int	channel- selects UART channel to read from, 
*							 must be < MAX_CHANNEL   
*   port_struct		*port  - pointer to the data structure for this UART port
- uses xmit buffer and the tx_int_flg       
*
* Output parameters: port.xmit_buf.buf_outptr  
*					 port.hw.tx_int_flg 

*
* Return value:  	none
* 
*********************************************************************************/
void uart_isr (unsigned int channel, port_struct *port)
{
    USART_TypeDef *uart = gUartConfig[channel].uart;
    if (USART_GetFlagStatus(uart, USART_FLAG_RXNE)) {
        uint8_t ch = uart->DR;
        uint8_t comBuffSuccess;
        comBuffSuccess = COM_buf_in(&(port->rec_buf), &ch, 1);
  		if ( comBuffSuccess == 0) { logUARTerrorBIT(channel, CBUFOVR); }
        // fixme: signal OS to run task user comm
    }
    if (USART_GetFlagStatus(uart, USART_FLAG_TXE)) {
        if (COM_buffer_bytes (&(port->xmit_buf))) {
            uint8_t txCharacter;
            COM_buf_out(&(port->xmit_buf), &txCharacter, 1);
            USART_SendData(uart, txCharacter);
        } else { // circular buffer is empty
            USART_ITConfig( uart, USART_IT_TXE, DISABLE );
            port->hw.tx_int_flg = INT_DISABLED;
        }        
    }    
} /* end function uart_isr */ 

void USER_A_UART_IRQ()
{
    OSDisableHook();
    unsigned int channel = kUserA_UART;
    port_struct *port = &gPort0;
    uart_isr (channel, port);    
    OSEnableHook();
}

void USER_B_UART_IRQ()
{
    OSDisableHook();
    unsigned int channel = kUserB_UART;
    port_struct *port = &gPort1;
    uart_isr (channel, port);    
    OSEnableHook();
}
/**********************************************************************************
* Module name:  bytes_remaining
*
* Description:
* -returns the total amount of bytes remaing in the transmit buffer and FIFO
*
* Trace:  [SDD_EXT_PORT_DRAIN <-- SRC_UART_BYTES_REMAINING] 
* Trace:  [SDD_EXT_PORT_DRAIN_WD <-- SRC_UART_BYTES_REMAINING] 
*
* Input parameters: 
*	unsigned int	channel	-UART channel #, range 0-3
*	port_struct		*port	-pointer to port structure    
*
* Output parameters: 
*	unsigned int	byte_total	-integer number of total bytes remaining to be sent 
*
* Return value: None
*********************************************************************************/ 
unsigned int bytes_remaining(unsigned int channel, port_struct *port)
{
	unsigned int	byte_total;
    byte_total = COM_buffer_bytes (&(port->xmit_buf));
   	return byte_total;
}

