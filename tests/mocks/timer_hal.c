/* timer_hal.c mock — monotonically incrementing tick */
#include "../../src/hal/timer_hal.h"
static uint32_t s_tick;
void     timer_hal_init(void)  { s_tick = 0; }
uint32_t hw_tick_get(void)     { return s_tick++; }
