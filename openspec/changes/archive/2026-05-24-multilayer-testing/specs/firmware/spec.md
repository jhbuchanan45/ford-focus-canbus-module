## MODIFIED Requirements

### Requirement: Build system supports ARM and x86 host targets via CMake
The project SHALL use CMake with two toolchain files: `cmake/toolchain-arm.cmake` (arm-none-eabi-gcc, Cortex-M3) and `cmake/toolchain-host.cmake` (gcc for x86). Output driver is selected via `-DCANMOD_OUTPUT=json|raise`. Target is selected via `-DCANMOD_TARGET=stm32f1|host`.

When `CANMOD_TARGET=host`, the build SHALL additionally produce a `canmod-tests` executable registered with CTest. Running `cmake --build build/host && ctest --test-dir build/host` SHALL build and execute the full unit test suite.

#### Scenario: ARM firmware build
- **WHEN** `cmake -DCANMOD_TARGET=stm32f1 -DCANMOD_OUTPUT=raise -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-arm.cmake ..`
- **THEN** an `.elf` and `.bin` are produced targeting STM32F103 with Raise UART output

#### Scenario: Host binary build
- **WHEN** `cmake -DCANMOD_TARGET=host -DCANMOD_OUTPUT=json -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-host.cmake ..`
- **THEN** a `canmod-host` x86 executable is produced with JSON log output and SocketCAN HAL

#### Scenario: Host test build
- **WHEN** `cmake -DCANMOD_TARGET=host -DCANMOD_OUTPUT=json -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-host.cmake .. && cmake --build . && ctest`
- **THEN** a `canmod-tests` executable is produced and CTest reports all registered tests
