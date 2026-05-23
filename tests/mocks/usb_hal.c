/**
 * tests/mocks/usb_hal.c — USB CDC HAL mock for unit tests
 *
 * Bytes written via usb_hal_write() accumulate in a string buffer.
 * Call mock_usb_reset() at the start of each test case.
 */

#include "../../src/hal/usb_hal.h"
#include <string.h>
#include <stdint.h>

#define USB_BUF_SIZE 4096
static char     s_buf[USB_BUF_SIZE + 1]; /* +1 for null terminator */
static uint16_t s_len;

/* ---- Public mock control API ------------------------------------------ */

void mock_usb_reset(void)
{
    memset(s_buf, 0, sizeof(s_buf));
    s_len = 0;
}

const char *mock_usb_get_buf(void) { return s_buf; }
uint16_t    mock_usb_get_len(void) { return s_len; }

/* ---- usb_hal.h interface implementation ------------------------------- */

void usb_hal_init(void) {}
void usb_hal_poll(void) {}

void usb_hal_write(const uint8_t *buf, uint16_t len)
{
    for (uint16_t i = 0; i < len && s_len < USB_BUF_SIZE; i++)
        s_buf[s_len++] = (char)buf[i];
    s_buf[s_len] = '\0';
}
