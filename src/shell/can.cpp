// Copyright (C) 2018 MS-Cheminformatics LLC

#include "can.hpp"
#include "stream.hpp"
#include "stm32f103.hpp"
#include <cstdint>

// each I/O port registers have to be accessed as 32bit words. (reference manual pp158/1133)
// reference
// http://akb77.com/g/files/media/Maple.HardwareCAN.0.0.12.rar

extern "C" {
    void enable_interrupt( stm32f103::IRQn_type IRQn );
    void disable_interrupt( stm32f103::IRQn_type IRQn );

    void can1_tx_handler();
    void can1_rx0_handler();
}

namespace stm32f103 {
    constexpr uint32_t pclk1 = 36000000;

    static bool can_active;

    // CAN Master Control Register bits
    enum CAN_MasterControlRegister {
        CAN_MCR_INRQ	  = 0x00000001	// Initialization request
        , CAN_MCR_SLEEP	  = 0x00000002	// Sleep mode request
        , CAN_MCR_TXFP	  = 0x00000004	// Transmit FIFO priority
        , CAN_MCR_RFLM	  = 0x00000008	// Receive FIFO locked mode
        , CAN_MCR_NART	  = 0x00000010	// No automatic retransmission
        , CAN_MCR_AWUM	  = 0x00000020	// Automatic wake up mode
        , CAN_MCR_ABOM	  = 0x00000040	// Automatic bus-off management
        , CAN_MCR_TTCM	  = 0x00000080	// Time triggered communication mode
        , CAN_MCR_RESET	  = 0x00008000	// bxCAN software master reset
        , CAN_MCR_DBF	  = 0x00010000	// Debug freeze
    };

    enum CAN_MasterStatusRegister {
        CAN_MSR_INAK	= 0x00000001	// Initialization acknowledge
        , CAN_MSR_SLAK	= 0x00000002	// Sleep acknowledge
        , CAN_MSR_ERRI	= 0x00000004	// Error interrupt
        , CAN_MSR_WKUI	= 0x00000008	// Wake-up interrupt
        , CAN_MSR_SLAKI	= 0x00000010	// Sleep acknowledge interrupt
        , CAN_MSR_TXM	= 0x00000100	// Transmit mode
        , CAN_MSR_RXM	= 0x00000200	// Receive mode
        , CAN_MSR_SAMP	= 0x00000400	// Last sample point
        , CAN_MSR_RX	= 0x00000800	// CAN Rx signal
    };

    // CAN Transmit Status Register bits
    enum CAN_TransmitStatusRegister {
        CAN_TSR_RQCP0		= 0x00000001    // Request completed mailbox0 
        , CAN_TSR_TXOK0		= 0x00000002    // Transmission OK of mailbox0 
        , CAN_TSR_ALST0		= 0x00000004	// Arbitration Lost for Mailbox0 
        , CAN_TSR_TERR0		= 0x00000008	// Transmission Error of Mailbox0 
        , CAN_TSR_ABRQ0		= 0x00000080    // Abort request for mailbox0 
        , CAN_TSR_RQCP1		= 0x00000100    // Request completed mailbox1 
        , CAN_TSR_TXOK1		= 0x00000200    // Transmission OK of mailbox1 
        , CAN_TSR_ALST1		= 0x00000400	// Arbitration Lost for Mailbox1 
        , CAN_TSR_TERR1		= 0x00000800	// Transmission Error of Mailbox1 
        , CAN_TSR_ABRQ1		= 0x00008000    // Abort request for mailbox1 
        , CAN_TSR_RQCP2		= 0x00010000    // Request completed mailbox2 
        , CAN_TSR_TXOK2		= 0x00020000    // Transmission OK of mailbox2 
        , CAN_TSR_ALST2		= 0x00040000	// Arbitration Lost for mailbox 2 
        , CAN_TSR_TERR2		= 0x00080000	// Transmission Error of Mailbox 2 
        , CAN_TSR_ABRQ2		= 0x00800000    // Abort request for mailbox2 
        , CAN_TSR_CODE		= 0x03000000	// Mailbox Code 
        , CAN_TSR_TME		= 0x1C000000	// TME[2:0] bits 
        , CAN_TSR_TME0		= 0x04000000    // Transmit mailbox 0 empty 
        , CAN_TSR_TME1		= 0x08000000    // Transmit mailbox 1 empty 
        , CAN_TSR_TME2		= 0x10000000    // Transmit mailbox 2 empty 
        , CAN_TSR_LOW		= 0xE0000000	// LOW[2:0] bits 
        , CAN_TSR_LOW0		= 0x20000000	// Lowest Priority Flag for Mailbox 0 
        , CAN_TSR_LOW1		= 0x40000000	// Lowest Priority Flag for Mailbox 1 
        , CAN_TSR_LOW2		= 0x80000000	// Lowest Priority Flag for Mailbox 2
    };

    enum CAN_ReceiveFIFO_0_Register {
        CAN_RF0R_FMP0		= 0x00000003    // FIFO 0 message pending 
        , CAN_RF0R_FULL0		= 0x00000008    // FIFO 0 full 
        , CAN_RF0R_FOVR0		= 0x00000010    // FIFO 0 overrun 
        , CAN_RF0R_RFOM0		= 0x00000020    // Release FIFO 0 output mailbox 
    };

    enum CAN_ReceiveFIFO_1_Register {
        CAN_RF1R_FMP1		= 0x00000003    // FIFO 1 message pending 
        , CAN_RF1R_FULL1		= 0x00000008    // FIFO 1 full 
        , CAN_RF1R_FOVR1		= 0x00000010    // FIFO 1 overrun 
        , CAN_RF1R_RFOM1		= 0x00000020    // Release FIFO 1 output mailbox 
    };


    enum CAN_InterruptEnableRegister {
        CAN_IER_TMEIE		= 0x00000001	// Transmit Mailbox Empty Interrupt Enable 
        , CAN_IER_FMPIE0	= 0x00000002	// FIFO Message Pending Interrupt Enable 
        , CAN_IER_FFIE0		= 0x00000004	// FIFO Full Interrupt Enable 
        , CAN_IER_FOVIE0	= 0x00000008	// FIFO Overrun Interrupt Enable 
        , CAN_IER_FMPIE1	= 0x00000010	// FIFO Message Pending Interrupt Enable 
        , CAN_IER_FFIE1		= 0x00000020	// FIFO Full Interrupt Enable 
        , CAN_IER_FOVIE1	= 0x00000040	// FIFO Overrun Interrupt Enable 
        , CAN_IER_EWGIE		= 0x00000100	// Error Warning Interrupt Enable 
        , CAN_IER_EPVIE		= 0x00000200	// Error Passive Interrupt Enable 
        , CAN_IER_BOFIE		= 0x00000400	// Bus-Off Interrupt Enable 
        , CAN_IER_LECIE		= 0x00000800	// Last Error Code Interrupt Enable 
        , CAN_IER_ERRIE		= 0x00008000	// Error Interrupt Enable 
        , CAN_IER_WKUIE		= 0x00010000	// Wakeup Interrupt Enable 
        , CAN_IER_SLKIE		= 0x00020000	// Sleep Interrupt Enable 
    };

    enum CAN_ErrorStatusRegister { // (CAN_ESR)
        CAN_ESR_EWGF		= 0x00000001	// Error Warning Flag 
        , CAN_ESR_EPVF		= 0x00000002	// Error Passive Flag 
        , CAN_ESR_BOFF		= 0x00000004	// Bus-Off Flag 
        , CAN_ESR_LEC		= 0x00000070	// LEC[2:0] bits (Last Error Code) 
        , CAN_ESR_LEC_0		= 0x00000010	// Bit 0 
        , CAN_ESR_LEC_1		= 0x00000020	// Bit 1 
        , CAN_ESR_LEC_2		= 0x00000040	// Bit 2 
        , CAN_ESR_TEC		= 0x00FF0000	// Least significant byte of the 9-bit Transmit Error Counter 
        , CAN_ESR_REC		= 0xFF000000	// Receive Error Counter 
    };

    enum {
        CAN_BTR_SJW_POS	= 24
        , CAN_BTR_TS2_POS	= 20
        , CAN_BTR_TS1_POS	= 16
        
        , CAN_BTR_BRP		= 0x000003FF	/* Baud Rate Prescaler */
        , CAN_BTR_TS1 	    = 0x000F0000	/* Time Segment 1 */
        , CAN_BTR_TS2 	    = 0x00700000	/* Time Segment 2 */
        , CAN_BTR_SJW 	    = 0x03000000	/* Resynchronization Jump Width */
        , CAN_BTR_LBKM	    = 0x40000000	/* Loop Back Mode (Debug) */
        , CAN_BTR_SILM	    = 0x80000000	/* Silent Mode */        
        
        , CAN_INAK_TimeOut	= 0x0000FFFF
        , CAN_SLAK_TimeOut	= 0x0000FFFF
        , CAN_CONTROL_MASK	= (CAN_MCR_TTCM | CAN_MCR_ABOM | CAN_MCR_AWUM | CAN_MCR_NART | CAN_MCR_RFLM | CAN_MCR_TXFP)
        , CAN_TIMING_MASK	= (CAN_BTR_SJW | CAN_BTR_TS2 | CAN_BTR_TS1 | CAN_BTR_BRP)
        , CAN_MODE_MASK		= (CAN_BTR_LBKM | CAN_BTR_SILM)
    };

    enum CAN_MailboxTransmitReauest {
        CAN_TMIDxR_TXRQ	= 0x00000001 /* Transmit mailbox request */
    };

    enum CAN_FilterMasterRegister {
        CAN_FMR_FINIT	= 0x00000001 /* Filter init mode */
    };

    constexpr const char * __register_names [] = {
        "MCR", "MSR", "TSR", "RF0R", "RF1R", "IER", "ESR", "BTR"
        , "TI0R", "TDT0R", "TDL0R", "TDH0R"
        , "TI1R", "TDT1R", "TDL1R", "TDH1R"
        , "TI2R", "TDT2R", "TDL2R", "TDH2R"
        , "RI0R", "RDT0R", "RDT0R", "RDL0R"
        , "RI1R", "RDT1R", "RDT1R", "RDL1R"
        , "RI2R", "RDT2R", "RDT2R", "RDL2R"
    };

}

using namespace stm32f103;

can::can() : status_( CAN_INIT_FAILED )
           , rx_head_( 0 )
           , rx_tail_( 0 )
           , rx_count_( 0 )
           , active_( 0 )
{
    can_active = 0;
}

CAN_STATUS
can::init_enter()
{
	volatile uint32_t wait_ack;

	status_ = CAN_OK;
	if ((can_->MSR & CAN_MSR_INAK) == 0) {	// Check for initialization mode already set
		can_->MCR |= CAN_MCR_INRQ;			// Request initialisation

		wait_ack = 0;						// Wait the acknowledge
		while ((wait_ack != CAN_INAK_TimeOut) && (( can_->MSR & CAN_MSR_INAK) == 0))
			wait_ack++;
		if (( can_->MSR & CAN_MSR_INAK) == 0)
			status_ = CAN_INIT_E_FAILED;		// Timeout
	}
	return status_;
}

CAN_STATUS
can::init_leave()
{
	volatile uint32_t wait_ack;

	status_ = CAN_OK;
	if ((can_->MSR & CAN_MSR_INAK) != 0) {	// Check for initialization mode already reset
		can_->MCR &= ~CAN_MCR_INRQ;			// Request initialization
        
		wait_ack = 0;						// Wait the acknowledge
		while ((wait_ack != CAN_INAK_TimeOut) && ((can_->MSR & CAN_MSR_INAK) != 0))
			wait_ack++;
		if ((can_->MSR & CAN_MSR_INAK) != 0)
			status_ = CAN_INIT_L_FAILED;
	}
	return status_;
}


CAN_STATUS
can::init( stm32f103::CAN_BASE base, uint32_t control )
{
    status_ = CAN_INIT_FAILED;
    rx_head_ = rx_tail_ = rx_count_ = rx_lost_ = 0;
    
    if ( auto CAN = reinterpret_cast< volatile stm32f103::CAN * >( base ) ) {
        can_ = CAN;
        can_->MCR &= ~CAN_MCR_SLEEP;
        
        stream() << "can::init" << std::endl;

        //rcc_clk_enable(RCC_AFIO);                       // enable clocks for AFIO
        //rcc_clk_enable(RCC_CAN);                        // and CAN
        //rcc_reset_dev(RCC_CAN);                         // reset CAN interface

        can_active = 1;                                         // set CAN active flag (for interrupt handler

        can_->MCR &= ~CAN_MCR_SLEEP;            // reset CAN sleep mode (default after reset)

        if ( init_enter() != CAN_OK)            // enter CAN initialization mode
            return status_;                              // error, so return

        can_->MCR &= ~CAN_CONTROL_MASK;         // set mode bits
        can_->MCR |= (control & CAN_CONTROL_MASK);

        can_->BTR &= ~CAN_TIMING_MASK;               // Set the bit timing register
        // can_->BTR |= (can_speed_table[speed].btr & CAN_TIMING_MASK);
        uint32_t brp = ( pclk1 / 18 ) / 500000; // 500kbps
        can_->BTR |=  ((((4-1) & 0x03) << 24) | (((5-1) & 0x07) << 20) | (((12-1) & 0x0F) << 16) | ((brp-1) & 0x1FF));
        
        can_->IER = (CAN_IER_FMPIE0 | CAN_IER_FMPIE1 | CAN_IER_TMEIE);

        if ( init_leave() == CAN_OK)  {
            while (!(can_->TSR & CAN_TSR_TME0));    // Transmit mailbox 0 is empty
            while (!(can_->TSR & CAN_TSR_TME1));    // Transmit mailbox 0 is empty
            while (!(can_->TSR & CAN_TSR_TME2));    // Transmit mailbox 0 is empty
        }
        enable_interrupt( stm32f103::CAN1_TX_IRQn );
        enable_interrupt( stm32f103::CAN1_RX0_IRQn );
    }
    return status_;
}

CAN_STATUS
can::set_mode( uint32_t mode )
{
	if ( init_enter() == CAN_OK)	{
		can_->BTR &= ~CAN_MODE_MASK;
		can_->BTR |= (mode & CAN_MODE_MASK);
		init_leave();
	}
    return status_;
}

CAN_STATUS
can::filter( uint8_t filter_idx, CAN_FIFO fifo, CAN_FILTER_SCALE scale, CAN_FILTER_MODE mode, uint32_t fr1, uint32_t fr2 )
{
	uint32_t mask = 0x00000001 << filter_idx;

	can_->FMR |= CAN_FMR_FINIT;		// Initialization mode for the filter
	can_->FA1R &= ~mask;			// Deactivation filter

	if (scale == CAN_FILTER_32BIT)
		can_->FS1R |= mask;
	else
		can_->FS1R &= ~mask;

	can_->filterRegister[filter_idx].FR1 = fr1;
	can_->filterRegister[filter_idx].FR2 = fr2;

	if (mode == CAN_FILTER_MASK)
		can_->FM1R &= ~mask;
	else
		can_->FM1R |= mask;

	if (fifo == CAN_FIFO_0)
		can_->FFA1R &= ~mask;
	else
		can_->FFA1R |= mask;

	can_->FA1R |= mask;
	can_->FMR &= ~CAN_FMR_FINIT;
	return CAN_OK;
}

CAN_TX_MBX
can::transmit( CanMsg * msg )
{
	CAN_TX_MBX mbx;
	uint32_t data;

    // stream() << "can::transmit" << std::endl;    

	/* Select one empty transmit mailbox */
	if (can_->TSR & CAN_TSR_TME0)
		mbx = CAN_TX_MBX0;
	else if (can_->TSR & CAN_TSR_TME1)
		mbx = CAN_TX_MBX1;
	else if (can_->TSR & CAN_TSR_TME2)
		mbx = CAN_TX_MBX2;
	else
	{
		status_ = CAN_NO_MB;
		return CAN_TX_NO_MBX;
	}

    /* Set up the Id */
    if (msg->IDE == CAN_ID_STD)
		data = (msg->ID << 21);
    else
		data = (msg->ID << 3) | CAN_ID_EXT;

	data |= msg->RTR;

	/* Set up the DLC */
    can_->txMailBox[mbx].TDTR = msg->DLC & 0x0F;

    /* Set up the data field */
    can_->txMailBox[mbx].TDLR = 
		uint32_t( msg->Data[3] ) << 24 | 
		uint32_t( msg->Data[2] ) << 16 |
		uint32_t( msg->Data[1] ) << 8 | 
		uint32_t( msg->Data[0] );

    can_->txMailBox[mbx].TDHR = 
		uint32_t( msg->Data[7] ) << 24 |
		uint32_t( msg->Data[6] ) << 16 |
		uint32_t( msg->Data[5] ) << 8 |
		uint32_t( msg->Data[4] );
        
    /* Request transmission */
        can_->txMailBox[mbx].TIR = (data | CAN_TMIDxR_TXRQ);
        status_ = CAN_OK;

	return mbx;
}

CAN_STATUS
can::tx_status( CAN_TX_MBX mbx )
{
	/* RQCP, TXOK and TME bits */
	uint8_t state = 0;

	switch (mbx) {
	case CAN_TX_MBX0:
		state |= (uint8_t)((can_->TSR & CAN_TSR_RQCP0) << 2);
		state |= (uint8_t)((can_->TSR & CAN_TSR_TXOK0) >> 0);
		state |= (uint8_t)((can_->TSR & CAN_TSR_TME0) >> 26);
		break;
	case CAN_TX_MBX1:
		state |= (uint8_t)((can_->TSR & CAN_TSR_RQCP1) >> 6);
		state |= (uint8_t)((can_->TSR & CAN_TSR_TXOK1) >> 8);
		state |= (uint8_t)((can_->TSR & CAN_TSR_TME1) >> 27);
		break;
	case CAN_TX_MBX2:
		state |= (uint8_t)((can_->TSR & CAN_TSR_RQCP2) >> 14);
		state |= (uint8_t)((can_->TSR & CAN_TSR_TXOK2) >> 16);
		state |= (uint8_t)((can_->TSR & CAN_TSR_TME2) >> 28);
		break;
	default:
		status_ = CAN_TX_FAILED;
		return status_;
	}

	// state = RQCP TXOK TME
	switch ( state ) {
	/* transmit pending  */
	case 0x0:
		status_ = CAN_TX_PENDING;
		break;
	/* transmit failed  */
	case 0x5:
		status_ = CAN_TX_FAILED;
		break;
	/* transmit succedeed  */
	case 0x7:
		status_ = CAN_OK;
		break;
	default:
		status_ = CAN_TX_FAILED;
		break;
	}
	return status_;
}

void
can::cancel( uint8_t mbx )
{
	/* abort transmission */
	switch (mbx) {
	case 0:
		can_->TSR |= CAN_TSR_ABRQ0;
		break;
	case 1:
		can_->TSR |= CAN_TSR_ABRQ1;
		break;
	case 2:
		can_->TSR |= CAN_TSR_ABRQ2;
		break;
	default:
		break;
	}
}

void
can::rx_queue_clear()
{
	// nvic_irq_disable(NVIC_USB_LP_CAN_RX0);
	rx_head_ = rx_tail_ = rx_count_ = rx_lost_ = 0;
	// nvic_irq_enable(NVIC_USB_LP_CAN_RX0);
}

uint8_t
can::rx_available(void)
{
	return rx_count_;
}

CanMsg *
can::rx_queue_get(void)
{
	if (rx_count_ == 0)
		return nullptr;
	return &( rx_queue_[ rx_tail_ ] );
}

void
can::rx_queue_free()
{
	if ( rx_count_ > 0 ) {
		rx_tail_ = ( rx_tail_ == (CAN_RX_QUEUE_SIZE - 1)) ? 0 : (rx_tail_ + 1);
		--rx_count_;
	}
}

CanMsg*
can::read( CAN_FIFO fifo, CanMsg* msg )
{
	uint32_t data = can_->fifoMailBox[fifo].RIR;

	/* Get the Id */
	if (data & CAN_ID_EXT)	{
		msg->ID = 0x1FFFFFFF & (data >> 3);
		msg->IDE = CAN_ID_EXT;
	}	else	{
		msg->ID = 0x000007FF & (data >> 21);
		msg->IDE = CAN_ID_STD;
	}

	msg->RTR = CAN_RTR_REMOTE & data;
	msg->DLC = 0x0F & can_->fifoMailBox[fifo].RDTR;
	msg->FMI = 0xFF & (can_->fifoMailBox[fifo].RDTR >> 8);

	/* Get the data field */
	data = can_->fifoMailBox[fifo].RDLR;
	uint8_t * p = msg->Data;
	*p++ = uint8_t(0xFF & data);
	*p++ = uint8_t(0xFF & (data >> 8));
	*p++ = uint8_t(0xFF & (data >> 16));
	*p++ = uint8_t(0xFF & (data >> 24));

	data = can_->fifoMailBox[fifo].RDHR;
	*p++ = uint8_t(0xFF & data);
    *p++ = uint8_t(0xFF & (data >> 8));
	*p++ = uint8_t(0xFF & (data >> 16));
	*p++ = uint8_t(0xFF & (data >> 24));

	return msg;
}

void
can::rx_release( CAN_FIFO fifo )
{
	if ( fifo == CAN_FIFO_0 )
		can_->RF0R |= (CAN_RF0R_RFOM0);	// Release FIFO0
	else
		can_->RF1R |= (CAN_RF1R_RFOM1);	// Release FIFO1
}

void
can::rx_read( CAN_FIFO fifo )
{
	if ( rx_count_ < CAN_RX_QUEUE_SIZE ) {	// read the message
		CanMsg * msg = &rx_queue_[rx_head_];
		read( fifo, msg);
		rx_head_ = (rx_head_ == (CAN_RX_QUEUE_SIZE - 1)) ? 0 : (rx_head_ + 1);
		rx_count_++;
	} else
		rx_lost_ = 1;						// no place in queue, ignore package

	rx_release(fifo);
}

void
can::rx_read( CAN * can, CAN_FIFO fifo )
{
    // if ( __instance && __instance->can_ == can )
    //     __instance->rx_read( fifo );
}



void
can::handle_rx0_interrupt()
{
    stream() << "can::rx0_handler" << std::endl;
    
    if ( can_active ) {
        if ( auto CAN = reinterpret_cast< stm32f103::CAN * >( stm32f103::CAN1_BASE ) ) {
            while ((CAN->RF0R & CAN_RF0R_FMP0) != 0)
                can::rx_read( CAN, CAN_FIFO_0 );		// message pending FIFO0
            while ((CAN->RF1R & CAN_RF1R_FMP1) != 0)
                can::rx_read(CAN, CAN_FIFO_1);		// message pending FIFO1
        }
    }
}

void
can::handle_tx_interrupt()
{
    stream() << "can::tx_handler" << std::endl;

    if ( can_active ) {
        if ( auto CAN = reinterpret_cast< stm32f103::CAN * >( stm32f103::CAN1_BASE ) ) {    
             	if (CAN->TSR & CAN_TSR_RQCP0)
                    CAN->TSR |= CAN_TSR_RQCP0;		// reset request complete mbx 0
                if (CAN->TSR & CAN_TSR_RQCP1)
                    CAN->TSR |= CAN_TSR_RQCP1;		// reset request complete mbx 1
                if (CAN->TSR & CAN_TSR_RQCP2)
                    CAN->TSR |= CAN_TSR_RQCP2;		// reset request complete mbx 2
        }
    }
}

void
can::print_registers()
{
    auto reg = reinterpret_cast< volatile uint32_t * >( can_ );
    for ( size_t i = 0; i < sizeof( __register_names ) / sizeof( __register_names[ 0 ] ); ++i )
        stream() << __register_names[ i ] << ": " << *reg++ << std::endl;
}
