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
#include "condition_wait.hpp"
#include "debug_print.hpp"
#include "i2c.hpp"
#include "scoped_spinlock.hpp"
#include "utility.hpp"
#include "stream.hpp"
#if defined __linux
#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#endif


namespace ad5593 {

    enum AD5593R_REG : uint8_t {
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

    enum AD5593R_MODE : uint8_t {
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

    constexpr static const char * __reg_name [] = {
        "AD5593R_REG_NOOP        "
        , "AD5593R_REG_DAC_READBACK"
        , "AD5593R_REG_ADC_SEQ     "
        , "AD5593R_REG_CTRL        "
        , "AD5593R_REG_ADC_EN      "
        , "AD5593R_REG_DAC_EN      "
        , "AD5593R_REG_PULLDOWN    "
        , "AD5593R_REG_LDAC        "
        , "AD5593R_REG_GPIO_OUT_EN "
        , "AD5593R_REG_GPIO_SET    "
        , "AD5593R_REG_GPIO_IN_EN  "
        , "AD5593R_REG_PD          "
        , "AD5593R_REG_OPEN_DRAIN  "
        , "AD5593R_REG_TRISTATE    "
        , "AD5593R_REG_RESET       "
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

    constexpr static const char * __io_function_name [] = {
        "ADC", "DAC", "GPIO", "HIGH", "LOW", "TRIST", "PULLDN", "n/a"
    };

    template< AD5593R_REG ... regs > struct set_io_function;

    template< AD5593R_REG reg > struct set_io_function< reg >  { // last
        void operator()( std::array< std::bitset< number_of_pins >, number_of_functions >& masks, int pin ) const {
            masks[ index_of< reg >::value ].set( pin );
        }
    };

    template< AD5593R_REG first, AD5593R_REG ... regs > struct set_io_function< first, regs ...> {
        void operator()( std::array< std::bitset< number_of_pins >, number_of_functions >& masks, int pin ) const {
            masks[ index_of< first >::value ].set( pin );
        }
    };

    template< AD5593R_REG ... regs > struct reset_io_function;

    template< AD5593R_REG last > struct reset_io_function< last >  { // last
        void operator()( std::array< std::bitset< number_of_pins >, number_of_functions >& masks, int pin ) const {
            masks[ index_of< last >::value ].reset( pin );
        }
    };

    template< AD5593R_REG first, AD5593R_REG ... regs > struct reset_io_function< first, regs ...> {
        void operator()( std::array< std::bitset< number_of_pins >, number_of_functions >& masks, int pin ) const {
            masks[ index_of< first >::value ].reset( pin );
            reset_io_function< regs ... >()( masks, pin );
        }
    };

    struct io_function {
        AD5593R_IO_FUNCTION operator()( const std::array< std::bitset< number_of_pins >, number_of_functions >& masks, int pin ) const {
            if ( masks[ index_of< AD5593R_REG_DAC_EN >::value ].test( pin ) )
                return DAC;
            if ( masks[ index_of< AD5593R_REG_ADC_EN >::value ].test( pin ) )
                return ADC;
            if ( masks[ index_of< AD5593R_REG_GPIO_OUT_EN >::value ].test( pin ) ||
                 masks[ index_of< AD5593R_REG_GPIO_SET >::value ].test( pin ) ||
                 masks[ index_of< AD5593R_REG_GPIO_IN_EN >::value ].test( pin ) )
                return GPIO;
            if ( masks[ index_of< AD5593R_REG_PULLDOWN >::value ].test( pin ) )
                return UNUSED_PULLDOWN;
            if ( masks[ index_of< AD5593R_REG_TRISTATE >::value ].test( pin ) )
                return UNUSED_TRISTATE;
            return INDETERMINATE;
        }
    };

    std::atomic_flag AD5593::mutex_;
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
}
#else
AD5593::AD5593( stm32f103::i2c& t, int address ) : i2c_( &t )
                                                 , address_( address )
                                                 , functions_{ UNUSED_PULLDOWN }
                                                 , dirty_( true )
{
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
AD5593::write( const uint8_t * data, size_t size ) const
{
    bool success( false );
    if ( i2c_ ) {
        if ( i2c_->has_dma( stm32f103::i2c::DMA_Tx ) )
            success = i2c_->dma_transfer( address_, data, size );
        else
            success = i2c_->write( address_, data, size );
    }
    if ( !success )
        i2c_->print_result( stream(__FILE__,__LINE__,__FUNCTION__) ) << std::endl;
    return success;
}

bool
AD5593::read(  uint8_t addr, uint8_t * data, size_t size ) const
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
                // workaround -- AD5593 often cause a read timeout -- retry up to 5 times --
                if ( condition_wait( 5 )( [&]{ return i2c_->dma_receive( address_, data, size ); } ) )
                    return true;
                else
                    i2c_->print_result( stream(__FILE__,__LINE__) ) << "\tread -- dma_recieve(" << addr << ") error\n";
            } else {
                return i2c_->read( address_, data, size );
            }
        } else {
            i2c_->print_result( stream(__FILE__,__LINE__) ) << "\tread -- write(" << addr << ") error\n";
        }
    }
    return false;
}

uint8_t
AD5593::adc_enabled() const
{
    return static_cast< uint8_t >( bitmaps_[ ADC ].to_ulong() & 0x0ff );
}

bool
AD5593::read_adc_sequence ( uint16_t * data, size_t size ) const
{
    scoped_spinlock<> lock( mutex_ );

    if ( read( AD5593R_MODE_ADC_READBACK, reinterpret_cast< uint8_t *>( data ), size * sizeof( uint16_t ) ) ) {

        std::transform( data, data + size, data, []( const uint16_t& b ) {
                auto p = reinterpret_cast< const uint8_t * >(&b); return uint16_t( p[0] ) << 8 | p[1]; } );

        return true;

    } else {

        i2c_->print_result( stream(__FILE__,__LINE__) ) << "\tread_adc_sequence failed.\n";
        return false;
    }
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
    default:
        return false;
    };
    return true;
}

AD5593R_IO_FUNCTION
AD5593::function( int pin ) const
{
    return functions_[ pin ];
}

bool
AD5593::reset()
{
    scoped_spinlock<> lock( mutex_ );
    return write( std::array< uint8_t, 1 >{ AD5593R_REG_RESET } );
}

bool
AD5593::fetch()
{
    scoped_spinlock<> lock( mutex_ );
    
    uint32_t failed(0);
    size_t i = 0;

    for ( auto reg: __fetch_reg_list ) {
        std::array< uint8_t, 2 > data;
        if ( read( AD5593R_MODE_REG_READBACK | reg, data ) ) {
            bitmaps_[ i++ ] = data[ 1 ];
        } else {
            stream(__FILE__,__LINE__) << "fetch failed to read " << __fetch_reg_name[ i ] << std::endl;
            failed |= 1 << i;
            bitmaps_[ i++ ] = uint16_t( -1 );
        }
    }

    dirty_ = !failed;

    if ( failed == 0 ) {
        for ( int pin = 0; pin < number_of_pins; ++pin )
            functions_[ pin ] = io_function()( bitmaps_, pin );
    }
        
    return !failed;
}

bool
AD5593::commit()
{
    scoped_spinlock<> lock( mutex_ );

    size_t i = 0;
    for ( auto reg: __fetch_reg_list ) {
        auto x = bitmaps_[ i++ ].to_ulong();
        if ( ! write( std::array< uint8_t, 3 >{ uint8_t( AD5593R_MODE_CONF | reg ), uint8_t( x >> 8 ), uint8_t( x ) } ) ) {
            i2c_->print_result( stream(__FILE__,__LINE__,__FUNCTION__) ) << std::endl;
            return false;
        }
    }
    dirty_ = false;
    return true;
}

bool
AD5593::set_value( int pin, uint16_t value )
{
    scoped_spinlock<> lock( mutex_ );
    
    switch( functions_.at( pin ) ) {
    case DAC:
        do {
            std::array< uint8_t, 3 > cmd = { uint8_t( AD5593R_MODE_DAC_WRITE | pin ), uint8_t( value >> 8 ), uint8_t( value ) };
            if ( write( cmd.data(), cmd.size() ) ) {
                std::array< uint8_t, 2 > data;
                if ( read( AD5593R_MODE_DAC_READBACK | pin, data ) ) {
                    uint16_t xvalue = uint16_t( data[0] & 0x0f ) << 8 | data[1];
                    auto xpin = data[ 0 ] >> 4;
                    if ( xpin == ( pin | 0x8 ) && xvalue == value ) {
                        return true;
                    } else {
                        print()( stream(), cmd, cmd.size(), "ERROR: DAC\t" ) << "\t";
                        print()( stream(), data, data.size(), "\t" ) << "\t" << (xpin & 07) << " " << xvalue << std::endl;
                    }
                    return true;
                }
            }
            i2c_->print_result( stream(__FILE__,__LINE__,__FUNCTION__) ) << std::endl;
        } while( 0 );
        break;
    default:
        break;
    }
    return false;
}

uint16_t
AD5593::value( int pin ) const
{
    scoped_spinlock<> lock( mutex_ );
    std::array< uint8_t, 2 > data;
    
    switch( functions_.at( pin ) ) {
    case ADC:
        if ( read( AD5593R_MODE_ADC_READBACK | pin, data ) ) {
            return uint16_t( data[0] ) << 8 | data[ 1 ];
        }
    case DAC:
        if ( read( AD5593R_MODE_DAC_READBACK | pin, data ) ) {
            return uint16_t( data[0] ) << 8 | data[ 1 ];
        }
    }
    return -1;
}

bool
AD5593::set_adc_sequence( uint16_t sequence )
{
    scoped_spinlock<> lock( mutex_ );
    return write( std::array< uint8_t, 3 >{ AD5593R_REG_ADC_SEQ, uint8_t( sequence >> 8 ), uint8_t( sequence ) } );
}

uint16_t
AD5593::adc_sequence() const
{
    std::array< uint8_t, 2 > data;
    if ( read( AD5593R_MODE_REG_READBACK | AD5593R_REG_ADC_SEQ, data ) ) {
        return uint16_t( data[0] ) << 8 | data[1];
    }
    return (-1);
}

void
AD5593::print_values( stream&& o ) const
{
    for ( int pin = 0; pin < number_of_pins; ++pin ) {
        if ( functions_[ pin ] < countof( __io_function_name ) )
            o << "pin #" << pin << " = " << __io_function_name[ functions_[ pin ] ] << "\t";
        else
            o << "pin #" << pin << " = " << functions_[ pin ] << "\t";
        uint16_t v = value( pin );
        if ( functions_[ pin ] == DAC ) {
            o << ( v & 80 ? "" : "FORMAT ERROR: ") << (( v >> 12 ) & 03) << "\t" << ( v & 0x0fff ) << std::endl;
        } else if ( functions_[ pin ] == ADC ) {
            o << ( v & 80 ? "FORMAT ERROR: " : "") << (( v >> 12 ) & 03) << "\t" << ( v & 0x0fff ) << std::endl;
        } else {
            o << v << std::endl;
        }
    }
}

void
AD5593::print_registers( stream&& o ) const
{
    o << "----------- AD5593 REGISTERS ------------\n";    
    size_t i = 0;
    for ( auto& reg: __reg_list ) {
        std::array< uint8_t, 2 > data { 0 };
        if ( read( AD5593R_MODE_REG_READBACK | reg, data ) ) {
            std::bitset< 16 > bits( uint16_t( data[0] ) << 8 | data[1] );
            print()( o, bits, __reg_name[i] ) << std::endl;
        } else {
            o << __reg_name[i] << " read failed\n";
        }
        ++i;
    }
}

void
AD5593::print_config( stream&& o ) const
{
    size_t i = 0;

    o << "----------- AD5593 CONFIG ------------\n";
    for ( auto& bitmap: bitmaps_ ) {
        print()( o, bitmap, __fetch_reg_name[ i++ ] );
        o << "\t0x" << uint16_t( bitmap.to_ulong() ) << std::endl;        
    }

    for ( int pin = 0; pin < number_of_pins; ++pin ) {
        if ( functions_[ pin ] < countof( __io_function_name ) )
            o << "pin #" << pin << " = " << __io_function_name[ functions_[ pin ] ] << ( pin == 3 ? "\n" : "\t" );
        else
            o << "pin #" << pin << " = " << functions_[ pin ] << ( pin == 3 ? "\n" : "\t" );
    }
    o << std::endl;
}

void
AD5593::print_adc_sequence( stream&& o, uint16_t * sequence, size_t size ) const
{
    constexpr int Vref = 2500;
    std::for_each( sequence, sequence + size, [&]( const uint16_t& a ){
            int pin = a >> 12;
            int v = int( a & 0x0fff ) * Vref / 4096;
            if ( pin != 8 ) {
                o << "[" << int( a >> 12 ) << "] " << v << "\t";
                if ( v < 999 )
                    o << "\t";
            } else {
                auto temp = ( 25000 + int(a & 0x0fff) - 820000 / 2654 ) / 10;
                auto minor = temp % 100;
                o << "[T] " << ( temp / 100 ) << "." << ( minor < 10 ? "0" : "" ) << minor << "(degC)\t";
            }
        });
    o << "\n";
}
