// Copyright (C) 2018 MS-Cheminformatics LLC
// Licence: CC BY-NC
// Author: Toshinobu Hondo, Ph.D.
// Contact: toshi.hondo@qtplatz.com
//

#pragma once

#include <atomic>
#include <cstdint>
#include "stm32f103.hpp"

namespace stm32f103 {

    class dma;

    // DMA1
    // Channel1 := ADC1
    // Channel2 := SPI1_RX | USART3_TX
    // Channel3 := SPI1_TX | USART3_RX
    // Channel4 := SPI2_RX | USART1_TX | I2C2_TX
    // Channel5 := SPI2_TX | USART1_RX | I2C2_RX
    // Channel6 := USART2_RX | I2C1_TX
    // Channel7 := USART2_TX | I2C1_RX

    enum DMA_CHANNEL : uint32_t {
        DMA_ADC1 = 1
        , DMA_SPI1_RX = 2
        , DMA_SPI1_TX = 3
        //, DMA_SPI2_RX = 4
        //, DMA_SPI2_TX = 5
        , DMA_I2C1_TX = 6
        , DMA_I2C1_RX = 7
    };

    // p286, bit4
    enum DMA_DIR {
        DMA_ReadFromPeripheral = 0
        , DMA_ReadFromMemory   = (1<<4)  // 0x0010
    };

    enum DMA_PRIORITY {
        DMA_PL_LOW = 0x00
        , DMA_PL_MEDIUM = 0x01
        , DMA_PL_HIGH = 0x02
        , DMA_PL_VERYHIGH = 0x03
    };

    template< DMA_CHANNEL >
    struct peripheral_address {
        static constexpr uint32_t value = 0;
    };

    template<> struct peripheral_address< DMA_I2C1_TX > {
        static constexpr uint32_t value = I2C1_BASE;
    };
    
    template<> struct peripheral_address< DMA_I2C1_RX > {
        static constexpr uint32_t value = I2C1_BASE;
    };

    template< DMA_CHANNEL >
    class dma_channel {
    public:
        static constexpr DMA_DIR dma_dir = DMA_ReadFromPeripheral;
        static constexpr uint32_t peripheral_address = 0;
    };

}

