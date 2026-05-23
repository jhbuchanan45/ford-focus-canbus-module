#ifndef CANMOD_GPIO_HAL_H
#define CANMOD_GPIO_HAL_H

/**
 * Initialise GPIO.
 *
 * STM32: configures PC13 (Blue Pill on-board LED, active-low) as push-pull
 *        output and turns it off.
 * Host:  no-op.
 */
void gpio_hal_init(void);

/**
 * Toggle the heartbeat LED.
 *
 * Call at 2 Hz (every 500 ms) for a 1 Hz blink rate.
 * On the host target this is a no-op.
 */
void gpio_hal_led_toggle(void);

#endif /* CANMOD_GPIO_HAL_H */
