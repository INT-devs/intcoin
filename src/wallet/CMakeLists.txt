# Copyright (c) 2025 INTcoin Core (Maddison Lane)
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

# Wallet module
add_library(wallet OBJECT
    wallet.cpp
)

target_include_directories(wallet
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_BINARY_DIR}/include
)

find_package(RocksDB REQUIRED)

target_link_libraries(wallet
    PUBLIC
        RocksDB::rocksdb
)
