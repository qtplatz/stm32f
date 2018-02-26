// Copyright (C) 2018 MS-Cheminformatics LLC

#include "uart.hpp"
#include <array>
#include <atomic>
#include <mutex>
#include "stm32f103.hpp"
#include "printf.h"

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
    static std::atomic< char > __input_char;
}

using namespace stm32f103;

extern uart __uart0;

uart::uart() : usart_( 0 )
             , baud_( 115200 )
{
}

bool
uart::init( stm32f103::USART_BASE addr, parity parity, int nbits, uint32_t baud, uint32_t pclk )
{
    usart_ = reinterpret_cast< stm32f103::USART * >( addr );
    baud_ = baud;

    __input_lock.clear();
    __input_char = 0;

    if ( usart_ ) {
        uint32_t flag( UE | TE | RE ); // uart enable, transmitter enable, receiver enable
        if ( parity != parity_none )
            flag |= ( PCE | ( parity << 8 ) ) & 0x0600;  // parity enable, [even|odd] parity

        if ( addr == stm32f103::USART1_BASE )        
            flag |= RXNEIE; // rx interrupt enable

        usart_->CR1  = flag;
        usart_->CR2  = 0;
        usart_->CR3  = 0;
        usart_->GTPR = 0;
        usart_->BRR  = pclk / baud;  // 72000000 / 115200

        if ( addr == stm32f103::USART1_BASE )
            enable_interrupt( stm32f103::USART1_IRQn );

        return true;
    }
    return false;
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
    while ( __input_lock.test_and_set( std::memory_order_acquire ) ) // acquire lock
        ;
    while ( __input_char.load() == 0 )
        ;
    auto c = __input_char.load();
    __input_char = 0;
    __input_lock.clear(); // release lock

    // echo
    if ( c >= 0x20 ) {
        __uart0.putc( c );
        if ( c == '\r' )
            __uart0.putc( '\n' );
    } else {
        __uart0.putc( '^' );
        __uart0.putc( c + 'A' );
    }
    
    return c;
}

// static
size_t
uart::gets( char * s, size_t size )
{
    char * p = s;
    while ( size > 0 ) {
        uint8_t c = __uart0.getc() & 0x7f;
        if ( c == '\r' ) {
            *p++ = '\n';
            break;
        } else {
            *p++ = c;
            if ( --size == 1 )
                break;
        }
    }
    *p = '\0';
    return size_t( p - s );
}

void
uart1_handler()
{
    auto usart = reinterpret_cast< stm32f103::USART * >( stm32f103::USART1_BASE );
    __input_char = usart->DR & 0xff;
}
