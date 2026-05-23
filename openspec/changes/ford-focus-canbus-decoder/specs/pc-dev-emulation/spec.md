## ADDED Requirements

### Requirement: Firmware compiles for x86 host with a SocketCAN HAL stub
The CMake build system SHALL support a host target (`-DCANMOD_TARGET=host`) that compiles all firmware code except `src/hal/` for x86 Linux. The host HAL stub (`src/hal/host/`) SHALL replace the STM32 CAN peripheral with a SocketCAN interface (Linux `PF_CAN` socket on a named interface, e.g. `vcan0`), and replace the STM32 UART with stdout.

#### Scenario: Host build succeeds
- **WHEN** `cmake -DCANMOD_TARGET=host .. && make` is run on an x86 Linux or WSL2 host
- **THEN** a `canmod-host` executable is produced with no MCU-specific dependencies

#### Scenario: Host build reads from vcan0
- **WHEN** `./canmod-host --interface vcan0` is executed and `vcan0` is up
- **THEN** the host binary receives CAN frames from the vcan0 interface, decodes them through the car module, and routes them to the configured output driver

---

### Requirement: vcan0 virtual CAN interface is used for development
The development workflow SHALL use the Linux `vcan` kernel module to create a virtual CAN interface. Frames can be injected using `cangen`, `cansend`, or `canplayer` from `can-utils`.

#### Scenario: Virtual interface created and frame injected
- **WHEN** the following commands are run:
  ```
  sudo modprobe vcan
  sudo ip link add dev vcan0 type vcan
  sudo ip link set up vcan0
  cansend vcan0 3B5#0300002B00000000
  ```
- **THEN** the `canmod-host` process receives the frame and decodes it as a HVAC_STATUS message

---

### Requirement: Real CAN captures can be replayed for regression testing
Captured CAN log files (`.log` format from `candump` or `.asc` from other tools) SHALL be replayable against the host build using `canplayer`. This allows full signal decode regression testing without the vehicle.

#### Scenario: Log file replayed
- **WHEN** `canplayer -I capture.log vcan0` is run while `canmod-host` is running
- **THEN** the host binary processes all frames from the log file and produces JSON output equivalent to what would have been produced on real hardware

---

### Requirement: WSL2 is a supported development environment
The build system and dev loop SHALL work on WSL2 (Windows Subsystem for Linux 2) with no Windows-specific tooling required. All development tools (arm-none-eabi-gcc, CMake, can-utils, Python) are installable via `apt`.

#### Scenario: Full dev loop on WSL2
- **WHEN** a developer installs the required packages on WSL2 Ubuntu and clones the repo
- **THEN** they can build the host target, create vcan0, replay a log file, and observe decoded JSON output — all without physical hardware

---

### Requirement: A canplayer-compatible log capture procedure is documented
The project SHALL document the one-time procedure for capturing a real CAN log from the vehicle using a USB CAN adapter (e.g. CANable, £15) plugged into the quad-lock harness, producing a `.log` file that can be used as a permanent test corpus.

#### Scenario: Capture procedure followed
- **WHEN** a CANable adapter is connected to quad-lock pins A9/A10 and `candump -l vcan0` is run
- **THEN** a timestamped `.log` file is produced that can be replayed indefinitely for offline development
