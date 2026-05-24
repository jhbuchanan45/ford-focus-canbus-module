## ADDED Requirements

### Requirement: CTest unit test suite runs on the host build with no hardware
A `canmod-tests` executable SHALL be built as part of the `CANMOD_TARGET=host` CMake configuration and registered with CTest. Running `ctest --test-dir build/host` SHALL execute all unit tests and report pass/fail with no hardware, no vcan0, and no root privileges required.

#### Scenario: Test suite runs clean from a fresh checkout
- **WHEN** a developer runs `cmake -DCANMOD_TARGET=host -DCANMOD_OUTPUT=json -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-host.cmake -B build/host . && cmake --build build/host && ctest --test-dir build/host`
- **THEN** all registered tests pass and the exit code is 0

#### Scenario: Test failure reported with file and line
- **WHEN** a `TEST_ASSERT` condition evaluates to false
- **THEN** the test runner prints the failing file name, line number, and condition string, and exits with a non-zero code

---

### Requirement: SWC ring buffer is fully covered by unit tests
The `car_swc_dequeue` / `swc_enqueue` ring buffer (8-entry, drop-oldest) SHALL have test cases covering normal operation, boundary conditions, and overflow.

#### Scenario: Dequeue from empty queue returns 0
- **WHEN** `car_swc_dequeue()` is called on an empty queue
- **THEN** it returns 0 and does not modify the output pointer

#### Scenario: Events are dequeued in FIFO order
- **WHEN** three events with distinct button IDs are enqueued and then dequeued
- **THEN** they are returned in the same order they were enqueued

#### Scenario: Queue full — oldest entry is dropped on overflow
- **WHEN** 8 events are enqueued and then a 9th is enqueued
- **THEN** the 9th event is stored, the 1st event is lost, and subsequent dequeues return events 2 through 9 in order

#### Scenario: Queue wraps around correctly after drain and refill
- **WHEN** 8 events are enqueued, 8 are dequeued, and then 4 more are enqueued
- **THEN** the 4 new events are dequeued correctly with no corruption

---

### Requirement: Raise packet encoders are verified byte-for-byte
Each `raise_send_*()` function SHALL have a unit test that calls it with known `car_get_*()` state, captures the bytes written to the UART mock, and asserts the full byte sequence including header, payload, and checksum.

#### Scenario: Vehicle info packet checksum is correct
- **WHEN** `raise_send_vehicle_info()` is called with speed=5000 (50.00 km/h), rpm=2000, temp=90°C, voltage=12600 mV, odometer=12345 km
- **THEN** the transmitted bytes begin with `0x2E 0x41` and the final byte equals `(0x41 + LEN + sum(DATA)) ^ 0xFF`

#### Scenario: Door status packet encodes all bits correctly
- **WHEN** `raise_send_doors()` is called with FL and RR doors open and all others closed
- **THEN** the door byte in the packet has bit0=1, bit1=0, bit2=0, bit3=1, bit4=0, bit5=0

#### Scenario: Status flags packet encodes reverse correctly
- **WHEN** the car module state has selector=`e_selector_r`
- **THEN** the `0x24` packet byte has bit0=1

#### Scenario: Steering angle packet uses big-endian signed int16
- **WHEN** `raise_send_steering()` is called with wheel=-3600 (−360.0°)
- **THEN** the two angle bytes in the `0x26` packet equal `0xFF 0x9C` (big-endian representation of −360 as int16)

#### Scenario: PDC distance inversion formula is correct
- **WHEN** a rear sensor raw distance of 0 (closest obstacle) is encoded
- **THEN** the Raise value in the `0x22` packet is 11 (`RMAX + 1 - 0 = 11`)

#### Scenario: PDC clear distance maps to zero
- **WHEN** a rear sensor raw distance of 99 (RADAR_DIST_CLEAR) is encoded
- **THEN** the Raise value in the `0x22` packet is 0

#### Scenario: SWC press packet has correct layout
- **WHEN** `raise_send_swc(0x01, 1)` is called (Vol+ press)
- **THEN** the transmitted bytes are `0x2E 0x20 0x02 0x01 0x01 <CSUM>` where CSUM = `(0x20 + 0x02 + 0x01 + 0x01) ^ 0xFF`

#### Scenario: AC status packet encodes all five bytes
- **WHEN** `raise_send_ac()` is called with ac_on=1, fan=3, driver_temp=42 (21°C×2), pass_temp=42, recirculation=0, wind=1, middle=1, floor=0, dual_zone=0
- **THEN** the `0x21` packet DATA bytes are `[0x01, 0x03, 0x2A, 0x2A, 0x03]` in that order

---

### Requirement: JSON log output is verified for correctness and format
Each `json_log_emit()` call for a known message ID SHALL produce a valid NDJSON line with the correct `id` string, correct signal field names, and correctly scaled numeric values.

#### Scenario: Speed is emitted as floating-point km/h
- **WHEN** `json_log_emit()` is called for ID `0x217` with `car_get_speed()` returning 5050 (50.50 km/h)
- **THEN** the emitted JSON contains `"speed_kmh":50.50`

#### Scenario: Battery voltage is emitted in volts
- **WHEN** `json_log_emit()` is called for ID `0x230` with `car_get_voltage()` returning 12600 (12.600 V)
- **THEN** the emitted JSON contains `"battery_v":12.600`

#### Scenario: HVAC temperatures are emitted as half-degree floats
- **WHEN** `json_log_emit()` is called for ID `0x3B5` with `car_get_air_temp_driver()` returning 43 (21.5°C)
- **THEN** the emitted JSON contains `"temp_driver":21.5`

#### Scenario: Unknown frame ID produces no output
- **WHEN** `json_log_emit()` is called for an unrecognised ID such as `0xABC`
- **THEN** nothing is written to the USB mock capture buffer

---

### Requirement: Signal decode tests provide a scaffold for post-FORScan implementation
One test file SHALL exist per MS-CAN message ID (`test_decode_<name>.c`). Each file SHALL contain `TODO`-marked test stubs for every signal in that message. The stubs SHALL compile and be registered with CTest as `SKIP` until the decode logic is implemented.

#### Scenario: Stub tests are visible in CTest output
- **WHEN** `ctest --test-dir build/host -V` is run before any decode logic is implemented
- **THEN** each stub test appears in the output with status `Not Run` or `Skip` and no test is reported as `FAILED`
