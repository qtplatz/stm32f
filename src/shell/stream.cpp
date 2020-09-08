// Copyright (C) 2018 MS-Cheminformatics LLC
// Licence: CC BY-NC
// Author: Toshinobu Hondo, Ph.D.
// Contact: toshi.hondo@qtplatz.com
//

#include "stream.hpp"
#include "stm32f103.hpp"
#include "uart.hpp"
#include <atomic>
#include <cmath>
#include <type_traits>
#include <typeinfo>

using namespace stm32f103;

class stream_t {
    constexpr const static char * __chars__ = "0123456789abcdef";
public:
    template<typename T>
    void operator()( uart& o_, T d ) const {
        if ( std::is_signed< T >::value ) {
            // signed int -- output in decimal numbers
            char buf[ 22 ]; // 64bit signed max = 20 chars + sign and '\0'
            char * p = &buf[21];
            *p-- = '\0';
            
            if ( d < 0 ) {
                o_.putc( '-' );
                d = -d;
            }
            do  {
                *p-- = __chars__[ d % 10 ];
                d /= 10;
            } while ( d );
            o_ << ++p;            

        } else {
            // unsigned values always output in hex
            for ( size_t i = 0; i < sizeof(T) * 2; ++i )
                o_.putc( __chars__[ ( d >> (( sizeof(T) * 2 - 1 - i )*4) ) & 0x0f ] );
        }
    }
};

template<> void stream_t::operator()( uart& o, double d ) const {
    o << "stream_t::operator()(uart&, double) " << std::endl;
}


stream::stream() : uart_( *stm32f103::uart_t< stm32f103::USART1_BASE >::instance() )
{
}

stream::stream( uart& t ) : uart_( t )
{
}

stream::stream( const char * file, const int line, const char * function ) : uart_( *uart_t< USART1_BASE >::instance() )
{
    (*this) << file << " " << line << ": ";
    if ( function )
        (*this) << function << "\t";
}

stream&
stream::operator << ( const bool c )
{
    uart_.putc( c ? '1' : '0' );
    return *this;    
}

stream&
stream::operator << ( const char c )
{
    uart_.putc( c );
    return *this;    
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
    stream_t()( uart_, uint64_t( d ) );
    return *this;
}

stream&
stream::operator << ( const uint64_t d )
{
    stream_t()( uart_, d );
    return *this;
}

stream&
stream::operator << ( const double d )
{
    stream_t()( uart_, d );
    return *this;
}

#if __GNUC__ >= 7
stream&
stream::operator << ( const int d )
{
    stream_t()( uart_, static_cast< const int32_t > (d) );
    return *this;
}

stream&
stream::operator << ( const size_t d )
{
    stream_t()( uart_, static_cast< const uint32_t > (d) );
    return *this;
}
#endif

void
stream::flush()
{
    // nothing to do
}
