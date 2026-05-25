# CAN Bus Signal Spec — Ford Focus Mk3 2015 (MS-CAN)

## Overview

Defines the MS-CAN message IDs and byte-level signal definitions for the Ford Focus Mk3 2015 Titanium decoded by this firmware.

**Only MS-CAN (125 kbps) is used.** MS-CAN is accessed via the quad-lock connector (pins A9 H / A10 L). HS-CAN (500 kbps) is not connected. Powertrain signals (speed, RPM, temperature) that live natively on HS-CAN are available on MS-CAN as copies mirrored by the GEM gateway module.

All IDs and byte layouts below are **⚠ community-researched estimates**. Each is marked for confirmation via FORScan capture on the target vehicle (task 1.2). Update the ⚠ markers and correct any wrong fields once confirmed.

---

## MS-CAN Messages

### 0x072 — BCM Odometer
- Cycle: 1000 ms (estimated)

| Signal | Bytes | Type | Scale | Offset | Unit | Range |
|--------|-------|------|-------|--------|------|-------|
| `odometer` | d[1..3] 24-bit BE | unsigned | 1 | 0 | km | 0–16,777,215 |

### 0x080 — EPAS Steering Angle ⚠
- Cycle: 10 ms (estimated)
- Source: Electric Power Assisted Steering column module

| Signal | Bytes | Type | Scale | Offset | Unit | Range |
|--------|-------|------|-------|--------|------|-------|
| `steering_angle` | d[0..1] signed int16 BE | signed | 0.1 | 0 | degrees | -540 to +540 |

Internal unit: `deg × 10` (e.g. 3600 = 360.0°). Negative = left.

### 0x165 — TCM Gear Selector ⚠
- Cycle: 100 ms (estimated)
- Source: TCM (PowerShift gearbox), mirrored by GEM

| Signal | Bytes | Encoding |
|--------|-------|----------|
| `gear_position` | d[0] | 0x00=P, 0x01=R, 0x02=N, 0x03=D |

### 0x1A9 — Steering Wheel Controls ⚠
- Cycle: event-driven
- Source: Steering column module (SYNC 2 controls)

| Signal | Bytes | Encoding |
|--------|-------|----------|
| `button_code` | d[0] | 0x00=none, 0x01=Vol+, 0x02=Vol−, 0x04=Next, 0x08=Prev, 0x10=Mode, 0x20=Mute, 0x40=Answer, 0x80=Hangup |
| `button_state` | d[1] | 0x01=pressed, 0x00=released |

### 0x217 — GEM Speed + RPM ⚠
- Cycle: 20 ms (estimated)
- Source: GEM gateway (mirrors HS-CAN powertrain values to MS-CAN)

| Signal | Bytes | Type | Scale | Offset | Unit | Range |
|--------|-------|------|-------|--------|------|-------|
| `vehicle_speed` | d[0..1] BE | unsigned | 0.01 | 0 | km/h | 0–655 |
| `engine_rpm` | d[2..3] BE | unsigned | 0.25 | 0 | RPM | 0–16383 |

Internal units: speed = `km/h × 100`; RPM = raw `>> 2`.

### 0x230 — BCM Battery Voltage ⚠
- Cycle: 1000 ms (estimated)

| Signal | Bytes | Type | Scale | Offset | Unit | Range |
|--------|-------|------|-------|--------|------|-------|
| `battery_voltage` | d[0..1] BE | unsigned | 0.1 | 0 | V | 0–65.5 |

Internal unit: millivolts (raw × 100).

### 0x3B5 — HVAC Module Climate State ⚠
- Cycle: 500 ms (estimated)
- Source: FCIM (Front Controls Interface Module)
- Reference frame: `3B5#0300002B00000000`

| Signal | Bytes | Bit | Encoding |
|--------|-------|-----|----------|
| `ac_on` | d[0] | 0 | 1=compressor on |
| `recirculation` | d[0] | 1 | 1=recirculating |
| `dual_zone` | d[0] | 2 | 1=dual zone active |
| `fan_speed_raw` | d[1] | bits[3:0] | 0–15; Raise fan = raw >> 1 (0–7) |
| `temp_driver` | d[2] | — | °C × 2 (e.g. 42 = 21.0°C) |
| `temp_pass` | d[3] | — | °C × 2 |
| `air_windscreen` | d[4] | 0 | 1=on |
| `air_middle` | d[4] | 1 | 1=on |
| `air_floor` | d[4] | 2 | 1=on |

### 0x420 — GEM Coolant Temperature ⚠
- Cycle: 1000 ms (estimated)
- Source: GEM gateway (mirrors ECU coolant temp to MS-CAN)

| Signal | Bytes | Type | Scale | Offset | Unit | Range |
|--------|-------|------|-------|--------|------|-------|
| `coolant_temp` | d[0] | unsigned | 1 | -40 | °C | -40–215 |

### 0x4B0 — BCM Lighting Status ⚠
- Cycle: 200 ms (estimated)

| Signal | Bytes | Bit | Encoding |
|--------|-------|-----|----------|
| `near_lights` | d[0] | 0 | 1=sidelights or headlights on |

### 0x540 — BCM Door / Body Status ⚠
- Cycle: event-driven + 500 ms keepalive (estimated)

| Signal | Bytes | Bit | Encoding |
|--------|-------|-----|----------|
| `door_fl` | d[0] | 0 | 1=open |
| `door_fr` | d[0] | 1 | 1=open |
| `door_rl` | d[0] | 2 | 1=open |
| `door_rr` | d[0] | 3 | 1=open |
| `tailgate` | d[0] | 4 | 1=open |
| `bonnet` | d[0] | 5 | 1=open |
| `park_brake` | d[0] | 6 | 1=engaged |

### 0x5C0 — Parking Aid Module (PDC) ⚠
- Cycle: 100 ms when active
- ⚠ Availability at quad-lock MS-CAN must be confirmed (task 1.2). Only rear sensors are guaranteed; front sensors depend on fitment.

| Signal | Bytes | Encoding |
|--------|-------|----------|
| `pdc_active` | d[0] bit 0 | 1=sensors active |
| `rear_rl` | d[1] | Ford zone 0–6 (0=clear, 6=closest) → internal 0–99 |
| `rear_rlm` | d[2] | same |
| `rear_rrm` | d[3] | same |
| `rear_rr` | d[4] | same |
| `front_fl` | d[5] | same (if 8-sensor variant) |
| `front_flm` | d[6] | same |
| `front_frm` | d[7] | same |
| `front_fr` | d[8] | same |

Zone-to-distance conversion: `zone 0 → RADAR_DIST_CLEAR (99)`; `zone n (1–6) → (6−n) × 14`.

---

## Signal Summary Table

| ID | Name | Signals decoded | Source |
|----|------|-----------------|--------|
| 0x072 | BCM Odometer | odometer | BCM |
| 0x080 | EPAS Steering | steering_angle | EPAS column |
| 0x165 | Gear Selector | gear_position | TCM/GEM |
| 0x1A9 | SWC | button_code, button_state | Steering column |
| 0x217 | GEM Speed/RPM | vehicle_speed, engine_rpm | GEM |
| 0x230 | Battery | battery_voltage | BCM |
| 0x3B5 | HVAC | ac_on, fan, temps, airflow, recirc, dual_zone | FCIM |
| 0x420 | Coolant Temp | coolant_temp | GEM |
| 0x4B0 | Lighting | near_lights | BCM |
| 0x540 | Doors/Body | 6× doors, park_brake | BCM |
| 0x5C0 | PDC | pdc_active, 8× sensor distances | PAM |

---

## Non-goals

- HS-CAN (500 kbps OBD-II) is not connected or decoded
- LIN bus signals (seat modules, mirrors)
- OBD-II PID parsing
- Transmitting CAN frames
