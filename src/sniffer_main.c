/**
 * sniffer_main.c — Entry point for canmod-sniffer (Blue Pill 2)
 *
 * Emulates the ATOTO head unit: receives Raise VW PQ packets from the FD60
 * decoder, ACKs each valid packet with 0xFF, and logs all packets to USB CDC.
 *
 * No CAN bus. No car state. No periodic transmit timers.
 * Wire: PA3 (RX) ← FD60 UART TX,  PA2 (TX) → FD60 UART RX (ACKs only).
 */

#include "output/raise_sniffer.h"
#include "hal/uart_hal.h"
#include "hal/usb_hal.h"
#include "hal/gpio_hal.h"
#include "hal/timer_hal.h"

#include <libopencm3/stm32/rcc.h>

static void clock_setup(void)
{
    rcc_clock_setup_in_hse_8mhz_out_72mhz();
}

static inline uint32_t elapsed(uint32_t since)
{
    return hw_tick_get() - since;
}

int main(void)
{
    clock_setup();
    timer_hal_init();
    gpio_hal_init();
    usb_hal_init();
    uart_hal_init();

    uint32_t t_heartbeat = hw_tick_get();

    while (1) {
        sniffer_rx_poll();
        usb_hal_poll();

        if (elapsed(t_heartbeat) >= 500u) {
            t_heartbeat = hw_tick_get();
            gpio_hal_led_toggle();
        }
    }

    return 0;
}
