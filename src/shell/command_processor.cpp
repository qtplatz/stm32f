// Copyright (C) 2018 MS-Cheminformatics LLC
// Licence: CC BY-NC
// Author: Toshinobu Hondo, Ph.D.
// Contact: toshi.hondo@qtplatz.com
//

#include "command_processor.hpp"
#include "gpio_mode.hpp"
#include "printf.h"
#include "spi.hpp"
#include "stream.hpp"
#include "stm32f103.hpp"
#include <atomic>
#include <algorithm>

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

void
spi_test( size_t argc, const char ** argv )
{
    while ( true ) {
        uint32_t d = atomic_jiffies.load();
        __spi0 << ( d & 0xffff );
        mdelay( 200 );
    }
}

void
alt_test( size_t argc, const char ** argv )
{
    // ALT
    using namespace stm32f103;
    gpio_mode()( PA15, GPIO_CNF_ALT_OUTPUT_PUSH_PULL, GPIO_MODE_OUTPUT_50M ); // NSS
    gpio_mode()( PB3,  GPIO_CNF_ALT_OUTPUT_PUSH_PULL, GPIO_MODE_OUTPUT_50M ); // SCLK
    gpio_mode()( PB4,  GPIO_CNF_INPUT_FLOATING,       GPIO_MODE_INPUT );      // MISO
    gpio_mode()( PB5,  GPIO_CNF_ALT_OUTPUT_PUSH_PULL, GPIO_MODE_OUTPUT_50M ); // MOSI
    
    if ( auto AFIO = reinterpret_cast< volatile stm32f103::AFIO * >( stm32f103::AFIO_BASE ) ) {
        AFIO->MAPR |= 1;            // clear SPI1 remap
        stream() << "\tAFIO MAPR: 0x" << AFIO->MAPR << std::endl;
    }
}

command_processor::command_processor()
{
}

bool
command_processor::operator()( size_t argc, const char ** argv ) const
{
    stream() << "command_procedssor: argc=" << argc << " argv = {";
    for ( size_t i = 0; i < argc; ++i )
        stream() << argv[i] << ( ( i < argc - 1 ) ? ", " : "" );
    stream() << "}" << std::endl;

    if ( argc > 0 ) {
        if ( strcmp( argv[0], "spi" ) == 0 )
            spi_test( argc, argv );
        if ( strcmp( argv[0], "alt" ) == 0 )
            alt_test( argc, argv );        
    }

    stream() << "\tpossbile commands are: spi" << std::endl;
}
