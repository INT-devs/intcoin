# Copyright (c) 2025 INTcoin Core (Maddison Lane)
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
#
# INTcoin Core CMake Build System

cmake_minimum_required(VERSION 4.0)

# Project information
project(INTcoin
    VERSION 1.2.0
    DESCRIPTION "Quantum-resistant cryptocurrency with SHA256 PoW"
    LANGUAGES CXX C
)

# Semantic versioning
set(INTCOIN_VERSION_MAJOR 1)
set(INTCOIN_VERSION_MINOR 2)
set(INTCOIN_VERSION_PATCH 0)
set(INTCOIN_VERSION "${INTCOIN_VERSION_MAJOR}.${INTCOIN_VERSION_MINOR}.${INTCOIN_VERSION_PATCH}")

# Require 64-bit architecture
if(CMAKE_SIZEOF_VOID_P EQUAL 4)
    message(FATAL_ERROR "INTcoin requires 64-bit architecture. 32-bit builds are not supported.")
endif()

# C++23 requirement
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# C11 for compatibility
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# Options
option(BUILD_TESTS "Build test suite" ON)
option(BUILD_QT_WALLET "Build Qt wallet GUI" ON)
option(BUILD_DAEMON "Build intcoind daemon" ON)
option(BUILD_CLI "Build intcoin-cli" ON)
option(BUILD_MINER "Build CPU miner" ON)
option(BUILD_EXPLORER "Build block explorer" ON)
option(ENABLE_LIGHTNING "Enable Lightning Network support" OFF)
option(ENABLE_SMART_CONTRACTS "Enable smart contract support" OFF)
option(ENABLE_CROSS_CHAIN "Enable cross-chain bridge" OFF)
option(ENABLE_WALLET "Enable wallet functionality" ON)
option(ENABLE_I2P "Enable I2P network support" ON)
option(ENABLE_ML "Enable Machine Learning features" ON)

# Platform detection
if(APPLE)
    set(MACOSX TRUE)
elseif(UNIX AND NOT APPLE)
    if(CMAKE_SYSTEM_NAME MATCHES "FreeBSD")
        set(FREEBSD TRUE)
    else()
        set(LINUX TRUE)
    endif()
elseif(WIN32)
    set(WINDOWS TRUE)
endif()

# Architecture detection
if(CMAKE_SYSTEM_PROCESSOR MATCHES "riscv64|riscv")
    set(RISCV TRUE)
    message(STATUS "RISC-V architecture detected")
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64|ARM64")
    set(ARM64 TRUE)
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64")
    set(X86_64 TRUE)
endif()

# Compiler flags
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(
        -Wall
        -Wextra
        -Wpedantic
        -Werror
        -Wno-unused-parameter
        -fstack-protector-strong
        -fPIC
    )

    # Architecture-specific optimizations
    if(RISCV)
        # RISC-V specific flags
        add_compile_options(-march=rv64gc -mabi=lp64d)
        message(STATUS "Using RISC-V optimizations: rv64gc, lp64d ABI")
    elseif(ARM64)
        # ARM64 optimizations
        if(NOT APPLE)
            add_compile_options(-march=armv8-a)
        endif()
    elseif(X86_64)
        # x86_64 optimizations (conservative for compatibility)
        add_compile_options(-march=x86-64)
    endif()

    if(CMAKE_BUILD_TYPE MATCHES "Release")
        add_compile_options(-O3 -DNDEBUG)
    elseif(CMAKE_BUILD_TYPE MATCHES "Debug")
        add_compile_options(-O0 -g -DDEBUG)
    endif()
elseif(MSVC)
    add_compile_options(
        /W4
        /WX
        /permissive-
        /std:c++latest
    )
    if(CMAKE_BUILD_TYPE MATCHES "Release")
        add_compile_options(/O2 /DNDEBUG)
    elseif(CMAKE_BUILD_TYPE MATCHES "Debug")
        add_compile_options(/Od /Zi /DDEBUG)
    endif()
endif()

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_BINARY_DIR}/include
)

# Add cmake module path for custom find modules
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

# Find required packages
find_package(Threads REQUIRED)
find_package(OpenSSL 3.0 REQUIRED)
find_package(RocksDB REQUIRED)

# Qt (Qt6 preferred, Qt5 fallback) for wallet GUI
if(BUILD_QT_WALLET)
    # Try Qt6 first (6.2+), fall back to Qt5 (5.9+)
    find_package(Qt6 6.2 COMPONENTS Core Widgets Network Gui QUIET)
    if(Qt6_FOUND)
        message(STATUS "Using Qt6 ${Qt6_VERSION}")
        set(QT_VERSION_MAJOR 6)
    else()
        find_package(Qt5 5.9 COMPONENTS Core Widgets Network Gui REQUIRED)
        message(STATUS "Using Qt5 ${Qt5_VERSION}")
        set(QT_VERSION_MAJOR 5)
    endif()
    set(CMAKE_AUTOMOC ON)
    set(CMAKE_AUTORCC ON)
    set(CMAKE_AUTOUIC ON)
endif()

# Boost libraries (modern CMake policy)
if(POLICY CMP0167)
    cmake_policy(SET CMP0167 NEW)
endif()
set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
find_package(Boost 1.70.0 REQUIRED)

# External dependencies
add_subdirectory(external/dilithium EXCLUDE_FROM_ALL)
add_subdirectory(external/kyber EXCLUDE_FROM_ALL)

# Generate version header
configure_file(
    ${CMAKE_SOURCE_DIR}/src/version.h.in
    ${CMAKE_BINARY_DIR}/include/intcoin/version.h
)

configure_file(
    ${CMAKE_SOURCE_DIR}/src/config.h.in
    ${CMAKE_BINARY_DIR}/include/intcoin/config.h
)

# Source files
add_subdirectory(src/crypto)
add_subdirectory(src/core)
add_subdirectory(src/network)
add_subdirectory(src/consensus)
add_subdirectory(src/utils)
add_subdirectory(src/db)

if(ENABLE_WALLET)
    add_subdirectory(src/wallet)
endif()

if(BUILD_MINER)
    add_subdirectory(src/miner)
endif()

if(ENABLE_LIGHTNING)
    add_subdirectory(src/lightning)
endif()

if(ENABLE_SMART_CONTRACTS)
    add_subdirectory(src/contracts)
endif()

if(ENABLE_CROSS_CHAIN)
    add_subdirectory(src/bridge)
endif()

if(ENABLE_I2P AND EXISTS ${CMAKE_SOURCE_DIR}/src/i2p)
    add_subdirectory(src/i2p)
endif()

if(ENABLE_ML AND EXISTS ${CMAKE_SOURCE_DIR}/src/ml)
    add_subdirectory(src/ml)
endif()

add_subdirectory(src/rpc)

# Core library
add_library(intcoin_core STATIC
    $<TARGET_OBJECTS:crypto>
    $<TARGET_OBJECTS:core>
    $<TARGET_OBJECTS:network>
    $<TARGET_OBJECTS:consensus>
    $<TARGET_OBJECTS:utils>
    $<TARGET_OBJECTS:db>
    $<TARGET_OBJECTS:rpc>
    $<$<BOOL:${ENABLE_WALLET}>:$<TARGET_OBJECTS:wallet>>
    $<$<BOOL:${BUILD_MINER}>:$<TARGET_OBJECTS:miner_lib>>
)

target_link_libraries(intcoin_core
    PUBLIC
        Threads::Threads
        OpenSSL::SSL
        OpenSSL::Crypto
        Boost::boost
        RocksDB::rocksdb
        dilithium
        kyber
)

# Link liboqs (REQUIRED for post-quantum crypto)
find_library(LIBOQS_LIBRARY NAMES oqs liboqs
    HINTS
        /usr/local/lib
        /opt/homebrew/lib
        /usr/lib
    REQUIRED
)
if(NOT LIBOQS_LIBRARY)
    message(FATAL_ERROR "liboqs is required but not found. Please install liboqs:\n"
        "  Ubuntu/Debian: Run scripts/install_liboqs_debian.sh\n"
        "  macOS: Run scripts/install_liboqs_macos.sh\n"
        "  FreeBSD: Run scripts/install_liboqs_freebsd.sh\n"
        "  Or build from source: https://github.com/open-quantum-safe/liboqs")
endif()
message(STATUS "Found liboqs: ${LIBOQS_LIBRARY}")
target_link_libraries(intcoin_core PUBLIC ${LIBOQS_LIBRARY})

# Find and link libusb and hidapi for hardware wallet support
find_library(LIBUSB_LIBRARY NAMES usb-1.0 libusb-1.0
    HINTS
        /usr/local/lib
        /opt/homebrew/lib
        /usr/lib
)
find_library(HIDAPI_LIBRARY NAMES hidapi hidapi-libusb
    HINTS
        /usr/local/lib
        /opt/homebrew/lib
        /usr/lib
)

# Find include directories for libusb and hidapi
find_path(LIBUSB_INCLUDE_DIR
    NAMES libusb.h
    HINTS
        /usr/local/include/libusb-1.0
        /opt/homebrew/include/libusb-1.0
        /usr/include/libusb-1.0
)
find_path(HIDAPI_INCLUDE_DIR
    NAMES hidapi.h
    HINTS
        /usr/local/include/hidapi
        /opt/homebrew/include/hidapi
        /usr/include/hidapi
)

if(LIBUSB_LIBRARY AND LIBUSB_INCLUDE_DIR)
    message(STATUS "Found libusb: ${LIBUSB_LIBRARY}")
    target_include_directories(intcoin_core PUBLIC ${LIBUSB_INCLUDE_DIR})
    target_link_libraries(intcoin_core PUBLIC ${LIBUSB_LIBRARY})
else()
    message(WARNING "libusb not found - hardware wallet support will be limited")
endif()

if(HIDAPI_LIBRARY AND HIDAPI_INCLUDE_DIR)
    message(STATUS "Found hidapi: ${HIDAPI_LIBRARY}")
    target_include_directories(intcoin_core PUBLIC ${HIDAPI_INCLUDE_DIR})
    target_link_libraries(intcoin_core PUBLIC ${HIDAPI_LIBRARY})
else()
    message(WARNING "hidapi not found - hardware wallet support will be limited")
endif()

# Daemon
if(BUILD_DAEMON)
    add_executable(intcoind
        src/daemon/main.cpp
    )
    target_link_libraries(intcoind
        PRIVATE
            intcoin_core
    )
    install(TARGETS intcoind DESTINATION bin)
endif()

# CLI tool
if(BUILD_CLI)
    add_executable(intcoin-cli
        src/cli/main.cpp
    )
    target_link_libraries(intcoin-cli
        PRIVATE
            intcoin_core
            rpc
            network
    )
    install(TARGETS intcoin-cli DESTINATION bin)
endif()

# Wallet tool
if(ENABLE_WALLET)
    add_executable(intcoin-wallet
        src/wallet/main.cpp
    )
    target_link_libraries(intcoin-wallet
        PRIVATE
            intcoin_core
            wallet
    )
    install(TARGETS intcoin-wallet DESTINATION bin)
endif()

# Qt Wallet
if(BUILD_QT_WALLET)
    add_subdirectory(src/qt)
endif()

# Miner
if(BUILD_MINER)
    add_executable(intcoin-miner
        src/miner/main.cpp
    )
    target_link_libraries(intcoin-miner
        PRIVATE
            intcoin_core
    )
    install(TARGETS intcoin-miner DESTINATION bin)
endif()

# Block Explorer
if(BUILD_EXPLORER)
    add_executable(intcoin-explorer
        src/explorer/main.cpp
    )
    target_link_libraries(intcoin-explorer
        PRIVATE
            intcoin_core
    )
    install(TARGETS intcoin-explorer DESTINATION bin)
endif()

# Tests
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# Tools
add_subdirectory(tools)

# Installation
install(DIRECTORY include/ DESTINATION include)
install(DIRECTORY config/ DESTINATION share/intcoin/config)

# Documentation
install(FILES
    README.md
    COPYING
    DESIGN-DECISIONS.md
    DESTINATION share/doc/intcoin
)

# CPack configuration for packaging
set(CPACK_PACKAGE_NAME "INTcoin")
set(CPACK_PACKAGE_VERSION "${INTCOIN_VERSION}")
set(CPACK_PACKAGE_VENDOR "INTcoin Core (Maddison Lane)")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Quantum-resistant cryptocurrency")
set(CPACK_PACKAGE_CONTACT "team@international-coin.org")

if(WIN32)
    set(CPACK_GENERATOR "NSIS;ZIP")
elseif(APPLE)
    set(CPACK_GENERATOR "DragNDrop;TGZ")
else()
    set(CPACK_GENERATOR "DEB;RPM;TGZ")
endif()

include(CPack)

# Print build configuration
message(STATUS "")
message(STATUS "INTcoin ${INTCOIN_VERSION} Configuration Summary")
message(STATUS "=====================================")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "C++ standard: C++${CMAKE_CXX_STANDARD}")
message(STATUS "Platform: ${CMAKE_SYSTEM_NAME}")
message(STATUS "Architecture: ${CMAKE_SYSTEM_PROCESSOR}")
if(RISCV)
    message(STATUS "  RISC-V: Yes (rv64gc)")
elseif(ARM64)
    message(STATUS "  ARM64: Yes")
elseif(X86_64)
    message(STATUS "  x86_64: Yes")
endif()
message(STATUS "")
message(STATUS "Build options:")
message(STATUS "  Tests:           ${BUILD_TESTS}")
message(STATUS "  Qt Wallet:       ${BUILD_QT_WALLET}")
message(STATUS "  Daemon:          ${BUILD_DAEMON}")
message(STATUS "  CLI:             ${BUILD_CLI}")
message(STATUS "  Miner:           ${BUILD_MINER}")
message(STATUS "  Explorer:        ${BUILD_EXPLORER}")
message(STATUS "  Lightning:       ${ENABLE_LIGHTNING}")
message(STATUS "  Smart Contracts: ${ENABLE_SMART_CONTRACTS}")
message(STATUS "  Cross-chain:     ${ENABLE_CROSS_CHAIN}")
message(STATUS "  Wallet:          ${ENABLE_WALLET}")
message(STATUS "  I2P Network:     ${ENABLE_I2P}")
message(STATUS "  Machine Learning: ${ENABLE_ML}")
message(STATUS "")
