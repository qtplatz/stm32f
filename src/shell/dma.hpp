// Copyright (C) 2018 MS-Cheminformatics LLC
// Licence: CC BY-NC
// Author: Toshinobu Hondo, Ph.D.
// Contact: toshi.hondo@qtplatz.com
//

#pragma once

#include <array>
#include <atomic>
#include <cstdint>
#include <cstddef>

// Section 13, p273 Introduction

namespace stm32f103 {

    // 13.4.7, p288, DMA register map
    struct DMAChannel {
        uint32_t CCR;    // 0x08, 0x01c
        uint32_t CNDTR;  // 0x0c, 0x020
        uint32_t CPAR;   // 0x10, 0x024
        uint32_t CMAR;   // 0x14, 0x028
        uint32_t resv;   // 0x18, 0x02c
    };

    struct DMA {
        uint32_t ISR;    // 0x00
        uint32_t IFCR;   // 0x04
        DMAChannel channels[7];
    };

    enum DMA_BASE : uint32_t;
    enum DMA_CHANNEL : uint32_t;
    enum DMA_DIR : uint32_t;

    class dma {
        volatile DMA * dma_;

        std::atomic_flag lock_;
        std::atomic< uint32_t  > interrupt_status_;
        std::array< void(*)( uint32_t ), 7 > callbacks_;

        dma( const dma& ) = delete;
        dma& operator = ( const dma& ) = delete;
        
    public:
        dma();
        
        void init( DMA_BASE );

        volatile DMAChannel& dmaChannel( uint32_t );

        bool init_channel( DMA_CHANNEL channel_number
                           , uint32_t peripheral_base_addr
                           , uint8_t * buffer_addr
                           , uint32_t buffer_size
                           , uint32_t dma_ccr );
        
        inline operator bool () const { return dma_; };

        void enable( uint32_t channel, bool );

        void set_transfer_buffer( uint32_t channel, const uint8_t * buffer, size_t size );
        void set_receive_buffer( uint32_t channel, uint8_t * buffer, size_t size );

        template< typename buffer_type >
        void set_transfer_buffer( uint32_t channel, const buffer_type * buffer, size_t size ) {
            set_transfer_buffer( channel, reinterpret_cast< const uint8_t * >( buffer ), size );
        }

        template< typename buffer_type >
        void set_receive_buffer( uint32_t channel, buffer_type * buffer, size_t size ) {
            set_transfer_buffer( channel, reinterpret_cast< uint8_t * >( buffer ), size );
        }

        bool transfer_complete( uint32_t channel );

        void set_callback( uint32_t channel, void(*callback)( uint32_t ) ) {
            callbacks_.at( channel ) = callback;
        }

        void clear_callback( uint32_t channel );
        
        void handle_interrupt( uint32_t );
    };

}

