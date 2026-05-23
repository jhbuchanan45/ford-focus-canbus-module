## 1. Pre-Implementation Spikes (do before writing any firmware)

- [ ] 1.1 Run FORScan on the car: connect a modified ELM327 or OBDLink MX to OBD-II, enable MS-CAN, capture a full `candump` log for 10 minutes including: engine running, doors open/close, AC on/off, steering lock-to-lock, reverse engaged, parking sensors active. Save as `tools/captures/forscan_baseline.log`.
- [ ] 1.2 Identify and document the confirmed MS-CAN message IDs for each required signal from the FORScan capture. Update all ⚠ entries in `openspec/changes/ford-focus-canbus-decoder/specs/canbus/spec.md` with confirmed IDs and bit layouts.
- [ ] 1.3 Measure resistance across quad-lock MS-CAN H/L (pins A9/A10) with OEM head unit removed. Record result — if ~120Ω, one termination resistor is in the head unit and the canbox board will need a 120Ω resistor fitted; if ~60Ω, termination is elsewhere and no resistor needed.
- [ ] 1.4 Confirm ATOTO S8 MS protocol setting name for Raise: Settings → Factory → CAN box → list all options; photograph the full list and record the exact string used for the Raise Ford Focus profile.

## 2. Repository Structure and Build System

- [ ] 2.1 Restructure source tree to match design: create `src/hal/stm32f1/`, `src/hal/host/`, `src/cars/`, `src/output/` directories. Move or create stub files in each.
- [ ] 2.2 Add `cmake/toolchain-arm.cmake` for arm-none-eabi-gcc targeting Cortex-M3 (STM32F103). Verify: `cmake -DCANMOD_TARGET=stm32f1 -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-arm.cmake ..` configures without error.
- [ ] 2.3 Add `cmake/toolchain-host.cmake` for x86 gcc. Verify: `cmake -DCANMOD_TARGET=host -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-host.cmake ..` configures without error.
- [ ] 2.4 Add libopencm3 as a git submodule at `lib/libopencm3` and wire it into the ARM CMake target.
- [ ] 2.5 Add `CANMOD_OUTPUT` CMake option (`json` | `raise`) and ensure the correct output driver source file is compiled based on the selection.

## 3. car.h API and car module skeleton

- [ ] 3.1 Write `src/car.h` defining all `car_get_*` function declarations, the `e_selector_t` enum, the `struct radar_t` type, and the `canmod_swc_event_t` type. Ensure no CAN frame types leak into this header.
- [ ] 3.2 Write `src/cars/ford_focus_mk3_2015.c` as a skeleton: all `car_get_*` functions returning safe defaults (0 / `e_selector_p` / `e_radar_undef`). Verify it compiles on both ARM and host targets.
- [ ] 3.3 Add the SWC event FIFO (`canmod_swc_event_t` ring buffer, 8 entries) to the car module. Verify: enqueue 8 events then a 9th — the oldest is dropped, newest is stored.

## 4. STM32F103 HAL (ARM target)

- [ ] 4.1 Implement `src/hal/stm32f1/can_hal.c`: bxCAN init at 125 kbps, Rx interrupt, hardware acceptance filters for confirmed MS-CAN IDs from Task 1.2. **Verification**: connect to vcan0 via USB-CAN adapter, send a known MS-CAN frame ID, verify Rx interrupt fires.
- [ ] 4.2 Implement `src/hal/stm32f1/uart_hal.c`: USART2 at 38400 baud, 8N1 for head unit UART; USART1 at 115200 for debug. **Verification**: transmit `0x2E 0x41 0x00 0xBE` (empty 0x41 packet) and verify bytes on USB-UART adapter at 38400.
- [ ] 4.3 Implement `src/hal/stm32f1/usb_hal.c`: USB CDC using libopencm3. **Verification**: connect Blue Pill to laptop, open CDC serial port, verify characters are received.
- [ ] 4.4 Implement `src/hal/stm32f1/timer_hal.c`: SysTick at 1 ms, `hw_tick_get()` returning monotonic ms counter. **Verification**: toggle LED every 500 ms using the tick counter; verify 1 Hz blink rate with a phone stopwatch.
- [ ] 4.5 Implement `src/hal/stm32f1/gpio_hal.c`: PC13 LED heartbeat. **Verification**: heartbeat blinks at 1 Hz on a powered Blue Pill with no other hardware connected.

## 5. Host HAL (x86 target)

- [ ] 5.1 Implement `src/hal/host/can_hal.c`: open a `PF_CAN` SocketCAN socket on the interface named by `--interface` argument (default: `vcan0`). `can_hal_rx()` is non-blocking (uses `O_NONBLOCK`). **Verification**: `sudo ip link add vcan0 type vcan && ip link set up vcan0`, run `canmod-host`, send `cansend vcan0 3B5#0300002B00000000`, verify frame is received.
- [ ] 5.2 Implement `src/hal/host/uart_hal.c`: write Raise packets to a named PTY (printed at startup) so they can be monitored with `cat`. **Verification**: run `canmod-host --output raise`, cat the PTY, inject a frame that triggers a SWC packet, verify `2E 20 02 01 01 ...` bytes appear.
- [ ] 5.3 Implement `src/hal/host/timer_hal.c`: use `clock_gettime(CLOCK_MONOTONIC)` for ms tick. **Verification**: inject 10 frames 10 ms apart via `canplayer`; verify JSON timestamps increment by ~10 ms each.

## 6. Ford Focus MS-CAN Decoder

- [ ] 6.1 Implement vehicle data decoding (speed, RPM, coolant temp) using confirmed IDs from Task 1.2. **Verification**: replay `forscan_baseline.log` on host target, verify `car_get_speed()` returns value matching FORScan reading at the same timestamp.
- [ ] 6.2 Implement battery voltage and odometer decoding. **Verification**: JSON log shows voltage in range 12.0–14.5 V and odometer matches known mileage ±5 km.
- [ ] 6.3 Implement door, tailgate, bonnet, and park brake decoding. **Verification**: replay log segment with door open/close events; JSON output shows `door_fl` toggling 1→0 at the correct timestamp.
- [ ] 6.4 Implement lighting status (near lights) decoding. **Verification**: replay log segment with lights on/off; `near_lights` toggles correctly.
- [ ] 6.5 Implement gear selector / reverse decoding. **Verification**: replay log segment with reverse engaged; `car_get_selector()` returns `e_selector_r` for the correct duration.
- [ ] 6.6 Implement HVAC/climate decoding (AC on, fan speed, temps, recirculation, airflow, dual zone). **Verification**: replay log with AC on at 21°C, fan speed 3; JSON shows matching values.
- [ ] 6.7 Implement steering angle decoding. **Verification**: replay log with lock-to-lock steering; `car_get_wheel()` returns values sweeping from near -100 to +100.
- [ ] 6.8 Implement SWC button decoding — map Ford SWC byte values to Raise button IDs. **Verification**: replay log with Vol+, Vol-, Next, Prev presses; event queue receives correct `canmod_swc_event_t` entries.
- [ ] 6.9 Implement PDC/parking sensor decoding (if confirmed present at quad-lock in Task 1.2). **Verification**: replay log with parking sensors active; `car_get_radar()` returns `e_radar_on` and distance values in 0–99 range.

## 7. JSON Log Output Driver (Phase 1)

- [ ] 7.1 Implement `src/output/json_log.c`: on each decoded frame emit NDJSON line `{"ts":<ms>,"id":"0xXXX","signals":{...}}` over USB CDC. **Verification**: build with `CANMOD_OUTPUT=json`, connect Blue Pill to laptop, open CDC serial, replay a 30-second capture — verify JSON lines appear and values match FORScan readings.
- [ ] 7.2 Capture a 10-minute baseline JSON log from the real car using the Phase 1 firmware. Save to `tools/captures/baseline_json.ndjson`. This becomes the permanent regression corpus.

## 8. Raise UART Output Driver (Phase 2)

- [ ] 8.1 Port `canbox_raise_vw_vehicle_info()` from smartgauges/canbox to `src/output/raise.c`. Adapt to use `canmod_` prefixed HAL calls. **Verification**: build with `CANMOD_OUTPUT=raise` on host target, replay a driving capture, verify 0x41/0x02 packets appear on the PTY at ~500 ms intervals with correct speed bytes.
- [ ] 8.2 Implement door status packet (0x41/0x01). **Verification**: replay log with door open; verify 0x41 0x01 packet transmitted with correct door bit set.
- [ ] 8.3 Implement warning flags packet (0x41/0x03). **Verification**: inject a low-fuel frame; verify 0x41 0x03 packet with bit7 set is transmitted.
- [ ] 8.4 Implement status flags packet (0x24) — reverse, park brake, near lights. **Verification**: replay reverse engage segment; verify 0x24 packet with bit0=1 transmitted.
- [ ] 8.5 Implement parking active packet (0x25). **Verification**: replay PDC-active segment; verify 0x25 0x02 packet transmitted on activation, 0x25 0x00 on deactivation.
- [ ] 8.6 Implement rear radar packet (0x22) and front radar packet (0x23). **Verification**: replay PDC segment with known distances; verify radar packets contain correctly scaled and inverted distance values (formula: `(rmax + 1) - scale(raw, 0, 99, 0, rmax)` with max=10).
- [ ] 8.7 Implement steering angle packet (0x26). **Verification**: replay steering segment; verify 0x26 packets contain signed int16_t values in range -540 to +540.
- [ ] 8.8 Implement AC status packet (0x21) — all 5 bytes (AC on, fan speed, temps, recirculation, airflow, dual zone, seat heat). **Verification**: replay AC-on segment at 21°C, fan 3; verify 0x21 packet byte layout matches canbox.c encoding.
- [ ] 8.9 Implement SWC button packet (0x20) — press and release. **Verification**: replay SWC segment with Vol+ press; verify 0x20 `[0x01, 0x01]` followed immediately by 0x20 `[0x01, 0x00]`.
- [ ] 8.10 Implement head unit RX handler: parse incoming 0x2E packets from ATOTO, send 0xFF ACK. **Verification**: connect to ATOTO UART via USB-UART adapter, power on ATOTO in Raise mode, verify 0xFF ACKs are transmitted after each head unit packet.

## 9. Hardware Integration and ATOTO Validation

- [ ] 9.1 Wire breadboard: Blue Pill + TJA1042 transceiver connected to quad-lock pins A9/A10 (MS-CAN H/L), power from quad-lock +12V ACC via LDO to 3.3V, head unit UART wired to ATOTO 8-pin harness. **Verification**: power on with car ignition, LED heartbeat blinks, USB CDC shows JSON log lines.
- [ ] 9.2 Flash Phase 1 (JSON) firmware and verify all signals decode correctly on the live car: speed, RPM, temp, doors, AC, reverse, SWC. Compare against FORScan values simultaneously. Resolve any ID mismatches.
- [ ] 9.3 Flash Phase 2 (Raise) firmware. Set ATOTO to Raise protocol. **Verification**: steering wheel Vol+/Vol- controls ATOTO volume; speed visible in ATOTO vehicle info widget; reverse gear triggers rear camera input.
- [ ] 9.4 Verify AC display on ATOTO: turn AC on at 21°C, fan speed 3 — confirm ATOTO climate widget shows matching values.
- [ ] 9.5 Verify PDC display on ATOTO (if PDC confirmed on quad-lock): engage reverse near a wall, confirm ATOTO shows parking sensor bars updating.
- [ ] 9.6 Verify door status: open each door individually, confirm ATOTO door graphic updates.
- [ ] 9.7 Stress test: 30-minute drive with Raise firmware. Confirm no UART lockup, no missed SWC presses, no watchdog resets. Log any anomalies.

## 10. Documentation and Spec Sync

- [ ] 10.1 Update `openspec/changes/ford-focus-canbus-decoder/specs/canbus/spec.md` with all confirmed MS-CAN IDs (replacing ⚠ placeholders) based on Tasks 1.2 and 9.2 findings.
- [ ] 10.2 Add `docs/hardware-setup.md` covering: breadboard wiring diagram, quad-lock pin tap, TJA1042 connections, power supply, and USB-UART for debug.
- [ ] 10.3 Add `docs/dev-workflow.md` covering: WSL2 setup, vcan0 creation, replay commands, building ARM and host targets, flashing with ST-Link.
- [ ] 10.4 Add `tools/replay.sh` convenience script: creates vcan0, runs canplayer on a log file, and pipes output to `canmod-host`.
- [ ] 10.5 Archive this change: run `/opsx:archive` — sync delta specs to main `openspec/specs/` and move to archive.
