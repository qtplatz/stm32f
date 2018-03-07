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
    enum DMA_ISRx {
        TEIF  = 1 << 3        
        , HTIF  = 1 << 2
        , TCIF  = 1 << 1
        , GIF   = 1
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
dma::init_channel( dma& dma
                   , DMA_CHANNEL channel_number
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
    
    auto& dmaChannel = dma.dmaChannel( channel_number );
    if ( &dmaChannel == &readOnlyChannel )
        return false;

    // p277
    dmaChannel.CPAR = peripheral_data_addr;
    dmaChannel.CMAR = reinterpret_cast< decltype( dmaChannel.CMAR ) >( buffer_addr );
    dmaChannel.CNDTR = buffer_size;

    dmaChannel.CCR = ( dmaChannel.CCR & 0xffff800f ) | dma_ccr;

    // 1. Set the peripheral register address in the DMA_CPARx register. The data will be
    // moved from/ to this address to/ from the memory after the peripheral event.
    // 2. Set the memory address in the DMA_CMARx register. The data will be written to or
    // read from this memory after the peripheral event.
    // 3. Configure the total number of data to be transferred in the DMA_CNDTRx register.
    // After each peripheral event, this value will be decremented.
    // 4. Configure the channel priority using the PL[1:0] bits in the DMA_CCRx register
    // 5. Configure data transfer direction, circular mode, peripheral & memory incremented
    // mode, peripheral & memory data size, and interrupt after half and/or full transfer in the
    // DMA_CCRx register
    // 6. Activate the channel by setting the ENABLE bit in the DMA_CCRx register.
}

void
dma::setEnabled( uint32_t channel_number, bool enable )
{
    if ( enable )
        dmaChannel( channel_number ).CCR |= EN;
    else
        dmaChannel( channel_number ).CCR &= ~EN;
}
