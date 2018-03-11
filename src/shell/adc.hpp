// Copyright (C) 2018 MS-Cheminformatics LLC
// Licence: CC BY-NC
// Author: Toshinobu Hondo, Ph.D.
// Contact: toshi.hondo@qtplatz.com
//

#include <atomic>
#include <cstdint>
#include <cstddef>

namespace stm32f103 {

    struct ADC;
    enum PERIPHERAL_BASE : uint32_t;

    class dma;

    class adc {
        adc( const adc& ) = delete;
        adc& operator = ( const adc& ) = delete;
        volatile ADC * adc_;
        std::atomic_flag lock_;
        std::atomic_bool flag_;
        std::atomic< uint16_t > data_;

    public:
        adc();
        ~adc();
        void init( PERIPHERAL_BASE );
        void attach( dma& );
        operator bool () const { return adc_; }

        bool start_conversion(); // software trigger

        uint32_t cr2() const;
        
        uint16_t data();

        void handle_interrupt();
        static void interrupt_handler( adc * _this );
    };
    
}

