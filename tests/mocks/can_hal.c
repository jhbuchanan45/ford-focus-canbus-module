/* can_hal.c mock — no-op, not used in output driver or queue tests */
#include "../../src/hal/can_hal.h"
void can_hal_init(const uint32_t *ids, uint8_t count) { (void)ids; (void)count; }
int  can_hal_rx(uint32_t *id, uint8_t *data, uint8_t *dlc)
     { (void)id; (void)data; (void)dlc; return 0; }
