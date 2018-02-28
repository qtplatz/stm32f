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
#include "gpio.hpp"
#include "gpio_mode.hpp"
#include "spi.hpp"
#include "stream.hpp"
#include "tokenizer.hpp"
#include "uart.hpp"
#include "stm32f103.hpp"
#include <array>
#include <atomic>
#include <utility>
#include <cstddef>

extern uint64_t jiffies;  // 100us
extern uint32_t _sbss, _ebss;

std::atomic< uint32_t > atomic_jiffies;          //  100us  (4.97 days)
std::atomic< uint32_t > atomic_milliseconds;     // 1000us  (49.71 days)
std::atomic< uint32_t > atomic_250_milliseconds;
std::atomic< uint32_t > atomic_seconds;          // 1s      (136.1925 years)

static std::atomic_flag __lock_flag;

stm32f103::uart __uart0;
stm32f103::spi __spi0;
stm32f103::spi __spi1;
stm32f103::can __can0;
stm32f103::adc __adc0;

extern "C" {
    void enable_interrupt( stm32f103::IRQn_type IRQn );
    void disable_interrupt( stm32f103::IRQn_type IRQn );
    void __aeabi_unwind_cpp_pr0 ( int, int *, int * ) {}
    void __aeabi_unwind_cpp_pr1 ( int, int *, int * ) {}

    void serial_puts( const char * s );
    void serial_putc( int );

    void systick_handler();
    void adc1_handler();
    void spi1_handler();
    void spi2_handler();

    void * memset( void * ptr, int value, size_t num );

    int main();
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
    // zero clear .bss
    memset( &_sbss, 0, reinterpret_cast< const char * >(&_ebss) - reinterpret_cast< const char * >(&_sbss) );

    if ( auto RCC = reinterpret_cast< volatile stm32f103::RCC * >( stm32f103::RCC_BASE ) ) {
        // clock/pll setup -->
        // RM0008 p98- 
        if ( auto FLASH = reinterpret_cast< volatile stm32f103::FLASH * >( stm32f103::FLASH_BASE ) )
            FLASH->ACR = 0x0010 | 0x0002; // FLASH_PREFETCH (enable prefetch buffer)| FLASH_WAIT2(for 48 < sysclk <= 72MHz);
        
        RCC->CFGR = ( 7 << 18 ) | ( 1 << 16 ) | ( 4 << 8 );           // pll input clock(9), PLLSRC=HSE, PPRE1=4
        RCC->CR   = ( 1 << 24 ) | ( 1 << 16 ) | (0b10000 << 3) | 1;   // PLLON, HSEON, HSITRIM(0b10000), HSION
        while ( ! RCC->CR & ( 1 << 17 ) )                             // Wait until HSE settles down (HSE RDY)
            ;
        RCC->CFGR |= 0x02;                      // SW(0b10, pll selected as system clock)
        // <--
        ///////////////////////////////////////////////////////
    }

    if ( auto RCC = reinterpret_cast< volatile stm32f103::RCC * >( stm32f103::RCC_BASE ) ) {
        // See RM0008 section 7.3.7 p111-112 (DocID 13902, Rev. 17) APB2 peripheral clock enable register
        RCC->APB2ENR |= 0x0001;     // AFIO enable
        
        RCC->APB2ENR |= 0x0004;     // IOPA EN := GPIO A enable
        RCC->APB2ENR |= 0x0008;     // IOPB EN := GPIO B enable
        RCC->APB2ENR |= 0x0010;     // IOPC EN := GPIO C enable

        RCC->APB2ENR |= (01 << 9);    // ADC1
        // RCC->APB2ENR |= (1 << 11); // TIM1

        RCC->APB2ENR |= (01 << 12);   // SPI1 enable;

        // RCC->APB2ENR |= (1 << 13); // TIM8
        
        RCC->APB2ENR |= (01 << 14);   // UART1 enable;

        // 7.3.8 p114 (APB1 peripheral clock enable register)
        // RCC->APB1ENR |= 0b111111;   // TIM 2..7
        // RCC->APB1ENR |= 07 << 6;    // TIM 12,13,14
        // RCC->APB1ENR |= 1 << 11;    // WWD GEN
        // RCC->APB1ENR |= 1 << 14;    // SPI2
        // RCC->APB1ENR |= 1 << 15;    // SPI3
        // RCC->APB1ENR |= 0x0f << 17; // USART 2..5
        // RCC->APB1ENR |= 03 << 21;   // I2C 1,2
        // RCC->APB1ENR |= 01 << 23;   // USB

        RCC->APB1ENR |= (1 << 25); // CAN clock enable

        // ADC prescaler
        RCC->CFGR &= ~( 0b11 << 14 );
        RCC->CFGR |= ( 0b10 << 14 );  // set prescaler to 6
    }

    atomic_jiffies = 0;
    atomic_milliseconds = 0;
    atomic_seconds = 0;

    {
        stm32f103::gpio_mode gpio_mode;
        
        // initialize UART0 for debug output
        gpio_mode( stm32f103::PA9, stm32f103::GPIO_CNF_ALT_OUTPUT_PUSH_PULL, stm32f103::GPIO_MODE_OUTPUT_50M ); // (2,3)
        gpio_mode( stm32f103::PA10, stm32f103::GPIO_CNF_INPUT_PUSH_PULL,     stm32f103::GPIO_MODE_INPUT );
        __uart0.init( stm32f103::USART1_BASE, stm32f103::uart::parity_even, 8, 115200, 72000000 );

        // ADC  (0)
        gpio_mode( stm32f103::PA0, stm32f103::GPIO_CNF_INPUT_ANALOG,         stm32f103::GPIO_MODE_INPUT ); // ADC1 (0,0)
        //gpio_mode( stm32f103::PA1, stm32f103::GPIO_CNF_ALT_OUTPUT_PUSH_PULL, stm32f103::GPIO_MODE_INPUT ); // ADC2 (0,0)
        //gpio_mode( stm32f103::PA2, stm32f103::GPIO_CNF_INPUT_FLOATING,       stm32f103::GPIO_MODE_INPUT ); // ADC3 (0,0)
        //gpio_mode( stm32f103::PA3, stm32f103::GPIO_CNF_ALT_OUTPUT_PUSH_PULL, stm32f103::GPIO_MODE_INPUT ); // ADC4 (0,0)
        
        // SPI
        // (see RM0008, p166, Table 25)
        gpio_mode( stm32f103::PA4, stm32f103::GPIO_CNF_ALT_OUTPUT_PUSH_PULL, stm32f103::GPIO_MODE_OUTPUT_50M ); // ~SS
        gpio_mode( stm32f103::PA5, stm32f103::GPIO_CNF_ALT_OUTPUT_PUSH_PULL, stm32f103::GPIO_MODE_OUTPUT_50M ); // SCLK
        gpio_mode( stm32f103::PA6, stm32f103::GPIO_CNF_INPUT_FLOATING,       stm32f103::GPIO_MODE_INPUT );      // MISO
        gpio_mode( stm32f103::PA7, stm32f103::GPIO_CNF_ALT_OUTPUT_PUSH_PULL, stm32f103::GPIO_MODE_OUTPUT_50M ); // MOSI
        
        // CAN
        gpio_mode( stm32f103::PA11, stm32f103::GPIO_CNF_INPUT_PUSH_PULL,      stm32f103::GPIO_MODE_INPUT );   // CAN1_RX
        gpio_mode( stm32f103::PA12, stm32f103::GPIO_CNF_ALT_OUTPUT_PUSH_PULL, stm32f103::GPIO_MODE_OUTPUT_50M ); // CAN1_TX
        enable_interrupt( stm32f103::CAN1_TX_IRQn );
        enable_interrupt( stm32f103::CAN1_RX0_IRQn );
        
        // LED
        gpio_mode(stm32f103::PC13, stm32f103::GPIO_CNF_OUTPUT_ODRAIN, stm32f103::GPIO_MODE_OUTPUT_2M );
    }
    
    if ( auto AFIO = reinterpret_cast< volatile stm32f103::AFIO * >( stm32f103::AFIO_BASE ) ) {
        AFIO->MAPR &= ~1;            // clear SPI1 remap
        AFIO->MAPR &= ~(0b11 << 13); // clear CAN remap (RX = PA11, TX = PA12)
        stream() << "\tAFIO MAPR: 0x" << AFIO->MAPR << std::endl;
    }

    {
        int size = reinterpret_cast< const char * >(&_ebss) - reinterpret_cast< const char * >(&_sbss);
        stream() << "\t**********************************************" << std::endl;
        stream() << "\t***** BSS = 0x" << uint32_t(&_sbss) << " -- 0x" << uint32_t(&_ebss) << "\t *****" << std::endl;
        stream() << "\t***** " << size << " octsts of bss segment is in use. ***" << std::endl;
        stream() << "\t***** Current stack pointer = " << uint32_t(&size) << "\t *****" << std::endl;
        stream() << "\t**********************************************" << std::endl;
    }

    init_systick( 7200, true ); // 100us tick

    __can0.init( stm32f103::CAN1_BASE );  // experimental, not working yet
    __spi0.init( stm32f103::SPI1_BASE );  // experimental, not working yet -- strange, no data out on MOSI line

    // CAN is experimental, not working yet.
    CanMsg msg;
    msg.IDE = CAN_ID_STD;
    msg.RTR = CAN_RTR_DATA;
    msg.ID  = 0x55;
    msg.DLC = 8;
    msg.Data[ 0 ] = 0xaa;
    msg.Data[ 1 ] = 0x55;
    msg.Data[ 2 ] = 0;
    msg.Data[ 3 ] = 0;
    msg.Data[ 4 ] = 0;
    msg.Data[ 5 ] = 0;
    msg.Data[ 6 ] = 0;
    msg.Data[ 7 ] = 0;

    {
        int x = 0;
        for ( size_t i = 0; i < 3; ++i, ++x ) {
            stream() << "Hello world: " << x<< "\tjiffies: " << atomic_jiffies.load() << std::endl;
            // __can0.transmit( &msg );
            __spi0 << uint16_t( ~x );
            mdelay( 500 );
        }

        std::array< char, 128 > cbuf;
        typedef tokenizer< 8 > tokenizer_type;

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
    __uart0 << s;
}

void
serial_putc( int c )
{
    __uart0.putc( c );
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

void
adc1_handler()
{
    stm32f103::adc::interrupt_handler( &__adc0 );
}

void
spi1_handler()
{
    stm32f103::spi::interrupt_handler( &__spi0 );
}

void
spi2_handler()
{
    stm32f103::spi::interrupt_handler( &__spi1 );
}
