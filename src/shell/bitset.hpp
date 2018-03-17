// Copyright (C) 2018 MS-Cheminformatics LLC
// Licence: CC BY-NC
// Author: Toshinobu Hondo, Ph.D.
// Contact: toshi.hondo@qtplatz.com
//

#pragma once

namespace stm32f103 {

    struct bitset {
        template< typename T, typename V > inline static void set( T& t, const V& mask ) {  t |= mask;  }
        template< typename T, typename V > inline static void reset( T& t, const V& mask ) {  t &= ~mask;  }
        template< typename T, typename V > inline static const bool test( T& t, const V& mask ) {  return (t & mask) == mask; }
    };
    
}

