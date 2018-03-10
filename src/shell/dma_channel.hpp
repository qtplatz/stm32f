// Copyright (C) 2018 MS-Cheminformatics LLC
// Licence: CC BY-NC
// Author: Toshinobu Hondo, Ph.D.
// Contact: toshi.hondo@qtplatz.com
//

#pragma once

#include "stm32f103.hpp"
#include "stream.hpp"
#include <atomic>
#include <cstdint>

namespace stm32f103 {

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
        , DMA_I2C2_TX = 4
        , DMA_I2C2_RX = 5        
        , DMA_I2C1_TX = 6
        , DMA_I2C1_RX = 7
    };

    // p286, bit4
    enum DMA_DIR : uint32_t {
        DMA_ReadFromPeripheral = 0
        , DMA_ReadFromMemory   = (1<<4)  // 0x0010
    };

    enum DMA_PRIORITY : uint32_t {
        DMA_PL_LOW = 0x00
        , DMA_PL_MEDIUM = 0x01
        , DMA_PL_HIGH = 0x02
        , DMA_PL_VERYHIGH = 0x03
    };

    enum DMA_CCRx : uint32_t {
        MEM2MEM     = 1 << 14    // 0x4000
        , PL_MASK     = 3 << 12  // 0x3000
        , PL_Low      = 0        
        , PL_Medium   = 01 << 12 // 0x1000
        , PL_High     = 02 << 12 // 0x2000 
        , PL_VeryHigh = 03 << 12 // 0x3000
        , MSIZE_MASK  = 03 << 10 // 0x0c00 Memory size 0:8bit, 1:16bit, 2:32bit
        , PSIZE_MASK  = 03 << 8  // 0x0300 Peripheral size 0:8bit, 1:16bit, 2:32bit
        , MINC        = 1 << 7   // 0x0080 Memory increment mode, 0:disabled, 1:enabled
        , PINC        = 1 << 6   // 0x0040 Peripheral increment mode, 0:disabled, 1:enabled
        , CIRC        = 1 << 5   // 0x0020 Circular mode, 0:disabled 1:enabled
        , DIR         = 1 << 4   // 0x0010 Data transfer direction, 0:Read from pheripheral, 1:Read from memory
        , TEIE        = 1 << 3   // 0x0008 Transfer error interrupt enable
        , HTIE        = 1 << 2   // 0x0004 Half transfer interrupt enable
        , TCIE        = 1 << 1   // 0x0002 Transfer complete interrupt enable
        , EN          = 1        // 0x0001 Channel enable
    };

    //---------------------------------------
    template< DMA_CHANNEL >
    struct peripheral_address {
        static constexpr uint32_t value = 0;
        static constexpr uint32_t dma_ccr = 0;
    };

    template<> struct peripheral_address< DMA_ADC1 > {
        static constexpr uint32_t value = ADC1_BASE;
        static constexpr uint32_t dma_ccr = DMA_ReadFromPeripheral;
    };

    template<> struct peripheral_address< DMA_I2C1_RX > {
        static constexpr uint32_t value = I2C1_BASE + 0x10;
        static constexpr uint32_t dma_ccr = PL_VeryHigh | MINC;  // memory inc enable, 8bit, 8bit, dir = 'from peripheral'
    };
    template<> struct peripheral_address< DMA_I2C1_TX > {
        static constexpr uint32_t value = I2C1_BASE + 0x10;
        static constexpr uint32_t dma_ccr = PL_High | DMA_ReadFromMemory | MINC;
    };
    
    template<> struct peripheral_address< DMA_I2C2_RX > {
        static constexpr uint32_t value = I2C2_BASE + 0x10;
        static constexpr uint32_t dma_ccr = PL_VeryHigh | MINC;  // memory inc enable, 8bit, 8bit, dir = 'from peripheral'
    };
    template<> struct peripheral_address< DMA_I2C2_TX > {
        static constexpr uint32_t value = I2C2_BASE + 0x10;
        static constexpr uint32_t dma_ccr = PL_High | DMA_ReadFromMemory | MINC;
    };

    //---------------------------------------
    template< DMA_CHANNEL >
    struct dma_buffer_size {
        static constexpr size_t value = 0;
    };
    
    template<> struct dma_buffer_size< DMA_ADC1 > { static constexpr size_t value = 32; };
    template<> struct dma_buffer_size< DMA_I2C1_RX > { static constexpr size_t value = 32; };
    template<> struct dma_buffer_size< DMA_I2C1_TX > { static constexpr size_t value = 32; };
    template<> struct dma_buffer_size< DMA_I2C2_RX > { static constexpr size_t value = 32; };
    template<> struct dma_buffer_size< DMA_I2C2_TX > { static constexpr size_t value = 32; };    

    template< size_t size >
    struct alignas( 4 ) dma_buffer {
        uint8_t data [ size ];
    };

    template< DMA_CHANNEL channel >
    class dma_channel_t {
        dma& dma_;
    public:
        dma_channel_t( dma& dma ) : dma_( dma ) {
            dma.init_channel( channel, peripheral_address, buffer.data, sizeof(buffer.data), dma_ccr );
        }

        inline void enable( bool enable ) {
            dma_.enable( channel, enable );
        }

        inline void set_transfer_buffer( const uint8_t * buffer, size_t size ) {
            dma_.set_transfer_buffer( channel, buffer, size );
        }

        inline void set_receive_buffer( uint8_t * buffer, size_t size ) {
            dma_.set_receive_buffer( channel, buffer, size );
        }        

        inline bool transfer_complete() const {
            return dma_.transfer_complete( channel );
        }
        
        inline void transfer_complate_clear()  {
            return dma_.transfer_complete_clear( channel );
        }        
        
        static constexpr uint32_t dma_ccr = peripheral_address< channel >::dma_ccr;
        static constexpr uint32_t peripheral_address = peripheral_address< channel >::value;
        dma_buffer< dma_buffer_size< channel >::value > buffer;
    };


}

