// Copyright (C) 2018 MS-Cheminformatics LLC
// Licence: CC BY-NC
// Author: Toshinobu Hondo, Ph.D.
// Contact: toshi.hondo@qtplatz.com
//

#include <cstdint>
#include <ctime>

namespace stm32f103 {
    
    enum BKP_BASE : uint32_t; // PERIPHERAL_BASE
    
    struct BKP {
        uint32_t RESV;
        uint32_t DR[10];   // 0x04 - 0x28
        uint32_t RTCCR;    // 0x2c
        uint32_t CR;       // 0x30
        uint32_t CSR;      // 0x34
        uint32_t RESV2;    // 0x38
        uint32_t RESV3;    // 0x3C
        uint32_t DR11[32]; // 0x40 -- 0xbc
    };
    
    enum BKP_RTCCR_MASK {
        ASOS = ( 1 << 9 )
        , ASOE = ( 1 << 8 )
        , CCO  = ( 1 << 7 )
        , CAL  = 0x7f
    };
    
    class bkp {
        bkp( const bkp& ) = delete;
        bkp& operator = ( const bkp& ) = delete;
        bkp();
        static void init();        
    public:
        static void print_registers();
    };

}
