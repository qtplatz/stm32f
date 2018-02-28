// Copyright (C) 2018 MS-Cheminformatics LLC

#include <atomic>
#include <cstdint>

namespace stm32f103 {

    enum SPI_BASE : uint32_t;
    struct SPI;

    class spi {
        volatile SPI * spi_;
        std::atomic_flag lock_;
        std::atomic< uint32_t > rxd_;
    public:
        void init( SPI_BASE );
        inline operator bool () const { return spi_; };
        
        spi& operator << ( uint16_t );

        void handle_interrupt();
        static void interrupt_handler( spi * );
    };

}

