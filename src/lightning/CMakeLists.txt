# Copyright (c) 2025 INTcoin Core (Maddison Lane)
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

# Lightning Network Module

# Source files
set(LIGHTNING_SOURCES
    lightning.cpp
)

# Header files
set(LIGHTNING_HEADERS
    ${CMAKE_SOURCE_DIR}/include/intcoin/lightning.h
)

# Create library
add_library(intcoin_lightning STATIC ${LIGHTNING_SOURCES})

# Include directories
target_include_directories(intcoin_lightning
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# Link dependencies
target_link_libraries(intcoin_lightning
    PUBLIC
        intcoin_core
        intcoin_crypto
        Boost::system
        Boost::thread
        OpenSSL::Crypto
        OpenSSL::SSL
)

# C++ standard
set_target_properties(intcoin_lightning PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

# Compiler warnings
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(intcoin_lightning PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        -Werror
    )
elseif(MSVC)
    target_compile_options(intcoin_lightning PRIVATE
        /W4
        /WX
    )
endif()

# Install
install(TARGETS intcoin_lightning
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
)

install(FILES ${LIGHTNING_HEADERS}
    DESTINATION include/intcoin/lightning
)

# Tests (if enabled)
if(BUILD_TESTS)
    add_subdirectory(tests)
endif()
