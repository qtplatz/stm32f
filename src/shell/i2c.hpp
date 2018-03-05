// Copyright (C) 2018 MS-Cheminformatics LLC
// Licence: CC BY-NC
// Author: Toshinobu Hondo, Ph.D.
// Contact: toshi.hondo@qtplatz.com
//


#include <atomic>
#include <cstdint>

namespace stm32f103 {

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
        std::atomic< uint32_t > rxd_;

        i2c( const i2c& ) = delete;
        i2c& operator = ( const i2c& ) = delete;

    public:
        i2c();
        
        void init( I2C_BASE );
        inline operator bool () const { return i2c_; };
        
        i2c& operator << ( uint16_t );
        
        void handle_event_interrupt();
        void handle_error_interrupt();
        static void interrupt_event_handler( i2c * );
        static void interrupt_error_handler( i2c * );
    };

}

