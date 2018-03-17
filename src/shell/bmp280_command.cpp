// Copyright (C) 2018 MS-Cheminformatics LLC
// Licence: CC BY-NC
// Author: Toshinobu Hondo, Ph.D.
// Contact: toshi.hondo@qtplatz.com
//

#include "bmp280.hpp"
#include "i2c.hpp"
#include "stream.hpp"
#include "debug_print.hpp"
#include "utility.hpp"

extern stm32f103::i2c __i2c0, __i2c1;
extern void i2c_command( size_t argc, const char ** argv );

void
bmp280_command( size_t argc, const char ** argv )
{
    using namespace bmp280;

    BMP280 bmp280( __i2c0, 0x76 );
    
    if ( !__i2c0 ) {
        const char * argv [] = { "i2c", nullptr };
        i2c_command( 1, argv );
    }

    std::array< uint8_t, 2 > id = { 0 };
    std::array< uint8_t, 3 > status = { 0 }; // status,ctrl_meas,config
    std::array< uint8_t, 6 > values = { 0 };

    if ( bmp280.read( 0xd0, id.data(), id.size() ) )
        debug_print( stream(__FILE__,__LINE__), id, id.size(), "bmp280 id : " );
    else
        stream(__FILE__,__LINE__) << "id failed" << std::endl;
    
    if ( bmp280.read( 0xf3, status.data(), status.size() ) )
        debug_print( stream(__FILE__,__LINE__), status, status.size(), "bmp280 status : " );
    else
        stream(__FILE__,__LINE__) << "status failed" << std::endl;

    if ( bmp280.read( 0xf7, values.data(), values.size() ) )
        debug_print( stream(__FILE__,__LINE__), values, values.size(), "bmp280 values : " );
    else
        stream(__FILE__,__LINE__) << "values failed" << std::endl;

    while ( --argc ) {
        ++argv;
        if ( strcmp( argv[0], "reset" ) == 0 ) {
            if ( bmp280.write( std::array< uint8_t, 2 >( { 0xe0, 0xb6 } ) ) ) {
                stream(__FILE__,__LINE__) << "reset OK" << std::endl;
            } else {
                stream(__FILE__,__LINE__) << "reset FAILED" << std::endl;
            }
        } else if ( strcmp( argv[0], "meas" ) == 0 ) {
            constexpr uint8_t osrs_t = 1 << 5; // x4 (18bit/0.0012degC)
            constexpr uint8_t osrs_p = 1 << 2;
            constexpr uint8_t power_mode = 3;  // normal mode
            constexpr uint8_t osrs = osrs_t|osrs_p|power_mode;
            if ( bmp280.write( std::array< uint8_t, 2 >( { 0xf3, osrs } ) ) ) { // oversampling temp[7:5], press[4:2], power mode[1:0]
                stream(__FILE__,__LINE__) << "meas OK" << std::endl;
            } else {
                stream(__FILE__,__LINE__) << "meas FAILED" << std::endl;
            }
            
        }
    }
}
