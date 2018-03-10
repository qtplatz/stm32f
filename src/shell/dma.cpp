// Copyright (C) 2018 MS-Cheminformatics LLC
// Licence: CC BY-NC
// Author: Toshinobu Hondo, Ph.D.
// Contact: toshi.hondo@qtplatz.com
//

#include "dma.hpp"
#include "dma_channel.hpp"
#include <array>
#include <atomic>
#include <mutex>
#include "stm32f103.hpp"
#include "printf.h"

extern "C" {
    void i2c1_handler();
    void enable_interrupt( stm32f103::IRQn_type IRQn );
}

namespace stm32f103 {
    template< DMA_BASE > struct dma_number_of_channels { static constexpr uint32_t value = 7; };
    template<> struct dma_number_of_channels< DMA2_BASE > { static constexpr uint32_t value = 5; };
}

namespace stm32f103 {
    // DMA_ISRx = [27:24][23:20]..[3:0]; // 4bit/word
    enum DMA_ISRx {       // interrupt status register
        TEIF  = 1 << 3        // transfer error flag
        , HTIF  = 1 << 2      // half transfer flag
        , TCIF  = 1 << 1      // transfer complete flag
        , GIF   = 1           // global interrupt flag
    };

    enum DMA_IFCRx {  // 4bit/word
        CTEIF  = 1 << 3  // Channel x transfer error clear
        , CHTIF  = 1 << 2  // Channel x half transfer clear
        , CTCIF  = 1 << 1  // Channel x transfer complete clear
        , CGIF   = 1       // Channel x global interrupt clear
    };

}

using namespace stm32f103;

extern dma __dma0;

dma::dma() : dma_( 0 )
{
}

void
dma::init( stm32f103::DMA_BASE addr )
{
    lock_.clear();
    interrupt_status_ = 0;

    if ( auto DMA = reinterpret_cast< volatile stm32f103::DMA * >( addr ) ) {
        dma_ = DMA;
    }
}

constexpr static const DMAChannel readOnlyChannel = { 0 }; // allocated on .data (ROM)

volatile DMAChannel&
dma::dmaChannel( uint32_t channel )
{
    auto number_of_channels = dma_number_of_channels< DMA1_BASE >::value;
    if ( dma_ == reinterpret_cast< volatile stm32f103::DMA * >( DMA1_BASE ) ) 
        dma_number_of_channels< stm32f103::DMA2_BASE >::value;

    if ( channel < number_of_channels )
        return dma_->channels[ channel ];

    // error return
    return const_cast< volatile DMAChannel& >(readOnlyChannel);
}

bool
dma::init_channel( DMA_CHANNEL channel_number
                   , uint32_t peripheral_data_addr
                   , uint8_t * buffer_addr
                   , uint32_t buffer_size
                   , uint32_t dma_ccr )
{
    stream() << "dma::init_channel(" << int( channel_number )
             << ", DR: 0x" << peripheral_data_addr
             << ", size: " << int(buffer_size)
             << ", CCR: 0x" << dma_ccr
             << ")"
             << std::endl;
    
    auto& channel = dmaChannel( channel_number );

    // p277
    channel.CPAR = peripheral_data_addr;
    channel.CMAR = reinterpret_cast< decltype( channel.CMAR ) >( buffer_addr );
    channel.CNDTR = buffer_size;

    //channel.CCR = ( channel.CCR & 0xffff800f ) | dma_ccr;
    channel.CCR = dma_ccr;

    if ( reinterpret_cast< uint32_t >( const_cast< DMA * >( dma_ ) ) == DMA1_BASE ) {
        enable_interrupt( IRQn( DMA1_Channel1_IRQn + channel_number ) );
    } else {
        enable_interrupt( IRQn( DMA2_Channel1_IRQn + channel_number ) );
    }
}

void
dma::enable( uint32_t channel_number, bool enable )
{
    if ( enable ) {
        while ( !lock_.test_and_set( std::memory_order_acquire ) )
            ;
        interrupt_status_ &= ~( 0x0f << channel_number );
        lock_.clear();
        dmaChannel( channel_number ).CCR |= EN | TCIE | TEIE; // channel enable, transfer complete interrupt enable, error irq
    } else {
        dmaChannel( channel_number ).CCR &= ~( EN | TCIE );
    }
    stream( __FILE__, __LINE__ ) << "dma dma #" << channel_number << ": " << enable << std::endl;
    stream() << "dma::enable DR: 0x" << dmaChannel( channel_number ).CMAR
             << ", size: " << int( dmaChannel( channel_number ).CNDTR )
             << ", CPAR: 0x" << dmaChannel( channel_number ).CPAR
             << ", CCR: 0x" << dmaChannel( channel_number ).CCR
             << std::endl;
    
}

void
dma::set_transfer_buffer( uint32_t channel_number, const uint8_t * buffer, size_t size )
{
    dmaChannel( channel_number ).CMAR = reinterpret_cast< uint32_t >( buffer );
    dmaChannel( channel_number ).CNDTR = size;

    stream( __FILE__, __LINE__ ) << "set_transfer_buffer #" << channel_number
                                 << ", DR: 0x" << dmaChannel( channel_number ).CMAR
                                 << ", size: " << int( dmaChannel( channel_number ).CNDTR ) << ", " << int( size )
                                 << ", CPAR: 0x" << dmaChannel( channel_number ).CPAR
                                 << ", CCR: 0x" << dmaChannel( channel_number ).CCR
                                 << std::endl;
}

void
dma::set_receive_buffer( uint32_t channel_number, uint8_t * buffer, size_t size )
{
    dmaChannel( channel_number ).CMAR = reinterpret_cast< uint32_t >( buffer );
    dmaChannel( channel_number ).CNDTR = size;    
}

bool
dma::transfer_complete( uint32_t channel )
{
    uint32_t isr(0);
    
    while ( ! lock_.test_and_set( std::memory_order_acquire ) )
            ;
    isr = interrupt_status_.load();
    lock_.clear();

    return ( ( isr >> (channel * 4) ) & TCIF );
}


void
dma::handle_interrupt( uint32_t channel )
{
    while ( !lock_.test_and_set( std::memory_order_acquire ) )
        ;
    uint32_t flag = dma_->ISR;
    interrupt_status_ = flag;
    lock_.clear();

    dma_->IFCR |= (0x0f << (channel * 4)) & flag;

    auto x = flag >> ( channel * 4 );

    stream() << "\tDMA: handle_interrupt: " << channel << " ISR=" << flag << " "
             << ((x & 0x8) ? "transfer error, " : "")
             << ((x & 0x4) ? "half transfer, " : "")
             << ((x & 0x2) ? "transfer complete, " : "")
             << ((x & 0x1) ? "global interrupt, " : "")
             << std::endl;
}
