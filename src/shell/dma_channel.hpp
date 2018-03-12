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
#include <utility>

namespace stm32f103 {

    // DMA1
    // 0 Channel1 := ADC1
    // 1 Channel2 := SPI1_RX | USART3_TX
    // 2 Channel3 := SPI1_TX | USART3_RX
    // 3 Channel4 := SPI2_RX | USART1_TX | I2C2_TX
    // 4 Channel5 := SPI2_TX | USART1_RX | I2C2_RX
    // 5 Channel6 := USART2_RX | I2C1_TX
    // 6 Channel7 := USART2_TX | I2C1_RX

    enum DMA_CHANNEL : uint32_t {
        DMA_ADC1 = 0
        , DMA_SPI1_RX = 1
        , DMA_SPI1_TX = 2
        //, DMA_SPI2_RX = 4
        //, DMA_SPI2_TX = 5
        , DMA_I2C2_TX = 3
        , DMA_I2C2_RX = 4        
        , DMA_I2C1_TX = 5
        , DMA_I2C1_RX = 6
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
        static constexpr uint32_t value = ADC1_BASE + offsetof( ADC, DR );
        static constexpr uint32_t dma_ccr = PL_High | DMA_ReadFromPeripheral | MINC | (1 << 10) | (1 << 8) | CIRC; // 16bit,16bit
    };

    template<> struct peripheral_address< DMA_I2C1_RX > {
        static constexpr uint32_t value = I2C1_BASE + 0x10;
        static constexpr uint32_t dma_ccr = PL_VeryHigh | DMA_ReadFromPeripheral | MINC;  // memory inc enable, 8bit, 8bit, dir = 'from peripheral'
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

    template< size_t size, typename T >
    struct alignas( sizeof(T) ) dma_buffer {
        T data [ size ];
    };

    template< typename dma_channel_type >
    class scoped_dma_channel_enable {
        dma_channel_type& _;
    public:
        scoped_dma_channel_enable( dma_channel_type& t ) : _( t ) {
            _.enable( true );
        }
        ~scoped_dma_channel_enable() {
            _.enable( false );
        }
    };

    template<>
    class scoped_dma_channel_enable< dma > {
        dma& dma_;
        uint32_t channel_;
    public:
        scoped_dma_channel_enable( dma& dma, uint32_t channel ) : dma_( dma ), channel_( channel ) {
            dma_.enable( channel_, true );
        }
        ~scoped_dma_channel_enable() {
            dma_.enable( channel_, false );
        }
    };

    template< DMA_CHANNEL channel >
    class dma_channel_t {
        dma& dma_;
    public:
        dma_channel_t( dma& dma, uint8_t * data, uint16_t size ) : dma_( dma ) {
            dma.init_channel( channel, peripheral_address, data, size, dma_ccr );
        }

        inline void enable( bool enable ) {
            dma_.enable( channel, enable );
        }

        template< typename buffer_type >
        inline void set_transfer_buffer( const buffer_type * buffer, size_t size ) {
            dma_.set_transfer_buffer( channel, buffer, size );
        }

        template< typename buffer_type >
        inline void set_receive_buffer( buffer_type * buffer, size_t size ) {
            dma_.set_receive_buffer( channel, buffer, size );
        }        

        inline bool transfer_complete() const {
            return dma_.transfer_complete( channel );
        }

        inline void set_callback( void(*callback)( uint32_t ) ) {
            dma_.set_callback( channel, callback );
        }
        
        inline void clear_callback() {
            dma_.clear_callback( channel );
        }
        
        static constexpr uint32_t dma_ccr = peripheral_address< channel >::dma_ccr;
        static constexpr uint32_t peripheral_address = peripheral_address< channel >::value;
        static constexpr uint32_t channel_number = channel;
    };


}

