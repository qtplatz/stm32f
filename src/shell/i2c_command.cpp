// Copyright (C) 2018 MS-Cheminformatics LLC
// Licence: CC BY-NC
// Author: Toshinobu Hondo, Ph.D.
// Contact: toshi.hondo@qtplatz.com
//

#include "i2c.hpp"
#include "gpio_mode.hpp"
#include "stream.hpp"
#include "stm32f103.hpp"
#include "utility.hpp"
#include <algorithm>

extern stm32f103::i2c __i2c0, __i2c1;
extern stm32f103::dma __dma0;

void i2c_command( size_t argc, const char ** argv );

void
i2cdetect( size_t argc, const char ** argv )
{
    int id = 0;
    if ( argc > 2 ) {
        if ( *argv[1] == '1' )
            id = 1;
    }
    
    stm32f103::I2C_BASE addr = ( id == 0 ) ? stm32f103::I2C1_BASE : stm32f103::I2C2_BASE;    
    auto& i2cx = ( addr == stm32f103::I2C1_BASE ) ? __i2c0 : __i2c1;

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
i2c_command( size_t argc, const char ** argv )
{
    int id = ( strcmp( argv[0], "i2c") == 0 ) ? 0 : 1;
    stm32f103::I2C_BASE addr = ( id == 0 ) ? stm32f103::I2C1_BASE : stm32f103::I2C2_BASE;

    auto& i2cx = ( addr == stm32f103::I2C1_BASE ) ? __i2c0 : __i2c1;

    auto it = std::find_if( argv, argv + argc, [](auto a){ return strcmp( a, "dma" ) == 0; } );
    const bool init_dma = it != ( argv + argc );

    using namespace stm32f103;

    // (see RM0008, p180, Table 55)
    // I2C ALT function  REMAP=0 { SCL,SDA } = { PB6, PB7 }, REMAP=1 { PB8, PB9 }
    // GPIO config in p167, Table 27
    if ( id == 0 ) {
        if ( ! __i2c0 ) {
            gpio_mode()( stm32f103::PB6, stm32f103::GPIO_CNF_ALT_OUTPUT_ODRAIN,  stm32f103::GPIO_MODE_OUTPUT_2M ); // SCL
            gpio_mode()( stm32f103::PB7, stm32f103::GPIO_CNF_ALT_OUTPUT_ODRAIN,  stm32f103::GPIO_MODE_OUTPUT_2M ); // SDA
            __i2c0.init( stm32f103::I2C1_BASE );
        }
    }
    if ( id == 1 ) {
        if ( ! __i2c1 ) {
            gpio_mode()( stm32f103::PB10, stm32f103::GPIO_CNF_ALT_OUTPUT_ODRAIN, stm32f103::GPIO_MODE_OUTPUT_2M ); // SCL
            gpio_mode()( stm32f103::PB11, stm32f103::GPIO_CNF_ALT_OUTPUT_ODRAIN, stm32f103::GPIO_MODE_OUTPUT_2M ); // SDA
            __i2c1.init( stm32f103::I2C2_BASE );
        }
    }

    if ( init_dma && !i2cx.has_dma( i2c::DMA_Both ) )
        i2cx.attach( __dma0, i2c::DMA_Both );

    static uint8_t i2caddr;
    std::array< uint8_t, 32 > rxdata = { 0 };
    std::array< uint8_t, 4 > txdata = { 0x71, 0 };
    uint32_t txd = 0;

    if ( i2caddr == 0 )
        i2caddr = 0x10; // DA5593R
    
    bool use_dma( false );

    constexpr static std::pair< const char *, size_t > rcmds [] = { { "r", 2 }, { "read", 2 }, {"rb", 1 }, {"r2", 2 }, {"r3", 3 }, {"r4", 4}, {"--read", 0 } };
    constexpr static auto rcmds_end = &rcmds[sizeof( rcmds ) / sizeof( rcmds[0] )];

    constexpr static std::pair< const char *, size_t > wcmds [] = { { "w", 2 }, { "write", 2 }, {"wb", 1 }, {"w2", 2 }, {"w3", 3 }, {"w4", 4 } };
    constexpr static auto wcmds_end = &wcmds[sizeof( wcmds ) / sizeof( wcmds[0] )];

    auto rx_print = [&]( stream&& o, size_t read_counts ){
        o << "i2c dma got data: (" << read_counts << ")[";
        std::for_each( rxdata.begin(), rxdata.begin() + read_counts, [&](auto x){ o << x << ", "; } );
        o << "] from " << i2caddr << std::endl;
    };

    auto tx_print = [&]( stream&& o, size_t write_counts ){
        o << "i2c sending data: (" << write_counts << ")[";
        std::for_each( txdata.begin(), txdata.begin() + write_counts, [&](auto x){ o << x << ", "; } );
        o << "] to " << i2caddr << std::endl;
    };  
    
    while ( --argc ) {
        ++argv;
        if ( strcmp( argv[0], "status" ) == 0 ) {
            i2cx.print_status();
        } else if ( strcmp( argv[0], "reset" ) == 0 ) {
            i2cx.reset();
            i2cx.print_status();
        } else if ( std::isdigit( *argv[0] ) ) {
            txd = strtox( argv[0] );
        } else if ( strcmp( argv[0], "dma" ) == 0 ) {
            use_dma = true;
        } else if ( strcmp( argv[0], "!dma" ) == 0 ) {
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
                if ( __i2c0.dma_receive(i2caddr, rxdata.data(), read_counts ) ) {
                    rx_print( stream(__FILE__,__LINE__), read_counts );
                } else {
                    stream(__FILE__,__LINE__) << "i2c -- dma read failed. addr=" << i2caddr << std::endl;
                }
            } else {
                if ( i2cx.read( i2caddr, rxdata.data(), read_counts ) ) {
                    rx_print( stream(__FILE__,__LINE__), read_counts );
                } else {
                    stream(__FILE__,__LINE__) << "i2c -- polling read failed. addr=" << i2caddr << std::endl;
                }
            }
        } else if ( std::find_if( wcmds, wcmds_end, [&](auto& a){ return strcmp( argv[0], a.first ) == 0; } ) != wcmds_end ) {

            auto it = std::find_if( wcmds, wcmds_end, [&](auto& a){ return strcmp( argv[0], a.first ) == 0; } );
            size_t write_counts = it->second;
            stream() << "write command : " << it->first << " write_counts=" << write_counts << std::endl;
            
            for ( size_t i = 0; i < write_counts; ++i )
                txdata[ write_counts - i - 1 ] = uint8_t( ( txd >> (8*i) ) & 0xff );
            
            tx_print( stream(__FILE__,__LINE__), write_counts );

            if ( use_dma ) {
                if ( __i2c0.dma_transfer(i2caddr, txdata.data(), write_counts ) ) {
                    stream() << "\nOK";
                } else {
                    stream(__FILE__,__LINE__,__FUNCTION__) << "\tdma transfer to " << i2caddr << " failed.\n";
                }
            } else {
                if ( i2cx.write( i2caddr, txdata.data(), write_counts ) ) {
                    stream() << "\nOK";
                } else {
                    stream(__FILE__,__LINE__,__FUNCTION__) << "\tpolling transfer to " << i2caddr << " failed.\n";
                }
            }
        } else if ( strcmp( argv[0], "--addr" ) == 0 ) {
            if ( argc && std::isdigit( *argv[1] ) ) {
                i2caddr = strtox( argv[ 1 ] );
                --argc; ++argv;
            }
        }
    }
}

