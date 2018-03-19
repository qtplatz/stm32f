// Copyright (C) 2018 MS-Cheminformatics LLC
// Licence: CC BY-NC
// Author: Toshinobu Hondo, Ph.D.
// Contact: toshi.hondo@qtplatz.com
//

#include "rtc.hpp"
#include "stream.hpp"
#include "stm32f103.hpp"
#include "bitset.hpp"
#include <cstdint>

extern "C" {
    void __rtc_handler();
    void enable_interrupt( stm32f103::IRQn_type IRQn );
}

extern std::atomic< uint32_t > atomic_jiffies;          //  100us  (4.97 days)

namespace stm32f103 {

    std::atomic_flag rtc::once_flag_;
    
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

void
rtc::handle_interrupt()
{
}

bool
rtc::enable()
{
    
}

