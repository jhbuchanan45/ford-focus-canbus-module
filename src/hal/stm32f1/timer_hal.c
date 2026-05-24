/**
 * timer_hal.c — STM32F103 SysTick 1 ms monotonic counter
 *
 * SysTick is sourced from the AHB clock (72 MHz after PLL setup in main).
 * Reload value = 72000 → 1 ms interrupt.
 *
 * Verification:
 *   Toggle the heartbeat LED every 500 calls to gpio_hal_led_toggle()
 *   from the main loop.  Verify with a phone stopwatch: 10 blinks in 10 s.
 */

#include "../timer_hal.h"

#include <libopencm3/cm3/systick.h>
#include <libopencm3/cm3/nvic.h>

/* AHB clock after rcc_clock_setup_in_hse_8mhz_out_72mhz() */
#define AHB_FREQ_HZ  72000000UL

static volatile uint32_t s_tick_ms;

void sys_tick_handler(void)
{
    s_tick_ms++;
}

void timer_hal_init(void)
{
    systick_set_clocksource(STK_CSR_CLKSOURCE_AHB);
    systick_set_reload(AHB_FREQ_HZ / 1000UL - 1UL); /* 1 ms */
    systick_counter_enable();
    systick_interrupt_enable();
    s_tick_ms = 0;
}

uint32_t hw_tick_get(void)
{
    return s_tick_ms;
}
