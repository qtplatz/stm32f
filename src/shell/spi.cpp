// Copyright (C) 2018 MS-Cheminformatics LLC

#include "gpio.hpp"
#include "spi.hpp"
#include "stm32f103.hpp"
#include "stream.hpp"
#include "printf.h"
#include "spinlock.hpp"
#include <atomic>

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

    constexpr uint32_t fclk = 06; // 1/256 ( ~= 0.8us)
    // constexpr uint32_t cr1 = BIDIMODE | DFF | SSM | SSI | SPE | ((fclk & 7) << 3) | MSTR; // BIDIMODE|16bit|SPI enable|MSTR
    // constexpr uint32_t cr1 = BIDIMODE | SSM | SSI | ((fclk & 7) << 3) | MSTR; // BIDIMODE|16bit|SPI enable|MSTR
    constexpr uint32_t _cr1 = BIDIMODE | DFF | ((fclk & 7) << 3) | CPOL | CPHA | MSTR; // BIDIMODE|16bit|SPI enable|MSTR
    constexpr uint32_t _cr2 = SSOE | (3 << 5); // SS output enable, Rx buffer not empty interrupt, Error interrupt
    
    void
    spi::init( stm32f103::SPI_BASE base, uint8_t gpio, uint32_t ss_n )
    {
        lock_.clear();
        rxd_ = 0;

        gpio_ = gpio;
        ss_n_ = ss_n;
        
        if ( auto SPI = reinterpret_cast< volatile stm32f103::SPI * >( base ) ) {
            spi_ = SPI;
            SPI->CR2 = _cr2;
            SPI->CR1 = _cr1 | SPE | ( gpio_ ? SSM | SSI : 0 );
            switch( base ) {
            case SPI1_BASE:
                enable_interrupt( stm32f103::SPI1_IRQn );
                break;
            case SPI2_BASE:
                enable_interrupt( stm32f103::SPI2_IRQn );
                break;
            case SPI3_BASE:
                enable_interrupt( stm32f103::SPI3_IRQn );
                break;
            }
        }

        if ( gpio_ )
            (*this) = true;  // ~SS -> H
    }

    void
    spi::operator = ( bool flag ) {
        switch( gpio_ ) {
        case 'A':
            stm32f103::gpio< GPIOA_PIN >( static_cast< GPIOA_PIN >( ss_n_ ) ) = flag;
            break;
        case 'B':
            stm32f103::gpio< GPIOB_PIN >( static_cast< GPIOB_PIN >( ss_n_ ) ) = flag;
            break;
        }
    }

    spi&
    spi::operator << ( uint16_t d )
    {
        while( rxd_.load() == 0 )
            ;
        auto rxd = rxd_.load() & 0xffff;
        {
            scoped_spinlock<> lock( lock_ );
            rxd_ = 0;
        }

        lock_.clear( std::memory_order_release ); // release lock

        (*this) = false;
        spi_->DATA = d;
        spi_->CR1 |= SPE | ( gpio_ ? SSM | SSI : 0 ); // enable

        //while( spi_->SR & 02 ) // wait while TX buffer not empty
        //    ;
        printf("Tx data: %x, Rx data: %x, CR1=%x, SR=%x\n", d, rxd, spi_->CR1, spi_->SR );
    }

    void
    spi::handle_interrupt()
    {
        if ( spi_ ) {
            if ( spi_->SR & 01 ) { // RX not empty
                rxd_ = spi_->DATA | 0x80000000;
                spi_->CR1 &= ~SPE; // disable
                (*this) = true;
            }
#if 0
            if ( spi_->SR ) {
                scoped_spinlock<> lock( lock_ );

                printf("SPI IRQ: " );
                if ( spi_->SR & 0x80 )
                    printf("BSY,");
                if ( spi_->SR & 0x40 )
                    printf("OVR,");
                if ( spi_->SR & 0x20 )
                    printf("MODF,");
                if ( spi_->SR & 0x10 )
                    printf("CRCERR,");
                if ( spi_->SR & 0x08 )
                    printf("UDR,");
                if ( spi_->SR & 04 )
                    printf("CHSIDE,");
                if ( spi_->SR & 02 )
                    printf("TXE" );
                printf(" Rx=[%x]\n", rxd_.load() );
            }
#endif
            if ( spi_->SR & (1 << 5 ) ) { // MODF (mode falt)
                spi_->CR1 = stm32f103::_cr1;
            }
        }
    }

    void
    spi::interrupt_handler( spi * _this )
    {
        _this->handle_interrupt();
    }

}
