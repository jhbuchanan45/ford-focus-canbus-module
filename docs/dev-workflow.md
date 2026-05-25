# Developer Workflow Guide

End-to-end guide for building, testing, and replaying CAN captures on a PC — no hardware required for the development loop.

---

## Prerequisites

### Linux / WSL2 (Windows)

```bash
# Build tools
sudo apt install build-essential cmake gcc git

# CAN utilities
sudo apt install can-utils   # provides cansend, candump, canplayer

# ARM cross-compiler (for STM32 firmware)
sudo apt install gcc-arm-none-eabi binutils-arm-none-eabi

# Optional: ST-Link flasher
sudo apt install stlink-tools
```

### WSL2 — Enable vcan (Virtual CAN)

WSL2 does not load `vcan` automatically. You need a custom kernel or use the module approach:

```bash
# Check if vcan module is available
modinfo vcan 2>/dev/null && echo "vcan available" || echo "vcan not available"

# If available:
sudo modprobe vcan

# If not (WSL2 default kernel):
# Build a custom WSL2 kernel with CONFIG_CAN_VCAN=m, or use a pre-built kernel:
# https://github.com/microsoft/WSL2-Linux-Kernel
# Then add to %USERPROFILE%\.wslconfig:
# [wsl2]
# kernel=C:\\path\\to\\bzImage
```

Once the module is available, create the virtual interface (also done automatically by `tools/replay.sh`):

```bash
sudo ip link add dev vcan0 type vcan
sudo ip link set up vcan0
ip link show vcan0   # should show UP
```

---

## Repository Structure

```
.
├── src/
│   ├── main.c                        # Application entry, main loop
│   ├── car.h                         # Public car_get_* API (no CAN types)
│   ├── cars/
│   │   └── ford_focus_mk3_2015.c    # MS-CAN decoder (only file touching raw frames)
│   ├── hal/
│   │   ├── stm32f1/                 # STM32F103 HAL (libopencm3)
│   │   └── host/                    # x86 HAL (SocketCAN + PTY)
│   └── output/
│       ├── json_log.c               # NDJSON over USB CDC (Phase 1)
│       └── raise.c                  # Raise VW PQ UART (Phase 2)
├── tests/                           # CTest unit tests (host target only)
├── cmake/
│   ├── toolchain-arm.cmake          # arm-none-eabi-gcc, Cortex-M3
│   └── toolchain-host.cmake         # gcc x86
├── lib/libopencm3/                  # Git submodule (ARM target only)
├── docs/                            # This guide + hardware-setup.md
└── tools/
    ├── replay.sh                    # Replay a candump log against canmod-host
    └── captures/                    # candump .log files go here
```

---

## Building

### Host target (x86) — development / testing

```bash
mkdir -p build/host && cd build/host

# JSON log output (Phase 1 — signal verification)
cmake -DCANMOD_TARGET=host \
      -DCANMOD_OUTPUT=json \
      -DCMAKE_TOOLCHAIN_FILE=../../cmake/toolchain-host.cmake \
      ../..
cmake --build . -j$(nproc)
# Produces: canmod-host

# Raise output (Phase 2 — head unit protocol)
cmake -DCANMOD_TARGET=host \
      -DCANMOD_OUTPUT=raise \
      -DCMAKE_TOOLCHAIN_FILE=../../cmake/toolchain-host.cmake \
      ../..
cmake --build . -j$(nproc)
```

### ARM target (STM32F103)

```bash
# Initialise libopencm3 submodule first (one-time)
git submodule update --init --recursive

mkdir -p build/arm && cd build/arm

cmake -DCANMOD_TARGET=stm32f1 \
      -DCANMOD_OUTPUT=raise \
      -DCMAKE_TOOLCHAIN_FILE=../../cmake/toolchain-arm.cmake \
      ../..
cmake --build . -j$(nproc)
# Produces: canmod-stm32f1.elf  canmod-stm32f1.bin
```

---

## Running the Unit Tests

Tests run on the host target only (no hardware needed):

```bash
cd build/host
ctest --output-on-failure
# Expected: 20/20 tests passing
```

Run a single test verbosely:

```bash
./test_decode_hvac      # or any test executable
```

---

## Replaying a CAN Capture

### Quick replay

```bash
# Ensure vcan0 is up
sudo ip link add dev vcan0 type vcan 2>/dev/null; sudo ip link set up vcan0

# Run canmod-host in one terminal (JSON output)
./build/host/canmod-host --interface vcan0

# Replay the capture in another terminal
canplayer -I tools/captures/forscan_baseline.log vcan0=vcan0
```

### Using replay.sh (convenience wrapper)

```bash
# Basic replay — streams JSON to stdout
tools/replay.sh tools/captures/forscan_baseline.log

# Golden-file comparison (strips ts field before diff)
tools/replay.sh --golden tools/captures/forscan_baseline.log \
                          tools/captures/forscan_baseline_golden.ndjson
```

### Capturing a new log (on car with USB-CAN adapter)

```bash
# On WSL2 with a USB-CAN adapter (e.g. CANtact, Kvaser Leaf)
sudo ip link set can0 type can bitrate 125000
sudo ip link set up can0
candump -l can0   # writes to candump-YYYY-MM-DD_HHMMSS.log
# Copy log to tools/captures/ and rename descriptively
```

---

## Monitoring Raise Output (Phase 2)

When running with `CANMOD_OUTPUT=raise`, the host HAL creates a PTY and prints its path at startup:

```
[uart_hal] Raise output PTY: /dev/pts/3
  Monitor: cat /dev/pts/3 | xxd
```

Watch the raw bytes:

```bash
cat /dev/pts/3 | xxd
# Example output (vehicle info packet every 500 ms):
# 2e 41 0d 02 00 00 00 00 00 00 4a 00 00 00 00 08  .A......J.......
```

Decode packets manually: `[0x2E] [CMD] [LEN] [DATA×LEN] [CSUM]`

---

## Flashing the STM32

### With ST-Link (recommended)

```bash
# Flash the .bin
st-flash write build/arm/canmod-stm32f1.bin 0x8000000

# Verify flash
st-flash verify build/arm/canmod-stm32f1.bin 0x8000000
```

### With OpenOCD

```bash
openocd -f interface/stlink.cfg \
        -f target/stm32f1x.cfg \
        -c "program build/arm/canmod-stm32f1.elf verify reset exit"
```

### DFU (USB bootloader — no ST-Link needed)

1. Hold BOOT0 high, press RESET
2. `dfu-util -a 0 -s 0x08000000:leave -D build/arm/canmod-stm32f1.bin`

---

## Common Issues

| Problem | Fix |
|---------|-----|
| `vcan: module not found` | WSL2 needs custom kernel — see §WSL2 Enable vcan |
| `canmod-host: can't open vcan0` | Run `sudo ip link set up vcan0` |
| Build fails: `arm-none-eabi-gcc not found` | `sudo apt install gcc-arm-none-eabi` |
| `libopencm3` missing headers | `git submodule update --init --recursive` |
| `ctest` reports 0 tests | Build with `CANMOD_TARGET=host`; ARM target has no tests |
| JSON output garbled | Check USB CDC serial port is open before ignition-on |
