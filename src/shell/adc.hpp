// Copyright (C) 2018 MS-Cheminformatics LLC
// Licence: CC BY-NC
// Author: Toshinobu Hondo, Ph.D.
// Contact: toshi.hondo@qtplatz.com
//

#include <cstdint>
#include <cstddef>

namespace stm32f103 {

    class adc {

        adc( const adc& ) = delete;
        adc& operator = ( const adc& ) = delete;
    public:
        adc();
        ~adc();
        void init();

        static adc * instance();
        
        static void inq_handler( adc * _this );
    };
    
}

