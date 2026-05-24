/**
 * test_decode_doors.c — Signal decode tests for CAN_ID_DOORS (0x540)
 *
 * Layout (⚠ verify with FORScan — task 6.3):
 *   d[0] bit0 = door_fl, bit1 = door_fr, bit2 = door_rl, bit3 = door_rr
 *   d[0] bit4 = tailgate, bit5 = bonnet, bit6 = park_brake
 */

#include "test_runner.h"
#include "test_helpers.h"

static void test_fl_door(void)
{
    car_test_reset();
    uint8_t d[8] = {0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    car_process_frame(0x540, d, 8);
    TEST_ASSERT_EQ(car_get_door_fl(), 1u);
    TEST_ASSERT_EQ(car_get_door_fr(), 0u);
    TEST_ASSERT_EQ(car_get_door_rl(), 0u);
    TEST_ASSERT_EQ(car_get_door_rr(), 0u);
}

static void test_all_doors_and_body(void)
{
    car_test_reset();
    /* bits 0-5 set = all 6 contacts open */
    uint8_t d[8] = {0x3F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    car_process_frame(0x540, d, 8);
    TEST_ASSERT_EQ(car_get_door_fl(),  1u);
    TEST_ASSERT_EQ(car_get_door_fr(),  1u);
    TEST_ASSERT_EQ(car_get_door_rl(),  1u);
    TEST_ASSERT_EQ(car_get_door_rr(),  1u);
    TEST_ASSERT_EQ(car_get_tailgate(), 1u);
    TEST_ASSERT_EQ(car_get_bonnet(),   1u);
    TEST_ASSERT_EQ(car_get_park_brake(), 0u);
}

static void test_park_brake(void)
{
    car_test_reset();
    /* bit 6 = park brake */
    uint8_t d[8] = {0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    car_process_frame(0x540, d, 8);
    TEST_ASSERT_EQ(car_get_park_brake(), 1u);
    TEST_ASSERT_EQ(car_get_door_fl(),    0u);
}

static void test_all_clear(void)
{
    car_test_reset();
    uint8_t d[8] = {0};
    car_process_frame(0x540, d, 8);
    TEST_ASSERT_EQ(car_get_door_fl(),    0u);
    TEST_ASSERT_EQ(car_get_door_fr(),    0u);
    TEST_ASSERT_EQ(car_get_door_rl(),    0u);
    TEST_ASSERT_EQ(car_get_door_rr(),    0u);
    TEST_ASSERT_EQ(car_get_tailgate(),   0u);
    TEST_ASSERT_EQ(car_get_bonnet(),     0u);
    TEST_ASSERT_EQ(car_get_park_brake(), 0u);
}

static void test_individual_bits(void)
{
    /* FR door */
    car_test_reset();
    uint8_t d_fr[8] = {0x02, 0};
    car_process_frame(0x540, d_fr, 8);
    TEST_ASSERT_EQ(car_get_door_fr(), 1u);

    /* RL door */
    car_test_reset();
    uint8_t d_rl[8] = {0x04, 0};
    car_process_frame(0x540, d_rl, 8);
    TEST_ASSERT_EQ(car_get_door_rl(), 1u);

    /* RR door */
    car_test_reset();
    uint8_t d_rr[8] = {0x08, 0};
    car_process_frame(0x540, d_rr, 8);
    TEST_ASSERT_EQ(car_get_door_rr(), 1u);

    /* Tailgate */
    car_test_reset();
    uint8_t d_tg[8] = {0x10, 0};
    car_process_frame(0x540, d_tg, 8);
    TEST_ASSERT_EQ(car_get_tailgate(), 1u);

    /* Bonnet */
    car_test_reset();
    uint8_t d_bn[8] = {0x20, 0};
    car_process_frame(0x540, d_bn, 8);
    TEST_ASSERT_EQ(car_get_bonnet(), 1u);
}

int main(void)
{
    test_fl_door();
    test_all_doors_and_body();
    test_park_brake();
    test_all_clear();
    test_individual_bits();
    return test_runner_result();
}
