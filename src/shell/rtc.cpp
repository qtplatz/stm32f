// Copyright (C) 2018 MS-Cheminformatics LLC
// Licence: CC BY-NC
// Author: Toshinobu Hondo, Ph.D.
// Contact: toshi.hondo@qtplatz.com
//

#include "bitset.hpp"
#include "condition_wait.hpp"
#include "debug_print.hpp"
#include "rcc.hpp"
#include "rtc.hpp"
#include "stream.hpp"
#include "stm32f103.hpp"
#include <bitset>
#include <cstdint>

extern "C" {
    void __rtc_handler();
    void enable_interrupt( stm32f103::IRQn_type IRQn );
}

extern std::atomic< uint32_t > atomic_jiffies;          //  100us  (4.97 days)

namespace stm32f103 {
    struct RTC {
        uint32_t CRH;
        uint32_t CRL;
        uint32_t PRLH;
        uint32_t PRLL;
        uint32_t DIVH;
        uint32_t DIVL;
        uint32_t CNTH;
        uint32_t CNTL;
        uint32_t ALRH;
        uint32_t ALRL;
    };

    constexpr static const char * rtc_register_names [] = {
        "CRH", "CRL", "PRLH", "PRLL", "DIVH", "DIVL", "CNTH", "CNTL", "ALRH", "ALRL"
    };

    // Table 15, PWR register map, p79
    struct PWR {
        uint32_t CR;
        uint32_t CSR;
    };

    /*----------------------------------------------------------------------------
      RTC
      *----------------------------------------------------------------------------*/
    enum RTC_CRL {
        RTC_CRL_SECF          	= 0x00000001
        , RTC_CRL_ALRF          	= 1 << 1 //0x00000002
        , RTC_CRL_OWF           	= 1 << 2 //0x00000004
        , RTC_CRL_RSF           	= 1 << 3 //0x00000008
        , RTC_CRL_CNF           	= 1 << 4 //0x00000010
        , RTC_CRL_RTOFF         	= 1 << 5 //0x00000020
    };

    constexpr uint32_t __rtc_period = 1000;
    constexpr uint32_t __rtc_time_h = 12;
    constexpr uint32_t __rtc_time_m = 0;
    constexpr uint32_t __rtc_time_s = 0;
    constexpr uint32_t __rtc_cnt_tics = __rtc_time_h * 3600ul + __rtc_time_m * 60ul + __rtc_time_s;
    constexpr uint32_t __rtc_cnt = __rtc_cnt_tics * 1000ul/__rtc_period;
    constexpr uint32_t __rtc_alarm_h = 12;
    constexpr uint32_t __rtc_alarm_m = 0;
    constexpr uint32_t __rtc_alarm_s = 0;
    constexpr uint32_t __rtc_alr_tics = __rtc_alarm_h * 3600ul + __rtc_alarm_m * 60ul + __rtc_alarm_s;
    constexpr uint32_t __rtc_alr = __rtc_alr_tics * 1000ul/__rtc_period;
}

using namespace stm32f103;

rtc::rtc()
{
}

// static
void
rtc::init()
{
}

bool
rtc::enable()
{
    if ( auto RCC = reinterpret_cast< volatile stm32f103::RCC * >( stm32f103::RCC_BASE ) ) {
        using namespace stm32f103;
        RCC->APB1ENR |= RCC_APB1ENR_PWREN;

        if ( auto PWR = reinterpret_cast< volatile stm32f103::PWR * >( stm32f103::PWR_BASE ) ) {
            PWR->CR |= 0x100; // enable access to RTC, BDC registers
        }

        stm32f103::bitset::set( RCC->BDCR, stm32f103::RCC_BDCR_LSEON );
        condition_wait()( [&](){ return RCC->BDCR & stm32f103::RCC_BDCR_LSERDY; } );

        // rtcsel [9:8], p149
        // 00 no clock, 01 := LSE, 10 := LSI, 11 := HSE / 128
        stm32f103::bitset::set( RCC->BDCR, 0x100 | RCC_BDCR_RTCEN );  // set RTC clock source, enable RTC clock
    }

    if ( auto RTC = reinterpret_cast< volatile stm32f103::RTC * >( stm32f103::RTC_BASE ) ) {

        stm32f103::bitset::set( RTC->CRL, RTC_CRL_CNF );      // set configuration mode

        RTC->PRLH  = ((__rtc_period/1000-1)>>16) & 0x00ff;    // set prescaler load register high
        RTC->PRLL  = ((__rtc_period*32768/1000-1)) & 0xffff;  // set prescaler load register low
        RTC->CNTH  = ((__rtc_cnt)>>16) & 0xffff;              // set counter high
        RTC->CNTL  = ((__rtc_cnt)    ) & 0xffff;              // set counter low
        RTC->ALRH  = ((__rtc_alr)>>16) & 0xffff;              // set alarm high
        RTC->ALRL  = ((__rtc_alr)    ) & 0xffff;              // set alarm low

        RTC->CRH = 3;                                         // enable RTC alarm, second interrupts
        enable_interrupt( RTC_IRQn );                         // enable interrupt
        
        stm32f103::bitset::reset( RTC->CRL, RTC_CRL_CNF );    // reset configuration mode

        if ( condition_wait()( [&](){ return RTC->CRL & RTC_CRL_RTOFF; } ) ) {
            if ( auto PWR = reinterpret_cast< volatile stm32f103::PWR * >( stm32f103::PWR_BASE ) )
                stm32f103::bitset::reset( PWR->CR, 0x100 );
        }
    }
    return true;
}

void
rtc::handle_interrupt() const
{
    if ( auto RTC = reinterpret_cast< volatile stm32f103::RTC * >( stm32f103::RTC_BASE ) ) {
        if ( stm32f103::bitset::test( RTC->CRL, 1 ) )
            stm32f103::bitset::reset( RTC->CRL, 1 );
    }
    stream() << "rtc::handle_interrupt" << std::endl;
}

void
rtc::print_registers()
{
    stream() << "See section 18.4, p486- on RM0008\n";
    auto * reg = reinterpret_cast< volatile uint32_t * >( stm32f103::RTC_BASE );
    for ( auto& name: rtc_register_names ) {
        std::bitset<16> bits( *reg );
        print()( stream(), bits, name ) << "\t" << *reg << std::endl;
        ++reg;
    }
}

rtc *
rtc::instance()
{
    static std::atomic_flag __once_flag;
    static rtc __instance;
    if ( !__once_flag.test_and_set() )
        __instance.init();
    return &__instance;
}

void
__rtc_handler()
{
    rtc::instance()->handle_interrupt();
}

