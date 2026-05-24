/**
 * car.h — Public API for the vehicle car module.
 *
 * This header is the ONLY interface between the output drivers (Raise, JSON)
 * and the vehicle-specific decoder.  Raw CAN frame types MUST NOT appear here
 * or in any file that includes this header.  The API contract is:
 *
 *   - Output drivers call car_get_*() to read decoded signal state.
 *   - The car module (src/cars/) is the sole consumer of raw CAN data.
 *   - Swapping the car (e.g. Ford Focus → VW Golf) means replacing the .c
 *     file in src/cars/ — no other file changes.
 *
 * Signal conventions
 * ------------------
 *   speed      : km/h × 100  (uint16, e.g. 5050 = 50.50 km/h)
 *   wheel      : degrees × 10 (int16, -5400 to +5400, i.e. -540° to +540°)
 *   voltage    : millivolts (uint16, e.g. 12600 = 12.6 V)
 *   temp       : °C (int8, -40 to +85)
 *   odometer   : kilometres (uint32)
 *   distances  : 0 = closest obstacle, RADAR_DIST_CLEAR = no obstacle
 */

#ifndef CANMOD_CAR_H
#define CANMOD_CAR_H

#include <stdint.h>

/* -------------------------------------------------------------------------
 * Gear selector
 * ---------------------------------------------------------------------- */

typedef enum {
    e_selector_p = 0,   /**< Park    */
    e_selector_r = 1,   /**< Reverse */
    e_selector_n = 2,   /**< Neutral */
    e_selector_d = 3,   /**< Drive   */
} e_selector_t;

/* -------------------------------------------------------------------------
 * Parking radar
 * ---------------------------------------------------------------------- */

typedef enum {
    e_radar_undef = 0,  /**< Sensors not fitted or state unknown */
    e_radar_off   = 1,  /**< Sensors fitted but inactive         */
    e_radar_on    = 2,  /**< Sensors active                      */
} e_radar_state_t;

/** Number of PDC sensors: rear 0-3 (RL, RLM, RRM, RR), front 4-7 (FL, FLM, FRM, FR) */
#define RADAR_SENSOR_COUNT  8
/** Distance value indicating no obstacle detected */
#define RADAR_DIST_CLEAR   99

typedef struct {
    e_radar_state_t state;
    uint8_t dist[RADAR_SENSOR_COUNT]; /**< 0 = closest, RADAR_DIST_CLEAR = clear */
} radar_t;

/* -------------------------------------------------------------------------
 * Steering wheel control event
 * ---------------------------------------------------------------------- */

/** Raise protocol button IDs — must match the ATOTO S8 MS Raise mapping */
typedef enum {
    RAISE_SWC_VOL_UP   = 0x01,
    RAISE_SWC_VOL_DOWN = 0x02,
    RAISE_SWC_NEXT     = 0x03,
    RAISE_SWC_PREV     = 0x04,
    RAISE_SWC_MODE     = 0x05,
    RAISE_SWC_MUTE     = 0x06,
    RAISE_SWC_ANSWER   = 0x07,
    RAISE_SWC_HANGUP   = 0x08,
} raise_swc_id_t;

typedef struct {
    uint8_t button_id; /**< Raise button ID (raise_swc_id_t)    */
    uint8_t pressed;   /**< 1 = button pressed, 0 = released    */
} canmod_swc_event_t;

/* -------------------------------------------------------------------------
 * Powertrain / motion
 * ---------------------------------------------------------------------- */

/** Vehicle speed in km/h × 100 (e.g. 5000 = 50.00 km/h) */
uint16_t car_get_speed(void);

/** Engine RPM */
uint16_t car_get_taho(void);

/** Coolant temperature in °C */
int8_t car_get_temp(void);

/** Battery voltage in millivolts (e.g. 12600 = 12.6 V) */
uint16_t car_get_voltage(void);

/** Odometer reading in whole kilometres */
uint32_t car_get_odometer(void);

/* -------------------------------------------------------------------------
 * Selector and parking
 * ---------------------------------------------------------------------- */

e_selector_t car_get_selector(void);

/** 1 = park brake engaged, 0 = released */
uint8_t car_get_park_brake(void);

/* -------------------------------------------------------------------------
 * Body status
 * ---------------------------------------------------------------------- */

uint8_t car_get_door_fl(void);
uint8_t car_get_door_fr(void);
uint8_t car_get_door_rl(void);
uint8_t car_get_door_rr(void);
uint8_t car_get_tailgate(void);
uint8_t car_get_bonnet(void);

/* -------------------------------------------------------------------------
 * Lighting
 * ---------------------------------------------------------------------- */

/** 1 = sidelights or headlights on, 0 = off */
uint8_t car_get_near_lights(void);

/* -------------------------------------------------------------------------
 * HVAC / climate
 * ---------------------------------------------------------------------- */

uint8_t car_get_air_ac(void);            /**< 1 = compressor active           */
uint8_t car_get_air_fan(void);           /**< Fan speed 0–7 (Raise scale)     */
uint8_t car_get_air_temp_driver(void);   /**< Driver setpoint, °C × 2         */
uint8_t car_get_air_temp_pass(void);     /**< Passenger setpoint, °C × 2      */
uint8_t car_get_air_recirculation(void); /**< 1 = recirculating                */
uint8_t car_get_air_wind(void);          /**< 1 = windscreen vent on           */
uint8_t car_get_air_middle(void);        /**< 1 = middle vents on              */
uint8_t car_get_air_floor(void);         /**< 1 = floor vents on               */
uint8_t car_get_air_dual_zone(void);     /**< 1 = dual zone active             */

/* -------------------------------------------------------------------------
 * Steering
 * ---------------------------------------------------------------------- */

/** Steering angle in degrees × 10.  Negative = left.  Range: -5400 to +5400. */
int16_t car_get_wheel(void);

/* -------------------------------------------------------------------------
 * Parking sensors (PDC)
 * ---------------------------------------------------------------------- */

radar_t car_get_radar(void);

/* -------------------------------------------------------------------------
 * Steering wheel controls (SWC) — event queue
 * ---------------------------------------------------------------------- */

/**
 * Dequeue one SWC event.
 *
 * @param evt  Output: the dequeued event.
 * @return     1 if an event was available and written to *evt, 0 if empty.
 *
 * The queue holds 8 events.  If the queue is full when the car module tries
 * to enqueue, the oldest event is dropped (ring-buffer behaviour).
 */
int car_swc_dequeue(canmod_swc_event_t *evt);

/* -------------------------------------------------------------------------
 * Frame ingestion — called by the main loop, not by output drivers
 * ---------------------------------------------------------------------- */

/**
 * Process a single received CAN frame.
 *
 * The car module updates its internal signal state from the frame.
 * If the frame contains an SWC event it is enqueued for car_swc_dequeue().
 *
 * @param id    11-bit CAN identifier.
 * @param data  Frame payload (up to 8 bytes).
 * @param dlc   Data length code (0–8).
 */
void car_process_frame(uint32_t id, const uint8_t *data, uint8_t dlc);

#endif /* CANMOD_CAR_H */
