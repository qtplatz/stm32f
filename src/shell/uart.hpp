// Copyright (C) 2018 MS-Cheminformatics LLC
// Licence: CC BY-NC
// Author: Toshinobu Hondo, Ph.D.
// Contact: toshi.hondo@qtplatz.com
//

#include <atomic>
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
        uart();
    public:
        enum parity { parity_even, parity_odd, parity_none };

        template< typename GPIO_PIN_type >
        bool enable( GPIO_PIN_type tx, GPIO_PIN_type rx
                     , parity = parity_none, int nbits = 8, uint32_t baud = 115200, uint32_t pclk = 72000000 );

        bool config( parity = parity_even, int nbits = 8, uint32_t baud = 115200, uint32_t pclk = 72000000 );

        uart& operator << ( const char * );

        void putc( int );

        void handle_interrupt();

        // printf & console interface
        static int getc();
        static size_t gets( char * p, size_t size );
    private:
        bool init( USART_BASE addr );
        template< USART_BASE > friend struct uart_t;
    };

    template< USART_BASE base > struct uart_t {
        static std::atomic_flag once_flag_;

        static inline uart * instance() {
            static uart __instance;
            if ( !once_flag_.test_and_set() )
                __instance.init( base );
            return &__instance;
        }
    };
    template< USART_BASE base > std::atomic_flag uart_t< base >::once_flag_;

}
