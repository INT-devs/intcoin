# Copyright (c) 2025 INTcoin Core (Maddison Lane)
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
#
# CRYSTALS-Dilithium post-quantum signatures

cmake_minimum_required(VERSION 3.20)
project(dilithium C)

# Note: This is a placeholder. In production, you should:
# 1. Git submodule: https://github.com/pq-crystals/dilithium
# 2. Or use liboqs: https://github.com/open-quantum-safe/liboqs

# For now, create a stub library
add_library(dilithium STATIC
    stub.c
)

target_include_directories(dilithium PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# TODO: Replace with actual CRYSTALS-Dilithium implementation
# Recommended: Use reference implementation from NIST or liboqs
