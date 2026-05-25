# MS-CAN Input Spec

## Overview

Defines how the firmware receives and dispatches MS-CAN frames. The CAN peripheral is initialised once at boot; all frame handling is interrupt- or poll-driven via the HAL abstraction.

## Requirements

### Requirement: CAN peripheral initialises MS-CAN at 125 kbps
The firmware SHALL initialise the CAN peripheral to receive on MS-CAN at 125 kbps (bxCAN on STM32F103 via TJA1042 transceiver, or SocketCAN on host target). Initialisation SHALL complete within 10 ms of boot.

#### Scenario: Successful init
- **WHEN** the firmware starts
- **THEN** the CAN peripheral is configured at 125 kbps 8N1 and begins receiving frames within 10 ms

#### Scenario: Hardware acceptance filter applied
- **WHEN** the CAN peripheral is initialised
- **THEN** hardware filters are set to pass only the 11 known MS-CAN IDs (0x072, 0x080, 0x165, 0x1A9, 0x217, 0x230, 0x3B5, 0x420, 0x4B0, 0x540, 0x5C0); all other IDs are silently dropped at hardware level

---

### Requirement: Ford Focus MS-CAN signal map is maintained in a dedicated car module
The firmware SHALL implement the Ford Focus Mk3 2015 Titanium decoder at `src/cars/ford_focus_mk3_2015.c`. This file is the ONLY file that reads raw CAN frame data. It maintains internal signal state updated on each received frame.

#### Scenario: Known frame decoded
- **WHEN** a CAN frame with a recognised MS-CAN ID is received
- **THEN** the car module updates the corresponding internal state variable(s) within one main loop tick

#### Scenario: Unknown frame ignored
- **WHEN** a CAN frame with an unrecognised ID is received
- **THEN** the car module takes no action and does not modify any state

---

### Requirement: car_get_* API exposes all signal state to output drivers
The car module SHALL expose all vehicle state exclusively through the `car_get_*` API defined in `car.h`. Output drivers (Raise, JSON) SHALL NOT access raw CAN data or internal car module state directly.

#### Scenario: API contract enforced at compile time
- **WHEN** any file outside `src/cars/` attempts to include raw CAN types
- **THEN** compilation fails (CAN frame types are not exposed in public headers)

#### Scenario: Speed queried
- **WHEN** `car_get_speed()` is called
- **THEN** it returns the most recently decoded vehicle speed in km/h × 100, or 0 if no frame has been received

---

### Requirement: All Raise-required signals are decoded from MS-CAN

| Signal | `car_get_*` function | MS-CAN ID | Notes |
|--------|----------------------|-----------|-------|
| Vehicle speed | `car_get_speed()` | ⚠ 0x217 | GEM mirror, km/h × 100 |
| Engine RPM | `car_get_taho()` | ⚠ 0x217 | GEM mirror |
| Coolant temp | `car_get_temp()` | ⚠ 0x420 | GEM mirror, °C |
| Battery voltage | `car_get_voltage()` | ⚠ 0x230 | BCM, mV |
| Odometer | `car_get_odometer()` | ⚠ 0x072 | BCM, km |
| FL/FR/RL/RR door | `car_get_door_*()` | ⚠ 0x540 | BCM |
| Tailgate | `car_get_tailgate()` | ⚠ 0x540 | BCM |
| Bonnet | `car_get_bonnet()` | ⚠ 0x540 | BCM |
| Park brake | `car_get_park_brake()` | ⚠ 0x540 | BCM |
| Near lights | `car_get_near_lights()` | ⚠ 0x4B0 | BCM/lighting |
| Gear selector | `car_get_selector()` | ⚠ 0x165 | TCM/GEM |
| Steering angle | `car_get_wheel()` | ⚠ 0x080 | EPAS, deg × 10 |
| AC on | `car_get_air_ac()` | ⚠ 0x3B5 | FCIM |
| Fan speed | `car_get_air_fan()` | ⚠ 0x3B5 | FCIM, Raise 0–7 |
| Driver temp | `car_get_air_temp_driver()` | ⚠ 0x3B5 | FCIM, °C × 2 |
| Passenger temp | `car_get_air_temp_pass()` | ⚠ 0x3B5 | FCIM, °C × 2 |
| Recirculation | `car_get_air_recirculation()` | ⚠ 0x3B5 | FCIM |
| Airflow (3 bits) | `car_get_air_wind/middle/floor()` | ⚠ 0x3B5 | FCIM |
| Dual zone | `car_get_air_dual_zone()` | ⚠ 0x3B5 | FCIM |
| PDC 8 sensors | `car_get_radar().dist[0..7]` | ⚠ 0x5C0 | PAM |
| PDC active | `car_get_radar().state` | ⚠ 0x5C0 | PAM |
| SWC events | `car_swc_dequeue()` | ⚠ 0x1A9 | Steering column |

---

### Requirement: SWC button events are queued, not polled
SWC button presses SHALL be decoded and placed onto a FIFO event queue (8 entries, drop-oldest on overflow). Output drivers call `car_swc_dequeue()` to consume one event per tick.

#### Scenario: Button press event queued
- **WHEN** a SWC frame is received with a button code and press state
- **THEN** a `canmod_swc_event_t` with the correct `button_id` and `pressed=1` is appended to the queue

#### Scenario: Queue full — oldest event dropped
- **WHEN** a SWC frame arrives and the queue holds 8 events
- **THEN** the oldest event is dropped and the new event is appended (no blocking, no dynamic allocation)
