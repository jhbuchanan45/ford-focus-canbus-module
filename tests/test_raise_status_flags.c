/**
 * test_raise_status_flags.c — 0x24 status flags packet tests
 *
 * Flags byte: bit0=reverse, bit1=park_brake, bit2=near_lights
 * Packet: 2E 24 01 <flags> CSUM
 */

#include "test_runner.h"
#include "test_helpers.h"
#include "../src/output/raise.h"

static uint8_t flags_byte(void)
{
    return mock_uart_get_buf()[3];
}

static void test_all_clear(void)
{
    mock_uart_reset(); car_test_reset();
    raise_send_status_flags();
    TEST_ASSERT_EQ(mock_uart_get_buf()[1], 0x24);
    TEST_ASSERT_EQ(flags_byte(), 0x00);
}

static void test_reverse_sets_bit0(void)
{
    mock_uart_reset(); car_test_reset();
    car_test_set_selector(e_selector_r);
    raise_send_status_flags();
    TEST_ASSERT_EQ(flags_byte() & 0x01, 0x01);
    TEST_ASSERT_EQ(flags_byte() & ~0x01, 0x00);
}

static void test_park_in_drive_clears_bit0(void)
{
    mock_uart_reset(); car_test_reset();
    car_test_set_selector(e_selector_d);
    raise_send_status_flags();
    TEST_ASSERT_EQ(flags_byte() & 0x01, 0x00);
}

static void test_park_brake_sets_bit1(void)
{
    mock_uart_reset(); car_test_reset();
    car_test_set_park_brake(1);
    raise_send_status_flags();
    TEST_ASSERT_EQ(flags_byte() & 0x02, 0x02);
}

static void test_near_lights_sets_bit2(void)
{
    mock_uart_reset(); car_test_reset();
    car_test_set_near_lights(1);
    raise_send_status_flags();
    TEST_ASSERT_EQ(flags_byte() & 0x04, 0x04);
}

static void test_all_flags_combined(void)
{
    mock_uart_reset(); car_test_reset();
    car_test_set_selector(e_selector_r);
    car_test_set_park_brake(1);
    car_test_set_near_lights(1);
    raise_send_status_flags();
    TEST_ASSERT_EQ(flags_byte(), 0x07);
}

static void test_reverse_checksum(void)
{
    mock_uart_reset(); car_test_reset();
    car_test_set_selector(e_selector_r);
    raise_send_status_flags();
    /* 2E 24 01 01 CSUM  →  CSUM = (0x24+0x01+0x01)^0xFF = 0x26^0xFF = 0xD9 */
    const uint8_t expected[] = {0x2E, 0x24, 0x01, 0x01, 0xD9};
    TEST_ASSERT_EQ(mock_uart_get_len(), (uint16_t)sizeof(expected));
    TEST_ASSERT_BYTES(mock_uart_get_buf(), expected, sizeof(expected));
}

int main(void)
{
    test_all_clear();
    test_reverse_sets_bit0();
    test_park_in_drive_clears_bit0();
    test_park_brake_sets_bit1();
    test_near_lights_sets_bit2();
    test_all_flags_combined();
    test_reverse_checksum();
    return test_runner_result();
}
