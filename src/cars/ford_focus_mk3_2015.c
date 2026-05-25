/**
 * ford_focus_mk3_2015.c — Ford Focus Mk3 2015 MS-CAN decoder
 *
 * This is the ONLY file that reads raw CAN frame data.  All other layers
 * use the car_get_*() API defined in car.h.
 *
 * Status
 * ------
 * All signal decode logic is STUBBED pending FORScan verification (tasks
 * 1.1–1.2).  Each CAN ID marked ⚠ is a community-researched estimate;
 * update the CAN_ID_* constants and the decode functions in task group 6
 * once the real IDs and bit layouts are confirmed.
 *
 * Safe defaults are returned by all car_get_*() functions so that the full
 * stack (HAL → car module → output driver) can be built and exercised
 * end-to-end before the vehicle is available.
 */

#include "car.h"
#include <string.h>

/* -------------------------------------------------------------------------
 * ⚠ MS-CAN message IDs — all unverified, update after FORScan session
 * ---------------------------------------------------------------------- */

#define CAN_ID_EPAS_STEERING    0x080u  /* ⚠ EPAS steering angle              */
#define CAN_ID_ODOMETER         0x072u  /* ⚠ BCM odometer                     */
#define CAN_ID_SWC              0x1A9u  /* ⚠ Steering wheel controls          */
#define CAN_ID_SPEED_RPM        0x217u  /* ⚠ GEM-mirrored speed + RPM         */
#define CAN_ID_BATTERY          0x230u  /* ⚠ Battery voltage                  */
#define CAN_ID_GEAR_SELECTOR    0x165u  /* ⚠ TCM gear selector (alt: 0x336)   */
#define CAN_ID_HVAC             0x3B5u  /* ⚠ HVAC module climate state        */
#define CAN_ID_COOLANT_TEMP     0x420u  /* ⚠ GEM-mirrored coolant temperature */
#define CAN_ID_LIGHTING         0x4B0u  /* ⚠ BCM lighting status              */
#define CAN_ID_DOORS            0x540u  /* ⚠ BCM door / body status           */
#define CAN_ID_PDC              0x5C0u  /* ⚠ Parking aid module (PDC)         */

/* -------------------------------------------------------------------------
 * Internal signal state
 * ---------------------------------------------------------------------- */

static struct {
    /* Powertrain */
    uint16_t speed;         /* km/h × 100 */
    uint16_t rpm;
    int8_t   coolant_temp;
    uint16_t voltage_mv;
    uint32_t odometer_km;

    /* Selector */
    e_selector_t selector;
    uint8_t park_brake;

    /* Body */
    uint8_t door_fl, door_fr, door_rl, door_rr;
    uint8_t tailgate, bonnet;

    /* Lighting */
    uint8_t near_lights;

    /* HVAC */
    uint8_t ac_on;
    uint8_t fan_speed;      /* 0–7, Raise scale */
    uint8_t temp_driver;    /* °C × 2 */
    uint8_t temp_pass;      /* °C × 2 */
    uint8_t recirculation;
    uint8_t air_wind;
    uint8_t air_middle;
    uint8_t air_floor;
    uint8_t dual_zone;

    /* Steering */
    int16_t wheel;          /* degrees × 10 */

    /* PDC */
    radar_t radar;
} s_state;

/* -------------------------------------------------------------------------
 * SWC event ring buffer (8 entries, drop-oldest on overflow — task 3.3)
 * ---------------------------------------------------------------------- */

#define SWC_FIFO_SIZE 8u

static struct {
    canmod_swc_event_t buf[SWC_FIFO_SIZE];
    uint8_t head;   /* index of next slot to write */
    uint8_t tail;   /* index of next slot to read  */
    uint8_t count;
} s_swc;

static void swc_enqueue(uint8_t button_id, uint8_t pressed)
{
    if (s_swc.count == SWC_FIFO_SIZE) {
        /* Queue full — drop oldest entry */
        s_swc.tail = (s_swc.tail + 1u) % SWC_FIFO_SIZE;
        s_swc.count--;
    }
    s_swc.buf[s_swc.head].button_id = button_id;
    s_swc.buf[s_swc.head].pressed   = pressed;
    s_swc.head = (s_swc.head + 1u) % SWC_FIFO_SIZE;
    s_swc.count++;
}

int car_swc_dequeue(canmod_swc_event_t *evt)
{
    if (s_swc.count == 0u) {
        return 0;
    }
    *evt = s_swc.buf[s_swc.tail];
    s_swc.tail = (s_swc.tail + 1u) % SWC_FIFO_SIZE;
    s_swc.count--;
    return 1;
}

/* -------------------------------------------------------------------------
 * Signal decode helpers
 *
 * All byte layouts are ⚠ community-researched estimates for the Ford Focus
 * Mk3 2015 MS-CAN.  Each function is marked with the task that will confirm
 * or correct the layout via FORScan capture (task 6.x).
 *
 * DLC guards prevent reads past the end of short frames.
 * ---------------------------------------------------------------------- */

/**
 * 0x217 — Speed + RPM  (task 6.1)
 *
 * ⚠ Layout (confirm with FORScan):
 *   d[0..1] big-endian uint16: vehicle speed in 0.01 km/h per bit
 *           → already in our km/h×100 internal unit
 *   d[2..3] big-endian uint16: engine RPM in 0.25 RPM per bit
 *           → rpm = raw >> 2
 */
static void decode_speed_rpm(const uint8_t *d, uint8_t dlc)
{
    if (dlc < 4u) return;
    s_state.speed = (uint16_t)(((uint16_t)d[0] << 8) | d[1]);
    s_state.rpm   = (uint16_t)((((uint16_t)d[2] << 8) | d[3]) >> 2);
}

/**
 * 0x420 — Coolant temperature  (task 6.1)
 *
 * ⚠ d[0]: raw temp byte, offset -40 °C
 *   temp_c = (int8_t)d[0] - 40
 *   Range of d[0]: 0 (→ -40°C) to 167 (→ 127°C)
 */
static void decode_coolant_temp(const uint8_t *d, uint8_t dlc)
{
    if (dlc < 1u) return;
    int16_t t = (int16_t)d[0] - 40;
    /* Clamp to int8_t range for safety */
    s_state.coolant_temp = (t < -128) ? (int8_t)-128 :
                           (t >  127) ? (int8_t) 127  : (int8_t)t;
}

/**
 * 0x230 — Battery voltage  (task 6.2)
 *
 * ⚠ d[0..1] big-endian uint16: voltage in 0.1 V per bit
 *   voltage_mv = raw × 100
 *   e.g. 12.6 V → raw = 126 → voltage_mv = 12600
 */
static void decode_battery(const uint8_t *d, uint8_t dlc)
{
    if (dlc < 2u) return;
    uint16_t raw = (uint16_t)(((uint16_t)d[0] << 8) | d[1]);
    s_state.voltage_mv = (uint16_t)(raw * 100u);
}

/**
 * 0x072 — Odometer  (task 6.2)
 *
 * ⚠ d[1..3]: 24-bit big-endian odometer in whole kilometres
 *   Range: 0 to 16,777,215 km
 */
static void decode_odometer(const uint8_t *d, uint8_t dlc)
{
    if (dlc < 4u) return;
    s_state.odometer_km = ((uint32_t)d[1] << 16) |
                          ((uint32_t)d[2] <<  8) |
                           (uint32_t)d[3];
}

/**
 * 0x540 — Door / body / park brake status  (task 6.3)
 *
 * ⚠ d[0] bit-field:
 *   bit 0 = door_fl
 *   bit 1 = door_fr
 *   bit 2 = door_rl
 *   bit 3 = door_rr
 *   bit 4 = tailgate (boot lid)
 *   bit 5 = bonnet (hood)
 *   bit 6 = park brake engaged
 */
static void decode_doors(const uint8_t *d, uint8_t dlc)
{
    if (dlc < 1u) return;
    s_state.door_fl    = (d[0] >> 0) & 1u;
    s_state.door_fr    = (d[0] >> 1) & 1u;
    s_state.door_rl    = (d[0] >> 2) & 1u;
    s_state.door_rr    = (d[0] >> 3) & 1u;
    s_state.tailgate   = (d[0] >> 4) & 1u;
    s_state.bonnet     = (d[0] >> 5) & 1u;
    s_state.park_brake = (d[0] >> 6) & 1u;
}

/**
 * 0x4B0 — Lighting status  (task 6.4)
 *
 * ⚠ d[0] bit 0: near lights on (sidelights or headlights)
 */
static void decode_lighting(const uint8_t *d, uint8_t dlc)
{
    if (dlc < 1u) return;
    s_state.near_lights = (d[0] >> 0) & 1u;
}

/**
 * 0x165 — Gear selector  (task 6.5)
 *
 * ⚠ d[0]: gear position byte
 *   0x00 = P (park / default)
 *   0x01 = R (reverse)
 *   0x02 = N (neutral)
 *   0x03+ = D (drive or sport/manual modes)
 *
 * Ford PowerShift may use different nibble encoding — verify with FORScan.
 */
static void decode_gear_selector(const uint8_t *d, uint8_t dlc)
{
    if (dlc < 1u) return;
    switch (d[0]) {
    case 0x01: s_state.selector = e_selector_r; break;
    case 0x02: s_state.selector = e_selector_n; break;
    case 0x03: s_state.selector = e_selector_d; break;
    case 0x00:
    default:   s_state.selector = e_selector_p; break;
    }
}

/**
 * 0x3B5 — HVAC / climate  (task 6.6)
 *
 * ⚠ Layout (confirm with FORScan; reference: 3B5#0300002B00000000):
 *   d[0] bit 0 = AC compressor on
 *   d[0] bit 1 = recirculation
 *   d[0] bit 2 = dual zone active
 *   d[1] bits 3..0 = fan speed raw (0–15) → Raise 0–7 (divide by 2)
 *   d[2] = driver setpoint temperature (°C × 2)
 *   d[3] = passenger setpoint temperature (°C × 2)
 *   d[4] bit 0 = windscreen vent
 *   d[4] bit 1 = middle vents
 *   d[4] bit 2 = floor vents
 */
static void decode_hvac(const uint8_t *d, uint8_t dlc)
{
    if (dlc < 4u) return;
    s_state.ac_on         = (d[0] >> 0) & 1u;
    s_state.recirculation = (d[0] >> 1) & 1u;
    s_state.dual_zone     = (d[0] >> 2) & 1u;
    s_state.fan_speed     = (d[1] & 0x0Fu) >> 1u;  /* 0–15 raw → 0–7 Raise */
    s_state.temp_driver   = d[2];
    s_state.temp_pass     = d[3];
    if (dlc >= 5u) {
        s_state.air_wind   = (d[4] >> 0) & 1u;
        s_state.air_middle = (d[4] >> 1) & 1u;
        s_state.air_floor  = (d[4] >> 2) & 1u;
    }
}

/**
 * 0x080 — EPAS steering angle  (task 6.7)
 *
 * ⚠ d[0..1]: signed int16 big-endian, scale 0.1 deg per bit
 *   → already matches our internal unit (deg × 10)
 *   Range: -5400 to +5400 (±540°)
 */
static void decode_steering(const uint8_t *d, uint8_t dlc)
{
    if (dlc < 2u) return;
    s_state.wheel = (int16_t)(((uint16_t)d[0] << 8) | d[1]);
}

/**
 * 0x5C0 — PDC parking sensor distances  (task 6.9)
 *
 * ⚠ Verify PDC is present on quad-lock MS-CAN before relying on this.
 *
 * Suspected layout:
 *   d[0] bit 0: sensors active (1 = on, 0 = off/inactive)
 *   d[1..4]: rear sensors RL, RLM, RRM, RR
 *   d[5..8]: front sensors FL, FLM, FRM, FR (if 8-sensor variant)
 *
 * Ford distance zone encoding (⚠ verify):
 *   0 = no obstacle (clear)  → our RADAR_DIST_CLEAR (99)
 *   1 = farthest zone        → our ~84
 *   ...
 *   6 = closest zone (<20cm) → our ~0
 *
 * Conversion: ford 0 → 99; ford n (1-6) → (6-n) × 14
 */
static uint8_t pdc_zone_to_dist(uint8_t zone)
{
    if (zone == 0u) return RADAR_DIST_CLEAR;
    if (zone > 6u)  zone = 6u;
    return (uint8_t)((6u - zone) * 14u);
}

static void decode_pdc(const uint8_t *d, uint8_t dlc)
{
    if (dlc < 1u) return;
    s_state.radar.state = ((d[0] & 1u) != 0u) ? e_radar_on : e_radar_off;

    /* Rear sensors: d[1..4] */
    for (uint8_t i = 0u; i < 4u; i++) {
        s_state.radar.dist[i] = ((uint8_t)(i + 1u) < dlc)
                                ? pdc_zone_to_dist(d[i + 1u])
                                : RADAR_DIST_CLEAR;
    }
    /* Front sensors: d[5..8] */
    for (uint8_t i = 0u; i < 4u; i++) {
        s_state.radar.dist[4u + i] = ((uint8_t)(i + 5u) < dlc)
                                     ? pdc_zone_to_dist(d[i + 5u])
                                     : RADAR_DIST_CLEAR;
    }
}

/**
 * 0x1A9 — Steering wheel controls  (task 6.8)
 *
 * ⚠ Layout (confirm with FORScan):
 *   d[0]: Ford button code (0x00 = no button)
 *   d[1]: 0x01 = pressed, 0x00 = released
 *
 * Ford → Raise button ID mapping (⚠ verify raw values with FORScan):
 *   0x01 → VOL_UP    0x02 → VOL_DOWN
 *   0x04 → NEXT      0x08 → PREV
 *   0x10 → MODE      0x20 → MUTE
 *   0x40 → ANSWER    0x80 → HANGUP
 */
static uint8_t ford_swc_to_raise(uint8_t btn)
{
    switch (btn) {
    case 0x01: return RAISE_SWC_VOL_UP;
    case 0x02: return RAISE_SWC_VOL_DOWN;
    case 0x04: return RAISE_SWC_NEXT;
    case 0x08: return RAISE_SWC_PREV;
    case 0x10: return RAISE_SWC_MODE;
    case 0x20: return RAISE_SWC_MUTE;
    case 0x40: return RAISE_SWC_ANSWER;
    case 0x80: return RAISE_SWC_HANGUP;
    default:   return 0u;
    }
}

static void decode_swc(const uint8_t *d, uint8_t dlc)
{
    if (dlc < 2u) return;
    uint8_t btn_raw = d[0];
    uint8_t pressed = d[1] & 1u;

    if (btn_raw == 0u) return;         /* no button active */
    uint8_t raise_id = ford_swc_to_raise(btn_raw);
    if (raise_id == 0u) return;        /* unmapped button — ignore */

    swc_enqueue(raise_id, pressed);
}

/* -------------------------------------------------------------------------
 * Frame dispatch
 * ---------------------------------------------------------------------- */

void car_process_frame(uint32_t id, const uint8_t *data, uint8_t dlc)
{
    switch (id) {
    case CAN_ID_SPEED_RPM:      decode_speed_rpm(data, dlc);      break;
    case CAN_ID_COOLANT_TEMP:   decode_coolant_temp(data, dlc);   break;
    case CAN_ID_BATTERY:        decode_battery(data, dlc);         break;
    case CAN_ID_ODOMETER:       decode_odometer(data, dlc);        break;
    case CAN_ID_DOORS:          decode_doors(data, dlc);           break;
    case CAN_ID_LIGHTING:       decode_lighting(data, dlc);        break;
    case CAN_ID_GEAR_SELECTOR:  decode_gear_selector(data, dlc);   break;
    case CAN_ID_HVAC:           decode_hvac(data, dlc);            break;
    case CAN_ID_EPAS_STEERING:  decode_steering(data, dlc);        break;
    case CAN_ID_PDC:            decode_pdc(data, dlc);             break;
    case CAN_ID_SWC:            decode_swc(data, dlc);             break;
    default:                    /* unknown frame — ignore */        break;
    }
}

/* -------------------------------------------------------------------------
 * car_get_* API — returns internal state (safe defaults until task 6.x)
 * ---------------------------------------------------------------------- */

uint16_t car_get_speed(void)            { return s_state.speed; }
uint16_t car_get_taho(void)             { return s_state.rpm; }
int8_t   car_get_temp(void)             { return s_state.coolant_temp; }
uint16_t car_get_voltage(void)          { return s_state.voltage_mv; }
uint32_t car_get_odometer(void)         { return s_state.odometer_km; }

e_selector_t car_get_selector(void)     { return s_state.selector; }
uint8_t car_get_park_brake(void)        { return s_state.park_brake; }

uint8_t car_get_door_fl(void)           { return s_state.door_fl; }
uint8_t car_get_door_fr(void)           { return s_state.door_fr; }
uint8_t car_get_door_rl(void)           { return s_state.door_rl; }
uint8_t car_get_door_rr(void)           { return s_state.door_rr; }
uint8_t car_get_tailgate(void)          { return s_state.tailgate; }
uint8_t car_get_bonnet(void)            { return s_state.bonnet; }

uint8_t car_get_near_lights(void)       { return s_state.near_lights; }

uint8_t car_get_air_ac(void)            { return s_state.ac_on; }
uint8_t car_get_air_fan(void)           { return s_state.fan_speed; }
uint8_t car_get_air_temp_driver(void)   { return s_state.temp_driver; }
uint8_t car_get_air_temp_pass(void)     { return s_state.temp_pass; }
uint8_t car_get_air_recirculation(void) { return s_state.recirculation; }
uint8_t car_get_air_wind(void)          { return s_state.air_wind; }
uint8_t car_get_air_middle(void)        { return s_state.air_middle; }
uint8_t car_get_air_floor(void)         { return s_state.air_floor; }
uint8_t car_get_air_dual_zone(void)     { return s_state.dual_zone; }

int16_t car_get_wheel(void)             { return s_state.wheel; }

radar_t car_get_radar(void)             { return s_state.radar; }

/* -------------------------------------------------------------------------
 * Test-only API — compiled only when CANMOD_TESTING is defined.
 * Provides direct state injection so unit tests can set up known car state
 * without needing real CAN frames (decode stubs are all no-ops until 6.x).
 * ---------------------------------------------------------------------- */

#ifdef CANMOD_TESTING

void car_test_reset(void)
{
    memset(&s_state, 0, sizeof(s_state));
    s_swc.head  = 0;
    s_swc.tail  = 0;
    s_swc.count = 0;
}

void car_test_set_speed(uint16_t v)          { s_state.speed       = v; }
void car_test_set_taho(uint16_t v)           { s_state.rpm         = v; }
void car_test_set_temp(int8_t v)             { s_state.coolant_temp = v; }
void car_test_set_voltage(uint16_t v)        { s_state.voltage_mv  = v; }
void car_test_set_odometer(uint32_t v)       { s_state.odometer_km = v; }

void car_test_set_selector(e_selector_t v)   { s_state.selector    = v; }
void car_test_set_park_brake(uint8_t v)      { s_state.park_brake  = v; }

void car_test_set_door_fl(uint8_t v)         { s_state.door_fl     = v; }
void car_test_set_door_fr(uint8_t v)         { s_state.door_fr     = v; }
void car_test_set_door_rl(uint8_t v)         { s_state.door_rl     = v; }
void car_test_set_door_rr(uint8_t v)         { s_state.door_rr     = v; }
void car_test_set_tailgate(uint8_t v)        { s_state.tailgate    = v; }
void car_test_set_bonnet(uint8_t v)          { s_state.bonnet      = v; }

void car_test_set_near_lights(uint8_t v)     { s_state.near_lights = v; }

void car_test_set_air_ac(uint8_t v)          { s_state.ac_on          = v; }
void car_test_set_air_fan(uint8_t v)         { s_state.fan_speed       = v; }
void car_test_set_air_temp_driver(uint8_t v) { s_state.temp_driver     = v; }
void car_test_set_air_temp_pass(uint8_t v)   { s_state.temp_pass       = v; }
void car_test_set_air_recirculation(uint8_t v){ s_state.recirculation  = v; }
void car_test_set_air_wind(uint8_t v)        { s_state.air_wind        = v; }
void car_test_set_air_middle(uint8_t v)      { s_state.air_middle      = v; }
void car_test_set_air_floor(uint8_t v)       { s_state.air_floor       = v; }
void car_test_set_air_dual_zone(uint8_t v)   { s_state.dual_zone       = v; }

void car_test_set_wheel(int16_t v)           { s_state.wheel           = v; }

void car_test_set_radar_state(e_radar_state_t v) { s_state.radar.state = v; }
void car_test_set_radar_dist(uint8_t idx, uint8_t v)
{
    if (idx < RADAR_SENSOR_COUNT)
        s_state.radar.dist[idx] = v;
}

/** Directly enqueue an SWC event (bypasses the CAN decode path). */
void car_test_enqueue_swc(uint8_t button_id, uint8_t pressed)
{
    swc_enqueue(button_id, pressed);
}

#endif /* CANMOD_TESTING */
