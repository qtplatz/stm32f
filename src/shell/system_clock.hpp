// Copyright (C) 2018 MS-Cheminformatics LLC
// Licence: CC BY-NC
// Author: Toshinobu Hondo, Ph.D.
// Contact: toshi.hondo@qtplatz.com
//

#pragma once

#include <chrono>
#include <ctime>

namespace stm32f103 {

    struct system_clock {

        using rep  = std::int64_t;
        // denominator should be 2^n in order to avoid 64bit div computation
        // 16384 := 61.035us resolution
        using period = std::ratio<1,16384>;

        using duration = std::chrono::duration< rep, period >;
        using time_point = std::chrono::time_point< system_clock >;
        
        static constexpr bool is_steady = true;
        
        static time_point now() noexcept;

        static std::time_t to_time_t( const time_point& t ) noexcept;

        static constexpr time_point zero = time_point{ duration{ 0 } };

        static void set_epoch_compensation( uint32_t );
    private:
        static uint32_t epoch_compensation;
    };

}
