## Why

Replacing the OEM head unit in a 2015 Ford Focus Titanium with an ATOTO S8 MS Android head unit requires a CAN bus decoder that reads vehicle signals from the MS-CAN bus (available at the quad-lock harness behind the head unit) and outputs them in the Raise VW PQ UART protocol, which the ATOTO S8 MS accepts natively. No off-the-shelf decoder (e.g. Raise RZ-FD60) exposes its firmware for modification or extension; building a custom decoder gives full control over signal mapping, output fidelity, and future expansion.

## What Changes

- **New**: STM32 firmware that reads Ford Focus Mk3 MS-CAN frames from the quad-lock harness at 125 kbps and extracts all vehicle signals needed by the head unit
- **New**: Raise VW PQ UART output driver that encodes decoded signals into the Raise protocol (38400 baud, 8N1) for consumption by the ATOTO S8 MS
- **New**: Phase 1 JSON log output driver for development — logs decoded signals over USB CDC before the Raise driver is complete
- **New**: PC-side development loop using SocketCAN (vcan0 on WSL2) to replay captured CAN frames against the firmware compiled for x86
- **Modified**: Firmware architecture spec updated to reflect the actual layer boundaries, HAL target (STM32), and the car\_get\_\* API contract between car module and protocol encoder
- **Modified**: CAN bus spec updated to reflect that only MS-CAN is used (quad-lock, not OBD-II) and to pin confirmed signal IDs for all required outputs

## Capabilities

### New Capabilities

- `ms-can-input`: MS-CAN HAL driver and Ford Focus Mk3 signal decoder — reads raw CAN frames at 125 kbps via a TJA1042 transceiver and maps Ford-specific message IDs to the internal `car_get_*` API. Covers all signals required for Raise output: SWC buttons, vehicle speed, RPM, coolant temp, voltage, odometer, doors, reverse gear, park brake, illumination, AC/climate state, front/rear PDC distances, and steering angle.
- `raise-output`: Raise VW PQ UART output driver — encodes internal signal state as Raise protocol packets (0x2E header, 38400 baud) and transmits them to the ATOTO S8 MS on the head unit UART line. Covers all packet types: SWC (0x20), AC (0x21), rear radar (0x22), front radar (0x23), status flags (0x24), park active (0x25), steering angle (0x26), door status (0x41/0x01), vehicle data (0x41/0x02), and warnings (0x41/0x03).
- `json-log-output`: Development output driver — emits decoded signal state as JSON lines over USB CDC at 115200 baud. Runs in place of the Raise driver during Phase 1. Enables signal verification against FORScan captures without needing a live head unit.
- `pc-dev-emulation`: Host build target — compiles the firmware (minus MCU HAL) for x86 Linux/WSL2 with a SocketCAN backend replacing the CAN peripheral. Enables full signal decode and UART output testing using `cangen`/`canplayer` against `vcan0` without hardware.

### Modified Capabilities

- `firmware`: Architecture spec updated — HAL target confirmed as STM32 (bxCAN or FDCAN variant TBD), build system CMake (multi-target: ARM + x86 host), and `car_get_*` API defined as the contract between car module and protocol encoder layer.
- `canbus`: CAN bus spec updated — scope narrowed to MS-CAN only (quad-lock harness, 125 kbps); signal table extended with all IDs and bit layouts required for Raise output; unverified IDs section updated with research findings.

## Impact

- **Layers affected**: All three — HAL (CAN peripheral + UART driver), Decoder (Ford MS-CAN signal mapping), Application (main loop, output driver selection)
- **Bus**: MS-CAN only at 125 kbps. HS-CAN is out of scope for this change.
- **Hardware**: Inline quad-lock harness splice (not OBD-II). Power from quad-lock +12V ACC. CAN transceiver: TJA1042 (3.3V compatible).
- **Reference implementation**: [smartgauges/canbox](https://github.com/smartgauges/canbox) — the Raise UART encoder (`canbox.c`) and `car_get_*` API (`car.h`) are the primary references. The Ford car module (`ford_focus_mk3_2015.c`) is new work; the encoder is adapted directly.
- **External dependency**: ATOTO S8 MS must be configured to "Raise" protocol in its CAN box settings. No firmware changes to the head unit.

## Non-Goals

- HS-CAN decoding (powertrain bus) — not present at quad-lock, out of scope
- HiWorld or any other head unit protocol — Raise VW PQ only
- Writing/transmitting CAN frames to the vehicle bus
- OBD-II connection or OBD-II PID parsing
- Custom PCB design — breadboard/dev board for this change; PCB is a future change
- Signal decoding for vehicles other than 2015 Ford Focus Mk3 Titanium
- Wireless output (BLE, WiFi) — USB CDC and UART only
