/**
 * test_json_unknown.c — Unknown frame ID produces no JSON output
 */

#include "test_runner.h"
#include "test_helpers.h"
#include "../src/output/json_log.h"

int main(void)
{
    mock_usb_reset(); car_test_reset();
    json_log_emit(100, 0xABC); /* unknown ID */
    TEST_ASSERT_EQ(mock_usb_get_len(), 0u);

    mock_usb_reset();
    json_log_emit(100, 0x000); /* also unknown */
    TEST_ASSERT_EQ(mock_usb_get_len(), 0u);

    /* Verify known IDs do produce output (sanity check) */
    mock_usb_reset();
    json_log_emit(100, 0x217);
    TEST_ASSERT(mock_usb_get_len() > 0u);

    return test_runner_result();
}
