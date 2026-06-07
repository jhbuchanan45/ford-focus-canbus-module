/**
 * raise_sniffer.c — Raise VW PQ packet receiver and logger
 *
 * Receives 0x2E-framed packets from the FD60 decoder on USART2 RX (PA3),
 * logs every packet to USB CDC as a hex dump line, and ACKs valid packets
 * with 0xFF on USART2 TX (PA2) to keep the FD60 transmitting.
 *
 * Log format (one line per packet):
 *   T+NNNNNN 2E CC LL | DD DD ... | C=XX OK\n    — valid
 *   T+NNNNNN 2E CC LL | DD DD ... | C=XX BAD\n   — bad checksum
 *   T+NNNNNN 2E CC LL | DD DD ... | C=XX TRUNC\n — LEN > 16, partial capture
 *
 * Timestamp wraps at 999999 ms (~16 min); wrap is visible as a counter reset.
 */

#include "raise_sniffer.h"
#include "../hal/uart_hal.h"
#include "../hal/usb_hal.h"
#include "../hal/timer_hal.h"

#include <stdio.h>
#include <stdint.h>

#define DATA_BUF_MAX 16u

typedef enum {
    RX_WAIT_SOF = 0,
    RX_CMD,
    RX_LEN,
    RX_DATA,
    RX_CSUM,
} rx_state_t;

static rx_state_t s_state;
static uint8_t    s_cmd;
static uint8_t    s_len;
static uint8_t    s_remaining;
static uint8_t    s_csum_accum;
static uint8_t    s_data[DATA_BUF_MAX];
static uint8_t    s_data_idx;
static uint8_t    s_truncated;

static void reset_state(void)
{
    s_state      = RX_WAIT_SOF;
    s_truncated  = 0;
    s_data_idx   = 0;
    s_csum_accum = 0;
}

static void emit_log(uint8_t rx_csum)
{
    uint32_t ts       = hw_tick_get() % 1000000u;
    uint8_t  expected = s_csum_accum ^ 0xFF;
    int      valid    = (rx_csum == expected);

    const char *status = s_truncated ? "TRUNC" : (valid ? "OK" : "BAD");

    char    buf[128];
    int     pos = 0;

    pos += snprintf(buf + pos, sizeof(buf) - (size_t)pos,
        "T+%06lu 2E %02X %02X |",
        (unsigned long)ts, s_cmd, s_len);

    for (uint8_t i = 0; i < s_data_idx; i++) {
        pos += snprintf(buf + pos, sizeof(buf) - (size_t)pos,
            " %02X", s_data[i]);
    }

    pos += snprintf(buf + pos, sizeof(buf) - (size_t)pos,
        " | C=%02X %s\n", rx_csum, status);

    usb_hal_write((const uint8_t *)buf, (uint16_t)pos);

    if (valid && !s_truncated) {
        uart_hal_tx(0xFF);
    }
}

void sniffer_rx_poll(void)
{
    uint8_t byte;
    while (uart_hal_rx(&byte)) {
        switch (s_state) {
        case RX_WAIT_SOF:
            if (byte == 0x2E) s_state = RX_CMD;
            break;

        case RX_CMD:
            s_cmd        = byte;
            s_csum_accum = byte;
            s_state      = RX_LEN;
            break;

        case RX_LEN:
            s_len         = byte;
            s_csum_accum += byte;
            s_data_idx    = 0;
            if (byte == 0) {
                s_state = RX_CSUM;
            } else {
                s_truncated = (byte > DATA_BUF_MAX) ? 1u : 0u;
                s_remaining = byte;
                s_state     = RX_DATA;
            }
            break;

        case RX_DATA:
            /* Accumulate all bytes into checksum; buffer only first 16 */
            s_csum_accum += byte;
            if (s_data_idx < DATA_BUF_MAX) {
                s_data[s_data_idx++] = byte;
            }
            s_remaining--;
            if (s_remaining == 0) s_state = RX_CSUM;
            break;

        case RX_CSUM:
            emit_log(byte);
            reset_state();
            break;

        default:
            reset_state();
            break;
        }
    }
}
