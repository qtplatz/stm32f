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

namespace bmp280 {

}


using namespace bmp280;

BMP280::~BMP280()
{
}

#if defined __linux
BMP280::BMP280( const char * device
                , int address ) : i2c_( std::make_unique< i2c_linux::i2c >() )
                                , address_( address )
                                , dirty_( true )
{
    i2c_->init( device, address );
}
#else
BMP280::BMP280( stm32f103::i2c& t, int address ) : i2c_( &t )
                                                 , address_( address )
                                                 , dirty_( true )
{
}
#endif

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

bool
BMP280::write( const uint8_t * data, size_t size ) const
{
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

