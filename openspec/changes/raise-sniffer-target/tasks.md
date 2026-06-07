## 1. CMake changes

- [ ] 1.1 Add `sniffer` to the `CANMOD_OUTPUT` string property list in `CMakeLists.txt` (alongside `json` and `raise`). Update the `FATAL_ERROR` message to include `sniffer` in the valid values list.
- [ ] 1.2 Add a `CANMOD_OUTPUT STREQUAL "sniffer"` branch inside the `CANMOD_TARGET STREQUAL "stm32f1"` block. This branch SHALL add a second executable target `canmod-sniffer` with sources: `src/sniffer_main.c`, `src/output/raise_sniffer.c`, and all `HAL_SOURCES`. It SHALL NOT include `src/main.c` or `src/cars/ford_focus_mk3_2015.c`. **Verification**: `cmake -DCANMOD_TARGET=stm32f1 -DCANMOD_OUTPUT=sniffer -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-arm.cmake ..` configures without error and `cmake --build .` produces `canmod-sniffer.elf` and `canmod-sniffer.bin`.
- [ ] 1.3 Add post-build `objcopy` and `size` commands for `canmod-sniffer` matching the existing ones for `canmod`. **Verification**: `canmod-sniffer.bin` exists after build and `arm-none-eabi-size` output is printed.

## 2. raise_sniffer.c / raise_sniffer.h

- [ ] 2.1 Create `src/output/raise_sniffer.h` declaring one public function: `void sniffer_rx_poll(void)`.
- [ ] 2.2 Implement the RX state machine in `src/output/raise_sniffer.c`. States: `WAIT_SOF → CMD → LEN → DATA → CSUM`. Static state: `rx_cmd`, `rx_len`, `rx_remaining`, `rx_csum_accum`, `rx_data[16]`, `rx_data_idx`. On `WAIT_SOF`: advance only on `0x2E`. On `LEN`: if `len == 0` skip to `CSUM`; if `len > 16` set a `rx_truncated` flag and cap `rx_remaining` at 16. **Verification**: unit-readable state machine with no external dependencies beyond `uart_hal.h` and `usb_hal.h`.
- [ ] 2.3 Implement the log line formatter inside `sniffer_rx_poll()`. On reaching `CSUM` state: compute expected checksum over `rx_cmd + rx_len + sum(rx_data[0..rx_data_idx-1])`; compare to received byte; format one ASCII line to a local stack buffer using the format `T+NNNNNN HH HH HH | HH ... | C=HH OK\n` (or `BAD` / `TRUNC`); call `usb_hal_write()`. **Verification**: known byte sequence fed to state machine produces exactly the expected log line string byte-for-byte.
- [ ] 2.4 Send 0xFF ACK via `uart_hal_tx(0xFF)` immediately after `usb_hal_write()` if and only if the checksum was valid (not `BAD`, not `TRUNC`). **Verification**: valid packet → ACK transmitted; bad checksum → no ACK.
- [ ] 2.5 Reset all static state to `WAIT_SOF` after processing each packet (valid or invalid). **Verification**: two consecutive valid packets are both logged correctly with no state bleed.

## 3. sniffer_main.c

- [ ] 3.1 Create `src/sniffer_main.c` with a `main()` for STM32 only (no `--interface` argument parsing, no `#ifdef CANMOD_TARGET_HOST` branch). Call: `clock_setup()`, `timer_hal_init()`, `gpio_hal_init()`, `usb_hal_init()`, `uart_hal_init()`. Do NOT call `can_hal_init()`. **Verification**: firmware boots on Blue Pill, heartbeat LED blinks, USB CDC enumerates on host PC.
- [ ] 3.2 Implement the main loop: call `sniffer_rx_poll()`, `usb_hal_poll()`, and toggle the heartbeat LED at 1 Hz using `hw_tick_get()`. No periodic send timers. **Verification**: with nothing connected to PA3, no log lines appear on USB CDC; heartbeat blinks at 1 Hz.

## 4. Integration and hardware verification

- [ ] 4.1 Wire Blue Pill 2: PA3 → FD60 UART TX; PA2 → FD60 UART RX; GND common. Flash `canmod-sniffer.bin`. Open USB CDC serial port at 115200 on PC. **Verification**: with car ignition on and FD60 powered, log lines appear at regular intervals (vehicle info packets every ~500 ms).
- [ ] 4.2 Verify ACK behaviour: disconnect PA2 (no ACKs sent to FD60). **Verification**: FD60 stops transmitting within a few seconds, confirming ACK is required. Reconnect PA2 and verify transmissions resume.
- [ ] 4.3 Capture a 10-minute FD60 log with the car running: engine on, doors opened/closed, AC on/off, reverse engaged, steering lock-to-lock. Save to `tools/captures/fd60_baseline.log`. **Verification**: log contains all nine Raise CMD types (0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x41 subcmd 0x01, 0x41 subcmd 0x02, 0x41 subcmd 0x03) or documents which types are absent.
- [ ] 4.4 Compare FD60 log against custom firmware output: for a given driving scenario, verify that the same CMD packets appear at similar rates and that payload bytes match (speed, RPM, door bits, etc.). Document any discrepancies in `docs/fd60-comparison.md`.
