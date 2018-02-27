// Copyright (C) 2018 MS-Cheminformatics LLC
// Licence: CC BY-NC
// Author: Toshinobu Hondo, Ph.D.
// Contact: toshi.hondo@qtplatz.com
//

#include "command_processor.hpp"
#include "gpio.hpp"
#include "gpio_mode.hpp"
#include "printf.h"
#include "spi.hpp"
#include "stream.hpp"
#include "stm32f103.hpp"
#include <atomic>
#include <algorithm>
#include <functional>

extern stm32f103::spi __spi0;
extern std::atomic< uint32_t > atomic_jiffies;
extern void mdelay( uint32_t ms );

int
strcmp( const char * a, const char * b )
{
    while ( *a && (*a == *b ) )
        a++, b++;
    return *reinterpret_cast< const unsigned char *>(a) - *reinterpret_cast< const unsigned char *>(b);
}

int
strtod( const char * s )
{
    int sign = 1;
    int num = 0;
    while ( *s ) {
        if ( *s == '-' )
            sign = -1;
        else if ( '0' <= *s && *s <= '9' )
            num += (num * 10) + ((*s) - '0');
        ++s;
    }
    return num * sign;
}

void
spi_test( size_t argc, const char ** argv )
{
    // spi num
    size_t count = 1024;

    if ( argc >= 2 ) {
        count = strtod( argv[ 1 ] );
        if ( count == 0 )
            count = 1;
    }

    while ( count-- ) {
        uint32_t d = atomic_jiffies.load();
        __spi0 << ( d & 0xffff );
        mdelay( 100 );
    }
}

void
alt_test( size_t argc, const char ** argv )
{
    // alt spi [remap]
    using namespace stm32f103;    
    auto AFIO = reinterpret_cast< volatile stm32f103::AFIO * >( stm32f103::AFIO_BASE );
    if ( argc > 1 && strcmp( argv[1], "spi" ) == 0 ) {
        if ( argc == 2 ) {
            gpio_mode()( stm32f103::PA4, GPIO_CNF_ALT_OUTPUT_PUSH_PULL, GPIO_MODE_OUTPUT_50M ); // ~SS
            gpio_mode()( stm32f103::PA5, GPIO_CNF_ALT_OUTPUT_PUSH_PULL, GPIO_MODE_OUTPUT_50M ); // SCLK
            gpio_mode()( stm32f103::PA6, GPIO_CNF_INPUT_FLOATING,       GPIO_MODE_INPUT );      // MISO
            gpio_mode()( stm32f103::PA7, GPIO_CNF_ALT_OUTPUT_PUSH_PULL, GPIO_MODE_OUTPUT_50M ); // MOSI
            AFIO->MAPR &= ~1;            // clear SPI1 remap
        }
        if ( argc > 2 && strcmp( argv[2], "remap" ) == 0 ) {
            gpio_mode()( stm32f103::PA15, GPIO_CNF_ALT_OUTPUT_PUSH_PULL, GPIO_MODE_OUTPUT_50M ); // NSS
            gpio_mode()( stm32f103::PB3,  GPIO_CNF_ALT_OUTPUT_PUSH_PULL, GPIO_MODE_OUTPUT_50M ); // SCLK
            gpio_mode()( stm32f103::PB4,  GPIO_CNF_INPUT_FLOATING,       GPIO_MODE_INPUT );      // MISO
            gpio_mode()( stm32f103::PB5,  GPIO_CNF_ALT_OUTPUT_PUSH_PULL, GPIO_MODE_OUTPUT_50M ); // MOSI
            AFIO->MAPR |= 1;          // set SPI1 remap
        }
        stream() << "\tEnable SPI, AFIO MAPR: 0x" << AFIO->MAPR << std::endl;
    } else {
        stream()
            << "\tError: insufficent arguments\nalt spi [remap]"
            << "\tAFIO MAPR: 0x" << AFIO->MAPR
            << std::endl;
    }
}

void
gpio_test( size_t argc, const char ** argv )
{
    using namespace stm32f103;
    
    if ( argc >= 2 ) {
        if ( strcmp( argv[1], "spi" ) == 0 ) {
            gpio_mode()( PA4, GPIO_CNF_OUTPUT_PUSH_PULL, GPIO_MODE_OUTPUT_2M );
            gpio_mode()( PA5, GPIO_CNF_OUTPUT_PUSH_PULL, GPIO_MODE_OUTPUT_2M );
            gpio_mode()( PA6, GPIO_CNF_OUTPUT_PUSH_PULL, GPIO_MODE_OUTPUT_2M );
            gpio_mode()( PA7, GPIO_CNF_OUTPUT_PUSH_PULL, GPIO_MODE_OUTPUT_2M );
            for ( int i = 0; i < 0xfffff; ++i ) {
                stm32f103::gpio< decltype( stm32f103::PA4 ) >( stm32f103::PA4 ) = bool( i & 01 );
                stm32f103::gpio< decltype( stm32f103::PA5 ) >( stm32f103::PA5 ) = bool( i & 01 );
                stm32f103::gpio< decltype( stm32f103::PA6 ) >( stm32f103::PA6 ) = bool( i & 01 );
                stm32f103::gpio< decltype( stm32f103::PA7 ) >( stm32f103::PA7 ) = bool( i & 01 );
                auto tp = atomic_jiffies.load();
                while ( tp == atomic_jiffies.load() ) // 100us
                    ;
            }
        }
    } else {
        stream() << "gpio spi" << std::endl;
    }
}

command_processor::command_processor()
{
}

class premitive {
public:
    const char * arg0_;
    void (*f_)(size_t, const char **);
    const char * help_;
};

static const premitive command_table [] = {
    { "spi", spi_test,     " [number]" }
    , { "alt", alt_test,   " spi [remap]" }
    , { "gpio", gpio_test, " spi -- (toggles A4-A7 as GPIO)" }
};

bool
command_processor::operator()( size_t argc, const char ** argv ) const
{
    stream() << "command_procedssor: argc=" << argc << " argv = {";
    for ( size_t i = 0; i < argc; ++i )
         stream() << argv[i] << ( ( i < argc - 1 ) ? ", " : "" );
    stream() << "}" << std::endl;
    
    if ( argc > 0 ) {
        for ( auto& cmd: command_table ) {
            if ( strcmp( cmd.arg0_, argv[0] ) == 0 ) {
                cmd.f_( argc, argv );
                break;
            }
        }
    } else {
        stream() << "command processor -- help" << std::endl;
        for ( auto& cmd: command_table )
            stream() << "\t" << cmd.arg0_ << cmd.help_ << std::endl;
    }
}
