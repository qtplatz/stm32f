// Copyright (C) 2018 MS-Cheminformatics LLC
// Licence: CC BY-NC
// Author: Toshinobu Hondo, Ph.D.
// Contact: toshi.hondo@qtplatz.com
//

#include <array>
#include <atomic>
#include <cstdint>
#include "scoped_spinlock.hpp"

namespace stm32f103 {

    enum RTC_BASE : uint32_t;

    class rtc {
        rtc( const rtc& ) = delete;
        rtc& operator = ( const rtc& ) = delete;
        rtc();
        static void init();        
    public:
        bool enable();
        void reset();
        static constexpr uint32_t clock_rate(); // Hz
        static uint32_t clock( uint32_t& div );
        static rtc * instance();
        static void print_registers();
        void handle_interrupt() const;

        constexpr static uint32_t __epoch_offset__ = 1514764800; // duration (seconds) from 1970-JAN-01 00:00 UTC to 2018-JAN-01 UTC
    };
}
