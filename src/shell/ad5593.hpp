/**************************************************************************
** Copyright (C) 2016-2017 Toshinobu Hondo, Ph.D.
** Copyright (C) 2016-2017 MS-Cheminformatics LLC, Toin, Mie Japan
*
** Contact: toshi.hondo@qtplatz.com
** 
** Reference: http://www.analog.com/media/en/technical-documentation/data-sheets/AD5593R.pdf
**           AD5593R: 8-Channel, 12-Bit, Configurable ADC/DAC with On-Chip Reference, I2C Interface Data Sheet
**           Rev.C (Last Content Update: 05/03/2017)
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

namespace ad5593 {

    enum AD5593R_IO_FUNCTION {
        ADC                    // 0
        , DAC                  // 1
        , GPIO                 // 2
        , UNUSED_HIGH          // 3
        , UNUSED_LOW           // 4
        , UNUSED_TRISTATE      // 5
        , UNUSED_PULLDOWN      // 6
        , nFunctions
    };
    
    enum AD5593_GPIO_DIRECTION {
        GPIO_INPUT
        , GPIO_OUTPUT
    };

    constexpr size_t number_of_pins = 8;

    class AD5593 {
        std::array< AD5593R_IO_FUNCTION, number_of_pins > functions_;
        std::array< std::bitset< number_of_pins >, 7 > bitmaps_;
#if defined __linux        
        std::unique_ptr< i2c_linux::i2c > i2c_;
#else
        stm32f103::i2c * i2c_;
#endif
        bool dirty_;
        const int address_;
    public:
#if defined __linux
        AD5593( const char * device = "/dev/i2c-2", int address = 0x10 );
#else
        AD5593( stm32f103::i2c& t, int address = 0x10 );
#endif
        ~AD5593();

        const std::system_error& error_code() const;

        operator bool () const;

        inline bool is_dirty() const { return dirty_; }

        bool write( uint8_t addr, uint16_t value ) const;
        uint16_t read( uint8_t addr ) const;

        bool set_value( int pin, uint16_t value );

        uint16_t value( int pin ) const;
        
        bool set_function( int pin, AD5593R_IO_FUNCTION );
        AD5593R_IO_FUNCTION function( int pin ) const;
        
        bool fetch();
        bool commit();

        void print_config() const;
    };
    
}
