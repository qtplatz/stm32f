// Copyright (C) 2018 MS-Cheminformatics LLC
// Licence: CC BY-NC
// Author: Toshinobu Hondo, Ph.D.
// Contact: toshi.hondo@qtplatz.com
//

#pragma once

#include <array>
#include <cstddef>

template< size_t argv_size >
class tokenizer {
    tokenizer( const tokenizer& ) = delete;
    tokenizer& operator = ( const tokenizer& ) = delete;
    
public:
    typedef std::array< const char *, argv_size > argv_type;
    
    tokenizer() {}

    size_t operator()( char * buffer, argv_type& argv ) const {

        std::fill( argv.begin(), argv.end(), static_cast< const char * >(0) );

        if ( buffer == nullptr )
            return 0;

        size_t argc = 0;
        char * p = buffer;
        
        while ( argc < argv_size ) {
            while ( *p && ( *p == ' ' || *p == '\t' ) )
                ++p;
            if ( *p == '\n' )
                break;
            argv[ argc++ ] = p;
            while ( *p && ( *p != ' ' && *p != '\t' && *p != '\n' ) )
                ++p;
            if ( *p )
                *p++ = '\0'; // split buffer
            if ( *p == '\0' )
                break;
        }
        return argc;
    }
};
