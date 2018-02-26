// Copyright (C) 2018 MS-Cheminformatics LLC

#include <cstdint>


enum CAN_STATUS {
    CAN_OK = 0,
    CAN_INIT_FAILED,
    CAN_INIT_E_FAILED,
    CAN_INIT_L_FAILED,
    CAN_TX_FAILED,
    CAN_TX_PENDING,
    CAN_NO_MB,
    CAN_FILTER_FULL
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
	CAN_TX_MBX0 = 0,
	CAN_TX_MBX1 = 1,
	CAN_TX_MBX2 = 2,
	CAN_TX_NO_MBX = CAN_NO_MB
};

struct CanMsg {
	uint32_t ID;		// CAN ID
	uint8_t IDE;		// CAN_ID_STD for standard and CAN_ID_EXT for extended
	uint8_t RTR;
	uint8_t DLC;
	uint8_t Data[8];
	uint8_t FMI;
};

enum CAN_Identifier : uint32_t {
    CAN_ID_STD	 = 0x00 //  Standard Id
    , CAN_ID_EXT = 0x04
};

enum CAN_RemoteTransmissionRequest : uint8_t {
    CAN_RTR_DATA = 0x00
    , CAN_RTR_REMOTE = 0x02
};

namespace stm32f103 {

    enum PERIPHERAL_BASE : uint32_t;

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
        CanMsg rx_queue_[ CAN_RX_QUEUE_SIZE ];
        CAN_STATUS init_enter();
        CAN_STATUS init_leave();

    public:
        can();

        inline CAN_STATUS status() const { return status_; }
        
        CAN_STATUS init( stm32f103::PERIPHERAL_BASE );
        CAN_STATUS set_mode( uint32_t );
        CAN_STATUS filter( uint8_t filter_idx, CAN_FIFO fifo, CAN_FILTER_SCALE scale, CAN_FILTER_MODE mode, uint32_t fr1, uint32_t fr2 );
        CAN_TX_MBX transmit( CanMsg* msg );
        CAN_STATUS tx_status( CAN_TX_MBX mbx );
        void cancel( uint8_t );
        uint8_t rx_available(void);
        void rx_read( CAN_FIFO fifo );
        void rx_release( CAN_FIFO fifo );
        void rx_queue_clear();
        void rx_queue_free();
        CanMsg * read( CAN_FIFO fifo, CanMsg* msg );
        CanMsg * rx_queue_get();
        static void rx_read( CAN *, CAN_FIFO fifo );
        static void rx_release( CAN *, CAN_FIFO fifo );        
    };

}

