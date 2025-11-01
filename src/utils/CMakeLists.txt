# Copyright (c) 2025 INTcoin Core (Maddison Lane)
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

# Utilities module

add_library(utils OBJECT
    placeholder.cpp
    # util.cpp
    # logging.cpp
    # Add more as implemented
)

target_include_directories(utils
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_BINARY_DIR}/include
)
