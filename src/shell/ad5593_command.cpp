// Copyright (C) 2018 MS-Cheminformatics LLC
// Licence: CC BY-NC
// Author: Toshinobu Hondo, Ph.D.
// Contact: toshi.hondo@qtplatz.com
//

#include "ad5593.hpp"
#include "dma.hpp"
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

        auto& i2cx = *stm32f103::i2c_t< stm32f103::I2C1_BASE >::instance();
        if ( !i2cx.has_dma( stm32f103::i2c::DMA_Both ) )
            i2cx.attach( *stm32f103::dma_t< stm32f103::DMA1_BASE >::instance(), stm32f103::i2c::DMA_Both );

        if ( __ad5593 = new ( __ad5593_allocator__ ) ad5593::AD5593( i2cx, 0x10 ) ) {
            if ( ! __ad5593->fetch() )
                stream() << "fetch error\n";
        }

        __ad5593->write( std::array< uint8_t, 3 >{ 0x0b, 0x02, 0x00 } ); // b9 set
        __ad5593->fetch();
    }
    
    double volts = 1.0;
    double Vref = 3.3;

    if ( argc == 1 ) {
        if ( __ad5593->fetch() )
            __ad5593->print_config( stream(__FILE__,__LINE__) );
        else
            stream(__FILE__,__LINE__) << "fetch failed\n";
    }

    while ( --argc ) {
        ++argv;
        if ( strcmp( argv[0], "?" ) == 0 ) {  // ad5593 dac 0 1 2...

            __ad5593->print_registers( stream(__FILE__,__LINE__) );
            
        } else if ( strcmp( argv[0], "fetch" ) == 0 ) {  // ad5593 dac 0 1 2...

            if ( __ad5593->fetch() )
                __ad5593->print_config( stream(__FILE__,__LINE__) );
            else
                stream(__FILE__,__LINE__) << "fetch failed\n";
                
        } else if ( strcmp( argv[0], "pd" ) == 0 ) {  // internal reference
            __ad5593->write( std::array< uint8_t, 3 >{ 0x0b, 02, 0x00 } );

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
                uint32_t value = strtox( argv[1] );
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
