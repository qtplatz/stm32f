// Copyright (C) 2018 MS-Cheminformatics LLC
// Licence: CC BY-NC
// Author: Toshinobu Hondo, Ph.D.
// Contact: toshi.hondo@qtplatz.com
//

#include "can.hpp"
#include "condition_wait.hpp"
#include "dma.hpp"
#include "stm32f103.hpp"
#include "stream.hpp"
#include "utility.hpp"
#include <algorithm>
#include <bitset>

constexpr const char * __can_status_strings [] = {
    "CAN_OK"
    , "CAN_INIT_FAILED"
    , "CAN_INIT_E_FAILED"
    , "CAN_INIT_L_FAILED"
    , "CAN_TX_FAILED"
    , "CAN_TX_PENDING"
    , "CAN_NO_MB"
    , "CAN_FILTER_FULL"
};

extern void mdelay( uint32_t );

void
can_command( size_t argc, const char ** argv )
{
    if ( argc == 1 ) {
        if ( auto cbus = stm32f103::can_t< stm32f103::CAN1_BASE >::instance() ) {
            cbus->print_registers();
        }
    }

    while ( --argc ) {
        ++argv;
        if ( strcmp( argv[ 0 ], "--transmit" ) == 0 ) {
            auto can = stm32f103::can_t< stm32f103::CAN1_BASE >::instance();
            can->filter( 0, CAN_FIFO_0, CAN_FILTER_32BIT, CAN_FILTER_MASK, 0, 0 );

            size_t count = 1;
            if ( argc >= 1 && std::isdigit( *argv[1] ) ) {
                --argc; ++argv;
                count = strtod( argv[ 0 ] );
            }
            
            for ( int i = 0; i < count; ++i ) {

                CanMsg msg;
                msg.IDE = CAN_ID_STD;
                msg.RTR = CAN_RTR_DATA;
                msg.ID  = 0x55;
                msg.DLC = 8;
                msg.Data[ 0 ] = 0xaa;
                msg.Data[ 1 ] = 0x55;
                msg.Data[ 2 ] = 'A';
                msg.Data[ 3 ] = 'T';
                msg.Data[ 4 ] = (count - i) >> 24;
                msg.Data[ 5 ] = (count - i) >> 16;
                msg.Data[ 6 ] = (count - i) >> 8;
                msg.Data[ 7 ] = (count - i);
                
                auto mbx = can->transmit( &msg );
                auto state = stm32f103::can_t< stm32f103::CAN1_BASE >::instance()->tx_status( mbx );

                size_t count = 100;
                if ( count-- && !can->rx_available() )
                    mdelay( 10 );

                while ( auto rx = can->rx_queue_get() ) {
                    stream(__FILE__,__LINE__) << "\tRx ID: " << rx->ID << ", RTR: " << rx->RTR
                                              << ", DLC: " << rx->DLC << ", FMI: " << rx->FMI << std::endl;
                    stream() << "\t\t\t data: ";
                    for ( int i = 0; i < sizeof( rx->Data ); ++i )
                        stream() << rx->Data[ i ] << ", ";
                    can->rx_queue_free();
                }
            }
        } else if ( strcmp( argv[ 0 ], "--noloopback" ) == 0 ) {
            stm32f103::can_t< stm32f103::CAN1_BASE >::instance()->set_loopback_mode( false );
        } else if ( strcmp( argv[ 0 ], "--loopback" ) == 0 ) {
            stm32f103::can_t< stm32f103::CAN1_BASE >::instance()->set_loopback_mode( true );
        } else if ( strcmp( argv[ 0 ], "--nosilent" ) == 0 ) {
            stm32f103::can_t< stm32f103::CAN1_BASE >::instance()->set_silent_mode( false );
        } else if ( strcmp( argv[ 0 ], "--silent" ) == 0 ) {
            stm32f103::can_t< stm32f103::CAN1_BASE >::instance()->set_silent_mode( true );
        }
    }
}

