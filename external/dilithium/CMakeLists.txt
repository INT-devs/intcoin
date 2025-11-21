# Copyright (c) 2025 INTcoin Core (Maddison Lane)
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
#
# CRYSTALS-Dilithium post-quantum signatures

cmake_minimum_required(VERSION 3.20)
project(dilithium C)

# Check if liboqs is available (preferred method)
find_package(liboqs QUIET)

if(liboqs_FOUND)
    message(STATUS "Found liboqs - using Open Quantum Safe implementation")
    add_library(dilithium INTERFACE)
    target_link_libraries(dilithium INTERFACE OQS::oqs)
    target_compile_definitions(dilithium INTERFACE INTCOIN_USE_LIBOQS)
else()
    message(STATUS "liboqs not found - using reference implementation")

    # Check for git submodule (NIST reference implementation)
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/ref/sign.c)
        message(STATUS "Found CRYSTALS-Dilithium reference implementation")

        # Build reference implementation (Dilithium5)
        add_library(dilithium STATIC
            ref/sign.c
            ref/packing.c
            ref/polyvec.c
            ref/poly.c
            ref/ntt.c
            ref/reduce.c
            ref/rounding.c
            ref/symmetric-shake.c
            ref/fips202.c
        )

        target_include_directories(dilithium PUBLIC
            ${CMAKE_CURRENT_SOURCE_DIR}/ref
            ${CMAKE_CURRENT_SOURCE_DIR}/include
        )

        target_compile_definitions(dilithium PUBLIC
            DILITHIUM_MODE=5
            INTCOIN_USE_DILITHIUM_REF
        )
    else()
        # Fallback: Use stub implementation with clear warning
        message(WARNING "CRYSTALS-Dilithium implementation not found!")
        message(WARNING "To use production implementation, either:")
        message(WARNING "  1. Install liboqs: https://github.com/open-quantum-safe/liboqs")
        message(WARNING "  2. Or clone reference: git submodule add https://github.com/pq-crystals/dilithium external/dilithium/ref")
        message(WARNING "Using STUB implementation - NOT SECURE FOR PRODUCTION!")

        add_library(dilithium STATIC
            stub.c
        )

        target_include_directories(dilithium PUBLIC
            ${CMAKE_CURRENT_SOURCE_DIR}/include
        )

        target_compile_definitions(dilithium PUBLIC
            INTCOIN_USE_DILITHIUM_STUB
        )
    endif()
endif()

# Common compile options
if(NOT liboqs_FOUND)
    target_compile_options(dilithium PRIVATE
        -Wall
        -Wextra
        -O3
        -march=native
    )
endif()
