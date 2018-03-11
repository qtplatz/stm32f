// Copyright (C) 2018 MS-Cheminformatics LLC
// Licence: CC BY-NC
// Author: Toshinobu Hondo, Ph.D.
// Contact: toshi.hondo@qtplatz.com
//

#include "command_processor.hpp"
#include "ad5593.hpp"
#include "adc.hpp"
#include "dma.hpp"
#include "dma_channel.hpp"
#include "gpio.hpp"
#include "gpio_mode.hpp"
#include "printf.h"
#include "i2c.hpp"
#include "spi.hpp"
#include "stream.hpp"
#include "stm32f103.hpp"
#include <atomic>
#include <algorithm>
#include <cctype>
#include <functional>

extern stm32f103::i2c __i2c0, __i2c1;
extern stm32f103::spi __spi0, __spi1;
extern stm32f103::adc __adc0;
extern stm32f103::dma __dma0;
extern std::atomic< uint32_t > atomic_jiffies;
extern void mdelay( uint32_t ms );

void i2c_test( size_t argc, const char ** argv );

int
strcmp( const char * a, const char * b )
{
    while ( *a && (*a == *b ) )
        a++, b++;
    return *reinterpret_cast< const unsigned char *>(a) - *reinterpret_cast< const unsigned char *>(b);
}

size_t
strlen( const char * s )
{
    const char * p = s;
    while ( *p )
        ++p;
    return size_t( p - s );
}

int
strtod( const char * s )
{
    int sign = 1;
    int num = 0;
    while ( *s ) {
        if ( *s == '-' )
            sign = -1;
        else if ( '0' <= *s && *s <= '9' )
            num += (num * 10) + ((*s) - '0');
        ++s;
    }
    return num * sign;
}

void
i2cdetect( size_t argc, const char ** argv )
{
    int id = 0;
    if ( argc > 2 ) {
        if ( *argv[1] == '1' )
            id = 1;
    }
    
    stm32f103::I2C_BASE addr = ( id == 0 ) ? stm32f103::I2C1_BASE : stm32f103::I2C2_BASE;    
    auto& i2cx = ( addr == stm32f103::I2C1_BASE ) ? __i2c0 : __i2c1;

    if ( !i2cx ) {
        const char * argv [] = { id == 0 ? "i2c" : "i2c2", nullptr };
        i2c_test( 1, argv );
    }

    if ( i2cx ) {

        stream() << "\t 0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f" << std::endl;
        stream() << "00:\t         ";

        for ( uint8_t addr = 3; addr <= 0x77; ++addr ) {

            if ( ( addr % 16 ) == 0 )
                stream() << std::endl << addr << ":\t";
            
            uint8_t data;
            if ( i2cx.read( addr, &data, 1 ) ) {
                stream() << addr << " ";
            } else {
                stream() << "-- ";
            }
        }
    } else {
        stream() << "i2c not initalized" << std::endl;
    }
}

void
i2c_test( size_t argc, const char ** argv )
{
    int id = ( strcmp( argv[0], "i2c") == 0 ) ? 0 : 1;
    stm32f103::I2C_BASE addr = ( id == 0 ) ? stm32f103::I2C1_BASE : stm32f103::I2C2_BASE;

    auto& i2cx = ( addr == stm32f103::I2C1_BASE ) ? __i2c0 : __i2c1;

    bool use_dma( false );
    auto it = std::find_if( argv, argv + argc, [](auto a){ return strcmp( a, "dma" ) == 0; } );
    use_dma = it != ( argv + argc );

    using namespace stm32f103;

    // (see RM0008, p180, Table 55)
    // I2C ALT function  REMAP=0 { SCL,SDA } = { PB6, PB7 }, REMAP=1 { PB8, PB9 }
    // GPIO config in p167, Table 27
    if ( use_dma || ( id == 0 ) ) {
        if ( ! __i2c0 ) {
            gpio_mode()( stm32f103::PB6, stm32f103::GPIO_CNF_ALT_OUTPUT_ODRAIN,  stm32f103::GPIO_MODE_OUTPUT_2M ); // SCL
            gpio_mode()( stm32f103::PB7, stm32f103::GPIO_CNF_ALT_OUTPUT_ODRAIN,  stm32f103::GPIO_MODE_OUTPUT_2M ); // SDA
            __i2c0.init( stm32f103::I2C1_BASE );
        }
    }
    if ( use_dma || ( id == 1 ) ) {
        if ( ! __i2c1 ) {
            gpio_mode()( stm32f103::PB10, stm32f103::GPIO_CNF_ALT_OUTPUT_ODRAIN, stm32f103::GPIO_MODE_OUTPUT_2M ); // SCL
            gpio_mode()( stm32f103::PB11, stm32f103::GPIO_CNF_ALT_OUTPUT_ODRAIN, stm32f103::GPIO_MODE_OUTPUT_2M ); // SDA
            __i2c1.init( stm32f103::I2C2_BASE );
        }
    }
    if ( use_dma ) {
        __i2c0.attach( __dma0, i2c::DMA_Both );
        __i2c1.attach( __dma0, i2c::DMA_Both );
    }
    
    uint8_t i2caddr = 0x10; // DA5593R

    while ( --argc ) {
        ++argv;
        if ( strcmp( argv[0], "status" ) == 0 ) {
            i2cx.print_status();
        } else if ( strcmp( argv[0], "reset" ) == 0 ) {
            i2cx.reset();
        } else if ( strcmp( argv[0], "addr" ) == 0 ) {
            if ( argc ) {
                --argc;
                ++argv;
                i2caddr = strtod( argv[ 0 ] );
            }
        } else if ( strcmp( argv[0], "read" ) == 0 ) {
            uint8_t data;
            i2cx.enable();
            if ( i2cx.read( i2caddr, &data, 1 ) ) {
                stream() << "got data: " << data << std::endl;
            }
        } else if ( strcmp( argv[0], "write" ) == 0 ) {
            i2cx.enable();
            //if ( i2cx.write( i2caddr, 'A' ) ) {
            uint8_t data = 'a';
            if ( i2cx.write( i2caddr, &data, 1 ) ) {
                stream() << "i2c " << data << " sent out." << std::endl;
            }
        } else if ( strcmp( argv[ 0 ], "dma" ) == 0 ) {
            //const int8_t i2caddr = 0x04;
            const uint8_t * rp;
            constexpr static const uint8_t txd [] = { 0x00, 0x01 };
            if ( __i2c0.dma_transfer(i2caddr, txd, 2 ) ) {
                stream() << "--------------- dma transfer ----------------" << std::endl;
                static std::array< uint8_t, 8 > rxdata;
                if ( __i2c0.dma_receive(i2caddr, rxdata.data(), 2 ) ) {
                    stream() << "--------------- dma recv ----------------" << std::endl;
                    stream() << rxdata[0] << ", " << rxdata[1] << std::endl;
                }
            }
        }
    }
}

void
spi_test( size_t argc, const char ** argv )
{
    auto id = ( strcmp( argv[0], "spi") == 0 ) ? 0 : ( strcmp( argv[0], "spi2" ) == 0 ) ? 1 : (-1);
    auto& spix = ( id == 0 ) ? __spi0 : __spi1;

    // spi num
    size_t count = 1024;
    bool spi_read( false );
    bool spi_ss_soft = false;

    while ( --argc ) {
        ++argv;
        if ( std::isdigit( *argv[ 0 ] ) ) {
            count = strtod( argv[ 0 ] );
            if ( count == 0 )
                count = 1;
        } else if ( *argv[0] == 's' ) {
            spi_ss_soft = true;
        } else if ( *argv[0] == 'r' ) {
            spi_read = true;
        }
    }

    if ( ! spix ) {
        using namespace stm32f103;

        if ( id == 1 ) {
            // SPI 2
            // (see RM0008, p166, Table 25)
            gpio_mode()( stm32f103::PB12, stm32f103::GPIO_CNF_OUTPUT_PUSH_PULL,     stm32f103::GPIO_MODE_OUTPUT_50M ); // ~SS
            
            gpio_mode()( stm32f103::PB13, stm32f103::GPIO_CNF_ALT_OUTPUT_PUSH_PULL, stm32f103::GPIO_MODE_OUTPUT_50M ); // SCLK
            gpio_mode()( stm32f103::PB14, stm32f103::GPIO_CNF_INPUT_FLOATING,       stm32f103::GPIO_MODE_INPUT );      // MISO
            gpio_mode()( stm32f103::PB15, stm32f103::GPIO_CNF_ALT_OUTPUT_PUSH_PULL, stm32f103::GPIO_MODE_OUTPUT_50M ); // MOSI

            if ( spi_ss_soft )
                spix.init( stm32f103::SPI2_BASE, 'B', 12 );
            else
                spix.init( stm32f103::SPI2_BASE );
            
        } else {
            // SPI 1
            gpio_mode()( stm32f103::PA4, stm32f103::GPIO_CNF_ALT_OUTPUT_PUSH_PULL, stm32f103::GPIO_MODE_OUTPUT_50M ); // ~SS
            gpio_mode()( stm32f103::PA5, stm32f103::GPIO_CNF_ALT_OUTPUT_PUSH_PULL, stm32f103::GPIO_MODE_OUTPUT_50M ); // SCLK
            gpio_mode()( stm32f103::PA6, stm32f103::GPIO_CNF_INPUT_FLOATING,       stm32f103::GPIO_MODE_INPUT );      // MISO
            gpio_mode()( stm32f103::PA7, stm32f103::GPIO_CNF_ALT_OUTPUT_PUSH_PULL, stm32f103::GPIO_MODE_OUTPUT_50M ); // MOSI

            if ( spi_ss_soft )
                __spi0.init( stm32f103::SPI1_BASE, 'A', 4 );
            else
                __spi0.init( stm32f103::SPI1_BASE );

            gpio_mode()( stm32f103::PB12, stm32f103::GPIO_CNF_INPUT_FLOATING,       stm32f103::GPIO_MODE_INPUT ); // ~SS
            gpio_mode()( stm32f103::PB13, stm32f103::GPIO_CNF_INPUT_FLOATING,       stm32f103::GPIO_MODE_INPUT ); // SCLK
            gpio_mode()( stm32f103::PB14, stm32f103::GPIO_CNF_ALT_OUTPUT_PUSH_PULL, stm32f103::GPIO_MODE_OUTPUT_50M ); // MISO
            gpio_mode()( stm32f103::PB15, stm32f103::GPIO_CNF_INPUT_FLOATING,       stm32f103::GPIO_MODE_INPUT ); // MOSI

            __spi1.slave_init( stm32f103::SPI2_BASE );
        }
    }
    
    if ( spi_read ) {
        uint16_t rxd;
        while ( count-- ) {
            spix >> rxd;
            stream() << "spi read: " << rxd << std::endl;
            mdelay( 10 );
        }
    } else {
        while ( count-- ) {
            uint32_t d = atomic_jiffies.load();
            stream() << "spi write: " << ( d & 0xffff ) << std::endl;
            spix << uint16_t( d & 0xffff );
            mdelay( 10 );
        }
    }
}

void
alt_test( size_t argc, const char ** argv )
{
    // alt spi [remap]
    using namespace stm32f103;    
    auto AFIO = reinterpret_cast< volatile stm32f103::AFIO * >( stm32f103::AFIO_BASE );
    if ( argc > 1 && strcmp( argv[1], "spi" ) == 0 ) {
        if ( argc == 2 ) {
            gpio_mode()( stm32f103::PA4, GPIO_CNF_ALT_OUTPUT_PUSH_PULL, GPIO_MODE_OUTPUT_50M ); // ~SS
            gpio_mode()( stm32f103::PA5, GPIO_CNF_ALT_OUTPUT_PUSH_PULL, GPIO_MODE_OUTPUT_50M ); // SCLK
            gpio_mode()( stm32f103::PA6, GPIO_CNF_INPUT_FLOATING,       GPIO_MODE_INPUT );      // MISO
            gpio_mode()( stm32f103::PA7, GPIO_CNF_ALT_OUTPUT_PUSH_PULL, GPIO_MODE_OUTPUT_50M ); // MOSI
            AFIO->MAPR &= ~1;            // clear SPI1 remap
        }
        if ( argc > 2 && strcmp( argv[2], "remap" ) == 0 ) {
            gpio_mode()( stm32f103::PA15, GPIO_CNF_ALT_OUTPUT_PUSH_PULL, GPIO_MODE_OUTPUT_50M ); // NSS
            gpio_mode()( stm32f103::PB3,  GPIO_CNF_ALT_OUTPUT_PUSH_PULL, GPIO_MODE_OUTPUT_50M ); // SCLK
            gpio_mode()( stm32f103::PB4,  GPIO_CNF_INPUT_FLOATING,       GPIO_MODE_INPUT );      // MISO
            gpio_mode()( stm32f103::PB5,  GPIO_CNF_ALT_OUTPUT_PUSH_PULL, GPIO_MODE_OUTPUT_50M ); // MOSI
            AFIO->MAPR |= 1;          // set SPI1 remap
        }
        stream() << "\tEnable SPI, AFIO MAPR: 0x" << AFIO->MAPR << std::endl;
    } else {
        stream()
            << "\tError: insufficent arguments\nalt spi [remap]"
            << "\tAFIO MAPR: 0x" << AFIO->MAPR
            << std::endl;
    }
}

void
gpio_test( size_t argc, const char ** argv )
{
    using namespace stm32f103;

    const size_t replicates = 0x7fffff;
    
    if ( argc >= 2 ) {
        const char * pin = argv[1];
        int no = 0;
        if (( pin[0] == 'P' && ( 'A' <= pin[1] && pin[1] <= 'C' ) ) && ( pin[2] >= '0' && pin[2] <= '9' ) ) {
            no = pin[2] - '0';
            if ( pin[3] >= '0' && pin[3] <= '9' )
                no = no * 10 + pin[3] - '0';

            stream() << "Pulse out to P" << pin[1] << no << std::endl;

            switch( pin[1] ) {
            case 'A':
                gpio_mode()( static_cast< GPIOA_PIN >(no), GPIO_CNF_OUTPUT_PUSH_PULL, GPIO_MODE_OUTPUT_50M );
                stream() << pin << ": " << gpio_mode::toString( gpio_mode()( static_cast< GPIOA_PIN >(no) ) ) << std::endl;
                for ( size_t i = 0; i < replicates; ++i )
                    gpio< GPIOA_PIN >( static_cast< GPIOA_PIN >( no ) ) = bool( i & 01 );
                break;
            case 'B':
                gpio_mode()( static_cast< GPIOB_PIN >(no), GPIO_CNF_OUTPUT_PUSH_PULL, GPIO_MODE_OUTPUT_50M );
                stream() << pin << ": " << gpio_mode::toString( gpio_mode()( static_cast< GPIOB_PIN >(no) ) ) << std::endl;
                for ( size_t i = 0; i < replicates; ++i )
                    gpio< GPIOB_PIN >( static_cast< GPIOB_PIN >( no ) ) = bool( i & 01 );
                break;                
            case 'C':
                gpio_mode()( static_cast< GPIOC_PIN >(no), GPIO_CNF_OUTPUT_PUSH_PULL, GPIO_MODE_OUTPUT_50M );
                stream() << pin << ": " << gpio_mode::toString( gpio_mode()( static_cast< GPIOC_PIN >(no) ) ) << std::endl;
                for ( size_t i = 0; i < replicates; ++i )
                    gpio< GPIOC_PIN >( static_cast< GPIOC_PIN >( no ) ) = bool( i & 01 );
                break;                                
            }
            
        } else {
            stream() << "gpio 2nd argment format mismatch" << std::endl;
        }
    } else {
        for ( int i = 0; i < 16;  ++i )
            stream() << "PA" << i << ":\t" << gpio_mode::toString( gpio_mode()( static_cast< GPIOA_PIN >(i) ) ) << std::endl;
        for ( int i = 0; i < 16;  ++i )
            stream() << "PB" << i << ":\t" << gpio_mode::toString( gpio_mode()( static_cast< GPIOB_PIN >(i) ) ) << std::endl;
        for ( int i = 13; i < 16;  ++i )
            stream() << "PC" << i << ":\t" << gpio_mode::toString( gpio_mode()( static_cast< GPIOC_PIN >(i) ) ) << std::endl;
        
        stream() << "gpio <pin#>" << std::endl;
    }
}


void
adc_test( size_t argc, const char ** argv )
{
    size_t count = 1;

    if ( argc >= 2 ) {
        count = strtod( argv[ 1 ] );
        if ( count == 0 )
            count = 1;
    }
    bool use_dma( false );
    auto it = std::find_if( argv, argv + argc, [](auto a){ return strcmp( a, "dma" ) == 0; } );
    use_dma = it != ( argv + argc );

    if ( !__adc0 ) {
        using namespace stm32f103;
        
        stream() << "adc0 not initialized." << std::endl;
        
        // it must be initalized in main though, just in case
        gpio_mode()( stm32f103::PA0, GPIO_CNF_INPUT_ANALOG, GPIO_MODE_INPUT ); // ADC1 (0,0)
        
        __adc0.init( stm32f103::ADC1_BASE );
        stream() << "adc reset & calibration: status " << (( __adc0.cr2() & 0x0c ) == 0 ? " PASS" : " FAIL" )  << std::endl;
    }

    if ( use_dma ) {

        __adc0.attach( __dma0 );
        if ( __adc0.start_conversion() ) { // software trigger
            uint32_t d = __adc0.data(); // can't read twince
            stream() << "adc data= 0x" << d
                     << "\t" << int(d) << "(mV)"
                     << std::endl;
        }
        
    } else {
        for ( size_t i = 0; i < count; ++i ) {
            if ( __adc0.start_conversion() ) { // software trigger
                uint32_t d = __adc0.data(); // can't read twince
                stream() << "[" << int(i) << "] adc data= 0x" << d
                         << "\t" << int(d) << "(mV)"
                         << std::endl;
            }
        }
    }
}

void
ad5593_test( size_t argc, const char ** argv )
{
    if ( !__i2c0 ) {
        const char * argv [] = { "i2c", nullptr };
        i2c_test( 1, argv );
    }
    size_t count = 1;
    for ( size_t i = 1; i < argc; ++i ) {
        if ( std::isdigit( *argv[i] ) ) {
            count = strtod( argv[i] );
            if ( count == 0 )
                count = 1;
        }
    }
    bool use_dma( false );
    auto it = std::find_if( argv, argv + argc, [](auto a){ return strcmp( a, "dma" ) == 0; } );
    use_dma = it != ( argv + argc );    

    ad5593::ad5593dev ad5593( &__i2c0, 0x10, use_dma );

    double volts = 1.0;
    double Vref = 3.3;
    
    uint16_t value = uint16_t( 0.5 + volts * 4096 / Vref );

    for ( size_t i = 0; i < count; ++i ) {

        for ( size_t pin = 0; pin < 4; ++pin ) {
            auto io = ad5593::io( ad5593, pin, ad5593::DAC_AND_ADC );
            io.set( value );
            uint16_t readValue = io.get();
            stream() << "\t[" << pin << "] " << value << ", " << readValue;
        }
        stream() << std::endl;
        for ( size_t pin = 4; pin < 8; ++pin ) {
            auto value = ad5593::io( ad5593, pin, ad5593::ADC ).get();
            stream() << "\t[" << pin << "] " << value << "\t";
        }
        stream() << std::endl << std::endl;
    }
}

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
    }
}

void
dma_test( size_t argc, const char ** argv )
{
    uint32_t channel = 1;

    auto it = std::find_if( argv, argv + argc, [](auto a){ return std::isdigit( a[0] ); } );
    if ( it != (argv + argc) ) {
        channel = **it - '0';
    }
    
    uint32_t src [] = { 1, 2, 3, 4 };
    uint32_t dst [ 4 ] = { 0 };

    using namespace stm32f103;
    constexpr uint32_t ccr = MEM2MEM | PL_High | 2 << 10 | 2 << 8 | MINC | PINC;

    __dma0.init_channel( DMA_CHANNEL(channel), reinterpret_cast< uint32_t >( src ), reinterpret_cast< uint8_t * >( dst ), 4, ccr );
    __dma0.enable( DMA_CHANNEL(channel), true );

    size_t count = 2000;
    while ( ! __dma0.transfer_complete( DMA_CHANNEL(channel) ) && --count)
        ;

    for ( int i = 0; i < 4; ++i )
        stream() << src[i] << " == " << dst[ i ] << std::endl;

    if ( count == 0 )
        stream() << "\tdma timeout\n";
}

void
afio_test( size_t argc, const char ** argv )
{
    if ( auto AFIO = reinterpret_cast< volatile stm32f103::AFIO * >( stm32f103::AFIO_BASE ) ) {    
        stream() << "\tAFIO MAPR: 0x" << AFIO->MAPR << std::endl;
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

///////////////////////////////////////////////////////

command_processor::command_processor()
{
}

class premitive {
public:
    const char * arg0_;
    void (*f_)(size_t, const char **);
    const char * help_;
};

static const premitive command_table [] = {
    { "spi",    spi_test,       " spi [replicates]" }
    , { "spi2", spi_test,       " spi2 [replicates]" }
    , { "alt",  alt_test,       " spi [remap]" }
    , { "gpio", gpio_test,      " pin# (toggle PA# as GPIO, where # is 0..12)" }
    , { "adc",  adc_test,       " replicates (1)" }
    , { "rcc",  rcc_status,     " RCC clock enable register list" }
    , { "disable", rcc_enable,  " reg1 [reg2...] Disable clock for specified peripheral." }
    , { "enable", rcc_enable,   " reg1 [reg2...] Enable clock for specified peripheral." }
    , { "afio", afio_test,      " AFIO MAPR list" }
    , { "i2c",  i2c_test,       " I2C-1 test" }
    , { "i2c2", i2c_test,       " I2C-2 test" }
    , { "i2cdetect", i2cdetect, " i2cdetect [0|1]" }
    , { "ad5593", ad5593_test,  " ad5593" }
    , { "dma",    dma_test,     " dma" }
};

bool
command_processor::operator()( size_t argc, const char ** argv ) const
{
    if ( argc ) {
#if 0
        stream() << "command_procedssor: argc=" << argc << " argv = {";
        for ( size_t i = 0; i < argc; ++i )
            stream() << argv[i] << ( ( i < argc - 1 ) ? ", " : "" );
        stream() << "}" << std::endl;
#endif

        bool processed( false );
    
        if ( argc > 0 ) {
            for ( auto& cmd: command_table ) {
                if ( strcmp( cmd.arg0_, argv[0] ) == 0 ) {
                    processed = true;
                    cmd.f_( argc, argv );
                    break;
                }
            }
        }
        
        if ( ! processed ) {
            stream() << "command processor -- help" << std::endl;
            for ( auto& cmd: command_table )
                stream() << "\t" << cmd.arg0_ << cmd.help_ << std::endl;

            stream() << "----------------- RCC -----------------" << std::endl;
            static const char * rcc_argv[] = { "rcc", 0 };
            rcc_status( 1, rcc_argv );
        }
    }
    stream() << std::endl;
}
