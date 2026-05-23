/**
 * test_raise_vehicle_info.c — 0x41/0x02 vehicle info packet tests
 *
 * Expected packet for speed=5000 (50 km/h), rpm=2000, temp=90°C,
 * voltage=12600 mV, odometer=12345 km:
 *
 *   2E 41 0D 02 00 32 07 D0 82 00 00 7E 00 00 30 39 3D
 *
 * Derivation:
 *   spd_kmh = 5000/100 = 50 → 0x0032
 *   rpm     = 2000         → 0x07D0
 *   temp+40 = 90+40 = 130  → 0x82
 *   fuel    = 0x00
 *   vol_01v = 12600/100=126 → 0x007E
 *   odo     = 12345        → 0x00003039
 *   LEN     = 13 = 0x0D
 *   CSUM    = (0x41+0x0D+sum_of_data) ^ 0xFF = 0x3D
 */

#include "test_runner.h"
#include "test_helpers.h"
#include "../src/output/raise.h"

int main(void)
{
    mock_uart_reset();
    car_test_reset();

    car_test_set_speed(5000);
    car_test_set_taho(2000);
    car_test_set_temp(90);
    car_test_set_voltage(12600);
    car_test_set_odometer(12345);

    raise_send_vehicle_info();

    const uint8_t expected[] = {
        0x2E, 0x41, 0x0D,
        0x02,                   /* subcmd */
        0x00, 0x32,             /* speed: 50 km/h */
        0x07, 0xD0,             /* rpm:  2000     */
        0x82,                   /* temp: 130 (90+40) */
        0x00,                   /* fuel: 0        */
        0x00, 0x7E,             /* voltage: 126 (0.1V units) */
        0x00, 0x00, 0x30, 0x39, /* odometer: 12345 km */
        0x3D                    /* checksum */
    };

    TEST_ASSERT_EQ(mock_uart_get_len(), (uint16_t)sizeof(expected));
    TEST_ASSERT_BYTES(mock_uart_get_buf(), expected, sizeof(expected));

    /* CMD byte must be 0x41 */
    TEST_ASSERT_EQ(mock_uart_get_buf()[1], 0x41);
    /* subcmd must be 0x02 */
    TEST_ASSERT_EQ(mock_uart_get_buf()[3], 0x02);

    return test_runner_result();
}
