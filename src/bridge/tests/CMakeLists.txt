# Cross-Chain Bridge Tests
# Copyright (c) 2025 INTcoin Core (Maddison Lane)
# Note: Bridge implementation is planned for future version

# Test executable (stub tests for build verification)
add_executable(test_bridge
    test_bridge.cpp
)

target_link_libraries(test_bridge
    intcoin_crypto
    intcoin_core
    ${OPENSSL_LIBRARIES}
    ${Boost_LIBRARIES}
)

target_include_directories(test_bridge PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
)

# Register tests with CTest
add_test(NAME BridgeTests COMMAND test_bridge)
