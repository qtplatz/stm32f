/**************************************************************************
** Copyright (C) 2016-2017 Toshinobu Hondo, Ph.D.
** Copyright (C) 2016-2017 MS-Cheminformatics LLC, Toin, Mie Japan
*
** Contact: toshi.hondo@qtplatz.com
** 
** This code was modified from url: https://github.com/analogdevicesinc/AD5593-AuxIO-Win10-IoT
**
**************************************************************************/
/********************************************************************************
* Copyright 2017(c)Analog Devices, Inc.
*
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met :
*-Redistributions of source code must retain the above copyright
*    notice, this list of conditions and the following disclaimer.
*  -Redistributions in binary form must reproduce the above copyright
*    notice, this list of conditions and the following disclaimer in
*    the documentation and / or other materials provided with the
*    distribution.
*  -Neither the name of Analog Devices, Inc.nor the names of its
*    contributors may be used to endorse or promote products derived
*    from this software without specific prior written permission.
*  -The use of this software may or may not infringe the patent rights
*    of one or more patent holders.This license does not release you
*    from the requirement that you obtain separate licenses from these
*    patent holders to use this software.
*  -Use of the software either in source or binary form, must be run
*    on or directly connected to an Analog Devices Inc.component.
*
* THIS SOFTWARE IS PROVIDED BY ANALOG DEVICES "AS IS" AND ANY EXPRESS OR
* IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, NON - INFRINGEMENT,
* MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
* IN NO EVENT SHALL ANALOG DEVICES BE LIABLE FOR ANY DIRECT, INDIRECT,
* INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES(INCLUDING, BUT NOT
* LIMITED TO, INTELLECTUAL PROPERTY RIGHTS, PROCUREMENT OF SUBSTITUTE GOODS OR
* SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
* CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
* OR TORT(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*******************************************************************************/

#pragma once

#include <array>
#include <string>
#include <system_error>

namespace stm32f103 {
    class i2c;
}

namespace ad5593 {

    enum ioFunction : uint8_t {
        ADC
            , DAC
            , DAC_AND_ADC
            , GPIO
            , UNUSED_HIGH
            , UNUSED_LOW
            , UNUSED_TRISTATE
            , UNUSED_PULLDOWN
            };

    enum gpioDirection : uint8_t {
        GPIO_INPUT
            , GPIO_OUTPUT
            };

    enum Register : uint8_t {
        AD5593R_REG_NOOP = 0x0
            , AD5593R_REG_DAC_READBACK = 0x1
            , AD5593R_REG_ADC_SEQ      = 0x2
            , AD5593R_REG_CTRL         = 0x3
            , AD5593R_REG_ADC_EN       = 0x4
            , AD5593R_REG_DAC_EN       = 0x5
            , AD5593R_REG_PULLDOWN     = 0x6
            , AD5593R_REG_LDAC         = 0x7
            , AD5593R_REG_GPIO_OUT_EN  = 0x8
            , AD5593R_REG_GPIO_SET     = 0x9
            , AD5593R_REG_GPIO_IN_EN   = 0xa
            , AD5593R_REG_PD           = 0xb
            , AD5593R_REG_OPEN_DRAIN   = 0xc
            , AD5593R_REG_TRISTATE     = 0xd
            , AD5593R_REG_RESET        = 0xf
            };
    
    enum AD5593Mode : uint8_t  {
        AD5593R_MODE_CONF                = (0 << 4)
            , AD5593R_MODE_DAC_WRITE     = (1 << 4)
            , AD5593R_MODE_ADC_READBACK  = (4 << 4)
            , AD5593R_MODE_DAC_READBACK  = (5 << 4)
            , AD5593R_MODE_GPIO_READBACK = (6 << 4)
            , AD5593R_MODE_REG_READBACK  = (7 << 4)
            };

    class ad5593dev;
    
    class io {
        // friend class ad5593dev;
    
        uint16_t gpio_out = 0;
        uint16_t gpio_in = 0;
        uint16_t gpio_set = 0;
        uint16_t adc_en = 0;
        uint16_t dac_en = 0;
        uint16_t tristate = 0;
        uint16_t pulldown = 0xff;
    
        uint8_t ident_;
        ad5593dev * parent_;
        ioFunction func_;
        gpioDirection dir_;

        template<typename T = bool> T isBitSet( uint16_t mask, uint8_t pos ) const;

        void setBit(uint16_t &mask) const;
        void clearBit(uint16_t &mask) const;
        void setOrClear(uint16_t &mask, bool set) const;

    public:

        io();
        io( const io& );
        io( ad5593dev& parent, uint8_t ident, ioFunction func );
        
        void function( ioFunction func);
        ioFunction function();
        void direction( gpioDirection dir );
        void direction( gpioDirection dir, uint16_t value );
        gpioDirection direction();

        bool set( uint16_t value );
        uint16_t get();
    };

    template<> bool io::isBitSet( uint16_t mask, uint8_t pos ) const;
    template<> uint16_t io::isBitSet( uint16_t mask, uint8_t pos ) const;


    //////////////////////////////////////////////
    class ad5593dev {
        friend class io;
        std::array< io, 8 > ios_;
        uint8_t address_; // 0x10 | 0x11
        stm32f103::i2c * i2c_;
        bool use_dma_;
    public:
        ad5593dev( stm32f103::i2c * i2c, uint8_t address, bool use_dma = false ) : address_( address )
                                                                                 , i2c_( i2c )
                                                                                 , use_dma_( use_dma ) {
        }
        bool write( uint8_t addr, uint16_t value );
        uint16_t read( uint8_t addr );
        void reset();
        void reference_internal();
        void reference_range_high();
        const io& operator []( uint8_t pin ) const;
    };
}
