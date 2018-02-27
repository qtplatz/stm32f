// Copyright (C) 2018 MS-Cheminformatics LLC
// Licence: CC BY-NC
// Author: Toshinobu Hondo, Ph.D.
// Contact: toshi.hondo@qtplatz.com
//

#pragma once

#include <bitset>
#include <cstdint>
#include <cstddef>
#include <type_traits>
#include "stm32f103.hpp"

namespace stm32f103 {

    struct gpio_base {
        template< typename PIN_type > volatile GPIO * operator()( PIN_type pin ) const;
    };

    template< typename GPIO_PIN_type >
    class gpio {
        GPIO_PIN_type pin_;
        gpio( const gpio& ) = delete;
        const gpio& operator = ( const gpio& ) = delete;
    public:
        gpio( GPIO_PIN_type pin ) : pin_( pin ) {}
        
        void operator = ( bool flag ) {
            if ( flag )
                gpio_base()( pin_ )->BSRR = ( 1 << pin_ ); // set
            else
                gpio_base()( pin_ )->BRR = ( 1 << pin_ ); // reset
        }
    };
}
