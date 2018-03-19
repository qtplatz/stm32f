// Copyright (C) 2018 MS-Cheminformatics LLC

#include "dma.hpp"
#include "dma_channel.hpp"
#include "gpio.hpp"
#include "spi.hpp"
#include "stm32f103.hpp"
#include "stream.hpp"
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
    enum SPI_CR1 {
        BIDIMODE   = (01 << 15) // Bidirectional data mode enable
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
        SSOE         = 04 //(01 << 2) // SS output enable
    };

    constexpr uint32_t fclk = 04;
    constexpr uint32_t cr1 = ((fclk & 7) << 3)          // clock
                                            //| BIDIMODE  // 1: 1-line bidirectional, 0: 2-line unidirectional data
                                            //| BIDIOE
                                            | DFF       // 16bit
                                            | CPOL      // polarity
                                            | CPHA      // phase
                                            | SSI
                                            | MSTR      // master mode
                                            ;
}

using namespace stm32f103;
    
void
spi::init( stm32f103::SPI_BASE base, uint8_t gpio, uint32_t ss_n )
{
    lock_.clear();
    rxd_ = 0;

    gpio_ = gpio;
    ss_n_ = ss_n;

    scoped_spinlock<> lock( lock_ );
    stream() << "spi::init gpio = " << char( gpio ) << ", ss_n=" << int( ss_n ) << std::endl;
        
    if ( auto SPI = reinterpret_cast< volatile stm32f103::SPI * >( base ) ) {
        spi_ = SPI;
        if ( gpio ) {
            SPI->CR1 = cr1 | SPE | SSM;
            SPI->CR2 = (3 << 5);        // SS output disable, IRQ {Rx buffer not empty, Error}
        } else {
            SPI->CR1 = cr1 | SPE; // | BIDIMODE | BIDIOE;
            SPI->CR2 = (3 << 5) | SSOE; // SS output enable, IRQ {Rx buffer not empty, Error}
        }
            
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
        cr1_ = spi_->CR1;
    }
    (*this) = true;  // ~SS -> H
}

void
spi::slave_setup()
{
    rxd_ = 0;
    gpio_ = 0;
    ss_n_ = 0;

    spi_->CR1 = ( cr1 | SPE ) & ~MSTR;
    spi_->CR2 = (3 << 5);
    cr1_ = spi_->CR1;
}

void
spi::setup( uint8_t gpio, uint32_t ss_n )
{
    gpio_ = gpio;
    ss_n_ = ss_n;

    scoped_spinlock<> lock( lock_ );
        
    if ( gpio ) {
        spi_->CR1 = cr1 | SPE | SSM;
        spi_->CR2 = (3 << 5);        // SS output disable, IRQ {Rx buffer not empty, Error}
    } else {
        spi_->CR1 = cr1 | SPE; // | BIDIMODE | BIDIOE;
        spi_->CR2 = (3 << 5) | SSOE; // SS output enable, IRQ {Rx buffer not empty, Error}
    }
    cr1_ = spi_->CR1;

    (*this) = true;  // ~SS -> H
}

// void
// spi::init( SPI_BASE base, dma& dma )
// {
//     // dma_channel_t< DMA_SPI1_TX > tx_dma( dma );
//     //dma_ = &dma;
//     //dma_channel_ = channel;
//     if ( auto SPI = reinterpret_cast< volatile stm32f103::SPI * >( base ) ) {
//         spi_ = SPI;        
//         spi_->CR1 = cr1 | SPE;
//         spi_->CR2 = SSOE;
//         cr1_ = spi_->CR1;
//     }
// }

void
spi::operator = ( bool flag ) {
    // if ( flag )
    //     //spi_->CR1 &= ~SSI;
    //     spi_->CR1 |= SSI;
    // else
    //     spi_->CR1 |= SSI;
    uint32_t flags = spi_->CR1;
    if ( ( flags & MSTR ) && ( flags & SSM ) ) {
#if 0
        switch( gpio_ ) {
        case 'A':
            stm32f103::gpio< GPIOA_PIN >( static_cast< GPIOA_PIN >( ss_n_ ) ) = flag;
            break;
        case 'B':
            stm32f103::gpio< GPIOB_PIN >( static_cast< GPIOB_PIN >( ss_n_ ) ) = flag;
            break;
        }
#endif
    }

}

spi&
spi::operator >> ( uint16_t& d )
{
    (*this) = false;       // ~SS -> L

    // cr1_ &= ~BIDIOE; // read only
    // spi_->CR1 = cr1_;
    
    while( ! rxd_ )
        ;

    {
        scoped_spinlock<> lock( lock_ );
        d = rxd_.load() & 0xffff;
        rxd_ = 0;
    }
    return * this;
}    

spi&
spi::operator << ( uint16_t d )
{
    uint32_t wait = 0xffffff;
    (*this) = false;

    while ( --wait && txd_ )
        ;
    
    if ( wait == 0 ) {
        scoped_spinlock<> lock( lock_ );
        stream() << "spi tx timeout" << std::endl;
    }
    txd_ = d;
    // spi_->CR1 |= SPE | BIDIOE; // SPI enable, output only mode
    // cr1_ = spi_->CR1;
    spi_->CR2 |= (1 << 7);     // Tx empty irq
}

void
spi::handle_interrupt()
{
    if ( spi_ ) {
        if ( spi_->SR & 01 ) { // RX not empty
            rxd_ = spi_->DATA | 0x80000000;
            (*this) = true;        // ~SS = 'H'
            // spi_->CR1 |= BIDIOE;   // switch to write-only mode
            scoped_spinlock<> lock( lock_ );
            if ( spi_ == reinterpret_cast< volatile stm32f103::SPI * >( SPI2_BASE ) )
                stream() << "SPI2 got : " << ( rxd_.load() & 0xffff ) << std::endl;
        }
            
        if ( spi_->SR & 02 ) { // Tx empty
            if ( txd_ ) {
                (*this) = false;  // ~SS = 'L'
                spi_->DATA = txd_.load();
                txd_ = 0;
            } else {
                spi_->CR2 &= ~(1 << 7); // Tx empty irq disable
                (*this) = true;        // ~SS = 'H'                    
            }
        }

        if ( auto flags = ( spi_->SR & 0x7c ) ) { // ignore BSY, RX not empty, TX empty
            scoped_spinlock<> lock( lock_ );
            
            stream() << "SPI IRQ: [" << flags << " CR1=" << spi_->CR1 << "]";
            if ( flags & 0x80 )
                stream() << ("BSY,");
            if ( flags & 0x40 )
                stream() << ("OVR,");
            if ( flags & 0x20 )
                stream() << ("MODF,");
            if ( flags & 0x10 )
                stream() << ("CRCERR,");
            if ( flags & 0x08 )
                stream() << ("UDR,");
            if ( flags & 04 )
                stream() << ("CHSIDE,");
            stream() << std::endl;
        }

        if ( spi_->SR & (1 << 5 ) ) { // MODF (mode falt)
            spi_->CR1 = spi_->CR1;
            stream() << "SPI MODF: [" << spi_->SR << " CR1=" << spi_->CR1 << "]" << std::endl;
        }
    }
}

void
spi::interrupt_handler( spi * _this )
{
    _this->handle_interrupt();
}

