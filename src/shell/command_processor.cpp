// Copyright (C) 2018 MS-Cheminformatics LLC
// Licence: CC BY-NC
// Author: Toshinobu Hondo, Ph.D.
// Contact: toshi.hondo@qtplatz.com
//

#include "command_processor.hpp"
#include "adc.hpp"
#include "bkp.hpp"
#include "condition_wait.hpp"
#include "dma.hpp"
#include "dma_channel.hpp"
#include "gpio.hpp"
#include "gpio_mode.hpp"
#include "i2c.hpp"
#include "rtc.hpp"
#include "spi.hpp"
#include "stream.hpp"
#include "stm32f103.hpp"
#include "system_clock.hpp"
#include "timer.hpp"
#include "utility.hpp"
#include <atomic>
#include <algorithm>
#include <cctype>
#include <chrono>

extern std::atomic< uint32_t > atomic_jiffies;
extern void mdelay( uint32_t ms );

void can_command( size_t argc, const char ** argv );
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
void date_command( size_t argc, const char ** argv );
void hwclock_command( size_t argc, const char ** argv );
void dc_command( size_t argc, const char ** argv );
void help( size_t argc, const char ** argv );

void
system_reset( size_t argc, const char ** argv )
{
    auto SCB = reinterpret_cast< volatile stm32f103::SCB * >( stm32f103::SCB_BASE );
    SCB->AIRCR = 0x5fa << 16 |  (SCB->AIRCR & 0x700) | (1 << 2);
    __asm volatile ("dsb");
}

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
            num = (num * 10) + ((*s) - '0');
        ++s;
    }
    return num * sign;
}

uint32_t
strtox( const char * s, char ** end )
{
    int sign = 1;
    uint32_t xnum = 0;

    while ( *s && (*s == ' ' || *s == '\t' ) )
        ++s;

    if ( *s == '-' ) {
        sign = -1;
        ++s;
    }

    size_t count = 8;
    while ( count-- && *s && ( ('0' <= *s && *s <= '9') || ('a' <= *s && *s <= 'f') || ('A' <= *s && *s <= 'F') ) ) {
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

    if ( end )
        *end = const_cast< char * >(s);

    return xnum * sign;
}

char *
strncpy( char * dst, const char * src, size_t size )
{
    auto save(dst);
    while ( size-- && ( *dst++ = *src++ ) )
        ;
    return save;
}

char *
strncat( char * dst, const char * src, size_t size )
{
    auto save(dst);
    while ( size-- && *dst++ )
        ;
    strncpy( --dst, src, size + 1 );
    return save;
}

void
bkp_command( size_t argc, const char ** argv )
{
    stm32f103::bkp::print_registers();
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

    auto it = std::find_if( argv, argv + argc, [](auto a){ return strcmp( a, "help" ) == 0; } );
    if ( argc < 2 || ( it != (argv + argc ) ) ) { // help
        stream() << "adc on NN -- enable ADC; start AD conversion by software cpu cycle, NN replicates.\n";
        stream() << "adc off -- disable ADC.\n";        
        stream() << "adc dma -- auto (hardware) repeat AD conversion in background (continue until cpu reset).\n";
        return;
    }

    auto& __adc = *stm32f103::adc::instance();

    if ( !__adc ) {
        using namespace stm32f103;

        stream() << "adc0 not initialized." << std::endl;

        // it must be initalized in main though, just in case
        gpio_mode()( stm32f103::PA0, GPIO_CNF_INPUT_ANALOG, GPIO_MODE_INPUT ); // ADC1 (0,0)

        stream() << "adc reset & calibration: status " << (( __adc.cr2() & 0x0c ) == 0 ? " PASS" : " FAIL" )  << std::endl;
    }

    while ( --argc ) {
        ++argv;
        if ( strcmp( argv[0], "off" ) == 0 ) {
            stream() << "adc.enable( false )" << std::endl;
            __adc.enable( false );
        } else if ( strcmp( argv[0], "on" ) == 0 ) {
            stream() << "adc.enable( true )" << std::endl;
            __adc.enable( true );
        } else if ( strcmp( argv[0], "dma" ) == 0 || strcmp( argv[0], "start" ) == 0 ) {
            stream() << "adc.attach( dma1 )" << std::endl;
            __adc.attach( *stm32f103::dma_t< stm32f103::DMA1_BASE >::instance() );
            if ( __adc.start_conversion() ) { // software trigger
                uint32_t d = __adc.data(); // can't read twince
                stream() << "adc data= 0x" << d
                         << "\t" << int(d) << "(mV)"
                         << std::endl;
            }
        } else if ( std::isdigit( *argv[0] ) ) {
            count = strtod( *argv );
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
}

void
dma_command( size_t argc, const char ** argv )
{
    uint32_t channel = 1;

    auto it = std::find_if( argv, argv + argc, [](auto a){ return std::isdigit( a[0] ); } );
    if ( it != (argv + argc) ) {
        channel = **it - '0';
    }

    constexpr static uint32_t src [] = { 0x1a2b3c4d, uint32_t(-2), uint32_t(-3), 4, 5, 6, 7, 8 };
    uint32_t dst [ countof( src ) ] = { 0 };

    int i = 0;
    for ( auto& s: src )
        stream() << s << " -> " << dst[ i++ ] << std::endl;

    using namespace stm32f103;
    //constexpr uint32_t ccr = MEM2MEM | PL_High | 2 << 10 | 2 << 8 | MINC | PINC;  // [11:10], [1:0] size {0,1,3} = {8,16,32 bits}
    constexpr uint32_t ccr = MEM2MEM | PL_High | 0 << 10 | 0 << 8 | MINC | PINC;  // [11:10], [1:0] size {0,1,3} = {8,16,32 bits}

    dma_t< DMA1_BASE >::instance()->init_channel( DMA_CHANNEL(channel), reinterpret_cast< uint32_t >( src ), reinterpret_cast< uint8_t * >( dst ), 5, ccr );

    stm32f103::scoped_dma_channel_enable<stm32f103::dma> enable_dma_channel( *dma_t< DMA1_BASE >::instance(), channel );

    if ( ! condition_wait()([&]{ return dma_t< DMA1_BASE >::instance()->transfer_complete( DMA_CHANNEL(channel) ); } ) ) {
        stream() << "\tdma timeout\n";
        return;
    }

    stream() << "dma result\n";
    i = 0;
    for ( auto& s: src )
        stream() << s << " == " << dst[ i++ ] << std::endl;
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

class primitive {
public:
    const char * arg0_;
    void (*f_)(size_t, const char **);
    const char * help_;
};

static const primitive command_table [] = {
    { "ad5593",      ad5593_command,  " ad5593" }
    , { "adc",       adc_command,     " replicates (1)" }
    , { "afio",      afio_test,       " AFIO MAPR list" }
    , { "alt",       alt_test,        " spi [remap]" }
    , { "bkp",       bkp_command,     " backup registers" }
    , { "bmp",       bmp280_command,  " start|stop" }
    , { "can",       can_command,     " can" }
    , { "candump",   can_command,     " candump" }
    , { "cansend",   can_command,     " cansend 01a#11223333aabbccdd" }
    , { "date",      date_command,    " show current date time; date --set 'iso format date'" }
    , { "dc",        dc_command,      " dc values..." }    
    , { "disable",   rcc_enable,      " reg1 [reg2...] Disable clock for specified peripheral." }
    , { "dma",       dma_command,     " ram to ram dma copy teset" }
    , { "enable",    rcc_enable,      " reg1 [reg2...] Enable clock for specified peripheral." }
    , { "gpio",      gpio_command,    " pin# (toggle PA# as GPIO, where # is 0..12)" }
    , { "hwclock",   hwclock_command, "" }
    , { "i2c",       i2c_command,     " I2C-1 test" }
    , { "i2c2",      i2c_command,     " I2C-2 test" }
    , { "i2cdetect", i2cdetect,       " i2cdetect [0|1]" }
    , { "rcc",       rcc_status,      " RCC clock enable register list" }
    , { "reset",     system_reset,    "" }
    , { "rtc",       rtc_status,      " RTC register print" }
    , { "spi",       spi_command,     " spi [replicates]" }
    , { "spi2",      spi_command,     " spi2 [replicates]" }
    , { "timer",     timer_command,   "" }
    , { "help",      help, "" }
    , { "?", help, "" }
};

constexpr size_t command_table_size = sizeof(command_table)/sizeof(command_table[0]);

void
help( size_t argc, const char ** argv )
{
    stream() << "command processor -- help" << std::endl;
    for ( auto& cmd: command_table )
        stream() << "\t" << cmd.arg0_ << cmd.help_ << std::endl;

    stream() << "----------------- RCC -----------------" << std::endl;
    static const char * rcc_argv[] = { "rcc", 0 };
    rcc_status( 1, rcc_argv );
}

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
            auto it = std::find_if( command_table, command_table + command_table_size
                                    , [&](const auto& cmd){ return strcmp(cmd.arg0_, argv[0]) == 0; });
            if ( it != command_table + command_table_size ) {
                processed = true;
                stream() << std::endl;
                it->f_( argc, argv );                
            }
        }

        if ( ! processed && argc > 0 ) {
            stream() << "Unknown command: " << argv[0] << "\ttype help for command list" << std::endl;
        }
    }
    stream() << std::endl;

    return true;
}
