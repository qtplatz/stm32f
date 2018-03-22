// Copyright (C) 2018 MS-Cheminformatics LLC
// Licence: CC BY-NC
// Author: Toshinobu Hondo, Ph.D.
// Contact: toshi.hondo@qtplatz.com
//

#include "bitset.hpp"
#include "condition_wait.hpp"
#include "debug_print.hpp"
#include "rcc.hpp"
#include "bkp.hpp"
#include "stream.hpp"
#include "stm32f103.hpp"
#include <bitset>
#include <cstdint>

using namespace stm32f103;

bkp::bkp()
{
}

void
bkp::set_data( size_t idx, uint16_t&& data )
{
    if ( auto PWR = reinterpret_cast< volatile stm32f103::PWR * >( stm32f103::PWR_BASE ) )
        stm32f103::bitset::set( PWR->CR, 0x100 ); // DBP = 1, Access to RTC and Backup registers enabled
    
    auto * reg = reinterpret_cast< volatile BKP * >( stm32f103::BKP_BASE );
    if ( idx < sizeof( reg->DR ) / sizeof( reg->DR[0] ) )
        reg->DR[ idx ] = data;
    idx -= sizeof( reg->DR ) / sizeof( reg->DR[0] );
    if ( idx < sizeof( reg->DR11 ) / sizeof( reg->DR11[0] ) )
        reg->DR11[ idx ] = data;
}

uint16_t
bkp::data( size_t idx )
{
    if ( auto PWR = reinterpret_cast< volatile stm32f103::PWR * >( stm32f103::PWR_BASE ) )
        stm32f103::bitset::set( PWR->CR, 0x100 ); // DBP = 1, Access to RTC and Backup registers enabled
    
    auto * reg = reinterpret_cast< volatile BKP * >( stm32f103::BKP_BASE );
    if ( idx < sizeof( reg->DR ) / sizeof( reg->DR[0] ) )
        return reg->DR[ idx ];
    idx -= sizeof( reg->DR ) / sizeof( reg->DR[0] );
    if ( idx < sizeof( reg->DR11 ) / sizeof( reg->DR11[0] ) )
        return reg->DR11[ idx ];
}

void
bkp::print_registers()
{
    if ( auto * reg = reinterpret_cast< volatile BKP * >( stm32f103::BKP_BASE ) ) {
        size_t n(0);
        for ( const auto& dr: reg->DR ) {
            stream() << "DR#" << int(n++) << "\t" << dr << "\t";
            if ( n % 4 == 0 )
                stream() << std::endl;
        }
        for ( const auto& dr: reg->DR11 ) {
            stream() << "DR#" << int(n++) << "\t" << dr << "\t";
            if ( n % 4 == 0 )
                stream() << std::endl;            
        }
        stream() << "\nSee section 6.4, p82- on RM0008\n";
        stream() << "RTCCR" << "\t" << reg->RTCCR << std::endl;
        stream() << "CR" << "\t" << reg->CR << std::endl;
        stream() << "CSR" << "\t" << reg->CSR << std::endl;
    }
}

