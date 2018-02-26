// Copyright (C) 2018 MS-Cheminformatics LLC

#include <cstdint>

namespace stm32f103 {

    enum SPI_BASE : uint32_t;
    struct SPI;

    class spi {
        volatile SPI * spi_;
    public:
        void init( SPI_BASE );
        spi& operator << ( uint16_t );
    };

}

