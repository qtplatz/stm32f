// Copyright (C) 2018 MS-Cheminformatics LLC
// Licence: CC BY-NC
// Author: Toshinobu Hondo, Ph.D.
// Contact: toshi.hondo@qtplatz.com

#pragma once

#include <cstddef>
#include <cctype>
#include <algorithm>
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
};
