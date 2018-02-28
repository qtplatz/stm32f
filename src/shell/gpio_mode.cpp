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

    template<> void gpio_mode::operator()( stm32f103::GPIOA_PIN pin, stm32f103::GPIO_CNF cnf, stm32f103::GPIO_MODE mode ) const {
        set( stm32f103::GPIOA_BASE, pin, cnf, mode );
    }
    
    template<> void gpio_mode::operator()( stm32f103::GPIOB_PIN pin, stm32f103::GPIO_CNF cnf, stm32f103::GPIO_MODE mode ) const {
        set( stm32f103::GPIOB_BASE, pin, cnf, mode );
    }
    
    template<> void gpio_mode::operator()( stm32f103::GPIOC_PIN pin, stm32f103::GPIO_CNF cnf, stm32f103::GPIO_MODE mode ) const {
        set( stm32f103::GPIOC_BASE, pin, cnf, mode );
    }    
}