# Copyright (c) 2025 INTcoin Core (Maddison Lane)
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

# Crypto module

add_library(crypto OBJECT
    sha3_impl.cpp
    sha256_impl.cpp
    dilithium.cpp
    kyber.cpp
    address.cpp
    random.cpp
    hkdf.cpp
    mnemonic.cpp
    secure_memory.cpp
)

target_include_directories(crypto
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_BINARY_DIR}/include
)

target_link_libraries(crypto
    PUBLIC
        OpenSSL::Crypto
)

# Find and link liboqs for post-quantum cryptography
find_library(LIBOQS_LIBRARY NAMES oqs liboqs
    HINTS
        /usr/local/lib
        /opt/homebrew/lib
        /usr/lib
)

if(LIBOQS_LIBRARY)
    message(STATUS "Found liboqs: ${LIBOQS_LIBRARY}")
    target_link_libraries(crypto PUBLIC ${LIBOQS_LIBRARY})

    # Find include directory
    find_path(LIBOQS_INCLUDE_DIR oqs/oqs.h
        HINTS
            /usr/local/include
            /opt/homebrew/include
            /usr/include
    )

    if(LIBOQS_INCLUDE_DIR)
        target_include_directories(crypto PUBLIC ${LIBOQS_INCLUDE_DIR})
    endif()
else()
    message(WARNING "liboqs not found - quantum-resistant crypto will not be available")
    message(WARNING "Install from: https://github.com/open-quantum-safe/liboqs")
endif()
