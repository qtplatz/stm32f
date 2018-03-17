// Copyright (C) 2018 MS-Cheminformatics LLC
// Licence: CC BY-NC
// Author: Toshinobu Hondo, Ph.D.
// Contact: toshi.hondo@qtplatz.com
//

#include <cstdint>
#include <cstddef>
#include <algorithm>

template<typename stream_type, typename array_type>
const char * debug_print( stream_type&& o, const array_type& a, size_t count, const char * preamble, const char * postamble = "\n" ) {
    o << preamble;
    std::for_each( a.begin(), a.begin() + count, [&](auto x){ o << x << ","; } );
    o << postamble;
    return "";
}

