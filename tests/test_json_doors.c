/**
 * test_json_doors.c — JSON door/body status output tests (0x540)
 */

#include "test_runner.h"
#include "test_helpers.h"
#include "../src/output/json_log.h"

int main(void)
{
    /* Only FL door open */
    mock_usb_reset(); car_test_reset();
    car_test_set_door_fl(1);
    json_log_emit(10, 0x540);

    TEST_ASSERT_CONTAINS(mock_usb_get_buf(), "\"door_fl\":1");
    TEST_ASSERT_CONTAINS(mock_usb_get_buf(), "\"door_fr\":0");
    TEST_ASSERT_CONTAINS(mock_usb_get_buf(), "\"door_rl\":0");
    TEST_ASSERT_CONTAINS(mock_usb_get_buf(), "\"door_rr\":0");
    TEST_ASSERT_CONTAINS(mock_usb_get_buf(), "\"tailgate\":0");
    TEST_ASSERT_CONTAINS(mock_usb_get_buf(), "\"bonnet\":0");
    TEST_ASSERT_CONTAINS(mock_usb_get_buf(), "\"id\":\"0x540\"");
    TEST_ASSERT(mock_usb_get_buf()[mock_usb_get_len() - 1] == '\n');

    /* All doors closed */
    mock_usb_reset(); car_test_reset();
    json_log_emit(0, 0x540);
    TEST_ASSERT_CONTAINS(mock_usb_get_buf(), "\"door_fl\":0");

    return test_runner_result();
}
