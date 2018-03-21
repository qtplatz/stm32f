// Copyright (C) 2018 MS-Cheminformatics LLC
// Licence: CC BY-NC
// Author: Toshinobu Hondo, Ph.D.
// Contact: toshi.hondo@qtplatz.com
//

#include "system_clock.hpp"
#include "rtc.hpp"
#include "stream.hpp"

namespace stm32f103 {

    constexpr system_clock::time_point system_clock::zero;

    time_t system_clock::epoch_compensation;

    system_clock::time_point
    system_clock::now() noexcept
    {
        uint32_t div;
        auto seconds = stm32f103::rtc::instance()->clock(div);
        auto tp = seconds * system_clock::period::den + uint32_t( div * system_clock::period::den ) / stm32f103::rtc::clock(div);

        return time_point{ duration{ tp } };
    }

    std::time_t
    system_clock::to_time_t( const time_point& t ) noexcept
    {
        return std::chrono::duration_cast< std::chrono::seconds >( t - zero ).count() + stm32f103::rtc::__epoch_offset__; // 2018-JAN-01 00:00 UTC
    }
}
