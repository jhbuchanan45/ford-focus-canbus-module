# JSON Log Output Driver Spec

## Purpose

The JSON log output driver (`src/output/json_log.c`) emits newline-delimited JSON (NDJSON) over USB CDC for Phase 1 development and signal verification. It is selected at compile time via `-DCANMOD_OUTPUT=json` and is mutually exclusive with the Raise driver.

## Requirements

### Requirement: JSON log driver emits NDJSON over USB CDC
One JSON object SHALL be emitted per decoded CAN frame, on a single line terminated by `\n`. The object contains a millisecond timestamp, the CAN message ID as a hex string, and all decoded signal fields for that message.

Format:
```json
{"ts":12345,"id":"0x3B5","signals":{"ac_on":1,"fan_speed":3,"temp_driver":21.5}}
```

#### Scenario: Frame decoded and logged
- **WHEN** a MS-CAN frame is received and decoded by the car module
- **THEN** within one main loop tick the JSON log driver emits a complete NDJSON line over USB CDC

#### Scenario: Unknown frame not logged
- **WHEN** a CAN frame is received that the car module does not recognise
- **THEN** nothing is emitted (unknown frames are not echoed as raw hex)

---

### Requirement: Signal field names and scaling are consistent with car.h conventions
JSON output fields SHALL use the exact names and scaling defined in the table below, derived from `car_get_*` return values.

#### Scenario: Speed field emitted with correct scaling
- **WHEN** a 0x217 frame is received with vehicle speed 5000 (50.00 km/h × 100)
- **THEN** the emitted JSON contains `"speed_kmh":50.00`

| CAN ID | JSON field | Source | Scale |
|--------|-----------|--------|-------|
| 0x217 | `speed_kmh` | `car_get_speed()` | ÷ 100.0 → float, 2dp |
| 0x217 | `rpm` | `car_get_taho()` | integer |
| 0x420 | `coolant_temp_c` | `car_get_temp()` | integer |
| 0x230 | `battery_v` | `car_get_voltage()` | ÷ 1000.0 → float, 3dp |
| 0x072 | `odometer_km` | `car_get_odometer()` | integer |
| 0x540 | `door_fl/fr/rl/rr` | `car_get_door_*()` | 0/1 |
| 0x540 | `tailgate`, `bonnet` | `car_get_tailgate/bonnet()` | 0/1 |
| 0x540 | `park_brake` | `car_get_park_brake()` | 0/1 |
| 0x4B0 | `near_lights` | `car_get_near_lights()` | 0/1 |
| 0x165 | `gear` | `car_get_selector()` | 0=P, 1=R, 2=N, 3=D |
| 0x3B5 | `ac_on`, `fan_speed` | `car_get_air_ac/fan()` | integers |
| 0x3B5 | `temp_driver`, `temp_pass` | `car_get_air_temp_*()` | ÷ 2.0 → float, 1dp |
| 0x3B5 | `recirculation`, `dual_zone` | `car_get_air_recirculation/dual_zone()` | 0/1 |
| 0x080 | `steering_deg` | `car_get_wheel()` | ÷ 10.0 → float, 1dp |

---

### Requirement: Timestamps are milliseconds since boot
The `ts` field SHALL be a monotonic uint32 millisecond counter from `hw_tick_get()`, wrapping at 2^32 ms (≈ 49 days).

#### Scenario: Timestamp increments correctly
- **WHEN** two frames are logged 10 ms apart
- **THEN** the second frame's `ts` is 10 greater than the first (±1 ms timer resolution)

---

### Requirement: JSON build and Raise build are mutually exclusive
The build system SHALL compile exactly one output driver per binary. `CANMOD_OUTPUT=json` enables `json_log.c` and defines `CANMOD_OUTPUT_JSON=1`. `CANMOD_OUTPUT=raise` enables `raise.c` and defines `CANMOD_OUTPUT_RAISE=1`. Both SHALL NOT be compiled into the same binary.

#### Scenario: Only one output driver compiled per build
- **WHEN** `cmake` is invoked with `-DCANMOD_OUTPUT=json`
- **THEN** `raise.c` is not compiled and `CANMOD_OUTPUT_RAISE` is not defined

---

### Requirement: JSON log is usable as a replay corpus
Log files captured from the car SHALL be replayable against `canmod-host` using `canplayer` for offline regression testing. The `tools/replay.sh --golden` flag enables golden-file comparison (stripping `ts` fields before diffing).

#### Scenario: Captured log replayed produces matching output
- **WHEN** `forscan_baseline.log` is replayed via `canplayer` and output is compared to `forscan_baseline_golden.ndjson` (with timestamps stripped)
- **THEN** all signal fields match and the comparison exits 0
