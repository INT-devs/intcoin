#!/bin/bash
# INTcoin - liboqs Installation Script for Ubuntu/Debian
# Version: 1.0.0
# Last Updated: November 15, 2025
#
# This script builds and installs liboqs (Open Quantum Safe library)
# from source on Ubuntu/Debian-based distributions.
#
# Usage:
#   sudo ./scripts/install_liboqs_debian.sh
#
# Or for non-interactive mode:
#   sudo SKIP_CONFIRM=1 ./scripts/install_liboqs_debian.sh

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

echo -e "${BLUE}╔════════════════════════════════════════════════════════════════╗${NC}"
echo -e "${BLUE}║          INTcoin - liboqs Installation (Debian/Ubuntu)         ║${NC}"
echo -e "${BLUE}║                    Version ${LIBOQS_VERSION}                                  ║${NC}"
echo -e "${BLUE}╚════════════════════════════════════════════════════════════════╝${NC}"
echo

# Check if running as root
if [[ $EUID -ne 0 ]]; then
   echo -e "${RED}Error: This script must be run as root (use sudo)${NC}"
   exit 1
fi

# Detect distribution
if [ -f /etc/os-release ]; then
    . /etc/os-release
    OS_NAME=$NAME
    OS_VERSION=$VERSION_ID
    echo -e "${GREEN}✓${NC} Detected: $OS_NAME $OS_VERSION"
else
    echo -e "${RED}✗${NC} Cannot detect distribution"
    exit 1
fi

# Check if already installed
if pkg-config --exists liboqs 2>/dev/null; then
    INSTALLED_VERSION=$(pkg-config --modversion liboqs)
    echo -e "${YELLOW}⚠${NC}  liboqs ${INSTALLED_VERSION} is already installed"

    if [ -z "$SKIP_CONFIRM" ]; then
        read -p "Do you want to reinstall? (y/N): " -n 1 -r
        echo
        if [[ ! $REPLY =~ ^[Yy]$ ]]; then
            echo "Installation cancelled."
            exit 0
        fi
    fi
fi

echo
echo -e "${BLUE}► Installing build dependencies...${NC}"

# Update package list
apt-get update -qq

# Install required dependencies
DEPENDENCIES=(
    build-essential
    cmake
    git
    libssl-dev
    pkg-config
    ninja-build
    astyle
    python3
    python3-pytest
    python3-pytest-xdist
    unzip
    xsltproc
    doxygen
    graphviz
)

echo -e "${GREEN}Installing:${NC} ${DEPENDENCIES[*]}"
apt-get install -y "${DEPENDENCIES[@]}"

echo
echo -e "${BLUE}► Downloading liboqs ${LIBOQS_VERSION}...${NC}"

# Create build directory
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Download source
echo "Downloading from: $LIBOQS_URL"
curl -L "$LIBOQS_URL" -o liboqs.tar.gz

# Extract
echo "Extracting..."
tar -xzf liboqs.tar.gz
cd "liboqs-${LIBOQS_VERSION}"

echo
echo -e "${BLUE}► Building liboqs...${NC}"

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
    ..

# Build
echo "Compiling (this may take a few minutes)..."
ninja

# Run tests (optional, can be skipped with SKIP_TESTS=1)
if [ -z "$SKIP_TESTS" ]; then
    echo
    echo -e "${BLUE}► Running tests...${NC}"
    ninja run_tests || {
        echo -e "${YELLOW}⚠${NC}  Some tests failed, but continuing installation..."
    }
fi

echo
echo -e "${BLUE}► Installing liboqs...${NC}"

# Install
ninja install

# Update library cache
ldconfig

echo
echo -e "${GREEN}╔════════════════════════════════════════════════════════════════╗${NC}"
echo -e "${GREEN}║                   Installation Successful!                     ║${NC}"
echo -e "${GREEN}╚════════════════════════════════════════════════════════════════╝${NC}"

# Verify installation
if pkg-config --exists liboqs; then
    INSTALLED_VERSION=$(pkg-config --modversion liboqs)
    echo -e "${GREEN}✓${NC} liboqs ${INSTALLED_VERSION} installed successfully"
    echo -e "${GREEN}✓${NC} Library location: $(pkg-config --variable=libdir liboqs)"
    echo -e "${GREEN}✓${NC} Include location: $(pkg-config --variable=includedir liboqs)"
else
    echo -e "${RED}✗${NC} Installation verification failed"
    exit 1
fi

# Cleanup
echo
echo -e "${BLUE}► Cleaning up...${NC}"
cd /
rm -rf "$BUILD_DIR"
echo -e "${GREEN}✓${NC} Build files removed"

echo
echo -e "${GREEN}Installation complete!${NC}"
echo
echo "You can now build INTcoin with:"
echo "  cmake -S . -B build && cmake --build build -j\$(nproc)"
echo

exit 0
