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
        static std::atomic_flag once_flag_;
    public:
        rtc();
        static bool enable();
        static void handle_interrupt();
    private:
        static void init();
        static void print_registers();
        
    };
}

