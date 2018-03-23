// Copyright (C) 2018 MS-Cheminformatics LLC
// Licence: CC BY-NC
// Author: Toshinobu Hondo, Ph.D.
// Contact: toshi.hondo@qtplatz.com
//

#include "bmp280.hpp"
#include "i2c.hpp"
#include "stm32f103.hpp"
#include "stream.hpp"
#include "debug_print.hpp"
#include "utility.hpp"

extern void i2c_command( size_t argc, const char ** argv );
void mdelay( uint32_t );

void
bmp280_command( size_t argc, const char ** argv )
{
    static std::atomic_flag __once_flag;
    
    using namespace bmp280;

    if ( !__once_flag.test_and_set() ) {
        const char * argv [] = { "i2c", "dma", nullptr };
        i2c_command( 2, argv );
        BMP280::instance( *stm32f103::i2c_t< stm32f103::I2C1_BASE >::instance() );
    }

    BMP280& bmp280 = *BMP280::instance();

    std::array< uint8_t, 2 > id = { 0 };
    std::array< uint8_t, 3 > status = { 0 }; // status,ctrl_meas,config
    std::array< uint8_t, 6 > values = { 0 };

    if ( argc == 1 ) {
        if ( bmp280.read( 0xd0, id.data(), id.size() ) )
            array_print( stream(__FILE__,__LINE__), id, id.size(), "bmp280 id : " );
        else
            stream(__FILE__,__LINE__) << "id failed" << std::endl;
        
        if ( bmp280.read( 0xf3, status.data(), status.size() ) )
            array_print( stream(__FILE__,__LINE__), status, status.size(), "bmp280 status : " );
        else
            stream(__FILE__,__LINE__) << "status failed" << std::endl;
        
        if ( bmp280.read( 0xf7, values.data(), values.size() ) )
            array_print( stream(__FILE__,__LINE__), values, values.size(), "bmp280 values : " );
        else
            stream(__FILE__,__LINE__) << "values failed" << std::endl;
    }

    while ( --argc ) {
        ++argv;
        if ( strcmp( argv[0], "reset" ) == 0 ) {
            if ( bmp280.write( std::array< uint8_t, 2 >( { 0xe0, 0xb6 } ) ) ) {
                stream(__FILE__,__LINE__) << "reset OK" << std::endl;
            } else {
                stream(__FILE__,__LINE__) << "reset FAILED" << std::endl;
            }
        } else if ( strcmp( argv[0], "trim" ) == 0 ) {
            bmp280.trimming_parameter_readout();
        } else if ( strcmp( argv[0], "start" ) == 0 ) {
            bmp280.measure();
        } else if ( strcmp( argv[0], "stop" ) == 0 ) {
            bmp280.stop();
        } else if ( strcmp( argv[0], "--read" ) == 0 ) {
            size_t count = 10;
            if ( argc >= 1 && std::isdigit( *argv[1] ) ) {
                --argc; ++argv;
                count = strtod( argv[ 0 ] );
            }
            constexpr uint8_t meas = 1 << 5 | 1 << 2 | 3;
            constexpr uint8_t config = 1 << 5 | 1 << 2;
            if ( bmp280.write( std::array< uint8_t, 4 >( { 0xf4, meas, 0xf5, config } ) ) ) {
                while ( --count ) {
                    auto pair = bmp280.readout();
                    mdelay( 500 );
                }
            }
        }
    }
}
