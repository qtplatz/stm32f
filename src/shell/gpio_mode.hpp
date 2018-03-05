// Copyright (C) 2018 MS-Cheminformatics LLC
// Licence: CC BY-NC
// Author: Toshinobu Hondo, Ph.D.
// Contact: toshi.hondo@qtplatz.com
//

#pragma once

#include <cstdint>
#include <cstddef>
#include "stm32f103.hpp"

namespace stm32f103 {

    class gpio_mode {
        static void set( stm32f103::GPIO_BASE base, int pin, stm32f103::GPIO_CNF cnf, stm32f103::GPIO_MODE mode );
        static uint16_t get( stm32f103::GPIO_BASE base, int pin );
    public:
        template< typename GPIO_PIN_type >
        void operator()( GPIO_PIN_type pin, stm32f103::GPIO_CNF cnf, stm32f103::GPIO_MODE mode ) const;

        template< typename GPIO_PIN_type >
        uint16_t operator()( GPIO_PIN_type pin ) const;  // cnf << 2 | mode

        static const char * toString( uint16_t );
    };

}
