// Copyright (C) 2018 MS-Cheminformatics LLC
// Licence: CC BY-NC
// Author: Toshinobu Hondo, Ph.D.
// Contact: toshi.hondo@qtplatz.com
//

#include "can.hpp"
#include "dma.hpp"
#include "stm32f103.hpp"
#include "utility.hpp"
#include <algorithm>

void
can_command( size_t argc, const char ** argv )
{
    if ( auto cbus = stm32f103::can_t< stm32f103::CAN1_BASE >::instance() ) {
        cbus->print_registers();

        CanMsg msg;
        msg.IDE = CAN_ID_STD;
        msg.RTR = CAN_RTR_DATA;
        msg.ID  = 0x55;
        msg.DLC = 8;
        msg.Data[ 0 ] = 0xaa;
        msg.Data[ 1 ] = 0x55;
        msg.Data[ 2 ] = 0;
        msg.Data[ 3 ] = 0;
        msg.Data[ 4 ] = 0;
        msg.Data[ 5 ] = 0;
        msg.Data[ 6 ] = 0;
        msg.Data[ 7 ] = 0;

        for ( int i = 0; i < 10000; ++i )
            cbus->transmit( &msg );
    }
}

