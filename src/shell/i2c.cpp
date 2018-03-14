// Copyright (C) 2018 MS-Cheminformatics LLC
// Licence: CC BY-NC
// Author: Toshinobu Hondo, Ph.D.
// Contact: toshi.hondo@qtplatz.com
//

#include "bitset.hpp"
#include "dma.hpp"
#include "dma_channel.hpp"
#include "i2c.hpp"
#include "i2cdebug.hpp"
#include "stream.hpp"
#include "stm32f103.hpp"
#include <array>
#include <atomic>
#include <mutex>

extern uint32_t __pclk1, __pclk2;
extern stm32f103::i2c __i2c0;
extern void mdelay( uint32_t );
extern std::atomic< uint32_t > atomic_jiffies;

extern "C" {
    void i2c1_handler();
    void enable_interrupt( stm32f103::IRQn_type IRQn );
}

extern stm32f103::dma __dma0;

// bits in the status register
namespace stm32f103 {

    constexpr uint32_t i2c_clock_speed = 100'000; //'; //100kHz
    
    enum I2C_CR1_MASK {
        SWRST          = 1 << 15  // Software reset (0 := not under reset, 0 := under reset state
        , RES0         = 1 << 14  //
        , ALERT        = 1 << 13  //
        , PEC          = 1 << 12  //
        , POS          = 1 << 11  // Acknowledge/PEC Position (for data reception)
        , ACK          = 1 << 10  //
        , STOP         = 1 <<  9  //
        , START        = 1 <<  8  //
        , NOSTRETCH    = 1 <<  7  //
        , ENGC         = 1 <<  6  //
        , ENPEC        = 1 <<  5  //
        , ENARP        = 1 <<  4  //
        , SMBTYPE      = 1 <<  3  //
        , RES1         = 1 <<  2  //
        , SMBUS        = 1 <<  1  //
        , PE           = 1 <<  0  // Peripheral enable
    };

    enum I2C_CR2_MASK {
        LAST          = 1 << 12
        , DMAEN       = 1 << 11    // DMA requests enable
        , ITBUFFN     = 1 << 10    // Buffer interrupt enable
        , ITEVTEN     = 1 <<  9    // Event interrupt enable
        , ITERREN     = 1 <<  8    // Error interrupt enable
        , FREQ        = 0x3f       // Peirpheral clock frequence ( 0x02(2MHz) .. 0x32(50MHz) )
    };

   // p782
    enum I2C_STATUS {
        // SR2, p782
        ST_PEC          = 0xff << 8  // Packet error checking rigister
        , DUALF         = 1 << 7     // Dual flag (slave mode)
        , SMBHOST       = 1 << 6     // SMBus host header (slave mode)
        , SMBDEFAULT    = 1 << 5     // SMB deault, SMBus device default address (slave mode)
        , GENCALL       = 1 << 4     // General call address (slave mode)
        , TRA           = 1 << 2     // Transmitter/receiver (0: data bytes received, 1: data bytes transmitted)
        , BUSY          = 1 << 1     // Bus busy
        , MSL           = 1          // Master/slave
        // SR1, p778
        , SMB_ALART     = 1 << 15    // SMBus alert
        , TIME_OUT      = 1 << 14    // Timeout or Tlow error
        , PEC_ERR       = 1 << 12    // PEC Error in reception
        , OVR           = 1 << 11    // Overrun/Underrun
        , AF            = 1 << 10    // Acknowledge failure
        , ARLO          = 1 << 9     // arbitration lost
        , BERR          = 1 << 8     // Bus error
        , TxE           = 1 << 7     // Data register empty (transmitter)
        , RxNE          = 1 << 6     // Date register not empty (receiver)
        , STOPF         = 1 << 4     // Stop detection (slave)
        , ADD10         = 1 << 3     // 10-bit header sent
        , BTF           = 1 << 2     // Byte transfer finished
        , ADDR          = 1 << 1     // Address sent (master), matched(slave)
        , SB            = 1          // Start bit (master mode); 1: start condition generated
    };

    constexpr uint32_t error_condition = SMB_ALART | TIME_OUT | PEC_ERR | OVR | AF | ARLO | BERR;
    
    struct i2c_status {
        volatile I2C& _;
        i2c_status( volatile I2C& t ) : _( t ) {}
        inline bool busy() const { return _.SR2 & BUSY; }

        // p783, Reading SR2 after reading SR1 clears ADDR flag.
        inline uint32_t status() const { return ( (_.SR1 & STOPF) == 0
                                                  || ((_.SR1 & ADDR) == ADDR ) ) ? ( _.SR2 << 16 ) | _.SR1 : _.SR1; }

        inline uint32_t operator ()() const { return status(); }

        inline bool is_equal( uint32_t flags ) const {
            return ( status() & flags ) == flags;
        }

        inline bool is_equal( uint32_t flags, uint32_t mask ) const {
            return ( status() & mask ) == flags;
        }
    };

    // master start
    struct i2c_start {
        volatile I2C& _;
        i2c_start( volatile I2C& t ) : _( t ) {}

        constexpr static uint32_t master_mode_selected = ( ( BUSY | MSL ) << 16 ) | SB;

        inline bool operator()() const {
            size_t count = 0x7fff;
            i2c_status st( _ );

            _.CR1 |= START;
                
            while( !st.is_equal( master_mode_selected ) && --count )
                ;

            return count != 0;
        }
    };

    struct condition_waiter {
        size_t count;
        condition_waiter() : count( 0xffff ) {}
        template< typename functor >  bool operator()( functor condition ) {
            while ( --count && !condition() )
                ;
            return count != 0;
        }
    };

    struct scoped_i2c_dma_enable {
        volatile I2C& _;
        scoped_i2c_dma_enable( volatile I2C& t ) : _( t ) {
            _.CR2 |= DMAEN;
        }
        ~scoped_i2c_dma_enable() {
            _.CR2 &= ~DMAEN;
        }        
    };

    struct scoped_i2c_start {
        volatile I2C& _;
        bool success;
        scoped_i2c_start( volatile I2C& t ) : _( t ), success( false ) {}
        ~scoped_i2c_start() {
            if ( success ) {
                bitset::set( _.CR1, STOP );
                condition_waiter()( [&](){ return !_.CR1 & STOP; } );
            }
        }
        
        bool operator()() {
            success = i2c_start( _ )();
            return success;
        }
    };

    enum I2C_DIRECTION { Transmitter, Receiver };

    template< I2C_DIRECTION >    
    struct i2c_address {
        inline bool operator()( volatile I2C& _, uint8_t address ) {
            
            _.DR = ( address << 1 );
            
            constexpr uint32_t byte_transmitting = (( TRA | BUSY | MSL) << 16 ) | TxE;
            i2c_status st( _ );
            size_t count = 0x7fff;
            while( !st.is_equal( byte_transmitting ) && --count )
                ;
            return st.is_equal( byte_transmitting );
            return true;
        }
        void static clear( volatile I2C& ) {}
    };

    template<> inline bool i2c_address<Receiver>::operator()( volatile I2C& _, uint8_t address ) {
        _.DR = (address << 1) | 1;
        return condition_waiter()( [&](){ return _.SR1 & ADDR; } );
    }
    
    template<> void i2c_address<Receiver>::clear( volatile I2C& _ ) {
        volatile auto x = _.SR1;
        volatile auto y = _.SR2;
    }

    // AN2824, I2C optimized examples en.CD00209826.pdf
    // receiving
    template< size_t >
    struct polling_master_receiver {
        volatile I2C& _;
        polling_master_receiver( volatile I2C& t ) : _( t ) {}
        bool operator()( uint8_t address, uint8_t * data, size_t size ) {
            if ( i2c_start( _ )() ) {
                if ( i2c_address< Receiver >()( _, address ) ) {
                    i2c_address<Receiver>().clear( _ );
                    condition_waiter()( [&](){ return !_.SR1 & ADDR; } );
                    while ( size > 3 ) {
                        if ( condition_waiter()( [&](){ return _.SR1 & (RxNE|BTF); } ) ) {
                            if ( _.SR1 & RxNE )
                                *data++ = _.DR;
                        }
                    }
                    if ( condition_waiter()( [&](){ return _.SR1 & BTF; } ) ) {
                        bitset::reset( _.CR1, ACK );
                        // disable irq
                        bitset::set( _.CR1, STOP );
                        *data++ = _.DR;  // Data N-1
                        --size;
                    }
                    if ( condition_waiter()( [&](){ return _.SR1 & RxNE; } ) ) {
                        *data++ = _.DR;
                        --size;
                        if ( condition_waiter()( [&](){ return !_.SR1 & STOP; } ) ) // wait untl STOP is cleard
                            bitset::set( _.CR1, ACK ); // to be ready for another reception
                        return size == 0;
                    }
                } else {
                    stream(__FILE__,__LINE__) << "polling_master_receiver<3> address failed.\n";
                }
            } else {
                stream(__FILE__,__LINE__) << "polling_master_receiver<3> start failed.\n";
            }
            return false;                
        }
    };

    template<> bool polling_master_receiver< 2 >::operator()( uint8_t address, uint8_t * data, size_t size ) {
        if ( i2c_start( _ )() ) {
            if ( i2c_address< Receiver >()( _, address ) ) {
                bitset::set( _.CR1, POS );
                i2c_address<Receiver>().clear( _ );
                bitset::reset( _.CR1, ACK );
                if ( condition_waiter()( [&](){ return _.SR1 & BTF; } ) ) {
                    bitset::set( _.CR1, STOP );
                    *data++ = _.DR;
                    --size;
                    if ( condition_waiter()( [&](){ return _.SR1 & (RxNE|BTF); } ) ) {
                        *data++ = _.DR;
                        --size;
                        if ( condition_waiter()( [&](){ return !_.SR1 & STOP; } ) ) { // wait untl STOP is cleard
                            bitset::reset( _.CR1, POS ); // to be ready for another reception
                            bitset::set( _.CR1, ACK ); // to be ready for another reception
                        }
                    }
                }
                return size == 0;
            } else {
                stream(__FILE__,__LINE__) << "polling_master_receiver<3> address failed.\n";
            }
        } else {
            stream(__FILE__,__LINE__) << "polling_master_receiver<3> start failed.\n";
        }
    }

    template<> bool polling_master_receiver< 1 >::operator()( uint8_t address, uint8_t * data, size_t size ) {
        if ( i2c_start( _ )() ) {
            if ( i2c_address< Receiver >()( _, address ) ) {
                bitset::reset( _.CR1, ACK );
                i2c_address<Receiver>().clear( _ );
                bitset::set( _.CR1, STOP );
                if ( condition_waiter()( [&](){ return _.SR1 & RxNE; } ) ) {
                    *data++ = _.DR;
                    --size;
                    if ( condition_waiter()( [&](){ return !_.SR1 & STOP; } ) ) { // wait untl STOP is cleard
                        bitset::set( _.CR1, ACK ); // to be ready for another reception
                    }
                }
                return size == 0;
            } else {
                stream(__FILE__,__LINE__) << "polling_master_receiver<3> address failed.\n";
            }
        } else {
            stream(__FILE__,__LINE__) << "polling_master_receiver<3> start failed.\n";
        }                
        return false;
    }
    /////////////////////////////////////////////////////

    // master transmit
    struct i2c_transmitter {
        volatile I2C& _;
        i2c_transmitter( volatile I2C& t ) : _( t ) {}

        inline bool operator << ( uint8_t data ) {
            constexpr size_t byte_transferred = (( TRA | BUSY | MSL ) << 16) | TxE | BTF; // /*0x00070084*/
            _.DR = data;
            size_t count = 0x7fff;
            i2c_status st( _ );
            while ( !st.is_equal( byte_transferred ) && --count )
                ;
            return st.is_equal( byte_transferred );
        }

        inline bool operator >> ( uint8_t& data ) {
            constexpr size_t byte_received = (( BUSY | MSL ) << 16) | RxNE;
            size_t count = 0x7fff;
            i2c_status st( _ );
            while ( !st.is_equal( byte_received ) && --count )
                ;
            if ( st.is_equal( byte_received ) ) {
                data = _.DR;
                return true;
            }
            return false;
        }
    };

    struct i2c_reset {
        bool operator()( volatile I2C& i2c, uint8_t own_addr ) {
            if ( own_addr == 0 )
                own_addr = i2c.OAR1 >> 1;

            bitset::reset( i2c.CR1, PE );
            bitset::set( i2c.CR1, SWRST );
            while ( i2c.SR1 && i2c.SR2 )
                ;
            bitset::reset( i2c.CR1, SWRST );
            bitset::reset( i2c.CR1, PE );
            
            uint16_t freqrange = uint16_t( __pclk1 / 1000'000 /*'*/ ); // souce clk in MHz
            uint16_t cr2 = freqrange;
            i2c.CR2 |= cr2;
            
            // p784, 
            i2c.TRISE = freqrange + 1;
            i2c.OAR1 = own_addr << 1;
            i2c.OAR2 = 0;
            
            uint16_t ccr = __pclk1 / ( i2c_clock_speed * 2 );  // ratio make it 5us for 10us interval clock
            i2c.CCR = ccr;  // Sm mode
        }
    };

    struct i2c_ready_wait {
        volatile I2C& i2c;
        uint8_t own_addr_;
        i2c_ready_wait( volatile I2C& t, uint8_t own_addr ) : i2c( t ), own_addr_( own_addr ) {}
        
        bool operator()( bool reset = false ) const {
            i2c_status st( i2c );
            size_t retry = 3;
            do {
                size_t count = 3000;
                while ( ( st.busy() || ( st() & error_condition ) ) && --count ) {
                    if ( ( i2c.SR1 & error_condition ) && ( i2c.SR2 & (BUSY| MSL) ) )
                        i2c_reset()( i2c, own_addr_ );
                }
            } while ( st.busy() && --retry );
            if ( st.busy() && reset ) {
                stream() << "i2c_ready_wait -- busy -- resetting..." << std::endl;
                i2c_reset()( i2c, own_addr_ );
            }
            return !st.busy();
        }
    };
}

using namespace stm32f103;
using namespace stm32f103::i2cdebug;

static uint8_t __i2c1_tx_dma[ sizeof( dma_channel_t< DMA_I2C1_TX > ) ];
static uint8_t __i2c1_rx_dma[ sizeof( dma_channel_t< DMA_I2C1_RX > ) ];
static uint8_t __i2c2_tx_dma[ sizeof( dma_channel_t< DMA_I2C2_TX > ) ];
static uint8_t __i2c2_rx_dma[ sizeof( dma_channel_t< DMA_I2C2_RX > ) ];

static dma_channel_t< DMA_I2C1_TX > * __dma_i2c1_tx;
static dma_channel_t< DMA_I2C1_RX > * __dma_i2c1_rx;
static dma_channel_t< DMA_I2C2_TX > * __dma_i2c2_tx;
static dma_channel_t< DMA_I2C2_RX > * __dma_i2c2_rx;

i2c::i2c() : i2c_( 0 )
{
}

void
i2c::attach( dma& dma, DMA_Direction dir )
{
    uint32_t addr = reinterpret_cast< uint32_t >(const_cast< I2C * >(i2c_));

    stream() << "*********************** attach " << addr
             << " with dma (" << (dir == DMA_Tx ? "Tx" : ( dir == DMA_Rx ? "Rx" : "Both"))
             << ") **********************"
             << std::endl;

    if ( addr == I2C1_BASE ) {
        if ( dir == DMA_Rx || dir == DMA_Both ) {
            if ( __dma_i2c1_rx = new (&__i2c1_rx_dma) dma_channel_t< DMA_I2C1_RX >( dma, 0, 0 ) ) {
                __dma_i2c1_rx->set_callback( +[]( uint32_t flag ){
                        stream() << "\n\tI2C-1 Rx irq: " << flag << std::endl;
                    });
            }
        }
        if ( dir == DMA_Tx || dir == DMA_Both ) {
            if ( __dma_i2c1_tx = new (&__i2c1_tx_dma) dma_channel_t< DMA_I2C1_TX >( dma, 0, 0 ) ) {
                __dma_i2c1_tx->set_callback( +[]( uint32_t flag ){
                        stream() << "\n\tI2C-1 Tx irq: " << flag << std::endl;
                    });                
            }
        }
    } else if ( addr == I2C2_BASE ) {
        if ( dir == DMA_Rx || dir == DMA_Both ) {
            if ( __dma_i2c2_rx = new (&__i2c2_rx_dma) dma_channel_t< DMA_I2C2_RX >( dma, 0, 0 ) ) {
                __dma_i2c2_rx->set_callback( +[]( uint32_t flag ){
                        stream() << "\n\tI2C-2 Rx irq: " << flag << std::endl;
                    });
            }
        }
        if ( dir == DMA_Tx || dir == DMA_Both ) {
            if ( __dma_i2c2_tx = new (&__i2c2_tx_dma) dma_channel_t< DMA_I2C2_TX >( dma, 0, 0 ) ) {
                __dma_i2c2_tx->set_callback( +[]( uint32_t flag ){
                        stream() << "\n\tI2C-2 Tx irq: " << flag << std::endl;
                    });
            }
        }
    }
}

void
i2c::init( stm32f103::I2C_BASE addr )
{
    lock_.clear();
    own_addr_ = ( addr == I2C1_BASE ) ? 0x03 : 0x04;

    if ( auto I2C = reinterpret_cast< volatile stm32f103::I2C * >( addr ) ) {
        i2c_ = I2C;
        
        reset();
        
        switch ( addr ) {
        case I2C1_BASE:
            enable_interrupt( I2C1_EV_IRQn );
            enable_interrupt( I2C1_ER_IRQn );
            break;
        case I2C2_BASE:
            enable_interrupt( I2C2_EV_IRQn );
            enable_interrupt( I2C2_ER_IRQn );            
            break;
        }
    }

}

void
i2c::set_own_addr( uint8_t addr )
{
    own_addr_ = addr;
    i2c_->OAR1 = own_addr_ << 1;
}

uint8_t
i2c::own_addr() const
{
    return uint8_t( i2c_->OAR1 >> 1 );
}

void
i2c::reset()
{
    i2c_reset()( *i2c_, own_addr_ );
}

bool
i2c::has_dma( DMA_Direction dir ) const
{
    if ( reinterpret_cast< uint32_t >(const_cast< I2C * >(i2c_)) == I2C1_BASE ) {
        if ( dir == DMA_Rx )
            return __dma_i2c1_rx != nullptr;
        else if ( dir == DMA_Tx )
            return __dma_i2c1_tx != nullptr;
        else if ( dir == DMA_Both )
            return __dma_i2c1_rx != nullptr && __dma_i2c1_tx != nullptr;
        else
            return __dma_i2c1_rx == nullptr && __dma_i2c1_tx == nullptr;
    } else if ( reinterpret_cast< uint32_t >(const_cast< I2C * >(i2c_)) == I2C2_BASE ) {
        if ( dir == DMA_Rx )
            return __dma_i2c2_rx != nullptr;
        else if ( dir == DMA_Tx )
            return __dma_i2c2_tx != nullptr;
        else if ( dir == DMA_Both )
            return __dma_i2c2_rx != nullptr && __dma_i2c2_tx != nullptr;
        else
            return __dma_i2c2_rx == nullptr && __dma_i2c2_tx == nullptr;
    }
    return false;
}

void
i2c::print_status() const
{
    stream() << "OAR1 : 0x" << i2c_->OAR1 << "\tOwn address: " << int(i2c_->OAR1 >> 1) << std::endl;
    stream() << "CCR  : {" << int( i2c_->CCR & 0x7ff ) << "}\t";
    stream() << "TRISE: {" << int( i2c_->TRISE ) << "(" << ((i2c_->TRISE&0x3f)-1)/(i2c_->CR2&0x3f) << "us)}" << std::endl;
    
    stream() << "CR1,2: [" << i2c_->CR1 << "," << i2c_->CR2 << "]\t" << CR1_to_string( i2c_->CR1 ) << "\t" << CR2_to_string( i2c_->CR2 ) << std::endl;
    stream() << "SR1,2: " << status32_to_string( i2c_status( *i2c_ )() ) << std::endl;

    int count = 10;
    while( --count && ( i2c_->SR2 & BUSY ) )
        mdelay( 1 );

    if ( count == 0 )
        stream() << "I2C keep busy -- check SDA line, it must be high" << std::endl;
}


bool
i2c::read( uint8_t address, uint8_t * data, size_t size )
{
    bitset::set( i2c_->CR1, PE );

    if ( ! i2c_ready_wait( *i2c_, own_addr_ )() ) {
        stream() << __FUNCTION__ << " i2c_ready_wait failed\n";
        return false;
    }

    if ( size > 3 )
        return polling_master_receiver<3>( *i2c_ )( address, data, size );
    if ( size == 2 )
        return polling_master_receiver<2>( *i2c_ )( address, data, size );
    if ( size == 1 )
        return polling_master_receiver<1>( *i2c_ )( address, data, size );
    return false;
}

bool
i2c::write( uint8_t address, const uint8_t * data, size_t size )
{
    bitset::set( i2c_->CR1, PE );

    if ( ! i2c_ready_wait( *i2c_, own_addr_ )() ) {
        stream(__FILE__,__LINE__) << __FUNCTION__ << " i2c_ready_wait failed\n";
        return false;
    }

    scoped_i2c_start start( *i2c_ );

    if ( start() ) {    
        if ( i2c_address< Transmitter >()( *i2c_, address ) ) { // address phase
            i2c_transmitter writer( *i2c_ );
            while ( size && ( writer << *data++ ) )
                --size;
            return size == 0;
        }
    }
    return false;
}

namespace stm32f103 {

    struct dma_master_transfer {
        volatile I2C& _;
        dma_master_transfer( volatile I2C& t ) : _( t ) {
            _.CR1 |= ACK | PE;  // ACK enable, peripheral enable
        }

        template< typename T >
        bool operator()( T& dma_channel, uint8_t address, const uint8_t * data, size_t size ) const {

            scoped_i2c_start start( _ );
            if ( start() ) { // generate start condition (master start)
                dma_channel.set_transfer_buffer( data, size );
                
                if ( i2c_address< Transmitter >()( _, address ) ) {

                    scoped_dma_channel_enable enable_dma_channel( dma_channel );
                    scoped_i2c_dma_enable dma_enable( _ );
                    
                    size_t count = 0x7fff;
                    while ( --count && !dma_channel.transfer_complete() )
                        ;
                    if ( count == 0 )
                        stream(__FILE__,__LINE__) << "dma_master_transfer timeout\n";
                    return count != 0;
                } else {
                    stream(__FILE__,__LINE__) << "i2c::dma_master_transfer -- address phase failed: " << status32_to_string( i2c_status( _ )() ) << std::endl;
                }
            } else {
                stream(__FILE__,__LINE__) << "i2c::dma_master_transfer() -- can't generate start condition" << std::endl;
            }
            return false;
        }
    };

    struct dma_master_receiver {
        volatile I2C& _;
        dma_master_receiver( volatile I2C& t ) : _( t ) {
            bitset::set( _.CR1, PE );  // peripheral enable
        }

        template< typename T >
        bool operator()( T& dma_channel, uint8_t address, uint8_t * data, size_t size ) const {

            dma_channel.set_receive_buffer( data, size );
            scoped_dma_channel_enable dma_channel_enable( dma_channel );
            scoped_i2c_dma_enable dma_enable( _ ); // DMAEN set
            bitset::set( _.CR2, LAST );
            scoped_i2c_start start( _ );
            if ( start() ) { // generate start condition (master start)
                if ( i2c_address< Receiver >()( _, address ) ) {
                    i2c_address< Receiver >::clear(_);
                    if ( condition_waiter()( [&](){ return dma_channel.transfer_complete(); } ) ) {
                        return true;
                    }
                } else {
                    stream(__FILE__,__LINE__) << "i2c::dma_master_receiver address failed\n";
                }
            } else {
                stream(__FILE__,__LINE__) << "i2c::dma_master_receiver address failed\n";
            }
            return false;
        }
        
    };

}

bool
i2c::dma_transfer( uint8_t address, const uint8_t * data, size_t size )
{
    if ( ! i2c_ready_wait( *i2c_, own_addr_ )() ) {
        stream() << __FUNCTION__ << " i2c_ready_wait failed\n";
        return false;
    }
    
    auto base_addr = reinterpret_cast< uint32_t >( const_cast< I2C * >(i2c_) );

    if ( base_addr == I2C1_BASE && __dma_i2c1_tx != nullptr ) {

        return dma_master_transfer( *i2c_ )( *__dma_i2c1_tx, address, data, size );
        
    } else if ( base_addr == I2C2_BASE && __dma_i2c2_tx != nullptr ) {

        return dma_master_transfer( *i2c_ )( *__dma_i2c2_tx, address, data, size );
    }

    return false;
}

bool
i2c::dma_receive( uint8_t address, uint8_t * data, size_t size )
{
    const auto base_addr = reinterpret_cast< uint32_t >( const_cast< I2C * >(i2c_) );

    if ( base_addr == I2C1_BASE && __dma_i2c1_rx == nullptr ) {
        stream() << "I2C1 Rx DMA not attached\n";
        return false;
    } else if ( base_addr == I2C2_BASE && __dma_i2c2_rx == nullptr ) {
        stream() << "I2C2 Rx DMA not attached\n";
        return false;
    }    

    if ( ! i2c_ready_wait( *i2c_, own_addr_ )() ) {
        stream() << __FUNCTION__ << " i2c_ready_wait failed\n";
        return false;
    }
    
    if ( base_addr == I2C1_BASE ) {
        return dma_master_receiver( *i2c_ )( *__dma_i2c1_rx, address, data, size );
    } else if ( base_addr == I2C2_BASE ) {
        return dma_master_receiver( *i2c_ )( *__dma_i2c2_rx, address, data, size );
    }

    return false;
}

void
i2c::handle_event_interrupt()
{
    // stream() << "EVENT: " << status32_to_string( i2c_status( *i2c_ )() ) << std::endl;
    if ( i2c_->SR1 & RxNE ) {
        stream() << "\ti2c irq" << std::endl;
    } else {
        i2c_->CR2 &= ~ITEVTEN;
    }
}

void
i2c::handle_error_interrupt()
{
    stream() << "ERROR irq: " << status32_to_string( i2c_status( *i2c_ )() ) << std::endl;

    constexpr uint32_t error_condition = SMB_ALART | TIME_OUT | PEC_ERR | OVR | AF | ARLO | BERR;
    i2c_->SR1 &= ~error_condition;
}

//static
void
i2c::interrupt_event_handler( i2c * _this )
{
    _this->handle_event_interrupt();
}

// static
void
i2c::interrupt_error_handler( i2c * _this )
{
    _this->handle_error_interrupt();
}

