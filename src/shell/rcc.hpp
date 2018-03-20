// Copyright (C) 2018 MS-Cheminformatics LLC
// Licence: CC BY-NC
// Author: Toshinobu Hondo, Ph.D.
// Contact: toshi.hondo@qtplatz.com
//


#pragma once

#include <atomic>
#include <cstdint>

namespace stm32f103 {

    class rcc {
    public:
        uint32_t system_frequency() const;
        uint32_t pclk1() const;
        uint32_t pclk2() const;
    };

    enum RCC_APB1ENR {
        RCC_APB1ENR_TIM2EN    = 0x00000001
        , RCC_APB1ENR_TIM3EN    = 0x00000002
        , RCC_APB1ENR_TIM4EN    = 0x00000004
        , RCC_APB1ENR_USART2EN  = 0x00020000
        , RCC_APB1ENR_USART3EN  = 0x00040000
        , RCC_APB1ENR_CANEN     = 0x02000000
        , RCC_APB1ENR_BKPEN     = 0x08000000
        , RCC_APB1ENR_PWREN     = 0x10000000
    };

    enum RCC_APB2ENR {
        RCC_APB2ENR_AFIOEN    = 0x00000001
        , RCC_APB2ENR_IOPAEN    = 0x00000004
        , RCC_APB2ENR_IOPBEN    = 0x00000008
        , RCC_APB2ENR_IOPCEN    = 0x00000010
        , RCC_APB2ENR_IOPDEN    = 0x00000020
        , RCC_APB2ENR_IOPEEN    = 0x00000040
        , RCC_APB2ENR_IOPFEN    = 0x00000080
        , RCC_APB2ENR_IOPGEN    = 0x00000100
        , RCC_APB2ENR_ADC1EN    = 0x00000200
        , RCC_APB2ENR_ADC2EN    = 0x00000400
        , RCC_APB2ENR_TIM1EN    = 0x00000800
        , RCC_APB2ENR_SPI1EN    = 0x00001000
        , RCC_APB2ENR_USART1EN  = 0x00004000
    };

    /* register RCC_CSR ---------------------------------------------------------*/
    enum RCC_CSR {
        RCC_CSR_LSION         	= 0x00000001
        , RCC_CSR_LSIRDY        = 0x00000002
    };

    enum RCC_BDCR {
        RCC_BDCR_LSEON        	= 0x00000001
        , RCC_BDCR_LSERDY       	= 0x00000002
        , RCC_BDCR_RTCSEL       	= 0x00000300
        , RCC_BDCR_RTCEN        	= 0x00008000
    };

}

