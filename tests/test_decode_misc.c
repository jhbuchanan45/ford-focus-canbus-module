/**
 * test_decode_misc.c — Decode stubs for remaining signals
 *
 * Covers: battery (0x230), odometer (0x072), coolant temp (0x420),
 *         lighting (0x4B0), gear selector (0x165), PDC (0x5C0), SWC (0x1A9).
 *
 * STATUS: STUB — pending tasks 6.2, 6.3, 6.4, 6.5, 6.8, 6.9.
 */

#include "test_runner.h"
#include "test_helpers.h"

int main(void)
{
#if 0 /* TODO: tasks 6.2–6.9 */

    /* Battery voltage: 0x230, suspected scale 0.1V/bit → 12.6V = 126 = 0x007E */
    car_test_reset();
    uint8_t data_batt[8] = {0x00, 0x7E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    car_process_frame(0x230, data_batt, 8);
    TEST_ASSERT_EQ(car_get_voltage(), 12600u); /* mV */

    /* Odometer: 0x072 */
    car_test_reset();
    uint8_t data_odo[8] = {0x00, 0x00, 0x30, 0x39, 0x00, 0x00, 0x00, 0x00};
    car_process_frame(0x072, data_odo, 8);
    TEST_ASSERT_EQ(car_get_odometer(), 12345u);

    /* Coolant temp: 0x420, offset -40 → 90°C raw = 130 = 0x82 */
    car_test_reset();
    uint8_t data_temp[8] = {0x82, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    car_process_frame(0x420, data_temp, 8);
    TEST_ASSERT_EQ(car_get_temp(), 90);

    /* Near lights: 0x4B0 */
    car_test_reset();
    uint8_t data_lights[8] = {0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    car_process_frame(0x4B0, data_lights, 8);
    TEST_ASSERT_EQ(car_get_near_lights(), 1u);

    /* Gear selector: 0x165, reverse */
    car_test_reset();
    uint8_t data_reverse[8] = {0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    car_process_frame(0x165, data_reverse, 8);
    TEST_ASSERT_EQ(car_get_selector(), e_selector_r);

    /* PDC: 0x5C0 */
    car_test_reset();
    /* TODO: fill in once PDC confirmed on quad-lock (task 1.2) */

    /* SWC: 0x1A9 — Vol+ press should enqueue event */
    car_test_reset();
    uint8_t data_swc[8] = {0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    car_process_frame(0x1A9, data_swc, 8);
    canmod_swc_event_t evt;
    TEST_ASSERT_EQ(car_swc_dequeue(&evt), 1);
    TEST_ASSERT_EQ(evt.button_id, RAISE_SWC_VOL_UP);
    TEST_ASSERT_EQ(evt.pressed, 1u);

#endif /* TODO tasks 6.2–6.9 */

    g_test_count++;
    printf("SKIP: test_decode_misc (pending tasks 6.2–6.9)\n");
    return 0;
}
