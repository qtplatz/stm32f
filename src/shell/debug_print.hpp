// Copyright (C) 2018 MS-Cheminformatics LLC
// Licence: CC BY-NC
// Author: Toshinobu Hondo, Ph.D.
// Contact: toshi.hondo@qtplatz.com
//

#include <cstdint>
#include <cstddef>
#include <algorithm>
#include <bitset>

class stream;

template<typename stream_type, typename array_type>
const char * array_print( stream_type&& o, const array_type& a, size_t count, const char * preamble, const char * postamble = "\n" ) {
    o << preamble;
    std::for_each( a.begin(), a.begin() + count, [&](auto x){ o << x << ","; } );
    o << postamble;
    return "";
}

struct print {

    template< size_t N, typename stream_type >
    stream_type& operator()( stream_type&& o, const std::bitset<N>& a, const char * preamble, const std::bitset<N>& mask = std::bitset<N>{ 0 } ) const {
        o << preamble << "\t";
        for ( int i = 0; i < N; ++i ) {
            if ( mask.test( N - i - 1 ) )
                o << '-';
            else
                o << a.test( N - i - 1 );
            if ( (N-1 != i) && ( N - i - 1 ) % 8 == 0 )
                o << '\'';
        }
        return o;
    }

    template< typename array_type, typename stream_type >
    stream_type& operator()( stream_type&& o, const array_type& a, size_t count, const char * preamble ) const {
        o << preamble;
        std::for_each( a.begin(), a.begin() + count, [&](auto x){ o << x << ","; } );
        return o;
    }
    
};
