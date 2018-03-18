// Copyright (C) 2018 MS-Cheminformatics LLC
// Licence: CC BY-NC
// Author: Toshinobu Hondo, Ph.D.
// Contact: toshi.hondo@qtplatz.com
//

#include "gpio.hpp"
#include "gpio_mode.hpp"
#include "stream.hpp"

void
gpio_command( size_t argc, const char ** argv )
{
    using namespace stm32f103;

    const size_t replicates = 0x7fffff;
    
    if ( argc >= 2 ) {
        const char * pin = argv[1];
        int no = 0;
        if (( pin[0] == 'P' && ( 'A' <= pin[1] && pin[1] <= 'C' ) ) && ( pin[2] >= '0' && pin[2] <= '9' ) ) {
            no = pin[2] - '0';
            if ( pin[3] >= '0' && pin[3] <= '9' )
                no = no * 10 + pin[3] - '0';

            stream() << "Pulse out to P" << pin[1] << no << std::endl;

            switch( pin[1] ) {
            case 'A':
                gpio_mode()( static_cast< GPIOA_PIN >(no), GPIO_CNF_OUTPUT_PUSH_PULL, GPIO_MODE_OUTPUT_50M );
                stream() << pin << ": " << gpio_mode::toString( gpio_mode()( static_cast< GPIOA_PIN >(no) ) ) << std::endl;
                for ( size_t i = 0; i < replicates; ++i )
                    gpio< GPIOA_PIN >( static_cast< GPIOA_PIN >( no ) ) = bool( i & 01 );
                break;
            case 'B':
                gpio_mode()( static_cast< GPIOB_PIN >(no), GPIO_CNF_OUTPUT_PUSH_PULL, GPIO_MODE_OUTPUT_50M );
                stream() << pin << ": " << gpio_mode::toString( gpio_mode()( static_cast< GPIOB_PIN >(no) ) ) << std::endl;
                for ( size_t i = 0; i < replicates; ++i )
                    gpio< GPIOB_PIN >( static_cast< GPIOB_PIN >( no ) ) = bool( i & 01 );
                break;                
            case 'C':
                gpio_mode()( static_cast< GPIOC_PIN >(no), GPIO_CNF_OUTPUT_PUSH_PULL, GPIO_MODE_OUTPUT_50M );
                stream() << pin << ": " << gpio_mode::toString( gpio_mode()( static_cast< GPIOC_PIN >(no) ) ) << std::endl;
                for ( size_t i = 0; i < replicates; ++i )
                    gpio< GPIOC_PIN >( static_cast< GPIOC_PIN >( no ) ) = bool( i & 01 );
                break;                                
            }
            
        } else {
            stream() << "gpio 2nd argment format mismatch" << std::endl;
        }
    } else {
        for ( int i = 0; i < 16;  ++i )
            stream() << "PA" << i << ":\t" << gpio_mode::toString( gpio_mode()( static_cast< GPIOA_PIN >(i) ) ) << std::endl;
        for ( int i = 0; i < 16;  ++i )
            stream() << "PB" << i << ":\t" << gpio_mode::toString( gpio_mode()( static_cast< GPIOB_PIN >(i) ) ) << std::endl;
        for ( int i = 13; i < 16;  ++i )
            stream() << "PC" << i << ":\t" << gpio_mode::toString( gpio_mode()( static_cast< GPIOC_PIN >(i) ) ) << std::endl;
        
        stream() << "gpio <pin#>" << std::endl;
    }
}

