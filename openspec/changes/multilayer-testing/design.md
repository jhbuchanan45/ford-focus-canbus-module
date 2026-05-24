## Context

The firmware uses a clean layered architecture — HAL → car module → output drivers — where the car module and output drivers have no MCU-specific dependencies. The `CANMOD_TARGET=host` CMake target already compiles all of that logic into a native Linux binary. This means a full unit test suite can run on the host with zero hardware, using the existing build system as the substrate.

The only obstacle is that the output drivers call real HAL functions (`uart_hal_tx`, `usb_hal_write`) which are either PTY writes or stdout in the host HAL. Those need to be swapped for capture-buffer mocks when running under test.

There are no existing tests, no test framework, and no CI. The build produces two executables today; after this change it will produce those plus a `canmod-tests` test runner.

## Goals / Non-Goals

**Goals:**
- CTest suite runnable with `ctest --test-dir build/host` — no hardware, no vcan0, no root
- HAL mock layer that captures uart/usb bytes so output driver tests can assert byte sequences
- Raise packet encoder tests: every packet type verified for byte layout, checksum, and endianness
- JSON log formatter tests: every message ID verified for valid NDJSON and correct signal scaling
- SWC ring buffer tests: queue full, drop-oldest, wrap-around
- Signal decode scaffolding: one test file per CAN message ID, bodies are `TODO` stubs that become real tests after FORScan
- Golden-file regression runner: replay a candump log and diff against saved baseline
- GitHub Actions CI: build host target + run ctest on every push

**Non-Goals:**
- On-target hardware test execution
- Code coverage (gcov/lcov) — deferred
- DBC-driven test generation — deferred post-FORScan
- STM32 peripheral driver testing

## Decisions

### D1 — No external test framework: use plain C with a minimal assert macro

**Decision:** Write tests as standalone C files with a simple `TEST_ASSERT(cond)` macro. No Unity, no CMocka, no Ceedling.

**Rationale:** The functions under test are pure C with simple inputs and outputs. An external framework adds a dependency, a learning curve, and build complexity for negligible gain at this scale. A `PASS`/`FAIL` macro printing the file/line on failure is sufficient and keeps the test files readable by anyone who knows C.

**Alternative considered:** Unity (ThrowTheSwitch) — popular for embedded C, but requires a separate fetch/submodule and a test runner generator. Deferred to a future change if the suite grows large enough to justify it.

---

### D2 — Mock HAL via a dedicated `tests/mocks/` source directory, not function pointers

**Decision:** Provide `tests/mocks/uart_hal.c` and `tests/mocks/usb_hal.c` that replace the real host HAL files in the test build. Each mock maintains an internal capture buffer. Tests call `mock_uart_get_buf()` / `mock_usb_get_buf()` to inspect what was transmitted.

**Rationale:** The HAL API (`uart_hal_tx`, `usb_hal_write`) is already a clean C interface — no function pointers or vtables needed. Swapping the `.c` file in CMake is the simplest approach and requires zero changes to production code. The `src/hal/can_hal.h` contract is not needed for output driver tests (tests call output functions directly with pre-set car state) so `can_hal.c` is not mocked.

**Alternative considered:** Wrapping with `--wrap` linker flag (GNU ld) — works but is toolchain-specific and harder to understand at a glance.

---

### D3 — Test build is a separate CMake target `canmod-tests`, not a separate CMake project

**Decision:** Add `enable_testing()` and `add_subdirectory(tests)` to the root `CMakeLists.txt` when `CANMOD_TARGET=host`. The `tests/CMakeLists.txt` defines a single `canmod-tests` executable and registers it with `add_test()`.

**Rationale:** Keeps a single configure step. `cmake -DCANMOD_TARGET=host ... && cmake --build build/host && ctest --test-dir build/host` is the full workflow. A separate CMake project would require two configure/build steps.

---

### D4 — Signal decode tests use a two-phase pattern: fixture → assert state

**Decision:** Each decode test: (1) call `car_process_frame(id, data, dlc)` with a crafted byte array, then (2) assert the corresponding `car_get_*()` return value.

**Rationale:** `car_process_frame` is the public entry point. Testing through it exercises the full dispatch + decode path and will catch a wrong `switch` case as well as a wrong bit extraction. Internal `decode_*` functions are `static` and not directly callable from tests — this is by design to enforce the API boundary.

---

### D5 — Golden-file regression test is opt-in, gated on the capture file existing

**Decision:** The regression test in `tests/test_regression.c` checks for the existence of `tools/captures/forscan_baseline.log` and `tools/captures/forscan_baseline_golden.ndjson` at runtime. If either is absent, the test is skipped (`SKIP`) rather than failed.

**Rationale:** The capture files don't exist yet (task 1.1). Making their absence a test failure would break CI from day one. Once the files are committed, the test activates automatically with no CMake changes needed.

---

### D6 — CI runs only the host/json build and test suite; ARM build is not CI-gated

**Decision:** GitHub Actions runs `cmake -DCANMOD_TARGET=host -DCANMOD_OUTPUT=json ... && cmake --build ... && ctest`. The ARM cross-compile is not run in CI (requires `arm-none-eabi-gcc` and libopencm3 build, which is slow and the output cannot be executed).

**Rationale:** The ARM HAL is hardware-verified separately (tasks 4.x, 9.x). Cross-compiling in CI would catch syntax errors but not logical ones. The host build covers 100% of the testable logic. ARM compile can be added later as a non-blocking check.

## Risks / Trade-offs

**[Risk] Static decode functions not directly testable** → Mitigated by D4 — all testing goes through `car_process_frame()`. If the `switch` dispatch is wrong, a test will still fail.

**[Risk] Mocked HAL diverges from real HAL behaviour** → Mitigated by keeping the mock as thin as possible (capture buffer only, no logic). The real HAL files are tested on hardware (tasks 4.x).

**[Risk] Golden-file test is brittle to timestamp differences** → Mitigated by stripping `"ts":` fields before diffing, comparing only `"id"` and `"signals"` content.

**[Risk] Decode stubs mean most signal tests are TODO until FORScan** → Acceptable. The scaffold means tests can be written in the same commit as the decode implementation, keeping the TODO count visible in CI output.

## Open Questions

- **OQ1:** Should the test binary be split per-module (one executable per test file) or a single combined runner? Single runner chosen for now (D3); revisit if compile times become an issue.
- **OQ2:** Once FORScan confirms IDs, should the golden-file baseline be auto-generated on first run or manually curated? Lean toward manual curation so a signal regression is always a deliberate diff.
