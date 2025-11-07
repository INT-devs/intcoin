# Copyright (c) 2025 INTcoin Core (Maddison Lane)
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

# Database module

add_library(db OBJECT
    db.cpp
)

target_include_directories(db
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_BINARY_DIR}/include
)

# Find RocksDB
find_package(RocksDB REQUIRED)

target_link_libraries(db
    PUBLIC
        RocksDB::rocksdb
)
