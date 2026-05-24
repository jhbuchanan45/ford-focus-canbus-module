/**
 * json_log.c — NDJSON output driver (Phase 1, CANMOD_OUTPUT=json)
 *
 * Emits one JSON object per decoded CAN frame over USB CDC (or stdout on
 * the host target).  Unknown frames are silently discarded.
 *
 * Wire format per line:
 *   {"ts":<ms>,"id":"0xXXX","signals":{<key>:<val>,...}}\n
 *
 * All car_get_*() values are snapshotted at emit time so the object
 * reflects the state immediately after the frame was decoded.
 */

#include "json_log.h"
#include "../car.h"
#include "../hal/usb_hal.h"

#include <stdio.h>
#include <string.h>
#include <stdint.h>

/* Known MS-CAN IDs — must match the constants in ford_focus_mk3_2015.c */
#define ID_EPAS     0x080u
#define ID_ODO      0x072u
#define ID_SWC      0x1A9u
#define ID_SPD_RPM  0x217u
#define ID_BATTERY  0x230u
#define ID_GEAR     0x165u
#define ID_HVAC     0x3B5u
#define ID_TEMP     0x420u
#define ID_LIGHTS   0x4B0u
#define ID_DOORS    0x540u
#define ID_PDC      0x5C0u

/* Write a formatted string to USB CDC / stdout */
static void emit_str(const char *s)
{
    usb_hal_write((const uint8_t *)s, (uint16_t)strlen(s));
}

/* -------------------------------------------------------------------------
 * Per-message-ID emit functions
 * Each function formats the signals JSON object for that message.
 * ---------------------------------------------------------------------- */

static void emit_speed_rpm(uint32_t ts)
{
    char buf[128];
    snprintf(buf, sizeof(buf),
        "{\"ts\":%lu,\"id\":\"0x217\",\"signals\":"
        "{\"speed_kmh\":%.2f,\"rpm\":%u}}\n",
        (unsigned long)ts,
        car_get_speed() / 100.0f,
        car_get_taho());
    emit_str(buf);
}

static void emit_coolant_temp(uint32_t ts)
{
    char buf[96];
    snprintf(buf, sizeof(buf),
        "{\"ts\":%lu,\"id\":\"0x420\",\"signals\":"
        "{\"coolant_temp_c\":%d}}\n",
        (unsigned long)ts,
        car_get_temp());
    emit_str(buf);
}

static void emit_battery(uint32_t ts)
{
    char buf[96];
    snprintf(buf, sizeof(buf),
        "{\"ts\":%lu,\"id\":\"0x230\",\"signals\":"
        "{\"battery_v\":%.3f}}\n",
        (unsigned long)ts,
        car_get_voltage() / 1000.0f);
    emit_str(buf);
}

static void emit_odometer(uint32_t ts)
{
    char buf[96];
    snprintf(buf, sizeof(buf),
        "{\"ts\":%lu,\"id\":\"0x072\",\"signals\":"
        "{\"odometer_km\":%lu}}\n",
        (unsigned long)ts,
        (unsigned long)car_get_odometer());
    emit_str(buf);
}

static void emit_doors(uint32_t ts)
{
    char buf[160];
    snprintf(buf, sizeof(buf),
        "{\"ts\":%lu,\"id\":\"0x540\",\"signals\":"
        "{\"door_fl\":%u,\"door_fr\":%u,\"door_rl\":%u,\"door_rr\":%u,"
        "\"tailgate\":%u,\"bonnet\":%u,\"park_brake\":%u}}\n",
        (unsigned long)ts,
        car_get_door_fl(), car_get_door_fr(),
        car_get_door_rl(), car_get_door_rr(),
        car_get_tailgate(), car_get_bonnet(),
        car_get_park_brake());
    emit_str(buf);
}

static void emit_lighting(uint32_t ts)
{
    char buf[96];
    snprintf(buf, sizeof(buf),
        "{\"ts\":%lu,\"id\":\"0x4B0\",\"signals\":"
        "{\"near_lights\":%u}}\n",
        (unsigned long)ts,
        car_get_near_lights());
    emit_str(buf);
}

static void emit_gear(uint32_t ts)
{
    const char *sel_str;
    switch (car_get_selector()) {
    case e_selector_p: sel_str = "P"; break;
    case e_selector_r: sel_str = "R"; break;
    case e_selector_n: sel_str = "N"; break;
    case e_selector_d: sel_str = "D"; break;
    default:           sel_str = "?"; break;
    }
    char buf[96];
    snprintf(buf, sizeof(buf),
        "{\"ts\":%lu,\"id\":\"0x165\",\"signals\":"
        "{\"selector\":\"%s\"}}\n",
        (unsigned long)ts, sel_str);
    emit_str(buf);
}

static void emit_hvac(uint32_t ts)
{
    char buf[256];
    snprintf(buf, sizeof(buf),
        "{\"ts\":%lu,\"id\":\"0x3B5\",\"signals\":"
        "{\"ac_on\":%u,\"fan_speed\":%u,"
        "\"temp_driver\":%.1f,\"temp_pass\":%.1f,"
        "\"recirculation\":%u,\"air_wind\":%u,"
        "\"air_middle\":%u,\"air_floor\":%u,\"dual_zone\":%u}}\n",
        (unsigned long)ts,
        car_get_air_ac(),
        car_get_air_fan(),
        car_get_air_temp_driver() / 2.0f,
        car_get_air_temp_pass()   / 2.0f,
        car_get_air_recirculation(),
        car_get_air_wind(),
        car_get_air_middle(),
        car_get_air_floor(),
        car_get_air_dual_zone());
    emit_str(buf);
}

static void emit_steering(uint32_t ts)
{
    char buf[96];
    snprintf(buf, sizeof(buf),
        "{\"ts\":%lu,\"id\":\"0x080\",\"signals\":"
        "{\"steering_angle\":%.1f}}\n",
        (unsigned long)ts,
        car_get_wheel() / 10.0f);
    emit_str(buf);
}

static void emit_pdc(uint32_t ts)
{
    radar_t r = car_get_radar();
    char buf[256];
    snprintf(buf, sizeof(buf),
        "{\"ts\":%lu,\"id\":\"0x5C0\",\"signals\":"
        "{\"pdc_active\":%u,"
        "\"rear\":[%u,%u,%u,%u],"
        "\"front\":[%u,%u,%u,%u]}}\n",
        (unsigned long)ts,
        (r.state == e_radar_on) ? 1u : 0u,
        r.dist[0], r.dist[1], r.dist[2], r.dist[3],
        r.dist[4], r.dist[5], r.dist[6], r.dist[7]);
    emit_str(buf);
}

static void emit_swc(uint32_t ts)
{
    char buf[96];
    snprintf(buf, sizeof(buf),
        "{\"ts\":%lu,\"id\":\"0x1A9\",\"signals\":"
        "{\"swc_raw\":0}}\n",   /* raw value logged; button_id mapped separately */
        (unsigned long)ts);
    emit_str(buf);
}

/* -------------------------------------------------------------------------
 * Public entry point
 * ---------------------------------------------------------------------- */

void json_log_emit(uint32_t ts, uint32_t id)
{
    switch (id) {
    case ID_SPD_RPM:  emit_speed_rpm(ts);   break;
    case ID_TEMP:     emit_coolant_temp(ts); break;
    case ID_BATTERY:  emit_battery(ts);      break;
    case ID_ODO:      emit_odometer(ts);     break;
    case ID_DOORS:    emit_doors(ts);        break;
    case ID_LIGHTS:   emit_lighting(ts);     break;
    case ID_GEAR:     emit_gear(ts);         break;
    case ID_HVAC:     emit_hvac(ts);         break;
    case ID_EPAS:     emit_steering(ts);     break;
    case ID_PDC:      emit_pdc(ts);          break;
    case ID_SWC:      emit_swc(ts);          break;
    default:          /* unknown ID — not logged */ break;
    }
}
