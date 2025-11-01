# Copyright (c) 2025 INTcoin Core (Maddison Lane)
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

# Crypto module

add_library(crypto OBJECT
    sha3.cpp
    sha256.cpp
    dilithium.cpp
    kyber.cpp
    address.cpp
    random.cpp
    hkdf.cpp
    mnemonic.cpp
)

target_include_directories(crypto
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_BINARY_DIR}/include
        /opt/homebrew/include  # liboqs on macOS
)

target_link_libraries(crypto
    PUBLIC
        OpenSSL::Crypto
        oqs  # liboqs for quantum-safe crypto
)
