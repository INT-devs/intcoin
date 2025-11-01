#!/bin/sh
# Copyright (c) 2025 INTcoin Core (Maddison Lane)
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

# INTcoin FreeBSD Build Script
# Usage: ./build-freebsd.sh [options]

set -e

# Default options
BUILD_TYPE="Release"
CLEAN=0
WITH_TESTS=1
WITH_QT=1
INSTALL=0
CREATE_PACKAGE=0
JOBS=$(sysctl -n hw.ncpu)

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# Print colored message
print_step() {
    printf "${CYAN}[$1/$2] $3${NC}\n"
}

print_success() {
    printf "${GREEN}✓ $1${NC}\n"
}

print_warning() {
    printf "${YELLOW}⚠ $1${NC}\n"
}

print_error() {
    printf "${RED}✗ $1${NC}\n"
}

# Usage
usage() {
    cat << EOF
INTcoin FreeBSD Build Script

Usage: $0 [OPTIONS]

Options:
    -c, --clean           Clean build directory before building
    -d, --debug           Build in Debug mode (default: Release)
    -t, --no-tests        Skip building tests
    -q, --no-qt           Build without Qt GUI wallet
    -i, --install         Install after building
    -p, --package         Create TGZ package
    -j N, --jobs N        Use N parallel jobs (default: $(sysctl -n hw.ncpu))
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
while [ $# -gt 0 ]; do
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

printf "${CYAN}================================${NC}\n"
printf "${CYAN}INTcoin FreeBSD Build Script${NC}\n"
printf "${CYAN}================================${NC}\n"
printf "\n"

# Check prerequisites
print_step 1 7 "Checking prerequisites..."

if ! command -v cmake > /dev/null 2>&1; then
    print_error "CMake not found. Please install: pkg install cmake"
    exit 1
fi

if ! command -v git > /dev/null 2>&1; then
    print_error "Git not found. Please install: pkg install git"
    exit 1
fi

if ! command -v gmake > /dev/null 2>&1; then
    print_error "gmake not found. Please install: pkg install gmake"
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

printf "\n"

# Clean if requested
if [ $CLEAN -eq 1 ] && [ -d "build" ]; then
    print_step 2 7 "Cleaning build directory..."
    rm -rf build
    print_success "Build directory cleaned"
else
    print_step 2 7 "Skipping clean (use --clean to clean)"
fi
printf "\n"

# Create build directory
print_step 3 7 "Creating build directory..."
mkdir -p build
print_success "Build directory ready"
printf "\n"

# Configure
print_step 4 7 "Configuring with CMake..."
printf "${CYAN}Build type: $BUILD_TYPE${NC}\n"
printf "${CYAN}Qt GUI: "
[ $WITH_QT -eq 1 ] && printf "Enabled" || printf "Disabled"
printf "${NC}\n"
printf "${CYAN}Tests: "
[ $WITH_TESTS -eq 1 ] && printf "Enabled" || printf "Disabled"
printf "${NC}\n"

cd build

QT_FLAG="OFF"
[ $WITH_QT -eq 1 ] && QT_FLAG="ON"

TEST_FLAG="OFF"
[ $WITH_TESTS -eq 1 ] && TEST_FLAG="ON"

cmake .. \
    -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
    -DBUILD_QT_WALLET=$QT_FLAG \
    -DBUILD_TESTS=$TEST_FLAG \
    -DBUILD_DAEMON=ON \
    -DBUILD_CLI=ON \
    -DBUILD_MINER=ON \
    -DCMAKE_PREFIX_PATH=/usr/local

if [ $? -ne 0 ]; then
    print_error "CMake configuration failed"
    exit 1
fi

print_success "Configuration complete"
printf "\n"

# Build
print_step 5 7 "Building ($BUILD_TYPE configuration)..."
printf "${CYAN}Using $JOBS parallel jobs${NC}\n"

gmake -j$JOBS

if [ $? -ne 0 ]; then
    print_error "Build failed"
    exit 1
fi

print_success "Build complete"
printf "\n"

# Run tests
if [ $WITH_TESTS -eq 1 ]; then
    print_step 6 7 "Running tests..."
    ctest || print_warning "Some tests failed"
else
    print_step 6 7 "Skipping tests"
fi
printf "\n"

# Install or package
if [ $INSTALL -eq 1 ]; then
    print_step 7 7 "Installing..."
    gmake install
    print_success "Installation complete"
elif [ $CREATE_PACKAGE -eq 1 ]; then
    print_step 7 7 "Creating package..."
    cpack -G TGZ
    print_success "TGZ package created"
else
    print_step 7 7 "Skipping installation (use --install or --package)"
fi

cd ..

printf "\n"
printf "${CYAN}================================${NC}\n"
printf "${CYAN}Build Summary${NC}\n"
printf "${CYAN}================================${NC}\n"
printf "Build type: ${BUILD_TYPE}\n"
printf "Build directory: build/\n"
printf "\n"
printf "${YELLOW}Executables:${NC}\n"
printf "  - build/intcoind (Daemon)\n"
printf "  - build/intcoin-cli (CLI)\n"
[ $WITH_QT -eq 1 ] && printf "  - build/intcoin-qt (GUI Wallet)\n"
printf "  - build/intcoin-miner (CPU Miner)\n"
printf "\n"

if [ $INSTALL -eq 1 ]; then
    printf "${YELLOW}Installation:${NC}\n"
    printf "  Binaries: /usr/local/bin/\n"
    printf "  Config: /usr/local/share/intcoin/\n"
    printf "\n"
fi

if [ $CREATE_PACKAGE -eq 1 ]; then
    printf "${YELLOW}Package:${NC}\n"
    ls -1 build/INTcoin-*.tar.gz 2>/dev/null || true
    printf "\n"
fi

printf "${YELLOW}To run:${NC}\n"
[ $WITH_QT -eq 1 ] && printf "  ${CYAN}./build/intcoin-qt${NC}\n"
printf "  ${CYAN}./build/intcoind${NC}\n"
printf "\n"
print_success "Build completed successfully!"
