// Copyright (C) 2018 MS-Cheminformatics LLC
// Licence: CC BY-NC
// Author: Toshinobu Hondo, Ph.D.
// Contact: toshi.hondo@qtplatz.com
//

#include <cstdint>
#include <cstddef>

namespace stm32f103 {

    enum USART_BASE : uint32_t;
    struct USART;
    
    class uart {
        volatile USART * usart_;
        uint32_t baud_;

        uart( const uart& ) = delete;
        uart& operator = ( const uart& ) = delete;
    public:
        enum parity { parity_even, parity_odd, parity_none };
        
        uart();
    
        bool init( USART_BASE addr, parity = parity_even, int nbits = 8, uint32_t baud = 115200, uint32_t pclk = 72000000 );

        uart& operator << ( const char * );

        void putc( int );

        void handle_interrupt();
        static void interrupt_handler( uart * );

        // printf & console interface
        static int getc();
        static size_t gets( char * p, size_t size );
    };
    
}

