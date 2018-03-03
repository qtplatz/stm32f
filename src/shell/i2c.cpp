// Copyright (C) 2018 MS-Cheminformatics LLC
// Licence: CC BY-NC
// Author: Toshinobu Hondo, Ph.D.
// Contact: toshi.hondo@qtplatz.com
//

#include "i2c.hpp"
#include <array>
#include <atomic>
#include <mutex>
#include "stm32f103.hpp"
#include "printf.h"

// I2C_InitStruct->I2C_ClockSpeed = 5000;            /* Initialize the I2C_Mode member */
// I2C_InitStruct->I2C_Mode = I2C_Mode_I2C;          /* Initialize the I2C_DutyCycle member */
// I2C_InitStruct->I2C_DutyCycle = I2C_DutyCycle_2;  /* Initialize the I2C_OwnAddress1 member */
// I2C_InitStruct->I2C_OwnAddress1 = 0;              /* Initialize the I2C_Ack member */
// I2C_InitStruct->I2C_Ack = I2C_Ack_Disable;        /* Initialize the I2C_AcknowledgedAddress member */
// I2C_InitStruct->I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit; // 0x4000;

// bits in the status register
enum I2C_CR1_MASK {
    SWRST          = 1 << 15  // Software reset (0 := not under reset, 0 := under reset state
    , RES0         = 1 << 14  //
    , ALERT        = 1 << 13  //
    , PEC          = 1 << 12  //
    , POS          = 1 << 11  //
    , ACK          = 1 << 10  //
    , STOP         = 1 <<  9  //
    , START        = 1 <<  8  //
    , NOSTRETCH    = 1 <<  7  //
    , ENGC         = 1 <<  6  //
    , ENPEC        = 1 <<  5  //
    , ENARP        = 1 <<  4  //
    , SMBTYPE      = 1 <<  3  //
    , RES1         = 1 <<  2  //
    , SMBUS        = 1 <<  1  //
    , PE           = 1 <<  0  // Peripheral enable
};

enum I2C_CR2_MASK {
    LAST          = 1 << 12
    , DMAEN       = 1 << 11    // DMA requests enable
    , ITBUFFN     = 1 << 10    // Buffer interrupt enable
    , ITEVTEN     = 1 <<  9    // Event interrupt enable
    , ITERREN     = 1 <<  8    // Error interrupt enable
    , FREQ        = 0x3f       // Peirpheral clock frequence ( 0x02(2MHz) .. 0x32(50MHz) )
};

// p778
enum I2C_STATUS {
    ST_PEC        = 0xff << 8  // Packet error checking rigister
    , DUALF       = 1 << 7     // Dual flag (slave mode)
    , SMBHOST     = 1 << 6     // SMBus host header (slave mode)
    , SMBDEFAULT  = 1 << 5     // SMB deault, SMBus device default address (slave mode)
    , GENCALL     = 1 << 4     // General call address (slave mode)
    , TRA         = 1 << 2     // Transmitter/receiver (0: data bytes received, 1: data bytes transmitted)
    , BUSY        = 1 << 1     // Bus busy
    , MSL         = 1          // Master/slave
};

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
