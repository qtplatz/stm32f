// Copyright (C) 2018 MS-Cheminformatics LLC
// Licence: CC BY-NC
// Author: Toshinobu Hondo, Ph.D.
// Contact: toshi.hondo@qtplatz.com
//

#include "gpio_mode.hpp"

namespace stm32f103 {

    void
    gpio_mode::set( stm32f103::GPIO_BASE base, int pin, stm32f103::GPIO_CNF cnf, stm32f103::GPIO_MODE mode )
    {
        if ( auto GPIO = reinterpret_cast< volatile stm32f103::GPIO * >( base ) ) {
            const uint32_t shift = ( pin % 8 ) * 4;
            const uint32_t mask  = 0x0f << shift;
            
            if ( pin < 8 ) {
                GPIO->CRL &= ~mask;
                GPIO->CRL |= ((cnf << 2) | mode ) << shift;
            } else {
                GPIO->CRH &= ~mask;
                GPIO->CRH |= ((cnf << 2) | mode ) << shift;
            }
        }
    }

    uint16_t
    gpio_mode::get( stm32f103::GPIO_BASE base, int pin )
    {
        if ( auto GPIO = reinterpret_cast< volatile stm32f103::GPIO * >( base ) ) {
            const uint32_t shift = ( pin % 8 ) * 4;
            const uint32_t mask  = 0x0f << shift;
            
            if ( pin < 8 ) {
                return ( GPIO->CRL & mask ) >> shift;
            } else {
                return ( GPIO->CRH & mask ) >> shift;
            }
        }
        return (-1);
    }    

    template<> void gpio_mode::operator()( stm32f103::GPIOA_PIN pin, stm32f103::GPIO_CNF cnf, stm32f103::GPIO_MODE mode ) const {
        set( stm32f103::GPIOA_BASE, pin, cnf, mode );
    }
    
    template<> void gpio_mode::operator()( stm32f103::GPIOB_PIN pin, stm32f103::GPIO_CNF cnf, stm32f103::GPIO_MODE mode ) const {
        set( stm32f103::GPIOB_BASE, pin, cnf, mode );
    }
    
    template<> void gpio_mode::operator()( stm32f103::GPIOC_PIN pin, stm32f103::GPIO_CNF cnf, stm32f103::GPIO_MODE mode ) const {
        set( stm32f103::GPIOC_BASE, pin, cnf, mode );
    }

    template<> uint16_t gpio_mode::operator()( stm32f103::GPIOA_PIN pin ) const {
        return get( stm32f103::GPIOA_BASE, pin );
    }
    
    template<> uint16_t gpio_mode::operator()( stm32f103::GPIOB_PIN pin ) const {
        return get( stm32f103::GPIOB_BASE, pin );
    }
    
    template<> uint16_t gpio_mode::operator()( stm32f103::GPIOC_PIN pin ) const {
        return get( stm32f103::GPIOC_BASE, pin );
    }

    constexpr const char * __input_mode_string__ [] = {
        "GPIO_INPUT_ANALOG" // 0
        , "GPIO_INPUT_FLOATING" // = 1
        , "GPIO_INPUT_PUSH_PULL" // = 2
        , "n/a"
    };
    
    constexpr const char * __output_mode_string__ [] = {
        // 0x00 input input
        "n/a"                                         // 0,0
        , "GPIO_CNF_OUTPUT_PUSH_PULL,OUTPUT_10M"      // 0,1
        , "GPIO_CNF_OUTPUT_PUSH_PULL,OUTPUT_2M"       // 0,2
        , "GPIO_CNF_OUTPUT_PUSH_PULL,OUTPUT_50M"      // 0,3
        , "n/a"                                       // 1,0
        , "GPIO_CNF_OUTPUT_ODRAIN,OUTPUT_10M"         // 1,1
        , "GPIO_CNF_OUTPUT_ODRAIN,OUTPUT_10M"         // 1,2
        , "GPIO_CNF_OUTPUT_ODRAIN,OUTPUT_10M"         // 1,3
        , "n/a"                                       // 2,0
        , "GPIO_CNF_ALT_OUTPUT_PUSH_PULL,OUTPUT_10M"  // 2,1
        , "GPIO_CNF_ALT_OUTPUT_PUSH_PULL,OUTPUT_2M"  // 2,2
        , "GPIO_CNF_ALT_OUTPUT_PUSH_PULL,OUTPUT_50M"  // 2,3
        , "n/a"
        , "GPIO_CNF_ALT_OUTPUT_ODRAIN,OUTPUT_10M"     // 3,1
        , "GPIO_CNF_ALT_OUTPUT_ODRAIN,OUTPUT_2M"     // 3,2
        , "GPIO_CNF_ALT_OUTPUT_ODRAIN,OUTPUT_50M"     // 3,3
    };

    const char * gpio_mode::toString( uint16_t mode ) {
        if ( ( mode & 03 ) == 0 )
            return __input_mode_string__[ ( mode >> 2 ) & 0x03 ];
        else
            return __output_mode_string__[ mode & 0x0f ];
    }
}
