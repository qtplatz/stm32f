// Copyright (C) 2018 MS-Cheminformatics LLC
// Licence: CC BY-NC
// Author: Toshinobu Hondo, Ph.D.
// Contact: toshi.hondo@qtplatz.com
//

#include "i2c.hpp"
#include "stream.hpp"
#include "stm32f103.hpp"
#include <array>
#include <atomic>
#include <mutex>

extern uint32_t __pclk1, __pclk2;

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

extern stm32f103::i2c __i2c0;
extern void mdelay( uint32_t );

using namespace stm32f103;


i2c::i2c() : i2c_( 0 )
{
}

void
i2c::init( stm32f103::I2C_BASE addr )
{
    lock_.clear();
    rxd_ = 0;

    // AD5593R address is 0b010000[0|1]

    stream() << "i2c::init(" << addr << ")" << std::endl;
    
     if ( auto I2C = reinterpret_cast< volatile stm32f103::I2C * >( addr ) ) {
        i2c_ = I2C;

        i2c_->CR1 = SWRST; // Software reset
        i2c_->CR1 = 0; // ~PE;
        
        uint16_t freq = uint16_t( __pclk1 / 1000000 );
        i2c_->CR2 = freq;
        i2c_->CR2 |= ITBUFFN |  ITEVTEN | ITERREN;

        i2c_->OAR1 = 0x20 << 1;
        i2c_->OAR2 = 0;
       
        switch ( addr ) {
        case I2C1_BASE:
            enable_interrupt( I2C1_EV_IRQn );
            enable_interrupt( I2C1_ER_IRQn );
            break;
        case I2C2_BASE:
            enable_interrupt( I2C2_EV_IRQn );
            enable_interrupt( I2C2_ER_IRQn );            
            break;
        }

        i2c_->CR1 |= PE; // peripheral enable
        stream() << "freq: " << freq << std::endl;        
     }

     stream() << "CR1  : " << i2c_->CR1 << std::endl;
     stream() << "CR2  : " << i2c_->CR2 << std::endl;
     stream() << "OAR1 : " << i2c_->OAR1 << std::endl;
     stream() << "OAR2 : " << i2c_->OAR2 << std::endl;
     stream() << "SR1  : " << i2c_->SR1 << std::endl;
     stream() << "SR2  : " << i2c_->SR2 << std::endl;
     stream() << "CCR  : " << i2c_->CCR << std::endl;
     stream() << "TRISE: " << i2c_->TRISE << std::endl;
}

i2c&
i2c::operator << ( uint16_t d )
{
    size_t count = 3;
    while ( i2c_->CR1 & START ) {
        // stream() << "CR1 START CR1: " << i2c_->CR1 << " SR: " << i2c_->SR1 << ", " << i2c_->SR2 << "[" << count << "]" << std::endl;
        if ( --count == 0 )
            return *this;
    }
    i2c_->CR1 |= START | PE;
    i2c_->DR = d & 0xff;
    stream() << "i2c write: " << i2c_->DR << " SR:" << i2c_->SR1 << ", " << i2c_->SR2 << std::endl;
    return *this;
}

void
i2c::handle_event_interrupt()
{
    stream() << "i2c::handle_event_interrupt()" << std::endl;
}

void
i2c::handle_error_interrupt()
{
    stream() << "i2c::handle_error_interrupt()" << std::endl;
}

//static
void
i2c::interrupt_event_handler( i2c * _this )
{
    _this->handle_event_interrupt();
}

// static
void
i2c::interrupt_error_handler( i2c * _this )
{
    _this->handle_error_interrupt();
}

