// Copyright (C) 2018 MS-Cheminformatics LLC
// Licence: CC BY-NC
// Author: Toshinobu Hondo, Ph.D.
// Contact: toshi.hondo@qtplatz.com
//

#include "command_processor.hpp"
#include "adc.hpp"
#include "clock.hpp"
#include "dma.hpp"
#include "dma_channel.hpp"
#include "gpio.hpp"
#include "gpio_mode.hpp"
#include "i2c.hpp"
#include "rtc.hpp"
#include "spi.hpp"
#include "stream.hpp"
#include "stm32f103.hpp"
#include "timer.hpp"
#include "utility.hpp"
#include <atomic>
#include <algorithm>
#include <cctype>
#include <chrono>

extern stm32f103::i2c __i2c0, __i2c1;
extern std::atomic< uint32_t > atomic_jiffies;
extern void mdelay( uint32_t ms );

void i2c_command( size_t argc, const char ** argv );
void i2cdetect( size_t argc, const char ** argv );
void bmp280_command( size_t argc, const char ** argv );
void ad5593_command( size_t argc, const char ** argv );
void rcc_status( size_t argc, const char ** argv );
void rtc_status( size_t argc, const char ** argv );
void rcc_enable( size_t argc, const char ** argv );
void timer_command( size_t argc, const char ** argv );
void gpio_command( size_t argc, const char ** argv );
void timer_command( size_t argc, const char ** argv );

int
strcmp( const char * a, const char * b )
{
    while ( *a && (*a == *b ) )
        a++, b++;
    return *reinterpret_cast< const unsigned char *>(a) - *reinterpret_cast< const unsigned char *>(b);
}

int
strncmp( const char * a, const char * b, size_t n )
{
    while ( --n && *a && (*a == *b ) )
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

uint32_t
strtox( const char * s )
{
    uint32_t xnum = 0;
    while ( *s && ( ('0' <= *s && *s <= '9') || ('a' <= *s && *s <= 'f') || ('A' <= *s && *s <= 'F') ) ) {
        uint8_t x = 0;
        if ( '0' <= *s && *s <= '9' )
            x = *s - '0';
        else if ( 'a' <= *s && *s <= 'f' )
            x = *s - 'a' + 10;
        else if ( 'A' <= *s && *s <= 'F' )
            x = *s - 'A' + 10;
        xnum = (xnum << 4) | x;
        ++s;
    }
    return xnum;
}

void
spi_command( size_t argc, const char ** argv )
{
    using namespace stm32f103;
    auto id = ( strcmp( argv[0], "spi") == 0 ) ? 0 : ( strcmp( argv[0], "spi2" ) == 0 ) ? 1 : (-1);
    auto& spix = ( id == 0 ) ? *spi_t< SPI1_BASE >::instance() : *spi_t< SPI2_BASE >::instance();

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
                spix.setup( 'B', 12 );
            else
                spix.setup( 0, 0 );
            
        } else {
            // SPI 1
            gpio_mode()( stm32f103::PA4, stm32f103::GPIO_CNF_ALT_OUTPUT_PUSH_PULL, stm32f103::GPIO_MODE_OUTPUT_50M ); // ~SS
            gpio_mode()( stm32f103::PA5, stm32f103::GPIO_CNF_ALT_OUTPUT_PUSH_PULL, stm32f103::GPIO_MODE_OUTPUT_50M ); // SCLK
            gpio_mode()( stm32f103::PA6, stm32f103::GPIO_CNF_INPUT_FLOATING,       stm32f103::GPIO_MODE_INPUT );      // MISO
            gpio_mode()( stm32f103::PA7, stm32f103::GPIO_CNF_ALT_OUTPUT_PUSH_PULL, stm32f103::GPIO_MODE_OUTPUT_50M ); // MOSI

            if ( spi_ss_soft )
                spi_t< SPI1_BASE >::instance()->setup( 'A', 4 );
            else
                spi_t< SPI1_BASE >::instance()->setup( 0, 0 );

            gpio_mode()( stm32f103::PB12, stm32f103::GPIO_CNF_INPUT_FLOATING,       stm32f103::GPIO_MODE_INPUT ); // ~SS
            gpio_mode()( stm32f103::PB13, stm32f103::GPIO_CNF_INPUT_FLOATING,       stm32f103::GPIO_MODE_INPUT ); // SCLK
            gpio_mode()( stm32f103::PB14, stm32f103::GPIO_CNF_ALT_OUTPUT_PUSH_PULL, stm32f103::GPIO_MODE_OUTPUT_50M ); // MISO
            gpio_mode()( stm32f103::PB15, stm32f103::GPIO_CNF_INPUT_FLOATING,       stm32f103::GPIO_MODE_INPUT ); // MOSI

            spi_t< SPI1_BASE >::instance()->slave_setup();
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
adc_command( size_t argc, const char ** argv )
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

    auto& __adc = *stm32f103::adc::instance();

    if ( !__adc ) {
        using namespace stm32f103;
        
        stream() << "adc0 not initialized." << std::endl;
        
        // it must be initalized in main though, just in case
        gpio_mode()( stm32f103::PA0, GPIO_CNF_INPUT_ANALOG, GPIO_MODE_INPUT ); // ADC1 (0,0)
        
        stream() << "adc reset & calibration: status " << (( __adc.cr2() & 0x0c ) == 0 ? " PASS" : " FAIL" )  << std::endl;
    }

    if ( use_dma ) {

        __adc.attach( *stm32f103::dma_t< stm32f103::DMA1_BASE >::instance() );
        if ( __adc.start_conversion() ) { // software trigger
            uint32_t d = __adc.data(); // can't read twince
            stream() << "adc data= 0x" << d
                     << "\t" << int(d) << "(mV)"
                     << std::endl;
        }
        
    } else {
        for ( size_t i = 0; i < count; ++i ) {
            if ( __adc.start_conversion() ) { // software trigger
                uint32_t d = __adc.data(); // can't read twince
                stream() << "[" << int(i) << "] adc data= 0x" << d
                         << "\t" << int(d) << "(mV)"
                         << std::endl;
            }
        }
    }
}

void
date_command( size_t argc, const char ** argv )
{
    extern clock::time_point __uptime;
    stream() << int( std::chrono::duration_cast< std::chrono::seconds >(clock::now() - clock::zero).count() ) << std::endl;
    stream() << int( std::chrono::duration_cast< std::chrono::milliseconds >(clock::now() - clock::zero).count() ) << std::endl;
}

void
dma_command( size_t argc, const char ** argv )
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

    dma_t< DMA1_BASE >::instance()->init_channel( DMA_CHANNEL(channel), reinterpret_cast< uint32_t >( src ), reinterpret_cast< uint8_t * >( dst ), 4, ccr );

    stm32f103::scoped_dma_channel_enable<stm32f103::dma> enable_dma_channel( *dma_t< DMA1_BASE >::instance(), channel );

    size_t count = 2000;
    while ( ! dma_t< DMA1_BASE >::instance()->transfer_complete( DMA_CHANNEL(channel) ) && --count)
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
rtc_status( size_t argc, const char ** argv )
{
    if ( argc == 1 )
        stm32f103::rtc::print_registers();
    while ( --argc ) {
        ++argv;
        if ( strcmp( argv[0], "reset" ) == 0 ) {
            stm32f103::rtc::instance()->reset();
        } else if ( strcmp( argv[0], "enable" ) == 0 ) {
            stm32f103::rtc::instance()->enable();
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
    { "spi",    spi_command,    " spi [replicates]" }
    , { "spi2", spi_command,    " spi2 [replicates]" }
    , { "alt",  alt_test,       " spi [remap]" }
    , { "gpio", gpio_command,   " pin# (toggle PA# as GPIO, where # is 0..12)" }
    , { "adc",  adc_command,    " replicates (1)" }
    , { "rcc",  rcc_status,     " RCC clock enable register list" }
    , { "rtc",  rtc_status,     " RTC register print" }
    , { "disable", rcc_enable,  " reg1 [reg2...] Disable clock for specified peripheral." }
    , { "enable", rcc_enable,   " reg1 [reg2...] Enable clock for specified peripheral." }
    , { "afio", afio_test,      " AFIO MAPR list" }
    , { "i2c",  i2c_command,    " I2C-1 test" }
    , { "i2c2", i2c_command,    " I2C-2 test" }
    , { "i2cdetect", i2cdetect, " i2cdetect [0|1]" }
    , { "ad5593", ad5593_command,  " ad5593" }
    , { "dma",    dma_command,     " dma" }
    , { "bmp",    bmp280_command,     "" }
    , { "timer",  timer_command,     "" }
    , { "date",   date_command,     "" }    
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
                    stream() << std::endl;
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
