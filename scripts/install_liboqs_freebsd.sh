#!/bin/sh
# INTcoin - liboqs Installation Script for FreeBSD
# Version: 1.0.0
# Last Updated: November 15, 2025
#
# This script builds and installs liboqs (Open Quantum Safe library)
# from source on FreeBSD.
#
# Usage:
#   sudo ./scripts/install_liboqs_freebsd.sh
#
# Or for non-interactive mode:
#   sudo SKIP_CONFIRM=1 ./scripts/install_liboqs_freebsd.sh

set -e  # Exit on error

# Color output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuration
LIBOQS_VERSION="0.10.1"
LIBOQS_URL="https://github.com/open-quantum-safe/liboqs/archive/refs/tags/${LIBOQS_VERSION}.tar.gz"
BUILD_DIR="/tmp/liboqs-build-$$"
INSTALL_PREFIX="/usr/local"

printf "${BLUE}╔════════════════════════════════════════════════════════════════╗${NC}\n"
printf "${BLUE}║            INTcoin - liboqs Installation (FreeBSD)             ║${NC}\n"
printf "${BLUE}║                    Version ${LIBOQS_VERSION}                                  ║${NC}\n"
printf "${BLUE}╚════════════════════════════════════════════════════════════════╝${NC}\n"
printf "\n"

# Check if running as root
if [ "$(id -u)" -ne 0 ]; then
   printf "${RED}Error: This script must be run as root (use sudo)${NC}\n"
   exit 1
fi

# Detect FreeBSD version
FREEBSD_VERSION=$(freebsd-version)
printf "${GREEN}✓${NC} Detected: FreeBSD $FREEBSD_VERSION\n"

# Check if already installed
if pkg-config --exists liboqs 2>/dev/null; then
    INSTALLED_VERSION=$(pkg-config --modversion liboqs)
    printf "${YELLOW}⚠${NC}  liboqs ${INSTALLED_VERSION} is already installed\n"

    if [ -z "$SKIP_CONFIRM" ]; then
        printf "Do you want to reinstall? (y/N): "
        read -r REPLY
        case "$REPLY" in
            [Yy]* ) ;;
            * ) printf "Installation cancelled.\n"; exit 0;;
        esac
    fi
fi

printf "\n"
printf "${BLUE}► Installing build dependencies...${NC}\n"

# Update package repository
pkg update

# Install required dependencies
DEPENDENCIES="cmake ninja openssl pkgconf git python3 py39-pytest"

printf "${GREEN}Installing:${NC} $DEPENDENCIES\n"
pkg install -y $DEPENDENCIES

printf "\n"
printf "${BLUE}► Downloading liboqs ${LIBOQS_VERSION}...${NC}\n"

# Create build directory
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Download source
printf "Downloading from: $LIBOQS_URL\n"
fetch -o liboqs.tar.gz "$LIBOQS_URL"

# Extract
printf "Extracting...\n"
tar -xzf liboqs.tar.gz
cd "liboqs-${LIBOQS_VERSION}"

printf "\n"
printf "${BLUE}► Building liboqs...${NC}\n"

# Create build directory
mkdir -p build
cd build

# Configure with CMake
cmake -GNinja \
    -DCMAKE_INSTALL_PREFIX="$INSTALL_PREFIX" \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_SHARED_LIBS=ON \
    -DOQS_BUILD_ONLY_LIB=ON \
    -DOQS_DIST_BUILD=ON \
    -DOQS_USE_OPENSSL=ON \
    -DOPENSSL_ROOT_DIR=/usr \
    ..

# Build
printf "Compiling (this may take a few minutes)...\n"
ninja

# Run tests (optional, can be skipped with SKIP_TESTS=1)
if [ -z "$SKIP_TESTS" ]; then
    printf "\n"
    printf "${BLUE}► Running tests...${NC}\n"
    ninja run_tests || {
        printf "${YELLOW}⚠${NC}  Some tests failed, but continuing installation...\n"
    }
fi

printf "\n"
printf "${BLUE}► Installing liboqs...${NC}\n"

# Install
ninja install

# Update library cache
ldconfig -m "$INSTALL_PREFIX/lib"

printf "\n"
printf "${GREEN}╔════════════════════════════════════════════════════════════════╗${NC}\n"
printf "${GREEN}║                   Installation Successful!                     ║${NC}\n"
printf "${GREEN}╚════════════════════════════════════════════════════════════════╝${NC}\n"

# Verify installation
if pkg-config --exists liboqs; then
    INSTALLED_VERSION=$(pkg-config --modversion liboqs)
    printf "${GREEN}✓${NC} liboqs ${INSTALLED_VERSION} installed successfully\n"
    printf "${GREEN}✓${NC} Library location: $(pkg-config --variable=libdir liboqs)\n"
    printf "${GREEN}✓${NC} Include location: $(pkg-config --variable=includedir liboqs)\n"
else
    printf "${RED}✗${NC} Installation verification failed\n"
    exit 1
fi

# Cleanup
printf "\n"
printf "${BLUE}► Cleaning up...${NC}\n"
cd /
rm -rf "$BUILD_DIR"
printf "${GREEN}✓${NC} Build files removed\n"

printf "\n"
printf "${GREEN}Installation complete!${NC}\n"
printf "\n"
printf "You can now build INTcoin with:\n"
printf "  cmake -S . -B build && cmake --build build -j$(sysctl -n hw.ncpu)\n"
printf "\n"

exit 0
