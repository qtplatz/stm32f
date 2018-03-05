// Copyright (C) 2018 MS-Cheminformatics LLC
// Licence: CC BY-NC
// Author: Toshinobu Hondo, Ph.D.
// Contact: toshi.hondo@qtplatz.com
//

#pragma once

#include <atomic>
#include <cstdint>

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

    enum DMA_CHANNEL {
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
    class dma_channel_t {
        dma& dma_;
        DMA_CHANNEL channel_;
        const uint8_t * source_addr_;
        uint8_t * destination_addr_;
        uint32_t transfer_size_;
    public:
        constexpr const static uint8_t * peripheral_addr = 0;

        dma_channel_t( dma& dma
                       , const uint8_t * source_addr
                       , uint8_t * destination_addr
                       , uint32_t transfer_size ) : dma_( dma )
                                                  , source_addr_( source_addr )
                                                  , destination_addr_( destination_addr )
                                                  , transfer_size_( transfer_size ) {
            
            dma.init_channel( channel(), configuration(), source_addr_, destination_addr_, transfer_size_ );
        }

        uint32_t channel() const { return 1; }
        uint32_t configuration() const { return 0; } 

        void source_address( const uint8_t * src ) {
            source_addr_ = src;
        }

        void destination_address( uint8_t * dest ) {
            destination_addr_ = dest;
        }

        void transfer_size( uint32_t sz ) {
            transfer_size_ = sz;
        }
    };

    // template<> uint32_t dma_channel_t< DMA_SPI1_TX >::configuration() const {
    //     // MEM2MEM disabled (bit14)
    //     // Memory size 8bits (bit 11:10]
    //     return DMA_PL_MEDIUM | DMA_ReadFromMemory | 2 | 1; // complete irq, channel enable
    // };
    
}

