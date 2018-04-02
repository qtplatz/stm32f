// Copyright (C) 2018 MS-Cheminformatics LLC
// Licence: CC BY-NC
// Author: Toshinobu Hondo, Ph.D.
// Contact: toshi.hondo@qtplatz.com
//

#include <array>
#include <atomic>
#include <cstdint>
#include "scoped_spinlock.hpp"

namespace stm32f103 {

    enum TIM_BASE : uint32_t;
    template< TIM_BASE > struct timer_t;
    
    class timer {
        timer( const timer& ) = delete;
        timer& operator = ( const timer& ) = delete;
    public:
        timer();
        void handle_interrupt();
    private:
        template< TIM_BASE > friend struct timer_t;
        static void init( TIM_BASE );
        static void enable( TIM_BASE, bool );
        static void set_interval( TIM_BASE, size_t ); // 1Hz default
        static void print_registers( TIM_BASE );
    };

    //////////////////

    template< TIM_BASE base > class timer_t {
        static std::atomic_flag flag_initialized_;
        static std::atomic_flag guard_;
        static void(*callback_)();
    public:
        timer_t() {
            if ( !flag_initialized_.test_and_set() )
                timer::init( base );
        }
        
        operator bool () const { return flag_initialized_; }

        static void print_registers() { timer::print_registers( base ); }

        inline void enable( bool enable ) const { timer::enable( base, enable ); };

        inline void set_interval( size_t arr ) const { timer::set_interval( base, arr ); };

        void set_callback( void (*cb)() ) { // required ctor
            scoped_spinlock<> guard( guard_ );
            callback_ = cb;
        }

        static void clear_callback( bool allow_recursive_call = false ) {
            if ( allow_recursive_call ) {
                callback_ = nullptr;
            } else {
                scoped_spinlock<> guard( guard_ );            
                callback_ = nullptr;
            }
        }
        
        static bool callback() {
            scoped_spinlock<> guard( guard_ );
            if ( stm32f103::timer_t< base >::callback_ ) {
                stm32f103::timer_t< base >::callback_();
                return true;
            }
            return false;
        }
    };

    template< TIM_BASE base > std::atomic_flag timer_t<base>::flag_initialized_;
    template< TIM_BASE base > std::atomic_flag timer_t<base>::guard_;
    template< TIM_BASE base > void (*timer_t<base>::callback_)();
}

