// Copyright (C) 2018 MS-Cheminformatics LLC
// Licence: CC BY-NC
// Author: Toshinobu Hondo, Ph.D.
// Contact: toshi.hondo@qtplatz.com
//

#pragma once

#include <array>
#include <atomic>
#include <cstdint>

class stream;

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

    enum I2C_RESULT_CODE {
        I2C_RESULT_SUCCESS
        , I2C_BUS_BUSY
        , I2C_DMA_MASTER_RECEIVER_HAS_NO_DMA
        , I2C_DMA_MASTER_RECEIVER_START_FAILED
        , I2C_DMA_MASTER_RECEIVER_ADDRESS_FAILED
        , I2C_DMA_MASTER_RECEIVER_RECV_TIMEOUT
        , I2C_DMA_MASTER_TRANSMITTER_HAS_NO_DMA
        , I2C_DMA_MASTER_TRANSMITTER_START_FAILED
        , I2C_DMA_MASTER_TRANSMITTER_ADDRESS_FAILED
        , I2C_DMA_MASTER_TRANSMITTER_SEND_TIMEOUT
        , I2C_POLLING_MASTER_RECEIVER_START_FAILED
        , I2C_POLLING_MASTER_RECEIVER_ADDRESS_FAILED
        , I2C_POLLING_MASTER_RECEIVER_RECV_TIMEOUT
        , I2C_POLLING_MASTER_TRANSMITTER_START_FAILED
        , I2C_POLLING_MASTER_TRANSMITTER_ADDRESS_FAILED
        , I2C_POLLING_MASTER_TRANSMITTER_SEND_TIMEOUT
        , I2C_DEVICE_ERROR_CONDITION
    };

    // I^2C 26.5, p773 RM0008
    
    enum I2C_BASE : uint32_t;
    
    struct I2C;

    class i2c {
        volatile I2C * i2c_;
        std::atomic_flag lock_;
        uint8_t own_addr_;
        I2C_RESULT_CODE result_code_;

        i2c( const i2c& ) = delete;
        i2c& operator = ( const i2c& ) = delete;
        
    public:
        i2c();
        inline volatile I2C * base_addr() const { return i2c_; };

        enum DMA_Direction { DMA_None, DMA_Tx, DMA_Rx, DMA_Both };
        
        void init( I2C_BASE );

        void attach( dma&, DMA_Direction );

        void reset();

        bool listen( uint8_t own_addr );
        
        inline operator bool () const { return i2c_; };

        // bool write( uint8_t address, uint8_t data );
        bool write( uint8_t address, const uint8_t * data, size_t );
        // bool read( uint8_t address, uint8_t& data );
        bool read( uint8_t address, uint8_t * data, size_t );

        bool dma_transfer( uint8_t address, const uint8_t *, size_t );
        bool dma_receive( uint8_t address, uint8_t * data, size_t );
        uint32_t status() const;
        bool start();
        bool stop();
        bool disable();
        bool enable();

        bool dmaEnable( bool );
        bool has_dma( DMA_Direction ) const;

        I2C_RESULT_CODE result_code() const;
        stream& print_result( stream&& ) const;
        stream& print_status( stream&& ) const;

        void handle_event_interrupt();
        void handle_error_interrupt();
    };

    template< I2C_BASE base > struct i2c_t {
        static std::atomic_flag once_flag_;
        static inline i2c * instance() {
            static i2c __instance;
            if ( !once_flag_.test_and_set() )
                __instance.init( base );            
            return &__instance;
        }
    };
    
    template< I2C_BASE base > std::atomic_flag i2c_t< base >::once_flag_;
    
}

