## MODIFIED Requirements

### Requirement: MS-CAN is the only active bus for this decoder
For this decoder, only the MS-CAN bus at 125 kbps SHALL be used. MS-CAN is accessed via the quad-lock connector (pins A9 H, A10 L), not the OBD-II port. HS-CAN (500 kbps, powertrain) is NOT decoded in this change; signals from HS-CAN that are needed (vehicle speed, RPM, temperature) are available on MS-CAN as mirrored copies via the GEM gateway module.

#### Scenario: Only MS-CAN frames processed
- **WHEN** the firmware is running connected to the quad-lock harness
- **THEN** the CAN peripheral is initialised at 125 kbps and only MS-CAN frames are received; no HS-CAN connection is made

## ADDED Requirements

### Requirement: Full MS-CAN signal set for Raise output is specified
The following MS-CAN messages SHALL be decoded by the Ford Focus Mk3 2015 car module. All IDs are community-researched and marked ⚠ until verified with FORScan on the target vehicle.

#### Scenario: GEM-mirrored powertrain signals present on MS-CAN
- **WHEN** the engine is running and the car is in motion
- **THEN** vehicle speed, RPM, and coolant temperature are present on MS-CAN (mirrored by GEM gateway) and the car module decodes them from the appropriate IDs

---

### Requirement: Steering wheel control messages are decoded
The car module SHALL decode SWC button events from the steering column frame and enqueue them for the Raise output driver.

⚠ Suspected ID: ~0x1A9. SWC button state for the Ford Focus Mk3 Titanium with SYNC 2.

| Signal | Bit Start | Bit Len | Encoding |
|---|---|---|---|
| `swc_button_id` | TBD | 8 | Button identifier byte (verify with FORScan) |
| `swc_state` | TBD | 2 | 0x01=press, 0x00=release |

#### Scenario: Volume up button decoded
- **WHEN** the volume up button on the steering wheel is pressed
- **THEN** the car module enqueues a SWC event with `button_id` matching Raise code 0x01 (Vol+)

---

### Requirement: Gear selector / reverse signal is decoded
The car module SHALL decode the gear selector position and expose it via `car_get_selector()`, with reverse (`e_selector_r`) used to trigger the reverse camera and status flag packet.

⚠ Suspected ID: ~0x165 or ~0x336. Reverse state for camera trigger and status flag packet.

| Signal | Encoding |
|---|---|
| `gear_selector` | 0=P, 1=R, 2=N, 3=D |

#### Scenario: Reverse gear detected
- **WHEN** the gear selector frame is received with reverse position
- **THEN** `car_get_selector()` returns `e_selector_r` within one CAN frame processing cycle

---

### Requirement: Vehicle data signals are decoded (GEM-mirrored)
The car module SHALL decode vehicle speed, engine RPM, coolant temperature, battery voltage, and odometer from GEM-mirrored MS-CAN frames.

⚠ Suspected IDs: ~0x217 (speed/RPM), ~0x420 (temps). Mirrored to MS-CAN by GEM.

| Message ID | Signal | Scale | Offset | Unit |
|---|---|---|---|---|
| ~0x217 | `vehicle_speed` | 0.01 | 0 | km/h |
| ~0x217 | `engine_rpm` | 0.25 | 0 | rpm |
| ~0x420 | `coolant_temp` | 1 | -40 | °C |
| ~0x230 | `battery_voltage` | 0.1 | 0 | V |
| ~0x072 | `odometer` | 1 | 0 | km |

#### Scenario: Speed and RPM decoded while driving
- **WHEN** the GEM speed/RPM frame is received at its normal cycle rate
- **THEN** `car_get_speed()` and `car_get_taho()` return values consistent with actual vehicle speed and engine RPM

---

### Requirement: Door and body status signals are decoded
The car module SHALL decode all door, tailgate, bonnet, and park brake signals from the BCM door status frame.

⚠ Suspected ID: ~0x540 (BCM door status).

| Signal | Encoding |
|---|---|
| `door_fl`, `door_fr`, `door_rl`, `door_rr` | 1=open, 0=closed |
| `tailgate` | 1=open, 0=closed |
| `bonnet` | 1=open, 0=closed |
| `park_brake` | 1=engaged, 0=released |

#### Scenario: Front left door open decoded
- **WHEN** the driver door is opened and the BCM door status frame is received
- **THEN** `car_get_door_fl()` returns 1

---

### Requirement: Lighting status signals are decoded
The car module SHALL decode the near-lights state from the BCM lighting frame and expose it via `car_get_near_lights()`.

⚠ Suspected ID: ~0x4B0 (BCM lighting module). Near-lights state for Raise 0x24 illumination flag.

| Signal | Encoding |
|---|---|
| `near_lights` | 1=sidelights or headlights on, 0=off |

#### Scenario: Headlights on decoded
- **WHEN** the driver turns on headlights and the lighting status frame is received
- **THEN** `car_get_near_lights()` returns 1

---

### Requirement: HVAC/climate signals are decoded
The car module SHALL decode all HVAC signals listed below from the FCIM climate frame and expose them via the corresponding `car_get_air_*()` functions.

⚠ Suspected ID: ~0x3B5 (HVAC module). Climate state for Raise 0x21 AC packet.

| Signal | Encoding |
|---|---|
| `ac_on` | 1=compressor on |
| `fan_speed` | 0–15 (raw), scaled to 0–7 for Raise |
| `driver_temp` | °C × 2 (half-degree steps) |
| `passenger_temp` | °C × 2 |
| `recirculation` | 1=recirculating |
| `airflow_wind` | 1=windscreen vent on |
| `airflow_middle` | 1=middle vents on |
| `airflow_floor` | 1=floor vents on |
| `dual_zone` | 1=dual zone active |

#### Scenario: AC compressor state decoded
- **WHEN** the AC compressor is switched on and the HVAC frame is received
- **THEN** `car_get_air_ac()` returns 1

---

### Requirement: Parking sensor (PDC) distances are decoded
The car module SHALL decode PDC active state and up to 8 sensor distances from the parking aid module frame and expose them via `car_get_radar()`.

⚠ Suspected ID: ~0x5C0 (parking aid module). 8 sensor distances (front + rear) for Raise 0x22/0x23 packets. PDC availability at quad-lock must be verified with FORScan — if absent, radar packets are omitted with no impact on other outputs.

| Signal | Encoding |
|---|---|
| `pdc_rl`, `pdc_rlm`, `pdc_rrm`, `pdc_rr` | 0–99 (0=closest, 99=clear) |
| `pdc_fl`, `pdc_flm`, `pdc_frm`, `pdc_fr` | 0–99 |
| `pdc_active` | 1=sensors active |

#### Scenario: Rear PDC active and distances decoded
- **WHEN** reverse is engaged and the PDC frame is received
- **THEN** `car_get_radar().state` returns `e_radar_on` and distance values are in the range 0–99

---

### Requirement: Steering angle is decoded
The car module SHALL decode the signed steering angle (in degrees × 10) from the EPAS column frame and expose it via `car_get_wheel()`.

⚠ Suspected ID: ~0x080 (EPAS steering column module). Steering angle for Raise 0x26 packet.

| Signal | Scale | Unit | Range |
|---|---|---|---|
| `steering_angle` | ~0.1 | degrees | -540 to +540 |

#### Scenario: Steering angle decoded at full lock
- **WHEN** the steering wheel is at full lock and the EPAS frame is received
- **THEN** `car_get_wheel()` returns a value near ±100 (mapped from raw degrees)
