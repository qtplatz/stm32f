// Copyright (C) 2018 MS-Cheminformatics LLC
// Licence: CC BY-NC
// Author: Toshinobu Hondo, Ph.D.
// Contact: toshi.hondo@qtplatz.com
//

#include "stm32f103.hpp"
#include "debug_print.hpp"
#include "stream.hpp"
#include "timer.hpp"
#include "utility.hpp"

void
timer_command( size_t argc, const char ** argv )
{
    using namespace stm32f103;

    stm32f103::timer_t< stm32f103::TIM2_BASE > timx;

    while ( --argc ) {
        ++argv;
        if ( strcmp( argv[0], "help" ) == 0 ) {
            stream() << "timer help|status" << std::endl;
        } else if ( strcmp( argv[0], "status" ) == 0 ) {
            timx.print_registers();
        }
    }
}
