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

#include "ad5593.hpp"
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

namespace ad5593 {

    enum AD5593R_REG {
        AD5593R_REG_NOOP           = 0x0
        , AD5593R_REG_DAC_READBACK = 0x1
        , AD5593R_REG_ADC_SEQ      = 0x2
        , AD5593R_REG_CTRL         = 0x3
        , AD5593R_REG_ADC_EN       = 0x4
        , AD5593R_REG_DAC_EN       = 0x5
        , AD5593R_REG_PULLDOWN     = 0x6
        , AD5593R_REG_LDAC         = 0x7
        , AD5593R_REG_GPIO_OUT_EN  = 0x8
        , AD5593R_REG_GPIO_SET     = 0x9
        , AD5593R_REG_GPIO_IN_EN   = 0xa
        , AD5593R_REG_PD           = 0xb
        , AD5593R_REG_OPEN_DRAIN   = 0xc
        , AD5593R_REG_TRISTATE     = 0xd
        , AD5593R_REG_RESET        = 0xf
    };

    enum AD5593R_MODE {
        AD5593R_MODE_CONF            = (0 << 4)
        , AD5593R_MODE_DAC_WRITE     = (1 << 4)
        , AD5593R_MODE_ADC_READBACK  = (4 << 4)
        , AD5593R_MODE_DAC_READBACK  = (5 << 4)
        , AD5593R_MODE_GPIO_READBACK = (6 << 4)
        , AD5593R_MODE_REG_READBACK  = (7 << 4)
    };

    constexpr static AD5593R_REG __reg_list [] = {
        AD5593R_REG_NOOP          
        , AD5593R_REG_DAC_READBACK
        , AD5593R_REG_ADC_SEQ     
        , AD5593R_REG_CTRL        
        , AD5593R_REG_ADC_EN      
        , AD5593R_REG_DAC_EN      
        , AD5593R_REG_PULLDOWN    
        , AD5593R_REG_LDAC        
        , AD5593R_REG_GPIO_OUT_EN 
        , AD5593R_REG_GPIO_SET    
        , AD5593R_REG_GPIO_IN_EN  
        , AD5593R_REG_PD          
        , AD5593R_REG_OPEN_DRAIN  
        , AD5593R_REG_TRISTATE    
        , AD5593R_REG_RESET
    };

    constexpr static AD5593R_REG __fetch_reg_list [] = {
        AD5593R_REG_ADC_EN        // bitmaps_[0]
        , AD5593R_REG_DAC_EN      // bitmaps_[1]
        , AD5593R_REG_PULLDOWN    // bitmaps_[2]
        , AD5593R_REG_GPIO_OUT_EN // bitmaps_[3]
        , AD5593R_REG_GPIO_SET    // bitmaps_[4]
        , AD5593R_REG_GPIO_IN_EN  // bitmaps_[5]
        , AD5593R_REG_TRISTATE    // bitmaps_[6]
    };

    template< AD5593R_REG > struct index_of {
        constexpr static size_t value = 0;
    };

    template<> struct index_of<AD5593R_REG_ADC_EN>      { constexpr static size_t value = 0; };
    template<> struct index_of<AD5593R_REG_DAC_EN>      { constexpr static size_t value = 1; };
    template<> struct index_of<AD5593R_REG_PULLDOWN>    { constexpr static size_t value = 2; };
    template<> struct index_of<AD5593R_REG_GPIO_OUT_EN> { constexpr static size_t value = 3; };
    template<> struct index_of<AD5593R_REG_GPIO_SET>    { constexpr static size_t value = 4; };
    template<> struct index_of<AD5593R_REG_GPIO_IN_EN>  { constexpr static size_t value = 5; };
    template<> struct index_of<AD5593R_REG_TRISTATE>    { constexpr static size_t value = 6; };

    static_assert(AD5593R_REG_ADC_EN == __fetch_reg_list[ index_of< AD5593R_REG_ADC_EN >::value ]);
    static_assert(AD5593R_REG_DAC_EN == __fetch_reg_list[ index_of< AD5593R_REG_DAC_EN >::value ]);
    static_assert(AD5593R_REG_PULLDOWN == __fetch_reg_list[ index_of< AD5593R_REG_PULLDOWN >::value ]);
    static_assert(AD5593R_REG_GPIO_OUT_EN == __fetch_reg_list[ index_of< AD5593R_REG_GPIO_OUT_EN >::value ]);
    static_assert(AD5593R_REG_GPIO_SET == __fetch_reg_list[ index_of< AD5593R_REG_GPIO_SET >::value ]);
    static_assert(AD5593R_REG_GPIO_IN_EN == __fetch_reg_list[ index_of< AD5593R_REG_GPIO_IN_EN >::value ]);
    static_assert(AD5593R_REG_TRISTATE == __fetch_reg_list[ index_of< AD5593R_REG_TRISTATE >::value ]);

    constexpr static const char * __fetch_reg_name [] = {
        "AD5593R_REG_ADC_EN"        // bitmaps_[0]
        , "AD5593R_REG_DAC_EN"      // bitmaps_[1]
        , "AD5593R_REG_PULLDOWN"    // bitmaps_[2]
        , "AD5593R_REG_GPIO_OUT_EN" // bitmaps_[3]
        , "AD5593R_REG_GPIO_SET"    // bitmaps_[4]
        , "AD5593R_REG_GPIO_IN_EN"  // bitmaps_[5]
        , "AD5593R_REG_TRISTATE"    // bitmaps_[6]
    };

    template< AD5593R_REG ... regs > struct set_io_function;

    template< AD5593R_REG reg > struct set_io_function< reg >  { // last
        void operator()( std::array< std::bitset< 8 >, 7 >& masks, int pin ) const {
            masks[ index_of< reg >::value ].set( pin );
        }
    };

    template< AD5593R_REG first, AD5593R_REG ... regs > struct set_io_function< first, regs ...> {
        void operator()( std::array< std::bitset< 8 >, 7 >& masks, int pin ) const {
            masks[ index_of< first >::value ].set( pin );
        }
    };

    template< AD5593R_REG ... regs > struct reset_io_function;

    template< AD5593R_REG last > struct reset_io_function< last >  { // last
         void operator()( std::array< std::bitset< 8 >, 7 >& masks, int pin ) const {
             masks[ index_of< last >::value ].reset( pin );
             // std::cout << "reset<last>(" << pin << ", " << __fetch_reg_name[ index_of< last >::value ] << ")";
             // std::cout << "\t=" << masks[ index_of< last >::value ].to_ulong() << std::endl;
         }
    };

    template< AD5593R_REG first, AD5593R_REG ... regs > struct reset_io_function< first, regs ...> {
        void operator()( std::array< std::bitset< 8 >, 7 >& masks, int pin ) const {
            masks[ index_of< first >::value ].reset( pin );
            reset_io_function< regs ... >()( masks, pin );
            // std::cout << "reset(" << pin << ", " << __fetch_reg_name[ index_of< first >::value ] << ")";
            // std::cout << "\t=" << masks[ index_of< first >::value ].to_ulong() << std::endl;
        }
    };

    struct io_function {
        AD5593R_IO_FUNCTION operator()( const std::array< std::bitset< 8 >, 7 >& masks, int pin ) const {
        }
    };
    
}

using namespace ad5593;

AD5593::~AD5593()
{
}

#if defined __linux
AD5593::AD5593( const char * device
                , int address ) : i2c_( std::make_unique< i2c_linux::i2c >() )
                                , address_( address )
                                , functions_{ UNUSED_PULLDOWN }
                                , dirty_( true )
{
    i2c_->init( device, address );
    fetch();
}
#else
AD5593::AD5593( stm32f103::i2c& t, int address ) : i2c_( &t )
                                                 , address_( address )
                                                 , functions_{ UNUSED_PULLDOWN }
                                                 , dirty_( true )
{
    fetch();
}
#endif

#if defined __linux
const std::system_error&
AD5593::error_code() const
{
    return i2c_->error_code();
}
#endif

AD5593::operator bool () const
{
    return *i2c_;
}

bool
AD5593::write( uint8_t addr, uint16_t value ) const
{
    if ( i2c_ ) {
        std::array< uint8_t, 3 > buf = { addr, uint8_t(value >> 8), uint8_t(value & 0xff) };
        return i2c_->write( address_, buf.data(), buf.size() );
    }
    return false;
}

uint16_t
AD5593::read( uint8_t addr ) const
{
    if ( i2c_ ) {
        if ( i2c_->write( address_, &addr, 1 ) ) {
            
            std::array< uint8_t, 2 > buf = { 0 };
            if ( i2c_->read( address_, buf.data(), buf.size() ) )
                return uint16_t( buf[ 0 ] ) << 8 | buf[ 1 ];
        } else {
            stream() << "read -- write(" << addr << ") error\n"; 
        }
    }
    return -1;
}

bool
AD5593::set_function( int pin, AD5593R_IO_FUNCTION f )
{
    dirty_ = true;
    functions_[ pin ] = f;
    switch( f ) {
    case ADC:
        set_io_function< AD5593R_REG_ADC_EN >()( bitmaps_, pin );
        reset_io_function< AD5593R_REG_PULLDOWN
                           , AD5593R_REG_DAC_EN
                           , AD5593R_REG_GPIO_OUT_EN >()( bitmaps_, pin );        
        break;
    case DAC:
        set_io_function< AD5593R_REG_DAC_EN >()( bitmaps_, pin );
        reset_io_function< AD5593R_REG_PULLDOWN
                           , AD5593R_REG_GPIO_OUT_EN
                           ,  AD5593R_REG_GPIO_IN_EN >()( bitmaps_, pin );
        break;
    case GPIO:
        reset_io_function< AD5593R_REG_PULLDOWN
                           , AD5593R_REG_ADC_EN
                           , AD5593R_REG_DAC_EN >()( bitmaps_, pin );
        break;
    case UNUSED_LOW:
        set_io_function< AD5593R_REG_GPIO_OUT_EN >()( bitmaps_, pin );
        reset_io_function< AD5593R_REG_PULLDOWN
                           , AD5593R_REG_ADC_EN
                           , AD5593R_REG_DAC_EN
                           , AD5593R_REG_GPIO_SET
                           , AD5593R_REG_GPIO_IN_EN >()( bitmaps_, pin );        
        break;
    case UNUSED_HIGH:
        set_io_function< AD5593R_REG_GPIO_SET, AD5593R_REG_GPIO_OUT_EN >()( bitmaps_, pin );
        reset_io_function< AD5593R_REG_PULLDOWN
                           , AD5593R_REG_ADC_EN
                           , AD5593R_REG_DAC_EN
                           , AD5593R_REG_GPIO_SET
                           , AD5593R_REG_GPIO_IN_EN >()( bitmaps_, pin );        
        break;
    case UNUSED_TRISTATE:
        set_io_function< AD5593R_REG_TRISTATE >()( bitmaps_, pin );
        reset_io_function< AD5593R_REG_PULLDOWN
                           , AD5593R_REG_ADC_EN
                           , AD5593R_REG_DAC_EN
                           , AD5593R_REG_GPIO_OUT_EN
                           , AD5593R_REG_GPIO_IN_EN >()( bitmaps_, pin );        
        break;
    case UNUSED_PULLDOWN:
        set_io_function< AD5593R_REG_PULLDOWN >()( bitmaps_, pin );
        reset_io_function< AD5593R_REG_ADC_EN
                           , AD5593R_REG_DAC_EN
                           , AD5593R_REG_GPIO_OUT_EN
                           , AD5593R_REG_GPIO_SET
                           , AD5593R_REG_TRISTATE
                           , AD5593R_REG_GPIO_IN_EN >()( bitmaps_, pin );
        break;
    };
}

AD5593R_IO_FUNCTION
AD5593::function( int pin ) const
{
    return functions_[ pin ];
}

bool
AD5593::fetch()
{
    size_t i = 0;
    for ( auto reg: __fetch_reg_list ) {
        auto value = this->read( reg | AD5593R_MODE_REG_READBACK );
        if ( value == (-1) )
            return false;
        bitmaps_[ i++ ] = uint8_t( value );
    }
    dirty_ = false;
}

void
AD5593::print_config() const
{
    size_t i = 0;
#if defined __linux
    for ( auto& bitmap: bitmaps_ )
        std::cout << __fetch_reg_name[ i++ ] << "\t" << bitmap.to_string() << "\t" << bitmap.to_ulong() << std::endl;
#else
    stream(__FILE__,__LINE__) << "----------- AD5593 CONFIG ------------\n";
    for ( auto& bitmap: bitmaps_ ) {
        stream() << __fetch_reg_name[ i++ ] << "\t";
        for ( auto i = 0; i < bitmap.size(); ++i )
            stream() << bitmap.test( bitmap.size() - i - 1 );
        stream() << "\t" << bitmap.to_ulong() << std::endl;        
    }
#endif
}

bool
AD5593::commit()
{
    size_t i = 0;
    for ( auto reg: __fetch_reg_list )
        this->write( reg, static_cast< uint16_t >( bitmaps_[ i++ ].to_ulong() ) );
    dirty_ = false;
    return true;
}

bool
AD5593::set_value( int pin, uint16_t value )
{
    switch( functions_.at( pin ) ) {
    case DAC:
        return this->write( AD5593R_MODE_DAC_WRITE | pin, value );
    case ADC:
        break;
    }
    return false;
}

uint16_t
AD5593::value( int pin ) const
{
    uint16_t value = 0;

    switch( functions_.at( pin ) ) {
    case ADC:
        return this->read( AD5593R_MODE_ADC_READBACK | pin ) & 0x0fff;
    case DAC:
        return this->read( AD5593R_MODE_DAC_READBACK | pin ) & 0x0fff;
    }
    return -1;
}
