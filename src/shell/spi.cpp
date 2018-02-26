// Copyright (C) 2018 MS-Cheminformatics LLC

#include "spi.hpp"
#include "stm32f103.hpp"
#include "printf.h"

// each I/O port registers have to be accessed as 32bit words. (reference manual pp158/1133)

namespace stm32f103 {

    enum { SPI_CR1 = 0, SPI_CR2 = 0x04, SPI_SR = 0x08, SPI_DR = 0x0c };
    enum { BIDIMODE   = 0x8000 // Bidirectional data mode enable
           , BIDIOE   = 0x4000 // 2-line unidirectional data mode|1-line bidirectional data mode
           , CRCEN    = 0x2000 // Hardware CRC calculation enable
           , CRCNEXT  = 0x1000 // CRC transfer next
           , DFF      = 0x0800 // 0: 8bit data frameformat, 1: 16-bit data frame format
           , RXONLY   = 0x0400 // Receive only (0: Full duplex, 1: Output disabled)
           , SSM      = 0x0200 // Software slave management (0: disabled, 1: enabled)
           , SSI      = 0x0100 // Internal slave select
           , LSBFIRST = 0x0080 // Frame format
           , BR       = 0x0038 // Baud rate control
           , MSTR     = 0x0004 // Master select
           , CPOL     = 0x0002
           , CPHA     = 0x0001
    };

    void
    spi::init( stm32f103::SPI_BASE base )
    {
        const int fclk = 0; // 1/2 fpclk
        if ( auto SPI = reinterpret_cast< volatile stm32f103::SPI * >( base ) ) {
            spi_ = SPI;
            SPI->CR1 = 0x88004 | SSI | ((fclk & 7) << 3) | SSM | MSTR; // BIDIMODE|DFF=16bit|MSTR
            // SPI->CR1 = 0x88004 | SSI | ((fclk & 7) << 3) | MSTR; // BIDIMODE|DFF=16bit|MSTR
            SPI->CR2 = 0;
        }
    }

    spi&
    spi::operator << ( uint16_t d )
    {
        
        if ( auto GPIOA = reinterpret_cast< volatile stm32f103::GPIO * >( stm32f103::GPIOA_BASE ) ) {
            GPIOA->BSRR = ( 1 << stm32f103::PA4 + 16 );
        }

        spi_->DATA = d;
        printf( "spi data=0x%x\t0x%x\n", d, spi_->DATA );

        if ( auto GPIOA = reinterpret_cast< volatile stm32f103::GPIO * >( stm32f103::GPIOA_BASE ) ) {
            GPIOA->BSRR = ( 1 << stm32f103::PA4 );
        }
    }

}
