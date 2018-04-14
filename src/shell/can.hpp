// Copyright (C) 2018 MS-Cheminformatics LLC

#include <array>
#include <atomic>
#include <cstdint>
#include "scoped_spinlock.hpp"

//  CAN Master Control Register bits
enum CAN_MasterControlRegister {
    CAN_MCR_INRQ  =  (0x00000001)    /* Initialization request */
    , CAN_MCR_SLEEP =  (0x00000002)    /* Sleep mode request */
    , CAN_MCR_TXFP  =  (0x00000004)    /* Transmit FIFO priority */
    , CAN_MCR_RFLM  =  (0x00000008)    /* Receive FIFO locked mode */
    , CAN_MCR_NART  =  (0x00000010)    /* No automatic retransmission */
    , CAN_MCR_AWUM  =  (0x00000020)    /* Automatic wake up mode */
    , CAN_MCR_ABOM  =  (0x00000040)    /* Automatic bus-off management */
    , CAN_MCR_TTCM  =  (0x00000080)    /* Time triggered communication mode */
    , CAN_MCR_RESET =  (0x00008000)    /* bxCAN software master reset */
    , CAN_MCR_DBF   =  (0x00010000)    /* Debug freeze */
};

enum CAN_STATUS {
    CAN_OK = 0
    , CAN_INIT_FAILED
    , CAN_INIT_E_FAILED
    , CAN_INIT_L_FAILED
    , CAN_TX_FAILED
    , CAN_TX_PENDING
    , CAN_NO_MB
    , CAN_FILTER_FULL
};

enum CAN_FIFO {
    CAN_FIFO_0    = 0
    , CAN_FIFO_1  = 1
};

enum CAN_FILTER_MODE {
    CAN_FILTER_MASK  = 0
    , CAN_FILTER_LIST = 1
};

enum CAN_FILTER_SCALE {
	CAN_FILTER_32BIT	 = 0
	, CAN_FILTER_16BIT	 = 1
};

enum CAN_TX_MBX {
	CAN_TX_MBX0 = 0
	, CAN_TX_MBX1 = 1
	, CAN_TX_MBX2 = 2
	, CAN_TX_NO_MBX = CAN_NO_MB
};

struct CanMsg {
	uint32_t ID;		// CAN ID
	uint8_t IDE;		// CAN_ID_STD for standard and CAN_ID_EXT for extended
	uint8_t RTR;
	uint8_t DLC;
	uint8_t Data[8];
	uint8_t FMI;
    CanMsg() : ID(0), IDE(0), RTR(0), DLC(0), Data{ 0 }, FMI(0) {}
};

enum CAN_Identifier : uint32_t {
    CAN_ID_STD	 = 0x00 //  Standard Id
    , CAN_ID_EXT = 0x04
};

enum CAN_RemoteTransmissionRequest : uint8_t {
    CAN_RTR_DATA     = 0x00
    , CAN_RTR_REMOTE = 0x02
};

namespace stm32f103 {

    enum CAN_BASE : uint32_t;

    constexpr int CAN_RX_QUEUE_SIZE = 8;

    struct CAN;

    class can {
        volatile CAN * can_;

        CAN_STATUS status_;
        uint8_t rx_head_;
        uint8_t rx_tail_;
        uint8_t rx_count_;
        uint8_t rx_lost_;
        uint8_t active_;
        std::atomic< uint8_t > tx_status_[3];
        
        CanMsg rx_queue_[ CAN_RX_QUEUE_SIZE ];
        CAN_STATUS init_enter();
        CAN_STATUS init_leave();
        can();        
        template< CAN_BASE > friend struct can_t;
        CAN_STATUS init( stm32f103::CAN_BASE, uint32_t control = CAN_MCR_NART );

        void rx_read( CAN_FIFO fifo );
        void rx_release( CAN_FIFO fifo );
        CanMsg * read( CAN_FIFO fifo, CanMsg* msg );
        bool fifo_ready( CAN_FIFO fifo ) const;

    public:

        inline CAN_STATUS status() const { return status_; }

        // CAN_MCR_TTCM    time triggered communication mode
        //                      CAN_MCR_ABOM    automatic bus-off management
        //                      CAN_MCR_AWUM    automatic wake-up mode
        //                      CAN_MCR_NART    no automatic retransmission
        //                      CAN_MCR_RFLM    receive FIFO locked mode
        //                      CAN_MCR_TXFP    transmit FIFO priority

        CAN_STATUS set_silent_mode( bool );
        CAN_STATUS set_loopback_mode( bool );
        bool loopback_mode() const;
        bool silent_mode() const;
        
        CAN_STATUS filter( uint8_t filter_idx
                           , CAN_FIFO fifo = CAN_FIFO_0
                           , CAN_FILTER_SCALE scale = CAN_FILTER_32BIT
                           , CAN_FILTER_MODE mode = CAN_FILTER_MASK
                           , uint32_t fr1 = 0, uint32_t fr2 = 0 );

        CAN_TX_MBX transmit( CanMsg* msg );
        CAN_STATUS tx_status( CAN_TX_MBX mbx, uint32_t timeout = 0xffff );

        void cancel( uint8_t );

        uint8_t rx_available(void);
        void rx_queue_clear();
        void rx_queue_free();
        CanMsg * rx_queue_get();

        void handle_tx_interrupt();
        void handle_rx0_interrupt();
        void handle_rx1_interrupt();
        void handle_sce_interrupt();

        void print_registers();
    };

    template< CAN_BASE base > struct can_t {

        static std::atomic_flag once_flag_;
        static std::atomic_flag guard_;
        
        static void(*callback_)();

        static inline can * instance() {
            static can __instance;
            if ( !once_flag_.test_and_set() )
                __instance.init( base );            
            return &__instance;
        }

        static void set_callback( void (*cb)() ) {
            scoped_spinlock<> guard( guard_ );
            callback_ = cb;
        }

        static void clear_callback() {
            scoped_spinlock<> guard( guard_ );            
            callback_ = nullptr;
        }

        static bool callback() {
            scoped_spinlock<> guard( guard_ );
            if ( stm32f103::can_t< base >::callback_ ) {
                stm32f103::can_t< base >::callback_();
                return true;
            }
            return false;
        }
    };
    
    template< CAN_BASE base > std::atomic_flag can_t< base >::once_flag_;
    template< CAN_BASE base > std::atomic_flag can_t<base>::guard_;
    template< CAN_BASE base > void (*can_t<base>::callback_)();
}

