// Copyright (C) 2018 MS-Cheminformatics LLC
// Licence: CC BY-NC
// Author: Toshinobu Hondo, Ph.D.
// Contact: toshi.hondo@qtplatz.com
//

#include "gpio.hpp"

namespace stm32f103 {
    template<> volatile GPIO * gpio_base::operator()( GPIOA_PIN pin ) const { return reinterpret_cast< volatile GPIO * >( GPIOA_BASE ); }
    template<> volatile GPIO * gpio_base::operator()( GPIOB_PIN pin ) const { return reinterpret_cast< volatile GPIO * >( GPIOB_BASE ); }
    template<> volatile GPIO * gpio_base::operator()( GPIOC_PIN pin ) const { return reinterpret_cast< volatile GPIO * >( GPIOC_BASE ); }
}

