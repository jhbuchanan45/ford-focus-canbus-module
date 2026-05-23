# CAN Bus Signal Spec

## Overview

Defines the CAN message IDs and signal definitions for Ford Focus HS-CAN and MS-CAN networks.
This is the source of truth for what the decoder layer implements.

## Buses

| ID | Name | Baud Rate | Description |
|----|------|-----------|-------------|
| `hs_can` | High-Speed CAN | 500 kbps | Powertrain, ABS, engine, transmission. Accessible via OBD-II pins 6 (H) and 14 (L) |
| `ms_can` | Medium-Speed CAN | 125 kbps | Body electronics: HVAC, instrument cluster, lighting, BCM |

## Signal Definition Format

Each signal within a message is described by:

| Field | Description |
|-------|-------------|
| `start_bit` | LSB index (Intel) or MSB index (Motorola) |
| `bit_length` | Number of bits |
| `byte_order` | `little_endian` (Intel) or `big_endian` (Motorola) |
| `value_type` | `unsigned` or `signed` |
| `scale` | Multiply raw value (physical = raw × scale + offset) |
| `offset` | Add after scaling |
| `unit` | Engineering unit |
| `range` | [min, max] in physical units (informational) |

---

## HS-CAN Messages

### 0x201 — ENGINE_RPM
- Cycle: 10 ms

| Signal | Start Bit | Length | Order | Type | Scale | Offset | Unit | Range |
|--------|-----------|--------|-------|------|-------|--------|------|-------|
| `rpm` | 0 | 16 | big_endian | unsigned | 0.25 | 0 | rpm | 0–8000 |
| `throttle_position` | 24 | 8 | big_endian | unsigned | 0.392 | 0 | % | 0–100 |

### 0x202 — VEHICLE_SPEED
- Cycle: 20 ms

| Signal | Start Bit | Length | Order | Type | Scale | Offset | Unit | Range |
|--------|-----------|--------|-------|------|-------|--------|------|-------|
| `vehicle_speed` | 0 | 16 | big_endian | unsigned | 0.01 | 0 | km/h | 0–280 |
| `wheel_speed_fl` | 16 | 16 | big_endian | unsigned | 0.01 | 0 | km/h | 0–280 |
| `wheel_speed_fr` | 32 | 16 | big_endian | unsigned | 0.01 | 0 | km/h | 0–280 |

### 0x420 — ENGINE_TEMPS
- Cycle: 1000 ms

| Signal | Start Bit | Length | Order | Type | Scale | Offset | Unit | Range |
|--------|-----------|--------|-------|------|-------|--------|------|-------|
| `coolant_temp` | 0 | 8 | big_endian | unsigned | 1 | -40 | °C | -40–215 |
| `oil_temp` | 8 | 8 | big_endian | unsigned | 1 | -40 | °C | -40–215 |

### 0x703 — ABS_STATUS
- Cycle: 20 ms

| Signal | Start Bit | Length | Order | Type | Scale | Offset | Unit | Range |
|--------|-----------|--------|-------|------|-------|--------|------|-------|
| `abs_active` | 0 | 1 | big_endian | unsigned | 1 | 0 | bool | 0–1 |
| `traction_control_active` | 1 | 1 | big_endian | unsigned | 1 | 0 | bool | 0–1 |
| `esp_active` | 2 | 1 | big_endian | unsigned | 1 | 0 | bool | 0–1 |

---

## MS-CAN Messages

### 0x072 — INSTRUMENT_CLUSTER
- Cycle: 100 ms

| Signal | Start Bit | Length | Order | Type | Scale | Offset | Unit | Range |
|--------|-----------|--------|-------|------|-------|--------|------|-------|
| `odometer` | 0 | 24 | big_endian | unsigned | 1 | 0 | km | 0–16777215 |

### 0x3B5 — HVAC_STATUS
- Cycle: 500 ms

| Signal | Start Bit | Length | Order | Type | Scale | Offset | Unit | Range |
|--------|-----------|--------|-------|------|-------|--------|------|-------|
| `fan_speed` | 0 | 4 | big_endian | unsigned | 1 | 0 | level | 0–8 |
| `target_temp_driver` | 8 | 8 | big_endian | unsigned | 0.5 | 14 | °C | 14–28 |
| `ac_compressor_on` | 32 | 1 | big_endian | unsigned | 1 | 0 | bool | 0–1 |

### 0x4B0 — LIGHTING_STATUS
- Cycle: 200 ms

| Signal | Start Bit | Length | Order | Type | Scale | Offset | Unit | Range |
|--------|-----------|--------|-------|------|-------|--------|------|-------|
| `low_beam` | 0 | 1 | big_endian | unsigned | 1 | 0 | bool | 0–1 |
| `high_beam` | 1 | 1 | big_endian | unsigned | 1 | 0 | bool | 0–1 |
| `fog_front` | 2 | 1 | big_endian | unsigned | 1 | 0 | bool | 0–1 |
| `fog_rear` | 3 | 1 | big_endian | unsigned | 1 | 0 | bool | 0–1 |
| `indicator_left` | 4 | 1 | big_endian | unsigned | 1 | 0 | bool | 0–1 |
| `indicator_right` | 5 | 1 | big_endian | unsigned | 1 | 0 | bool | 0–1 |

---

## Unverified IDs (community research — needs confirmation on real bus)

| ID | Bus | Suspected Purpose |
|----|-----|-------------------|
| 0x080 | hs_can | Steering angle sensor |
| 0x120 | hs_can | Fuel injection timing |
| 0x230 | hs_can | Battery / charging system voltage |
| 0x540 | ms_can | Door ajar / window position status |
| 0x625 | ms_can | Power window control |

## Non-goals

- Transmitting CAN frames
- LIN bus signals (seat modules, mirrors)
- OBD-II PID parsing (handled separately by ISO 15765-2 layer, out of scope for now)
