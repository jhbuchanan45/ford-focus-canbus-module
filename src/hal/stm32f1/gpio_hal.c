/**
 * gpio_hal.c — STM32F103 GPIO HAL
 *
 * PC13 — Blue Pill on-board LED, active-low.
 *
 * Heartbeat verification:
 *   Build with any output driver.  Connect USB power to the Blue Pill.
 *   The LED should blink at 1 Hz (toggle every 500 ms via the main loop).
 */

#include "../gpio_hal.h"

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>

void gpio_hal_init(void)
{
    rcc_periph_clock_enable(RCC_GPIOC);
    /* PC13 as push-pull output, 2 MHz (LED doesn't need speed) */
    gpio_set_mode(GPIOC, GPIO_MODE_OUTPUT_2_MHZ,
                  GPIO_CNF_OUTPUT_PUSHPULL, GPIO13);
    gpio_set(GPIOC, GPIO13); /* LED off (active-low) */
}

void gpio_hal_led_toggle(void)
{
    gpio_toggle(GPIOC, GPIO13);
}
