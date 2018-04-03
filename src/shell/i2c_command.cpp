// Copyright (C) 2018 MS-Cheminformatics LLC
// Licence: CC BY-NC
// Author: Toshinobu Hondo, Ph.D.
// Contact: toshi.hondo@qtplatz.com
//

#include "i2c.hpp"
#include "dma.hpp"
#include "i2c_string.hpp"
#include "gpio_mode.hpp"
#include "stream.hpp"
#include "stm32f103.hpp"
#include "utility.hpp"
#include <algorithm>

void i2c_command( size_t argc, const char ** argv );

static void
i2c_probe( int id )
{
    stm32f103::I2C_BASE addr = ( id == 0 ) ? stm32f103::I2C1_BASE : stm32f103::I2C2_BASE;    
    auto& i2cx = ( addr == stm32f103::I2C1_BASE )
        ? *stm32f103::i2c_t< stm32f103::I2C1_BASE >::instance() : *stm32f103::i2c_t< stm32f103::I2C2_BASE >::instance();

    if ( !i2cx ) {
        const char * argv [] = { id == 0 ? "i2c" : "i2c2", nullptr };
        i2c_command( 1, argv );
    }

    if ( i2cx ) {

        stream() << "\t 0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f" << std::endl;
        stream() << "00:\t         ";

        for ( uint8_t addr = 3; addr <= 0x77; ++addr ) {

            if ( ( addr % 16 ) == 0 )
                stream() << std::endl << addr << ":\t";
            
            uint8_t data;
            if ( i2cx.read( addr, &data, 1 ) ) {
                stream() << addr << " ";
            } else {
                stream() << "-- ";
            }
        }
    } else {
        stream() << "i2c not initalized" << std::endl;
    }
}

void
i2cdetect( size_t argc, const char ** argv )
{
    int id = 0;
    if ( argc > 2 ) {
        if ( *argv[1] == '1' )
            id = 1;
    }
    i2c_probe( id );
}

void
i2c_command( size_t argc, const char ** argv )
{
    int id = ( strcmp( argv[0], "i2c") == 0 ) ? 0 : 1;
    stm32f103::I2C_BASE addr = ( id == 0 ) ? stm32f103::I2C1_BASE : stm32f103::I2C2_BASE;

    auto& i2cx = ( addr == stm32f103::I2C1_BASE )
        ? *stm32f103::i2c_t< stm32f103::I2C1_BASE >::instance() : *stm32f103::i2c_t< stm32f103::I2C2_BASE >::instance();    

    using namespace stm32f103;

    if ( !i2cx.has_dma( i2c::DMA_Both ) )
        i2cx.attach( *dma_t< DMA1_BASE >::instance(), i2c::DMA_Both );

    static uint32_t replicates;
    static uint8_t chipaddr;
    static uint32_t txd;
    
    std::array< uint8_t, 32 > rxdata = { 0 };
    std::array< uint8_t, 8 > txdata = { 0x71, 0 };

    if ( chipaddr == 0 )
        chipaddr = 0x10; // DA5593R

    if ( replicates == 0 )
        replicates = 1;
    
    bool use_dma( true );

    constexpr static std::pair< const char *, size_t > rcmds [] = {
        { "r", 1 }, {"rb", 1 }, {"r2", 2 }, {"r3", 3 }, {"r4", 4}, {"--read", 0 }
    };
    constexpr static auto rcmds_end = &rcmds[sizeof( rcmds ) / sizeof( rcmds[0] )];

    constexpr static std::pair< const char *, size_t > wcmds [] = {
        { "w", 1 }, {"wb", 1 }, {"w2", 2 }, {"w3", 3 }, {"w4", 4 }, {"w5", 5}
    };
    
    constexpr static auto wcmds_end = &wcmds[sizeof( wcmds ) / sizeof( wcmds[0] )];

    auto rx_print = [&]( stream&& o, size_t read_counts, uint32_t status ){
        o << "i2c dma got data: (" << int(read_counts) << ")[";
        std::for_each( rxdata.begin(), rxdata.begin() + read_counts, [&](auto x){ o << x << ", "; } );
        o << "] from " << chipaddr << (status == 0 ? " OK" : "" ) << std::endl;
        if ( status )
            i2c_string::status32( std::move( o ), status, nullptr );
    };

    auto tx_print = [&]( stream&& o, size_t write_counts ){
        o << "i2c sending data: (" << int(write_counts) << ")[";
        std::for_each( txdata.begin(), txdata.begin() + write_counts, [&](auto x){ o << x << ", "; } );
        o << "] to " << chipaddr << std::endl;
    };

    if ( argc == 1 ) {
        stream() << "i2c command arguments:\n"
            "i2c --addr <chip-addr>  // i2c chip address\n"
            "i2c <hex value> [w|w2|w3|w4]   // write <hex value> as byte|2,3 or 4 bytes words\n"
            "i2c r[2|3|4]   // read n-word data from i2c device\n"
            "i2c --read <numbuer>   // read number-byte adrray data i2c device\n"
            "i2c probe\n"
            "i2c reset\n"
            "i2c status\n"
                 << std::endl;
        i2c_string::print_registers( stream(), i2cx.base_addr() );
    }

    while ( --argc ) {
        ++argv;
        if ( strcmp( argv[0], "status" ) == 0 ) {
            i2cx.print_status( stream() );
        } else if ( strcmp( argv[0], "reset" ) == 0 ) {
            i2cx.reset();
        } else if ( strcmp( argv[0], "probe" ) == 0 ) {
            i2c_probe( id );
        } else if ( strcmp( argv[0], "--slave" ) == 0 ) {
            stm32f103::i2c_t< stm32f103::I2C2_BASE >::instance()->listen( 0x20 ); // make it 'slave'
        } else if ( std::isdigit( *argv[0] ) ) {
            txd = strtox( argv[0] );
        } else if ( strcmp( argv[0], "dma" ) == 0 ) {
            use_dma = true;
        } else if ( strcmp( argv[0], "-dma" ) == 0 ) {
            use_dma = false;
        } else if ( std::find_if( rcmds, rcmds_end, [&](auto& a){ return strcmp( argv[0], a.first ) == 0; } ) != rcmds_end ) {
            auto it = std::find_if( rcmds, rcmds_end, [&](auto& a){ return strcmp( argv[0], a.first ) == 0; } );
            size_t read_counts = it->second;
            if ( read_counts == 0 && argc >= 1 && std::isdigit( *argv[1] ) ) {
                --argc; ++argv;
                read_counts = strtod( argv[0] );
            }
            read_counts = read_counts == 0 ? 1 : (read_counts < rxdata.size() ? read_counts : rxdata.size() );
            
            rxdata = { 0 };
            if ( use_dma ) {
                if ( stm32f103::i2c_t< stm32f103::I2C1_BASE >::instance()->dma_receive(chipaddr, rxdata.data(), read_counts ) ) {
                    rx_print( stream(__FILE__,__LINE__), read_counts, stm32f103::i2c_t< stm32f103::I2C1_BASE >::instance()->status() );
                } else {
                    stream(__FILE__,__LINE__) << "i2c -- dma read failed. addr=" << chipaddr << std::endl;
                }
            } else {
                if ( i2cx.read( chipaddr, rxdata.data(), read_counts ) ) {
                    rx_print( stream(__FILE__,__LINE__), read_counts, stm32f103::i2c_t< stm32f103::I2C1_BASE >::instance()->status() );
                } else {
                    stream(__FILE__,__LINE__) << "i2c -- polling read failed. addr=" << chipaddr << std::endl;
                }
            }
        } else if ( std::find_if( wcmds, wcmds_end, [&](auto& a){ return strcmp( argv[0], a.first ) == 0; } ) != wcmds_end ) {

            auto it = std::find_if( wcmds, wcmds_end, [&](auto& a){ return strcmp( argv[0], a.first ) == 0; } );
            size_t write_counts = it->second;
            if ( write_counts == 0 && argc >= 1 && std::isdigit( *argv[1] ) ) {
                --argc; ++argv;
                write_counts = strtod( argv[0] );
            }
            
            for ( size_t i = 0; i < write_counts; ++i )
                txdata[ write_counts - i - 1 ] = uint8_t( ( txd >> (8*i) ) & 0xff );
            
            tx_print( stream(__FILE__,__LINE__), write_counts );

            if ( use_dma ) {
                for ( size_t i = 0; i < replicates; ++i ) {
                    if ( stm32f103::i2c_t< stm32f103::I2C1_BASE >::instance()->dma_transfer(chipaddr, txdata.data(), write_counts ) ) {
                        if ( auto st = stm32f103::i2c_t< stm32f103::I2C1_BASE >::instance()->status() ) {
                            i2c_string::status32( stream(), st, i2cx.base_addr() );
                            break;
                        } else {
                            stream() << "\tOK(dma)\n";
                        }
                    } else {
                        stm32f103::i2c_t< stm32f103::I2C1_BASE >::instance()->print_result( stream(__FILE__,__LINE__) )
                            << "\tdma transfer to " << chipaddr << " failed.\n";
                    }
                }
            } else {
                if ( i2cx.write( chipaddr, txdata.data(), write_counts ) ) {
                    if ( auto st = i2cx.status() )
                        i2c_string::status32( stream(), st, i2cx.base_addr() );
                    else
                        stream() << "\tOK(polling)\n";
                } else {
                    i2cx.print_result( stream(__FILE__,__LINE__) ) << "\tdma transfer to " << chipaddr << " failed.\n";
                }
            }
        } else if ( strcmp( argv[0], "--repeat" ) == 0 ) {
            if ( argc && std::isdigit( *argv[1] ) ) {
                replicates = strtox( argv[ 1 ] );
                --argc; ++argv;
            }
            stream() << "i2c replicates: " << replicates << std::endl;            
        } else if ( strcmp( argv[0], "--addr" ) == 0 ) {
            if ( argc && std::isdigit( *argv[1] ) ) {
                chipaddr = strtox( argv[ 1 ] );
                --argc; ++argv;
            }
            stream() << "i2c chip addr: " << chipaddr << std::endl;
        }
    }
}

