# Firmware Architecture Spec

## Purpose

Portable, layered firmware for decoding Ford Focus Mk3 MS-CAN frames on an STM32F103C8T6 and outputting the Raise VW PQ protocol to an ATOTO S8 MS head unit. The firmware is chip-agnostic at the Car Module and Output Driver layers; only the HAL is MCU-specific.

## Layers

### 1. Hardware Abstraction Layer (HAL) — `src/hal/`

**Responsibility:** Wrap all MCU-specific peripheral access. Swapping MCU target means only modifying `src/hal/`; all layers above are untouched.

| Module | File | Responsibility |
|--------|------|----------------|
| CAN peripheral | `can_hal.c/.h` | Init bxCAN at 125 kbps, Rx via interrupt, hardware acceptance filters |
| UART (head unit) | `uart_hal.c/.h` | Raise protocol UART at 38400 baud 8N1 |
| USB CDC | `usb_hal.c/.h` | USB CDC output for JSON log driver (Phase 1) |
| GPIO | `gpio_hal.c/.h` | PC13 LED heartbeat |
| Timers | `timer_hal.c/.h` | 1 ms SysTick, `hw_tick_get()` monotonic counter |

Two HAL implementations exist:

| Target | Directory | Backend |
|--------|-----------|---------|
| STM32F103C8T6 | `src/hal/stm32f1/` | libopencm3, bxCAN remapped to PB8/PB9 |
| x86 host | `src/hal/host/` | SocketCAN (`PF_CAN`), PTY UART, stdout USB |

### 2. Car Module — `src/cars/ford_focus_mk3_2015.c`

**Responsibility:** The ONLY file that reads raw CAN frame data. Maintains internal signal state and exposes it exclusively through the `car_get_*` API. Swapping vehicle model means replacing only this file.

- Entry point: `car_process_frame(id, data, dlc)` — called by the main loop for every received frame
- Output: `car_get_*()` functions defined in `src/car.h`
- SWC events: internal 8-entry drop-oldest FIFO, consumed via `car_swc_dequeue()`

### 3. Output Driver — `src/output/`

**Responsibility:** Read car state via `car_get_*()` and produce output. Two drivers, selected at compile time by `-DCANMOD_OUTPUT=json|raise`:

| Driver | File | Output |
|--------|------|--------|
| JSON log | `json_log.c` | NDJSON over USB CDC — Phase 1 development |
| Raise VW PQ | `raise.c` | Raise protocol packets over UART 38400 — Phase 2 production |

### 4. Application Layer — `src/main.c`

**Responsibility:** Wire HAL, car module, and output driver. Run the main loop with periodic output timers.

- Initialises all HAL modules and the output driver
- Polls `can_hal_rx()` and calls `car_process_frame()` on each received frame
- Maintains periodic timers (500 ms: vehicle info, doors, warnings, status, AC; 100 ms: steering, radar when active)
- Dispatches SWC events immediately via `car_swc_dequeue()`

## Directory Structure

```
src/
├── main.c
├── car.h                          # Public car module API (no CAN types)
├── cars/
│   └── ford_focus_mk3_2015.c     # Vehicle-specific MS-CAN decoder
├── hal/
│   ├── can_hal.h / uart_hal.h / usb_hal.h / gpio_hal.h / timer_hal.h
│   ├── stm32f1/                  # STM32F103 implementation (libopencm3)
│   └── host/                     # x86 SocketCAN stub
└── output/
    ├── raise.c / raise.h         # Raise VW PQ UART protocol
    └── json_log.c / json_log.h   # NDJSON USB CDC log
tests/
├── mocks/                        # HAL mock implementations (CTest)
├── test_runner.h                 # TEST_ASSERT macros
└── test_*.c                      # Per-capability test executables
cmake/
├── toolchain-arm.cmake           # arm-none-eabi-gcc, Cortex-M3
└── toolchain-host.cmake          # x86 gcc
lib/
└── libopencm3/                   # Git submodule (ARM target only)
```

## Requirements

### Requirement: Build system supports ARM and x86 host targets via CMake
The project SHALL use CMake with two toolchain files: `cmake/toolchain-arm.cmake` (arm-none-eabi-gcc, Cortex-M3) and `cmake/toolchain-host.cmake` (gcc for x86). Output driver is selected via `-DCANMOD_OUTPUT=json|raise`. Target is selected via `-DCANMOD_TARGET=stm32f1|host`.

When `CANMOD_TARGET=host`, the build SHALL additionally produce all test executables registered with CTest.

#### Scenario: ARM firmware build
- **WHEN** `cmake -DCANMOD_TARGET=stm32f1 -DCANMOD_OUTPUT=raise -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-arm.cmake ..`
- **THEN** an `.elf` and `.bin` are produced targeting STM32F103 with Raise UART output

#### Scenario: Host binary build
- **WHEN** `cmake -DCANMOD_TARGET=host -DCANMOD_OUTPUT=json -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-host.cmake ..`
- **THEN** a `canmod-host` x86 executable is produced with JSON log output and SocketCAN HAL

#### Scenario: Host test build
- **WHEN** `cmake -DCANMOD_TARGET=host -DCANMOD_OUTPUT=json -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-host.cmake .. && cmake --build . && ctest`
- **THEN** all registered test executables are built and CTest reports all tests passing

## Non-goals (v1)

- BLE output
- Multi-bus simultaneous decode (MS-CAN only)
- Writing / transmitting CAN frames
- OBD-II PID parsing
