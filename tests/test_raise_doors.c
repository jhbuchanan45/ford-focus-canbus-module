/**
 * test_raise_doors.c — 0x41/0x01 door status packet tests
 *
 * Door byte bit layout:
 *   bit0=FL, bit1=FR, bit2=RL, bit3=RR, bit4=tailgate, bit5=bonnet
 */

#include "test_runner.h"
#include "test_helpers.h"
#include "../src/output/raise.h"

static uint8_t door_byte_from_packet(void)
{
    /* Packet: 2E 41 02 01 <door_byte> CSUM — door byte is at index 4 */
    return mock_uart_get_buf()[4];
}

static void test_all_closed(void)
{
    mock_uart_reset(); car_test_reset();
    raise_send_doors();
    TEST_ASSERT_EQ(mock_uart_get_buf()[1], 0x41); /* CMD */
    TEST_ASSERT_EQ(mock_uart_get_buf()[3], 0x01); /* subcmd */
    TEST_ASSERT_EQ(door_byte_from_packet(), 0x00);
}

static void test_fl_open(void)
{
    mock_uart_reset(); car_test_reset();
    car_test_set_door_fl(1);
    raise_send_doors();
    TEST_ASSERT_EQ(door_byte_from_packet() & 0x01, 0x01);
    TEST_ASSERT_EQ(door_byte_from_packet() & ~0x01, 0x00);
}

static void test_fr_open(void)
{
    mock_uart_reset(); car_test_reset();
    car_test_set_door_fr(1);
    raise_send_doors();
    TEST_ASSERT_EQ(door_byte_from_packet(), 0x02);
}

static void test_rl_open(void)
{
    mock_uart_reset(); car_test_reset();
    car_test_set_door_rl(1);
    raise_send_doors();
    TEST_ASSERT_EQ(door_byte_from_packet(), 0x04);
}

static void test_rr_open(void)
{
    mock_uart_reset(); car_test_reset();
    car_test_set_door_rr(1);
    raise_send_doors();
    TEST_ASSERT_EQ(door_byte_from_packet(), 0x08);
}

static void test_tailgate_open(void)
{
    mock_uart_reset(); car_test_reset();
    car_test_set_tailgate(1);
    raise_send_doors();
    TEST_ASSERT_EQ(door_byte_from_packet(), 0x10);
}

static void test_bonnet_open(void)
{
    mock_uart_reset(); car_test_reset();
    car_test_set_bonnet(1);
    raise_send_doors();
    TEST_ASSERT_EQ(door_byte_from_packet(), 0x20);
}

static void test_fl_and_rr_open(void)
{
    mock_uart_reset(); car_test_reset();
    car_test_set_door_fl(1);
    car_test_set_door_rr(1);
    raise_send_doors();
    TEST_ASSERT_EQ(door_byte_from_packet(), 0x09);
}

static void test_all_open(void)
{
    mock_uart_reset(); car_test_reset();
    car_test_set_door_fl(1); car_test_set_door_fr(1);
    car_test_set_door_rl(1); car_test_set_door_rr(1);
    car_test_set_tailgate(1); car_test_set_bonnet(1);
    raise_send_doors();
    TEST_ASSERT_EQ(door_byte_from_packet(), 0x3F);
}

static void test_checksum_correct(void)
{
    mock_uart_reset(); car_test_reset();
    car_test_set_door_fl(1);
    raise_send_doors();
    /* Packet: 2E 41 02 01 01 CSUM
     * CSUM = (0x41 + 0x02 + 0x01 + 0x01) ^ 0xFF = 0x45 ^ 0xFF = 0xBA */
    const uint8_t expected[] = {0x2E, 0x41, 0x02, 0x01, 0x01, 0xBA};
    TEST_ASSERT_EQ(mock_uart_get_len(), (uint16_t)sizeof(expected));
    TEST_ASSERT_BYTES(mock_uart_get_buf(), expected, sizeof(expected));
}

int main(void)
{
    test_all_closed();
    test_fl_open();
    test_fr_open();
    test_rl_open();
    test_rr_open();
    test_tailgate_open();
    test_bonnet_open();
    test_fl_and_rr_open();
    test_all_open();
    test_checksum_correct();
    return test_runner_result();
}
