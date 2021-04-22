/**
 * 
 */ 

#include "../src/board/iar_stm32f207zg_sk.h"

#define EERPOM_FLASH_ADDR                   0x080E0000 // actually set in icf file
#define EEPROM_FLASH_LENGTH                 (3*1024)
#define EEPROM_MAX_FLASH_LENGTH             (128*1024)
#define EEPROM_FLASH_SECTOR                 FLASH_Sector_11
#define EEPROM_FLASH_VOLTAGE                VoltageRange_3 // FIXME: this depends on the processor voltage

#define DEBUG_USART                        USART1
#define DEBUG_USART_CLK                    RCC_APB2Periph_USART1
#define DEBUG_USART_TX_PIN                 GPIO_Pin_9
#define DEBUG_USART_TX_GPIO_PORT           GPIOA
#define DEBUG_USART_TX_GPIO_CLK            RCC_AHB1Periph_GPIOA
#define DEBUG_USART_TX_SOURCE              GPIO_PinSource9
#define DEBUG_USART_TX_AF                  GPIO_AF_USART1
#define DEBUG_USART_RX_PIN                 GPIO_Pin_10
#define DEBUG_USART_RX_GPIO_PORT           GPIOA
#define DEBUG_USART_RX_GPIO_CLK            RCC_AHB1Periph_GPIOA
#define DEBUG_USART_RX_SOURCE              GPIO_PinSource10
#define DEBUG_USART_RX_AF                  GPIO_AF_USART1
#define DEBUG_USART_IRQn                   USART1_IRQn
#define DEBUG_USART_IRQ                    USART1_IRQHandler

// The User pins can be SPI (MOSI, MISO, CLK, Select or 
// they can be two UARTS (4 and 5) or a USART and a UART(3 and 5)
#define kUserA_UART                        0 // where it is in the uart.c gUartConfig structure
#define USER_A_UART                        UART4
#define USER_A_UART_CLK                    RCC_APB1Periph_UART4
#define USER_A_UART_TX_PIN                 GPIO_Pin_10
#define USER_A_UART_TX_GPIO_PORT           GPIOC
#define USER_A_UART_TX_GPIO_CLK            RCC_AHB1Periph_GPIOC
#define USER_A_UART_TX_SOURCE              GPIO_PinSource10
#define USER_A_UART_TX_AF                  GPIO_AF_UART4
#define USER_A_UART_RX_PIN                 GPIO_Pin_11
#define USER_A_UART_RX_GPIO_PORT           GPIOC
#define USER_A_UART_RX_GPIO_CLK            RCC_AHB1Periph_GPIOC
#define USER_A_UART_RX_SOURCE              GPIO_PinSource11
#define USER_A_UART_RX_AF                  GPIO_AF_UART4
#define USER_A_UART_IRQn                   UART4_IRQn
#define USER_A_UART_IRQ                    UART4_IRQHandler

#define kUserB_UART                        1 // where it is in the uart.c gUartConfig structure
#define USER_B_UART                        UART5
#define USER_B_UART_CLK                    RCC_APB1Periph_UART5
#define USER_B_UART_TX_PIN                 GPIO_Pin_12
#define USER_B_UART_TX_GPIO_PORT           GPIOC
#define USER_B_UART_TX_GPIO_CLK            RCC_AHB1Periph_GPIOC
#define USER_B_UART_TX_SOURCE              GPIO_PinSource2
#define USER_B_UART_TX_AF                  GPIO_AF_UART5
#define USER_B_UART_RX_PIN                 GPIO_Pin_2
#define USER_B_UART_RX_GPIO_PORT           GPIOD
#define USER_B_UART_RX_GPIO_CLK            RCC_AHB1Periph_GPIOD
#define USER_B_UART_RX_SOURCE              GPIO_PinSource2
#define USER_B_UART_RX_AF                  GPIO_AF_UART5
#define USER_B_UART_IRQn                   UART5_IRQn
#define USER_B_UART_IRQ                    UART5_IRQHandler

/************************** END USER INTERFACE PINS ************************/

// I2C1 port is B6 (SCL) and B7 (SDA), this is used for the magnetometer
#define kMagnetometerI2C        I2C1
#define kTemperatureSensorI2C   I2C1
#define I2C1_SCL_PIN                    GPIO_Pin_6
#define I2C1_SCL_GPIO_PORT              GPIOB
#define I2C1_GPIO_CLK                   RCC_AHB1Periph_GPIOB
#define I2C1_SCL_SOURCE                 GPIO_PinSource6
#define I2C1_SDA_PIN                    GPIO_Pin_7
#define I2C1_SDA_GPIO_PORT              GPIOB
#define I2C1_SDA_SOURCE                 GPIO_PinSource7
#define I2C1_SPEED                      400000 
#define I2C1_INTR_SUBPRIO               1      // fixme
#define I2C1_INTR_PRIO                  0      // fixme

/**
 * @brief Magnetometer data ready interrupt
 */
#define MAG_DATA_READY_GPIO_PIN              GPIO_Pin_8
#define MAG_DATA_READY_GPIO_PORT             GPIOB
#define MAG_DATA_READY_GPIO_CLK              RCC_AHB1Periph_GPIOB
#define MAG_DATA_READY_EXTI_LINE             EXTI_Line8
#define MAG_DATA_READY_EXTI_PORT_SOURCE      EXTI_PortSourceGPIOB
#define MAG_DATA_READY_EXTI_PIN_SOURCE       EXTI_PinSource8
#define MAG_DATA_READY_EXTI_IRQn             EXTI9_5_IRQn

// I2C3 port is A8 (SCL) and C9 (SDA), this is used for the accelerometer
#define kAccelerometerI2C               I2C3 
#define I2C3_SCL_PIN                    GPIO_Pin_8
#define I2C3_SCL_GPIO_PORT              GPIOA
#define I2C3_GPIO_SCL_CLK               RCC_AHB1Periph_GPIOA
#define I2C3_SCL_SOURCE                 GPIO_PinSource8
#define I2C3_SDA_PIN                    GPIO_Pin_9
#define I2C3_SDA_GPIO_PORT              GPIOC
#define I2C3_SDA_SOURCE                 GPIO_PinSource9
#define I2C3_GPIO_SDA_CLK               RCC_AHB1Periph_GPIOC

// accelerometer can go to 2.25MHz but the STM32F2xx cannot, 
// 600000 is possible but the asymetric-ness of the clock gets odd
#define I2C3_SPEED                      400000  
#define I2C3_INTR_SUBPRIO               1      // fixme
#define I2C3_INTR_PRIO                  0      // fixme

/**
 * @brief Accelerometer data ready interrupt 
 */
#define ACCEL_DATA_READY_GPIO_PIN              GPIO_Pin_6
#define ACCEL_DATA_READY_GPIO_PORT             GPIOC
#define ACCEL_DATA_READY_GPIO_CLK              RCC_AHB1Periph_GPIOC
#define ACCEL_DATA_READY_EXTI_LINE             EXTI_Line6
#define ACCEL_DATA_READY_EXTI_PORT_SOURCE      EXTI_PortSourceGPIOC
#define ACCEL_DATA_READY_EXTI_PIN_SOURCE       EXTI_PinSource6
#define ACCEL_DATA_READY_EXTI_IRQn             EXTI9_5_IRQn


// SPI1 port is A6 (SPI1_MISO), A5 (SPI1_SCLK), A4 (SPI1_NSS) 
// and B5 (SPI1_MOSI) but that is because it isn't easy to get to on the eval board
// these are the connections fro the MPU3300
#define kGyroSPI                     SPI1
#define SPI1_MOSI_PIN                GPIO_Pin_5
#define SPI1_MOSI_PORT               GPIOB
#define SPI1_GPIO_MOSI_CLK           RCC_AHB1Periph_GPIOB
#define SPI1_MOSI_SOURCE             GPIO_PinSource5

#define SPI1_MISO_PIN                GPIO_Pin_6
#define SPI1_MISO_PORT               GPIOA
#define SPI1_GPIO_MISO_CLK           RCC_AHB1Periph_GPIOA
#define SPI1_MISO_SOURCE             GPIO_PinSource6

#define SPI1_SCK_PIN                 GPIO_Pin_5
#define SPI1_SCK_PORT                GPIOA
#define SPI1_GPIO_SCK_CLK            RCC_AHB1Periph_GPIOA
#define SPI1_SCK_SOURCE              GPIO_PinSource5

#define SPI1_DMA                        DMA2
#define SPI1_DMA_TX_STREAM              DMA2_Stream3
#define SPI1_DMA_RX_STREAM              DMA2_Stream0
#define SPI1_DMA_CHANNEL                DMA_Channel_3 
#define SPI1_DMA_CLK                    RCC_AHB1Periph_DMA2    
#define SPI1_DMA_RX_STREAM_IRQHANDLER   DMA2_Stream0_IRQHandler
#define SPI1_DMA_TX_STREAM_IRQHANDLER   DMA2_Stream3_IRQHandler
#define SPI1_DMA_RX_STREAM_IRQ          DMA2_Stream0_IRQn


#define SPI1_DMA_TX_DONE                DMA_FLAG_TCIF3
#define SPI1_DMA_TX_FLAGS               (DMA_FLAG_FEIF3 | DMA_FLAG_DMEIF3 | DMA_FLAG_TEIF3 | DMA_FLAG_HTIF3 | DMA_FLAG_TCIF3)
#define SPI1_DMA_RX_DONE                DMA_FLAG_TCIF0 
#define SPI1_DMA_RX_FLAGS               (DMA_FLAG_FEIF0 | DMA_FLAG_DMEIF0 | DMA_FLAG_TEIF0 | DMA_FLAG_HTIF0 | DMA_FLAG_TCIF0)
#define SPI1_DMA_RX_INT_DONE            DMA_IT_TCIF0

#define INVGYRO_SELECT_PIN                  GPIO_Pin_4
#define INVGYRO_SELECT_PORT                 GPIOA
#define INVGYRO_SELECT_CLK                  RCC_AHB1Periph_GPIOA

#define INVGYRO_DATA_READY_PIN              GPIO_Pin_5
#define INVGYRO_DATA_READY_PORT             GPIOC
#define INVGYRO_DATA_READY_CLK              RCC_AHB1Periph_GPIOC
#define INVGYRO_DATA_READY_EXTI_LINE        EXTI_Line5
#define INVGYRO_DATA_READY_EXTI_PORT_SOURCE EXTI_PortSourceGPIOC
#define INVGYRO_DATA_READY_EXTI_PIN_SOURCE  EXTI_PinSource5
#define INVGYRO_DATA_READY_EXTI_IRQn        EXTI9_5_IRQn


#define MAXGYRO_SELECT_PIN                  GPIO_Pin_0
#define MAXGYRO_SELECT_PORT                 GPIOB
#define MAXGYRO_SELECT_CLK                  RCC_AHB1Periph_GPIOB

#define MAXGYRO_DATA_READY_PIN              GPIO_Pin_1
#define MAXGYRO_DATA_READY_PORT             GPIOB
#define MAXGYRO_DATA_READY_CLK              RCC_AHB1Periph_GPIOB
#define MAXGYRO_DATA_READY_EXTI_LINE        EXTI_Line1
#define MAXGYRO_DATA_READY_EXTI_PORT_SOURCE EXTI_PortSourceGPIOB
#define MAXGYRO_DATA_READY_EXTI_PIN_SOURCE  EXTI_PinSource1
#define MAXGYRO_DATA_READY_EXTI_IRQn        EXTI1_IRQn

#define MAXGYRO_SYNC_PIN                    GPIO_Pin_10
#define MAXGYRO_SYNC_PORT                   GPIOB
#define MAXGYRO_SYNC_CLK                    RCC_AHB1Periph_GPIOB
