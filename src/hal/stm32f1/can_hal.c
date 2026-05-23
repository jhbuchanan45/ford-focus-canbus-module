/**
 * can_hal.c — STM32F103 bxCAN HAL at 125 kbps (MS-CAN)
 *
 * Pin assignment:
 *   CAN1 RX → PB8  (remapped via AFIO to avoid clash with USB on PA11)
 *   CAN1 TX → PB9
 *   TJA1042 transceiver SILENT pin → pull low (always enabled)
 *
 * Bit timing at 36 MHz APB1 (PCLK1):
 *   Prescaler = 18  → TQ = 500 ns
 *   TS1 = 13, TS2 = 2, SJW = 1  → 16 TQ/bit = 125 kbps
 *   Sample point = (1 + 13) / 16 = 87.5%
 *
 * Frames are stored in a small software FIFO so the Rx interrupt empties
 * the hardware FIFO quickly and the main loop drains the software FIFO.
 */

#include "../can_hal.h"

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/can.h>
#include <libopencm3/stm32/f1/nvic.h>
#include <libopencm3/cm3/nvic.h>

#include <string.h>

/* -------------------------------------------------------------------------
 * Software receive FIFO (interrupt → main loop)
 * ---------------------------------------------------------------------- */

#define SW_FIFO_SIZE 16u

typedef struct {
    uint32_t id;
    uint8_t  data[8];
    uint8_t  dlc;
} sw_frame_t;

static volatile sw_frame_t s_fifo[SW_FIFO_SIZE];
static volatile uint8_t s_head;
static volatile uint8_t s_tail;
static volatile uint8_t s_count;

static void fifo_push(uint32_t id, const uint8_t *data, uint8_t dlc)
{
    if (s_count == SW_FIFO_SIZE) return; /* drop on overflow */
    s_fifo[s_head].id  = id;
    s_fifo[s_head].dlc = dlc;
    memcpy((void *)s_fifo[s_head].data, data, dlc);
    s_head = (s_head + 1u) % SW_FIFO_SIZE;
    s_count++;
}

/* -------------------------------------------------------------------------
 * Initialisation
 * ---------------------------------------------------------------------- */

void can_hal_init(const uint32_t *acceptance_ids, uint8_t count)
{
    /* Enable clocks */
    rcc_periph_clock_enable(RCC_AFIO);
    rcc_periph_clock_enable(RCC_GPIOB);
    rcc_periph_clock_enable(RCC_CAN1);

    /* Remap CAN1 to PB8/PB9 (away from USB PA11/PA12) */
    AFIO_MAPR |= AFIO_MAPR_CAN1_REMAP_PORTB;

    /* PB8 = CAN_RX (input floating), PB9 = CAN_TX (alt function push-pull) */
    gpio_set_mode(GPIOB, GPIO_MODE_INPUT,  GPIO_CNF_INPUT_FLOAT,  GPIO8);
    gpio_set_mode(GPIOB, GPIO_MODE_OUTPUT_50_MHZ,
                  GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, GPIO9);

    /* Reset and configure bxCAN */
    can_reset(CAN1);

    /* Init mode, 125 kbps:  prescaler=18, ts1=13, ts2=2, sjw=1 */
    if (can_init(CAN1,
                 false,  /* TTCM */
                 true,   /* ABOM — auto bus-off recovery */
                 false,  /* AWUM */
                 false,  /* NART (auto-retransmit enabled) */
                 false,  /* RFLM (receive FIFO not locked) */
                 false,  /* TXFP */
                 CAN_BTR_SJW_1TQ,
                 CAN_BTR_TS1_13TQ,
                 CAN_BTR_TS2_2TQ,
                 18,     /* prescaler */
                 false,  /* loopback */
                 false   /* silent */
                 ) != 0) {
        /* Initialisation failed — stay in init loop for debug */
        while (1) {}
    }

    /* Hardware acceptance filters */
    if (acceptance_ids != NULL && count > 0) {
        /* Use 32-bit ID list mode, bank 0 onwards */
        uint8_t bank = 0;
        for (uint8_t i = 0; i < count; i += 2) {
            uint32_t id0 = (acceptance_ids[i] << 21);   /* 11-bit → bits[31:21] */
            uint32_t id1 = (i + 1 < count)
                         ? (acceptance_ids[i + 1] << 21)
                         : (acceptance_ids[i] << 21);   /* duplicate if odd count */

            can_filter_id_list_32bit_init(bank++, id0, id1, 0, true);
        }
    } else {
        /* Accept all: single mask filter that passes everything */
        can_filter_id_mask_32bit_init(0, 0, 0, 0, true);
    }

    /* Enable Rx FIFO 0 message-pending interrupt */
    nvic_enable_irq(NVIC_USB_LP_CAN_RX0_IRQ);
    nvic_set_priority(NVIC_USB_LP_CAN_RX0_IRQ, 1);
    can_enable_irq(CAN1, CAN_IER_FMPIE0);
}

/* -------------------------------------------------------------------------
 * Rx interrupt handler — drains hardware FIFO 0 into software FIFO
 * ---------------------------------------------------------------------- */

void usb_lp_can_rx0_isr(void)
{
    while (CAN_RF0R(CAN1) & CAN_RF0R_FMP0_MASK) {
        uint32_t id;
        bool ext, rtr;
        uint8_t fmi, dlc, data[8];
        uint16_t ts;

        can_receive(CAN1, 0,     /* FIFO 0 */
                    false,        /* don't release yet */
                    &id, &ext, &rtr, &fmi, &dlc, data, &ts);

        if (!ext && !rtr) {
            fifo_push(id, data, dlc);
        }

        can_fifo_release(CAN1, 0);
    }
}

/* -------------------------------------------------------------------------
 * Main-loop receive (non-blocking drain of software FIFO)
 * ---------------------------------------------------------------------- */

int can_hal_rx(uint32_t *id, uint8_t *data, uint8_t *dlc)
{
    if (s_count == 0u) return 0;

    /* Disable interrupt briefly to read safely */
    nvic_disable_irq(NVIC_USB_LP_CAN_RX0_IRQ);

    *id  = s_fifo[s_tail].id;
    *dlc = s_fifo[s_tail].dlc;
    memcpy(data, (const void *)s_fifo[s_tail].data, s_fifo[s_tail].dlc);
    s_tail = (s_tail + 1u) % SW_FIFO_SIZE;
    s_count--;

    nvic_enable_irq(NVIC_USB_LP_CAN_RX0_IRQ);
    return 1;
}
