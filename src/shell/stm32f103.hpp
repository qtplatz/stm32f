// Copyright (C) 2018 MS-Cheminformatics LLC
// Licence: CC BY-NC
// Author: Toshinobu Hondo, Ph.D.
// Contact: toshi.hondo@qtplatz.com
//

#pragma once

#ifdef __cplusplus
#include <cstdint>
#endif

#ifdef __cplusplus
namespace stm32f103 {  // as known as Blue Pill
#endif

    enum STACK {
        STACKINIT = 0x20005000   // stack top address (SRAM = 0x20000000 - 0x20004FFFF)
    };

#ifdef __cplusplus
    enum PERIPHERAL_BASE : uint32_t
#else
    enum PERIPHERAL_BASE
#endif
    {
        SRAM_BASE         = 0x20000000
        , PERIPH_BASE     = 0x40000000
        , RTC_BASE        = 0x40002800 // -- 0x40002bff
        // , APB1PERIPH_BASE = PERIPH_BASE
        //, AHBPERIPH_BASE  = ( PERIPH_BASE + 0x20000 )
        , RCC_BASE        = 0x40021000 // ( AHBPERIPH_BASE + 0x1000 )  //   RCC base address is 0x40021000
        , FLASH_BASE      = 0x40022000 // ( AHBPERIPH_BASE + 0x2000 )  // FLASH base address is 0x40022000
        , APB2PERIPH_BASE = 0x40010000 // ( PERIPH_BASE + 0x10000 )
        , AFIO_BASE       = 0x40010000 // ( APB2PERIPH_BASE + 0x0000 ) //  AFIO base address is 0x40010000
        , CAN1_BASE       = 0x40006400
        , CAN2_BASE       = 0x40006800
        , ADC1_BASE	      = 0x40012400  // Table 3. p50, RM0008, Rev 17
        , ADC2_BASE	      = 0x40012800
        , SYSTICK_BASE	  = 0xe000e010
        , NVIC_BASE       = 0xe000e100
    };

    enum GPIO_BASE {
        GPIOA_BASE	      = ( APB2PERIPH_BASE + 0x0800 ) // GPIOA base address is 0x40010800
        , GPIOB_BASE      = ( APB2PERIPH_BASE + 0x0c00 ) // GPIOB base address is 0x40010C00
        , GPIOC_BASE      = ( APB2PERIPH_BASE + 0x1000 ) // GPIOC base address is 0x40011000        
    };
#ifdef __cplusplus    
    enum USART_BASE : uint32_t
#else
    enum USART_BASE
#endif
    {
        USART1_BASE	      = 0x40013800
        , USART2_BASE      = 0x40004400
        , USART3_BASE      = 0x40004800
    };
    
#ifdef __cplusplus    
    enum SPI_BASE : uint32_t
#else
    enum SPI_BASE
#endif
    { // p51, Table 3
        SPI1_BASE	    = 0x40013000 
        , SPI2_BASE     = 0x40003800
        , SPI3_BASE	    = 0x40003c00
    };

#ifdef __cplusplus    
    enum I2C_BASE : uint32_t
#else
    enum I2C_BASE
#endif
    {  // p51, Table 3
        I2C1_BASE	    = 0x40005400  // 0x4000 5400 - 0x4000 57FF, 
        , I2C2_BASE	    = 0x40005800  // 0x4000 5800 - 0x4000 5BFF
    };

#ifdef __cplusplus    
    enum DMA_BASE : uint32_t
#else
    enum DMA_BASE
#endif
    {  // p49, Table 3
        DMA1_BASE	    = 0x40020000  // 0x4002 0000 - 0x4002 03FF
        , DMA2_BASE	    = 0x40020400  // 0x4002 0400 - 0x4002 07FF
    };

#ifdef __cplusplus    
    enum TIM_BASE : uint32_t
#else
    enum TIM_BASE
#endif
    {  // p49, Table 3
        TIM2_BASE     = 0x40000000 //- 0x4000 03FF TIM2 timer; 15.4.19 page 422
        , TIM3_BASE   = 0x40000400 //- 0x4000 07FF TIM3 timer
        , TIM4_BASE   = 0x40000800 //- 0x4000 0BFF TIM4 timer
        , TIM5_BASE   = 0x40000C00 //- 0x4000 0FFF TIM5 timer
        , TIM6_BASE   = 0x40001000 //- 0x4000 13FF TIM6 timer
        , TIM7_BASE   = 0x40001400 //- 0x4000 17FF TIM7 timer
        , TIM12_BASE  = 0x40001800 //- 0x4000 1BFF TIM12 timer
        , TIM13_BASE  = 0x40001C00 //- 0x4000 1FFF TIM13 timer
        // 0x4001 2C00 - 0x4001 2FFF TIM1 timer Section 14.4.21 on page 362
        // 0x4001 3400 - 0x4001 37FF TIM8 timer Section 14.4.21 on page 362
        // 0x4001 5000 - 0x4001 53FF TIM10 timer Section 16.5.11 on page 467
        // 0x4001 5400 - 0x4001 57FF TIM11 timer Section 16.5.11 on page 467
    };

    enum GPIO_CNF {
        GPIO_CNF_OUTPUT_PUSH_PULL = 0
        , GPIO_CNF_OUTPUT_ODRAIN  = 1
        , GPIO_CNF_ALT_OUTPUT_PUSH_PULL = 2
        , GPIO_CNF_ALT_OUTPUT_ODRAIN = 3
        , GPIO_CNF_INPUT_ANALOG = 0
        , GPIO_CNF_INPUT_FLOATING = 1
        , GPIO_CNF_INPUT_PUSH_PULL = 2
    };

    enum GPIO_MODE {
        GPIO_MODE_INPUT        = 0
        , GPIO_MODE_OUTPUT_10M = 1
        , GPIO_MODE_OUTPUT_2M  = 2
        , GPIO_MODE_OUTPUT_50M  = 3
    };

    typedef struct ADC {
        uint32_t SR;        /* Address offset: 0x00 */
        uint32_t CR1;       /* Address offset: 0x04 */
        uint32_t CR2;       /* Address offset: 0x08 */
        uint32_t SMPR1;     /* Address offset: 0x0C */
        uint32_t SMPR2;     /* Address offset: 0x10 */
        uint32_t JOFR1;     /* Address offset: 0x14 */
        uint32_t JOFR2;     /* Address offset: 0x18 */
        uint32_t JOFR3;     /* Address offset: 0x1C */
        uint32_t JOFR4;     /* Address offset: 0x20 */
        uint32_t HTR;       /* Address offset: 0x24 */
        uint32_t LTR;       /* Address offset: 0x28 */
        uint32_t SQR1;      /* Address offset: 0x2C */
        uint32_t SQR2;      /* Address offset: 0x30 */
        uint32_t SQR3;      /* Address offset: 0x34 */
        uint32_t JSQR;      /* Address offset: 0x38 */
        uint32_t JDR1;      /* Address offset: 0x3C */
        uint32_t JDR2;      /* Address offset: 0x40 */
        uint32_t JDR3;      /* Address offset: 0x44 */
        uint32_t JDR4;      /* Address offset: 0x48 */
        uint32_t DR;        /* Address offset: 0x4C */
    } ADC_type;

    typedef struct AFIO {
        uint32_t EVCR;      /* Address offset: 0x00 */
        uint32_t MAPR;      /* Address offset: 0x04 */
        uint32_t EXTICR1;   /* Address offset: 0x08 */
        uint32_t EXTICR2;   /* Address offset: 0x0C */
        uint32_t EXTICR3;   /* Address offset: 0x10 */
        uint32_t EXTICR4;   /* Address offset: 0x14 */
        uint32_t MAPR2;     /* Address offset: 0x18 */
    } AFIO_type;

    // p674, RM0008, p695 Table 181
    struct CAN_TxMailBox {
        uint32_t TIR;
        uint32_t TDTR;
        uint32_t TDLR;
        uint32_t TDHR;        
    };
    struct  CAN_FIFOMailBox {
        uint32_t RIR;
        uint32_t RDTR;
        uint32_t RDLR;
        uint32_t RDHR;
    };
    struct  CAN_FilterRegister {
        uint32_t FR1;
        uint32_t FR2;
    };

    typedef struct CAN {
        uint32_t MCR;
        uint32_t MSR;
        uint32_t TSR;
        uint32_t RF0R;
        uint32_t RF1R;
        uint32_t IER;   // interrupt enable register
        uint32_t ESR;   // error status register
        uint32_t BTR;   // bit timing register  0x01c
        uint32_t Reserved0[ 0x58 ];
        struct CAN_TxMailBox txMailBox[ 3 ];
        struct CAN_FIFOMailBox fifoMailBox[ 3 ];
        uint32_t Reserved1[ 12 ];
        uint32_t FMR;
        uint32_t FM1R;
        uint32_t Reserved2;
        uint32_t FS1R;
        uint32_t Reserved3;
        uint32_t FFA1R;
        uint32_t Reserved4;
        uint32_t FA1R;
        uint32_t Reserved5[ 8 ];
        struct CAN_FilterRegister filterRegister[ 14 ];
    } CAN_type;

    typedef struct FLASH {
        uint32_t ACR;
        uint32_t KEYR;
        uint32_t OPTKEYR;
        uint32_t SR;
        uint32_t CR;
        uint32_t AR;
        uint32_t RESERVED;
        uint32_t OBR;
        uint32_t WRPR;
    } FLASH_type;

    typedef struct GPIO {
        uint32_t CRL;      /* GPIO port configuration register low,      Address offset: 0x00 */
        uint32_t CRH;      /* GPIO port configuration register high,     Address offset: 0x04 */
        uint32_t IDR;      /* GPIO port input data register,             Address offset: 0x08 */
        uint32_t ODR;      /* GPIO port output data register,            Address offset: 0x0C */
        uint32_t BSRR;     /* GPIO port bit set/reset register,          Address offset: 0x10 */
        uint32_t BRR;      /* GPIO port bit reset register,              Address offset: 0x14 */
        uint32_t LCKR;     /* GPIO port configuration lock register,     Address offset: 0x18 */
    } GPIO_type;

    typedef struct NVIC {
        uint32_t   ISER[8];     /* Address offset: 0x000 - 0x01C */
        uint32_t  RES0[24];     /* Address offset: 0x020 - 0x07C */
        uint32_t   ICER[8];     /* Address offset: 0x080 - 0x09C */
        uint32_t  RES1[24];     /* Address offset: 0x0A0 - 0x0FC */
        uint32_t   ISPR[8];     /* Address offset: 0x100 - 0x11C */
        uint32_t  RES2[24];     /* Address offset: 0x120 - 0x17C */
        uint32_t   ICPR[8];     /* Address offset: 0x180 - 0x19C */
        uint32_t  RES3[24];     /* Address offset: 0x1A0 - 0x1FC */
        uint32_t   IABR[8];     /* Address offset: 0x200 - 0x21C */
        uint32_t  RES4[56];     /* Address offset: 0x220 - 0x2FC */
        uint8_t   IPR[240];     /* Address offset: 0x300 - 0x3EC */
        uint32_t RES5[644];     /* Address offset: 0x3F0 - 0xEFC */
        uint32_t       STIR;    /* Address offset:         0xF00 */
    } NVIC_type;

    typedef struct  RCC {
        uint32_t CR;       /* RCC clock control register,                Address offset: 0x00 */
        uint32_t CFGR;     /* RCC clock configuration register,          Address offset: 0x04 */
        uint32_t CIR;      /* RCC clock interrupt register,              Address offset: 0x08 */
        uint32_t APB2RSTR; /* RCC APB2 peripheral reset register,        Address offset: 0x0C */
        uint32_t APB1RSTR; /* RCC APB1 peripheral reset register,        Address offset: 0x10 */
        uint32_t AHBENR;   /* RCC AHB peripheral clock enable register,  Address offset: 0x14 */
        uint32_t APB2ENR;  /* RCC APB2 peripheral clock enable register, Address offset: 0x18 */
        uint32_t APB1ENR;  /* RCC APB1 peripheral clock enable register, Address offset: 0x1C */
        uint32_t BDCR;     /* RCC backup domain control register,        Address offset: 0x20 */
        uint32_t CSR;      /* RCC control/status register,               Address offset: 0x24 */
        uint32_t AHBRSTR;  /* RCC AHB peripheral clock reset register,   Address offset: 0x28 */
        uint32_t CFGR2;    /* RCC clock configuration register 2,        Address offset: 0x2C */
    } RCC_type;
    
    typedef struct STK {
        uint32_t CSR;      /* SYSTICK control and status register,       Address offset: 0x00 */
        uint32_t RVR;      /* SYSTICK reload value register,             Address offset: 0x04 */
        uint32_t CVR;      /* SYSTICK current value register,            Address offset: 0x08 */
        uint32_t CALIB;    /* SYSTICK calibration value register,        Address offset: 0x0C */
    } STK_type;


    // each I/O port registers have to be accessed as 32bit words. (reference manual pp158/1133)
    // p751 register map
    typedef struct SPI {
        uint32_t CR1;	    // 0
        uint32_t CR2;	    // 4
        uint32_t SR;        // 8
        uint32_t DATA;      // c
        uint32_t CRCPR;
        uint32_t RXCRCR;
        uint32_t TXCRCR;
        uint32_t I2SCFGR;   // 0x1c {b11 (0=SPI, 1=I2S)}
        uint32_t SPI_I2PR;
    } SPI_type;

    typedef struct USART {
        uint32_t SR;       /* Address offset: 0x00 */
        uint32_t DR;       /* Address offset: 0x04 */
        uint32_t BRR;      /* Address offset: 0x08 */
        uint32_t CR1;      /* Address offset: 0x0C */
        uint32_t CR2;      /* Address offset: 0x10 */
        uint32_t CR3;      /* Address offset: 0x14 */
        uint32_t GTPR;     /* Address offset: 0x18 */
    } USART_type;

    /*
     * STM32F107 Interrupt Number Definition
     */
    typedef enum IRQn {
        NonMaskableInt_IRQn         = -14,    /* 2 Non Maskable Interrupt                             */
        MemoryManagement_IRQn       = -12,    /* 4 Cortex-M3 Memory Management Interrupt              */
        BusFault_IRQn               = -11,    /* 5 Cortex-M3 Bus Fault Interrupt                      */
        UsageFault_IRQn             = -10,    /* 6 Cortex-M3 Usage Fault Interrupt                    */
        SVCall_IRQn                 = -5,     /* 11 Cortex-M3 SV Call Interrupt                       */
        DebugMonitor_IRQn           = -4,     /* 12 Cortex-M3 Debug Monitor Interrupt                 */
        PendSV_IRQn                 = -2,     /* 14 Cortex-M3 Pend SV Interrupt                       */
        SysTick_IRQn                = -1,     /* 15 Cortex-M3 System Tick Interrupt                   */
        WWDG_IRQn                   = 0,      /* Window WatchDog Interrupt                            */
        PVD_IRQn                    = 1,      /* PVD through EXTI Line detection Interrupt            */
        TAMPER_IRQn                 = 2,      /* Tamper Interrupt                                     */
        RTC_IRQn                    = 3,      /* RTC global Interrupt                                 */
        FLASH_IRQn                  = 4,      /* FLASH global Interrupt                               */
        RCC_IRQn                    = 5,      /* RCC global Interrupt                                 */
        EXTI0_IRQn                  = 6,      /* EXTI Line0 Interrupt                                 */
        EXTI1_IRQn                  = 7,      /* EXTI Line1 Interrupt                                 */
        EXTI2_IRQn                  = 8,      /* EXTI Line2 Interrupt                                 */
        EXTI3_IRQn                  = 9,      /* EXTI Line3 Interrupt                                 */
        EXTI4_IRQn                  = 10,     /* EXTI Line4 Interrupt                                 */
        DMA1_Channel1_IRQn          = 11,     /* DMA1 Channel 1 global Interrupt                      */
        DMA1_Channel2_IRQn          = 12,     /* DMA1 Channel 2 global Interrupt                      */
        DMA1_Channel3_IRQn          = 13,     /* DMA1 Channel 3 global Interrupt                      */
        DMA1_Channel4_IRQn          = 14,     /* DMA1 Channel 4 global Interrupt                      */
        DMA1_Channel5_IRQn          = 15,     /* DMA1 Channel 5 global Interrupt                      */
        DMA1_Channel6_IRQn          = 16,     /* DMA1 Channel 6 global Interrupt                      */
        DMA1_Channel7_IRQn          = 17,     /* DMA1 Channel 7 global Interrupt                      */
        ADC1_2_IRQn                 = 18,     /* ADC1 and ADC2 global Interrupt                       */
        CAN1_TX_IRQn                = 19,     /* USB Device High Priority or CAN1 TX Interrupts       */
        CAN1_RX0_IRQn               = 20,     /* USB Device Low Priority or CAN1 RX0 Interrupts       */
        CAN1_RX1_IRQn               = 21,     /* CAN1 RX1 Interrupt                                   */
        CAN1_SCE_IRQn               = 22,     /* CAN1 SCE Interrupt                                   */
        EXTI9_5_IRQn                = 23,     /* External Line[9:5] Interrupts                        */
        TIM1_BRK_IRQn               = 24,     /* TIM1 Break Interrupt                                 */
        TIM1_UP_IRQn                = 25,     /* TIM1 Update Interrupt                                */
        TIM1_TRG_COM_IRQn           = 26,     /* TIM1 Trigger and Commutation Interrupt               */
        TIM1_CC_IRQn                = 27,     /* TIM1 Capture Compare Interrupt                       */
        TIM2_IRQn                   = 28,     /* TIM2 global Interrupt                                */
        TIM3_IRQn                   = 29,     /* TIM3 global Interrupt                                */
        TIM4_IRQn                   = 30,     /* TIM4 global Interrupt                                */
        I2C1_EV_IRQn                = 31,     /* I2C1 Event Interrupt                                 */
        I2C1_ER_IRQn                = 32,     /* I2C1 Error Interrupt                                 */
        I2C2_EV_IRQn                = 33,     /* I2C2 Event Interrupt                                 */
        I2C2_ER_IRQn                = 34,     /* I2C2 Error Interrupt                                 */
        SPI1_IRQn                   = 35,     /* SPI1 global Interrupt                                */
        SPI2_IRQn                   = 36,     /* SPI2 global Interrupt                                */
        USART1_IRQn                 = 37,     /* USART1 global Interrupt                              */
        USART2_IRQn                 = 38,     /* USART2 global Interrupt                              */
        USART3_IRQn                 = 39,     /* USART3 global Interrupt                              */
        EXTI15_10_IRQn              = 40,     /* External Line[15:10] Interrupts                      */
        RTCAlarm_IRQn               = 41,     /* RTC Alarm through EXTI Line Interrupt                */
        OTG_FS_WKUP_IRQn            = 42,     /* USB OTG FS WakeUp from suspend through EXTI Line Int */
        TIM5_IRQn                   = 50,     /* TIM5 global Interrupt                                */
        SPI3_IRQn                   = 51,     /* SPI3 global Interrupt                                */
        UART4_IRQn                  = 52,     /* UART4 global Interrupt                               */
        UART5_IRQn                  = 53,     /* UART5 global Interrupt                               */
        TIM6_IRQn                   = 54,     /* TIM6 global Interrupt                                */
        TIM7_IRQn                   = 55,     /* TIM7 global Interrupt                                */
        DMA2_Channel1_IRQn          = 56,     /* DMA2 Channel 1 global Interrupt                      */
        DMA2_Channel2_IRQn          = 57,     /* DMA2 Channel 2 global Interrupt                      */
        DMA2_Channel3_IRQn          = 58,     /* DMA2 Channel 3 global Interrupt                      */
        DMA2_Channel4_IRQn          = 59,     /* DMA2 Channel 4 global Interrupt                      */
        DMA2_Channel5_IRQn          = 60,     /* DMA2 Channel 5 global Interrupt                      */
        ETH_IRQn                    = 61,     /* Ethernet global Interrupt                            */
        ETH_WKUP_IRQn               = 62,     /* Ethernet Wakeup through EXTI line Interrupt          */
        CAN2_TX_IRQn                = 63,     /* CAN2 TX Interrupt                                    */
        CAN2_RX0_IRQn               = 64,     /* CAN2 RX0 Interrupt                                   */
        CAN2_RX1_IRQn               = 65,     /* CAN2 RX1 Interrupt                                   */
        CAN2_SCE_IRQn               = 66,     /* CAN2 SCE Interrupt                                   */
        OTG_FS_IRQn                 = 67      /* USB OTG FS global Interrupt                          */
    } IRQn_type;

    enum GPIOA_PIN {
        PA0 = 0   // ADC0, CTS2, T2C1E, WKUP
        , PA1     // ADC1, RTS2, T2C2
        , PA2     // ADC2, TX2,  T2C3
        , PA3     // ADC3
        , PA4     // ADC4
        , PA5     // ADC5
        , PA6     // ADC6
        , PA7     // ADC7
        , PA8     // 
        , PA9     // 
        , PA10
        , PA11
        , PA12
        , PA13
        , PA14
        , PA15
    };
    enum GPIOB_PIN {
        PB0 = 0 // ADC8, T3C3
        , PB1     // ADC9, T3C4
        , PB2
        , PB3
        , PB4
        , PB5
        , PB6
        , PB7
        , PB8
        , PB9
        , PB10   // SCL2, RX3
        , PB11   // SDA2, TX3
        , PB12
        , PB13
        , PB14
        , PB15
    };
    enum GPIOC_PIN {
        PC0 = 0
        , PC1
        , PC2
        , PC3
        , PC4
        , PC5
        , PC6
        , PC7
        , PC8
        , PC9
        , PC10
        , PC11
        , PC12
        , PC13
        , PC14
        , PC15
    };

#ifdef __cplusplus    
}
#endif
