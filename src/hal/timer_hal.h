#ifndef CANMOD_TIMER_HAL_H
#define CANMOD_TIMER_HAL_H

#include <stdint.h>

/**
 * Initialise the monotonic millisecond timer.
 *
 * STM32: configures SysTick at 1 ms with the AHB clock source and enables
 *        the SysTick interrupt.
 * Host:  records the boot time via clock_gettime(CLOCK_MONOTONIC).
 */
void timer_hal_init(void);

/**
 * Return the number of milliseconds elapsed since timer_hal_init().
 *
 * The counter wraps at 2^32 ms (≈ 49.7 days).  Callers must handle wrap
 * correctly by comparing with subtraction rather than greater-than.
 */
uint32_t hw_tick_get(void);

#endif /* CANMOD_TIMER_HAL_H */
