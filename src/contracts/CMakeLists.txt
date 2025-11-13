# Copyright (c) 2025 INTcoin Core (Maddison Lane)
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

# Smart Contracts Module

# Source files
set(CONTRACTS_SOURCES
    vm.cpp
    contract.cpp
    opcodes.cpp
    gas.cpp
    storage.cpp
    security_analyzer.cpp
)

# Header files
set(CONTRACTS_HEADERS
    ${CMAKE_SOURCE_DIR}/include/intcoin/contracts/vm.h
    ${CMAKE_SOURCE_DIR}/include/intcoin/contracts/contract.h
)

# Create library
add_library(intcoin_contracts STATIC ${CONTRACTS_SOURCES})

# Include directories
target_include_directories(intcoin_contracts
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# Link dependencies
target_link_libraries(intcoin_contracts
    PUBLIC
        intcoin_core
        intcoin_crypto
        Boost::boost
        OpenSSL::Crypto
)

# C++ standard
set_target_properties(intcoin_contracts PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

# Compiler warnings
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(intcoin_contracts PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        -Werror
    )
elseif(MSVC)
    target_compile_options(intcoin_contracts PRIVATE
        /W4
        /WX
    )
endif()

# Install
install(TARGETS intcoin_contracts
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
)

install(FILES ${CONTRACTS_HEADERS}
    DESTINATION include/intcoin/contracts
)

# Tests (if enabled)
if(BUILD_TESTS)
    add_subdirectory(tests)
endif()
