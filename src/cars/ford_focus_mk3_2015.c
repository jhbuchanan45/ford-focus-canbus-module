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

/* Used by decode_swc() — suppressed until task 6.8 fills in the decode body */
__attribute__((unused))
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
 * ⚠ Signal decode helpers — STUB implementations
 *
 * Replace each stub body with real decode logic in task group 6 once the
 * CAN IDs and bit layouts are confirmed from the FORScan capture.
 * ---------------------------------------------------------------------- */

static void decode_speed_rpm(const uint8_t *d, uint8_t dlc)
{
    (void)dlc;
    /* TODO(task 6.1): decode vehicle speed and RPM from CAN_ID_SPEED_RPM.
     * Suspected layout (⚠ verify with FORScan):
     *   d[0..1] = raw speed, scale 0.01 km/h/bit → speed_kmh100 = (d[0]<<8|d[1])
     *   d[2..3] = raw RPM,   scale 0.25 RPM/bit  → rpm = (d[2]<<8|d[3]) / 4
     */
    (void)d;
}

static void decode_coolant_temp(const uint8_t *d, uint8_t dlc)
{
    (void)dlc;
    /* TODO(task 6.1): decode coolant temperature from CAN_ID_COOLANT_TEMP.
     * Suspected: d[0] = raw_temp, offset -40°C → temp_c = d[0] - 40
     */
    (void)d;
}

static void decode_battery(const uint8_t *d, uint8_t dlc)
{
    (void)dlc;
    /* TODO(task 6.2): decode battery voltage from CAN_ID_BATTERY.
     * Suspected: d[0..1] = raw_mv, scale 0.1 V/bit → voltage_mv = (d[0]<<8|d[1]) * 100
     */
    (void)d;
}

static void decode_odometer(const uint8_t *d, uint8_t dlc)
{
    (void)dlc;
    /* TODO(task 6.2): decode odometer from CAN_ID_ODOMETER. */
    (void)d;
}

static void decode_doors(const uint8_t *d, uint8_t dlc)
{
    (void)dlc;
    /* TODO(task 6.3): decode door / body / park-brake status from CAN_ID_DOORS.
     * Suspected: d[0] bit-field
     *   bit0=door_fl, bit1=door_fr, bit2=door_rl, bit3=door_rr,
     *   bit4=tailgate, bit5=bonnet, bit6=park_brake
     */
    (void)d;
}

static void decode_lighting(const uint8_t *d, uint8_t dlc)
{
    (void)dlc;
    /* TODO(task 6.4): decode near-lights status from CAN_ID_LIGHTING. */
    (void)d;
}

static void decode_gear_selector(const uint8_t *d, uint8_t dlc)
{
    (void)dlc;
    /* TODO(task 6.5): decode gear selector / reverse from CAN_ID_GEAR_SELECTOR. */
    (void)d;
}

static void decode_hvac(const uint8_t *d, uint8_t dlc)
{
    (void)dlc;
    /* TODO(task 6.6): decode HVAC state from CAN_ID_HVAC (0x3B5).
     * Known reference signal: cansend vcan0 3B5#0300002B00000000
     * Suspected layout (⚠ verify):
     *   d[0] bit2 = ac_on
     *   d[1]      = fan_speed raw (0–15), Raise scale = raw / 2
     *   d[2]      = driver_temp × 2
     *   d[3]      = passenger_temp × 2
     *   d[0] bit0 = recirculation
     *   d[4] bits = airflow
     *   d[0] bit1 = dual_zone
     */
    (void)d;
}

static void decode_steering(const uint8_t *d, uint8_t dlc)
{
    (void)dlc;
    /* TODO(task 6.7): decode steering angle from CAN_ID_EPAS_STEERING.
     * Suspected: d[0..1] signed int16, scale 0.1 deg/bit → wheel = (int16)((d[0]<<8)|d[1])
     */
    (void)d;
}

static void decode_pdc(const uint8_t *d, uint8_t dlc)
{
    (void)dlc;
    /* TODO(task 6.9): decode PDC distances from CAN_ID_PDC.
     * ⚠ Verify PDC is present on quad-lock (task 1.2) before implementing.
     */
    (void)d;
}

static void decode_swc(const uint8_t *d, uint8_t dlc)
{
    (void)dlc;
    /* TODO(task 6.8): map Ford SWC byte values to Raise button IDs.
     * ⚠ Verify actual byte values with FORScan (task 1.2).
     *
     * Suspected layout:
     *   d[0] = button_raw (0x00 = none)
     *   d[1] = state (0x01 = press, 0x00 = release)
     *
     * Example mapping (UNVERIFIED — fill in real values after FORScan):
     *   0x01 → RAISE_SWC_VOL_UP
     *   0x02 → RAISE_SWC_VOL_DOWN
     *   0x04 → RAISE_SWC_NEXT
     *   0x08 → RAISE_SWC_PREV
     */
    (void)d;
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
