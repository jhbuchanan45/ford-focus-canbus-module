## Why

The firmware has no test coverage. All signal decode logic is currently stubbed and will be filled in after the FORScan capture session; when that happens there will be no automated way to verify correctness, catch regressions, or validate the Raise packet byte layout before connecting to the ATOTO head unit. A testing infrastructure added now means every decode stub can be verified immediately as it is implemented.

## What Changes

- Add a `tests/` directory with a CMake-integrated CTest suite runnable on the host target (no hardware required)
- Add unit tests for the SWC event ring buffer (already implemented, currently untested)
- Add unit tests for all Raise protocol packet encoders — byte layout, checksum, endianness
- Add unit tests for all JSON log formatters — valid NDJSON, correct field names and scaling
- Add a test scaffold for signal decode table tests (one test file per CAN message ID; bodies are TODO stubs that fill in after FORScan)
- Add HAL mock stubs (`uart_hal_tx` capture buffer, `usb_hal_write` capture buffer) so output drivers can be tested without hardware
- Add a golden-file regression test runner that replays a candump log through `canmod-host` and diffs JSON output against a saved baseline (activated once `forscan_baseline.log` exists)
- Add a GitHub Actions CI workflow that builds the host target and runs the CTest suite on every push

## Capabilities

### New Capabilities

- `unit-testing`: CTest-based unit test suite covering the SWC queue, Raise packet encoding, JSON formatting, and signal decode scaffolding; runnable with `ctest --test-dir build/host` with no hardware
- `hal-mocks`: Minimal HAL mock implementations (`uart_hal_tx` → byte capture buffer, `usb_hal_write` → string buffer) that let output driver tests run in isolation
- `regression-testing`: Golden-file replay test — pipes a candump log through `canmod-host` and compares JSON output line-by-line against a saved baseline file

### Modified Capabilities

- `firmware`: Build system gains a `tests/` subdirectory and CTest integration; `CANMOD_TARGET=host` build is the test execution environment
- `pc-dev-emulation`: `canmod-host` binary is now also the unit-under-test for integration/regression tests; replay.sh extended with a `--assert` mode that fails if output differs from golden file

## Impact

- `CMakeLists.txt` — adds `enable_testing()`, `add_subdirectory(tests)`
- `tests/` — new directory: CMakeLists.txt, mock HAL sources, and test .c files
- `tools/replay.sh` — extended with optional `--golden <file>` diff assertion
- `.github/workflows/ci.yml` — new CI pipeline: cmake configure → build → ctest
- No changes to `src/` except that the HAL headers must remain mockable (they already are — no static inlines that would prevent substitution)

## Non-goals

- On-target (STM32 hardware) test execution — all tests run on the host build only
- Code coverage tooling (gcov/lcov) — worthwhile but deferred
- Test generation from CAN DBC files — deferred until DBC format is confirmed post-FORScan
- Testing the STM32 HAL peripheral drivers (bxCAN, USART, USB CDC) — hardware-in-the-loop only
