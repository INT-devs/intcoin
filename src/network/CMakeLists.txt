# Copyright (c) 2025 INTcoin Core (Maddison Lane)
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

# Network module

add_library(network OBJECT
    placeholder.cpp
    p2p.cpp
    # net.cpp
    # peer.cpp
    # protocol.cpp
    # Add more as implemented
)

target_include_directories(network
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_BINARY_DIR}/include
)

find_package(RocksDB REQUIRED)

target_link_libraries(network
    PUBLIC
        RocksDB::rocksdb
)
