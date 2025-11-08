# Copyright (c) 2025 INTcoin Core (Maddison Lane)
# FindRocksDB.cmake - Find RocksDB library
#
# This module defines:
#   RocksDB_FOUND - True if RocksDB is found
#   RocksDB_INCLUDE_DIRS - Include directories for RocksDB
#   RocksDB_LIBRARIES - Libraries to link for RocksDB
#   RocksDB::rocksdb - Imported target for RocksDB

find_path(RocksDB_INCLUDE_DIR
    NAMES rocksdb/db.h
    HINTS
        /usr/include
        /usr/local/include
        /opt/homebrew/include
        $ENV{ROCKSDB_ROOT}/include
)

find_library(RocksDB_LIBRARY
    NAMES rocksdb
    HINTS
        /usr/lib
        /usr/local/lib
        /usr/lib/x86_64-linux-gnu
        /opt/homebrew/lib
        $ENV{ROCKSDB_ROOT}/lib
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(RocksDB
    REQUIRED_VARS RocksDB_LIBRARY RocksDB_INCLUDE_DIR
)

if(RocksDB_FOUND)
    set(RocksDB_LIBRARIES ${RocksDB_LIBRARY})
    set(RocksDB_INCLUDE_DIRS ${RocksDB_INCLUDE_DIR})

    if(NOT TARGET RocksDB::rocksdb)
        add_library(RocksDB::rocksdb UNKNOWN IMPORTED)
        set_target_properties(RocksDB::rocksdb PROPERTIES
            IMPORTED_LOCATION "${RocksDB_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${RocksDB_INCLUDE_DIR}"
        )
    endif()
endif()

mark_as_advanced(RocksDB_INCLUDE_DIR RocksDB_LIBRARY)
