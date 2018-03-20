//

#pragma once

#include <cstddef>
#include <cctype>
#include <algorithm>

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

bool parse_time( const char * s, int& hour, int& min, int& second, int& tzoffs );

std::pair< bool, bool > parse_date( const char * s
                                    , int& year, int& month, int& day
                                    , int& hour, int& min, int& second, int& tzoffs );
