/**
 * test_swc_queue.c — SWC ring buffer unit tests
 *
 * Tests the 8-entry drop-oldest ring buffer implemented in
 * ford_focus_mk3_2015.c via the car_swc_dequeue() / car_test_enqueue_swc()
 * public interface.
 *
 * Run: ctest -R swc
 */

#include "test_runner.h"
#include "test_helpers.h"

/* ---- Test cases -------------------------------------------------------- */

static void test_dequeue_empty_returns_zero(void)
{
    car_test_reset();
    canmod_swc_event_t evt;
    TEST_ASSERT_EQ(car_swc_dequeue(&evt), 0);
}

static void test_single_roundtrip(void)
{
    car_test_reset();
    car_test_enqueue_swc(RAISE_SWC_VOL_UP, 1);

    canmod_swc_event_t evt;
    TEST_ASSERT_EQ(car_swc_dequeue(&evt), 1);
    TEST_ASSERT_EQ(evt.button_id, RAISE_SWC_VOL_UP);
    TEST_ASSERT_EQ(evt.pressed,   1);
    /* Queue should now be empty */
    TEST_ASSERT_EQ(car_swc_dequeue(&evt), 0);
}

static void test_fifo_order_four_events(void)
{
    car_test_reset();
    car_test_enqueue_swc(0x01, 1);
    car_test_enqueue_swc(0x02, 0);
    car_test_enqueue_swc(0x03, 1);
    car_test_enqueue_swc(0x04, 0);

    canmod_swc_event_t evt;

    car_swc_dequeue(&evt);
    TEST_ASSERT_EQ(evt.button_id, 0x01); TEST_ASSERT_EQ(evt.pressed, 1);

    car_swc_dequeue(&evt);
    TEST_ASSERT_EQ(evt.button_id, 0x02); TEST_ASSERT_EQ(evt.pressed, 0);

    car_swc_dequeue(&evt);
    TEST_ASSERT_EQ(evt.button_id, 0x03); TEST_ASSERT_EQ(evt.pressed, 1);

    car_swc_dequeue(&evt);
    TEST_ASSERT_EQ(evt.button_id, 0x04); TEST_ASSERT_EQ(evt.pressed, 0);

    TEST_ASSERT_EQ(car_swc_dequeue(&evt), 0);
}

static void test_ninth_enqueue_drops_oldest(void)
{
    car_test_reset();
    /* Fill the 8-entry queue */
    for (uint8_t i = 1; i <= 8; i++)
        car_test_enqueue_swc(i, 1);

    /* 9th enqueue — entry 1 (button_id=1) must be dropped, entry 9 stored */
    car_test_enqueue_swc(9, 0);

    canmod_swc_event_t evt;
    /* First dequeue should now be entry 2, not entry 1 */
    car_swc_dequeue(&evt);
    TEST_ASSERT_EQ(evt.button_id, 2);

    /* Drain entries 3–8 */
    for (uint8_t i = 3; i <= 8; i++) {
        car_swc_dequeue(&evt);
        TEST_ASSERT_EQ(evt.button_id, i);
    }

    /* Last entry should be the 9th (button_id=9) */
    TEST_ASSERT_EQ(car_swc_dequeue(&evt), 1);
    TEST_ASSERT_EQ(evt.button_id, 9);
    TEST_ASSERT_EQ(evt.pressed,   0);

    /* Queue is empty */
    TEST_ASSERT_EQ(car_swc_dequeue(&evt), 0);
}

static void test_drain_and_refill_wrap(void)
{
    car_test_reset();
    /* Fill and drain twice to force wrap-around */
    for (uint8_t round = 0; round < 2; round++) {
        for (uint8_t i = 0; i < 8; i++)
            car_test_enqueue_swc(i, round);

        canmod_swc_event_t evt;
        for (uint8_t i = 0; i < 8; i++) {
            TEST_ASSERT_EQ(car_swc_dequeue(&evt), 1);
            TEST_ASSERT_EQ(evt.button_id, i);
            TEST_ASSERT_EQ(evt.pressed,   round);
        }
        TEST_ASSERT_EQ(car_swc_dequeue(&evt), 0);
    }

    /* Now enqueue 4 more and verify */
    for (uint8_t i = 10; i < 14; i++)
        car_test_enqueue_swc(i, 1);

    canmod_swc_event_t evt;
    for (uint8_t i = 10; i < 14; i++) {
        TEST_ASSERT_EQ(car_swc_dequeue(&evt), 1);
        TEST_ASSERT_EQ(evt.button_id, i);
    }
    TEST_ASSERT_EQ(car_swc_dequeue(&evt), 0);
}

/* ---- Entry point ------------------------------------------------------- */

int main(void)
{
    test_dequeue_empty_returns_zero();
    test_single_roundtrip();
    test_fifo_order_four_events();
    test_ninth_enqueue_drops_oldest();
    test_drain_and_refill_wrap();
    return test_runner_result();
}
