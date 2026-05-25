/**
 * test_decode_speed_rpm.c — Signal decode tests for CAN_ID_SPEED_RPM (0x217)
 *
 * Layout (⚠ verify with FORScan — task 6.1):
 *   d[0..1] big-endian uint16: speed in 0.01 km/h/bit (= km/h × 100)
 *   d[2..3] big-endian uint16: RPM in 0.25 RPM/bit (raw >> 2 = RPM)
 */

#include "test_runner.h"
#include "test_helpers.h"

static void test_speed_decode(void)
{
    car_test_reset();
    /* 50.00 km/h → internal 5000 → raw 0x1388 */
    uint8_t d[8] = {0x13, 0x88, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    car_process_frame(0x217, d, 8);
    TEST_ASSERT_EQ(car_get_speed(), 5000u);
}

static void test_rpm_decode(void)
{
    car_test_reset();
    /* 2000 RPM → raw = 2000 × 4 = 8000 = 0x1F40 */
    uint8_t d[8] = {0x00, 0x00, 0x1F, 0x40, 0x00, 0x00, 0x00, 0x00};
    car_process_frame(0x217, d, 8);
    TEST_ASSERT_EQ(car_get_taho(), 2000u);
}

static void test_speed_and_rpm_combined(void)
{
    car_test_reset();
    /* speed = 12050 (120.50 km/h) = 0x2F12; rpm = 3500 → raw 14000 = 0x36B0 */
    uint8_t d[8] = {0x2F, 0x12, 0x36, 0xB0, 0x00, 0x00, 0x00, 0x00};
    car_process_frame(0x217, d, 8);
    TEST_ASSERT_EQ(car_get_speed(), 12050u);
    TEST_ASSERT_EQ(car_get_taho(), 3500u);
}

static void test_zero_values(void)
{
    car_test_reset();
    uint8_t d[8] = {0};
    car_process_frame(0x217, d, 8);
    TEST_ASSERT_EQ(car_get_speed(), 0u);
    TEST_ASSERT_EQ(car_get_taho(), 0u);
}

static void test_short_frame_ignored(void)
{
    car_test_reset();
    /* DLC < 4 — should not modify state */
    uint8_t d[8] = {0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00};
    car_process_frame(0x217, d, 3);
    TEST_ASSERT_EQ(car_get_speed(), 0u);
    TEST_ASSERT_EQ(car_get_taho(), 0u);
}

int main(void)
{
    test_speed_decode();
    test_rpm_decode();
    test_speed_and_rpm_combined();
    test_zero_values();
    test_short_frame_ignored();
    return test_runner_result();
}
