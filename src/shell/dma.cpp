// Copyright (C) 2018 MS-Cheminformatics LLC
// Licence: CC BY-NC
// Author: Toshinobu Hondo, Ph.D.
// Contact: toshi.hondo@qtplatz.com
//

#include "dma.hpp"
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

extern i2c __i2c0;

i2c::i2c() : i2c_( 0 )
{
}

void
i2c::init( stm32f103::I2C_BASE addr )
{
    lock_.clear();
    rxd_ = 0;

    constexpr uint32_t freq = 4; // 72000000 / ( 5000 * 2 );
    constexpr uint32_t pclk1 = 72000000 / 4;  // (18000000)
    
    if ( auto I2C = reinterpret_cast< volatile stm32f103::I2C * >( addr ) ) {
        i2c_ = I2C;

        

        i2c_->CR1 = SWRST; // Software reset

        i2c_->CR2 = freq;
        
        i2c_->CR1 |= PE; // peripheral enable
    }
}

i2c&
i2c::operator << ( uint16_t d )
{
    i2c_->DR = d & 0xff;
    return *this;
}

void
i2c1_handler()
{
}
