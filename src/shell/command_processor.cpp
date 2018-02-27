// Copyright (C) 2018 MS-Cheminformatics LLC
// Licence: CC BY-NC
// Author: Toshinobu Hondo, Ph.D.
// Contact: toshi.hondo@qtplatz.com
//

#include "command_processor.hpp"
#include "printf.h"
#include "spi.hpp"
#include "stream.hpp"
#include <atomic>
#include <algorithm>

extern stm32f103::spi __spi0;
extern std::atomic< uint32_t > atomic_jiffies;
extern void delay_ms( int ms );

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
        __spi0 << atomic_jiffies.load();
        delay_ms( 500 );
        printf("spi_test\n");
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
    }

    stream() << "\tpossbile commands are: spi" << std::endl;
}
