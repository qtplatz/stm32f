/**************************************************************************
** Copyright (C) 2016-2018 MS-Cheminformatics LLC, Toin, Mie Japan
** Author: Toshinobu Hondo, Ph.D.
*
** Contact: toshi.hondo@qtplatz.com
**
** Commercial Usage
**
** Licensees holding valid MS-Cheminfomatics commercial licenses may use this file in
** accordance with the MS-Cheminformatics Commercial License Agreement provided with
** the Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and MS-Cheminformatics.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.TXT included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
**************************************************************************/

#include "bmp280.hpp"
#include "i2c.hpp"
#include "timer.hpp"
#include "scoped_spinlock.hpp"
#include "stm32f103.hpp"
#include <atomic>
#if defined __linux
#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#else
#include "stream.hpp"
#endif
#include "debug_print.hpp"

namespace bmp280 {
    std::atomic_flag __flag, __once_flag;
    BMP280 * BMP280::__instance;
    static uint8_t __bmp280_allocator[ sizeof(BMP280) ];
};

using namespace bmp280;

BMP280 *
BMP280::instance()
{
    return __instance;
}

// static
BMP280 *
BMP280::instance( stm32f103::i2c& t, int address )  // or 0x77
{
    if ( ! __once_flag.test_and_set() )
        __instance = new (__bmp280_allocator) BMP280( t, address );
    return __instance;    
}

BMP280::~BMP280()
{
}

BMP280::BMP280( stm32f103::i2c& t, int address ) : i2c_( &t )
                                                 , address_( address )
                                                 , has_callback_( false )
{
}

#if defined __linux
const std::system_error&
BMP280::error_code() const
{
    return i2c_->error_code();
}
#endif

BMP280::operator bool () const
{
    return *i2c_;
}

void
BMP280::measure()
{
    constexpr uint8_t meas = 1 << 5 | 1 << 2 | 3;
    constexpr uint8_t config = 1 << 5 | 1 << 2;
    // oversampling temp[7:5], press[4:2], power mode[1:0]
    if ( write( std::array< uint8_t, 4 >( { 0xf4, meas, 0xf5, config } ) ) ) {
        has_callback_ = true;
        stm32f103::timer_t< stm32f103::TIM2_BASE >().set_callback( handle_timer );
    }
}

void
BMP280::stop()
{
    if ( has_callback_ ) {
        stm32f103::timer_t< stm32f103::TIM2_BASE >::clear_callback();
        has_callback_ = false;
    }
}

bool
BMP280::write( const uint8_t * data, size_t size ) const
{
    scoped_spinlock<> lock( __flag );
    if ( i2c_ ) {
        if ( i2c_->has_dma( stm32f103::i2c::DMA_Tx ) ) {
            return i2c_->dma_transfer( address_, data, size );
        } else {
            return i2c_->write( address_, data, size );
        }
    }
    return false;
}

bool
BMP280::read(  uint8_t addr, uint8_t * data, size_t size ) const
{
    scoped_spinlock<> lock( __flag );
    if ( i2c_ ) {
        bool success( false );

        if ( i2c_->has_dma( stm32f103::i2c::DMA_Tx ) ) {
            success = i2c_->dma_transfer( address_, &addr, 1 );
        } else {
            success = i2c_->write( address_, &addr, 1 );
        }
        
        if ( success ) {
            std::array< uint8_t, 2 > buf = { 0 };
            
            if ( i2c_->has_dma( stm32f103::i2c::DMA_Rx ) ) {
                return i2c_->dma_receive( address_, data, size );
            } else {
                return i2c_->read( address_, data, size );
            }
        } else {
            stream(__FILE__,__LINE__) << "read -- write(" << addr << ") error\n";
        }
    }
    return false;
}

std::pair< uint64_t, uint64_t > 
BMP280::readout()
{
    std::array< uint8_t, 6 > data;
    if ( read( 0xf7, data.data(), data.size() ) ) {
        array_print( stream(__FILE__,__LINE__), data, data.size(), "bmp280 values : " );        
    }
}

//static
void
BMP280::handle_timer()
{
    if ( auto p = instance() )
        auto pair = p->readout();
}

