/**************************************************************************
** Copyright (C) 2016-2017 Toshinobu Hondo, Ph.D.
** Copyright (C) 2016-2017 MS-Cheminformatics LLC, Toin, Mie Japan
*
** Contact: toshi.hondo@qtplatz.com
** 
** Reference: BMA150 data sheet Rev. 1.4, BST-BMP280-DS001-19.pdf
**
**************************************************************************/

#pragma once

#include <array>
#include <bitset>
#include <string>
#include <system_error>
#include <memory>

#if defined __linux
namespace i2c_linux { class i2c; }
#else
namespace stm32f103 { class i2c; }
#endif

namespace bmp280 {

    enum REGISTERS {
        temp_xlsb    = 0xfc  // read only data 
        , temp_lsb   = 0xfb  // read only data 
        , temp_msb   = 0xfa  // read only data
        , press_xlsb = 0xf9  // read only data
        , press_lsb  = 0xf8  // read only data
        , press_msb  = 0xf7  // read only data
        , config     = 0xf5  // t_sb[7:5], filter[4:2], spi3w_en[0]
        , ctrl_meas  = 0xf4  // osrs_t[7:5], osrs_p[4:2], mode[1:0]
        , status     = 0xf3  // read only
        , reset      = 0xe0  // write only
        , id         = 0xd0  // chip id [7:0], read only
    };
    // calib25..calib00 = 0xa1 .. 0x88 // read only

    enum BMP280_MODE {  // 0xf4
        SLEEP_MODE = 0x00
        , FORCED_MODE = 0x01
        , NORMAL_MODE = 0x03
    };

    class BMP280 {
#if defined __linux        
        std::unique_ptr< i2c_linux::i2c > i2c_;
#else
        stm32f103::i2c * i2c_;
#endif
        bool dirty_;
        const int address_;
    public:
#if defined __linux
        BMP280( const char * device = "/dev/i2c-2", int address = 0x10 );
#else
        BMP280( stm32f103::i2c& t, int address = 0x76 );  // or 0x77
#endif
        ~BMP280();

        const std::system_error& error_code() const;

        operator bool () const;

        bool write( const uint8_t *, size_t size ) const;
        bool read( uint8_t addr, uint8_t *, size_t size ) const;

        template< size_t N >  bool write ( std::array< uint8_t, N >&& a ) const {
            return write( a.data(), a.size() );
        }
    };
    
}
