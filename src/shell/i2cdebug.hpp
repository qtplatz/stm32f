// Copyright (C) 2018 MS-Cheminformatics LLC
// Licence: CC BY-NC
// Author: Toshinobu Hondo, Ph.D.
// Contact: toshi.hondo@qtplatz.com
//

#pragma once

#include <cstdint>

namespace stm32f103 {
    namespace i2cdebug {
        const char * CR1_to_string( uint32_t reg );
        const char * CR2_to_string( uint32_t reg );
        const char * SR1_to_string( uint32_t reg );
        const char * SR2_to_string( uint32_t reg );
        const char * status32_to_string( uint32_t reg );
    }
}

