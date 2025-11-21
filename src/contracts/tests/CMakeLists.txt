# Smart Contracts Tests
# Copyright (c) 2025 INTcoin Core (Maddison Lane)
# Note: Smart contract VM implementation is planned for 2026

# Test executable (stub tests for build verification)
add_executable(test_contracts
    test_contracts.cpp
)

target_link_libraries(test_contracts
    intcoin_crypto
    intcoin_core
    ${OPENSSL_LIBRARIES}
    ${Boost_LIBRARIES}
)

target_include_directories(test_contracts PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
)

# Register tests with CTest
add_test(NAME ContractsTests COMMAND test_contracts)
