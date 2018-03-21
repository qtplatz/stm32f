// Copyright (C) 2018 MS-Cheminformatics LLC
// Licence: CC BY-NC
// Author: Toshinobu Hondo, Ph.D.
// Contact: toshi.hondo@qtplatz.com
//

#include "stream.hpp"
#include "system_clock.hpp"
#include "rtc.hpp"
#include "utility.hpp"
#include <date_time/date_time.hpp>

void
date_command( size_t argc, const char ** argv )
{
    using stm32f103::system_clock;
    if ( argc == 1 )  {
        std::time_t time = system_clock::to_time_t( system_clock::now() );
        std::array< char, 30 > str;
        stream(__FILE__,__LINE__,__FUNCTION__) << date_time::to_string( str.data(), str.size(), time, true )
                                               << std::endl;
    }
    
    while ( --argc ) {
        ++argv;
        if ( ( strcmp( argv[ 0 ], "--set" ) == 0 ) || ( strcmp( argv[ 0 ], "-s" ) == 0 ) )  {
            // 2018-03-21T16:34:00" | 2018-03-21 16:34:00"
            
            std::array< char, 40 > input;
            if ( argc ) {
                --argc; ++argv;
                strncpy( input.data(), argv[ 0 ], input.size() );
            }
            if ( argc ) {
                --argc; ++argv;
                strncat( input.data(), " ", input.size() );
                strncat( input.data(), argv[ 0 ], input.size() );
            }

            std::tm tm;
            auto state = date_time::parse( input.data(), tm );
            if ( state & date_time::date_time_both ) {
                std::array< char, 30 > dstr;
                date_time::to_string( dstr.data(), dstr.size(), tm, true );
                std::time_t time = date_time::time( tm );
                stream() << "parsed: " << dstr.data() << "\t" << time << "\t" << (time - stm32f103::rtc::__epoch_offset__) << std::endl;
            }
        }
    }
}

void
hwclock_command( size_t argc, const char ** argv )
{
    uint32_t div;
    auto count = stm32f103::rtc::clock( div );
    stream() << "rtc count: " << count << "\tdiv: " << div << std::endl;
}
