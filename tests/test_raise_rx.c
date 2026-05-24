/**
 * test_raise_rx.c — head-unit RX handler tests (raise_rx_poll)
 *
 * The ATOTO sends packets: 2E CMD LEN [DATA...] CSUM
 * raise_rx_poll() reads bytes from uart_hal_rx() and sends 0xFF ACK
 * after each valid packet.  Invalid checksum → no ACK.
 */

#include "test_runner.h"
#include "test_helpers.h"
#include "../src/output/raise.h"

static void test_valid_packet_triggers_ack(void)
{
    mock_uart_reset();

    /* Build a valid 2E CMD LEN CSUM packet (LEN=0, no data)
     * CMD=0x01, LEN=0x00 → CSUM=(0x01+0x00)^0xFF=0xFE */
    const uint8_t pkt[] = {0x2E, 0x01, 0x00, 0xFE};
    mock_uart_inject(pkt, sizeof(pkt));

    raise_rx_poll();

    /* One 0xFF ACK should have been transmitted */
    TEST_ASSERT_EQ(mock_uart_get_len(), 1u);
    TEST_ASSERT_EQ(mock_uart_get_buf()[0], 0xFF);
}

static void test_invalid_checksum_no_ack(void)
{
    mock_uart_reset();

    /* Same packet but with wrong CSUM (0x00 instead of 0xFE) */
    const uint8_t pkt[] = {0x2E, 0x01, 0x00, 0x00};
    mock_uart_inject(pkt, sizeof(pkt));

    raise_rx_poll();

    TEST_ASSERT_EQ(mock_uart_get_len(), 0u); /* no ACK */
}

static void test_packet_with_data(void)
{
    mock_uart_reset();

    /* 2E 20 02 01 01 CSUM where CSUM=(0x20+0x02+0x01+0x01)^0xFF=0xDB */
    const uint8_t pkt[] = {0x2E, 0x20, 0x02, 0x01, 0x01, 0xDB};
    mock_uart_inject(pkt, sizeof(pkt));

    raise_rx_poll();

    TEST_ASSERT_EQ(mock_uart_get_len(), 1u);
    TEST_ASSERT_EQ(mock_uart_get_buf()[0], 0xFF);
}

static void test_two_back_to_back_packets(void)
{
    mock_uart_reset();

    /* Two zero-data packets concatenated */
    const uint8_t pkts[] = {
        0x2E, 0x01, 0x00, 0xFE,   /* valid */
        0x2E, 0x02, 0x00, 0xFD,   /* valid, CMD=0x02 */
    };
    mock_uart_inject(pkts, sizeof(pkts));

    raise_rx_poll();

    /* Two ACKs */
    TEST_ASSERT_EQ(mock_uart_get_len(), 2u);
    TEST_ASSERT_EQ(mock_uart_get_buf()[0], 0xFF);
    TEST_ASSERT_EQ(mock_uart_get_buf()[1], 0xFF);
}

static void test_garbage_before_sof_is_skipped(void)
{
    mock_uart_reset();

    /* Some garbage bytes, then a valid packet */
    const uint8_t data[] = {
        0xAA, 0xBB, 0xCC,
        0x2E, 0x01, 0x00, 0xFE
    };
    mock_uart_inject(data, sizeof(data));

    raise_rx_poll();

    TEST_ASSERT_EQ(mock_uart_get_len(), 1u);
    TEST_ASSERT_EQ(mock_uart_get_buf()[0], 0xFF);
}

int main(void)
{
    test_valid_packet_triggers_ack();
    test_invalid_checksum_no_ack();
    test_packet_with_data();
    test_two_back_to_back_packets();
    test_garbage_before_sof_is_skipped();
    return test_runner_result();
}
