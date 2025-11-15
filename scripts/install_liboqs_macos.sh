#!/bin/bash
# INTcoin - liboqs Installation Script for macOS
# Version: 1.0.0
# Last Updated: November 15, 2025
#
# This script builds and installs liboqs (Open Quantum Safe library)
# from source on macOS.
#
# Usage:
#   ./scripts/install_liboqs_macos.sh
#
# Or for non-interactive mode:
#   SKIP_CONFIRM=1 ./scripts/install_liboqs_macos.sh

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

# Detect architecture and set install prefix
ARCH=$(uname -m)
if [ "$ARCH" = "arm64" ]; then
    INSTALL_PREFIX="/opt/homebrew"
    HOMEBREW_PREFIX="/opt/homebrew"
else
    INSTALL_PREFIX="/usr/local"
    HOMEBREW_PREFIX="/usr/local"
fi

echo -e "${BLUE}╔════════════════════════════════════════════════════════════════╗${NC}"
echo -e "${BLUE}║            INTcoin - liboqs Installation (macOS)               ║${NC}"
echo -e "${BLUE}║                    Version ${LIBOQS_VERSION}                                  ║${NC}"
echo -e "${BLUE}╚════════════════════════════════════════════════════════════════╝${NC}"
echo

# Detect macOS version
MACOS_VERSION=$(sw_vers -productVersion)
echo -e "${GREEN}✓${NC} Detected: macOS $MACOS_VERSION ($ARCH)"

# Check if Homebrew is installed
if ! command -v brew &> /dev/null; then
    echo -e "${RED}✗${NC} Homebrew is not installed"
    echo
    echo "Please install Homebrew first:"
    echo "  /bin/bash -c \"\$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)\""
    exit 1
fi

echo -e "${GREEN}✓${NC} Homebrew found at: $(which brew)"

# Check if already installed
if [ -f "$INSTALL_PREFIX/lib/liboqs.dylib" ] || [ -f "$INSTALL_PREFIX/lib/liboqs.a" ]; then
    echo -e "${YELLOW}⚠${NC}  liboqs appears to be already installed"

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
echo -e "${BLUE}► Installing build dependencies via Homebrew...${NC}"

# Install dependencies
DEPENDENCIES=(
    cmake
    ninja
    openssl@3
    pkg-config
    doxygen
    astyle
)

for dep in "${DEPENDENCIES[@]}"; do
    if brew list "$dep" &>/dev/null; then
        echo -e "${GREEN}✓${NC} $dep already installed"
    else
        echo -e "${BLUE}Installing $dep...${NC}"
        brew install "$dep"
    fi
done

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

# Set OpenSSL paths for Homebrew
OPENSSL_ROOT="$HOMEBREW_PREFIX/opt/openssl@3"

# Configure with CMake
cmake -GNinja \
    -DCMAKE_INSTALL_PREFIX="$INSTALL_PREFIX" \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_SHARED_LIBS=ON \
    -DOQS_BUILD_ONLY_LIB=ON \
    -DOQS_DIST_BUILD=ON \
    -DOQS_USE_OPENSSL=ON \
    -DOPENSSL_ROOT_DIR="$OPENSSL_ROOT" \
    -DOPENSSL_CRYPTO_LIBRARY="$OPENSSL_ROOT/lib/libcrypto.dylib" \
    -DOPENSSL_INCLUDE_DIR="$OPENSSL_ROOT/include" \
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

# Install (may need sudo on some systems)
if [ -w "$INSTALL_PREFIX" ]; then
    ninja install
else
    echo "Requesting administrator privileges for installation..."
    sudo ninja install
fi

# Update dyld cache (macOS specific)
if command -v update_dyld_shared_cache &> /dev/null; then
    sudo update_dyld_shared_cache || true
fi

echo
echo -e "${GREEN}╔════════════════════════════════════════════════════════════════╗${NC}"
echo -e "${GREEN}║                   Installation Successful!                     ║${NC}"
echo -e "${GREEN}╚════════════════════════════════════════════════════════════════╝${NC}"

# Verify installation
if [ -f "$INSTALL_PREFIX/lib/liboqs.dylib" ] || [ -f "$INSTALL_PREFIX/lib/liboqs.a" ]; then
    echo -e "${GREEN}✓${NC} liboqs installed successfully"
    echo -e "${GREEN}✓${NC} Library location: $INSTALL_PREFIX/lib"
    echo -e "${GREEN}✓${NC} Include location: $INSTALL_PREFIX/include"

    # Check if pkg-config can find it
    if pkg-config --exists liboqs 2>/dev/null; then
        INSTALLED_VERSION=$(pkg-config --modversion liboqs)
        echo -e "${GREEN}✓${NC} pkg-config version: ${INSTALLED_VERSION}"
    fi
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
echo "  cmake -S . -B build && cmake --build build -j\$(sysctl -n hw.ncpu)"
echo

exit 0
