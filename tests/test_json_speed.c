/**
 * test_json_speed.c — JSON speed/RPM output tests (0x217)
 */

#include "test_runner.h"
#include "test_helpers.h"
#include "../src/output/json_log.h"

int main(void)
{
    /* speed=5050 → 50.50 km/h */
    mock_usb_reset(); car_test_reset();
    car_test_set_speed(5050);
    car_test_set_taho(2000);
    json_log_emit(100, 0x217);

    TEST_ASSERT_CONTAINS(mock_usb_get_buf(), "\"speed_kmh\":50.50");
    TEST_ASSERT_CONTAINS(mock_usb_get_buf(), "\"rpm\":2000");
    TEST_ASSERT_CONTAINS(mock_usb_get_buf(), "\"id\":\"0x217\"");
    TEST_ASSERT_CONTAINS(mock_usb_get_buf(), "\"ts\":100");
    /* Must end with newline */
    TEST_ASSERT(mock_usb_get_len() > 0 &&
                mock_usb_get_buf()[mock_usb_get_len() - 1] == '\n');

    /* speed=0 → 0.00 km/h */
    mock_usb_reset(); car_test_reset();
    json_log_emit(0, 0x217);
    TEST_ASSERT_CONTAINS(mock_usb_get_buf(), "\"speed_kmh\":0.00");
    TEST_ASSERT_CONTAINS(mock_usb_get_buf(), "\"rpm\":0");

    return test_runner_result();
}
