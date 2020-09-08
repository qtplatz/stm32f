/* shell.c
 * Copyright (C) MS-Cheminformatics LLC
 * Author: Toshinobu Hondo, Ph.D., 
 *
 * Project was started with a reference project from 
 * https://github.com/trebisky/stm32f103
 */

#include "adc.hpp"
#include "can.hpp"
#include "command_processor.hpp"
#include "system_clock.hpp"
#include "dma.hpp"
#include "gpio.hpp"
#include "gpio_mode.hpp"
#include "i2c.hpp"
#include "rcc.hpp"
#include "rtc.hpp"
#include "spi.hpp"
#include "stm32f103.hpp"
#include "stream.hpp"
#include "tokenizer.hpp"
#include "uart.hpp"
#include <array>
#include <atomic>
#include <algorithm>
#include <cstddef>
#include <utility>
extern "C" {
#include "../common/fdlibm.h"
}

extern uint64_t jiffies;  // 100us
extern uint32_t __bss_start, __bss_end;
extern uint32_t __data_start, __data_end;

uint32_t __system_clock;
uint32_t __pclk1, __pclk2;
stm32f103::system_clock::time_point __uptime;

std::atomic< uint32_t > atomic_jiffies;          //  100us  (4.97 days)
std::atomic< uint32_t > atomic_milliseconds;     // 1000us  (49.71 days)
std::atomic< uint32_t > atomic_250_milliseconds;
std::atomic< uint32_t > atomic_seconds;          // 1s      (136.1925 years)

extern void uart1_handler();

extern "C" {
    void enable_interrupt( stm32f103::IRQn_type IRQn );
    void disable_interrupt( stm32f103::IRQn_type IRQn );

    void serial_puts( const char * s );
    void serial_putc( int );

    void systick_handler();

    void * memset( void * ptr, int value, size_t num );
    
    void __dma1_ch1_handler( void );
    void __dma1_ch2_handler( void );
    void __dma1_ch3_handler( void );
    void __dma1_ch4_handler( void );
    void __dma1_ch5_handler( void );
    void __dma1_ch6_handler( void );
    void __dma1_ch7_handler( void );
    void __dma2_ch1_handler( void );
    void __dma2_ch2_handler( void );
    void __dma2_ch3_handler( void );
    void __dma2_ch4_handler( void );
    void __dma2_ch5_handler( void );

    void __adc1_handler( void );
    void __i2c1_event_handler( void );
    void __i2c1_error_handler( void );
    void __i2c2_event_handler( void );
    void __i2c2_error_handler( void );
    void __spi1_handler( void );
    void __spi2_handler( void );
    void __usart1_handler( void );
    void __systick_handler( void );
    void __rcc_handler( void );

    void __hard_fault( void );
    void __bus_fault( void );
    void __usage_fault( void );
    
    int main();
}

extern "C" {
    double __ieee754_log(double x);
    double __ieee754_log10( double );
}

void
mdelay ( uint32_t ms )
{
    auto t0 = atomic_jiffies.load();
	while ( atomic_jiffies.load() - t0 < (10*ms) )
	    ;
}

/*
 * Initialize SysTick Timer
 *
 * Since it is set up to run at 1Mhz, an s value of
 * 1Khz needed to make 1 millisecond timer
 */
void
init_systick( uint32_t s, bool en )
{
    if ( auto SYSTICK = reinterpret_cast< volatile stm32f103::STK * >( stm32f103::SYSTICK_BASE ) ) {
        // See PM0056, DockID15491 Rev 6 - page 151 (Not in the RM0008 document) 

        SYSTICK->CSR |= 4; // processor clock (72MHz)
        //SYSTICK->CSR &= (~4) & 0x10007; // AHB
        
        // Enable exception
        SYSTICK->CSR |= ( en ? 1 : 0 ) << 1;

        // Load the reload value
        SYSTICK->RVR = s;

        // Set the current value to 0
        SYSTICK->CVR = 0;

        // Enable SysTick
        SYSTICK->CSR |= 1;
    }
}

int
main()
{
    memset( &__bss_start, 0, reinterpret_cast< const char * >(&__bss_end) - reinterpret_cast< const char * >(&__bss_start) + 1);

    if ( auto RCC = reinterpret_cast< volatile stm32f103::RCC * >( stm32f103::RCC_BASE ) ) {
        // clock/pll setup -->
        // RM0008 p98- 
        if ( auto FLASH = reinterpret_cast< volatile stm32f103::FLASH * >( stm32f103::FLASH_BASE ) )
            FLASH->ACR = 0x0010 | 0x0002; // FLASH_PREFETCH (enable prefetch buffer)| FLASH_WAIT2(for 48 < sysclk <= 72MHz);

        RCC->CFGR = ( 7 << 18 ) | ( 1 << 16 ) | ( 4 << 8 );           // PLLMUL(input x9), PLLSRC(HSE), PPRE1(HCLK/2)
        RCC->CR   = ( 1 << 24 ) | ( 1 << 16 ) | (0b10000 << 3) | 1;   // PLLON, HSEON, HSITRIM(0b10000), HSION
        while ( ! RCC->CR & ( 1 << 1 ) )                             // Wait HSI RDY
            ;
        while ( ! RCC->CR & ( 1 << 17 ) )                             // Wait HSE RDY
            ;
        while ( ! RCC->CR & ( 1 << 25 ) )                             // Wait PLL RDY
            ;        
        RCC->CFGR |= 0x02;                      // SW(0b10, pll selected as system clock)

        // RCC->CIR = (3 << 8); // HSI, LSE, LSI
        // enable_interrupt( stm32f103::RCC_IRQn );

        // pclk1 (APB low speed clock) should be 720000 / 2  := 32000000Hz
        // pclk2 (APB high-speed clock) is HCLK not divided, := 72000000Hz
        // HSE := high speed clock signal
        // HSI := internal 8MHz RC Oscillator clock
        __system_clock = 72000000;
        __pclk2 = __system_clock;
        __pclk1 = __system_clock / 2;

        // ADC prescaler
        RCC->CFGR &= ~( 0b11 << 14 );
        RCC->CFGR |= ( 0b10 << 14 );  // set prescaler to 6
        // <--

        // DMA
        RCC->AHBENR |= 0x01; // DMA1 enable
        // 
        ///////////////////////////////////////////////////////
    }

    if ( auto RCC = reinterpret_cast< volatile stm32f103::RCC * >( stm32f103::RCC_BASE ) ) {
        // See RM0008 section 7.3.7 p111-112 (DocID 13902, Rev. 17) APB2 peripheral clock enable register
        RCC->APB2ENR |= 0x0001;     // AFIO enable
        
        RCC->APB2ENR |= 0x0004;     // IOPA EN := GPIO A enable
        RCC->APB2ENR |= 0x0008;     // IOPB EN := GPIO B enable
        RCC->APB2ENR |= 0x0010;     // IOPC EN := GPIO C enable

        RCC->APB2ENR |= (01 << 9);    // ADC1

        RCC->APB2ENR |= (01 << 12);   // SPI1 enable;

        RCC->APB1ENR |= 1 << 14;      // SPI2 (based on PCLK1, not equal to SPI1)

        RCC->APB2ENR |= (01 << 14);   // UART1 enable;

        // 7.3.8 p114 (APB1 peripheral clock enable register)
        RCC->APB1ENR |= (1 << 25); // CAN clock enable

        // I2C clock enable
        RCC->APB1ENR |= (1 << 21); // I2C 1 clock enable
        RCC->APB1ENR |= (1 << 22); // I2C 2 clock enable

        // TIM2
        RCC->APB1ENR |= 0x01;  // TIM2 General purpose timer (uing in bmp280)
        // TIM3
        RCC->APB1ENR |= 0x02;  // TIM3 General purpose timer (uing in bmp280)
        //RCC->APB1ENR |= 0x3e;  // TIM3..TIM7 General purpose timer (calibration trial)
    }

    atomic_jiffies = 0;
    atomic_milliseconds = 0;
    atomic_seconds = 0;

    // enable serial console
    stm32f103::uart_t< stm32f103::USART1_BASE >::instance()->enable( stm32f103::PA9, stm32f103::PA10 );
    
    // enable RTC
    stm32f103::rtc::instance()->enable();
    
    __uptime = stm32f103::system_clock::now();
    
    {
        stm32f103::gpio_mode gpio_mode;
        
        // ADC  (0)
        gpio_mode( stm32f103::PA0, stm32f103::GPIO_CNF_INPUT_ANALOG,         stm32f103::GPIO_MODE_INPUT ); // ADC1 (0,0)
        gpio_mode( stm32f103::PA1, stm32f103::GPIO_CNF_INPUT_ANALOG,         stm32f103::GPIO_MODE_INPUT ); // ADC2
        gpio_mode( stm32f103::PA2, stm32f103::GPIO_CNF_INPUT_ANALOG,         stm32f103::GPIO_MODE_INPUT ); // ADC3
        gpio_mode( stm32f103::PA3, stm32f103::GPIO_CNF_INPUT_ANALOG,         stm32f103::GPIO_MODE_INPUT ); // ADC4
        
        // SPI
        // (see RM0008, p166, Table 25)
        gpio_mode( stm32f103::PA4, stm32f103::GPIO_CNF_ALT_OUTPUT_PUSH_PULL, stm32f103::GPIO_MODE_OUTPUT_50M ); // ~SS
        gpio_mode( stm32f103::PA5, stm32f103::GPIO_CNF_ALT_OUTPUT_PUSH_PULL, stm32f103::GPIO_MODE_OUTPUT_50M ); // SCLK
        gpio_mode( stm32f103::PA6, stm32f103::GPIO_CNF_INPUT_FLOATING,       stm32f103::GPIO_MODE_INPUT );      // MISO
        gpio_mode( stm32f103::PA7, stm32f103::GPIO_CNF_ALT_OUTPUT_PUSH_PULL, stm32f103::GPIO_MODE_OUTPUT_50M ); // MOSI

        // I2C1
        // (see RM0008, p180, Table 55)
        // I2C ALT function  REMAP=0 { SCL,SDA } = { PB6, PB7 }, REMAP=1 { PB8, PB9 }
        // GPIO config in p167, Table 27
        gpio_mode( stm32f103::PB6, stm32f103::GPIO_CNF_ALT_OUTPUT_ODRAIN,  stm32f103::GPIO_MODE_OUTPUT_2M ); // SCL
        gpio_mode( stm32f103::PB7, stm32f103::GPIO_CNF_ALT_OUTPUT_ODRAIN,  stm32f103::GPIO_MODE_OUTPUT_2M ); // SDA

        // I2C2
        gpio_mode( stm32f103::PB10, stm32f103::GPIO_CNF_ALT_OUTPUT_ODRAIN, stm32f103::GPIO_MODE_OUTPUT_2M ); // SCL
        gpio_mode( stm32f103::PB11, stm32f103::GPIO_CNF_ALT_OUTPUT_ODRAIN, stm32f103::GPIO_MODE_OUTPUT_2M ); // SDA
        
        // CAN
        // (see RM0008, p167, Table 28)
        //gpio_mode( stm32f103::PA11, stm32f103::GPIO_CNF_INPUT_FLOATING,      stm32f103::GPIO_MODE_INPUT );   // CAN1_RX
        //gpio_mode( stm32f103::PA12, stm32f103::GPIO_CNF_ALT_OUTPUT_PUSH_PULL, stm32f103::GPIO_MODE_OUTPUT_50M ); // CAN1_TX
        if ( auto AFIO = reinterpret_cast< volatile stm32f103::AFIO * >( stm32f103::AFIO_BASE ) )
            AFIO->MAPR |= 0b10 << 13;  // CAN_RX = PB8, TX = PB9
        gpio_mode( stm32f103::PB8, stm32f103::GPIO_CNF_INPUT_FLOATING,       stm32f103::GPIO_MODE_INPUT );   // CAN1_RX
        gpio_mode( stm32f103::PB9, stm32f103::GPIO_CNF_ALT_OUTPUT_PUSH_PULL, stm32f103::GPIO_MODE_OUTPUT_50M ); // CAN1_TX
        
        // LED
        gpio_mode( stm32f103::PC13, stm32f103::GPIO_CNF_OUTPUT_ODRAIN, stm32f103::GPIO_MODE_OUTPUT_2M );
    }
    
    {
        uint32_t bsize = reinterpret_cast< const char * >(&__bss_end) - reinterpret_cast< const char * >(&__bss_start);
        uint32_t dsize = reinterpret_cast< const char * >(&__data_end) - reinterpret_cast< const char * >(&__data_start);
        __system_clock = stm32f103::rcc().system_frequency();
        __pclk1 = stm32f103::rcc().pclk1(); // 72MHz
        __pclk2 = stm32f103::rcc().pclk2(); // 36MHz
        stream() << "\n\t****************************************************************" << std::endl;
        stream() << "\t***** .DATA 0x" << uint32_t(&__data_start) << " -- 0x" << uint32_t(&__data_end) << "\t0x" << dsize << "\t*****" << std::endl;
        stream() << "\t***** .BSS  0x" << uint32_t(&__bss_start)  << " -- 0x" << uint32_t(&__bss_end)  << "\t0x" << bsize << "\t*****" << std::endl;
        stream() << "\t***** Current stack pointer = " << uint32_t(&dsize) << "\t *****" << std::endl;
        stream() << "\t***** SYSCLK = " << int32_t( __system_clock ) << "Hz" << std::endl;
        stream() << "\t***** PCLK1  = " << int32_t( __pclk1 ) << "Hz" << std::endl;
        stream() << "\t***** PCLK2  = " << int32_t( __pclk2 ) << "Hz" << std::endl;
        stream() << "\t******************************************************************" << std::endl;
        stream() << "\t\ti2c-1 SCL = PB6; SDA = PB7;\tCAN RX = PB8; TX = PB9" << std::endl;
    }

    init_systick( 7200, true ); // 100us tick

    {
        int x = 0;

        std::array< char, 128 > cbuf;
        typedef tokenizer< 32 > tokenizer_type;

        tokenizer_type::argv_type argv;

        while ( true ) {
            stream() << "stm32f103 > ";
            auto length = stm32f103::uart::gets( cbuf.data(), cbuf.size() );
            auto argc = tokenizer_type()( cbuf.data(), argv );
            command_processor()( argc, argv.data() );
        }
    }

    return 0;
}

void
serial_puts( const char * s )
{
    (*stm32f103::uart_t< stm32f103::USART1_BASE >::instance()) << s;
}

void
serial_putc( int c )
{
    stm32f103::uart_t< stm32f103::USART1_BASE >::instance()->putc( c );
}

void
systick_handler()
{
    ++atomic_jiffies;
    
    auto tp = atomic_jiffies.load();

    if ( ( tp % 10 ) == 0 )
        ++atomic_milliseconds;

    if ( ( tp % 10000 ) == 0 )
        ++atomic_seconds;

    if ( ( tp % 10 * 250 ) == 0 )
        ++atomic_250_milliseconds;

    // systick (100us)
    // stm32f103::gpio< decltype( stm32f103::PB12 ) >( stm32f103::PB12 ) = bool( tp & 01 );

    // LED (200ms)
    if ( ( tp % ( 10 * 200 ) ) == 0 ) {
        static uint32_t blink = 0;
        stm32f103::gpio< decltype( stm32f103::PC13 ) >( stm32f103::PC13 ) = bool( blink++ & 01 );
    }
}

void __hard_fault( void )
{
    serial_puts( "\nHard fault\n" );
    while( true );
}

void __bus_fault( void )
{
    serial_puts( "\nBus fault\n" );
    while( true );    
}

void __usage_fault( void )
{
    serial_puts( "\nUsage fault\n" );
    while( true );
}

void
__adc1_handler(void)
{
    stm32f103::adc::interrupt_handler( stm32f103::adc::instance() );
}

void
__i2c1_event_handler()
{
    stm32f103::i2c_t< stm32f103::I2C1_BASE >::instance()->handle_event_interrupt();
}

void
__i2c1_error_handler()
{
    stm32f103::i2c_t< stm32f103::I2C1_BASE >::instance()->handle_error_interrupt();
}

void
__i2c2_event_handler()
{
    stm32f103::i2c_t< stm32f103::I2C2_BASE >::instance()->handle_event_interrupt();    
}

void
__i2c2_error_handler()
{
    stm32f103::i2c_t< stm32f103::I2C2_BASE >::instance()->handle_error_interrupt();    
}

void
__spi1_handler(void)
{
    stm32f103::spi::interrupt_handler( stm32f103::spi_t<stm32f103::SPI1_BASE>::instance() );    
}

void
__spi2_handler(void)
{
    stm32f103::spi::interrupt_handler( stm32f103::spi_t<stm32f103::SPI2_BASE>::instance() );
}

void
__usart1_handler(void)
{
    stm32f103::uart_t< stm32f103::USART1_BASE >::instance()->handle_interrupt();
}

void
__systick_handler( void )
{
    ++jiffies;
    systick_handler();
}

void
__dma1_ch1_handler( void )
{
    stm32f103::dma_t< stm32f103::DMA1_BASE >::instance()->handle_interrupt( 0 );
}

void
__dma1_ch2_handler( void )
{
    stm32f103::dma_t< stm32f103::DMA1_BASE >::instance()->handle_interrupt( 1 );
}

void
__dma1_ch3_handler( void )
{
    stm32f103::dma_t< stm32f103::DMA1_BASE >::instance()->handle_interrupt( 2 );
}

void
__dma1_ch4_handler( void )
{
    stm32f103::dma_t< stm32f103::DMA1_BASE >::instance()->handle_interrupt( 3 );
}

void
__dma1_ch5_handler( void )
{
    stm32f103::dma_t< stm32f103::DMA1_BASE >::instance()->handle_interrupt( 4 );
}

void
__dma1_ch6_handler( void )
{
    stm32f103::dma_t< stm32f103::DMA1_BASE >::instance()->handle_interrupt( 5 );
}

void
__dma1_ch7_handler( void )
{
    stm32f103::dma_t< stm32f103::DMA1_BASE >::instance()->handle_interrupt( 6 );
}

void
__dma2_ch1_handler( void )
{
    stm32f103::dma_t< stm32f103::DMA2_BASE >::instance()->handle_interrupt( 0 );
}

void
__dma2_ch2_handler( void )
{
    stm32f103::dma_t< stm32f103::DMA2_BASE >::instance()->handle_interrupt( 1 );
}

void
__dma2_ch3_handler( void )
{
    stm32f103::dma_t< stm32f103::DMA2_BASE >::instance()->handle_interrupt( 2 );
}

void
__dma2_ch4_handler( void )
{
    stm32f103::dma_t< stm32f103::DMA2_BASE >::instance()->handle_interrupt( 3 );
}

void
__dma2_ch5_handler( void )
{
    stm32f103::dma_t< stm32f103::DMA2_BASE >::instance()->handle_interrupt( 4 );
}

void
__rcc_handler( void )
{
    if ( auto RCC = reinterpret_cast< volatile stm32f103::RCC * >( stm32f103::RCC_BASE ) ) {
        uint32_t cir = RCC->CIR;
        stream(__FILE__,__LINE__,__FUNCTION__) << "##### RCC CIR: " << cir << std::endl;
        RCC->CIR &= ~((cir & 0x7f) << 8);
        stream(__FILE__,__LINE__,__FUNCTION__) << "##### RCC CIR: " << cir << std::endl;
    }
    disable_interrupt( stm32f103::RCC_IRQn );
}

namespace std {
    void __throw_out_of_range_fmt( char const *, ... ) { while( true ); }
};
