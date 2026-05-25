/**
 * test_decode_hvac.c — Signal decode tests for CAN_ID_HVAC (0x3B5)
 *
 * Layout (⚠ verify with FORScan — task 6.6):
 *   d[0] bit0 = ac_on, bit1 = recirculation, bit2 = dual_zone
 *   d[1] bits[3:0] = fan speed raw (0–15) → Raise 0–7 (raw / 2)
 *   d[2] = driver temp °C × 2
 *   d[3] = passenger temp °C × 2
 *   d[4] bit0 = windscreen, bit1 = middle, bit2 = floor
 *
 * Reference frame: 3B5#0300002B00000000
 *   d[0]=0x03 (ac=1, recirc=1), d[1]=0x00 (fan=0), d[2]=0x00, d[3]=0x2B=43
 */

#include "test_runner.h"
#include "test_helpers.h"

static void test_ac_on(void)
{
    car_test_reset();
    /* d[0]=0x03: bit0(ac)=1, bit1(recirc)=1 */
    uint8_t d[8] = {0x03, 0x00, 0x00, 0x2B, 0x00, 0x00, 0x00, 0x00};
    car_process_frame(0x3B5, d, 8);
    TEST_ASSERT_EQ(car_get_air_ac(), 1u);
    TEST_ASSERT_EQ(car_get_air_recirculation(), 1u);
}

static void test_all_off(void)
{
    car_test_reset();
    uint8_t d[8] = {0};
    car_process_frame(0x3B5, d, 8);
    TEST_ASSERT_EQ(car_get_air_ac(), 0u);
    TEST_ASSERT_EQ(car_get_air_fan(), 0u);
    TEST_ASSERT_EQ(car_get_air_recirculation(), 0u);
    TEST_ASSERT_EQ(car_get_air_dual_zone(), 0u);
    TEST_ASSERT_EQ(car_get_air_wind(), 0u);
    TEST_ASSERT_EQ(car_get_air_middle(), 0u);
    TEST_ASSERT_EQ(car_get_air_floor(), 0u);
}

static void test_fan_speed(void)
{
    car_test_reset();
    /* fan raw = 6 (bits[3:0]=0x06) → Raise fan = 6 >> 1 = 3 */
    uint8_t d[8] = {0x01, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    car_process_frame(0x3B5, d, 8);
    TEST_ASSERT_EQ(car_get_air_fan(), 3u);
}

static void test_temperatures(void)
{
    car_test_reset();
    /* driver = 21°C → temp_driver = 42 (°C × 2); pass = 22°C → 44 */
    uint8_t d[8] = {0x01, 0x00, 42, 44, 0x00, 0x00, 0x00, 0x00};
    car_process_frame(0x3B5, d, 8);
    TEST_ASSERT_EQ(car_get_air_temp_driver(), 42u);
    TEST_ASSERT_EQ(car_get_air_temp_pass(),   44u);
}

static void test_dual_zone(void)
{
    car_test_reset();
    /* d[0] bit2 = dual_zone */
    uint8_t d[8] = {0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    car_process_frame(0x3B5, d, 8);
    TEST_ASSERT_EQ(car_get_air_dual_zone(), 1u);
}

static void test_airflow_bits(void)
{
    car_test_reset();
    /* d[4]=0x07: windscreen(0)+middle(1)+floor(2) all on */
    uint8_t d[8] = {0x01, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00};
    car_process_frame(0x3B5, d, 8);
    TEST_ASSERT_EQ(car_get_air_wind(),   1u);
    TEST_ASSERT_EQ(car_get_air_middle(), 1u);
    TEST_ASSERT_EQ(car_get_air_floor(),  1u);
}

static void test_short_frame_ignored(void)
{
    car_test_reset();
    /* DLC < 4 — should not modify state */
    uint8_t d[8] = {0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00};
    car_process_frame(0x3B5, d, 3);
    TEST_ASSERT_EQ(car_get_air_ac(), 0u);
}

int main(void)
{
    test_ac_on();
    test_all_off();
    test_fan_speed();
    test_temperatures();
    test_dual_zone();
    test_airflow_bits();
    test_short_frame_ignored();
    return test_runner_result();
}
