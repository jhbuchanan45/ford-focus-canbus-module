## ADDED Requirements

### Requirement: CAN peripheral initialises MS-CAN bus
The firmware SHALL initialise the CAN peripheral to receive on MS-CAN at 125 kbps with the TJA1042 transceiver, using the bxCAN peripheral on STM32F103 (or FDCAN equivalent on other variants).

#### Scenario: Successful init
- **WHEN** the firmware starts
- **THEN** the CAN peripheral is configured at 125 kbps, 8N1, and begins receiving frames within 10 ms of power-on

#### Scenario: Hardware acceptance filter applied
- **WHEN** the CAN peripheral is initialised
- **THEN** hardware filters are set to pass only the MS-CAN message IDs defined in the Ford Focus signal map, and all other IDs are silently dropped at hardware level

---

### Requirement: Ford Focus MS-CAN signal map is maintained in a dedicated car module
The firmware SHALL implement a Ford Focus Mk3 2015 Titanium car module at `src/cars/ford_focus_mk3_2015.c` that decodes raw MS-CAN frames and maintains internal signal state. Signal IDs SHALL be verified against the real car using FORScan before the module is written.

#### Scenario: Known frame decoded
- **WHEN** a CAN frame with a recognised MS-CAN ID is received
- **THEN** the car module updates the corresponding internal state variable(s) within one main loop tick

#### Scenario: Unknown frame ignored
- **WHEN** a CAN frame with an unrecognised ID passes the hardware filter (filter catch-all)
- **THEN** the car module takes no action and does not modify any state

---

### Requirement: car_get_* API exposes all signal state to the encoder
The car module SHALL expose all vehicle state exclusively through the `car_get_*` API defined in `car.h`. The Raise encoder and output drivers SHALL NOT read raw CAN data directly.

#### Scenario: Speed queried
- **WHEN** `car_get_speed()` is called
- **THEN** it returns the most recently decoded vehicle speed in km/h as a float, or 0.0 if not yet received

#### Scenario: Door state queried
- **WHEN** `car_get_door_fl()` (and fr, rl, rr) is called
- **THEN** it returns 1 if that door is open, 0 if closed, based on the most recent MS-CAN door status frame

---

### Requirement: All Raise-required signals are decoded from MS-CAN
The car module SHALL decode and maintain state for every signal needed to produce a complete Raise VW PQ output. Signals marked "unverified" SHALL be implemented with placeholder IDs and updated when FORScan verification is complete.

Required signals and their suspected MS-CAN source (⚠ = unverified, must confirm with FORScan):

| Signal | car_get_* function | MS-CAN ID | Notes |
|---|---|---|---|
| Vehicle speed | `car_get_speed()` | ⚠ ~0x217 | GEM mirrors from HS-CAN |
| Engine RPM | `car_get_taho()` | ⚠ ~0x217 | GEM mirrors from HS-CAN |
| Coolant temp | `car_get_temp()` | ⚠ ~0x420 | GEM mirrors from HS-CAN |
| Battery voltage | `car_get_voltage()` | ⚠ ~0x230 | BCM |
| Odometer | `car_get_odometer()` | ⚠ ~0x072 | Instrument cluster |
| Low fuel flag | `car_get_low_fuel_level()` | ⚠ ~0x072 | Instrument cluster |
| FL/FR/RL/RR door | `car_get_door_*()` | ⚠ ~0x540 | BCM |
| Tailgate | `car_get_tailgate()` | ⚠ ~0x540 | BCM |
| Bonnet | `car_get_bonnet()` | ⚠ ~0x540 | BCM |
| Park brake | `car_get_park_break()` | ⚠ ~0x540 | BCM |
| Near lights | `car_get_near_lights()` | ⚠ ~0x4B0 | BCM/lighting module |
| Gear selector (reverse) | `car_get_selector()` | ⚠ ~0x165 | TCM/GEM |
| Steering angle | `car_get_wheel()` | ⚠ ~0x080 | EPAS column module |
| AC compressor on | `car_get_air_ac()` | ⚠ ~0x3B5 | HVAC module |
| AC fan speed | `car_get_air_fanspeed()` | ⚠ ~0x3B5 | HVAC module |
| Driver target temp | `car_get_air_l_temp()` | ⚠ ~0x3B5 | HVAC module |
| Passenger target temp | `car_get_air_r_temp()` | ⚠ ~0x3B5 | HVAC module |
| Recirculation | `car_get_air_recycling()` | ⚠ ~0x3B5 | HVAC module |
| Airflow direction | `car_get_air_wind/middle/floor()` | ⚠ ~0x3B5 | HVAC module |
| Dual zone | `car_get_air_dual()` | ⚠ ~0x3B5 | HVAC module |
| Rear PDC (4 sensors) | `car_get_radar().rl/rlm/rrm/rr` | ⚠ ~0x5C0 | Parking aid module |
| Front PDC (4 sensors) | `car_get_radar().fl/flm/frm/fr` | ⚠ ~0x5C0 | Parking aid module |
| PDC active state | `car_get_radar().state` | ⚠ ~0x5C0 | Parking aid module |
| SWC buttons | `canmod_swc_event_t` (event queue) | ⚠ ~0x1A9 | Steering column module |

#### Scenario: Reverse gear decoded
- **WHEN** a gear selector frame is received with reverse engaged
- **THEN** `car_get_selector()` returns `e_selector_r`

#### Scenario: PDC distances decoded
- **WHEN** a PDC frame is received while parking sensors are active
- **THEN** `car_get_radar().state` returns `e_radar_on` and each distance field (fl, flm, frm, fr, rl, rlm, rrm, rr) is set to a value in the range 0–99 (0 = closest, 99 = no object)

---

### Requirement: SWC button events are queued, not polled
SWC button presses from the steering column module SHALL be decoded and placed onto a small FIFO event queue (`canmod_swc_event_t[]`), not exposed as a polled state. The encoder reads and clears one event per tick.

#### Scenario: Button press event queued
- **WHEN** a SWC frame is received with a button pressed
- **THEN** a `canmod_swc_event_t` with the correct `button_id` and `state=PRESSED` is appended to the event queue

#### Scenario: Queue full — oldest event dropped
- **WHEN** a SWC frame arrives and the event queue is full (8 events)
- **THEN** the oldest event is dropped and the new event is appended (no blocking, no dynamic allocation)
