/**
 * gpio_hal.c — Host GPIO stub (no-op)
 *
 * No physical GPIO on the host target; the LED heartbeat is silently
 * suppressed.
 */

#include "../gpio_hal.h"

void gpio_hal_init(void)       { /* no-op on host */ }
void gpio_hal_led_toggle(void) { /* no-op on host */ }
