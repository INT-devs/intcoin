# Copyright (c) 2025 INTcoin Core (Maddison Lane)
# RISC-V 64-bit Cross-Compilation Toolchain File
#
# Usage:
#   cmake -DCMAKE_TOOLCHAIN_FILE=cmake/riscv64-linux-gnu.cmake ..

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR riscv64)

# Cross-compiler settings
set(CROSS_COMPILE_PREFIX "riscv64-linux-gnu-" CACHE STRING "Cross-compile prefix")

set(CMAKE_C_COMPILER ${CROSS_COMPILE_PREFIX}gcc)
set(CMAKE_CXX_COMPILER ${CROSS_COMPILE_PREFIX}g++)
set(CMAKE_AR ${CROSS_COMPILE_PREFIX}ar)
set(CMAKE_RANLIB ${CROSS_COMPILE_PREFIX}ranlib)
set(CMAKE_STRIP ${CROSS_COMPILE_PREFIX}strip)

# Target sysroot (adjust as needed)
# set(CMAKE_SYSROOT /path/to/riscv64-sysroot)

# Search for programs in the host directories
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)

# Search for libraries and headers in the target directories
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# RISC-V specific compiler flags
set(CMAKE_C_FLAGS_INIT "-march=rv64gc -mabi=lp64d")
set(CMAKE_CXX_FLAGS_INIT "-march=rv64gc -mabi=lp64d")

# Disable features that may not be available on RISC-V
set(BUILD_QT_WALLET OFF CACHE BOOL "Qt wallet disabled for cross-compile" FORCE)

message(STATUS "Cross-compiling for RISC-V 64-bit (rv64gc)")
