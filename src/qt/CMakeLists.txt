# Copyright (c) 2025 INTcoin Core (Maddison Lane)
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

# Qt GUI Wallet

set(QT_SOURCES
    main.cpp
    mainwindow.cpp
)

set(QT_HEADERS
    mainwindow.h
)

# MOC the headers
qt5_wrap_cpp(QT_MOC_SOURCES ${QT_HEADERS})

# Create executable
add_executable(intcoin-qt
    ${QT_SOURCES}
    ${QT_MOC_SOURCES}
)

target_include_directories(intcoin-qt
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_BINARY_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(intcoin-qt
    PRIVATE
        intcoin_core
        Qt5::Core
        Qt5::Widgets
        Qt5::Gui
        Qt5::Network
)

# Install
install(TARGETS intcoin-qt DESTINATION bin)
