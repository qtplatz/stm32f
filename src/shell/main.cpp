/* shell.c
 * Copyright (C) MS-Cheminformatics LLC
 * Author: Toshinobu Hondo, Ph.D., 
 *
 * Project was started with a reference project from 
 * https://github.com/trebisky/stm32f103
 */

#include "can.hpp"
#include "command_processor.hpp"
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

std::atomic< uint32_t > atomic_jiffies;      //  100us  (4.97 days)
std::atomic< uint32_t > atomic_milliseconds; // 1000us  (49.71 days)
std::atomic< uint32_t > atomic_seconds;      // 1s      (136.1925 years)

stm32f103::uart __uart0;
stm32f103::spi __spi0;
stm32f103::can __can0;

extern "C" {
    void enable_interrupt( stm32f103::IRQn_type IRQn );
    void disable_interrupt( stm32f103::IRQn_type IRQn );
    void __aeabi_unwind_cpp_pr0 ( int, int *, int * ) {}
    void __aeabi_unwind_cpp_pr1 ( int, int *, int * ) {}

    void serial_puts( const char * s );
    void serial_putc( int );
    void systick_handler();
    void * memset( void * ptr, int value, size_t num );

    int main();
}

static void
delay_one_ms ( void )
{
    auto t0 = atomic_jiffies.load();
	while ( atomic_jiffies.load() - t0 < 10 )
	    ;
}

void
delay_ms ( int ms )
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
    if ( auto RCC = reinterpret_cast< volatile stm32f103::RCC * >( stm32f103::RCC_BASE ) ) {
        // clock/pll setup -->
        // RM0008 p98- 
        if ( auto FLASH = reinterpret_cast< volatile stm32f103::FLASH * >( stm32f103::FLASH_BASE ) )
            FLASH->ACR = 0x0010 | 0x0002; // FLASH_PREFETCH (enable prefetch buffer)| FLASH_WAIT2(for 48 < sysclk <= 72MHz);
        
        RCC->CFGR = ( 7 << 18 ) | ( 1 << 16 ) | ( 4 << 8 );           // pll input clock(9), PLLSRC=HSE, PPRE1=4
        RCC->CR   = ( 1 << 24 ) | ( 1 << 16 ) | (0b10000 << 3) | 1;   // PLLON, HSEON, HSITRIM(0b10000), HSION
        while ( ! RCC->CR & ( 1 << 17 ) )                             // Wait until HSE settles down (HSE RDY)
            ;
        RCC->CFGR |= ( 2 << 14 ) | 0x02;                              // ADC prescaler (0b10 div by 6), SW(0b10, pll selected as system clock)
        // <--

        // See p100 of RM008 (DocID 13902, Rev. 17)
        RCC->APB2ENR |= 0x0004; // GPIO A enable
        RCC->APB2ENR |= 0x0008; // GPIO B enable
        RCC->APB2ENR |= 0x0010; // GPIO C enable
        RCC->APB2ENR |= (1 << 14); // UART1 enable;
        RCC->APB2ENR |= (1 << 12); // SPI1 enable;

        RCC->APB2ENR |= 0x0001;    // AFIO enable
        RCC->APB1ENR |= (1 << 25); // CAN clock enable
        // ADC prescaler
        RCC->CFGR &= ~( 0b11 << 14 );
        RCC->CFGR != ~( 0b10 << 14 );  // set prescaler to 6
    }

    atomic_jiffies = 0;
    atomic_milliseconds = 0;
    atomic_seconds = 0;

    {
        stm32f103::gpio_mode gpio_mode;
        
        // initialize UART0 for debug output
        gpio_mode( stm32f103::PA9, stm32f103::GPIO_CNF_ALT_OUTPUT_PUSH_PULL, stm32f103::GPIO_MODE_OUTPUT_50M ); // 2, 3
        gpio_mode( stm32f103::PA10, stm32f103::GPIO_CNF_INPUT_PUSH_PULL,     stm32f103::GPIO_MODE_INPUT );
        __uart0.init( stm32f103::USART1_BASE, stm32f103::uart::parity_even, 8, 115200, 72000000 );            
        
        gpio_mode( stm32f103::PA0, stm32f103::GPIO_CNF_OUTPUT_PUSH_PULL,     stm32f103::GPIO_MODE_OUTPUT_2M );
        
        // SPI
        gpio_mode( stm32f103::PA4, stm32f103::GPIO_CNF_ALT_OUTPUT_PUSH_PULL, stm32f103::GPIO_MODE_OUTPUT_50M ); // SPI nSS
        gpio_mode( stm32f103::PA5, stm32f103::GPIO_CNF_ALT_OUTPUT_PUSH_PULL, stm32f103::GPIO_MODE_OUTPUT_50M ); // SPI sclk
        gpio_mode( stm32f103::PA6, stm32f103::GPIO_CNF_INPUT_PUSH_PULL,      stm32f103::GPIO_MODE_INPUT );   // SPI miso
        gpio_mode( stm32f103::PA7, stm32f103::GPIO_CNF_ALT_OUTPUT_PUSH_PULL, stm32f103::GPIO_MODE_OUTPUT_50M ); // SPI mosi
        
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
        AFIO->MAPR &= ~(0b11) << 13; // clear CAN remap (RX = PA11, TX = PA12)
        stream() << "AFIO MAPR: 0x" << AFIO->MAPR << std::endl;
    }

    init_systick( 7200, true ); // 100us tick

    __spi0.init( stm32f103::SPI1_BASE );
    __can0.init( stm32f103::CAN1_BASE );

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
        for ( size_t i = 0; i < 10; ++i ) {
            delay_ms( 500 );
            stream() << "Hello world: " << x++ << "\tjiffies: " << atomic_jiffies.load() << std::endl;
            __spi0 << uint16_t( ~x );
            __can0.transmit( &msg );
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

    if ( auto GPIOA = reinterpret_cast< volatile stm32f103::GPIO * >( stm32f103::GPIOA_BASE ) ) {
        if ( tp & 01 )
            GPIOA->BRR = 1 << ( stm32f103::PA0 );
        else
            GPIOA->BSRR = 1 << ( stm32f103::PA0 );
    }

    if ( ( tp % (10 * 250) ) == 0 ) {
        static uint32_t x(0);
        if ( auto GPIOC = reinterpret_cast< volatile stm32f103::GPIO * >( stm32f103::GPIOC_BASE ) ) {
            if ( x++ & 01 )
                GPIOC->BRR = 1 << ( stm32f103::PC13 );       // LED RESET
            else
                GPIOC->BSRR = 1 << ( stm32f103::PC13 );      // LED SET
        }
    }
}

void *
memset( void * dest, int value, size_t num )
{
    char * p = reinterpret_cast< char * >( dest );
    while ( num-- )
        *p++ = value;
    return dest;
}
