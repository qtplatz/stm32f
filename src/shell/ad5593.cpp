/**************************************************************************
** Copyright (C) 2016-2017 Toshinobu Hondo, Ph.D.
** Copyright (C) 2016-2017 MS-Cheminformatics LLC, Toin, Mie Japan
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

using namespace ad5593;

bool
ad5593dev::write( uint8_t addr, uint16_t value )
{
    std::array< uint8_t, 3 > buf = { addr, uint8_t(value >> 8), uint8_t(value & 0xff) };
    if ( i2c_ ) {
        if ( use_dma_ )
            return i2c_->dma_transfer( address_, buf.data(), buf.size() );
        else 
            return i2c_->write( address_, buf.data(), buf.size() );
    }
    return false;
}

uint16_t
ad5593dev::read( uint8_t addr )
{
    if ( !(addr & 0xf0) )
        addr |= AD5593R_MODE_REG_READBACK;

    if ( i2c_ ) {
        uint8_t buf[ 2 ] = { 0 };
        
        if ( use_dma_ ) {
            return i2c_->dma_transfer( address_, &addr, 1 );
            if ( i2c_->dma_receive( address_, buf, sizeof(buf) ) ) {
                return uint16_t( buf[ 0 ] << 8 ) | buf[ 1 ];
            }
        } else {
            i2c_->write( address_, &addr, 1 );
            i2c_->read( address_, buf, sizeof(buf) );
            return uint16_t( buf[ 0 ] << 8 ) | buf[ 1 ];
        }
    }

    return -1;
}

void
ad5593dev::reset()
{
	write( AD5593R_REG_RESET, 0xdac );
}

void ad5593dev::reference_internal()
{
	write( AD5593R_REG_PD, 1 << 9 );
}

void
ad5593dev::reference_range_high()
{
	write( AD5593R_REG_CTRL, (1 << 4) | (1 << 5) );
}

const io& 
ad5593dev::operator []( uint8_t pin ) const
{
	return ios_[ pin ] ;
}

io::io() : gpio_out( 0 )
         , gpio_in( 0 )
         , gpio_set( 0 )
         , adc_en( 0 )
         , dac_en( 0 )
         , tristate( 0 )
         , pulldown( 0xff )
         , ident_( 0 )
         , parent_( 0 )
         , func_( ADC )
         , dir_( GPIO_INPUT )
{
}

io::io( const io& t ) : gpio_out( t.gpio_out )
                      , gpio_in( t.gpio_in )
                      , gpio_set( t.gpio_set )
                      , adc_en( t.adc_en )
                      , dac_en( t.dac_en )
                      , tristate( t.tristate )
                      , pulldown( t.pulldown)
                      , ident_( t.ident_ )
                      , parent_( t.parent_ )
                      , func_( t.func_ )
                      , dir_( t.dir_ )
{
}

template<> bool
io::isBitSet( uint16_t mask, uint8_t pos ) const
{
    return ( mask & (1 << pos) ) != 0;
}

template<> uint16_t
io::isBitSet( uint16_t mask, uint8_t pos) const
{
    return (mask & (1 << pos)) != 0;
}

void
io::setBit( uint16_t &mask ) const
{
    mask |= ( 1 << ident_ );
}

void
io::clearBit( uint16_t &mask ) const
{
    mask &= ~( 1 << ident_ );
}

void
io::setOrClear( uint16_t &mask, bool set ) const
{
    if (set)
        setBit(mask);
    else
        clearBit(mask);
}

io::io( ad5593dev& parent
        , uint8_t ident
        , ioFunction func ) : parent_( &parent )
                            , ident_( ident )
                            , func_( func )
{
	function(func_);
};

void
io::function( ioFunction func )
{
    if ( parent_ ) {
        pulldown = parent_->read( AD5593R_REG_PULLDOWN );
        tristate = parent_->read( AD5593R_REG_TRISTATE );
        dac_en = parent_->read( AD5593R_REG_DAC_EN );
        adc_en = parent_->read( AD5593R_REG_ADC_EN );
        gpio_set = parent_->read( AD5593R_REG_GPIO_SET );
        gpio_out = parent_->read( AD5593R_REG_GPIO_OUT_EN );
        gpio_in = parent_->read( AD5593R_REG_GPIO_IN_EN );

        clearBit(pulldown);
        clearBit(tristate);

        switch (func)  {
        case ADC:
            clearBit( dac_en );
            setBit( adc_en );
            clearBit( gpio_out );
            clearBit( gpio_in );
            break;
        case DAC:
            setBit( dac_en );
            clearBit( gpio_out );
            clearBit( gpio_in );
            break;
        case DAC_AND_ADC:
            setBit( adc_en );
            setBit( dac_en );
            clearBit( gpio_out );
            clearBit( gpio_in );
            break;
        case GPIO:
            clearBit( adc_en );
            clearBit( dac_en );
            break;
        case UNUSED_LOW:
            clearBit( adc_en );
            clearBit( dac_en );
            clearBit( gpio_set );
            setBit( gpio_out );
            clearBit( gpio_in );
            break;
        case UNUSED_HIGH:
            clearBit( adc_en );
            clearBit( dac_en );
            setBit( gpio_set );
            setBit( gpio_out );
            clearBit( gpio_in );
            break;
        case UNUSED_TRISTATE:
            clearBit( adc_en );
            clearBit( dac_en );
            clearBit( gpio_out );
            clearBit( gpio_in );
            setBit( tristate );
            break;
        case UNUSED_PULLDOWN:
            clearBit( adc_en );
            clearBit( dac_en );
            clearBit( gpio_out );
            clearBit( gpio_in );
            setBit( pulldown );
            break;
        };

        parent_->write( AD5593R_REG_PULLDOWN, pulldown );
        parent_->write( AD5593R_REG_TRISTATE, tristate );
        parent_->write( AD5593R_REG_DAC_EN, dac_en );
        parent_->write( AD5593R_REG_ADC_EN, adc_en );
        parent_->write( AD5593R_REG_GPIO_SET, gpio_set );
        parent_->write( AD5593R_REG_GPIO_OUT_EN, gpio_out );
        parent_->write( AD5593R_REG_GPIO_IN_EN, gpio_in );
    }
    func_ = func;
}

ioFunction
io::function()
{
	return func_;
}

void
io::direction( gpioDirection dir )
{
	switch ( func_ ) {
	case GPIO:
		gpio_out = parent_->read(AD5593R_REG_GPIO_OUT_EN);
		gpio_in = parent_->read(AD5593R_REG_GPIO_IN_EN);
        
		if (dir == GPIO_INPUT) {
			setBit(gpio_in);
			clearBit(gpio_out);
		} else if (dir == GPIO_OUTPUT)	{
			setBit(gpio_out);
			clearBit(gpio_in);
		}
		parent_->write(AD5593R_REG_GPIO_OUT_EN, gpio_out);
		parent_->write(AD5593R_REG_GPIO_IN_EN, gpio_in);
		dir_ = dir;
		return;
	}
}

void
io::direction( gpioDirection dir, uint16_t value )
{
	switch ( func_ )	{
	case GPIO:
		gpio_out = parent_->read(AD5593R_REG_GPIO_OUT_EN);
		gpio_in = parent_->read(AD5593R_REG_GPIO_IN_EN);
        
		if (dir == GPIO_INPUT)	{
			setBit(gpio_in);
			clearBit(gpio_out);
		} else if (dir == GPIO_OUTPUT)	{
			gpio_set = parent_->read(AD5593R_REG_GPIO_SET);

			setOrClear(gpio_set, value > 0);
			setBit(gpio_out);
			clearBit(gpio_in);

			parent_->write(AD5593R_REG_GPIO_SET, gpio_set);
		}

		parent_->write(AD5593R_REG_GPIO_OUT_EN, gpio_out);
		parent_->write(AD5593R_REG_GPIO_IN_EN, gpio_in);
		dir_ = dir;

		return;
	}
}

gpioDirection
io::direction()
{
	return dir_;
}

bool
io::set(uint16_t value)
{
	switch ( func_ )	{
	case DAC:
	case DAC_AND_ADC:
		parent_->write((AD5593R_MODE_DAC_WRITE | ident_), value );
		break;

	case GPIO:
		gpio_set = parent_->read( AD5593R_REG_GPIO_SET );
		setOrClear( gpio_set, value > 0 );
		parent_->write( AD5593R_REG_GPIO_SET, gpio_set );
        break;
	default:
		return false;
	};

    return true;
}

uint16_t
io::get()
{
    if ( parent_ ) {
        switch (this->func_) {
        case DAC:
            return parent_->read((AD5593R_REG_DAC_READBACK | ident_ ));
        
        case ADC:
        case DAC_AND_ADC:
            parent_->write(AD5593R_MODE_CONF | AD5593R_REG_ADC_SEQ, uint16_t(1 << ident_));
            return parent_->read( AD5593R_MODE_ADC_READBACK ) & 0x0fff;
	    case GPIO:
            gpio_out = parent_->read(AD5593R_REG_GPIO_OUT_EN);
            if (isBitSet(gpio_out, ident_)) {
                gpio_set = parent_->read(AD5593R_REG_GPIO_SET);
                return isBitSet< uint16_t >(gpio_set, ident_);
            } else	{
                return isBitSet< uint16_t >(parent_->read(AD5593R_MODE_GPIO_READBACK), ident_ );
            }
        default:
            break;
        }
    }
    return -1;
}


