# Copyright (c) 2025 INTcoin Core (Maddison Lane)
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

# RPC server and client module

add_library(rpc OBJECT
    rpc_server.cpp
)

target_include_directories(rpc
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
)

find_package(RocksDB REQUIRED)

target_link_libraries(rpc
    PUBLIC
        network
        RocksDB::rocksdb
)
