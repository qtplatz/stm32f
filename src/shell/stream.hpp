// Copyright (C) 2018 MS-Cheminformatics LLC
// Licence: CC BY-NC
// Author: Toshinobu Hondo, Ph.D.
// Contact: toshi.hondo@qtplatz.com
//

#pragma once

#include <cstdint>

class stream;

namespace std {
    constexpr const char * endl = "\n";
}

namespace stm32f103 {
    class uart;
}

extern stm32f103::uart __uart0;    

class stream {
    stm32f103::uart& uart_;
public:
    stream( stm32f103::uart& );
    stream() : uart_( __uart0 ) {}
    void flush();

    stream& operator << ( const char );    
    stream& operator << ( const char * );
    stream& operator << ( const int8_t );
    stream& operator << ( const uint8_t );
    stream& operator << ( const int16_t );
    stream& operator << ( const uint16_t );
    stream& operator << ( const int32_t );
    stream& operator << ( const uint32_t );
    stream& operator << ( const int64_t );
    stream& operator << ( const uint64_t );
};

