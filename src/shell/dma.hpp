// Copyright (C) 2018 MS-Cheminformatics LLC
// Licence: CC BY-NC
// Author: Toshinobu Hondo, Ph.D.
// Contact: toshi.hondo@qtplatz.com
//

#pragma once

#include <atomic>
#include <cstdint>
#include <cstddef>

typedef void (*irq_handler_type)();

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

        dma( const dma& ) = delete;
        dma& operator = ( const dma& ) = delete;

    public:
        dma();
        
        void init( DMA_BASE );

        volatile DMAChannel& dmaChannel( uint32_t );

        static bool init_channel( dma& dma
                                  , DMA_CHANNEL channel_number
                                  , uint32_t peripheral_base_addr
                                  , uint8_t * buffer_addr
                                  , uint32_t buffer_size
                                  , uint32_t dma_ccr );

        inline operator bool () const { return dma_; };

        void setEnabled( uint32_t, bool );
        
        void handle_interrupt();
        static void interrupt_handler( dma * );
    };

}

