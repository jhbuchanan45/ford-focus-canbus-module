## MODIFIED Requirements

### Requirement: Hardware Abstraction Layer wraps all MCU-specific peripheral access
The HAL (`src/hal/`) SHALL wrap all MCU-specific peripheral access. The STM32F103 is the initial hardware target using libopencm3. A host HAL stub (`src/hal/host/`) SHALL provide a SocketCAN backend for x86 builds. Swapping MCU target means only modifying `src/hal/`; all layers above are untouched.

| Module | File | Responsibility |
|---|---|---|
| CAN peripheral | `can_hal.c/.h` | Init bxCAN at 125 kbps, Rx via interrupt, hardware acceptance filters |
| UART (head unit) | `uart_hal.c/.h` | Raise protocol UART at 38400 baud, 8N1 |
| USB CDC | `usb_hal.c/.h` | USB CDC output at 115200 baud for JSON log driver |
| GPIO | `gpio_hal.c/.h` | LED heartbeat, digital outputs |
| Timers | `timer_hal.c/.h` | Periodic tick (1 ms), watchdog |

#### Scenario: HAL swap is isolated
- **WHEN** the MCU target changes (e.g. F103 → G0B1)
- **THEN** only files under `src/hal/` require modification; `src/cars/`, `src/canbox.c`, and output drivers are unchanged

#### Scenario: Host HAL substitutes SocketCAN
- **WHEN** the firmware is built with `CANMOD_TARGET=host`
- **THEN** `src/hal/host/` replaces `src/hal/stm32f1/` and CAN frames are read from a named SocketCAN interface

---

### Requirement: Car module implements the car_get_* API contract
The car module (`src/cars/ford_focus_mk3_2015.c`) SHALL be the only file that reads raw CAN frame data. It SHALL maintain module-level signal state and expose it exclusively through the `car_get_*` API defined in `car.h`. The Raise encoder and output drivers SHALL call only this API.

#### Scenario: API contract enforced at compile time
- **WHEN** any file outside `src/cars/` attempts to access raw CAN frame data directly
- **THEN** compilation fails (CAN frame types are not exposed in public headers)

---

### Requirement: Build system supports ARM and x86 host targets via CMake
The project SHALL use CMake with two toolchain files: `cmake/toolchain-arm.cmake` (arm-none-eabi-gcc, Cortex-M3) and `cmake/toolchain-host.cmake` (gcc for x86). Output driver is selected via `-DCANMOD_OUTPUT=json|raise`. Target is selected via `-DCANMOD_TARGET=stm32f1|host`.

#### Scenario: ARM firmware build
- **WHEN** `cmake -DCANMOD_TARGET=stm32f1 -DCANMOD_OUTPUT=raise -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-arm.cmake ..`
- **THEN** an `.elf` and `.bin` are produced targeting STM32F103 with Raise UART output

#### Scenario: Host binary build
- **WHEN** `cmake -DCANMOD_TARGET=host -DCANMOD_OUTPUT=json -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-host.cmake ..`
- **THEN** a `canmod-host` x86 executable is produced with JSON log output and SocketCAN HAL

## ADDED Requirements

### Requirement: Application layer manages periodic output timers
The application layer (`src/main.c`) SHALL maintain a 1 ms tick counter and call the active output driver's periodic functions at the correct intervals: vehicle data at 500 ms, AC status at 500 ms, door status on change, SWC events immediately, radar packets when active at 100 ms.

#### Scenario: Periodic send fires at correct interval
- **WHEN** 500 ms elapses since last vehicle data packet
- **THEN** `canbox_raise_vw_vehicle_info()` is called and the 0x41/0x02 packet is sent regardless of whether signal values changed
