/**
 * test_raise_steering.c — 0x26 steering angle packet tests
 *
 * car_get_wheel() returns degrees × 10 (range -5400 to +5400).
 * Raise receives angle/10 as signed int16 big-endian (range -540 to +540).
 *
 * Packet: 2E 26 02 <angle_hi> <angle_lo> CSUM
 */

#include "test_runner.h"
#include "test_helpers.h"
#include "../src/output/raise.h"
#include <stdint.h>

static void check_steering(int16_t wheel_raw,
                            uint8_t hi, uint8_t lo, uint8_t csum)
{
    mock_uart_reset(); car_test_reset();
    car_test_set_wheel(wheel_raw);
    raise_send_steering();

    TEST_ASSERT_EQ(mock_uart_get_len(), 6u);
    TEST_ASSERT_EQ(mock_uart_get_buf()[0], 0x2E);
    TEST_ASSERT_EQ(mock_uart_get_buf()[1], 0x26);
    TEST_ASSERT_EQ(mock_uart_get_buf()[2], 0x02);
    TEST_ASSERT_EQ(mock_uart_get_buf()[3], hi);
    TEST_ASSERT_EQ(mock_uart_get_buf()[4], lo);
    TEST_ASSERT_EQ(mock_uart_get_buf()[5], csum);
}

int main(void)
{
    /* Zero — no angle */
    /* angle=0 → 0x0000, CSUM=(0x26+0x02+0+0)^0xFF=0x28^0xFF=0xD7 */
    check_steering(0, 0x00, 0x00, 0xD7);

    /* +360° — wheel=3600, angle=360=0x0168
     * CSUM=(0x26+0x02+0x01+0x68)^0xFF=0x91^0xFF=0x6E */
    check_steering(3600, 0x01, 0x68, 0x6E);

    /* -360° — wheel=-3600, angle=-360
     * -360 as uint16 = 65536-360 = 65176 = 0xFE98
     * CSUM=(0x26+0x02+0xFE+0x98)^0xFF: 0x28+0xFE=0x26(wrap)+0x98=0xBE, ^0xFF=0x41 */
    check_steering(-3600, 0xFE, 0x98, 0x41);

    /* +540° full lock — wheel=5400, angle=540=0x021C
     * CSUM=(0x26+0x02+0x02+0x1C)^0xFF=0x46^0xFF=0xB9 */
    check_steering(5400, 0x02, 0x1C, 0xB9);

    /* -540° full lock — wheel=-5400, angle=-540
     * -540 = 65536-540 = 64996 = 0xFDE4
     * CSUM=(0x26+0x02+0xFD+0xE4)^0xFF: 0x28+0xFD=0x25(wrap)+0xE4=0x09(wrap), ^0xFF=0xF6 */
    check_steering(-5400, 0xFD, 0xE4, 0xF6);

    return test_runner_result();
}
