# Firmware Architecture Spec

## Overview

Portable, layered firmware for decoding CAN bus frames on an STM32 microcontroller.
The firmware is chip-agnostic at the Decoder and Application layers; only the HAL is MCU-specific.

## Layers

### 1. Hardware Abstraction Layer (HAL) — `src/hal/`

**Responsibility:** Wrap all MCU-specific peripheral access.

| Module | File | Responsibility |
|--------|------|----------------|
| CAN peripheral | `can_hal.c/.h` | Init, Tx/Rx, hardware filter config |
| UART | `uart_hal.c/.h` | Debug output at 115200 baud |
| GPIO | `gpio_hal.c/.h` | LED indicators, digital outputs |
| Timers | `timer_hal.c/.h` | Periodic tick, watchdog |

**Requirements:**
- `canmod_hal_can_init(bus_id, baud_kbps)` initialises the CAN peripheral
- `canmod_hal_can_rx(frame)` is non-blocking; returns 0 if no frame available
- Swapping MCU target means only modifying `src/hal/`; decoder and app layers are untouched

### 2. Decoder Layer — `src/can_decoder.c/.h`

**Responsibility:** Stateless extraction of named signals from raw CAN frames.

- Input: `canmod_frame_t { uint32_t id; uint8_t dlc; uint8_t data[8]; }`
- Output: `canmod_signal_t { const char *name; double value; const char *unit; }`
- Handles: bit extraction, little-endian (Intel) and big-endian (Motorola) byte orders, scale, offset
- Message/signal definitions hardcoded initially; code generation from spec is a future goal

**Requirements:**
- `canmod_decode(frame, signals_out, max_signals)` returns count of signals decoded
- Returns 0 for unknown/unfiltered frame IDs
- No dynamic allocation; all buffers caller-provided

### 3. Application Layer — `src/main.c`

**Responsibility:** Wire HAL and Decoder; manage the main loop and output.

**Requirements:**
- Receive loop polls or handles interrupt from HAL CAN Rx
- Each decoded frame is emitted as a JSON line over UART:
  `{"ts":<ms>,"id":"0x201","signals":{"rpm":2350.0,"throttle_position":23.5}}`
- Watchdog reset on hang (> 1 s without a frame on active bus)
- LED heartbeat at 1 Hz to indicate firmware is running

## Directory Structure

```
src/
├── main.c
├── can_decoder.c
├── can_decoder.h
└── hal/
    ├── can_hal.c
    ├── can_hal.h
    ├── uart_hal.c
    ├── uart_hal.h
    ├── gpio_hal.c
    ├── gpio_hal.h
    ├── timer_hal.c
    └── timer_hal.h
tests/
└── test_decoder.c     # Host-runnable unit tests (no MCU needed)
tools/
└── log_parse.py       # PC-side tool to parse UART JSON log
```

## Non-goals (v0)

- BLE / USB output (UART only for now)
- Code generation from openspec specs (manual sync initially)
- Writing/transmitting CAN frames
- Multi-bus simultaneous decode (single bus per firmware build initially)
