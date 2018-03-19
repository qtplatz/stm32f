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

    enum BMP280_OSRS_P {
        PRESSURE_MEASUREMENT_SKIPPED
        , ULTRA_LOW_POWER
        , LOW_POWER
        , STANDARD_RESOLUTION
        , HIGH_RESOLUTION
        , ULTRA_HIGH_RESOLUTION
    };

    class BMP280 {
#if defined __linux        
        std::unique_ptr< i2c_linux::i2c > i2c_;
#else
        stm32f103::i2c * i2c_;
#endif
        const int address_;
        bool has_callback_;
        bool has_trimming_parameter_;
        uint16_t dig_T1;
        int16_t dig_T2, dig_T3;
        uint16_t dig_P1;
        int16_t dig_P2, dig_P3, dig_P4, dig_P5, dig_P6, dig_P7, dig_P8, dig_P9;
        
        BMP280( const BMP280& ) = delete;
        BMP280& operator = ( const BMP280& ) = delete;
        static BMP280 * __instance;
        BMP280( stm32f103::i2c& t, int address = 0x76 );  // or 0x77
    public:

        ~BMP280();
        static BMP280 * instance( stm32f103::i2c& t, int address = 0x76 );  // or 0x77
        static BMP280 * instance();

        operator bool () const;

        bool write( const uint8_t *, size_t size ) const;
        bool read( uint8_t addr, uint8_t *, size_t size ) const;

        template< size_t N > bool write ( std::array< uint8_t, N >&& a ) const { return write( a.data(), a.size() ); }
        
        void trimming_parameter_readout();
        void measure();
        void stop();
        std::pair< uint32_t, uint32_t> readout();
        
        inline bool is_active() const { return has_callback_; }
    private:
        uint32_t compensate_P32( uint32_t adc_P, int32_t t_fine ) const;
        uint32_t compensate_P64( uint32_t adc_P, int32_t t_fine ) const;
        int32_t compensate_T( int32_t adc_T, int32_t& t_fine ) const;
        static void handle_timer();
    };
    
}
