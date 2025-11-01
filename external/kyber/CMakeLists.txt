# Copyright (c) 2025 INTcoin Core (Maddison Lane)
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
#
# CRYSTALS-Kyber post-quantum KEM

cmake_minimum_required(VERSION 3.20)
project(kyber C)

# Note: This is a placeholder. In production, you should:
# 1. Git submodule: https://github.com/pq-crystals/kyber
# 2. Or use liboqs: https://github.com/open-quantum-safe/liboqs

add_library(kyber STATIC
    stub.c
)

target_include_directories(kyber PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# TODO: Replace with actual CRYSTALS-Kyber implementation
