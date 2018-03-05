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

void
dma::init_channel( uint32_t channel
                   , uint32_t configuration          // CCR
                   , const uint8_t * source_addr
                   , uint8_t * destination_addr
                   , uint32_t transfer_size )
{
}

namespace stm32f103 {
    template<> uint32_t dma_channel_t< DMA_ADC1 >::channel() const { return 1; };

    template<> uint32_t dma_channel_t< DMA_SPI1_RX >::channel() const { return 2; };
    template<> uint32_t dma_channel_t< DMA_SPI1_TX >::channel() const { return 3; };
    
    template<> uint32_t dma_channel_t< DMA_I2C1_TX >::channel() const { return 6; };
    template<> uint32_t dma_channel_t< DMA_I2C1_RX >::channel() const { return 7; };    
}
    
