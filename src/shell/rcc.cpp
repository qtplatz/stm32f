// Copyright (C) 2018 MS-Cheminformatics LLC
// Licence: CC BY-NC
// Author: Toshinobu Hondo, Ph.D.
// Contact: toshi.hondo@qtplatz.com
//

#include "rcc.hpp"
#include "stm32f103.hpp"
#include "stream.hpp"

namespace stm32f103 {
    enum {
        HSI_Frequency = 8000000
        , HSE_Frequency = 8000000
    };
}

using namespace stm32f103;

constexpr static uint8_t prescaler_table [] = {0, 0, 0, 0, 1, 2, 3, 4, 1, 2, 3, 4, 6, 7, 8, 9};

uint32_t
rcc::system_frequency() const
{
    if ( auto RCC = reinterpret_cast< volatile stm32f103::RCC * >( stm32f103::RCC_BASE ) ) {    
    
        auto sysclk_source = RCC->CFGR & 0x0c; // [3:2], SWS, 7.3.2, p100 RM0008
        uint32_t sysclk_frequency = 0;

        if ( sysclk_source == 0x08 ) { // PLL
            uint32_t pllmul = (( RCC->CFGR >> 18 ) & 0x0f) + 2;  // PLLMUL origin is x2
            bool pllsource = RCC->CFGR & 0x00010000; // b16

            if ( pllsource ) {
                uint32_t prediv1factor = (RCC->CFGR2 & 0x0f) + 1;
                return HSE_Frequency / prediv1factor * pllmul;
            } else {
                return ( HSI_Frequency / 2 ) * pllmul;
            }
        } else if ( sysclk_source == 0x04 ) {
            return HSE_Frequency;
        } else {
            return HSI_Frequency;
        }
    }
    return (-1);
}

uint32_t
rcc::pclk1() const
{
    if ( auto RCC = reinterpret_cast< volatile stm32f103::RCC * >( stm32f103::RCC_BASE ) ) {

        uint32_t prescaler = prescaler_table [ ( RCC->CFGR & 0xf0 ) >> 4 ];

        uint32_t hclk_freq = system_frequency() >> prescaler;


        auto pclk1_prescaler = prescaler_table[ ( RCC->CFGR & 0x0700 ) >> 8 ];

        return hclk_freq >> pclk1_prescaler;
    }
    return (-1);
}

uint32_t
rcc::pclk2() const
{
    if ( auto RCC = reinterpret_cast< volatile stm32f103::RCC * >( stm32f103::RCC_BASE ) ) {

        uint32_t prescaler = prescaler_table [ ( RCC->CFGR & 0xf0 ) >> 4 ];

        uint32_t hclk_freq = system_frequency() >> prescaler;

        auto pclk2_prescaler = prescaler_table[ ( RCC->CFGR & 0x3800 ) >> 11 ];

        return hclk_freq >> pclk2_prescaler;
    }
    return (-1);
}
    
