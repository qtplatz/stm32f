// Copyright (C) 2018 MS-Cheminformatics LLC
// Licence: CC BY-NC
// Author: Toshinobu Hondo, Ph.D.
// Contact: toshi.hondo@qtplatz.com
//

#pragma once

#include <cstdint>

class stream;

namespace stm32f103 {

    class i2c;

    class i2c_string {
    public:
        static stream& CR1( stream&, uint16_t reg, volatile struct I2C * = nullptr );
        static stream& CR2( stream&, uint16_t reg, volatile struct I2C * = nullptr );
        static stream& OAR1( stream&, uint16_t reg, volatile struct I2C * = nullptr );
        static stream& OAR2( stream&, uint16_t reg, volatile struct I2C * = nullptr );
        static stream& SR1( stream&, uint16_t reg, volatile struct I2C * = nullptr );
        static stream& SR2( stream&, uint16_t reg, volatile struct I2C * = nullptr );
        static stream& DR( stream&, uint16_t reg, volatile struct I2C * = nullptr );
        static stream& CCR( stream&, uint16_t reg, volatile struct I2C * = nullptr );
        static stream& TRISE( stream&, uint16_t reg, volatile struct I2C * );

        static stream& status32( stream&&, uint32_t reg, volatile struct I2C * );
        static stream& print_registers( stream&&, volatile struct I2C * );
    };
}

