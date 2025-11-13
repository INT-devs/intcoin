# Copyright (c) 2025 INTcoin Core (Maddison Lane)
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

# Cross-Chain Bridge Module

# Source files
set(BRIDGE_SOURCES
    atomic_swap.cpp
    spv_proof.cpp
    bitcoin_bridge.cpp
    ethereum_bridge.cpp
    bridge_manager.cpp
    htlc.cpp
)

# Header files
set(BRIDGE_HEADERS
    ${CMAKE_SOURCE_DIR}/include/intcoin/bridge/atomic_swap.h
    ${CMAKE_SOURCE_DIR}/include/intcoin/bridge/spv_proof.h
    ${CMAKE_SOURCE_DIR}/include/intcoin/bridge/bridge.h
)

# Create library
add_library(intcoin_bridge STATIC ${BRIDGE_SOURCES})

# Include directories
target_include_directories(intcoin_bridge
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# Link dependencies
target_link_libraries(intcoin_bridge
    PUBLIC
        intcoin_core
        intcoin_crypto
        Boost::boost
        OpenSSL::Crypto
        OpenSSL::SSL
)

# C++ standard
set_target_properties(intcoin_bridge PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

# Compiler warnings
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(intcoin_bridge PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        -Werror
    )
elseif(MSVC)
    target_compile_options(intcoin_bridge PRIVATE
        /W4
        /WX
    )
endif()

# Install
install(TARGETS intcoin_bridge
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
)

install(FILES ${BRIDGE_HEADERS}
    DESTINATION include/intcoin/bridge
)

# Tests (if enabled)
if(BUILD_TESTS)
    add_subdirectory(tests)
endif()
