// Copyright (C) 2018 MS-Cheminformatics LLC
// Licence: CC BY-NC
// Author: Toshinobu Hondo, Ph.D.
// Contact: toshi.hondo@qtplatz.com
//

#include "clock.hpp"
#include "rtc.hpp"

clock::time_point
clock::now() noexcept
{
    uint32_t div;
    auto seconds = stm32f103::rtc::instance()->clock(div);
    auto tp = seconds * 1000 + ( div * 1000 / stm32f103::rtc::clock(div) );

    return time_point{ duration{ tp } };
}

