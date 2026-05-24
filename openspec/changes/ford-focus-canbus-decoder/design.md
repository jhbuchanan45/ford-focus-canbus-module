## Context

The ATOTO S8 MS Android head unit replaces the OEM Ford Focus Mk3 head unit. To retain vehicle integration (steering wheel controls, AC display, parking sensors, reverse camera trigger, door/speed info), the head unit expects a CAN bus decoder connected via UART speaking the **Raise VW PQ protocol** (38400 baud, 8N1).

The quad-lock connector behind the OEM head unit carries MS-CAN (125 kbps) on pins A9 (H) and A10 (L), along with +12V ACC and GND. This is the only CAN bus accessible without tapping deeper into the wiring loom. All signals the ATOTO needs are either natively on MS-CAN or mirrored there by the GEM gateway (e.g. vehicle speed is mirrored from HS-CAN).

An existing open source project — [smartgauges/canbox](https://github.com/smartgauges/canbox) — implements the Raise VW PQ protocol encoder for STM32F103 + libopencm3 with a clean `car_get_*` API separating vehicle-specific CAN decoding from the protocol encoder. It also has a QEMU-based emulation setup. This project is the primary reference.

## Goals / Non-Goals

**Goals:**
- Decode all MS-CAN signals needed for full Raise VW PQ output (SWC, speed, RPM, temp, voltage, odometer, doors, reverse, park brake, illumination, AC/climate, front/rear PDC, steering angle)
- Output Raise VW PQ UART packets to the ATOTO S8 MS head unit
- Phase 1: JSON log output over USB CDC for signal verification before head unit integration
- PC-side development loop using SocketCAN on WSL2 (vcan0) against the same firmware compiled for x86

**Non-Goals:**
- HS-CAN decoding (not at quad-lock)
- HiWorld or any other head unit protocol
- Transmitting CAN frames to the vehicle
- Custom PCB design (breadboard / dev board for this change)
- Wireless output

## Decisions

### D1: Adapt smartgauges/canbox rather than build from scratch

**Decision:** Use smartgauges/canbox's Raise protocol encoder (`canbox.c`) and `car_get_*` API contract as the direct foundation, adding `ford_focus_mk3_2015.c` as a new car module.

**Rationale:** The Raise VW PQ encoder in `canbox.c` is complete, correct, and already running on STM32F103. Building it from scratch would reproduce the same output with no benefit. The `car_get_*` API is a clean, car-agnostic boundary — adding a Ford car module is the only new work required on the decoder side.

**Alternative considered:** Build from scratch to avoid coupling to an external project's architecture. Rejected: the encoder is stable and well-understood from source review; the API contract is simple enough to own independently if needed.

### D2: STM32F103C8T6 (Blue Pill) as the initial MCU target

**Decision:** Target STM32F103C8T6 for the hardware build. Use libopencm3 as the HAL library, matching smartgauges/canbox.

**Rationale:** Only one CAN bus is needed (MS-CAN). The F103's single bxCAN peripheral is sufficient. The Blue Pill is £2–3, widely available, and is the exact target of the reference implementation — the HAL is already written. STM32G0B1 (2× FDCAN) would add capability but zero benefit for this change.

**Alternative considered:** STM32G0B1 for future-proofing (FDCAN, 2 buses). Deferred — if a second CAN bus is ever needed, a new MCU change can address it.

### D3: CMake as the build system with two toolchain targets

**Decision:** CMake with two toolchain files: `toolchain-arm.cmake` (arm-none-eabi-gcc) and `toolchain-host.cmake` (gcc for x86). The host target stubs `src/hal/` with a SocketCAN backend.

**Rationale:** CMake toolchain files are the standard approach for multi-target embedded builds. The same `CMakeLists.txt` produces both the ARM firmware and the x86 host binary, sharing all decoder and protocol encoder code. Makefile alternative was considered but becomes unwieldy for multi-target builds.

### D4: car_get_* API as the inter-layer contract

**Decision:** The Ford Focus MS-CAN decoder (`ford_focus_mk3_2015.c`) maintains module-level state and exposes it exclusively through the `car_get_*` API defined in `car.h`. The Raise encoder calls this API; it never reads CAN frame data directly.

**Rationale:** This is the boundary that makes the PC emulation loop work cleanly — the same API is called whether the data came from a real CAN peripheral or from a replayed SocketCAN trace. It also means the encoder is provably car-agnostic.

### D5: Pluggable output via compile-time selection in Phase 1/2

**Decision:** The output driver (JSON log vs. Raise UART) is selected at compile time via a `CANMOD_OUTPUT` CMake variable (`json` or `raise`). Both drivers implement the same `canmod_output_driver_t` interface.

**Rationale:** Runtime switching adds complexity with no benefit — you're either in dev mode (JSON) or deployment mode (Raise), not both. Compile-time selection keeps the binary size small and the code paths simple. A future change can add runtime selection if needed.

### D6: MS-CAN signal IDs require FORScan verification before implementation

**Decision:** The decoder module (`ford_focus_mk3_2015.c`) SHALL NOT be written until signal IDs are verified against the real car using FORScan. The specs in this change mark all IDs as "community research — verify with FORScan" where not confirmed.

**Rationale:** Writing the decoder against unverified IDs means the first debug session on the car produces nothing useful. A 30-minute FORScan session before writing code saves days of hardware debugging. This is the critical path.

## Risks / Trade-offs

**[Risk] MS-CAN signal IDs differ from community research values** → Run FORScan scan on the car before implementing the decoder. Log the complete MS-CAN frame dump with `candump` for 5 minutes of driving. Use this as the verified signal corpus.

**[Risk] Bus termination is disrupted by inline splice** → The MS-CAN bus requires 120Ω termination at each end. The OEM head unit module may contain a termination resistor. If the head unit is removed and the canbox takes its place, the termination may be broken. Measure the resistance across MS-CAN H/L with head unit removed; if it reads ~120Ω (not ~60Ω), add a 120Ω resistor to the canbox board.

**[Risk] SWC button encoding differs between Focus and Raise expected values** → The mapping between Ford MS-CAN SWC message bytes and Raise button IDs (0x01=Vol+, 0x02=Vol-, etc.) requires live testing with the ATOTO. Start with Volume Up/Down/Next/Prev — the most critical — and iterate.

**[Risk] PDC distances not present at quad-lock** → The Titanium's parking aid module may send distances on a sub-bus segment not accessible at the quad-lock. Verify with FORScan first. If absent, the radar packets are omitted from Phase 2 with no other impact.

**[Risk] STM32F103 flash (64KB) may be tight with full AC+PDC support** → The F103C8 has 64KB flash. The F103CB has 128KB at the same price. Monitor binary size during development; switch to CB variant if needed.

## Migration Plan

This is new firmware; there is no existing deployment to migrate. The ATOTO S8 MS operates without the canbox until the device is plugged in — removing or inserting the canbox is non-destructive and hot-pluggable from the head unit's perspective.

Rollback: unplug the canbox from the UART port on the ATOTO harness. The head unit continues operating without vehicle integration.

## Open Questions

1. **ATOTO S8 MS protocol setting name for Raise**: Needs confirmation — is the setting "Raise", "Raise Ford", "Raise VW PQ", or something else? Check ATOTO settings → CAN box → manufacturer list.
2. **FORScan ID verification**: Which specific MS-CAN IDs carry SWC bytes, PDC distances, AC state, and steering angle on the 2015 Focus Titanium? (Spike required before decoder implementation.)
3. **Bus termination**: Does the OEM head unit contain a 120Ω termination resistor on MS-CAN? (Measure with multimeter before first power-on.)
4. **STM32F103 variant**: C8 (64KB) or CB (128KB)? Defer until binary size is measured after initial implementation.
