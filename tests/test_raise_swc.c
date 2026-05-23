/**
 * test_raise_swc.c — 0x20 SWC button packet tests
 *
 * Packet: 2E 20 02 <button_id> <pressed> CSUM
 * CSUM = (0x20 + 0x02 + button_id + pressed) ^ 0xFF
 *
 * Vol+ press:   2E 20 02 01 01 DB
 * Vol+ release: 2E 20 02 01 00 DC
 */

#include "test_runner.h"
#include "test_helpers.h"
#include "../src/output/raise.h"

static void check_swc(uint8_t button_id, uint8_t pressed)
{
    mock_uart_reset();
    raise_send_swc(button_id, pressed);

    uint8_t csum = (uint8_t)((0x20u + 0x02u + button_id + pressed) ^ 0xFFu);

    TEST_ASSERT_EQ(mock_uart_get_len(), 6u);
    TEST_ASSERT_EQ(mock_uart_get_buf()[0], 0x2E);
    TEST_ASSERT_EQ(mock_uart_get_buf()[1], 0x20);
    TEST_ASSERT_EQ(mock_uart_get_buf()[2], 0x02);
    TEST_ASSERT_EQ(mock_uart_get_buf()[3], button_id);
    TEST_ASSERT_EQ(mock_uart_get_buf()[4], pressed);
    TEST_ASSERT_EQ(mock_uart_get_buf()[5], csum);
}

int main(void)
{
    /* Vol+ press: 2E 20 02 01 01 DB */
    check_swc(RAISE_SWC_VOL_UP,   1);
    /* Vol+ release: 2E 20 02 01 00 DC */
    check_swc(RAISE_SWC_VOL_UP,   0);

    /* All other defined button IDs, press and release */
    check_swc(RAISE_SWC_VOL_DOWN, 1);
    check_swc(RAISE_SWC_VOL_DOWN, 0);
    check_swc(RAISE_SWC_NEXT,     1);
    check_swc(RAISE_SWC_NEXT,     0);
    check_swc(RAISE_SWC_PREV,     1);
    check_swc(RAISE_SWC_PREV,     0);
    check_swc(RAISE_SWC_MODE,     1);
    check_swc(RAISE_SWC_MODE,     0);
    check_swc(RAISE_SWC_MUTE,     1);
    check_swc(RAISE_SWC_MUTE,     0);
    check_swc(RAISE_SWC_ANSWER,   1);
    check_swc(RAISE_SWC_ANSWER,   0);
    check_swc(RAISE_SWC_HANGUP,   1);
    check_swc(RAISE_SWC_HANGUP,   0);

    return test_runner_result();
}
