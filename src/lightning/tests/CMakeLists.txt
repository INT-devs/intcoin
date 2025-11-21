# Lightning Network Tests
# Copyright (c) 2025 INTcoin Core (Maddison Lane)

# Test executable
add_executable(test_lightning
    test_lightning.cpp
    test_channels.cpp
    test_htlc.cpp
    test_routing.cpp
    test_invoice.cpp
    test_watchtower.cpp
    test_amp.cpp
    test_ptlc.cpp
    test_eltoo.cpp
)

target_link_libraries(test_lightning
    intcoin_lightning
    intcoin_crypto
    intcoin_core
    ${OPENSSL_LIBRARIES}
    ${Boost_LIBRARIES}
)

target_include_directories(test_lightning PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
)

# Register tests with CTest
add_test(NAME LightningTests COMMAND test_lightning)

# Individual test targets for granular testing
add_test(NAME Lightning_Channels COMMAND test_lightning --channels)
add_test(NAME Lightning_HTLC COMMAND test_lightning --htlc)
add_test(NAME Lightning_Routing COMMAND test_lightning --routing)
add_test(NAME Lightning_Invoice COMMAND test_lightning --invoice)
add_test(NAME Lightning_Watchtower COMMAND test_lightning --watchtower)
add_test(NAME Lightning_AMP COMMAND test_lightning --amp)
add_test(NAME Lightning_PTLC COMMAND test_lightning --ptlc)
add_test(NAME Lightning_Eltoo COMMAND test_lightning --eltoo)
