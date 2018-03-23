// Copyright (C) 2018 MS-Cheminformatics LLC
// Licence: CC BY-NC
// Author: Toshinobu Hondo, Ph.D.
// Contact: toshi.hondo@qtplatz.com
//

#include "ad5593.hpp"
#include "i2c.hpp"
#include "stm32f103.hpp"
#include "stream.hpp"
#include "utility.hpp"

namespace ad5593 {
    ad5593::AD5593 * __ad5593;
    static uint8_t __ad5593_allocator__[ sizeof( ad5593::AD5593 ) ];
}

extern void i2c_command( size_t argc, const char ** argv );

void
ad5593_command( size_t argc, const char ** argv )
{
    using namespace ad5593;
    
    if ( __ad5593 == nullptr ) {
        if ( __ad5593 = new ( __ad5593_allocator__ ) ad5593::AD5593( *stm32f103::i2c_t< stm32f103::I2C1_BASE >::instance(), 0x10 ) )
            if ( ! __ad5593->fetch() )
                stream() << "fetch error\n";
    }
    
    uint8_t i2caddr = 0x10;
    uint8_t reg = 0x75; // default (DAC)

    double volts = 1.0;
    double Vref = 3.3;

    while ( --argc ) {
        ++argv;
        if ( strcmp( argv[0], "fetch" ) == 0 ) {  // ad5593 dac 0 1 2...
            if ( __ad5593->fetch() )
                __ad5593->print_config();
            else
                stream() << "fetch error\n";
        } else if ( strcmp( argv[0], "reg" ) == 0 ) {  // ad5593 dac 0 1 2...
            if ( argc && std::isdigit( *argv[1] ) ) {
                reg = strtox( argv[1] );
                --argc; ++argv;
            }
        } else if ( strcmp( argv[0], "read" ) == 0 ) {  // ad5593 dac 0 1 2...
            auto data = __ad5593->read( reg );
            const std::bitset< sizeof(data) * 8 > bits( data );
            stream() << "\nread[0x" << reg << "]=" << data << "\t" << std::endl;
        } else if ( strcmp( argv[0], "write" ) == 0 ) {  // ad5593 dac 0 1 2...
            if ( argc && std::isdigit( *argv[1] ) ) {
                auto data = strtox( argv[1] );
                stream() << "\nwrite[0x" << reg << "]=" << data << std::endl;
                __ad5593->write( reg, data );
                --argc; ++argv;
            }

        } else if ( strcmp( argv[0], "dac" ) == 0 ) {  // ad5593 dac 0 1 2...
            while ( argc && std::isdigit( *argv[1] ) ) {
                int pin = *argv[1] - '0';
                if ( pin < 8 )
                    __ad5593->set_function( pin, ad5593::DAC );
                --argc; ++argv;
            }
        } else if ( strcmp( argv[0], "adc" ) == 0 ) {  // ad5593 adc 0 1 2...
            while ( argc && std::isdigit( *argv[1] ) ) {
                int pin = *argv[1] - '0';
                if ( pin < 8 )
                    __ad5593->set_function( pin, ad5593::ADC );
                --argc; ++argv;
            }
        } else if ( strcmp( argv[0], "?" ) == 0 ) {
            __ad5593->print_config();
        } else if ( strcmp( argv[0], "pin?" ) == 0 ) {
            for ( int pin = 0; pin < 8; ++pin ) {
                stream() << "pin " << pin << " function = " << __ad5593->function( pin ) << std::endl;
            }            
        } else if ( strcmp( argv[0], "value?" ) == 0 ) {
            for ( int pin = 0; pin < 8; ++pin ) {
                stream() << "pin " << pin << " = " << __ad5593->value( pin ) << std::endl;
            }
        } else if ( strcmp( argv[0], "value" ) == 0 ) {
            int pin = 0;
            while ( argc && std::isdigit( *argv[1] ) && pin < 8 ) {
                uint32_t value = strtod( argv[1] );
                __ad5593->set_value( pin++, value );
                --argc; ++argv;                
            }
        }
    }

    if ( __ad5593->is_dirty() ) {
        __ad5593->commit();
        stream() << "commit config to AD5593R" << std::endl;
    }
}
