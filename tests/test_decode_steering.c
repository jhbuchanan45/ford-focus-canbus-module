/**
 * test_decode_steering.c — Signal decode tests for CAN_ID_EPAS (0x080)
 *
 * Layout (⚠ verify with FORScan — task 6.7):
 *   d[0..1]: signed int16 big-endian, 0.1 deg/bit
 *   → already in our deg×10 internal unit
 */

#include "test_runner.h"
#include "test_helpers.h"

static void test_positive_angle(void)
{
    car_test_reset();
    /* +360° → internal 3600 = 0x0E10 */
    uint8_t d[8] = {0x0E, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    car_process_frame(0x080, d, 8);
    TEST_ASSERT_EQ(car_get_wheel(), 3600);
}

static void test_negative_angle(void)
{
    car_test_reset();
    /* -360° → internal -3600 = 0xF1F0 (two's complement) */
    uint8_t d[8] = {0xF1, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    car_process_frame(0x080, d, 8);
    TEST_ASSERT_EQ(car_get_wheel(), -3600);
}

static void test_zero_angle(void)
{
    car_test_reset();
    uint8_t d[8] = {0};
    car_process_frame(0x080, d, 8);
    TEST_ASSERT_EQ(car_get_wheel(), 0);
}

static void test_full_lock_right(void)
{
    car_test_reset();
    /* +540° → 5400 = 0x1518 */
    uint8_t d[8] = {0x15, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    car_process_frame(0x080, d, 8);
    TEST_ASSERT_EQ(car_get_wheel(), 5400);
}

static void test_full_lock_left(void)
{
    car_test_reset();
    /* -540° → -5400 = 0xEAE8 */
    uint8_t d[8] = {0xEA, 0xE8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    car_process_frame(0x080, d, 8);
    TEST_ASSERT_EQ(car_get_wheel(), -5400);
}

static void test_short_frame_ignored(void)
{
    car_test_reset();
    uint8_t d[8] = {0x0E, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    car_process_frame(0x080, d, 1);  /* DLC < 2 */
    TEST_ASSERT_EQ(car_get_wheel(), 0);
}

int main(void)
{
    test_positive_angle();
    test_negative_angle();
    test_zero_angle();
    test_full_lock_right();
    test_full_lock_left();
    test_short_frame_ignored();
    return test_runner_result();
}
