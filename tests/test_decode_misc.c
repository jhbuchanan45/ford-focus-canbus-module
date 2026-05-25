/**
 * test_decode_misc.c — Decode tests for remaining signals
 *
 * Covers: battery (0x230), odometer (0x072), coolant temp (0x420),
 *         lighting (0x4B0), gear selector (0x165), PDC (0x5C0), SWC (0x1A9).
 *
 * All byte layouts ⚠ unverified — validate with FORScan (tasks 6.2–6.9).
 */

#include "test_runner.h"
#include "test_helpers.h"

/* ---- Battery (0x230) -------------------------------------------------- */

static void test_battery_voltage(void)
{
    car_test_reset();
    /* 12.6 V → raw = 126 = 0x007E → voltage_mv = 12600 */
    uint8_t d[8] = {0x00, 0x7E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    car_process_frame(0x230, d, 8);
    TEST_ASSERT_EQ(car_get_voltage(), 12600u);
}

static void test_battery_zero(void)
{
    car_test_reset();
    uint8_t d[8] = {0};
    car_process_frame(0x230, d, 8);
    TEST_ASSERT_EQ(car_get_voltage(), 0u);
}

/* ---- Odometer (0x072) -------------------------------------------------- */

static void test_odometer(void)
{
    car_test_reset();
    /* 12345 km → d[1]=0x00, d[2]=0x30, d[3]=0x39 (24-bit big-endian) */
    uint8_t d[8] = {0x00, 0x00, 0x30, 0x39, 0x00, 0x00, 0x00, 0x00};
    car_process_frame(0x072, d, 8);
    TEST_ASSERT_EQ(car_get_odometer(), 12345u);
}

static void test_odometer_large(void)
{
    car_test_reset();
    /* 100000 km = 0x0186A0 → d[1]=0x01, d[2]=0x86, d[3]=0xA0 */
    uint8_t d[8] = {0x00, 0x01, 0x86, 0xA0, 0x00, 0x00, 0x00, 0x00};
    car_process_frame(0x072, d, 8);
    TEST_ASSERT_EQ(car_get_odometer(), 100000u);
}

/* ---- Coolant temperature (0x420) --------------------------------------- */

static void test_coolant_normal(void)
{
    car_test_reset();
    /* 90°C → raw = 90 + 40 = 130 = 0x82 */
    uint8_t d[8] = {0x82, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    car_process_frame(0x420, d, 8);
    TEST_ASSERT_EQ(car_get_temp(), 90);
}

static void test_coolant_cold(void)
{
    car_test_reset();
    /* -40°C → raw = 0 */
    uint8_t d[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    car_process_frame(0x420, d, 8);
    TEST_ASSERT_EQ(car_get_temp(), -40);
}

static void test_coolant_operating(void)
{
    car_test_reset();
    /* 87°C (typical operating temp) → raw = 127 = 0x7F */
    uint8_t d[8] = {0x7F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    car_process_frame(0x420, d, 8);
    TEST_ASSERT_EQ(car_get_temp(), 87);
}

/* ---- Lighting (0x4B0) -------------------------------------------------- */

static void test_lights_on(void)
{
    car_test_reset();
    uint8_t d[8] = {0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    car_process_frame(0x4B0, d, 8);
    TEST_ASSERT_EQ(car_get_near_lights(), 1u);
}

static void test_lights_off(void)
{
    car_test_reset();
    uint8_t d[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    car_process_frame(0x4B0, d, 8);
    TEST_ASSERT_EQ(car_get_near_lights(), 0u);
}

/* ---- Gear selector (0x165) --------------------------------------------- */

static void test_gear_reverse(void)
{
    car_test_reset();
    uint8_t d[8] = {0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    car_process_frame(0x165, d, 8);
    TEST_ASSERT_EQ(car_get_selector(), e_selector_r);
}

static void test_gear_drive(void)
{
    car_test_reset();
    uint8_t d[8] = {0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    car_process_frame(0x165, d, 8);
    TEST_ASSERT_EQ(car_get_selector(), e_selector_d);
}

static void test_gear_neutral(void)
{
    car_test_reset();
    uint8_t d[8] = {0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    car_process_frame(0x165, d, 8);
    TEST_ASSERT_EQ(car_get_selector(), e_selector_n);
}

static void test_gear_park(void)
{
    car_test_reset();
    uint8_t d[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    car_process_frame(0x165, d, 8);
    TEST_ASSERT_EQ(car_get_selector(), e_selector_p);
}

/* ---- PDC (0x5C0) ------------------------------------------------------- */

static void test_pdc_active_rear_close(void)
{
    car_test_reset();
    /* d[0] bit0=1 (active), rear sensors all at zone 6 (closest) */
    uint8_t d[8] = {0x01, 0x06, 0x06, 0x06, 0x06, 0x00, 0x00, 0x00};
    car_process_frame(0x5C0, d, 8);
    radar_t r = car_get_radar();
    TEST_ASSERT_EQ(r.state, e_radar_on);
    TEST_ASSERT_EQ(r.dist[0], 0u);  /* zone 6 → (6-6)*14 = 0 */
}

static void test_pdc_inactive(void)
{
    car_test_reset();
    uint8_t d[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    car_process_frame(0x5C0, d, 8);
    radar_t r = car_get_radar();
    TEST_ASSERT_EQ(r.state, e_radar_off);
    TEST_ASSERT_EQ(r.dist[0], RADAR_DIST_CLEAR);
}

static void test_pdc_clear_sensors(void)
{
    car_test_reset();
    /* d[0] active, all sensors zone 0 (no obstacle) → RADAR_DIST_CLEAR */
    uint8_t d[8] = {0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    car_process_frame(0x5C0, d, 8);
    radar_t r = car_get_radar();
    TEST_ASSERT_EQ(r.dist[0], RADAR_DIST_CLEAR);
    TEST_ASSERT_EQ(r.dist[1], RADAR_DIST_CLEAR);
    TEST_ASSERT_EQ(r.dist[2], RADAR_DIST_CLEAR);
    TEST_ASSERT_EQ(r.dist[3], RADAR_DIST_CLEAR);
}

/* ---- SWC (0x1A9) ------------------------------------------------------- */

static void test_swc_vol_up_press(void)
{
    car_test_reset();
    /* d[0]=0x01 (Vol+), d[1]=0x01 (press) */
    uint8_t d[8] = {0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    car_process_frame(0x1A9, d, 8);
    canmod_swc_event_t evt;
    TEST_ASSERT_EQ(car_swc_dequeue(&evt), 1);
    TEST_ASSERT_EQ(evt.button_id, RAISE_SWC_VOL_UP);
    TEST_ASSERT_EQ(evt.pressed, 1u);
}

static void test_swc_vol_up_release(void)
{
    car_test_reset();
    /* d[0]=0x01 (Vol+), d[1]=0x00 (release) */
    uint8_t d[8] = {0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    car_process_frame(0x1A9, d, 8);
    canmod_swc_event_t evt;
    TEST_ASSERT_EQ(car_swc_dequeue(&evt), 1);
    TEST_ASSERT_EQ(evt.button_id, RAISE_SWC_VOL_UP);
    TEST_ASSERT_EQ(evt.pressed, 0u);
}

static void test_swc_vol_down(void)
{
    car_test_reset();
    uint8_t d[8] = {0x02, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    car_process_frame(0x1A9, d, 8);
    canmod_swc_event_t evt;
    TEST_ASSERT_EQ(car_swc_dequeue(&evt), 1);
    TEST_ASSERT_EQ(evt.button_id, RAISE_SWC_VOL_DOWN);
}

static void test_swc_next(void)
{
    car_test_reset();
    uint8_t d[8] = {0x04, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    car_process_frame(0x1A9, d, 8);
    canmod_swc_event_t evt;
    TEST_ASSERT_EQ(car_swc_dequeue(&evt), 1);
    TEST_ASSERT_EQ(evt.button_id, RAISE_SWC_NEXT);
}

static void test_swc_prev(void)
{
    car_test_reset();
    uint8_t d[8] = {0x08, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    car_process_frame(0x1A9, d, 8);
    canmod_swc_event_t evt;
    TEST_ASSERT_EQ(car_swc_dequeue(&evt), 1);
    TEST_ASSERT_EQ(evt.button_id, RAISE_SWC_PREV);
}

static void test_swc_no_button(void)
{
    car_test_reset();
    /* d[0]=0x00: no button → nothing enqueued */
    uint8_t d[8] = {0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    car_process_frame(0x1A9, d, 8);
    canmod_swc_event_t evt;
    TEST_ASSERT_EQ(car_swc_dequeue(&evt), 0);
}

int main(void)
{
    /* Battery */
    test_battery_voltage();
    test_battery_zero();

    /* Odometer */
    test_odometer();
    test_odometer_large();

    /* Coolant temp */
    test_coolant_normal();
    test_coolant_cold();
    test_coolant_operating();

    /* Lighting */
    test_lights_on();
    test_lights_off();

    /* Gear selector */
    test_gear_reverse();
    test_gear_drive();
    test_gear_neutral();
    test_gear_park();

    /* PDC */
    test_pdc_active_rear_close();
    test_pdc_inactive();
    test_pdc_clear_sensors();

    /* SWC */
    test_swc_vol_up_press();
    test_swc_vol_up_release();
    test_swc_vol_down();
    test_swc_next();
    test_swc_prev();
    test_swc_no_button();

    return test_runner_result();
}
