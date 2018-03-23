// Copyright (C) 2018 MS-Cheminformatics LLC
// Licence: CC BY-NC
// Author: Toshinobu Hondo, Ph.D.
// Contact: toshi.hondo@qtplatz.com
//

#pragma once

#include <array>
#include <atomic>
#include <cstdint>

namespace stm32f103 {

    class dma;

    /* Section 26.6.10 I^2C register map, p785 RM0008
     */
    struct I2C {
        uint32_t CR1;     // I^2C  control register 1,
        uint32_t CR2;     // I^2C  control register 2, 0x04
        uint32_t OAR1;    // I^2C  own address register 1, 0x08
        uint32_t OAR2;    // I^2C  own address register 2, 0x0c
        uint32_t DR;      // I^2C  data register           0x10
        uint32_t SR1;     // I^2C  status register         0x14
        uint32_t SR2;     // I^2C  status register         0x18
        uint32_t CCR;     // I^2C  status register         0x1c
        uint32_t TRISE;   // I^2C  status register         0x20
    };

    // I^2C 26.5, p773 RM0008
    
    enum I2C_BASE : uint32_t;
    
    struct I2C;

    class i2c {
        volatile I2C * i2c_;
        std::atomic_flag lock_;
        uint8_t own_addr_;

        i2c( const i2c& ) = delete;
        i2c& operator = ( const i2c& ) = delete;

        uint32_t recvp_;
        std::array< uint8_t, 32 > recv_;

    public:
        i2c();

        enum DMA_Direction { DMA_None, DMA_Tx, DMA_Rx, DMA_Both };
        
        void init( I2C_BASE );

        void attach( dma&, DMA_Direction );

        void reset();
        
        inline operator bool () const { return i2c_; };

        // bool write( uint8_t address, uint8_t data );
        bool write( uint8_t address, const uint8_t * data, size_t );
        // bool read( uint8_t address, uint8_t& data );
        bool read( uint8_t address, uint8_t * data, size_t );

        bool dma_transfer( uint8_t address, const uint8_t *, size_t );
        bool dma_receive( uint8_t address, uint8_t * data, size_t );
        uint32_t status() const;
        void print_status() const;
        bool start();
        bool stop();
        bool disable();
        bool enable();

        bool dmaEnable( bool );
        bool has_dma( DMA_Direction ) const;

        void set_own_addr( uint8_t );
        uint8_t own_addr() const;
        
        void handle_event_interrupt();
        void handle_error_interrupt();
        static void interrupt_event_handler( i2c * );
        static void interrupt_error_handler( i2c * );
    };

    template< I2C_BASE base > struct i2c_t {
        static std::atomic_flag once_flag_;
        static i2c * instance();
        // static inline i2c * instance() {
        //     static i2c __instance;
        //     if ( !once_flag_.test_and_set() )
        //         __instance.init( base );            
        //     return &__instance;
        // }
    };
    
    template< I2C_BASE base > std::atomic_flag i2c_t< base >::once_flag_;
    
}

