// Copyright (C) 2018 MS-Cheminformatics LLC
// Licence: CC BY-NC
// Author: Toshinobu Hondo, Ph.D.
// Contact: toshi.hondo@qtplatz.com
//
#include "debug_print.hpp"
#include "rcc.hpp"
#include "stream.hpp"
#include "stm32f103.hpp"
#include "utility.hpp"
#include <algorithm>
#include <bitset>
#include <cctype>

static const char * __apbenr__ [] = {
    "DMA1",     "DMA2",  "SRAM",  nullptr,  "FLITF", nullptr,  "CRCEN",  nullptr
    ,  nullptr, nullptr, nullptr, nullptr,  "OTGFS",  nullptr, "ETHMAC", "ETHMAC_TX"
    , "ETHMAC_RX"
};

static const char * __apb2enr__ [] = {
    "AFIO",    nullptr, "IOPA",  "IOPB",  "IOPC",  "IOPD",  "IOPE",  "IOPF"
    , "IOPG",  "ADC1",  "ADC2",  "TIM1",  "SPI1",  "TIM8",  "USART1","ADC3"
    , nullptr, nullptr, nullptr, "TIM9",  "TIM10", "TIM11"
};

static const char * __apb1enr__ [] = {
    "TIM2",     "TIM3",  "TIM4", "TIM5",  "TIM6",  "TIM7",  "TIM12", "TIM13"
    , "TIM14", nullptr, nullptr, "WWDG",  nullptr, nullptr, "SPI2",  "SPI3"
    , nullptr,"USART2","USART3", "USART4","USART5","I2C1",  "I2C2",  "USB"
    , nullptr,   "CAN", nullptr, "BPK",   "PWR",    "DAC"
};

void
rcc_status( size_t argc, const char ** argv )
{
    if ( auto RCC = reinterpret_cast< volatile stm32f103::RCC * >( stm32f103::RCC_BASE ) ) {
        stream() << "APB2, APB1 peripheral clock enable register (p112-116, RM0008, Rev 17) " << std::endl;
        stream() << "\tRCC->APB2ENR : " << RCC->APB2ENR << std::endl;
        stream() << "\tRCC->APB1ENR : " << RCC->APB1ENR << std::endl;

        {
            stream() << "\tEnables : ";
            int i = 0;
            for ( auto& p: __apbenr__ ) {
                if ( p && ( RCC->APB2ENR & (1<<i) ) )
                    stream() << p << ", ";                    
                ++i;
            }
            stream() << "\n";
        }
        
        {
            stream() << "\tEnables : ";
            int i = 0;
            for ( auto& p: __apb2enr__ ) {
                if ( p && ( RCC->APB2ENR & (1<<i) ) )
                    stream() << p << ", ";                    
                ++i;
            }
            stream() << "\n";
        }
        {
            stream() << "\tEnables : ";
            int i = 0;
            for ( auto& p: __apb1enr__ ) {
                if ( p && ( RCC->APB1ENR & (1<<i) ) )
                    stream() << p << ", ";
                ++i;
            }
        }
        stream() << std::endl;

        if ( RCC->APB1ENR & stm32f103::RCC_APB1ENR_PWREN ) {
            std::bitset< 17 > bits( RCC->BDCR );
            print()( stream(), bits, "RCC_BDCR") << "\t\t" << RCC->BDCR;
            stream() << " BDRST("  << bits.test( 16 ) << "),";
            stream() << "RTCEN("  << bits.test( 15 ) << "),";
            stream() << "RTCSEL("  << bits.test( 9 ) << bits.test( 8 ) << "),";
            stream() << "LSEBYP(" << bits.test( 2 ) << "),";
            stream() << "LSERDY(" << bits.test( 1 ) << "),";
            stream() << "LSEON("  << bits.test( 0 ) << ")";
            stream() << std::endl;
        }
        {
            std::bitset< 32 > bits( RCC->CSR );
            print()( stream(), bits, "RCC_CSR") << "\t" << RCC->CSR << " ";
            stream() << "LSIRDY("  << bits.test( 1 ) << "),";
            stream() << "LSION("  << bits.test( 0 ) << "),";
            stream() << std::endl;
        }
        {
            std::bitset< 24 > bits( RCC->CIR );
            print()( stream(), bits, "RCC_CIR") << "\t\t" << RCC->CIR << " ";
            stream() << "PLL RDY("  << bits.test( 4 ) << "),";
            stream() << "HSE RDY("  << bits.test( 3 ) << "),";
            stream() << "HSI RDY("  << bits.test( 2 ) << "),";
            stream() << "LSE RDY("  << bits.test( 1 ) << "),";
            stream() << "LSI RDY("  << bits.test( 0 ) << "),";
            stream() << std::endl;
        }
    }
}

void
rcc_enable( size_t argc, const char ** argv )
{
    const char * myname = argv[ 0 ];
    
    if ( argc == 1 ) {
        int n = 0;
        for ( auto& p: __apb2enr__ ) {
            if ( p ) {
                if ( ( ++n % 8 ) == 0 )
                    stream() << std::endl;
                stream() << p << " | ";
            }
        }
        stream() << std::endl;
        n = 0;
        for ( auto& p: __apb1enr__ ) {
            if ( p ) {
                if ( ( ++n % 8 ) == 0 )
                    stream() << std::endl;                
                stream() << p << " | ";
            }
        }
        return;
    }
    --argc;
    ++argv;
    uint32_t flags1( 0 ), flags2( 0 );
    while ( argc-- ) {
        stream() << "looking for : " << argv[ 0 ] << std::endl;
        uint32_t i = 0;
        for ( auto& p: __apb2enr__ ) {
            if ( strcmp( p, argv[ 0 ] ) == 0 ) {
                flags2 |= (1 << i);
                stream() << "\tfound on APB2ENR: " << flags2 << std::endl;
            }
            ++i;
        }
        i = 0;
        for ( auto& p: __apb1enr__ ) {
            if ( strcmp( p, argv[ 0 ] ) == 0 ) {
                flags1 |= (1 << i);
                stream() << "\tfound on APB1ENR: " << flags1 << std::endl;
            }
            ++i;
        }
        ++argv;
    }
    
    if ( auto RCC = reinterpret_cast< volatile stm32f103::RCC * >( stm32f103::RCC_BASE ) ) {
        auto prev1 = RCC->APB1ENR;
        auto prev2 = RCC->APB2ENR;
        stream() << myname << " : " << flags2 << ", " << flags1 << std::endl;
        if ( strcmp( myname, "enable" ) == 0 ) {
            if ( flags2 ) {
                RCC->APB2ENR |= flags2;
                stream() << "APB2NER: " << prev2 << " | " << flags2 << "->" << RCC->APB2ENR << std::endl;
            }
            if ( flags1 ) {
                RCC->APB1ENR |= flags1;
                stream() << "APB1NER: " << prev1 << " | " << flags2 << "->" << RCC->APB1ENR << std::endl;
            }
        } else {
            if ( flags2 ) {
                RCC->APB2ENR &= ~flags2;
                stream() << "APB2NER: " << prev2 << " & " << ~flags2 << "->" << RCC->APB2ENR << std::endl;
            }
            if ( flags1 ) {
                RCC->APB1ENR &= ~flags1;
                stream() << "APB1NER: " << prev1 << " & " << ~flags1 << "->" << RCC->APB1ENR << std::endl;
            }
        }
    }

    
}
