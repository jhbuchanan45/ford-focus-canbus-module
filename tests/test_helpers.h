#ifndef CANMOD_TEST_HELPERS_H
#define CANMOD_TEST_HELPERS_H

/**
 * test_helpers.h — Test-only car state injection API
 *
 * These functions are compiled into ford_focus_mk3_2015.c only when
 * CANMOD_TESTING=1 is defined.  They give tests direct write access to the
 * static s_state without going through car_process_frame() (which relies on
 * decode stubs that are empty until task group 6).
 *
 * Usage:
 *   car_test_reset();                  // zero all state
 *   car_test_set_speed(5000);          // 50.00 km/h
 *   raise_send_vehicle_info();
 *   TEST_ASSERT_EQ(mock_uart_get_buf()[1], 0x41);
 */

#include "../src/car.h"

/* Reset all car state and the SWC queue to zero */
void car_test_reset(void);

/* Powertrain */
void car_test_set_speed(uint16_t v);
void car_test_set_taho(uint16_t v);
void car_test_set_temp(int8_t v);
void car_test_set_voltage(uint16_t v);
void car_test_set_odometer(uint32_t v);

/* Selector */
void car_test_set_selector(e_selector_t v);
void car_test_set_park_brake(uint8_t v);

/* Body */
void car_test_set_door_fl(uint8_t v);
void car_test_set_door_fr(uint8_t v);
void car_test_set_door_rl(uint8_t v);
void car_test_set_door_rr(uint8_t v);
void car_test_set_tailgate(uint8_t v);
void car_test_set_bonnet(uint8_t v);

/* Lighting */
void car_test_set_near_lights(uint8_t v);

/* HVAC */
void car_test_set_air_ac(uint8_t v);
void car_test_set_air_fan(uint8_t v);
void car_test_set_air_temp_driver(uint8_t v);
void car_test_set_air_temp_pass(uint8_t v);
void car_test_set_air_recirculation(uint8_t v);
void car_test_set_air_wind(uint8_t v);
void car_test_set_air_middle(uint8_t v);
void car_test_set_air_floor(uint8_t v);
void car_test_set_air_dual_zone(uint8_t v);

/* Steering */
void car_test_set_wheel(int16_t v);

/* Radar */
void car_test_set_radar_state(e_radar_state_t v);
void car_test_set_radar_dist(uint8_t idx, uint8_t v);

/* SWC queue — enqueue directly without a CAN frame */
void car_test_enqueue_swc(uint8_t button_id, uint8_t pressed);

/* ---- Mock HAL control (declared here for convenience) ---------------- */

/* UART mock */
void           mock_uart_reset(void);
const uint8_t *mock_uart_get_buf(void);
uint16_t       mock_uart_get_len(void);
void           mock_uart_inject(const uint8_t *bytes, uint16_t len);

/* USB mock */
void        mock_usb_reset(void);
const char *mock_usb_get_buf(void);
uint16_t    mock_usb_get_len(void);

#endif /* CANMOD_TEST_HELPERS_H */
