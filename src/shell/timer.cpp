// Copyright (C) 2018 MS-Cheminformatics LLC
// Licence: CC BY-NC
// Author: Toshinobu Hondo, Ph.D.
// Contact: toshi.hondo@qtplatz.com
//

#include "timer.hpp"
#include "stream.hpp"
#include "stm32f103.hpp"
#include "bitset.hpp"
#include <cstdint>

extern "C" {
    void __tim2_handler();
    void enable_interrupt( stm32f103::IRQn_type IRQn );
}

extern std::atomic< uint32_t > atomic_jiffies;          //  100us  (4.97 days)

namespace stm32f103 {

    enum TIM_CR1_MASK : uint32_t {
        CEN     = 1         // 0x1
        , UIDS  = 1 << 1    // 0x2
        , URS   = 1 << 2    // 0x4
        , OPM   = 1 << 3    // 0x8
        , DIR   = 1 << 4    // 0x10
        , CMS   = 3 << 5    // 0x30
        , ARPE  = 1 << 7    // 0x40
        , CKD   = 3 << 8    // 0xc0
    };

    struct TIM {
        uint32_t CR1;
        uint32_t CR2;
        uint32_t SMCR;  // SMS=000
        uint32_t DIER;
        uint32_t SR;
        uint32_t EGR;
        uint32_t CCMR1;
        uint32_t CCMR2;
        uint32_t CCER;
        uint32_t CNT;
        uint32_t PSC;
        uint32_t ARR;
        uint32_t CCR1;
        uint32_t CCR2;
        uint32_t CCR3;
        uint32_t CCR4;
        uint32_t DCR;
        uint32_t DMAR;
    };
    static constexpr const char * register_names [] = {
        "CR1", "CR2", "SMCR", "DIER", "SR", "EGR", "CCMR1", "CCMR2", "CCER", "CNT", "PSC"
        , "ARR", "CCR1", "CCR2", "CCR3", "CCR4", "DCR", "DMAR"
    };

}

using namespace stm32f103;

timer::timer()
{
}

// static
void
timer::init( TIM_BASE base )
{
    stream() << "TIMER:" << base << " initializing\n";

    auto p = reinterpret_cast< volatile TIM * >( base );
    p->CR1 = 0;
    p->CR2 = 0;

    p->PSC = 7200 - 1;  // 10kHz
    p->ARR = 10000;     // ~= 1Hz
    
    p->CCR1 = 0;
    p->CCR2 = 0;
    p->CCR3 = 0;
    p->CCR4 = 0;
    p->CCMR1 = 0;
    p->CCMR2 = 0;
    p->CCER = 0;
    p->SMCR = 0;      // SMS = 000 := internal clock, 111 := external edge
    p->DIER = 1;      // interrupt enable

    p->CR1 = 4;
    p->CR2 = 0;
    
    switch( base ) {
    case TIM2_BASE: enable_interrupt( TIM2_IRQn ); break;
    case TIM3_BASE: enable_interrupt( TIM3_IRQn ); break;
    case TIM4_BASE: enable_interrupt( TIM4_IRQn ); break;
    case TIM5_BASE: enable_interrupt( TIM5_IRQn ); break;
    case TIM6_BASE: enable_interrupt( TIM6_IRQn ); break;
    case TIM7_BASE: enable_interrupt( TIM7_IRQn ); break;
    }
    p->CR1 |= 1;      // enable
}

void
timer::handle_interrupt()
{
}

void
timer::print_registers( TIM_BASE base )
{
    auto p = reinterpret_cast< volatile uint32_t * >( base );
    for ( const auto& reg: register_names )
        stream(__FILE__,__LINE__) << reg << "\t" << *p++ << "\n";
    stream() << std::endl;
}

void
__tim2_handler()
{
    auto p = reinterpret_cast< TIM * >( TIM2_BASE );    
	p->SR &= ~(1 << 0);
    stm32f103::timer_t< TIM2_BASE >::callback();
}

