/**
 * timer_hal.c — Host monotonic millisecond timer
 *
 * Uses clock_gettime(CLOCK_MONOTONIC) for sub-millisecond accuracy.
 * Rolls over at ~49.7 days, consistent with the STM32 SysTick counter.
 */

#include "../timer_hal.h"
#include <time.h>

static struct timespec s_boot;

void timer_hal_init(void)
{
    clock_gettime(CLOCK_MONOTONIC, &s_boot);
}

uint32_t hw_tick_get(void)
{
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);

    long sec_delta  = now.tv_sec  - s_boot.tv_sec;
    long nsec_delta = now.tv_nsec - s_boot.tv_nsec;
    if (nsec_delta < 0) {
        sec_delta--;
        nsec_delta += 1000000000L;
    }

    uint64_t ms = (uint64_t)sec_delta * 1000ULL +
                  (uint64_t)(nsec_delta / 1000000L);
    return (uint32_t)(ms & 0xFFFFFFFFULL);
}
