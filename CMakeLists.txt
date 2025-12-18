# Minimum CMake 3.0, prefer latest stable 4.x series
# Note: CMake 4.2.1 is the latest stable version as of documentation
cmake_minimum_required(VERSION 3.0...4.2.1)

# Set CMake policies for modern behavior
if(${CMAKE_VERSION} VERSION_LESS 3.28)
    cmake_policy(VERSION ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION})
else()
    cmake_policy(VERSION 3.28)
endif()

project(INTcoin
    VERSION 1.0.0
    DESCRIPTION "Quantum-Resistant Cryptocurrency"
    LANGUAGES CXX
)

# Set alpha version suffix
set(INTCOIN_VERSION_SUFFIX "-alpha")
set(INTCOIN_VERSION_FULL "${PROJECT_VERSION}${INTCOIN_VERSION_SUFFIX}")

# Only support 64-bit systems
if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "INTcoin requires a 64-bit system")
endif()

# ============================================================================
# C++ Standard Configuration
# ============================================================================
# Current: C++23 (required)
# TODO: Upgrade to latest C++ standard (C++26) when widely available
# NOTE: C++17 and C++18 support has been DROPPED as of v1.0.0
#       - C++17 is deprecated and no longer supported
#       - C++18 does not exist (C++20 was next after C++17)
#       - Minimum requirement is now C++23 for post-quantum crypto features
# ============================================================================
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
option(BUILD_FAUCET "Build testnet faucet" ON)
option(ENABLE_LIGHTNING "Enable Lightning Network" OFF)  # Deferred
option(ENABLE_TOR "Enable Tor support" OFF)
option(ENABLE_I2P "Enable I2P support" OFF)

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
    # GCC-specific warnings
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        add_compile_options(
            -Wno-stringop-overflow
            -Wno-uninitialized
            -Wno-maybe-uninitialized
        )
    endif()
endif()

# Find dependencies (currently optional for initial build)
find_package(Boost 1.89.0 COMPONENTS
    system
    filesystem
    thread
    program_options
)

find_package(OpenSSL 3.6.0)
find_package(Threads REQUIRED)

# RocksDB (10.7.5+)
find_package(RocksDB 10.7.5)

# liboqs (Open Quantum Safe) - 0.15.0+
find_library(OQS_LIB oqs)
find_path(OQS_INCLUDE_DIR oqs/oqs.h)

# RandomX (1.2.1+) - ASIC-resistant PoW algorithm
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

# ZeroMQ (4.3.5+)
find_library(ZMQ_LIB zmq)
find_path(ZMQ_INCLUDE_DIR zmq.h)

# libevent (2.1.12+)
find_library(EVENT_LIB event)
find_path(EVENT_INCLUDE_DIR event2/event.h)

# Qt6 (optional)
if(BUILD_WALLET_QT)
    find_package(Qt6 6.2 COMPONENTS
        Core
        Widgets
        Network
        Gui
        PrintSupport
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
    src/blockchain/sync.cpp

    # Storage
    src/storage/storage.cpp

    # Cryptography
    src/crypto/crypto.cpp

    # Consensus
    src/consensus/consensus.cpp

    # Network
    src/network/network.cpp

    # Privacy & Anonymous Networking
    src/privacy/privacy.cpp

    # Machine Learning
    src/ml/ml.cpp

    # RPC Server
    src/rpc/rpc.cpp

    # Wallet
    src/wallet/wallet.cpp

    # Mining
    src/mining/mining.cpp

    # Mining Pool
    src/pool/pool.cpp

    # Block Explorer
    src/explorer/explorer.cpp

    # Lightning Network
    src/lightning/lightning.cpp

    # Testnet Faucet
    src/faucet/faucet.cpp
)

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
if(BUILD_WALLET_QT)
    set(CMAKE_AUTOMOC ON)
    set(CMAKE_AUTORCC ON)
    set(CMAKE_AUTOUIC ON)

    # Find RocksDB for Qt wallet
    find_library(ROCKSDB_LIB rocksdb REQUIRED)

    add_executable(intcoin-qt
        src/qt/intcoin-qt.cpp
        src/qt/mainwindow.cpp
        src/qt/overviewpage.cpp
        src/qt/sendcoinspage.cpp
        src/qt/receivecoinspage.cpp
        src/qt/transactionspage.cpp
        src/qt/addressbookpage.cpp
        src/qt/settingspage.cpp
        src/qt/mnemonicDialog.cpp
        include/intcoin/qt/mainwindow.h
        include/intcoin/qt/overviewpage.h
        include/intcoin/qt/sendcoinspage.h
        include/intcoin/qt/receivecoinspage.h
        include/intcoin/qt/transactionspage.h
        include/intcoin/qt/addressbookpage.h
        include/intcoin/qt/settingspage.h
        include/intcoin/qt/mnemonicDialog.h
    )
    target_link_libraries(intcoin-qt
        intcoin_core
        ${ROCKSDB_LIB}
        Qt6::Core
        Qt6::Widgets
        Qt6::Network
        Qt6::Gui
        Qt6::PrintSupport
    )
    install(TARGETS intcoin-qt DESTINATION bin)
endif()

# Testnet Faucet
if(BUILD_FAUCET)
    find_library(ROCKSDB_LIB rocksdb REQUIRED)

    add_executable(intcoin-faucet
        src/faucet/intcoin-faucet.cpp
    )
    target_link_libraries(intcoin-faucet intcoin_core ${ROCKSDB_LIB})
    install(TARGETS intcoin-faucet DESTINATION bin)
endif()

# Genesis Block Miner Tool (development/testing)
# Disabled - file moved or removed
# find_library(ROCKSDB_LIB rocksdb REQUIRED)
# add_executable(mine_genesis
#     tools/mine_genesis.cpp
# )
# target_link_libraries(mine_genesis intcoin_core ${ROCKSDB_LIB} ssl crypto)

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
set(CPACK_PACKAGE_VERSION "${INTCOIN_VERSION_FULL}")
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
message(STATUS "  Version: ${INTCOIN_VERSION_FULL}")
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
message(STATUS "========================================")
message(STATUS "")
