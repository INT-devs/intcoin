cmake_minimum_required(VERSION 3.20)

project(INTcoin
    VERSION 1.0.0
    DESCRIPTION "Quantum-Resistant Cryptocurrency"
    LANGUAGES C CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Build options
option(BUILD_TESTS "Build tests" ON)
option(BUILD_WALLET_QT "Build Qt wallet" ON)
option(BUILD_DAEMON "Build intcoind daemon" ON)
option(BUILD_CLI "Build intcoin-cli" ON)
option(BUILD_MINER "Build miner" ON)
option(BUILD_EXPLORER "Build block explorer" ON)
option(ENABLE_LIGHTNING "Enable Lightning Network" ON)
option(ENABLE_TOR "Enable Tor support" OFF)
option(ENABLE_I2P "Enable I2P support" OFF)
option(ENABLE_GPU_MINING "Enable GPU mining" OFF)

# Compiler flags
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(
        -Wall
        -Wextra
        -Wpedantic
        -Werror
        -Wno-unused-parameter
        -march=native
        -O3
    )
endif()

# Find dependencies
find_package(Boost 1.75 REQUIRED COMPONENTS
    system
    filesystem
    thread
    program_options
)

find_package(OpenSSL 3.0 REQUIRED)
find_package(RocksDB REQUIRED)
find_package(Threads REQUIRED)

# liboqs (Open Quantum Safe)
find_library(OQS_LIB oqs REQUIRED)
find_path(OQS_INCLUDE_DIR oqs/oqs.h REQUIRED)

# RandomX
find_library(RANDOMX_LIB randomx REQUIRED)
find_path(RANDOMX_INCLUDE_DIR randomx.h REQUIRED)

# ZeroMQ
find_library(ZMQ_LIB zmq REQUIRED)
find_path(ZMQ_INCLUDE_DIR zmq.h REQUIRED)

# libevent
find_library(EVENT_LIB event REQUIRED)
find_path(EVENT_INCLUDE_DIR event2/event.h REQUIRED)

# Qt6 (optional)
if(BUILD_WALLET_QT)
    find_package(Qt6 6.5 COMPONENTS
        Core
        Widgets
        Network
        Gui
        REQUIRED
    )
endif()

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
    ${Boost_INCLUDE_DIRS}
    ${OPENSSL_INCLUDE_DIR}
    ${OQS_INCLUDE_DIR}
    ${RANDOMX_INCLUDE_DIR}
    ${ZMQ_INCLUDE_DIR}
    ${EVENT_INCLUDE_DIR}
)

# Source files
set(INTCOIN_CORE_SOURCES
    src/blockchain/block.cpp
    src/blockchain/transaction.cpp
    src/blockchain/utxo.cpp
    src/blockchain/chain.cpp
    src/crypto/dilithium.cpp
    src/crypto/kyber.cpp
    src/crypto/hash.cpp
    src/crypto/address.cpp
    src/consensus/pow.cpp
    src/consensus/validation.cpp
    src/consensus/params.cpp
    src/network/node.cpp
    src/network/protocol.cpp
    src/network/peers.cpp
    src/network/messages.cpp
    src/storage/database.cpp
    src/storage/index.cpp
    src/storage/utxo_set.cpp
    src/rpc/server.cpp
    src/rpc/blockchain_rpc.cpp
    src/rpc/wallet_rpc.cpp
    src/util/logging.cpp
    src/util/serialization.cpp
    src/util/time.cpp
)

# Lightning Network sources (optional)
if(ENABLE_LIGHTNING)
    list(APPEND INTCOIN_CORE_SOURCES
        src/lightning/channel.cpp
        src/lightning/payment.cpp
        src/lightning/routing.cpp
        src/lightning/invoice.cpp
    )
endif()

# Core library
add_library(intcoin_core STATIC ${INTCOIN_CORE_SOURCES})
target_link_libraries(intcoin_core
    ${Boost_LIBRARIES}
    OpenSSL::SSL
    OpenSSL::Crypto
    ${ROCKSDB_LIBRARIES}
    ${OQS_LIB}
    ${RANDOMX_LIB}
    ${ZMQ_LIB}
    ${EVENT_LIB}
    Threads::Threads
)

# intcoind daemon
if(BUILD_DAEMON)
    add_executable(intcoind
        src/daemon/intcoind.cpp
    )
    target_link_libraries(intcoind intcoin_core)
    install(TARGETS intcoind DESTINATION bin)
endif()

# intcoin-cli
if(BUILD_CLI)
    add_executable(intcoin-cli
        src/cli/intcoin-cli.cpp
    )
    target_link_libraries(intcoin-cli intcoin_core)
    install(TARGETS intcoin-cli DESTINATION bin)
endif()

# intcoin-miner
if(BUILD_MINER)
    add_executable(intcoin-miner
        src/miner/miner.cpp
        src/miner/randomx_miner.cpp
    )
    target_link_libraries(intcoin-miner intcoin_core)
    install(TARGETS intcoin-miner DESTINATION bin)
endif()

# Qt Wallet
if(BUILD_WALLET_QT)
    set(CMAKE_AUTOMOC ON)
    set(CMAKE_AUTORCC ON)
    set(CMAKE_AUTOUIC ON)

    add_executable(intcoin-qt
        desktop-wallet/src/main.cpp
        desktop-wallet/src/mainwindow.cpp
        desktop-wallet/src/sendcoins.cpp
        desktop-wallet/src/receivecoins.cpp
        desktop-wallet/src/transactions.cpp
    )
    target_link_libraries(intcoin-qt
        intcoin_core
        Qt6::Core
        Qt6::Widgets
        Qt6::Network
        Qt6::Gui
    )
    install(TARGETS intcoin-qt DESTINATION bin)
endif()

# Block Explorer
if(BUILD_EXPLORER)
    add_executable(intcoin-explorer
        explorer/src/main.cpp
        explorer/src/api.cpp
        explorer/src/frontend.cpp
    )
    target_link_libraries(intcoin-explorer intcoin_core)
    install(TARGETS intcoin-explorer DESTINATION bin)
endif()

# Tests
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# Installation
install(DIRECTORY include/ DESTINATION include/intcoin)
install(TARGETS intcoin_core DESTINATION lib)

# Package
set(CPACK_PACKAGE_NAME "INTcoin")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_VENDOR "INTcoin Team")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Quantum-Resistant Cryptocurrency")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "INTcoin")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")
set(CPACK_RESOURCE_FILE_README "${CMAKE_SOURCE_DIR}/README.md")

include(CPack)

# Print configuration
message(STATUS "")
message(STATUS "========================================")
message(STATUS "INTcoin Configuration")
message(STATUS "========================================")
message(STATUS "  Version: ${PROJECT_VERSION}")
message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Install prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "")
message(STATUS "Build options:")
message(STATUS "  Daemon: ${BUILD_DAEMON}")
message(STATUS "  CLI: ${BUILD_CLI}")
message(STATUS "  Qt Wallet: ${BUILD_WALLET_QT}")
message(STATUS "  Miner: ${BUILD_MINER}")
message(STATUS "  Explorer: ${BUILD_EXPLORER}")
message(STATUS "  Tests: ${BUILD_TESTS}")
message(STATUS "  Lightning Network: ${ENABLE_LIGHTNING}")
message(STATUS "  Tor: ${ENABLE_TOR}")
message(STATUS "  I2P: ${ENABLE_I2P}")
message(STATUS "  GPU Mining: ${ENABLE_GPU_MINING}")
message(STATUS "========================================")
message(STATUS "")
