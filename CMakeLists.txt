cmake_minimum_required(VERSION 3.16)
project(canmod C ASM)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# ---------------------------------------------------------------------------
# Build options
# ---------------------------------------------------------------------------
set(CANMOD_TARGET "host"  CACHE STRING "Build target:  stm32f1 | host")
set(CANMOD_OUTPUT "json"  CACHE STRING "Output driver: json    | raise")

set_property(CACHE CANMOD_TARGET PROPERTY STRINGS stm32f1 host)
set_property(CACHE CANMOD_OUTPUT PROPERTY STRINGS json raise)

message(STATUS "CANMOD_TARGET = ${CANMOD_TARGET}")
message(STATUS "CANMOD_OUTPUT = ${CANMOD_OUTPUT}")

# ---------------------------------------------------------------------------
# Common source files (target- and output-independent)
# ---------------------------------------------------------------------------
set(SRC ${CMAKE_SOURCE_DIR}/src)

set(COMMON_SOURCES
    ${SRC}/main.c
    ${SRC}/cars/ford_focus_mk3_2015.c
)

# ---------------------------------------------------------------------------
# Output driver
# ---------------------------------------------------------------------------
if(CANMOD_OUTPUT STREQUAL "json")
    list(APPEND COMMON_SOURCES ${SRC}/output/json_log.c)
    add_compile_definitions(CANMOD_OUTPUT_JSON=1)
elseif(CANMOD_OUTPUT STREQUAL "raise")
    list(APPEND COMMON_SOURCES ${SRC}/output/raise.c)
    add_compile_definitions(CANMOD_OUTPUT_RAISE=1)
else()
    message(FATAL_ERROR
        "CANMOD_OUTPUT must be 'json' or 'raise' (got '${CANMOD_OUTPUT}').\n"
        "Example: cmake -DCANMOD_OUTPUT=json ..")
endif()

# ---------------------------------------------------------------------------
# STM32F103 target
# ---------------------------------------------------------------------------
if(CANMOD_TARGET STREQUAL "stm32f1")
    add_compile_definitions(CANMOD_TARGET_STM32F1=1)

    set(HAL_DIR ${SRC}/hal/stm32f1)
    set(HAL_SOURCES
        ${HAL_DIR}/can_hal.c
        ${HAL_DIR}/uart_hal.c
        ${HAL_DIR}/usb_hal.c
        ${HAL_DIR}/gpio_hal.c
        ${HAL_DIR}/timer_hal.c
    )

    # libopencm3 submodule sanity check
    set(OPENCM3_DIR ${CMAKE_SOURCE_DIR}/lib/libopencm3)
    if(NOT EXISTS ${OPENCM3_DIR}/include/libopencm3)
        message(FATAL_ERROR
            "libopencm3 not found at ${OPENCM3_DIR}.\n"
            "Run:  git submodule update --init lib/libopencm3\n"
            "Then re-run cmake.")
    endif()

    # Build libopencm3 for STM32F1 (runs once; idempotent)
    add_custom_target(opencm3_build
        COMMAND ${CMAKE_MAKE_PROGRAM} TARGETS=stm32/f1
        WORKING_DIRECTORY ${OPENCM3_DIR}
        COMMENT "Building libopencm3 for STM32F1"
        VERBATIM
    )

    add_executable(canmod ${COMMON_SOURCES} ${HAL_SOURCES})
    add_dependencies(canmod opencm3_build)

    target_include_directories(canmod PRIVATE
        ${SRC}
        ${OPENCM3_DIR}/include
    )
    target_link_directories(canmod PRIVATE ${OPENCM3_DIR}/lib)
    target_link_libraries(canmod PRIVATE opencm3_stm32f1)

    set_target_properties(canmod PROPERTIES SUFFIX ".elf")

    # Linker script — use the bundled one for STM32F103x8 (64K flash / 20K RAM)
    set(LINKER_SCRIPT ${CMAKE_SOURCE_DIR}/stm32f103.ld)
    target_link_options(canmod PRIVATE
        -T${LINKER_SCRIPT}
        -Wl,-Map=${CMAKE_BINARY_DIR}/canmod.map
        -Wl,--gc-sections
        -nostartfiles
    )

    # Post-build: produce .bin for ST-Link / OpenOCD flashing
    add_custom_command(TARGET canmod POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} -O binary
            $<TARGET_FILE:canmod>
            ${CMAKE_BINARY_DIR}/canmod.bin
        COMMENT "Generating canmod.bin"
    )
    add_custom_command(TARGET canmod POST_BUILD
        COMMAND ${CMAKE_SIZE} $<TARGET_FILE:canmod>
        COMMENT "Firmware size"
    )

# ---------------------------------------------------------------------------
# Host (x86/x86_64) target
# ---------------------------------------------------------------------------
elseif(CANMOD_TARGET STREQUAL "host")
    add_compile_definitions(CANMOD_TARGET_HOST=1)

    set(HAL_DIR ${SRC}/hal/host)
    set(HAL_SOURCES
        ${HAL_DIR}/can_hal.c
        ${HAL_DIR}/uart_hal.c
        ${HAL_DIR}/usb_hal.c
        ${HAL_DIR}/gpio_hal.c
        ${HAL_DIR}/timer_hal.c
    )

    add_executable(canmod-host ${COMMON_SOURCES} ${HAL_SOURCES})

    target_include_directories(canmod-host PRIVATE ${SRC})
    target_compile_options(canmod-host PRIVATE -Wall -Wextra)

    # SocketCAN lives in the standard kernel headers; no extra lib needed
    # on modern Linux.  Link pthread for clock_gettime on older glibc.
    target_link_libraries(canmod-host PRIVATE m)

else()
    message(FATAL_ERROR
        "CANMOD_TARGET must be 'stm32f1' or 'host' (got '${CANMOD_TARGET}').\n"
        "Example: cmake -DCANMOD_TARGET=host ..")
endif()
