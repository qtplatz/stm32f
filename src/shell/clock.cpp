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
    return time_point{duration{ stm32f103::rtc::instance()->now() }};
}

