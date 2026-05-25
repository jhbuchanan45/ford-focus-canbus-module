# HAL Mocks Spec

## Purpose

Mock implementations of all HAL interfaces, used by the CTest unit test suite. Mocks replace production HAL `.c` files at link time via CMake; no production code is modified.

## Requirements

### Requirement: UART mock captures transmitted bytes in a buffer
A mock implementation of `uart_hal.c` (`tests/mocks/uart_hal.c`) SHALL maintain an internal byte capture buffer. Tests SHALL be able to reset the buffer, inspect its contents, and query its length without modifying production code.

#### Scenario: Bytes written via uart_hal_tx are captured
- **WHEN** `uart_hal_tx(0x2E)` and `uart_hal_tx(0x41)` are called in a test
- **THEN** `mock_uart_get_buf()` returns a pointer to `{0x2E, 0x41}` and `mock_uart_get_len()` returns 2

#### Scenario: Buffer resets between tests
- **WHEN** `mock_uart_reset()` is called at the start of a test
- **THEN** `mock_uart_get_len()` returns 0 and any previous bytes are discarded

#### Scenario: Buffer overflow is handled without crash
- **WHEN** more bytes are written than the buffer capacity (256 bytes)
- **THEN** excess bytes are silently dropped and the captured length is capped at 256

---

### Requirement: USB CDC mock captures output as a null-terminated string
A mock implementation of `usb_hal.c` (`tests/mocks/usb_hal.c`) SHALL accumulate all bytes written via `usb_hal_write()` into an internal string buffer, enabling JSON output to be inspected as a C string.

#### Scenario: JSON line written via usb_hal_write is captured
- **WHEN** `usb_hal_write()` is called with a JSON line
- **THEN** `mock_usb_get_buf()` returns the accumulated string including the newline character

#### Scenario: Buffer is reset between tests
- **WHEN** `mock_usb_reset()` is called
- **THEN** `mock_usb_get_buf()` returns an empty string and `mock_usb_get_len()` returns 0

---

### Requirement: Car state can be set directly from tests without a CAN frame
The test scaffold SHALL provide a `test_helpers.h` header with inline setters that write directly into the car module's internal state (via a `car_test_set_*` test-only API compiled in under `CANMOD_TESTING=1`). This allows output driver tests to set up known car state without needing a real or mocked CAN peripheral.

#### Scenario: Output driver test sets known speed and verifies packet
- **WHEN** a test sets speed to 5000 (50.00 km/h) and calls `raise_send_vehicle_info()`
- **THEN** the UART mock contains the correctly encoded speed bytes without any CAN frame being processed
