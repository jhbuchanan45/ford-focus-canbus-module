/**
 * test_decode_doors.c — Signal decode tests for CAN_ID_DOORS (0x540)
 *
 * STATUS: STUB — pending task 6.3.
 */

#include "test_runner.h"
#include "test_helpers.h"

int main(void)
{
#if 0 /* TODO: task 6.3 */

    /* FL door open — bit0 in suspected byte */
    car_test_reset();
    uint8_t data[8] = {0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    car_process_frame(0x540, data, 8);
    TEST_ASSERT_EQ(car_get_door_fl(), 1u);
    TEST_ASSERT_EQ(car_get_door_fr(), 0u);

    /* All doors */
    car_test_reset();
    uint8_t data_all[8] = {0x3F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    car_process_frame(0x540, data_all, 8);
    TEST_ASSERT_EQ(car_get_door_fl(),  1u);
    TEST_ASSERT_EQ(car_get_door_fr(),  1u);
    TEST_ASSERT_EQ(car_get_door_rl(),  1u);
    TEST_ASSERT_EQ(car_get_door_rr(),  1u);
    TEST_ASSERT_EQ(car_get_tailgate(), 1u);
    TEST_ASSERT_EQ(car_get_bonnet(),   1u);

    /* Park brake */
    car_test_reset();
    uint8_t data_pb[8] = {0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    car_process_frame(0x540, data_pb, 8);
    TEST_ASSERT_EQ(car_get_park_brake(), 1u);

#endif /* TODO task 6.3 */

    g_test_count++;
    printf("SKIP: test_decode_doors (pending task 6.3)\n");
    return 0;
}
