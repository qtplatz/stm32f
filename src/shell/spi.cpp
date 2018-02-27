// Copyright (C) 2018 MS-Cheminformatics LLC

#include "gpio.hpp"
#include "spi.hpp"
#include "stm32f103.hpp"
#include "stream.hpp"
#include "printf.h"

extern "C" {
    void spi1_handler();
    void enable_interrupt( stm32f103::IRQn_type IRQn );
    void disable_interrupt( stm32f103::IRQn_type IRQn );
}

namespace stm32f103 {
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
    constexpr uint32_t cr1 = BIDIMODE | ((fclk & 7) << 3) | MSTR; // BIDIMODE|16bit|SPI enable|MSTR
    constexpr uint32_t cr2 = SSOE | (3 << 5); // SS output enable, Rx buffer not empty interrupt, Error interrupt
    
    void
    spi::init( stm32f103::SPI_BASE base )
    {
        const int fclk = 0; // 1/2 fpclk
        
        if ( auto SPI = reinterpret_cast< volatile stm32f103::SPI * >( base ) ) {
            spi_ = SPI;
            SPI->CR2 = cr2;
            SPI->CR1 = cr1 | SPE;
            // enable_interrupt( stm32f103::SPI1_IRQn );
        }
    }

    spi&
    spi::operator << ( uint16_t d )
    {
        printf("Tx data: %x, CR1=%x == %x, SR=%x\n", d, spi_->CR1, cr1, spi_->SR );

        spi_->CR1 = cr1 | SPE;

        spi_->DATA = d;
        
        if ( spi_->SR ) {
            stream() << "SR: " << spi_->SR << std::endl;
            if ( spi_->SR & 01 ) // RX buffer not empty
                stream() << "Rx data: " << spi_->DATA << std::endl;
        }
        
        while ( spi_->SR & (1 << 5) ) {
            stream() << "MODF (mode falt)" << spi_->SR << " CR1=" << spi_->CR1 << std::endl;
            spi_->CR1 = cr1;
        }
    }

}

void
spi1_handler()
{
    if ( auto SPI = reinterpret_cast< volatile stm32f103::SPI * >( stm32f103::SPI1_BASE ) ) {
        if ( SPI->SR & 01 ) // RX not empty
            printf("##### spi1 handler SR %x data=%x\n", SPI->SR, SPI->DATA );
        if ( SPI->SR & (1 << 5 ) ) { // MODF (mode falt)
            printf("##### spi1 handler MODF SR %x\n", SPI->SR );
            SPI->CR1 = stm32f103::cr1;
        }
    }
}
