// Copyright (C) 2018 MS-Cheminformatics LLC
// Licence: CC BY-NC
// Author: Toshinobu Hondo, Ph.D.
// Contact: toshi.hondo@qtplatz.com
//

#include "bitset.hpp"
#include "rtc.hpp"
#include "stream.hpp"
#include "system_clock.hpp"
#include "stm32f103.hpp"
#include "timer.hpp"
#include "utility.hpp"
#include <array>
#include <date_time/date_time.hpp>

void lsi_calibration();
void mdelay( uint32_t );

void
date_command( size_t argc, const char ** argv )
{
    using stm32f103::system_clock;
    if ( argc == 1 )  {
        std::time_t time = system_clock::to_time_t( system_clock::now() );
        std::array< char, 30 > str;

        //stream(__FILE__,__LINE__,__FUNCTION__) <<
        stream() << date_time::to_string( str.data(), str.size(), time, false ) << std::endl;
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

                stream() << "parsed: " << dstr.data() << "\t" << time << std::endl;
                
                stm32f103::rtc::set_hwclock( time );
            }
        } else if ( strcmp( argv[ 0 ], "--calib" ) == 0 ) {
            // todo, p96
            lsi_calibration();
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

void
lsi_calibration()
{
    //1. Enable TIM5 timer and configure channel4 in input capture mode
    //2. Set the TIM5CH4_IREMAP bit in the AFIO_MAPR register to connect the LSI clock
    //   internally to TIM5 channel4 input capture for calibration purpose.
    //3. Measure the frequency of LSI clock using the TIM5 Capture/compare 4 event or
    //   interrupt.
    //4. Use the measured LSI frequency to update the 20-bit prescaler of the RTC depending
    //   on the desired time base and/or to compute the IWDG timeout.    

    stm32f103::timer_t< stm32f103::TIM3_BASE > timer;

    timer.set_callback( []{
            static size_t counter;
            ++counter;
            stream() << "timer  irq " << counter << std::endl;
        });

    if ( auto AFIO = reinterpret_cast< volatile stm32f103::AFIO * >( stm32f103::AFIO_BASE ) ) {
        // p177
        stm32f103::bitset::set(  AFIO->MAPR, 1 );  // TIM5CH4_IREMAP = 1 (LSI internal clock is connected to TIM5_CH4)
    }

    for ( int i = 0; i < 10; ++i ) {
        mdelay( 1000 );
        stream() << "waiting..." << std::endl;
    }

    timer.clear_callback();
}
