// Copyright (C) 2018 MS-Cheminformatics LLC

#include <atomic>
#include <cstdint>

namespace stm32f103 {

    class rcc {
    public:
        uint32_t system_frequency() const;
        uint32_t pclk1() const;
        uint32_t pclk2() const;
    };
}

