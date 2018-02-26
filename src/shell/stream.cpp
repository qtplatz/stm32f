// Copyright (C) 2018 MS-Cheminformatics LLC
// Licence: CC BY-NC
// Author: Toshinobu Hondo, Ph.D.
// Contact: toshi.hondo@qtplatz.com
//

#include "stream.hpp"
#include "uart.hpp"
#include <type_traits>
#include <typeinfo>

using namespace stm32f103;

class stream_t {
public:
    template<typename T>
    void operator()( uart& o_, T d ) const {
        const static char * __chars__ = "01234567890abcdef";
        char buf[ 22 ];
        char * p = &buf[21];
        *p-- = '\0';
        int base = 16;
        
        if ( std::is_signed< T >::value ) {
            base = 10;
            if ( d < 0 ) {
                o_.putc( '-' );
                d = -d;
            }
        }

        do  {
            *p-- = __chars__[ d % base ];
            d /= base;
        } while ( d );

        o_ << ++p;
    }
};


stream::stream( uart& t ) : uart_( t )
{
}

stream&
stream::operator << ( const char * s )
{
    uart_ << s;
    return *this;
}

stream&
stream::operator << ( const int8_t d )
{
    stream_t()( uart_, d );
    return *this;
}

stream&
stream::operator << ( const uint8_t d )
{
    stream_t()( uart_, d );
    return *this;
}

stream&
stream::operator << ( const int16_t d )
{
    stream_t()( uart_, d );
    return *this;
}

stream&
stream::operator << ( const uint16_t d )
{
    stream_t()( uart_, d );
    return *this;    
}

stream&
stream::operator << ( const int32_t d )
{
    stream_t()( uart_, d );
    return *this;        
}

stream&
stream::operator << ( const uint32_t d )
{
    stream_t()( uart_, d );
    return *this;            
}

// require lldivm
stream&
stream::operator << ( const int64_t d )
{
    stream_t()( uart_, static_cast< const uint32_t > (d) );
    stream_t()( uart_, static_cast< const uint32_t > (d >> 32) );
    return *this;
}

stream&
stream::operator << ( const uint64_t d )
{
    stream_t()( uart_, static_cast< const uint32_t > (d) );
    stream_t()( uart_, static_cast< const uint32_t > (d >> 32) );    
    return *this;
}

void
stream::flush()
{
    // nothing to do
}
