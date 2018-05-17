#include <common/ctor.h>
#include <hal.h>
#include <modules/can/can_driver.h>

#if !defined(CAN1) && defined(CAN)
#define CAN1 CAN
#endif

#undef CAN_BTR_BRP
#define CAN_BTR_BRP(n) (n)
#undef CAN_BTR_TS1
#define CAN_BTR_TS1(n) ((n) << 16)
#undef CAN_BTR_TS2
#define CAN_BTR_TS2(n) ((n) << 20)
#undef CAN_BTR_SJW
#define CAN_BTR_SJW(n) ((n) << 24)

#define NUM_TX_MAILBOXES 3
#define NUM_RX_MAILBOXES 2
#define RX_FIFO_DEPTH 3

static void can_driver_stm32_start(void* ctx, bool silent, bool auto_retransmit, uint32_t baudrate);
static void can_driver_stm32_stop(void* ctx);
bool can_driver_stm32_abort_tx_mailbox_I(void* ctx, uint8_t mb_idx);
bool can_driver_stm32_load_tx_mailbox_I(void* ctx, uint8_t mb_idx, struct can_frame_s* frame);

static const struct can_driver_iface_s can_driver_stm32_iface = {
    can_driver_stm32_start,
    can_driver_stm32_stop,
    can_driver_stm32_abort_tx_mailbox_I,
    can_driver_stm32_load_tx_mailbox_I,
};

struct can_driver_stm32_instance_s {
    struct can_instance_s* frontend;
    CAN_TypeDef* can;
};

static struct can_driver_stm32_instance_s can_instance[2];

RUN_ON(CAN_INIT) {
    // TODO make this index configurable and enable multiple instances
    can_instance[0].can = CAN1;
    can_instance[1].can = CAN2;
    can_instance[0].frontend = can_driver_register(0, &can_instance[0], &can_driver_stm32_iface, NUM_TX_MAILBOXES, NUM_RX_MAILBOXES, RX_FIFO_DEPTH);
    can_instance[1].frontend = can_driver_register(1, &can_instance[1], &can_driver_stm32_iface, NUM_TX_MAILBOXES, NUM_RX_MAILBOXES, RX_FIFO_DEPTH);
}

static void can_driver_stm32_start(void* ctx, bool silent, bool auto_retransmit, uint32_t baudrate) {
    struct can_driver_stm32_instance_s* instance = ctx;

    if (instance == &can_instance[1]) {
        rccEnableCAN2(FALSE);
    } else if (instance == &can_instance[0]) {
        rccEnableCAN1(FALSE);
    }

    instance->can->MCR &= ~CAN_MCR_SLEEP; // Exit sleep mode
    instance->can->MCR |= CAN_MCR_INRQ;   // Request init

    instance->can->IER = 0;                  // Disable interrupts while initialization is in progress

    if (instance == &can_instance[1]) {
        nvicEnableVector(STM32_CAN2_TX_NUMBER, STM32_CAN_CAN2_IRQ_PRIORITY);
        nvicEnableVector(STM32_CAN2_RX0_NUMBER, STM32_CAN_CAN2_IRQ_PRIORITY);
        nvicEnableVector(STM32_CAN2_SCE_NUMBER, STM32_CAN_CAN2_IRQ_PRIORITY);
    } else {
        nvicEnableVector(STM32_CAN1_TX_NUMBER, STM32_CAN_CAN1_IRQ_PRIORITY);
        nvicEnableVector(STM32_CAN1_RX0_NUMBER, STM32_CAN_CAN1_IRQ_PRIORITY);
        nvicEnableVector(STM32_CAN1_SCE_NUMBER, STM32_CAN_CAN1_IRQ_PRIORITY);
    }

    while((instance->can->MSR & CAN_MSR_INAK) == 0) {
        __asm__("nop");
    }

    // Adapted from libcanard's canardSTM32ComputeCANTimings
    uint8_t bs1;
    uint8_t bs2;
    uint32_t prescaler;

    {
        const uint8_t max_quanta_per_bit = (baudrate >= 1000000) ? 10 : 17;
        const uint32_t prescaler_bs = STM32_PCLK1 / baudrate;

        uint8_t bs1_bs2_sum = (uint8_t)(max_quanta_per_bit - 1);

        // Search for the highest valid prescalar value
        while ((prescaler_bs % (1 + bs1_bs2_sum)) != 0) {
            if (bs1_bs2_sum <= 2) {
                return;
            }
            bs1_bs2_sum--;
        }

        prescaler = prescaler_bs / (1 + bs1_bs2_sum);
        if (prescaler < 1 || prescaler > 1024) {
            return;
        }

        // The recommended sample point location is 87.5% or 7/8. Compute the values of BS1 and BS2 that satisfy BS1+BS2 == bs1_bs2_sum and minimize ((1+BS1)/(1+BS1/BS2) - 7/8)
        bs1 = ((7 * bs1_bs2_sum - 1) + 4) / 8;

        // Check sample point constraints
        const uint16_t max_sample_point_per_mille = 900;
        const uint16_t min_sample_point_per_mille = (baudrate >= 1000000) ? 750 : 850;

        if (1000 * (1 + bs1) / (1 + bs1_bs2_sum) >= max_sample_point_per_mille) {
            bs1--;
        }

        if (1000 * (1 + bs1) / (1 + bs1_bs2_sum) < min_sample_point_per_mille) {
            bs1++;
        }

        if (1000 * (1 + bs1) / (1 + bs1_bs2_sum) >= max_sample_point_per_mille) {
            return;
        }

        bs2 = bs1_bs2_sum-bs1;
    }

    instance->can->BTR = (silent?CAN_BTR_SILM:0) | CAN_BTR_SJW(0) | CAN_BTR_TS1(bs1-1) | CAN_BTR_TS2(bs2-1) | CAN_BTR_BRP(prescaler - 1);

    instance->can->MCR = CAN_MCR_ABOM | CAN_MCR_AWUM | (auto_retransmit?0:CAN_MCR_NART);

    instance->can->IER = CAN_IER_TMEIE | CAN_IER_FMPIE0; // TODO: review reference manual for other interrupt flags needed

    if (instance == &can_instance[0]) { 
        uint32_t fmr = can_instance[0].can->FMR & 0xFFFFC0F1;
        fmr |= 14UL << 8;
        can_instance[0].can->FMR = fmr | CAN_FMR_FINIT;
        can_instance[0].can->sFilterRegister[0].FR1 = 0;
        can_instance[0].can->sFilterRegister[0].FR2 = 0;
        can_instance[0].can->sFilterRegister[14].FR1 = 0;
        can_instance[0].can->sFilterRegister[14].FR2 = 0;
        can_instance[0].can->FM1R = 0;
        can_instance[0].can->FFA1R = 0;
        can_instance[0].can->FS1R = 0x7ffffffUL;
        can_instance[0].can->FA1R = 1UL | (1UL << 14);
        can_instance[0].can->FMR &= ~CAN_FMR_FINIT;
    }
}

static void can_driver_stm32_stop(void* ctx) {
    struct can_driver_stm32_instance_s* instance = ctx;

    instance->can->MCR = 0x00010002;
    instance->can->IER = 0x00000000;
    if (instance == &can_instance[1]) {
        nvicDisableVector(STM32_CAN2_TX_NUMBER);
        nvicDisableVector(STM32_CAN2_RX0_NUMBER);
        nvicDisableVector(STM32_CAN2_SCE_NUMBER);
        rccDisableCAN2(FALSE);
    } else {
        nvicDisableVector(STM32_CAN1_TX_NUMBER);
        nvicDisableVector(STM32_CAN1_RX0_NUMBER);
        nvicDisableVector(STM32_CAN1_SCE_NUMBER);
        rccDisableCAN1(FALSE);
    }
}

bool can_driver_stm32_abort_tx_mailbox_I(void* ctx, uint8_t mb_idx) {
    struct can_driver_stm32_instance_s* instance = ctx;

    chDbgCheckClassI();

    switch(mb_idx) {
        case 0:
            instance->can->TSR = CAN_TSR_ABRQ0;
            return true;
        case 1:
            instance->can->TSR = CAN_TSR_ABRQ1;
            return true;
        case 2:
            instance->can->TSR = CAN_TSR_ABRQ2;
            return true;
    }
    return false;
}

bool can_driver_stm32_load_tx_mailbox_I(void* ctx, uint8_t mb_idx, struct can_frame_s* frame) {
    struct can_driver_stm32_instance_s* instance = ctx;

    chDbgCheckClassI();

    CAN_TxMailBox_TypeDef* mailbox = &instance->can->sTxMailBox[mb_idx];

    mailbox->TDTR = frame->DLC;
    mailbox->TDLR = frame->data32[0];
    mailbox->TDHR = frame->data32[1];

    if (frame->IDE) {
        mailbox->TIR = ((uint32_t)frame->EID << 3) | (frame->RTR ? CAN_TI0R_RTR : 0) | CAN_TI0R_IDE | CAN_TI0R_TXRQ;
    } else {
        mailbox->TIR = ((uint32_t)frame->SID << 21) | (frame->RTR ? CAN_TI0R_RTR : 0) | CAN_TI0R_TXRQ;
    }

    return true;
}

static void can_driver_stm32_retreive_rx_frame_I(struct can_frame_s* frame, CAN_FIFOMailBox_TypeDef* mailbox) {
    frame->data32[0] = mailbox->RDLR;
    frame->data32[1] = mailbox->RDHR;
    frame->RTR = (mailbox->RIR & CAN_RI0R_RTR) != 0;
    frame->IDE = (mailbox->RIR & CAN_RI0R_IDE) != 0;
    if (frame->IDE) {
        frame->EID = (mailbox->RIR & (CAN_RI0R_STID|CAN_RI0R_EXID)) >> 3;
    } else {
        frame->SID = (mailbox->RIR & CAN_RI0R_STID) >> 21;
    }
    frame->DLC = mailbox->RDTR & CAN_RDT0R_DLC;
}

static void stm32_can_rx_handler(struct can_driver_stm32_instance_s* instance) {
    systime_t rx_systime = chVTGetSystemTimeX();
    while (true) {
        chSysLockFromISR();
        if ((instance->can->RF0R & CAN_RF0R_FMP0) == 0) {
            chSysUnlockFromISR();
            break;
        }
        struct can_frame_s frame;
        can_driver_stm32_retreive_rx_frame_I(&frame, &instance->can->sFIFOMailBox[0]);
        can_driver_rx_frame_received_I(instance->frontend, 0, rx_systime, &frame);
        instance->can->RF0R = CAN_RF0R_RFOM0;
        chSysUnlockFromISR();
    }

    while (true) {
        chSysLockFromISR();
        if ((instance->can->RF1R & CAN_RF1R_FMP1) == 0) {
            chSysUnlockFromISR();
            break;
        }
        struct can_frame_s frame;
        can_driver_stm32_retreive_rx_frame_I(&frame, &instance->can->sFIFOMailBox[1]);
        can_driver_rx_frame_received_I(instance->frontend, 1, rx_systime, &frame);
        instance->can->RF1R = CAN_RF1R_RFOM1;
        chSysUnlockFromISR();
    }
}

static void stm32_can_tx_handler(struct can_driver_stm32_instance_s* instance) {
    systime_t t_now = chVTGetSystemTimeX();

    chSysLockFromISR();
    if ((instance->can->TSR & CAN_TSR_RQCP0) != 0) {
        can_driver_tx_request_complete_I(instance->frontend, 0, (instance->can->TSR & CAN_TSR_TXOK0) != 0, t_now);
        instance->can->TSR = CAN_TSR_RQCP0;
    }

    if ((instance->can->TSR & CAN_TSR_RQCP1) != 0) {
        can_driver_tx_request_complete_I(instance->frontend, 1, (instance->can->TSR & CAN_TSR_TXOK1) != 0, t_now);
        instance->can->TSR = CAN_TSR_RQCP1;
    }

    if ((instance->can->TSR & CAN_TSR_RQCP2) != 0) {
        can_driver_tx_request_complete_I(instance->frontend, 2, (instance->can->TSR & CAN_TSR_TXOK2) != 0, t_now);
        instance->can->TSR = CAN_TSR_RQCP2;
    }
    chSysUnlockFromISR();
}

OSAL_IRQ_HANDLER(STM32_CAN1_TX_HANDLER) {
    OSAL_IRQ_PROLOGUE();

    stm32_can_tx_handler(&can_instance[0]);

    OSAL_IRQ_EPILOGUE();
}

OSAL_IRQ_HANDLER(STM32_CAN1_RX0_HANDLER) {
    OSAL_IRQ_PROLOGUE();

    stm32_can_rx_handler(&can_instance[0]);

    OSAL_IRQ_EPILOGUE();
}


OSAL_IRQ_HANDLER(STM32_CAN2_TX_HANDLER) {
    OSAL_IRQ_PROLOGUE();

    stm32_can_tx_handler(&can_instance[1]);

    OSAL_IRQ_EPILOGUE();
}

OSAL_IRQ_HANDLER(STM32_CAN2_RX0_HANDLER) {
    OSAL_IRQ_PROLOGUE();

    stm32_can_rx_handler(&can_instance[1]);

    OSAL_IRQ_EPILOGUE();
}
