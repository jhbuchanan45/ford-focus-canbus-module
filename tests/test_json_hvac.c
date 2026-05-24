/**
 * test_json_hvac.c — JSON HVAC output tests (0x3B5)
 */

#include "test_runner.h"
#include "test_helpers.h"
#include "../src/output/json_log.h"

int main(void)
{
    /* ac_on=1, fan=3, temp_driver=43 (21.5°C), recirculation=0 etc. */
    mock_usb_reset(); car_test_reset();
    car_test_set_air_ac(1);
    car_test_set_air_fan(3);
    car_test_set_air_temp_driver(43); /* 21.5°C × 2 */
    car_test_set_air_temp_pass(40);   /* 20.0°C × 2 */
    car_test_set_air_recirculation(0);
    car_test_set_air_wind(1);
    car_test_set_air_middle(0);
    car_test_set_air_floor(0);
    car_test_set_air_dual_zone(0);

    json_log_emit(200, 0x3B5);

    TEST_ASSERT_CONTAINS(mock_usb_get_buf(), "\"ac_on\":1");
    TEST_ASSERT_CONTAINS(mock_usb_get_buf(), "\"fan_speed\":3");
    TEST_ASSERT_CONTAINS(mock_usb_get_buf(), "\"temp_driver\":21.5");
    TEST_ASSERT_CONTAINS(mock_usb_get_buf(), "\"temp_pass\":20.0");
    TEST_ASSERT_CONTAINS(mock_usb_get_buf(), "\"recirculation\":0");
    TEST_ASSERT_CONTAINS(mock_usb_get_buf(), "\"air_wind\":1");
    TEST_ASSERT_CONTAINS(mock_usb_get_buf(), "\"id\":\"0x3B5\"");
    TEST_ASSERT(mock_usb_get_buf()[mock_usb_get_len() - 1] == '\n');

    /* AC off */
    mock_usb_reset(); car_test_reset();
    json_log_emit(0, 0x3B5);
    TEST_ASSERT_CONTAINS(mock_usb_get_buf(), "\"ac_on\":0");

    return test_runner_result();
}
