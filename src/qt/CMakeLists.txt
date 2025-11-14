# Copyright (c) 2025 INTcoin Core (Maddison Lane)
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

# Qt GUI Wallet

set(QT_SOURCES
    main.cpp
    mainwindow.cpp
    # Lightning window disabled until backend implementation is complete
    # lightningwindow.cpp
    # lightning_utils.cpp
)

set(QT_HEADERS
    mainwindow.h
    # Lightning window disabled until backend implementation is complete
    # lightningwindow.h
    # lightning_utils.h
)

set(QT_RESOURCES
    intcoin.qrc
)

# MOC the headers (Qt5/Qt6 compatible)
if(QT_VERSION_MAJOR EQUAL 6)
    qt6_wrap_cpp(QT_MOC_SOURCES ${QT_HEADERS})
    qt6_add_resources(QT_RESOURCE_SOURCES ${QT_RESOURCES})
else()
    qt5_wrap_cpp(QT_MOC_SOURCES ${QT_HEADERS})
    qt5_add_resources(QT_RESOURCE_SOURCES ${QT_RESOURCES})
endif()

# Create executable
add_executable(intcoin-qt
    ${QT_SOURCES}
    ${QT_MOC_SOURCES}
    ${QT_RESOURCE_SOURCES}
)

target_include_directories(intcoin-qt
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_BINARY_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}
)

find_package(RocksDB REQUIRED)

# Link Qt libraries (Qt5/Qt6 compatible)
if(QT_VERSION_MAJOR EQUAL 6)
    target_link_libraries(intcoin-qt
        PRIVATE
            intcoin_core
            RocksDB::rocksdb
            Qt6::Core
            Qt6::Widgets
            Qt6::Gui
            Qt6::Network
    )
else()
    target_link_libraries(intcoin-qt
        PRIVATE
            intcoin_core
            RocksDB::rocksdb
            Qt5::Core
            Qt5::Widgets
            Qt5::Gui
            Qt5::Network
    )
endif()

# Install
install(TARGETS intcoin-qt DESTINATION bin)
