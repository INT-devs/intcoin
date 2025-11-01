# Copyright (c) 2025 INTcoin Core (Maddison Lane)
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

# Tests

# C++ unit tests
add_executable(test_crypto
    test_crypto.cpp
)

target_link_libraries(test_crypto
    PRIVATE
        intcoin_core
)

# Disable NDEBUG for tests so assert() works
target_compile_definitions(test_crypto PRIVATE UNDEBUG)
target_compile_options(test_crypto PRIVATE -Wno-unused-variable)

# Add test
add_test(NAME crypto_tests COMMAND test_crypto)

# Python tests (to be added later)
# find_package(Python3 COMPONENTS Interpreter REQUIRED)
# add_test(NAME python_tests COMMAND ${Python3_EXECUTABLE} -m pytest ${CMAKE_SOURCE_DIR}/tests)
