// Copyright (C) 2018 MS-Cheminformatics LLC
// Licence: CC BY-NC
// Author: Toshinobu Hondo, Ph.D.
// Contact: toshi.hondo@qtplatz.com
//

#include "i2cdebug.hpp"
#include "stream.hpp"

namespace stm32f103 {
    namespace i2cdebug {

        constexpr const char * __CR1__ [] = {
            "SWRST",        nullptr, "ALERT", "PEC",  "POS",      "ACK",   "STOP", "START"
            , "NO_STRETCH", "ENGC", "ENPEC", "ENARP", "SMB_TYPE", nullptr, "SMBUS", "PE"
        };
        
        constexpr const char * __CR2__ [] = {
        nullptr,   nullptr, nullptr, "LAST", "DMAEN", "ITBUFEN", "ITEVTEN", "ITERREN"
        , nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,  nullptr,   nullptr
        };
        
        constexpr const char * __SR1__ [] = {
            "SMB_ALERT", "TIMEOUT", nullptr, "PEC_ERR", "OVR",   "AF",  "ARLO", "BUS_ERR"
            , "TxE",     "RxNE",    nullptr, "STOPF",   "ADD10", "BTF", "ADDR", "SB"
        };
        
        constexpr const char * __SR2__ [] = {
            "DUALF", "SMB_HOST", "SMBDE_FAULT", "GEN_CALL", nullptr, "TRA", "BUSY", "MSL"
        };
        
        const char * CR1_to_string( uint32_t reg ) {
            stream() << "CR1 {";
            for ( int i = 0; i < 16; ++i ) {
                int k = 15 - i;
                if ( __CR1__[i] && ( reg & (1 << k) ) )
                    stream() << __CR1__[ i ] << ", ";
            }
            stream()  << "}";
            return "";
        }

        const char * CR2_to_string( uint32_t reg ) {
            stream() << "CR2 {";
            for ( int i = 3; i <= 7; ++i ) {
                int k = 15 - i;
                if ( __CR2__[ i ] && ( reg & (1 << k) ) )
                    stream() << __CR2__[ i ] << ", ";
            }
            stream() << "FREQ=" << int( reg & 0x3f ) << "}";
            return "";
        }

        const char * SR1_to_string( uint32_t reg ) {
            stream() << "SR1 {";
            for ( int i = 0; i < 16; ++i ) {
                int k = 15 - i;
                if ( __SR1__[i] && ( reg & (1 << k) ) )
                    stream() << __SR1__[ i ] << ", ";
            }
            stream() << "}";
            return "";
        }

        const char * SR2_to_string( uint32_t reg ) {
            stream() << "SR2 {";
            if ( reg & 0xf0 )
                stream() << "PEC=" << int( ( reg >> 8 ) & 0xff ) << ", ";
            for ( int i = 0; i <= 7; ++i ) {
                int k = 7 - i;
                if ( __SR2__[ i ] && ( reg & (1 << k) ) )
                    stream() << __SR2__[ i ] << ", ";
            }
            stream() << "}";
            return "";
        }

        const char * status32_to_string( uint32_t reg ) {
            SR2_to_string( reg >> 16 );
            stream() << " ";
            SR1_to_string( reg & 0xffff );
            return "";
        }
    }
}
