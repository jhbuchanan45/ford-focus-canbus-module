/**
 * test_decode_steering.c — Signal decode tests for CAN_ID_EPAS (0x080)
 *
 * STATUS: STUB — pending task 6.7.
 * Expected: d[0..1] = signed int16, scale 0.1 deg/bit
 */

#include "test_runner.h"
#include "test_helpers.h"

int main(void)
{
#if 0 /* TODO: task 6.7 */

    /* +360° → raw = 3600 = 0x0E10 */
    car_test_reset();
    uint8_t data_pos[8] = {0x0E, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    car_process_frame(0x080, data_pos, 8);
    TEST_ASSERT_EQ(car_get_wheel(), 3600);

    /* -360° → raw = -3600 = 0xF1F0 (two's complement) */
    car_test_reset();
    uint8_t data_neg[8] = {0xF1, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    car_process_frame(0x080, data_neg, 8);
    TEST_ASSERT_EQ(car_get_wheel(), -3600);

    /* Zero */
    car_test_reset();
    uint8_t data_zero[8] = {0};
    car_process_frame(0x080, data_zero, 8);
    TEST_ASSERT_EQ(car_get_wheel(), 0);

#endif /* TODO task 6.7 */

    g_test_count++;
    printf("SKIP: test_decode_steering (pending task 6.7)\n");
    return 0;
}
