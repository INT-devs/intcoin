# Copyright (c) 2025 INTcoin Core (Maddison Lane)
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
#
# RandomX ASIC-resistant PoW algorithm

cmake_minimum_required(VERSION 3.20)
project(randomx CXX)

# Note: This is a placeholder. In production, you should:
# Git submodule: https://github.com/tevador/RandomX

add_library(randomx STATIC
    stub.cpp
)

target_include_directories(randomx PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# TODO: Replace with actual RandomX implementation
# Official repo: https://github.com/tevador/RandomX
