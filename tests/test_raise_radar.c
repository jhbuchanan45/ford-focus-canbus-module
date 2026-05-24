/**
 * test_raise_radar.c — 0x22/0x23 radar distance packet tests
 *
 * dist_to_raise formula (RMAX=10):
 *   raw >= 99 (RADAR_DIST_CLEAR) → 0 (no obstacle)
 *   otherwise → 11 - (raw * 10 / 98)     [integer division]
 *
 *   raw=0  → 11 - 0  = 11
 *   raw=49 → 11 - 5  = 6    (49*10/98 = 490/98 = 5)
 *   raw=98 → 11 - 10 = 1
 *   raw=99 → 0
 *
 * Rear packet (0x22): sensors [RL, RLM, RRM, RR] = dist[0..3]
 * Front packet (0x23): sensors [FL, FLM, FRM, FR] = dist[4..7]
 */

#include "test_runner.h"
#include "test_helpers.h"
#include "../src/output/raise.h"

/* Set all 8 sensors to the same value */
static void set_all_sensors(uint8_t raw)
{
    car_test_set_radar_state(e_radar_on);
    for (uint8_t i = 0; i < 8; i++)
        car_test_set_radar_dist(i, raw);
}

static void test_dist_formula_closest(void)
{
    mock_uart_reset(); car_test_reset();
    set_all_sensors(0);
    raise_send_rear_radar();
    /* All 4 rear bytes should be 11 */
    TEST_ASSERT_EQ(mock_uart_get_buf()[1], 0x22);
    TEST_ASSERT_EQ(mock_uart_get_buf()[3], 11);
    TEST_ASSERT_EQ(mock_uart_get_buf()[4], 11);
    TEST_ASSERT_EQ(mock_uart_get_buf()[5], 11);
    TEST_ASSERT_EQ(mock_uart_get_buf()[6], 11);
}

static void test_dist_formula_midpoint(void)
{
    mock_uart_reset(); car_test_reset();
    set_all_sensors(49);
    raise_send_rear_radar();
    TEST_ASSERT_EQ(mock_uart_get_buf()[3], 6);
}

static void test_dist_formula_almost_clear(void)
{
    mock_uart_reset(); car_test_reset();
    set_all_sensors(98);
    raise_send_rear_radar();
    TEST_ASSERT_EQ(mock_uart_get_buf()[3], 1);
}

static void test_dist_formula_clear(void)
{
    mock_uart_reset(); car_test_reset();
    set_all_sensors(99);
    raise_send_rear_radar();
    TEST_ASSERT_EQ(mock_uart_get_buf()[3], 0);
}

static void test_rear_packet_cmd(void)
{
    mock_uart_reset(); car_test_reset();
    set_all_sensors(0);
    raise_send_rear_radar();
    TEST_ASSERT_EQ(mock_uart_get_buf()[0], 0x2E);
    TEST_ASSERT_EQ(mock_uart_get_buf()[1], 0x22);
    TEST_ASSERT_EQ(mock_uart_get_buf()[2], 0x04); /* LEN = 4 */
    TEST_ASSERT_EQ(mock_uart_get_len(), 8u);       /* SOF+CMD+LEN+4data+CSUM = 8 */
}

static void test_front_packet_cmd(void)
{
    mock_uart_reset(); car_test_reset();
    set_all_sensors(0);
    raise_send_front_radar();
    TEST_ASSERT_EQ(mock_uart_get_buf()[1], 0x23);
    TEST_ASSERT_EQ(mock_uart_get_buf()[3], 11); /* all front sensors at 0 */
}

static void test_mixed_sensor_values(void)
{
    mock_uart_reset(); car_test_reset();
    car_test_set_radar_state(e_radar_on);
    car_test_set_radar_dist(0, 0);   /* RL  → 11 */
    car_test_set_radar_dist(1, 49);  /* RLM → 6  */
    car_test_set_radar_dist(2, 98);  /* RRM → 1  */
    car_test_set_radar_dist(3, 99);  /* RR  → 0  */
    raise_send_rear_radar();

    /* Packet: 2E 22 04 11 06 01 00 CSUM */
    TEST_ASSERT_EQ(mock_uart_get_buf()[3], 11);
    TEST_ASSERT_EQ(mock_uart_get_buf()[4], 6);
    TEST_ASSERT_EQ(mock_uart_get_buf()[5], 1);
    TEST_ASSERT_EQ(mock_uart_get_buf()[6], 0);
}

int main(void)
{
    test_dist_formula_closest();
    test_dist_formula_midpoint();
    test_dist_formula_almost_clear();
    test_dist_formula_clear();
    test_rear_packet_cmd();
    test_front_packet_cmd();
    test_mixed_sensor_values();
    return test_runner_result();
}
