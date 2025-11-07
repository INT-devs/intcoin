# Copyright (c) 2025 INTcoin Core (Maddison Lane)
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

# Core blockchain module

# Core blockchain implementation
add_library(core OBJECT
    block.cpp
    blockchain.cpp
    transaction.cpp
    merkle.cpp
    mempool.cpp
)

target_include_directories(core
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_BINARY_DIR}/include
)

# Link RocksDB for database support
find_package(RocksDB REQUIRED)
target_link_libraries(core
    PUBLIC
        RocksDB::rocksdb
)
