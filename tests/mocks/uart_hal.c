/**
 * tests/mocks/uart_hal.c — UART HAL mock for unit tests
 *
 * TX side: bytes written via uart_hal_tx() accumulate in s_tx_buf.
 * RX side: bytes injected via mock_uart_inject() are returned one-at-a-time
 *          by uart_hal_rx(), simulating head-unit → decoder traffic.
 *
 * Call mock_uart_reset() at the start of each test case.
 */

#include "../../src/hal/uart_hal.h"
#include <string.h>
#include <stdint.h>

/* TX capture buffer */
#define TX_BUF_SIZE 256
static uint8_t  s_tx_buf[TX_BUF_SIZE];
static uint16_t s_tx_len;

/* RX inject buffer */
#define RX_BUF_SIZE 256
static uint8_t  s_rx_buf[RX_BUF_SIZE];
static uint16_t s_rx_len;
static uint16_t s_rx_pos;

/* ---- Public mock control API ------------------------------------------ */

void mock_uart_reset(void)
{
    memset(s_tx_buf, 0, sizeof(s_tx_buf));
    s_tx_len = 0;
    s_rx_len = 0;
    s_rx_pos = 0;
}

const uint8_t *mock_uart_get_buf(void) { return s_tx_buf; }
uint16_t       mock_uart_get_len(void) { return s_tx_len; }

/** Queue bytes to be returned by uart_hal_rx() in sequence. */
void mock_uart_inject(const uint8_t *bytes, uint16_t len)
{
    if (len > RX_BUF_SIZE) len = RX_BUF_SIZE;
    memcpy(s_rx_buf, bytes, len);
    s_rx_len = len;
    s_rx_pos = 0;
}

/* ---- uart_hal.h interface implementation ------------------------------ */

void uart_hal_init(void) {}

void uart_hal_tx(uint8_t byte)
{
    if (s_tx_len < TX_BUF_SIZE)
        s_tx_buf[s_tx_len++] = byte;
}

int uart_hal_rx(uint8_t *byte)
{
    if (s_rx_pos >= s_rx_len) return 0;
    *byte = s_rx_buf[s_rx_pos++];
    return 1;
}
