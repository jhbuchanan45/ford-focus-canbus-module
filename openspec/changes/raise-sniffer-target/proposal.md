## Why

Before writing the custom CAN decoder firmware, it is valuable to observe what the existing off-the-shelf FD60 Raise decoder box actually transmits. The FD60 sits on the same MS-CAN bus and speaks the same Raise VW PQ UART protocol to the ATOTO head unit — its output is ground truth for what a correctly functioning decoder should produce. Capturing it allows direct comparison against the custom firmware's output, validates that the Raise protocol implementation is correct, and surfaces any signal mappings the community documentation may have missed.

Currently the codebase only transmits Raise packets; it has no way to receive and log them from an external source. A second Blue Pill flashed with a dedicated sniffer firmware can emulate the ATOTO head unit, accept all packets from the FD60, keep the FD60 alive with the required 0xFF ACKs, and log everything over USB CDC to a PC for analysis.

## What Changes

- **New**: `src/sniffer_main.c` — a minimal main loop for the sniffer target: no CAN bus, no car state, just UART RX polling and USB CDC output
- **New**: `src/output/raise_sniffer.c` / `.h` — Raise VW PQ packet receiver and logger: parses incoming 0x2E-framed packets, sends 0xFF ACKs, dumps every packet (valid and invalid) to USB CDC in a human-readable hex format
- **Modified**: `CMakeLists.txt` — new `CANMOD_OUTPUT=sniffer` option and `canmod-sniffer` build target, STM32 only

## Capabilities

### New Capabilities

- `raise-sniffer`: Second Blue Pill firmware that emulates the ATOTO head unit for passive discovery. Connects to the FD60's UART TX/RX lines, ACKs every valid Raise packet, and logs all received packets to USB CDC. Enables comparison of FD60 output against the custom decoder's output.

## Impact

- **No changes to existing firmware** — the sniffer is a completely separate CMake target (`canmod-sniffer`). Existing `canmod` (Blue Pill 1) firmware is unaffected.
- **Reuses all existing HAL** — `uart_hal`, `usb_hal`, `gpio_hal`, `timer_hal` are unchanged. No new HAL code required.
- **STM32-only target** — no host build. The sniffer only makes sense connected to real FD60 hardware; there is nothing to sniff on a PC.
- **Hardware required**: a second Blue Pill, wired to the FD60 UART output (PA3 RX from FD60 TX, PA2 TX to FD60 RX for ACKs).

## Non-Goals

- Decoding or interpreting packet payloads — logs raw bytes only; analysis is done on the PC
- Host (x86) build of the sniffer
- Modifying or injecting packets toward the FD60
- Any changes to the existing `canmod` (Blue Pill 1) firmware
- Replacing the ATOTO in a production wiring setup — sniffer is a development/discovery tool only
