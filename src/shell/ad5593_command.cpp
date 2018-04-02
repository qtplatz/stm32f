// Copyright (C) 2018 MS-Cheminformatics LLC
// Licence: CC BY-NC
// Author: Toshinobu Hondo, Ph.D.
// Contact: toshi.hondo@qtplatz.com
//

#include "ad5593.hpp"
#include "debug_print.hpp"
#include "dma.hpp"
#include "i2c.hpp"
#include "stm32f103.hpp"
#include "stream.hpp"
#include "timer.hpp"
#include "utility.hpp"

namespace ad5593 {
    ad5593::AD5593 * __ad5593;
    static uint8_t __ad5593_allocator__[ sizeof( ad5593::AD5593 ) ];
}

extern void i2c_command( size_t argc, const char ** argv );
void mdelay ( uint32_t ms );

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
            
        } else if ( strcmp( argv[0], "??" ) == 0 ) {  // ad5593 dac 0 1 2...

            if ( __ad5593->fetch() )
                __ad5593->print_config( stream(__FILE__,__LINE__) );
            else
                stream(__FILE__,__LINE__) << "fetch failed\n";
                
        } else if ( strcmp( argv[0], "pd" ) == 0 ) {  // internal reference
            __ad5593->write( std::array< uint8_t, 3 >{ 0x0b, 02, 0x00 } );

        } else if ( strcmp( argv[0], "config" ) == 0 ) {
            if ( argc == 1 ) {
                for ( int pin = 0; pin < 4; ++pin )
                    __ad5593->set_function( pin, ad5593::DAC );
                
                for ( int pin = 4; pin < 8; ++pin )
                    __ad5593->set_function( pin, ad5593::ADC );
                __ad5593->commit();

                if ( ! __ad5593->set_adc_sequence( __ad5593->adc_enabled() | 0x0200 ) )
                    stream(__FILE__,__LINE__) << "ADC sequence set failed\n";
                
                stream() << "ad5593 default configuration applied. -- it can be changed as\n"
                         << "\tad5593 config [adc|dac] (ex., config dac 00f adc 0f0)\n";
            } else {
                while ( --argc ) {
                    ++argv;
                    auto function = strcmp( argv[0], "dac" ) == 0 ? ad5593::DAC : ad5593::ADC;
                    if ( argc ) {
                        --argc; ++argv;
                        std::bitset< 8 > pins( strtox( argv[1] ) );
                        for ( size_t pin = 0; pin < 8; ++pin ) {
                            if ( pins.test( pin ) )
                                __ad5593->set_function( pin, function );
                        }
                    }
                }
                __ad5593->commit();
                if ( ! __ad5593->set_adc_sequence( __ad5593->adc_enabled() | 0x0200 ) )
                    stream(__FILE__,__LINE__) << "ADC sequence set failed\n";                
            }
        } else if ( strcmp( argv[0], "dac" ) == 0 ) {  // ad5593 dac 0 1 2...

        } else if ( strcmp( argv[0], "adc?" ) == 0 ) {  // ad5593 adc 0 1 2...
            std::array< uint16_t, 4 > data( { 0 } );
            if ( __ad5593->read_adc_sequence( data ) ) {
                print()( stream(__FILE__,__LINE__), data, data.size(), "ADC\t" ) << std::endl;
            } else {
                stream() << "adc sequence got failed.\n";
            }

        } else if ( strcmp( argv[0], "pin?" ) == 0 ) {
            for ( int pin = 0; pin < 8; ++pin ) {
                stream() << "pin " << pin << " function = " << __ad5593->function( pin ) << std::endl;
            }            
        } else if ( strcmp( argv[0], "value?" ) == 0 ) {
            __ad5593->print_values( stream() );
        } else if ( strcmp( argv[0], "value" ) == 0 ) {
            int pin = 0;
            while ( argc && std::isdigit( *argv[1] ) && pin < 8 ) {
                --argc; ++argv;
                uint32_t value = strtod( argv[0] );
                __ad5593->set_value( pin++, value );
            }
        } else if ( strcmp( argv[0], "stop" ) == 0 ) {

            stm32f103::timer_t< stm32f103::TIM3_BASE >::clear_callback();
            
        } else if ( strcmp( argv[0], "--ramp" ) == 0 ) {

            __ad5593->reset();

            for ( int pin = 0; pin < 4; ++pin )
                __ad5593->set_function( pin, ad5593::DAC );

            for ( int pin = 4; pin < 8; ++pin )
                __ad5593->set_function( pin, ad5593::ADC );
            __ad5593->commit();

            if ( ! __ad5593->set_adc_sequence( 0x03f0 ) )
                stream(__FILE__,__LINE__) << "ADC sequence set failed\n";

            stm32f103::timer_t< stm32f103::TIM3_BASE >().enable( false );
            stm32f103::timer_t< stm32f103::TIM3_BASE >().set_interval( 1000 ); // 100ms interval
            stm32f103::timer_t< stm32f103::TIM3_BASE >().enable( true );

            stm32f103::timer_t< stm32f103::TIM3_BASE >().set_callback( +[]{
                    static uint32_t value;
                    static uint32_t pin;
                    static bool flag;

                    flag = !flag;

                    if ( flag ) {
                        if ( pin >= 4 )
                            pin = 0;

                        if ( pin == 0 ) {
                            value += 8;
                            if ( value >= 4095 )
                                value = 0;
                        }
                        __ad5593->set_value( pin++, value );
                    } else {
                        std::array< uint16_t, 5 > adc( { 0 } );
                        if ( __ad5593->read_adc_sequence( adc ) )
                            __ad5593->print_adc_sequence( std::move(stream() << "\t"), adc.data(), adc.size() );
                    }
                });
        }
    }
}
