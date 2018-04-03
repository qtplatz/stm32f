// Copyright (C) 2018 MS-Cheminformatics LLC
// Licence: CC BY-NC
// Author: Toshinobu Hondo, Ph.D.
// Contact: toshi.hondo@qtplatz.com
//

#include "i2c.hpp"
#include "i2c_string.hpp"
#include "stream.hpp"
#include "debug_print.hpp"
#include "utility.hpp"
#include <utility>

namespace stm32f103 {

    constexpr const char * __CR1__ [] = {
        "SWRST",        nullptr, "ALERT", "PEC",  "POS",      "ACK",   "STOP", "START"
        , "NO_STRETCH", "ENGC", "ENPEC", "ENARP", "SMB_TYPE", nullptr, "SMBUS", "PE"
    };
    
    constexpr const char * __CR2__ [] = {
        nullptr,   nullptr, nullptr, "LAST", "DMAEN", "ITBUFEN", "ITEVTEN", "ITERREN"
        , nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,  nullptr,   nullptr
    };
    
    constexpr const char * __SR1__ [] = {
        "SMB_ALERT", "TIMEOUT", nullptr, "PEC_ERR", "OVR",   "AF",  "ARLO", "BUS_ERR"
        , "TxE",     "RxNE",    nullptr, "STOPF",   "ADD10", "BTF", "ADDR", "SB"
    };
    
    constexpr const char * __SR2__ [] = {
        "DUALF", "SMB_HOST", "SMBDE_FAULT", "GEN_CALL", nullptr, "TRA", "BUSY", "MSL"
    };

    namespace i2cdebug {
    
        const char * CR1_to_string( uint32_t reg ) {
            stream() << "CR1 {";
            for ( int i = 0; i < 16; ++i ) {
                int k = 15 - i;
                if ( __CR1__[i] && ( reg & (1 << k) ) )
                    stream() << __CR1__[ i ] << ", ";
            }
            stream()  << "}";
            return "";
        }

        const char * CR2_to_string( uint32_t reg ) {
            stream() << "CR2 {";
            for ( int i = 3; i <= 7; ++i ) {
                int k = 15 - i;
                if ( __CR2__[ i ] && ( reg & (1 << k) ) )
                    stream() << __CR2__[ i ] << ", ";
            }
            stream() << "FREQ=" << int( reg & 0x3f ) << "}";
            return "";
        }

        const char * SR1_to_string( uint32_t reg ) {
            stream() << "SR1 {";
            for ( int i = 0; i < 16; ++i ) {
                int k = 15 - i;
                if ( __SR1__[i] && ( reg & (1 << k) ) )
                    stream() << __SR1__[ i ] << ", ";
            }
            stream() << "}";
            return "";
        }

        const char * SR2_to_string( uint32_t reg ) {
            stream() << "SR2 {";
            if ( reg & 0xf0 )
                stream() << "PEC=" << int( ( reg >> 8 ) & 0xff ) << ", ";
            for ( int i = 0; i <= 7; ++i ) {
                int k = 7 - i;
                if ( __SR2__[ i ] && ( reg & (1 << k) ) )
                    stream() << __SR2__[ i ] << ", ";
            }
            stream() << "}";
            return "";
        }

        const char * status32_to_string( uint32_t reg ) {
            SR2_to_string( reg >> 16 );
            stream() << " ";
            SR1_to_string( reg & 0xffff );
            return "";
        }
    }

    struct i2c_register {
        const char * name;
        uint32_t mask;
        stream& (*details)( stream& o, uint16_t, volatile I2C * );
    };

    constexpr i2c_register __register_names [] = {
        { "CR1", 0x4004,     &i2c_string::CR1   }
        , { "CR2", 0xe0c0,   &i2c_string::CR2   }
        , { "OAR1", 0x7800,  &i2c_string::OAR1  }
        , { "OAR2", 0xff00,  &i2c_string::OAR2  }
        , { "DR", 0xff00,    &i2c_string::DR    }
        , { "SR1", 0x2020,   &i2c_string::SR1   }
        , { "SR2", 0x0080,   &i2c_string::SR2   }
        , { "CCR", 0x3000,   &i2c_string::CCR   }
        , { "TRISE", 0xffc0, &i2c_string::TRISE }
    };
    
}

using namespace stm32f103;

stream&
i2c_string::CR1( stream& o, uint16_t reg, volatile I2C * )
{
    o << "CR1 {";
    for ( int i = 0; i < 16; ++i ) {
        int k = 15 - i;
        if ( __CR1__[i] && ( reg & (1 << k) ) )
            o << __CR1__[ i ] << ", ";
    }
    o  << "}";
    return o;
}

stream&
i2c_string::CR2( stream& o, uint16_t reg, volatile I2C * )
{
    return print()( o, std::bitset< 16 >( reg ), "CR2  : {", __CR2__, 3, 7 ) << "FREQ=" << int( reg & 0x3f ) << "}";
}

stream&
i2c_string::SR1( stream& o, uint16_t reg, volatile I2C * )
{
    return print()( o, std::bitset< 16 >( reg ), "SR1  : {", __SR1__ ) << "}";
}

stream&
i2c_string::SR2( stream& o, uint16_t reg, volatile I2C * )
{
    o << "SR2  : {";
    if ( reg & 0xf0 )
        o << "PEC=" << int( ( reg >> 8 ) & 0xff ) << ", ";    
    return print()( o, std::bitset< 16 >( reg ), "", __SR2__, 0, 7 ) << "}";
}

stream&
i2c_string::OAR1( stream& o, uint16_t reg, volatile I2C * )
{
    o << "OAR1 : {own address = " << int( reg >> 1) << "}";
    return o;
}

stream&
i2c_string::OAR2( stream& o, uint16_t reg, volatile I2C * )
{
    return o;    
}

stream&
i2c_string::CCR( stream& o, uint16_t reg, volatile I2C * )
{
    o << "CCR  : {" << int( reg & 0x7ff ) << "}\t";
    return o;
}

stream&
i2c_string::DR( stream& o, uint16_t reg, volatile I2C * )
{
    return o;
}

stream&
i2c_string::TRISE( stream& o, uint16_t reg, volatile I2C * _ )
{
    o << "TRISE: {" << int( reg ) << "(" << int( ( reg - 1 )/( _->CR2 & 0x3f ) ) << "us)}";
    return o;
}

stream&
i2c_string::status32( stream&& o, uint32_t reg, volatile I2C * _ )
{
    SR2( o, reg >> 16, _ ) << " "; SR1( o, reg & 0xffff, _ );
    return o;
}

stream&
i2c_string::print_registers( stream&& o, volatile I2C * _ )
{
    o << "See p773, 26.5" << std::endl;

    auto reg = reinterpret_cast< volatile uint32_t * >( _ ) + countof( __register_names ) - 1;

    // read reg in reverse order (can't read SR1 then SR2 that will clar ADDR bit)
    std::array< uint16_t, countof( __register_names ) > temp;
    std::transform( temp.rbegin(), temp.rend(), temp.rbegin(), [&]( auto ){ return *reg--; } );

    size_t i = 0;
    for ( auto& name: __register_names ) {
        print()( std::move( o ), std::bitset< 16 >( temp[ i ] ), name.name, std::bitset<16>( name.mask ) )
            << "\t" << uint16_t( temp[ i ] )
            << "\t" << ( name.details( o, temp[i], _ ), "" )
            << std::endl;
        ++i;
    }
}

