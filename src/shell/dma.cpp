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
                   , DMA_DIR direction
                   , uint32_t peripheral_addr
                   , uint8_t * buffer_addr
                   , uint32_t buffer_size )
{
    // p277
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

// namespace stm32f103 {
//     template<> uint32_t dma_channel< DMA_ADC1 >::channel() const { return 1; };

//     template<> uint32_t dma_channel< DMA_SPI1_RX >::channel() const { return 2; };
//     template<> uint32_t dma_channel< DMA_SPI1_TX >::channel() const { return 3; };
    
//     template<> uint32_t dma_channel< DMA_I2C1_TX >::channel() const { return 6; };
//     template<> uint32_t dma_channel< DMA_I2C1_RX >::channel() const { return 7; };    
// }
    
