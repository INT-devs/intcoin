cmake_minimum_required(VERSION 4.2.0)

project(INTcoin
    VERSION 1.0.0
    DESCRIPTION "Quantum-Resistant Cryptocurrency"
    LANGUAGES CXX
)

# Only support 64-bit systems
if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "INTcoin requires a 64-bit system")
endif()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build options
option(BUILD_TESTS "Build tests" ON)
option(BUILD_WALLET_QT "Build Qt wallet" ON)
option(BUILD_DAEMON "Build intcoind daemon" ON)
option(BUILD_CLI "Build intcoin-cli" ON)
option(BUILD_MINER "Build miner" ON)
option(BUILD_EXPLORER "Build block explorer" ON)
option(ENABLE_LIGHTNING "Enable Lightning Network" OFF)  # Deferred
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

# Find dependencies (currently optional for initial build)
find_package(Boost 1.89.0 COMPONENTS
    system
    filesystem
    thread
    program_options
)

find_package(OpenSSL 3.5.4)
find_package(Threads REQUIRED)

# RocksDB
find_package(RocksDB)

# liboqs (Open Quantum Safe)
find_library(OQS_LIB oqs)
find_path(OQS_INCLUDE_DIR oqs/oqs.h)

# RandomX
find_library(RANDOMX_LIB randomx
    PATHS
        $ENV{HOME}/.local/lib
        /opt/homebrew/lib
        /usr/local/lib
)
find_path(RANDOMX_INCLUDE_DIR randomx.h
    PATHS
        $ENV{HOME}/.local/include
        /opt/homebrew/include
        /usr/local/include
)

# ZeroMQ
find_library(ZMQ_LIB zmq)
find_path(ZMQ_INCLUDE_DIR zmq.h)

# libevent
find_library(EVENT_LIB event)
find_path(EVENT_INCLUDE_DIR event2/event.h)

# Qt6 (optional)
if(BUILD_WALLET_QT)
    find_package(Qt6 6.8 COMPONENTS
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
)

# Add optional include directories if found
if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
endif()

if(OPENSSL_FOUND)
    include_directories(${OPENSSL_INCLUDE_DIR})
endif()

if(OQS_INCLUDE_DIR)
    include_directories(${OQS_INCLUDE_DIR})
endif()

if(RANDOMX_INCLUDE_DIR)
    include_directories(${RANDOMX_INCLUDE_DIR})
endif()

if(ZMQ_INCLUDE_DIR)
    include_directories(${ZMQ_INCLUDE_DIR})
endif()

if(EVENT_INCLUDE_DIR)
    include_directories(${EVENT_INCLUDE_DIR})
endif()

# Source files
set(INTCOIN_CORE_SOURCES
    # Core
    src/core/intcoin.cpp

    # Utilities
    src/util/types.cpp
    src/util/util.cpp

    # Blockchain
    src/blockchain/block.cpp
    src/blockchain/transaction.cpp
    src/blockchain/script.cpp
    src/blockchain/blockchain.cpp
    src/blockchain/validation.cpp

    # Storage
    src/storage/storage.cpp

    # Cryptography
    src/crypto/crypto.cpp

    # Consensus
    src/consensus/consensus.cpp

    # Network
    src/network/network.cpp

    # Machine Learning
    src/ml/ml.cpp

    # RPC Server
    src/rpc/rpc.cpp

    # Wallet
    src/wallet/wallet.cpp

    # Mining
    src/mining/mining.cpp

    # Block Explorer
    src/explorer/explorer.cpp
)

# Lightning Network sources (optional)
# TODO: Uncomment when Lightning Network implementation is added
# if(ENABLE_LIGHTNING)
#     list(APPEND INTCOIN_CORE_SOURCES
#         src/lightning/channel.cpp
#         src/lightning/payment.cpp
#         src/lightning/routing.cpp
#         src/lightning/invoice.cpp
#     )
# endif()

# Core library
add_library(intcoin_core STATIC ${INTCOIN_CORE_SOURCES})
target_link_libraries(intcoin_core
    Threads::Threads
)

# Link optional dependencies if found
if(Boost_FOUND)
    target_link_libraries(intcoin_core ${Boost_LIBRARIES})
endif()

if(OPENSSL_FOUND)
    target_link_libraries(intcoin_core OpenSSL::SSL OpenSSL::Crypto)
endif()

if(ROCKSDB_FOUND)
    target_link_libraries(intcoin_core ${ROCKSDB_LIBRARIES})
endif()

if(OQS_LIB)
    target_link_libraries(intcoin_core ${OQS_LIB})
endif()

if(RANDOMX_LIB)
    target_link_libraries(intcoin_core ${RANDOMX_LIB})
endif()

if(ZMQ_LIB)
    target_link_libraries(intcoin_core ${ZMQ_LIB})
endif()

if(EVENT_LIB)
    target_link_libraries(intcoin_core ${EVENT_LIB})
endif()

# intcoind daemon
if(BUILD_DAEMON)
    # Find RocksDB (same way as tests)
    find_library(ROCKSDB_LIB rocksdb REQUIRED)

    add_executable(intcoind
        src/daemon/intcoind.cpp
    )
    target_link_libraries(intcoind intcoin_core ${ROCKSDB_LIB})

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
    find_library(ROCKSDB_LIB rocksdb REQUIRED)

    add_executable(intcoin-miner
        src/miner/intcoin-miner.cpp
    )
    target_link_libraries(intcoin-miner intcoin_core ${ROCKSDB_LIB})
    install(TARGETS intcoin-miner DESTINATION bin)
endif()

# Qt Wallet
# if(BUILD_WALLET_QT)
#     set(CMAKE_AUTOMOC ON)
#     set(CMAKE_AUTORCC ON)
#     set(CMAKE_AUTOUIC ON)
#
#     add_executable(intcoin-qt
#         desktop-wallet/src/main.cpp
#         desktop-wallet/src/mainwindow.cpp
#         desktop-wallet/src/sendcoins.cpp
#         desktop-wallet/src/receivecoins.cpp
#         desktop-wallet/src/transactions.cpp
#     )
#     target_link_libraries(intcoin-qt
#         intcoin_core
#         Qt6::Core
#         Qt6::Widgets
#         Qt6::Network
#         Qt6::Gui
#     )
#     install(TARGETS intcoin-qt DESTINATION bin)
# endif()

# Block Explorer
# if(BUILD_EXPLORER)
#     add_executable(intcoin-explorer
#         explorer/src/main.cpp
#         explorer/src/api.cpp
#         explorer/src/frontend.cpp
#     )
#     target_link_libraries(intcoin-explorer intcoin_core)
#     install(TARGETS intcoin-explorer DESTINATION bin)
# endif()

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
