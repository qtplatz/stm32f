// Copyright (C) 2018 MS-Cheminformatics LLC
// Licence: CC BY-NC
// Author: Toshinobu Hondo, Ph.D.
// Contact: toshi.hondo@qtplatz.com
//

#include "adc.hpp"
#include "stm32f103.hpp"
#include <mutex>

extern "C" {
    void adc1_handler();
}

namespace stm32f103 {
    static adc * __adc1;
}

using namespace stm32f103;

adc::adc()
{
    __adc1 = this;  // constractor may not be called if instance is declared in global space
}

adc::~adc()
{
}

void
adc::init()
{
    __adc1 = this; // constractor may not be called if instance is declared in global space
    
    // RM0008, p251, ADC register map
    if ( auto ADC = reinterpret_cast< stm32f103::ADC * >( stm32f103::ADC1_BASE ) ) {
        ADC->CR1 |= (1 << 5);  // Enable end of conversion (EOC) interrupt
    }
}

// static
void
adc::inq_handler( adc * _this )
{
}

void
adc1_handler()
{
    
}
