#!/usr/bin/env bash
# INTcoin Windows Cross-Compilation Script (Linux -> Windows)
# Copyright (c) 2025 INTcoin Team (Neil Adamson)
# SPDX-License-Identifier: MIT License

set -e  # Exit on error
set -u  # Exit on undefined variable

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuration
INSTALL_PREFIX="${INSTALL_PREFIX:-/usr/x86_64-w64-mingw32}"
BUILD_JOBS="${BUILD_JOBS:-$(nproc)}"
BUILD_TYPE="${BUILD_TYPE:-Release}"
TEMP_DIR="/tmp/intcoin-cross-build-$$"

# MinGW configuration
MINGW_PREFIX="x86_64-w64-mingw32"
MINGW_HOST="${MINGW_PREFIX}"
TOOLCHAIN_FILE="$(pwd)/cmake/toolchain-mingw-w64.cmake"

# Version requirements
LIBOQS_VERSION="0.15.0"
RANDOMX_VERSION="v1.2.1"

# Print banner
print_banner() {
    echo -e "${BLUE}"
    echo "╔═══════════════════════════════════════════════════════╗"
    echo "║    INTcoin Windows Cross-Compilation Script          ║"
    echo "║                                                       ║"
    echo "║  Build Windows .exe from Linux using MinGW-w64       ║"
    echo "╚═══════════════════════════════════════════════════════╝"
    echo -e "${NC}"
}

# Print status message
info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

# Print success message
success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

# Print warning message
warn() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

# Print error message and exit
error() {
    echo -e "${RED}[ERROR]${NC} $1"
    exit 1
}

# Install MinGW cross-compilation toolchain
install_mingw() {
    info "Installing MinGW-w64 toolchain..."

    # Detect distribution
    if [ -f /etc/os-release ]; then
        . /etc/os-release
        DISTRO=$ID
    else
        error "Cannot detect Linux distribution"
    fi

    case "$DISTRO" in
        ubuntu|debian)
            sudo apt-get update
            sudo apt-get install -y \
                mingw-w64 \
                g++-mingw-w64-x86-64 \
                gcc-mingw-w64-x86-64 \
                binutils-mingw-w64-x86-64 \
                cmake \
                git \
                wget \
                pkg-config \
                autoconf \
                automake \
                libtool \
                nsis
            ;;

        fedora|rhel|centos)
            sudo dnf install -y \
                mingw64-gcc \
                mingw64-gcc-c++ \
                mingw64-binutils \
                mingw64-headers \
                cmake \
                git \
                wget \
                pkg-config \
                autoconf \
                automake \
                libtool \
                nsis
            ;;

        arch|manjaro)
            sudo pacman -Sy --noconfirm \
                mingw-w64-gcc \
                cmake \
                git \
                wget \
                pkg-config \
                autoconf \
                automake \
                libtool \
                nsis
            ;;

        *)
            error "Unsupported distribution: $DISTRO"
            ;;
    esac

    success "MinGW toolchain installed"
}

# Create CMake toolchain file
create_toolchain_file() {
    info "Creating CMake toolchain file..."

    mkdir -p cmake

    cat > "$TOOLCHAIN_FILE" <<'EOF'
# CMake toolchain file for cross-compiling to Windows with MinGW-w64

set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

# Specify the cross compiler
set(CMAKE_C_COMPILER x86_64-w64-mingw32-gcc)
set(CMAKE_CXX_COMPILER x86_64-w64-mingw32-g++)
set(CMAKE_RC_COMPILER x86_64-w64-mingw32-windres)

# Target environment
set(CMAKE_FIND_ROOT_PATH /usr/x86_64-w64-mingw32)

# Search for programs in the build host directories
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)

# Search for libraries and headers in the target directories
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

# Windows-specific settings
set(WIN32 TRUE)
set(MINGW TRUE)

# Static linking for Windows
set(CMAKE_EXE_LINKER_FLAGS "-static-libgcc -static-libstdc++ -static")
set(CMAKE_SHARED_LINKER_FLAGS "-static-libgcc -static-libstdc++")
EOF

    success "Toolchain file created: $TOOLCHAIN_FILE"
}

# Cross-compile liboqs
cross_compile_liboqs() {
    info "Cross-compiling liboqs $LIBOQS_VERSION for Windows..."

    cd "$TEMP_DIR"

    if [ ! -d "liboqs" ]; then
        git clone --depth 1 --branch "$LIBOQS_VERSION" \
            https://github.com/open-quantum-safe/liboqs.git
    fi

    cd liboqs
    mkdir -p build-mingw && cd build-mingw

    cmake \
        -DCMAKE_TOOLCHAIN_FILE="$TOOLCHAIN_FILE" \
        -DCMAKE_INSTALL_PREFIX="$INSTALL_PREFIX" \
        -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
        -DBUILD_SHARED_LIBS=OFF \
        -DOQS_BUILD_ONLY_LIB=ON \
        ..

    make -j"$BUILD_JOBS"
    sudo make install

    success "liboqs cross-compiled"
}

# Cross-compile RandomX
cross_compile_randomx() {
    info "Cross-compiling RandomX $RANDOMX_VERSION for Windows..."

    cd "$TEMP_DIR"

    if [ ! -d "RandomX" ]; then
        git clone --depth 1 --branch "$RANDOMX_VERSION" \
            https://github.com/tevador/RandomX.git
    fi

    cd RandomX
    mkdir -p build-mingw && cd build-mingw

    cmake \
        -DCMAKE_TOOLCHAIN_FILE="$TOOLCHAIN_FILE" \
        -DCMAKE_INSTALL_PREFIX="$INSTALL_PREFIX" \
        -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
        ..

    make -j"$BUILD_JOBS"
    sudo make install

    success "RandomX cross-compiled"
}

# Cross-compile INTcoin
cross_compile_intcoin() {
    info "Cross-compiling INTcoin for Windows..."

    # Assume we're running from the intcoin source directory
    INTCOIN_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
    cd "$INTCOIN_DIR"

    # Create build directory
    mkdir -p build-mingw && cd build-mingw

    # Configure
    cmake \
        -DCMAKE_TOOLCHAIN_FILE="$TOOLCHAIN_FILE" \
        -DCMAKE_INSTALL_PREFIX="$INSTALL_PREFIX" \
        -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
        -DBUILD_TESTING=OFF \
        ..

    # Build
    make -j"$BUILD_JOBS"

    success "INTcoin cross-compiled"

    # Copy binaries
    info "Binaries located at: build-mingw/"
    ls -lh build-mingw/*.exe 2>/dev/null || warn "No .exe files found"
}

# Create Windows installer package
create_windows_package() {
    info "Creating Windows installer package..."

    INTCOIN_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
    BUILD_DIR="$INTCOIN_DIR/build-mingw"
    PACKAGE_DIR="$BUILD_DIR/package"

    # Create package directory structure
    mkdir -p "$PACKAGE_DIR/bin"
    mkdir -p "$PACKAGE_DIR/conf"
    mkdir -p "$PACKAGE_DIR/doc"

    # Copy binaries
    if [ -f "$BUILD_DIR/intcoind.exe" ]; then
        cp "$BUILD_DIR/intcoind.exe" "$PACKAGE_DIR/bin/"
    fi
    if [ -f "$BUILD_DIR/intcoin-cli.exe" ]; then
        cp "$BUILD_DIR/intcoin-cli.exe" "$PACKAGE_DIR/bin/"
    fi
    if [ -f "$BUILD_DIR/intcoin-miner.exe" ]; then
        cp "$BUILD_DIR/intcoin-miner.exe" "$PACKAGE_DIR/bin/"
    fi

    # Copy MinGW runtime DLLs
    MINGW_LIB="/usr/x86_64-w64-mingw32/lib"
    if [ -d "$MINGW_LIB" ]; then
        cp "$MINGW_LIB"/*.dll "$PACKAGE_DIR/bin/" 2>/dev/null || true
    fi

    # Create configuration template
    cat > "$PACKAGE_DIR/conf/intcoin.conf" <<'EOF'
# INTcoin Configuration File

# Network settings
#testnet=0
#port=9333
#rpcport=9332

# Connection settings
#rpcuser=intcoinrpc
#rpcpassword=changeme
#rpcallowip=127.0.0.1

# Data directory
#datadir=C:\Users\USERNAME\AppData\Roaming\INTcoin

# Daemon settings
#daemon=1
#server=1

# Mining
#gen=0
#genproclimit=-1

# Logging
#debug=0
#printtoconsole=0
EOF

    # Copy documentation
    if [ -f "$INTCOIN_DIR/README.md" ]; then
        cp "$INTCOIN_DIR/README.md" "$PACKAGE_DIR/doc/"
    fi

    # Create installer batch script
    cat > "$PACKAGE_DIR/install.bat" <<'EOF'
@echo off
echo INTcoin Windows Installer
echo.

set INSTALL_DIR=%ProgramFiles%\INTcoin
set DATA_DIR=%APPDATA%\INTcoin

echo Installing to: %INSTALL_DIR%
echo.

mkdir "%INSTALL_DIR%\bin"
mkdir "%DATA_DIR%"

copy /Y bin\*.exe "%INSTALL_DIR%\bin\"
copy /Y bin\*.dll "%INSTALL_DIR%\bin\" 2>NUL
copy /Y conf\intcoin.conf "%DATA_DIR%\"

echo.
echo Installation complete!
echo.
echo Binaries installed to: %INSTALL_DIR%\bin
echo Configuration file: %DATA_DIR%\intcoin.conf
echo.
echo Add to PATH: setx PATH "%PATH%;%INSTALL_DIR%\bin"
echo.
pause
EOF

    # Create ZIP package
    cd "$BUILD_DIR"
    ZIP_FILE="intcoin-windows-x64-mingw.zip"
    zip -r "$ZIP_FILE" package/

    success "Windows package created: $BUILD_DIR/$ZIP_FILE"
}

# Print summary
print_summary() {
    echo ""
    echo -e "${GREEN}╔═══════════════════════════════════════════════════════╗${NC}"
    echo -e "${GREEN}║          Cross-Compilation Complete!                  ║${NC}"
    echo -e "${GREEN}╚═══════════════════════════════════════════════════════╝${NC}"
    echo ""
    echo -e "${BLUE}Build artifacts:${NC}"
    echo "  - Build directory: build-mingw/"
    echo "  - Package: build-mingw/intcoin-windows-x64-mingw.zip"
    echo ""
    echo -e "${BLUE}Windows binaries:${NC}"
    echo "  - intcoind.exe      : Full node daemon"
    echo "  - intcoin-cli.exe   : Command-line interface"
    echo "  - intcoin-miner.exe : Mining software"
    echo ""
    echo -e "${BLUE}Next steps:${NC}"
    echo "  1. Transfer the ZIP file to Windows"
    echo "  2. Extract and run install.bat"
    echo "  3. Configure intcoin.conf"
    echo "  4. Run intcoind.exe"
    echo ""
    echo -e "${YELLOW}Documentation: https://code.intcoin.org/intcoin/core/wiki${NC}"
    echo ""
}

# Main function
main() {
    print_banner

    # Create temp directory
    mkdir -p "$TEMP_DIR"

    # Build steps
    install_mingw
    create_toolchain_file
    cross_compile_liboqs
    cross_compile_randomx
    cross_compile_intcoin

    # Create package
    create_windows_package

    # Cleanup
    info "Cleaning up temporary files..."
    rm -rf "$TEMP_DIR"

    # Print summary
    print_summary
}

# Run main function
main "$@"
