/**
 * raise.c — Raise VW PQ UART output driver
 *
 * Implements all nine packet types required by the ATOTO S8 MS when
 * configured to the Raise / RZC FD60 protocol.
 *
 * Packet framing
 * --------------
 *   [0x2E] SOF
 *   [CMD]  command byte
 *   [LEN]  number of DATA bytes (does NOT include CSUM)
 *   [DATA] payload (LEN bytes)
 *   [CSUM] = (CMD + LEN + sum(DATA)) ^ 0xFF  (8-bit, wraps naturally)
 *
 * Reference: smartgauges/canbox, canbox_raise_vw.c
 */

#include "raise.h"
#include "../car.h"
#include "../hal/uart_hal.h"

#include <stdint.h>
#include <string.h>

/* -------------------------------------------------------------------------
 * Low-level framing
 * ---------------------------------------------------------------------- */

static void raise_send(uint8_t cmd, const uint8_t *data, uint8_t len)
{
    uint8_t csum = cmd + len;
    for (uint8_t i = 0; i < len; i++) csum += data[i];
    csum ^= 0xFF;

    uart_hal_tx(0x2E);
    uart_hal_tx(cmd);
    uart_hal_tx(len);
    for (uint8_t i = 0; i < len; i++) uart_hal_tx(data[i]);
    uart_hal_tx(csum);
}

/* -------------------------------------------------------------------------
 * 0x41 family — subcmd is data[0]
 * ---------------------------------------------------------------------- */

/**
 * 0x41 / 0x02 — Vehicle info
 *
 * Byte layout:
 *   [0]    subcmd = 0x02
 *   [1..2] speed  (km/h, uint16 big-endian)
 *   [3..4] RPM    (uint16 big-endian)
 *   [5]    coolant temp + 40 (to keep byte unsigned; 0 → -40°C)
 *   [6]    fuel percentage (0x00 — not applicable for Focus)
 *   [7..8] battery voltage in 0.1 V units (uint16 big-endian)
 *          e.g. 126 → 12.6 V
 *   [9..12] odometer km (uint32 big-endian)
 */
void raise_send_vehicle_info(void)
{
    uint16_t spd_kmh  = car_get_speed() / 100u;    /* strip 0.01 resolution */
    uint16_t rpm      = car_get_taho();
    int8_t   temp     = car_get_temp();
    uint16_t vol_01v  = car_get_voltage() / 100u;  /* mV → 0.1 V */
    uint32_t odo      = car_get_odometer();

    uint8_t buf[13];
    buf[0]  = 0x02;                         /* subcmd */
    buf[1]  = (uint8_t)(spd_kmh >> 8);
    buf[2]  = (uint8_t)(spd_kmh & 0xFF);
    buf[3]  = (uint8_t)(rpm >> 8);
    buf[4]  = (uint8_t)(rpm & 0xFF);
    buf[5]  = (uint8_t)(temp + 40);         /* offset so always unsigned */
    buf[6]  = 0x00;                         /* fuel */
    buf[7]  = (uint8_t)(vol_01v >> 8);
    buf[8]  = (uint8_t)(vol_01v & 0xFF);
    buf[9]  = (uint8_t)(odo >> 24);
    buf[10] = (uint8_t)(odo >> 16);
    buf[11] = (uint8_t)(odo >> 8);
    buf[12] = (uint8_t)(odo & 0xFF);

    raise_send(0x41, buf, sizeof(buf));
}

/**
 * 0x41 / 0x01 — Door status
 *
 *   [0] subcmd = 0x01
 *   [1] door byte:
 *         bit0 = door_fl
 *         bit1 = door_fr
 *         bit2 = door_rl
 *         bit3 = door_rr
 *         bit4 = tailgate
 *         bit5 = bonnet
 */
void raise_send_doors(void)
{
    uint8_t doors = 0;
    if (car_get_door_fl())  doors |= (1u << 0);
    if (car_get_door_fr())  doors |= (1u << 1);
    if (car_get_door_rl())  doors |= (1u << 2);
    if (car_get_door_rr())  doors |= (1u << 3);
    if (car_get_tailgate()) doors |= (1u << 4);
    if (car_get_bonnet())   doors |= (1u << 5);

    uint8_t buf[2] = {0x01, doors};
    raise_send(0x41, buf, sizeof(buf));
}

/**
 * 0x41 / 0x03 — Warning flags
 *
 *   [0] subcmd = 0x03
 *   [1] flags byte:
 *         bit7 = low fuel (always 0 for Focus — no fuel gauge signal on MS-CAN)
 *         (other bits reserved / add as needed)
 */
void raise_send_warnings(void)
{
    uint8_t buf[2] = {0x03, 0x00};
    raise_send(0x41, buf, sizeof(buf));
}

/* -------------------------------------------------------------------------
 * 0x24 — Status flags (reverse, park brake, near lights)
 *
 *   [0] flags byte:
 *         bit0 = reverse active (e_selector_r)
 *         bit1 = park brake engaged
 *         bit2 = near lights on
 * ---------------------------------------------------------------------- */

void raise_send_status_flags(void)
{
    uint8_t flags = 0;
    if (car_get_selector() == e_selector_r) flags |= (1u << 0);
    if (car_get_park_brake())               flags |= (1u << 1);
    if (car_get_near_lights())              flags |= (1u << 2);

    uint8_t buf[1] = {flags};
    raise_send(0x24, buf, sizeof(buf));
}

/* -------------------------------------------------------------------------
 * 0x25 — Parking active
 *
 *   [0] 0x02 = sensors active, 0x00 = inactive
 * ---------------------------------------------------------------------- */

void raise_send_parking_active(void)
{
    radar_t r = car_get_radar();
    uint8_t buf[1] = {(r.state == e_radar_on) ? 0x02 : 0x00};
    raise_send(0x25, buf, sizeof(buf));
}

/* -------------------------------------------------------------------------
 * 0x22 / 0x23 — Radar distance packets
 *
 * Distance encoding:
 *   Raw from car module: 0 = closest, RADAR_DIST_CLEAR = no obstacle
 *   Raise expects:       0 = no obstacle, 1–10 = distance levels (1=closest)
 *
 *   Formula (per smartgauges/canbox):
 *     raise_val = (RMAX + 1) - scale(raw, 0, RADAR_DIST_CLEAR, 0, RMAX)
 *     where RMAX = 10
 *
 *   This inverts and compresses the 0–99 range to 1–10.
 *   If raw == RADAR_DIST_CLEAR, raise_val = 0 (no obstacle).
 * ---------------------------------------------------------------------- */

#define RADAR_RMAX 10u

static uint8_t dist_to_raise(uint8_t raw)
{
    if (raw >= RADAR_DIST_CLEAR) return 0u;
    /* Scale raw (0..98) → 0..RMAX, then invert */
    uint8_t scaled = (uint8_t)((uint32_t)raw * RADAR_RMAX / (RADAR_DIST_CLEAR - 1u));
    return (uint8_t)(RADAR_RMAX + 1u - scaled);
}

/** 0x22 — Rear sensors: RL, RLM, RRM, RR */
void raise_send_rear_radar(void)
{
    radar_t r = car_get_radar();
    uint8_t buf[4] = {
        dist_to_raise(r.dist[0]),   /* RL  */
        dist_to_raise(r.dist[1]),   /* RLM */
        dist_to_raise(r.dist[2]),   /* RRM */
        dist_to_raise(r.dist[3]),   /* RR  */
    };
    raise_send(0x22, buf, sizeof(buf));
}

/** 0x23 — Front sensors: FL, FLM, FRM, FR */
void raise_send_front_radar(void)
{
    radar_t r = car_get_radar();
    uint8_t buf[4] = {
        dist_to_raise(r.dist[4]),   /* FL  */
        dist_to_raise(r.dist[5]),   /* FLM */
        dist_to_raise(r.dist[6]),   /* FRM */
        dist_to_raise(r.dist[7]),   /* FR  */
    };
    raise_send(0x23, buf, sizeof(buf));
}

/* -------------------------------------------------------------------------
 * 0x26 — Steering angle
 *
 *   [0..1] signed int16 big-endian, degrees (range -540 to +540)
 *          derived from car_get_wheel() / 10
 * ---------------------------------------------------------------------- */

void raise_send_steering(void)
{
    int16_t angle = (int16_t)(car_get_wheel() / 10);
    uint8_t buf[2] = {
        (uint8_t)((uint16_t)angle >> 8),
        (uint8_t)((uint16_t)angle & 0xFF),
    };
    raise_send(0x26, buf, sizeof(buf));
}

/* -------------------------------------------------------------------------
 * 0x21 — AC / climate status
 *
 *   [0] flags:
 *         bit0 = ac_on
 *         bit1 = recirculation
 *         bit2 = dual_zone
 *   [1] fan speed (0–7)
 *   [2] driver setpoint temperature (°C × 2)
 *   [3] passenger setpoint temperature (°C × 2)
 *   [4] airflow bits:
 *         bit0 = windscreen
 *         bit1 = middle vents
 *         bit2 = floor vents
 * ---------------------------------------------------------------------- */

void raise_send_ac(void)
{
    uint8_t flags = 0;
    if (car_get_air_ac())            flags |= (1u << 0);
    if (car_get_air_recirculation()) flags |= (1u << 1);
    if (car_get_air_dual_zone())     flags |= (1u << 2);

    uint8_t airflow = 0;
    if (car_get_air_wind())   airflow |= (1u << 0);
    if (car_get_air_middle()) airflow |= (1u << 1);
    if (car_get_air_floor())  airflow |= (1u << 2);

    uint8_t buf[5] = {
        flags,
        car_get_air_fan(),
        car_get_air_temp_driver(),
        car_get_air_temp_pass(),
        airflow,
    };
    raise_send(0x21, buf, sizeof(buf));
}

/* -------------------------------------------------------------------------
 * 0x20 — SWC button event
 *
 *   [0] Raise button ID (raise_swc_id_t)
 *   [1] 0x01 = pressed, 0x00 = released
 *
 * The main loop sends a press immediately followed by a release for
 * momentary buttons (dequeued from the SWC FIFO as a pair).
 * ---------------------------------------------------------------------- */

void raise_send_swc(uint8_t button_id, uint8_t pressed)
{
    uint8_t buf[2] = {button_id, pressed};
    raise_send(0x20, buf, sizeof(buf));
}

/* -------------------------------------------------------------------------
 * Head-unit RX handler (0x2E packets from ATOTO → 0xFF ACK)
 *
 * The ATOTO S8 MS sends periodic status packets when in Raise mode.
 * The decoder must respond with a 0xFF byte after each received packet.
 *
 * Minimal parser:
 *   State machine reads: SOF(0x2E) → CMD → LEN → DATA[LEN] → CSUM
 *   On valid packet: send 0xFF ACK.
 *   On framing error: reset state.
 * ---------------------------------------------------------------------- */

typedef enum {
    RX_WAIT_SOF = 0,
    RX_CMD,
    RX_LEN,
    RX_DATA,
    RX_CSUM,
} rx_state_t;

static rx_state_t s_rx_state;
static uint8_t    s_rx_cmd;
static uint8_t    s_rx_len;
static uint8_t    s_rx_remaining;
static uint8_t    s_rx_csum_accum;

void raise_rx_poll(void)
{
    uint8_t byte;
    while (uart_hal_rx(&byte)) {
        switch (s_rx_state) {
        case RX_WAIT_SOF:
            if (byte == 0x2E) s_rx_state = RX_CMD;
            break;

        case RX_CMD:
            s_rx_cmd        = byte;
            s_rx_csum_accum = byte;
            s_rx_state      = RX_LEN;
            break;

        case RX_LEN:
            s_rx_len         = byte;
            s_rx_remaining   = byte;
            s_rx_csum_accum += byte;
            s_rx_state       = (byte > 0) ? RX_DATA : RX_CSUM;
            break;

        case RX_DATA:
            s_rx_csum_accum += byte;
            s_rx_remaining--;
            if (s_rx_remaining == 0) s_rx_state = RX_CSUM;
            break;

        case RX_CSUM: {
            uint8_t expected = s_rx_csum_accum ^ 0xFF;
            if (byte == expected) {
                /* Valid packet — send ACK */
                uart_hal_tx(0xFF);
            }
            /* Reset regardless — be ready for next packet */
            s_rx_state = RX_WAIT_SOF;
            break;
        }

        default:
            s_rx_state = RX_WAIT_SOF;
            break;
        }
    }
}
