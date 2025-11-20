#!/bin/bash
# Copyright (c) 2025 INTcoin Core (Maddison Lane)
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
#
# Windows Cross-Compilation Build Script
# Builds Windows .exe binaries from Linux/macOS using MinGW-w64

set -e

echo "╔════════════════════════════════════════════╗"
echo "║   INTcoin Windows Build Script           ║"
echo "╚════════════════════════════════════════════╝"
echo ""

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Determine host OS
if [[ "$OSTYPE" == "darwin"* ]]; then
    HOST_OS="macOS"
    MINGW_PREFIX="x86_64-w64-mingw32"
elif [[ "$OSTYPE" == "linux-gnu"* ]]; then
    HOST_OS="Linux"
    MINGW_PREFIX="x86_64-w64-mingw32"
else
    echo -e "${RED}Error: Unsupported host OS: $OSTYPE${NC}"
    exit 1
fi

echo "Host OS: $HOST_OS"
echo ""

# Check for MinGW-w64
echo "Checking for MinGW-w64 cross-compiler..."
if ! command -v ${MINGW_PREFIX}-gcc &> /dev/null; then
    echo -e "${RED}Error: MinGW-w64 not found!${NC}"
    echo ""
    echo "Please install MinGW-w64:"
    if [[ "$HOST_OS" == "macOS" ]]; then
        echo "  brew install mingw-w64"
    else
        echo "  sudo apt install mingw-w64"
    fi
    exit 1
fi

MINGW_VERSION=$(${MINGW_PREFIX}-gcc --version | head -n1)
echo -e "${GREEN}✓${NC} Found: $MINGW_VERSION"
echo ""

# Configuration
BUILD_DIR="build-windows"
INSTALL_DIR="install-windows"
CMAKE_TOOLCHAIN_FILE="cmake/toolchain-mingw64.cmake"

# Parse arguments
BUILD_TYPE="Release"
ENABLE_LIGHTNING="ON"
ENABLE_I2P="ON"
ENABLE_ML="ON"
ENABLE_GUI="OFF"  # Qt GUI disabled for now (cross-compilation complex)

while [[ $# -gt 0 ]]; do
    case $1 in
        --debug)
            BUILD_TYPE="Debug"
            shift
            ;;
        --no-lightning)
            ENABLE_LIGHTNING="OFF"
            shift
            ;;
        --no-i2p)
            ENABLE_I2P="OFF"
            shift
            ;;
        --no-ml)
            ENABLE_ML="OFF"
            shift
            ;;
        --with-gui)
            ENABLE_GUI="ON"
            shift
            ;;
        *)
            echo "Unknown option: $1"
            echo "Usage: $0 [--debug] [--no-lightning] [--no-i2p] [--no-ml] [--with-gui]"
            exit 1
            ;;
    esac
done

echo "Build Configuration:"
echo "  Build Type: $BUILD_TYPE"
echo "  Lightning Network: $ENABLE_LIGHTNING"
echo "  I2P Network: $ENABLE_I2P"
echo "  Machine Learning: $ENABLE_ML"
echo "  Qt GUI: $ENABLE_GUI"
echo ""

# Create CMake toolchain file
echo "Creating CMake toolchain file..."
mkdir -p cmake
cat > "$CMAKE_TOOLCHAIN_FILE" <<'EOF'
# CMake toolchain file for cross-compiling to Windows from Linux/macOS
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

# Specify the cross compiler
set(CMAKE_C_COMPILER x86_64-w64-mingw32-gcc)
set(CMAKE_CXX_COMPILER x86_64-w64-mingw32-g++)
set(CMAKE_RC_COMPILER x86_64-w64-mingw32-windres)

# Where to find libraries and headers
set(CMAKE_FIND_ROOT_PATH /usr/x86_64-w64-mingw32)

# Search for programs in the build host directories
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)

# Search for libraries and headers in the target directories
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

# Set Windows-specific flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static-libgcc -static-libstdc++")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static")
EOF

echo -e "${GREEN}✓${NC} Toolchain file created: $CMAKE_TOOLCHAIN_FILE"
echo ""

# Clean previous build
if [ -d "$BUILD_DIR" ]; then
    echo "Cleaning previous build directory..."
    rm -rf "$BUILD_DIR"
fi

# Create build directory
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Configure with CMake
echo "Configuring build with CMake..."
cmake .. \
    -DCMAKE_TOOLCHAIN_FILE="../$CMAKE_TOOLCHAIN_FILE" \
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
    -DCMAKE_INSTALL_PREFIX="../$INSTALL_DIR" \
    -DENABLE_LIGHTNING="$ENABLE_LIGHTNING" \
    -DENABLE_I2P="$ENABLE_I2P" \
    -DENABLE_ML="$ENABLE_ML" \
    -DBUILD_QT_WALLET="$ENABLE_GUI" \
    -DBUILD_TESTS=OFF

echo ""
echo -e "${GREEN}✓${NC} Configuration complete"
echo ""

# Build
echo "Building INTcoin for Windows..."
NPROC=$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
echo "Using $NPROC parallel jobs"
echo ""

if make -j"$NPROC"; then
    echo ""
    echo -e "${GREEN}✓${NC} Build successful!"
else
    echo ""
    echo -e "${RED}✗${NC} Build failed!"
    exit 1
fi

# Install/Package
echo ""
echo "Installing to $INSTALL_DIR..."
make install

cd ..

# Create distributable package
echo ""
echo "Creating Windows distribution package..."

DIST_DIR="intcoin-windows-x64"
rm -rf "$DIST_DIR"
mkdir -p "$DIST_DIR"

# Copy binaries
cp "$INSTALL_DIR/bin/"*.exe "$DIST_DIR/" 2>/dev/null || true

# Copy documentation
cp README.md "$DIST_DIR/"
cp COPYING "$DIST_DIR/"
cp QUICK-START.md "$DIST_DIR/"
mkdir -p "$DIST_DIR/docs"
cp docs/*.md "$DIST_DIR/docs/" 2>/dev/null || true

# Create sample configuration
cat > "$DIST_DIR/intcoin.conf.example" <<'EOF'
# INTcoin Configuration File (Windows)
# Copy to %APPDATA%\INTcoin\intcoin.conf

[network]
port=9333
maxconnections=125

[rpc]
port=9334
user=intcoin
password=CHANGE_THIS_PASSWORD
allowip=127.0.0.1

[lightning]
enabled=1
port=9335

[i2p]
enabled=1
sam.port=9336

[ml]
enabled=1
enable_fee_prediction=1
enable_anomaly_detection=1
EOF

# Create startup script
cat > "$DIST_DIR/start-intcoin.bat" <<'EOF'
@echo off
echo Starting INTcoin daemon...
intcoind.exe
pause
EOF

cat > "$DIST_DIR/start-intcoin-gui.bat" <<'EOF'
@echo off
echo Starting INTcoin Qt wallet...
intcoin-qt.exe
pause
EOF

# Create README for Windows
cat > "$DIST_DIR/README-WINDOWS.txt" <<'EOF'
INTcoin for Windows - Quick Start
==================================

Installation:
1. Extract this archive to C:\Program Files\INTcoin\
2. Copy intcoin.conf.example to %APPDATA%\INTcoin\intcoin.conf
3. Edit intcoin.conf and change the RPC password

Running:
- Double-click start-intcoin.bat to start the daemon
- Or double-click start-intcoin-gui.bat for the GUI wallet
- Or open Command Prompt and run: intcoind.exe

Data Directory:
%APPDATA%\INTcoin\

Configuration:
%APPDATA%\INTcoin\intcoin.conf

Documentation:
See docs\ folder for complete documentation

Support:
- Website: https://international-coin.org
- Email: team@international-coin.org

EOF

# Create zip archive
echo "Creating zip archive..."
ARCHIVE_NAME="intcoin-v1.2.0-windows-x64.zip"

if command -v zip &> /dev/null; then
    zip -r "$ARCHIVE_NAME" "$DIST_DIR"
    echo -e "${GREEN}✓${NC} Created: $ARCHIVE_NAME"
else
    echo -e "${YELLOW}Warning: zip not found. Archive not created.${NC}"
    echo "Install zip: brew install zip (macOS) or apt install zip (Linux)"
fi

echo ""
echo "╔════════════════════════════════════════════╗"
echo "║   Build Complete!                        ║"
echo "╚════════════════════════════════════════════╝"
echo ""
echo "Binaries:"
ls -lh "$DIST_DIR/"*.exe 2>/dev/null || echo "  (no .exe files found)"
echo ""
echo "Distribution: $DIST_DIR/"
if [ -f "$ARCHIVE_NAME" ]; then
    echo "Archive: $ARCHIVE_NAME"
    echo "Size: $(du -h "$ARCHIVE_NAME" | cut -f1)"
fi
echo ""
echo "To test on Windows:"
echo "  1. Transfer $ARCHIVE_NAME to Windows machine"
echo "  2. Extract archive"
echo "  3. Run start-intcoin.bat"
echo ""
