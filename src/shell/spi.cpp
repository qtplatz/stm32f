// Copyright (C) 2018 MS-Cheminformatics LLC

#include "gpio.hpp"
#include "spi.hpp"
#include "stm32f103.hpp"
#include "stream.hpp"
#include "printf.h"
#include <atomic>

extern "C" {
    void spi1_handler();
    void enable_interrupt( stm32f103::IRQn_type IRQn );
    void disable_interrupt( stm32f103::IRQn_type IRQn );
}

namespace stm32f103 {

    static std::atomic_flag __spi_lock;
    static std::atomic< uint32_t > __spi_rxd;
    
    // p702 SPI
    // p742 RM0008
    enum SPI_CR1 { BIDIMODE   = (01 << 15) // Bidirectional data mode enable
           , BIDIOE   = (01 << 14) // 2-line unidirectional data mode|1-line bidirectional data mode
           , CRCEN    = (01 << 13) // Hardware CRC calculation enable
           , CRCNEXT  = (01 << 12) // CRC transfer next
           , DFF      = (01 << 11) // 0: 8bit data frame format, 1: 16-bit data frame format
           , RXONLY   = (01 << 10) // Receive only (0: Full duplex, 1: Output disabled)
           , SSM      = (01 <<  9) // Software slave management (0: disabled, 1: enabled)
           , SSI      = (01 <<  8) // Internal slave select
           , LSBFIRST = (01 <<  7) // LSB first
           , SPE      = (01 <<  6) // SPI enable
           , BR       = (07 <<  3) // Baud rate control
           , MSTR     = (01 <<  2) // Master configuration
           , CPOL     = (01 <<  1)
           , CPHA     = 0x0001
    };

    enum SPI_CR2 {
        SSOE         = (01 << 2) // SS output enable
    };

    constexpr uint32_t fclk = 05; // 1/64 ( ~= 0.8us)
    // constexpr uint32_t cr1 = BIDIMODE | DFF | SSM | SSI | SPE | ((fclk & 7) << 3) | MSTR; // BIDIMODE|16bit|SPI enable|MSTR
    // constexpr uint32_t cr1 = BIDIMODE | SSM | SSI | ((fclk & 7) << 3) | MSTR; // BIDIMODE|16bit|SPI enable|MSTR
    constexpr uint32_t cr1 = BIDIMODE | DFF | ((fclk & 7) << 3) | MSTR; // BIDIMODE|16bit|SPI enable|MSTR
    constexpr uint32_t cr2 = SSOE | (3 << 5); // SS output enable, Rx buffer not empty interrupt, Error interrupt
    
    void
    spi::init( stm32f103::SPI_BASE base )
    {
        const int fclk = 0; // 1/2 fpclk
        __spi_lock.clear();
        __spi_rxd = 0;
        
        if ( auto SPI = reinterpret_cast< volatile stm32f103::SPI * >( base ) ) {
            spi_ = SPI;
            SPI->CR2 = cr2;
            SPI->CR1 = cr1 | SPE;
            enable_interrupt( stm32f103::SPI1_IRQn );
        }
    }

    spi&
    spi::operator << ( uint16_t d )
    {
        while( __spi_lock.test_and_set( std::memory_order_acquire ) ) // acquire lock
            ;
        while( __spi_rxd.load() == 0 )
            ;
        auto rxd = __spi_rxd.load() & 0xffff;

        __spi_rxd = 0;

        __spi_lock.clear(); // release lock

        spi_->DATA = d;
        
        printf("Tx data: %x, Rx data: %x, CR1=%x, SR=%x\n", d, rxd, spi_->CR1, spi_->SR );

        spi_->CR1 |= SPE; // enable
    }

}

void
spi1_handler()
{
    if ( auto SPI = reinterpret_cast< volatile stm32f103::SPI * >( stm32f103::SPI1_BASE ) ) {
        using namespace stm32f103;
        if ( SPI->SR & 01 ) { // RX not empty
            __spi_rxd = SPI->DATA | 0x80000000;
            SPI->CR1 &= ~SPE; // disable
        }

        if ( SPI->SR ) {
            while( __spi_lock.test_and_set( std::memory_order_acquire ) ) // acquire lock
                ;            
            printf("SPI2 IRQ: " );
            if ( SPI->SR & 0x80 )
                printf("BSY,");
            if ( SPI->SR & 0x40 )
                printf("OVR,");
            if ( SPI->SR & 0x20 )
                printf("MODF,");
            if ( SPI->SR & 0x10 )
                printf("CRCERR,");
            if ( SPI->SR & 0x08 )
                printf("UDR,");
            if ( SPI->SR & 04 )
                printf("CHSIDE,");
            if ( SPI->SR & 02 )
                printf("TXE" );
            
            printf("[%x]\n", __spi_rxd.load() );

            __spi_lock.clear();
        }

        if ( SPI->SR & (1 << 5 ) ) { // MODF (mode falt)
            SPI->CR1 = stm32f103::cr1;
        }
    }
}
