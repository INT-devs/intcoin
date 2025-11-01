#!/bin/bash
# Copyright (c) 2025 INTcoin Core (Maddison Lane)
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

# INTcoin Linux Build Script
# Usage: ./build-linux.sh [options]

set -e

# Default options
BUILD_TYPE="Release"
CLEAN=0
WITH_TESTS=1
WITH_QT=1
INSTALL=0
CREATE_PACKAGE=0
JOBS=$(nproc)

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# Print colored message
print_step() {
    echo -e "${CYAN}[$1/$2] $3${NC}"
}

print_success() {
    echo -e "${GREEN}✓ $1${NC}"
}

print_warning() {
    echo -e "${YELLOW}⚠ $1${NC}"
}

print_error() {
    echo -e "${RED}✗ $1${NC}"
}

# Usage
usage() {
    cat << EOF
INTcoin Linux Build Script

Usage: $0 [OPTIONS]

Options:
    -c, --clean           Clean build directory before building
    -d, --debug           Build in Debug mode (default: Release)
    -t, --no-tests        Skip building tests
    -q, --no-qt           Build without Qt GUI wallet
    -i, --install         Install after building
    -p, --package         Create .deb or .rpm package
    -j N, --jobs N        Use N parallel jobs (default: $(nproc))
    -h, --help            Show this help message

Examples:
    $0                    # Standard release build
    $0 --clean            # Clean build
    $0 --debug --no-qt    # Debug build without GUI
    $0 --package          # Build and create package

EOF
    exit 0
}

# Parse arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        -c|--clean)
            CLEAN=1
            shift
            ;;
        -d|--debug)
            BUILD_TYPE="Debug"
            shift
            ;;
        -t|--no-tests)
            WITH_TESTS=0
            shift
            ;;
        -q|--no-qt)
            WITH_QT=0
            shift
            ;;
        -i|--install)
            INSTALL=1
            shift
            ;;
        -p|--package)
            CREATE_PACKAGE=1
            shift
            ;;
        -j|--jobs)
            JOBS="$2"
            shift 2
            ;;
        -h|--help)
            usage
            ;;
        *)
            echo "Unknown option: $1"
            usage
            ;;
    esac
done

echo -e "${CYAN}================================${NC}"
echo -e "${CYAN}INTcoin Linux Build Script${NC}"
echo -e "${CYAN}================================${NC}"
echo ""

# Check prerequisites
print_step 1 7 "Checking prerequisites..."

if ! command -v cmake &> /dev/null; then
    print_error "CMake not found. Please install CMake 3.20 or later."
    exit 1
fi

if ! command -v git &> /dev/null; then
    print_error "Git not found. Please install Git."
    exit 1
fi

CMAKE_VERSION=$(cmake --version | head -n1 | cut -d' ' -f3)
print_success "CMake $CMAKE_VERSION found"

if [ $WITH_QT -eq 1 ]; then
    if ! pkg-config --exists Qt5Core 2>/dev/null; then
        print_warning "Qt5 not found. GUI wallet will be disabled."
        WITH_QT=0
    else
        QT_VERSION=$(pkg-config --modversion Qt5Core)
        print_success "Qt $QT_VERSION found"
    fi
fi

echo ""

# Clean if requested
if [ $CLEAN -eq 1 ] && [ -d "build" ]; then
    print_step 2 7 "Cleaning build directory..."
    rm -rf build
    print_success "Build directory cleaned"
else
    print_step 2 7 "Skipping clean (use --clean to clean)"
fi
echo ""

# Create build directory
print_step 3 7 "Creating build directory..."
mkdir -p build
print_success "Build directory ready"
echo ""

# Configure
print_step 4 7 "Configuring with CMake..."
echo -e "${CYAN}Build type: $BUILD_TYPE${NC}"
echo -e "${CYAN}Qt GUI: $([ $WITH_QT -eq 1 ] && echo 'Enabled' || echo 'Disabled')${NC}"
echo -e "${CYAN}Tests: $([ $WITH_TESTS -eq 1 ] && echo 'Enabled' || echo 'Disabled')${NC}"

cd build

cmake .. \
    -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
    -DBUILD_QT_WALLET=$([ $WITH_QT -eq 1 ] && echo "ON" || echo "OFF") \
    -DBUILD_TESTS=$([ $WITH_TESTS -eq 1 ] && echo "ON" || echo "OFF") \
    -DBUILD_DAEMON=ON \
    -DBUILD_CLI=ON \
    -DBUILD_MINER=ON

if [ $? -ne 0 ]; then
    print_error "CMake configuration failed"
    exit 1
fi

print_success "Configuration complete"
echo ""

# Build
print_step 5 7 "Building ($BUILD_TYPE configuration)..."
echo -e "${CYAN}Using $JOBS parallel jobs${NC}"

make -j$JOBS

if [ $? -ne 0 ]; then
    print_error "Build failed"
    exit 1
fi

print_success "Build complete"
echo ""

# Run tests
if [ $WITH_TESTS -eq 1 ]; then
    print_step 6 7 "Running tests..."
    make test || print_warning "Some tests failed"
else
    print_step 6 7 "Skipping tests"
fi
echo ""

# Install or package
if [ $INSTALL -eq 1 ]; then
    print_step 7 7 "Installing..."
    sudo make install
    print_success "Installation complete"
elif [ $CREATE_PACKAGE -eq 1 ]; then
    print_step 7 7 "Creating package..."

    # Detect package type
    if command -v dpkg &> /dev/null; then
        cpack -G DEB
        print_success "Debian package created"
    elif command -v rpm &> /dev/null; then
        cpack -G RPM
        print_success "RPM package created"
    else
        cpack -G TGZ
        print_success "TGZ package created"
    fi
else
    print_step 7 7 "Skipping installation (use --install or --package)"
fi

cd ..

echo ""
echo -e "${CYAN}================================${NC}"
echo -e "${CYAN}Build Summary${NC}"
echo -e "${CYAN}================================${NC}"
echo -e "Build type: ${BUILD_TYPE}"
echo -e "Build directory: build/"
echo ""
echo -e "${YELLOW}Executables:${NC}"
echo -e "  - build/intcoind (Daemon)"
echo -e "  - build/intcoin-cli (CLI)"
[ $WITH_QT -eq 1 ] && echo -e "  - build/intcoin-qt (GUI Wallet)"
echo -e "  - build/intcoin-miner (CPU Miner)"
echo ""

if [ $INSTALL -eq 1 ]; then
    echo -e "${YELLOW}Installation:${NC}"
    echo -e "  Binaries: /usr/local/bin/"
    echo -e "  Config: /usr/local/share/intcoin/"
    echo ""
fi

if [ $CREATE_PACKAGE -eq 1 ]; then
    echo -e "${YELLOW}Package:${NC}"
    ls -1 build/INTcoin-*.{deb,rpm,tar.gz} 2>/dev/null || true
    echo ""
fi

echo -e "${YELLOW}To run:${NC}"
[ $WITH_QT -eq 1 ] && echo -e "  ${CYAN}./build/intcoin-qt${NC}"
echo -e "  ${CYAN}./build/intcoind${NC}"
echo ""
print_success "Build completed successfully!"
