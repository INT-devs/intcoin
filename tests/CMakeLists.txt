# Copyright (c) 2025 INTcoin Core (Maddison Lane)
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

# Tests

# Common test settings
set(TEST_COMPILE_OPTIONS -Wno-unused-variable -Wno-unused-parameter)
set(TEST_DEFINITIONS UNDEBUG)  # Disable NDEBUG for tests so assert() works

# Test executables
set(TEST_TARGETS
    test_crypto
    test_blockchain
    test_wallet
    test_network
    test_lightning
    test_tor
    test_security
    test_dilithium_nist
)

# Crypto tests
add_executable(test_crypto test_crypto.cpp)
target_link_libraries(test_crypto PRIVATE intcoin_core)
target_compile_definitions(test_crypto PRIVATE ${TEST_DEFINITIONS})
target_compile_options(test_crypto PRIVATE ${TEST_COMPILE_OPTIONS})
add_test(NAME crypto_tests COMMAND test_crypto)

# Blockchain tests
add_executable(test_blockchain test_blockchain.cpp)
target_link_libraries(test_blockchain PRIVATE intcoin_core)
target_compile_definitions(test_blockchain PRIVATE ${TEST_DEFINITIONS})
target_compile_options(test_blockchain PRIVATE ${TEST_COMPILE_OPTIONS})
add_test(NAME blockchain_tests COMMAND test_blockchain)

# Wallet tests
add_executable(test_wallet test_wallet.cpp)
target_link_libraries(test_wallet PRIVATE intcoin_core)
target_compile_definitions(test_wallet PRIVATE ${TEST_DEFINITIONS})
target_compile_options(test_wallet PRIVATE ${TEST_COMPILE_OPTIONS})
add_test(NAME wallet_tests COMMAND test_wallet)

# Network tests
add_executable(test_network test_network.cpp)
target_link_libraries(test_network PRIVATE intcoin_core)
target_compile_definitions(test_network PRIVATE ${TEST_DEFINITIONS})
target_compile_options(test_network PRIVATE ${TEST_COMPILE_OPTIONS})
add_test(NAME network_tests COMMAND test_network)

# Lightning tests
add_executable(test_lightning test_lightning.cpp)
target_link_libraries(test_lightning PRIVATE intcoin_core)
target_compile_definitions(test_lightning PRIVATE ${TEST_DEFINITIONS})
target_compile_options(test_lightning PRIVATE ${TEST_COMPILE_OPTIONS})
add_test(NAME lightning_tests COMMAND test_lightning)

# TOR tests
add_executable(test_tor test_tor.cpp)
target_link_libraries(test_tor PRIVATE intcoin_core)
target_compile_definitions(test_tor PRIVATE ${TEST_DEFINITIONS})
target_compile_options(test_tor PRIVATE ${TEST_COMPILE_OPTIONS})
add_test(NAME tor_tests COMMAND test_tor)

# Security tests
add_executable(test_security test_security.cpp)
target_link_libraries(test_security PRIVATE intcoin_core)
target_compile_definitions(test_security PRIVATE ${TEST_DEFINITIONS})
target_compile_options(test_security PRIVATE ${TEST_COMPILE_OPTIONS})
add_test(NAME security_tests COMMAND test_security)

# Dilithium NIST verification tests
add_executable(test_dilithium_nist test_dilithium_nist.cpp)
target_link_libraries(test_dilithium_nist PRIVATE intcoin_core)
target_compile_definitions(test_dilithium_nist PRIVATE ${TEST_DEFINITIONS})
target_compile_options(test_dilithium_nist PRIVATE ${TEST_COMPILE_OPTIONS})
add_test(NAME dilithium_nist_tests COMMAND test_dilithium_nist)

# Test coverage (optional, requires gcov/lcov)
if(ENABLE_COVERAGE)
    include(CodeCoverage)
    append_coverage_compiler_flags()
    setup_target_for_coverage_lcov(
        NAME coverage
        EXECUTABLE ctest
        EXCLUDE "/usr/*" "*/tests/*" "*/external/*"
    )
endif()

# Python tests (to be added later)
# find_package(Python3 COMPONENTS Interpreter REQUIRED)
# add_test(NAME python_tests COMMAND ${Python3_EXECUTABLE} -m pytest ${CMAKE_SOURCE_DIR}/tests)
