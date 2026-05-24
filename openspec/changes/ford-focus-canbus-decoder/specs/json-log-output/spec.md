## ADDED Requirements

### Requirement: JSON log driver emits decoded signal state over USB CDC
The JSON log output driver SHALL emit newline-delimited JSON (NDJSON) over USB CDC at 115200 baud. One JSON object is emitted per decoded CAN frame, containing a timestamp, the CAN message ID, and all decoded signal name/value pairs.

#### Scenario: Frame decoded and logged
- **WHEN** a MS-CAN frame is received and decoded by the car module
- **THEN** within one main loop tick the JSON log driver emits a line of the form:
  `{"ts":12345,"id":"0x3B5","signals":{"fan_speed":3,"target_temp_driver":21.5,"ac_compressor_on":1}}`

#### Scenario: Unknown frame not logged
- **WHEN** a CAN frame is received that the car module does not recognise
- **THEN** nothing is emitted (unknown frames are not echoed as raw hex)

---

### Requirement: JSON log driver is selected at compile time
The JSON log driver SHALL be activated by setting `CANMOD_OUTPUT=json` in the CMake configuration. When active, the Raise UART output SHALL be disabled. The two drivers SHALL NOT run simultaneously in the same build.

#### Scenario: JSON build produces log output, no Raise packets
- **WHEN** firmware is built with `CANMOD_OUTPUT=json`
- **THEN** USB CDC emits JSON lines on each decoded frame and the head unit UART line is silent

#### Scenario: Raise build produces Raise packets, no JSON
- **WHEN** firmware is built with `CANMOD_OUTPUT=raise`
- **THEN** the head unit UART line transmits Raise packets and USB CDC is available for debug prints only

---

### Requirement: Timestamps are milliseconds since boot
The `ts` field in each JSON object SHALL be a monotonic millisecond counter since firmware boot, wrapping at 2^32 ms (≈ 49 days). No real-time clock is required.

#### Scenario: Timestamp increments correctly
- **WHEN** two frames are logged 10 ms apart
- **THEN** the second frame's `ts` value is 10 greater than the first (±1 ms timer resolution)

---

### Requirement: JSON log is usable as a replay corpus
Log files captured from the car (using the JSON build on real hardware) SHALL be replayable against the firmware on the PC dev emulation target using `canplayer` or a Python replay script. This enables regression testing without the car.

#### Scenario: Captured log used for PC regression
- **WHEN** a `.log` file captured during a real drive is replayed on vcan0
- **THEN** the firmware running on the x86 host target emits identical JSON output to the original capture (signal values match; timestamps will differ)
