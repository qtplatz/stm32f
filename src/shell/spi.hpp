// Copyright (C) 2018 MS-Cheminformatics LLC

#include <atomic>
#include <cstdint>

namespace stm32f103 {

    template< typename T > class gpio;
    
    enum SPI_BASE : uint32_t;
    struct SPI;

    class dma;

    class spi {
        volatile SPI * spi_;
        std::atomic_flag lock_;
        std::atomic< uint32_t > rxd_;
        std::atomic< uint32_t > txd_;
        
        // workaround -- Initially, I thought GPIO and SPI controls are fully independent,
        // bit this peripheral seems exepecting a ~ss line control by software using GPIO.
        // I've designed GPIO control based on c++ type dispatch, but not for SPI.
        uint8_t gpio_;  // A|B|none
        uint32_t ss_n_;  // PA4|PB
        uint32_t cr1_;
        dma * dma_;
        uint32_t dma_channel_;
        void init( SPI_BASE, uint8_t gpio = 0, uint32_t ss_n = 0 );
        template< SPI_BASE > friend struct spi_t;
    public:
        // void init( SPI_BASE, dma& );
        void setup( uint8_t gpio = 0, uint32_t ss_n = 0 );
        void slave_setup();

        inline operator bool () const { return spi_; };
        
        spi& operator << ( uint16_t );
        spi& operator >> ( uint16_t& );
        
        void operator = ( bool flag ); // SS control

        void handle_interrupt();
        static void interrupt_handler( spi * );
    };

    template< SPI_BASE base >
    struct spi_t {
        static std::atomic_flag once_flag_;

        static inline spi * instance() {
            static spi __instance;
            if ( !once_flag_.test_and_set() )
                __instance.init( base );            
            return &__instance;
        }
    };

    template< SPI_BASE base > std::atomic_flag spi_t< base >::once_flag_;
}

