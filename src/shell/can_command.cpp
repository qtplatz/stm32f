// Copyright (C) 2018 MS-Cheminformatics LLC
// Licence: CC BY-NC
// Author: Toshinobu Hondo, Ph.D.
// Contact: toshi.hondo@qtplatz.com
//
// stm32f> can [cr]  // register disp
// stm32f> cansend 0ab#123456789a
// stm32f> candump [cr]

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
cansend( const char * data )
{
    CanMsg msg;

    msg.IDE = CAN_ID_STD;
    msg.RTR = CAN_RTR_DATA;
    msg.DLC = 8;

    if ( !data )
        data = "02#55aa112233445566";

    char * endp;
    msg.ID = strtox( data, &endp );
    stream(__FILE__,__LINE__) << "strtox endp: " << endp << std::endl;
    if ( endp && *endp == '#' ) {
        ++endp;
        size_t id(0);
        {
            uint32_t d = strtox( endp, &endp );
            msg.Data[ id++ ] = d >> 24;
            msg.Data[ id++ ] = d >> 16;
            msg.Data[ id++ ] = d >> 8;
            msg.Data[ id++ ] = d;
        }
        {
            uint32_t d = strtox( endp, &endp );
            stream() << "data l: " << d << std::endl;
            msg.Data[ id++ ] = d >> 24;
            msg.Data[ id++ ] = d >> 16;
            msg.Data[ id++ ] = d >> 8;
            msg.Data[ id++ ] = d;
        }
    }

    if ( msg.ID ) {
        auto can = stm32f103::can_t< stm32f103::CAN1_BASE >::instance();
        can->filter( 0, CAN_FIFO_0, CAN_FILTER_32BIT, CAN_FILTER_MASK, 0, 0 );
        auto mbx = can->transmit( &msg );
        mdelay(10);
        auto state = can->tx_status( mbx );
        stream(__FILE__,__LINE__) << __can_status_strings[ state ] << std::endl;
    }
}

void
candump()
{
    auto can = stm32f103::can_t< stm32f103::CAN1_BASE >::instance();

    size_t count = 1000;
    while ( count-- && !can->rx_available() )
        mdelay( 10 );

    while ( auto rx = can->rx_queue_get() ) {
        stream() << "\tID: " << rx->ID << ", RTR: " << rx->RTR
                 << ", DLC: " << rx->DLC << ", FMI: " << rx->FMI << "\tdata: \t";
        for ( int i = 0; i < sizeof( rx->Data ); ++i )
            stream() << rx->Data[ i ] << ", ";
        can->rx_queue_free();
    }
}

void
can_command( size_t argc, const char ** argv )
{
    if ( strcmp( argv[ 0 ], "can" ) == 0 ) {

        auto cbus = stm32f103::can_t< stm32f103::CAN1_BASE >::instance();

        if ( argc == 1 )
            cbus->print_registers();

        while ( --argc ) {
            ++argv;
            if ( strcmp( argv[ 0 ], "loopback" ) == 0 ) {
                if ( argc ) {
                    if ( strcmp( argv[ 1 ], "off" ) == 0 )
                        cbus->set_loopback_mode( false );
                    else if ( strcmp( argv[ 1 ], "on" ) == 0 )
                        cbus->set_loopback_mode( true );
                    ++argv; --argc;
                }
                stream() << "can loopback " << ( cbus->loopback_mode() ? "on" : "off" ) << std::endl;
            } else if ( strcmp( argv[ 0 ], "silent" ) == 0 ) {
                if ( argc ) {
                    if ( strcmp( argv[ 1 ], "off" ) == 0 )
                        cbus->set_silent_mode( false );
                    else if ( strcmp( argv[ 1 ], "on" ) == 0 )
                        cbus->set_silent_mode( true );
                    ++argv; --argc;
                }
                stream() << "can silent " << ( cbus->silent_mode() ? "on" : "off" ) << std::endl;
            } else {
                stream() << "unknown option: " << argv[ 0 ] << std::endl;
                stream() << "usage:\n\tcan loopback {on|off}" << std::endl;
                stream() << "\tcan silent {on|off}" << std::endl;
                return;
            }
        }
    } else if ( strcmp( argv[ 0 ], "cansend" ) == 0 ) {
        cansend( argc > 1 ? argv[1] : 0 );
    } else if ( strcmp( argv[ 0 ], "candump" ) == 0 ) {
        candump();
    }
}

