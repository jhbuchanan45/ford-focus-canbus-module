/**
 * test_raise_ac.c — 0x21 AC status packet tests
 *
 * Packet: 2E 21 05 <flags> <fan> <temp_driver> <temp_pass> <airflow> CSUM
 *   flags:   bit0=ac_on, bit1=recirculation, bit2=dual_zone
 *   airflow: bit0=wind,  bit1=middle,        bit2=floor
 *
 * Reference scenario (from spec):
 *   ac_on=1, fan=3, temp_driver=42 (21°C×2), temp_pass=42,
 *   recirculation=0, wind=1, middle=1, floor=0, dual_zone=0
 *   → DATA = 01 03 2A 2A 03
 *   CSUM = (0x21+0x05+0x01+0x03+0x2A+0x2A+0x03)^0xFF = 0x81^0xFF = 0x7E
 */

#include "test_runner.h"
#include "test_helpers.h"
#include "../src/output/raise.h"

static void test_reference_scenario(void)
{
    mock_uart_reset(); car_test_reset();
    car_test_set_air_ac(1);
    car_test_set_air_fan(3);
    car_test_set_air_temp_driver(42);
    car_test_set_air_temp_pass(42);
    car_test_set_air_recirculation(0);
    car_test_set_air_wind(1);
    car_test_set_air_middle(1);
    car_test_set_air_floor(0);
    car_test_set_air_dual_zone(0);

    raise_send_ac();

    const uint8_t expected[] = {
        0x2E, 0x21, 0x05,
        0x01,           /* flags: ac_on only */
        0x03,           /* fan speed 3 */
        0x2A,           /* driver temp: 42 */
        0x2A,           /* pass temp: 42 */
        0x03,           /* airflow: wind+middle */
        0x7E            /* checksum */
    };
    TEST_ASSERT_EQ(mock_uart_get_len(), (uint16_t)sizeof(expected));
    TEST_ASSERT_BYTES(mock_uart_get_buf(), expected, sizeof(expected));
}

static void test_ac_off(void)
{
    mock_uart_reset(); car_test_reset();
    raise_send_ac();
    TEST_ASSERT_EQ(mock_uart_get_buf()[1], 0x21);
    TEST_ASSERT_EQ(mock_uart_get_buf()[3] & 0x01, 0x00); /* ac_on bit clear */
}

static void test_recirculation_bit(void)
{
    mock_uart_reset(); car_test_reset();
    car_test_set_air_recirculation(1);
    raise_send_ac();
    TEST_ASSERT_EQ(mock_uart_get_buf()[3] & 0x02, 0x02);
}

static void test_dual_zone_bit(void)
{
    mock_uart_reset(); car_test_reset();
    car_test_set_air_dual_zone(1);
    raise_send_ac();
    TEST_ASSERT_EQ(mock_uart_get_buf()[3] & 0x04, 0x04);
}

static void test_fan_speed_zero(void)
{
    mock_uart_reset(); car_test_reset();
    car_test_set_air_fan(0);
    raise_send_ac();
    TEST_ASSERT_EQ(mock_uart_get_buf()[4], 0x00);
}

static void test_fan_speed_max(void)
{
    mock_uart_reset(); car_test_reset();
    car_test_set_air_fan(7);
    raise_send_ac();
    TEST_ASSERT_EQ(mock_uart_get_buf()[4], 0x07);
}

static void test_airflow_floor_only(void)
{
    mock_uart_reset(); car_test_reset();
    car_test_set_air_floor(1);
    raise_send_ac();
    TEST_ASSERT_EQ(mock_uart_get_buf()[7], 0x04);
}

static void test_packet_length(void)
{
    mock_uart_reset(); car_test_reset();
    raise_send_ac();
    /* SOF(1) + CMD(1) + LEN(1) + DATA(5) + CSUM(1) = 9 bytes */
    TEST_ASSERT_EQ(mock_uart_get_len(), 9u);
    TEST_ASSERT_EQ(mock_uart_get_buf()[2], 0x05); /* LEN field */
}

int main(void)
{
    test_reference_scenario();
    test_ac_off();
    test_recirculation_bit();
    test_dual_zone_bit();
    test_fan_speed_zero();
    test_fan_speed_max();
    test_airflow_floor_only();
    test_packet_length();
    return test_runner_result();
}
