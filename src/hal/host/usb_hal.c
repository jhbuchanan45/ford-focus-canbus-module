/**
 * usb_hal.c — Host USB CDC stub: maps to stdout
 *
 * On the host target, JSON output goes to stdout so it can be piped or
 * redirected by the caller.  usb_hal_poll() is a no-op.
 */

#include "../usb_hal.h"
#include <stdio.h>

void usb_hal_init(void)  { /* nothing to do on host */ }
void usb_hal_poll(void)  { /* nothing to do on host */ }

void usb_hal_write(const uint8_t *buf, uint16_t len)
{
    fwrite(buf, 1, len, stdout);
    fflush(stdout);
}
