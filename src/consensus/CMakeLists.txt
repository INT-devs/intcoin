# Copyright (c) 2025 INTcoin Core (Maddison Lane)
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

# Consensus module

add_library(consensus OBJECT
    placeholder.cpp
    # pow.cpp
    # difficulty.cpp
    # validation.cpp
    # Add more as implemented
)

target_include_directories(consensus
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_BINARY_DIR}/include
)
