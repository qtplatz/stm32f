// Copyright (C) 2018 MS-Cheminformatics LLC

#include "gpio_mode.hpp"
#include "stm32f103.hpp"
#include "uart.hpp"
#include <array>
#include <atomic>
#include <mutex>

// bits in the status register
enum UART_CR1_MASK {
    Reserved = 0xfffc0000
    , UE     = 0x2000  // uart enable
    , M      = 0x1000  // Word length 0 := 1-start bit, 8 bits, n stop bit, 1 := 1 start bit, 9 data bits, n stop bit
    , WAKE   = 0x0800  // 0: idle line, 1: address mark
    , PCE    = 0x0400  // Parity control enable ( 0: disabled, 1:enabled )
    , PS     = 0x0200  // Parity 0: even, 1: odd
    , PEIE   = 0x0100  // PE interrupt enable
    , TXEIE  = 0x0080  // TXE interrupt enable
    , TCIE   = 0x0040  // Transmission complete interrupt enable
    , RXNEIE = 0x0020  // RXNE interrupt enable
    , IDLEIE = 0x0010  // IDLE interrupt enable
    , TE     = 0x0008  // Transmitter enable
    , RE     = 0x0004  // Receiver enable
    , RWU    = 0x0002  // Receiver wakeup
    , SBK    = 0x0001  // Send brak
};

enum UART_STATUS {
    ST_PE      =	0x0001
    , ST_FE    =	0x0002
    , ST_NE	   =	0x0004
    , ST_OVER  =	0x0008
    , ST_IDLE  =	0x0010
    , ST_RXNE  =	0x0020		// Receiver not empty
    , ST_TC	   =	0x0040		// Transmission complete
    , ST_TXE   =	0x0080		// Transmitter empty
    , ST_BREAK =	0x0100
    , ST_CTS   =	0x0200
};

extern "C" {
    void uart1_handler();
    void enable_interrupt( stm32f103::IRQn_type IRQn );
}

namespace stm32f103 {
    
    static std::atomic_flag __input_lock;
    static std::atomic< int > __input_char;
}

using namespace stm32f103;

uart::uart() : usart_( 0 )
             , baud_( 115200 )
{
}

bool
uart::init( stm32f103::USART_BASE addr )
{
    usart_ = reinterpret_cast< stm32f103::USART * >( addr );
    return true;
}

bool
uart::config( parity parity, int nbits, uint32_t baud, uint32_t pclk )
{
    baud_ = baud;
    if ( usart_ ) {
        uint32_t flag( UE | TE | RE ); // uart enable, transmitter enable, receiver enable
        if ( parity != parity_none )
            flag |= ( PCE | ( parity << 8 ) ) & 0x0600;  // parity enable, [even|odd] parity

        if ( reinterpret_cast< uint32_t >( usart_ ) == stm32f103::USART1_BASE )        
            flag |= RXNEIE; // rx interrupt enable

        usart_->CR1  = flag;
        usart_->CR2  = 0;
        usart_->CR3  = 0;
        usart_->GTPR = 0;
        usart_->BRR  = pclk / baud;  // 72000000 / 115200

        if ( reinterpret_cast< uint32_t >( usart_ ) == stm32f103::USART1_BASE ) {
            enable_interrupt( stm32f103::USART1_IRQn );
        }

        return true;
    }
    return false;
}

template<> bool uart::enable( GPIOA_PIN pa9, GPIOA_PIN pa10, parity parity, int nbits, uint32_t baud, uint32_t pclk )
{
    stm32f103::gpio_mode gpio_mode;    
    gpio_mode( pa9, stm32f103::GPIO_CNF_ALT_OUTPUT_PUSH_PULL, stm32f103::GPIO_MODE_OUTPUT_50M ); // (2,3)
    gpio_mode( pa10, stm32f103::GPIO_CNF_INPUT_PUSH_PULL,     stm32f103::GPIO_MODE_INPUT );
    return config( parity, nbits, baud, pclk );
}

template<> bool uart::enable( GPIOB_PIN tx, GPIOB_PIN rx, parity parity, int nbits, uint32_t baud, uint32_t pclk )
{
    stm32f103::gpio_mode gpio_mode;    
    gpio_mode( tx, stm32f103::GPIO_CNF_ALT_OUTPUT_PUSH_PULL, stm32f103::GPIO_MODE_OUTPUT_50M ); // (2,3)
    gpio_mode( rx, stm32f103::GPIO_CNF_INPUT_PUSH_PULL,     stm32f103::GPIO_MODE_INPUT );
    return config( parity, nbits, baud, pclk );
}

uart&
uart::operator << ( const char * s )
{
    while ( s && *s ) {
        if ( *s == '\n' )
            putc( '\r' );
        putc( *s++ );
    }
    return *this;
}

void
uart::putc( int c )
{
	while ( ! (usart_->SR & ST_TXE) )
	    ;
    usart_->DR = c;
}

// always handle with usart1
// static
int
uart::getc()
{
    int c = 0;
    while ( ( c = __input_char.load() ) == 0 )
        ;
    __input_char = 0;
    __input_lock.clear(); // release lock

    uart_t< USART1_BASE >::instance()->putc( c ); // echo
    
    return c;
}

// static
size_t
uart::gets( char * s, size_t size )
{
    auto& uart0 = *uart_t< USART1_BASE >::instance();
    
    char * p = s;
    while ( size > 0 ) {
        uint8_t c = uart0.getc() & 0x7f;
        if ( c == '\r' ) {
            *p++ = '\n';
            break;
        } else if ( c == '\b' || c == 0x7f || c == 0x15 ) {
            if ( p > s ) {
                --p;
                ++size;
            }
        } else if ( c >= ' ' && c < 0x7f ) {
            *p++ = c;
            if ( --size == 1 )
                break;
        }
    }
    *p = '\0';
    return size_t( p - s );
}

void
uart::handle_interrupt()
{
    __input_char = usart_->DR & 0xff;
}

