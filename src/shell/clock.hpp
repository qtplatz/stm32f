// Copyright (C) 2018 MS-Cheminformatics LLC
// Licence: CC BY-NC
// Author: Toshinobu Hondo, Ph.D.
// Contact: toshi.hondo@qtplatz.com
//

#pragma once

#include <chrono>
#include <cstdint>

struct clock {
    using rep  = std::int64_t;
    using period = std::ratio<1,1024>;
    using duration = std::chrono::duration< rep, period >;
    using time_point = std::chrono::time_point< clock >;

    static constexpr bool is_steady = true;

    static time_point now() noexcept;
    static constexpr time_point zero = time_point{ duration{ 0 } };
};
