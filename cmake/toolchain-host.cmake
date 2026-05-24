# cmake/toolchain-host.cmake
# Native x86/x86_64 toolchain for the host (PC) build.
# Uses the system GCC; no cross-compilation.
#
# Usage:
#   cmake -DCANMOD_TARGET=host \
#         -DCANMOD_OUTPUT=json \
#         -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-host.cmake \
#         -B build/host ..

set(CMAKE_SYSTEM_NAME Linux)

# Use system compilers — find_program respects PATH
find_program(CMAKE_C_COMPILER   NAMES gcc cc   REQUIRED)
find_program(CMAKE_CXX_COMPILER NAMES g++ c++  REQUIRED)

set(CMAKE_C_FLAGS_INIT "-Wall -Wextra -g -O0" CACHE STRING "" FORCE)
