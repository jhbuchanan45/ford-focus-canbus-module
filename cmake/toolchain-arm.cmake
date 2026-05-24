# cmake/toolchain-arm.cmake
# Cross-compilation toolchain for STM32F103 (Cortex-M3) using arm-none-eabi-gcc.
#
# Usage:
#   cmake -DCANMOD_TARGET=stm32f1 \
#         -DCANMOD_OUTPUT=raise \
#         -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-arm.cmake \
#         -B build/arm ..

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

# Toolchain prefix — must be on PATH
set(TOOLCHAIN_PREFIX arm-none-eabi-)

set(CMAKE_C_COMPILER   ${TOOLCHAIN_PREFIX}gcc)
set(CMAKE_CXX_COMPILER ${TOOLCHAIN_PREFIX}g++)
set(CMAKE_ASM_COMPILER ${TOOLCHAIN_PREFIX}gcc)
set(CMAKE_AR           ${TOOLCHAIN_PREFIX}ar    CACHE FILEPATH "Archiver")
set(CMAKE_OBJCOPY      ${TOOLCHAIN_PREFIX}objcopy)
set(CMAKE_OBJDUMP      ${TOOLCHAIN_PREFIX}objdump)
set(CMAKE_SIZE         ${TOOLCHAIN_PREFIX}size)

# Prevent CMake from trying to link test executables for the target system
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# Cortex-M3 core flags (applied to all C and ASM compilation)
set(_CPU_FLAGS "-mcpu=cortex-m3 -mthumb")
set(CMAKE_C_FLAGS_INIT   "${_CPU_FLAGS} -Wall -Wextra -fno-common -ffunction-sections -fdata-sections" CACHE STRING "" FORCE)
set(CMAKE_ASM_FLAGS_INIT "${_CPU_FLAGS}" CACHE STRING "" FORCE)

set(CMAKE_EXE_LINKER_FLAGS_INIT
    "${_CPU_FLAGS} -Wl,--gc-sections -specs=nosys.specs -specs=nano.specs"
    CACHE STRING "" FORCE)
