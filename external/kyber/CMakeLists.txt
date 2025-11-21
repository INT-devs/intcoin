# Copyright (c) 2025 INTcoin Core (Maddison Lane)
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
#
# CRYSTALS-Kyber post-quantum KEM

cmake_minimum_required(VERSION 3.20)
project(kyber C)

# Check if liboqs is available (preferred method)
find_package(liboqs QUIET)

if(liboqs_FOUND)
    message(STATUS "Found liboqs - using Open Quantum Safe implementation")
    add_library(kyber INTERFACE)
    target_link_libraries(kyber INTERFACE OQS::oqs)
    target_compile_definitions(kyber INTERFACE INTCOIN_USE_LIBOQS)
else()
    message(STATUS "liboqs not found - using reference implementation")

    # Check for git submodule (NIST reference implementation)
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/ref/kem.c)
        message(STATUS "Found CRYSTALS-Kyber reference implementation")

        # Build reference implementation (Kyber1024)
        add_library(kyber STATIC
            ref/kem.c
            ref/indcpa.c
            ref/polyvec.c
            ref/poly.c
            ref/ntt.c
            ref/cbd.c
            ref/reduce.c
            ref/verify.c
            ref/symmetric-shake.c
            ref/fips202.c
        )

        target_include_directories(kyber PUBLIC
            ${CMAKE_CURRENT_SOURCE_DIR}/ref
            ${CMAKE_CURRENT_SOURCE_DIR}/include
        )

        target_compile_definitions(kyber PUBLIC
            KYBER_K=4
            INTCOIN_USE_KYBER_REF
        )
    else()
        # Fallback: Use stub implementation with clear warning
        message(WARNING "CRYSTALS-Kyber implementation not found!")
        message(WARNING "To use production implementation, either:")
        message(WARNING "  1. Install liboqs: https://github.com/open-quantum-safe/liboqs")
        message(WARNING "  2. Or clone reference: git submodule add https://github.com/pq-crystals/kyber external/kyber/ref")
        message(WARNING "Using STUB implementation - NOT SECURE FOR PRODUCTION!")

        add_library(kyber STATIC
            stub.c
        )

        target_include_directories(kyber PUBLIC
            ${CMAKE_CURRENT_SOURCE_DIR}/include
        )

        target_compile_definitions(kyber PUBLIC
            INTCOIN_USE_KYBER_STUB
        )
    endif()
endif()

# Common compile options
if(NOT liboqs_FOUND)
    target_compile_options(kyber PRIVATE
        -Wall
        -Wextra
        -O3
        -march=native
    )
endif()
