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

    struct trimming_parameter {
        template< typename T > void operator()( T& d, const uint8_t *& p ) const {
            d = uint16_t( p[0] ) | uint16_t( p[1] ) << 8;
            p += sizeof(T);
        }
    };
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
                                                 , has_trimming_parameter_( false )
                                                 , dig_T1(0), dig_T2(0), dig_T3(0)
                                                 , dig_P1(0), dig_P2(0), dig_P3(0), dig_P4(0)
                                                 , dig_P5(0), dig_P6(0), dig_P7(0), dig_P8(0)
                                                 , dig_P9(0)
                                                 , t_fine(0)
{
    trimming_parameter_readout();
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
BMP280::trimming_parameter_readout()
{
    std::array< uint8_t, 24 > data;
    
    if ( read( 0x88, data.data(), data.size() ) ) {
        has_trimming_parameter_ = true;
        const uint8_t * p = data.data();
        trimming_parameter()( dig_T1, p );
        trimming_parameter()( dig_T2, p );
        trimming_parameter()( dig_T3, p );
        trimming_parameter()( dig_P1, p );
        trimming_parameter()( dig_P2, p );
        trimming_parameter()( dig_P3, p );
        trimming_parameter()( dig_P4, p );
        trimming_parameter()( dig_P5, p );
        trimming_parameter()( dig_P6, p );
        trimming_parameter()( dig_P7, p );
        trimming_parameter()( dig_P8, p );
        trimming_parameter()( dig_P9, p );

        array_print( stream(__FILE__,__LINE__), data, data.size(), "trimming_parameter\t" );
        
        stream(__FILE__,__LINE__) << dig_T1 << ", " << dig_T2 << ", " << dig_T3 << std::endl;
        stream(__FILE__,__LINE__) << dig_P1 << ", " << dig_P2 << ", " << dig_P3 << ", " << dig_P4 << std::endl;
        stream(__FILE__,__LINE__) << dig_P5 << ", " << dig_P6 << ", " << dig_P7 << ", " << dig_P8 << std::endl;
        stream(__FILE__,__LINE__) << dig_P9 << std::endl;
    }
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

std::pair< uint32_t, uint32_t > 
BMP280::readout()
{
    std::array< uint8_t, 6 > data;
    if ( read( 0xf7, data.data(), data.size() ) ) {
        uint32_t press = uint32_t( data[0] ) << 12 | uint32_t( data[1] ) << 4 | data[2] & 0x0f;
        uint32_t temp  = uint32_t( data[3] ) << 12 | uint32_t( data[4] ) << 4 | data[5] & 0x0f;
        auto comp_temp = compensate_temperature_int32( temp );
        auto comp_press = compensate_pressure_int32( press );
        stream(__FILE__,__LINE__) << "press: " << int(press) << "/" << int( comp_press )
                                  << "\ttemp: " << int(temp) << "/" << int( comp_temp ) << std::endl;
        return { press, temp };
    }
    return { -1, -1 };
}

//static
void
BMP280::handle_timer()
{
    if ( auto p = instance() )
        auto pair = p->readout();
}

/*!
 *	@brief Reads actual temperature
 *	from uncompensated temperature
 *	@note Returns the value in 0.01 degree Centigrade
 *	@note Output value of "5123" equals 51.23 DegC.
 *
 *
 *
 *  @param v_uncomp_temperature_s32 : value of uncompensated temperature
 *
 *
 *
 *  @return Actual temperature output as s32
 *
 */
int32_t
BMP280::compensate_temperature_int32( int32_t temp )
{
	int32_t v_x1_u32r(0);
	int32_t v_x2_u32r(0);
	int32_t temperature(0);

	/* calculate true temperature*/
	/*calculate x1*/

	v_x1_u32r = ((((temp >> 3) - (static_cast<int32_t>(dig_T1) << 1))) * (static_cast<int32_t>(dig_T2))) >> 11;

	/*calculate x2*/
	v_x2_u32r = ( ((( (temp >> 4) - static_cast<int32_t>(dig_T1) )
                    * ( (temp >> 4) - static_cast<int32_t>(dig_T1) ) ) >> 12 )
                  * static_cast<int32_t>(dig_T3)) >> 14;

	/*calculate t_fine*/
	t_fine = v_x1_u32r + v_x2_u32r;

	/*calculate temperature*/
	temperature = (t_fine * 5 + 128)  >> 8;

	return temperature;
}

/*!
 *	@brief Reads actual pressure from uncompensated pressure
 *	and returns the value in Pascal(Pa)
 *	@note Output value of "96386" equals 96386 Pa =
 *	963.86 hPa = 963.86 millibar
 *
 *
 *
 *
 *  @param  v_uncomp_pressure_s32: value of uncompensated pressure
 *
 *
 *
 *  @return Returns the Actual pressure out put as s32
 *
 */
uint32_t
BMP280::compensate_pressure_int32( uint32_t press )
{
	int32_t v_x1_u32r(0);
	int32_t v_x2_u32r(0);
	uint32_t v_pressure_u32(0);

	/* calculate x1*/
	v_x1_u32r = (( int32_t(t_fine) ) >> 1 ) - 64000;

	/* calculate x2*/
	v_x2_u32r = (((v_x1_u32r >> 02)  * (v_x1_u32r >> 02)) >> 11) * (int32_t(dig_P6));
	v_x2_u32r = v_x2_u32r + ((v_x1_u32r * (int32_t(dig_P5))) << 01 );
	v_x2_u32r = (v_x2_u32r >> 02) + (( int32_t(dig_P4)) << 16);

	/* calculate x1*/
	v_x1_u32r = (((dig_P3 * (((v_x1_u32r >> 02) * (v_x1_u32r >> 02)) >> 13))  >> 03) + ((( int32_t(dig_P2)) * v_x1_u32r) >> 01 ))  >> 18;
	v_x1_u32r = ((((32768 + v_x1_u32r)) * ( int32_t(dig_P1))) >> 15);
    
	/* calculate pressure*/
	v_pressure_u32 = (( uint32_t((1048576) - press) - (v_x2_u32r >> 12))) * 3125;

	/* check overflow*/
	if ( v_pressure_u32 < 0x80000000)
		/* Avoid exception caused by division by zero */
		if ( v_x1_u32r != 0 )
			v_pressure_u32 = (v_pressure_u32 << 01) / ( uint32_t(v_x1_u32r));
		else
			return 0; // invalid
	else
        /* Avoid exception caused by division by zero */
        if ( v_x1_u32r != 0 )
            v_pressure_u32 = (v_pressure_u32 / uint32_t(v_x1_u32r)) * 2;
        else
            return 0; // BMP280_INVALID_DATA;

	/* calculate x1*/
	v_x1_u32r = (( int32_t(dig_P9)) * (int32_t(  ((v_pressure_u32 >> 03) * (v_pressure_u32 >> 03)) >> 13))) >> 12;

	/* calculate x2*/
	v_x2_u32r = ( ( int32_t(v_pressure_u32 >> 02)) * ( int32_t(dig_P8) ) )  >> 13;

	/* calculate true pressure*/
	v_pressure_u32 = int32_t(v_pressure_u32) + ( (v_x1_u32r + v_x2_u32r + dig_P7) >> 04 );
    
	return v_pressure_u32;
}
