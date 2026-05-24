/**
 * test_json_battery.c — JSON battery voltage output tests (0x230)
 */

#include "test_runner.h"
#include "test_helpers.h"
#include "../src/output/json_log.h"

int main(void)
{
    /* 12600 mV → 12.600 V */
    mock_usb_reset(); car_test_reset();
    car_test_set_voltage(12600);
    json_log_emit(50, 0x230);

    TEST_ASSERT_CONTAINS(mock_usb_get_buf(), "\"battery_v\":12.600");
    TEST_ASSERT_CONTAINS(mock_usb_get_buf(), "\"id\":\"0x230\"");
    TEST_ASSERT_CONTAINS(mock_usb_get_buf(), "\"ts\":50");
    TEST_ASSERT(mock_usb_get_buf()[mock_usb_get_len() - 1] == '\n');

    /* 14500 mV → 14.500 V (alternator charging) */
    mock_usb_reset(); car_test_reset();
    car_test_set_voltage(14500);
    json_log_emit(0, 0x230);
    TEST_ASSERT_CONTAINS(mock_usb_get_buf(), "\"battery_v\":14.500");

    return test_runner_result();
}
