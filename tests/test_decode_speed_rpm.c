/**
 * test_decode_speed_rpm.c — Signal decode tests for CAN_ID_SPEED_RPM (0x217)
 *
 * STATUS: STUB — bodies guarded with #if 0 until task 6.1 implements
 * decode_speed_rpm() in ford_focus_mk3_2015.c.
 *
 * Pattern (D4 from design.md):
 *   1. car_process_frame(0x217, data, dlc)  — exercise the decode path
 *   2. car_get_speed() / car_get_taho()     — assert decoded values
 */

#include "test_runner.h"
#include "test_helpers.h"

int main(void)
{
#if 0 /* TODO: task 6.1 — fill in after FORScan confirms bit layout */

    /* Speed: suspected scale 0.01 km/h/bit → 5000 = 50.00 km/h
     * d[0..1] = raw value for 50.00 km/h = 5000 → 0x1388 */
    car_test_reset();
    uint8_t data_speed[8] = {0x13, 0x88, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    car_process_frame(0x217, data_speed, 8);
    TEST_ASSERT_EQ(car_get_speed(), 5000u);

    /* RPM: suspected scale 0.25 RPM/bit → 2000 RPM = 8000 raw → 0x1F40 */
    car_test_reset();
    uint8_t data_rpm[8] = {0x00, 0x00, 0x1F, 0x40, 0x00, 0x00, 0x00, 0x00};
    car_process_frame(0x217, data_rpm, 8);
    TEST_ASSERT_EQ(car_get_taho(), 2000u);

    /* Zero values */
    car_test_reset();
    uint8_t data_zero[8] = {0};
    car_process_frame(0x217, data_zero, 8);
    TEST_ASSERT_EQ(car_get_speed(), 0u);
    TEST_ASSERT_EQ(car_get_taho(), 0u);

#endif /* TODO task 6.1 */

    /* Placeholder: always passes so CI is not broken before implementation */
    g_test_count++;
    printf("SKIP: test_decode_speed_rpm (pending task 6.1)\n");
    return 0;
}
