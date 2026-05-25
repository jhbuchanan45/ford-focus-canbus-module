# Ford Focus Mk3 MS-CAN Decoder

Firmware for decoding **Ford Focus Mk3 (2015) MS-CAN** frames on an **STM32F103C8T6** (Blue Pill) and outputting the **Raise VW PQ protocol** to an ATOTO S8 MS Android head unit.

> **Status:** Phase 1 in progress — MS-CAN signal decoding implemented and tested on host; pending FORScan verification on live vehicle before hardware integration.

---

## What It Does

The canbox connects inline between the quad-lock MS-CAN bus (pins A9/A10) and the ATOTO head unit UART port. It translates raw CAN frames into Raise VW PQ packets, giving the head unit:

- 🎵 Steering wheel controls (Vol+/−, Next, Prev, Mode, Mute, Answer, Hangup)
- 🚗 Vehicle info (speed, RPM, coolant temp, battery voltage, odometer)
- ❄️ Climate display (AC on/off, fan speed, driver/passenger temps, airflow, recirculation)
- 🚪 Door status (FL, FR, RL, RR, tailgate, bonnet)
- 🅿️ Status flags (reverse gear, park brake, near lights)
- 📡 Parking sensors (8 PDC distances — front + rear, if present at quad-lock)
- 🔄 Steering angle

All signals are decoded from **MS-CAN at 125 kbps** (the only bus at the quad-lock connector). Powertrain signals (speed, RPM, temp) are available on MS-CAN as GEM-mirrored copies of the HS-CAN bus.

---

## Hardware

| Component | Value |
|-----------|-------|
| MCU | STM32F103C8T6 (Blue Pill) |
| CAN transceiver | TJA1042T/3 |
| CAN bus | MS-CAN, 125 kbps, quad-lock pins A9 (H) / A10 (L) |
| Head unit UART | USART2, PA2/PA3, 38400 baud 8N1 (Raise VW PQ) |
| Debug UART | USART1, PA9/PA10, 115200 baud |
| JSON log (Phase 1) | USB CDC, PA11/PA12 |
| HAL library | libopencm3 |

See **[`docs/hardware-setup.md`](./docs/hardware-setup.md)** for the full wiring diagram and first power-on checklist.

---

## Building

### Prerequisites

```bash
# Build tools
sudo apt install build-essential cmake gcc git can-utils

# ARM cross-compiler (for STM32 target)
sudo apt install gcc-arm-none-eabi binutils-arm-none-eabi

# Initialise libopencm3 submodule (ARM target only)
git submodule update --init --recursive
```

### Host target (x86) — development and testing

```bash
mkdir -p build/host && cd build/host

# Phase 1: JSON log output over USB CDC (for signal verification)
cmake -DCANMOD_TARGET=host -DCANMOD_OUTPUT=json \
      -DCMAKE_TOOLCHAIN_FILE=../../cmake/toolchain-host.cmake ../..
cmake --build . -j$(nproc)
# → build/host/canmod-host

# Phase 2: Raise VW PQ UART output
cmake -DCANMOD_TARGET=host -DCANMOD_OUTPUT=raise \
      -DCMAKE_TOOLCHAIN_FILE=../../cmake/toolchain-host.cmake ../..
cmake --build . -j$(nproc)
```

### ARM target (STM32F103)

```bash
mkdir -p build/arm && cd build/arm
cmake -DCANMOD_TARGET=stm32f1 -DCANMOD_OUTPUT=raise \
      -DCMAKE_TOOLCHAIN_FILE=../../cmake/toolchain-arm.cmake ../..
cmake --build . -j$(nproc)
# → build/arm/canmod-stm32f1.elf  canmod-stm32f1.bin
```

### Tests

```bash
cd build/host && ctest --output-on-failure
# Expected: 20/20 passing
```

---

## Replaying a CAN Capture

```bash
# Create virtual CAN interface
sudo modprobe vcan
sudo ip link add dev vcan0 type vcan && sudo ip link set up vcan0

# Run canmod-host in one terminal
./build/host/canmod-host --interface vcan0

# Replay a capture in another terminal
canplayer -I tools/captures/forscan_baseline.log vcan0=can0

# Or use the convenience wrapper
tools/replay.sh tools/captures/forscan_baseline.log
```

See **[`docs/dev-workflow.md`](./docs/dev-workflow.md)** for the full development workflow including WSL2 setup, flashing, and golden-file regression testing.

---

## Architecture

```
┌─────────────────────────────────────────────────────┐
│  Application layer  (src/main.c)                    │
│  Periodic timers, main loop, SWC dispatch           │
├──────────────┬──────────────────────────────────────┤
│  Car module  │  Output driver                       │
│  src/cars/   │  src/output/raise.c   (Phase 2)      │
│  ford_focus  │  src/output/json_log.c (Phase 1)     │
│  _mk3_2015.c │                                      │
│              │  Selected by -DCANMOD_OUTPUT=raise|json│
├──────────────┴──────────────────────────────────────┤
│  HAL  (src/hal/)                                    │
│  stm32f1/ — bxCAN, USART2, USB CDC, SysTick, GPIO  │
│  host/    — SocketCAN (vcan0), PTY UART, clock_mono │
└─────────────────────────────────────────────────────┘
```

Only `src/cars/ford_focus_mk3_2015.c` reads raw CAN frame data. All other layers interact exclusively through the `car_get_*` API defined in `src/car.h`.

---

## Firmware Phases

| Phase | Output | Status |
|-------|--------|--------|
| 1 | JSON (NDJSON over USB CDC) — signal verification | ✅ Implemented, host-tested |
| 2 | Raise VW PQ UART — full ATOTO integration | ✅ Implemented, pending car verification |

---

## Spec-Driven Development

This project uses **[OpenSpec](https://github.com/Fission-AI/OpenSpec)** — all changes start with a proposal, specs, and design before implementation.

| Command | What it does |
|---------|-------------|
| `/opsx:propose` | Draft a new change with all artifacts in one step |
| `/opsx:ff` | Fast-forward through artifact creation |
| `/opsx:apply` | Implement tasks from an approved change |
| `/opsx:verify` | Verify implementation matches artifacts |
| `/opsx:archive` | Archive a completed change |
| `/opsx:explore` | Think through problems before implementing |
| `/opsx:sync` | Sync delta specs to main specs |

Specs live in [`openspec/specs/`](./openspec/specs/). Active changes are in [`openspec/changes/`](./openspec/changes/).

See **[`docs/development-guide.md`](./docs/development-guide.md)** for the full workflow.

---

## Repository Structure

```
.
├── src/
│   ├── main.c                        # Application entry, main loop
│   ├── car.h                         # Public car_get_* API
│   ├── cars/
│   │   └── ford_focus_mk3_2015.c    # MS-CAN decoder (only raw CAN access)
│   ├── hal/
│   │   ├── stm32f1/                 # STM32F103 HAL (libopencm3)
│   │   └── host/                    # x86 HAL (SocketCAN, PTY, clock_mono)
│   └── output/
│       ├── json_log.c               # NDJSON over USB CDC
│       └── raise.c                  # Raise VW PQ UART protocol
├── tests/                           # CTest unit tests (host target)
│   └── mocks/                       # HAL mock implementations
├── cmake/
│   ├── toolchain-arm.cmake          # arm-none-eabi-gcc, Cortex-M3
│   └── toolchain-host.cmake         # gcc x86
├── lib/libopencm3/                  # Git submodule
├── docs/
│   ├── development-guide.md         # OpenSpec workflow guide
│   ├── hardware-setup.md            # Wiring diagram, first power-on
│   └── dev-workflow.md              # Build, test, replay, flash
├── tools/
│   ├── replay.sh                    # CAN log replay convenience wrapper
│   └── captures/                    # candump .log files (gitignored)
└── openspec/
    ├── specs/                       # Authoritative specifications
    └── changes/                     # Active and archived changes
```

---

## License

TBD — likely MIT.
