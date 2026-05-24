/**
 * test_decode_hvac.c — Signal decode tests for CAN_ID_HVAC (0x3B5)
 *
 * STATUS: STUB — pending task 6.6.
 * Reference frame: cansend vcan0 3B5#0300002B00000000
 */

#include "test_runner.h"
#include "test_helpers.h"

int main(void)
{
#if 0 /* TODO: task 6.6 */

    /* AC on, fan=?, driver_temp=0x2B */
    car_test_reset();
    uint8_t data[8] = {0x03, 0x00, 0x00, 0x2B, 0x00, 0x00, 0x00, 0x00};
    car_process_frame(0x3B5, data, 8);
    /* TODO: assert specific values once bit layout confirmed */
    TEST_ASSERT_EQ(car_get_air_ac(), 1u);

    /* All zero → AC off */
    car_test_reset();
    uint8_t data_off[8] = {0};
    car_process_frame(0x3B5, data_off, 8);
    TEST_ASSERT_EQ(car_get_air_ac(), 0u);
    TEST_ASSERT_EQ(car_get_air_fan(), 0u);
    TEST_ASSERT_EQ(car_get_air_recirculation(), 0u);
    TEST_ASSERT_EQ(car_get_air_dual_zone(), 0u);

#endif /* TODO task 6.6 */

    g_test_count++;
    printf("SKIP: test_decode_hvac (pending task 6.6)\n");
    return 0;
}
