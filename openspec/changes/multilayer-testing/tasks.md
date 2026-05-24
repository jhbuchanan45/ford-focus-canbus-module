## 1. Test Infrastructure and HAL Mocks

- [x] 1.1 Create `tests/` directory. Add `tests/CMakeLists.txt` that defines a `canmod-tests` executable, links the car module and output driver sources, substitutes mock HAL files, and calls `add_test(NAME canmod-tests COMMAND canmod-tests)`.
- [x] 1.2 Add `enable_testing()` and `add_subdirectory(tests)` to the root `CMakeLists.txt`, guarded by `if(CANMOD_TARGET STREQUAL "host")`. Verify: `cmake -DCANMOD_TARGET=host ... -B build/host .` then `cmake --build build/host` produces a `canmod-tests` binary.
- [x] 1.3 Write `tests/mocks/uart_hal.c`: capture buffer (256 bytes), `mock_uart_reset()`, `mock_uart_get_buf()`, `mock_uart_get_len()`. Implement `uart_hal_init()`, `uart_hal_tx()`, `uart_hal_rx()` against the capture buffer. **Verification**: `uart_hal_tx(0xAB)` → `mock_uart_get_buf()[0] == 0xAB`.
- [x] 1.4 Write `tests/mocks/usb_hal.c`: string capture buffer (4096 bytes), `mock_usb_reset()`, `mock_usb_get_buf()`, `mock_usb_get_len()`. Implement `usb_hal_init()`, `usb_hal_write()`, `usb_hal_poll()`. **Verification**: `usb_hal_write((uint8_t*)"hi", 2)` → `strcmp(mock_usb_get_buf(), "hi") == 0`.
- [x] 1.5 Write `tests/mocks/can_hal.c`, `tests/mocks/gpio_hal.c`, `tests/mocks/timer_hal.c` as no-op stubs (needed for linking). `timer_hal.c` should return a monotonically incrementing value from `hw_tick_get()`.
- [x] 1.6 Write `tests/test_runner.h`: `TEST_ASSERT(cond)` macro that prints `FAIL: <file>:<line>: <cond>` and sets a global failure flag. Add `test_runner_result()` returning 0 on all-pass, 1 on any failure. Tests call this in `main()` to set the exit code.

## 2. SWC Ring Buffer Tests

- [x] 2.1 Write `tests/test_swc_queue.c`. Include tests for: dequeue from empty returns 0, single enqueue+dequeue roundtrip, FIFO order preserved across 4 events, 9th enqueue drops oldest, drain-and-refill wrap-around. Register with CTest. **Verification**: `ctest -R swc` passes.

## 3. Raise Packet Encoder Tests

- [x] 3.1 Write `tests/test_raise_vehicle_info.c`: set known car state (speed=5000, rpm=2000, temp=90, voltage=12600, odo=12345), call `raise_send_vehicle_info()`, assert `mock_uart_get_buf()` matches expected byte sequence byte-for-byte including checksum. **Verification**: `ctest -R raise_vehicle_info` passes.
- [x] 3.2 Write `tests/test_raise_doors.c`: test all six door/body bits individually and in combination. Assert `0x41 0x01` subcmd and correct door byte. **Verification**: `ctest -R raise_doors` passes.
- [x] 3.3 Write `tests/test_raise_status_flags.c`: test reverse (bit0), park brake (bit1), near lights (bit2) independently and combined. Assert `0x24` CMD and correct flags byte. **Verification**: `ctest -R raise_status` passes.
- [x] 3.4 Write `tests/test_raise_steering.c`: test positive angle (360°→ wheel=3600), negative angle (−360°→ wheel=−3600), zero, and full lock (±540°→ wheel=±5400). Assert `0x26` CMD and big-endian int16 bytes. **Verification**: `ctest -R raise_steering` passes.
- [x] 3.5 Write `tests/test_raise_radar.c`: test the `dist_to_raise()` formula: raw=0→11, raw=49→~6, raw=98→1, raw=99→0. Test rear (`0x22`) and front (`0x23`) packet layout with all 4 sensors. **Verification**: `ctest -R raise_radar` passes.
- [x] 3.6 Write `tests/test_raise_ac.c`: test with AC on/off, fan speeds 0 and 7, both temperatures, all airflow combinations, recirculation, dual zone. Assert `0x21` CMD and all 5 DATA bytes. **Verification**: `ctest -R raise_ac` passes.
- [x] 3.7 Write `tests/test_raise_swc.c`: test Vol+ press `raise_send_swc(0x01, 1)` → `2E 20 02 01 01 DB`, Vol+ release `raise_send_swc(0x01, 0)` → `2E 20 02 01 00 DC`, and all other button IDs. **Verification**: `ctest -R raise_swc` passes.
- [x] 3.8 Write `tests/test_raise_rx.c`: feed a valid `0x2E CMD LEN DATA CSUM` byte sequence to `raise_rx_poll()` via `uart_hal_rx` mock, assert `0xFF` ACK is transmitted. Feed a byte sequence with wrong checksum, assert no ACK. **Verification**: `ctest -R raise_rx` passes.

## 4. JSON Log Formatter Tests

- [x] 4.1 Write `tests/test_json_speed.c`: set speed=5050, call `json_log_emit(100, 0x217)`, assert `mock_usb_get_buf()` contains `"speed_kmh":50.50` and ends with `\n`. **Verification**: `ctest -R json_speed` passes.
- [x] 4.2 Write `tests/test_json_battery.c`: set voltage=12600, call `json_log_emit(100, 0x230)`, assert `"battery_v":12.600`. **Verification**: `ctest -R json_battery` passes.
- [x] 4.3 Write `tests/test_json_hvac.c`: set ac_on=1, fan=3, temp_driver=43 (21.5°C), call `json_log_emit(100, 0x3B5)`, assert `"ac_on":1`, `"fan_speed":3`, `"temp_driver":21.5`. **Verification**: `ctest -R json_hvac` passes.
- [x] 4.4 Write `tests/test_json_doors.c`: set FL door open, call `json_log_emit(100, 0x540)`, assert `"door_fl":1` and all other doors 0. **Verification**: `ctest -R json_doors` passes.
- [x] 4.5 Write `tests/test_json_unknown.c`: call `json_log_emit(100, 0xABC)`, assert `mock_usb_get_len() == 0` (no output for unknown ID). **Verification**: `ctest -R json_unknown` passes.

## 5. Signal Decode Test Scaffolding

- [x] 5.1 Write `tests/test_decode_speed_rpm.c`: add `TODO`-stub test cases for speed decode (raw byte array → `car_get_speed()`) and RPM decode. Mark with `#if 0 /* TODO task 6.1 */` guard so they compile but are skipped. Register with CTest.
- [x] 5.2 Write `tests/test_decode_hvac.c`: stub cases for all HVAC signals (ac_on, fan, temps, airflow, recirculation, dual_zone). Guard with `#if 0`.
- [x] 5.3 Write `tests/test_decode_doors.c`: stub cases for all door/body bits and park brake. Guard with `#if 0`.
- [x] 5.4 Write `tests/test_decode_steering.c`: stub cases for positive and negative steering angles. Guard with `#if 0`.
- [x] 5.5 Write `tests/test_decode_misc.c`: stub cases for battery, odometer, coolant temp, lighting, gear selector, PDC, SWC mapping. Guard with `#if 0`.
- [x] 5.6 Verify: `ctest --test-dir build/host -V` shows all stub test files in output with no `FAILED` entries.

## 6. Regression Test Runner

- [x] 6.1 Write `tests/test_regression.c`: at runtime check for `tools/captures/forscan_baseline.log` and `tools/captures/forscan_baseline_golden.ndjson`. If either is absent, print `SKIP: capture files not present` and exit 0. Otherwise spawn `canmod-host`, replay the log via `canplayer`, strip `"ts"` fields from output, and diff against golden file.
- [x] 6.2 Update `tools/replay.sh` to accept an optional `--golden <file>` argument. When provided, capture `canmod-host` stdout, strip `ts` fields with `sed`, and diff against the golden file. Exit non-zero and print a unified diff on mismatch.

## 7. CI Pipeline

- [x] 7.1 Create `.github/workflows/ci.yml`. Steps: checkout (with submodules), install `gcc` and `cmake`, configure with `CANMOD_TARGET=host CANMOD_OUTPUT=json`, build, run `ctest --test-dir build/host --output-on-failure`. Run on push and pull_request to all branches.
- [x] 7.2 Verify CI passes: push the branch, confirm the Actions run completes green with all tests reported.
