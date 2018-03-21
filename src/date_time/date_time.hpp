// Copyright (C) 2018 MS-Cheminformatics LLC
// Licence: CC BY-NC
// Author: Toshinobu Hondo, Ph.D.
// Contact: toshi.hondo@qtplatz.com

#pragma once

#include <cstddef>
#include <cstdint>
#include <cctype>
#include <algorithm>
#if __arm__
# define __TM_GMTOFF tm_gmtoff // arm-none-eabi-g++ has not been declard tm_gmtoff by default
#endif
#include <ctime>

struct scanner {
    template< typename T >
    bool read( T& d, const char *&s, size_t size ) const {
        d = T(0);
        while ( size-- && *s && std::isdigit( *s ) )
            d = d * 10 + *s++ - '0';
        return true;
    };

    template< char skip_char1, char ... skip_chars >
    void skip( const char *& s, size_t size = 1 ) const {
        const char chars [] = { skip_char1, ( skip_chars )... };
        while ( size-- && *s ) {
            auto it = std::find_if( chars, chars + sizeof(chars)
                                    , [&]( const char& a ) { return *s == a; } );
            if ( it == chars + sizeof( chars ) )
                return;
            ++s;
        }
    }
};

class date_time {
    static bool parse_time( const char * s, int& hour, int& min, int& second, int& tzoffs );    

    static std::pair< bool, bool >
    parse_date( const char * s
                , int& year, int& month, int& day
                , int& hour, int& min, int& second, int& tzoffs );

public:
    enum parse_state { date_time_none = 0
                       , date_time_date = 1
                       , date_time_time = 2
                       , date_time_both = date_time_date | date_time_time };

    static parse_state parse( const char * s, tm& );

    static std::time_t time( const tm& );

    static const char * to_string( char *, size_t, const tm&, bool gmtoffs = false );
    static const char * to_string( char *, size_t, const std::time_t&, bool gmtoffs = false );

    static std::tm * gmtime( const std::time_t& timer, std::tm& );
};
