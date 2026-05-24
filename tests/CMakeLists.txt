cmake_minimum_required(VERSION 3.16)

# ---------------------------------------------------------------------------
# Shared sources linked into every test executable
# ---------------------------------------------------------------------------
set(CAR_SOURCES
    ${CMAKE_SOURCE_DIR}/src/cars/ford_focus_mk3_2015.c
)

set(MOCK_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/mocks/uart_hal.c
    ${CMAKE_CURRENT_SOURCE_DIR}/mocks/usb_hal.c
    ${CMAKE_CURRENT_SOURCE_DIR}/mocks/can_hal.c
    ${CMAKE_CURRENT_SOURCE_DIR}/mocks/gpio_hal.c
    ${CMAKE_CURRENT_SOURCE_DIR}/mocks/timer_hal.c
)

set(COMMON_INCLUDES
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}
)

set(COMMON_DEFS
    CANMOD_TARGET_HOST=1
    CANMOD_TESTING=1
)

# ---------------------------------------------------------------------------
# Helper macro: add a single-file test executable and register with CTest
# ---------------------------------------------------------------------------
macro(add_canmod_test name source)
    add_executable(${name}
        ${source}
        ${CAR_SOURCES}
        ${MOCK_SOURCES}
        ${ARGN}  # optional extra sources (e.g. output driver)
    )
    target_include_directories(${name} PRIVATE ${COMMON_INCLUDES})
    target_compile_definitions(${name} PRIVATE ${COMMON_DEFS})
    target_compile_options(${name} PRIVATE -Wall -Wextra)
    add_test(NAME ${name} COMMAND ${name})
endmacro()

# ---------------------------------------------------------------------------
# 2. SWC queue
# ---------------------------------------------------------------------------
add_canmod_test(test-swc test_swc_queue.c)

# ---------------------------------------------------------------------------
# 3. Raise encoder tests — each needs raise.c + CANMOD_OUTPUT_RAISE
# ---------------------------------------------------------------------------
set(RAISE_SRC ${CMAKE_SOURCE_DIR}/src/output/raise.c)

macro(add_raise_test name source)
    add_canmod_test(${name} ${source} ${RAISE_SRC})
    target_compile_definitions(${name} PRIVATE CANMOD_OUTPUT_RAISE=1)
endmacro()

add_raise_test(test-raise-vehicle-info  test_raise_vehicle_info.c)
add_raise_test(test-raise-doors         test_raise_doors.c)
add_raise_test(test-raise-status-flags  test_raise_status_flags.c)
add_raise_test(test-raise-steering      test_raise_steering.c)
add_raise_test(test-raise-radar         test_raise_radar.c)
add_raise_test(test-raise-ac            test_raise_ac.c)
add_raise_test(test-raise-swc           test_raise_swc.c)
add_raise_test(test-raise-rx            test_raise_rx.c)

# ---------------------------------------------------------------------------
# 4. JSON formatter tests — each needs json_log.c + CANMOD_OUTPUT_JSON
# ---------------------------------------------------------------------------
set(JSON_SRC ${CMAKE_SOURCE_DIR}/src/output/json_log.c)

macro(add_json_test name source)
    add_canmod_test(${name} ${source} ${JSON_SRC})
    target_compile_definitions(${name} PRIVATE CANMOD_OUTPUT_JSON=1)
endmacro()

add_json_test(test-json-speed    test_json_speed.c)
add_json_test(test-json-battery  test_json_battery.c)
add_json_test(test-json-hvac     test_json_hvac.c)
add_json_test(test-json-doors    test_json_doors.c)
add_json_test(test-json-unknown  test_json_unknown.c)

# ---------------------------------------------------------------------------
# 5. Signal decode scaffolding (stub tests — always exit 0 / SKIP)
# ---------------------------------------------------------------------------
add_canmod_test(test-decode-speed-rpm   test_decode_speed_rpm.c)
add_canmod_test(test-decode-hvac        test_decode_hvac.c)
add_canmod_test(test-decode-doors       test_decode_doors.c)
add_canmod_test(test-decode-steering    test_decode_steering.c)
add_canmod_test(test-decode-misc        test_decode_misc.c)

# ---------------------------------------------------------------------------
# 6. Regression test (skips if capture files absent)
# ---------------------------------------------------------------------------
add_executable(test-regression
    test_regression.c
)
target_include_directories(test-regression PRIVATE ${COMMON_INCLUDES})
target_compile_definitions(test-regression PRIVATE
    CANMOD_HOST_PATH="${CMAKE_BINARY_DIR}/canmod-host"
)
target_compile_options(test-regression PRIVATE -Wall -Wextra)
add_test(NAME test-regression COMMAND test-regression
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
