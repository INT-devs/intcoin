#!/bin/bash
# Copyright (c) 2025 INTcoin Core (Maddison Lane)
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

# INTcoin Unified Build & Install Script
# Usage: ./build.sh [options]

set -e

# Default options
BUILD_TYPE="Release"
CLEAN=0
WITH_TESTS=1
WITH_QT=1
INSTALL=0
CREATE_PACKAGE=0
INSTALL_LIBOQS=0
LIBOQS_VERSION="0.10.1"
JOBS=$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Detect OS
if [[ "$OSTYPE" == "linux-gnu"* ]]; then
    OS="linux"
elif [[ "$OSTYPE" == "darwin"* ]]; then
    OS="macos"
elif [[ "$OSTYPE" == "freebsd"* ]]; then
    OS="freebsd"
else
    OS="unknown"
fi

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

print_info() {
    echo -e "${BLUE}ℹ $1${NC}"
}

# Usage
usage() {
    cat << EOF
${CYAN}INTcoin Unified Build & Install Script${NC}

${GREEN}Usage:${NC} $0 [OPTIONS]

${GREEN}Build Options:${NC}
    -c, --clean           Clean build directory before building
    -d, --debug           Build in Debug mode (default: Release)
    -t, --no-tests        Skip building tests
    -q, --no-qt           Build without Qt GUI wallet
    -j N, --jobs N        Use N parallel jobs (default: auto-detect)

${GREEN}Installation Options:${NC}
    -i, --install         Install binaries to system
    -p, --package         Create distribution package (.deb/.rpm/.pkg)
    --install-liboqs      Install liboqs (Open Quantum Safe library)
    --liboqs-version VER  liboqs version to install (default: $LIBOQS_VERSION)

${GREEN}Help:${NC}
    -h, --help            Show this help message

${GREEN}Examples:${NC}
    ${YELLOW}# Standard build${NC}
    $0

    ${YELLOW}# Clean build with liboqs installation${NC}
    $0 --clean --install-liboqs

    ${YELLOW}# Build and install to system${NC}
    $0 --install

    ${YELLOW}# Create distribution package${NC}
    $0 --package

    ${YELLOW}# Debug build without GUI${NC}
    $0 --debug --no-qt

${GREEN}Detected System:${NC} $OS

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
        --install-liboqs)
            INSTALL_LIBOQS=1
            shift
            ;;
        --liboqs-version)
            LIBOQS_VERSION="$2"
            shift 2
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

echo -e "${CYAN}========================================${NC}"
echo -e "${CYAN}INTcoin Build & Install Script${NC}"
echo -e "${CYAN}========================================${NC}"
echo ""
echo -e "${BLUE}Platform: $OS${NC}"
echo -e "${BLUE}Build Type: $BUILD_TYPE${NC}"
echo ""

# Check root for installation
if [ $INSTALL -eq 1 ] || [ $INSTALL_LIBOQS -eq 1 ]; then
    if [[ $EUID -ne 0 ]] && ! sudo -v 2>/dev/null; then
        print_error "Installation requires root privileges"
        print_info "Run with sudo or enable sudo for your user"
        exit 1
    fi
fi

TOTAL_STEPS=8
CURRENT_STEP=0

# Step 1: Install liboqs if requested
((CURRENT_STEP++))
if [ $INSTALL_LIBOQS -eq 1 ]; then
    print_step $CURRENT_STEP $TOTAL_STEPS "Installing liboqs $LIBOQS_VERSION..."

    # Check if liboqs is already installed
    if pkg-config --exists liboqs 2>/dev/null; then
        INSTALLED_VERSION=$(pkg-config --modversion liboqs)
        print_info "liboqs $INSTALLED_VERSION already installed"
        read -p "Reinstall? (y/N): " -n 1 -r
        echo
        if [[ ! $REPLY =~ ^[Yy]$ ]]; then
            print_success "Skipping liboqs installation"
            ((CURRENT_STEP++))
        fi
    fi

    if [ $INSTALL_LIBOQS -eq 1 ]; then
        # Create temp directory
        TEMP_DIR=$(mktemp -d)
        cd "$TEMP_DIR"

        # Install dependencies based on OS
        if [ "$OS" = "linux" ]; then
            print_info "Installing build dependencies..."
            if command -v apt-get &> /dev/null; then
                sudo apt-get update
                sudo apt-get install -y cmake ninja-build gcc g++ libssl-dev
            elif command -v dnf &> /dev/null; then
                sudo dnf install -y cmake ninja-build gcc gcc-c++ openssl-devel
            elif command -v yum &> /dev/null; then
                sudo yum install -y cmake ninja-build gcc gcc-c++ openssl-devel
            elif command -v pacman &> /dev/null; then
                sudo pacman -S --noconfirm cmake ninja gcc openssl
            fi
        elif [ "$OS" = "macos" ]; then
            print_info "Installing build dependencies..."
            if command -v brew &> /dev/null; then
                brew install cmake ninja openssl
            else
                print_warning "Homebrew not found. Please install dependencies manually."
            fi
        elif [ "$OS" = "freebsd" ]; then
            print_info "Installing build dependencies..."
            sudo pkg install -y cmake ninja openssl
        fi

        # Clone and build liboqs
        print_info "Downloading liboqs $LIBOQS_VERSION..."
        git clone --depth 1 --branch $LIBOQS_VERSION https://github.com/open-quantum-safe/liboqs.git

        cd liboqs
        mkdir build && cd build

        print_info "Configuring liboqs..."
        cmake -GNinja .. \
            -DCMAKE_INSTALL_PREFIX=/usr/local \
            -DCMAKE_BUILD_TYPE=Release \
            -DBUILD_SHARED_LIBS=ON \
            -DOQS_BUILD_ONLY_LIB=ON

        print_info "Building liboqs..."
        ninja

        print_info "Installing liboqs..."
        sudo ninja install

        # Update library cache
        if [ "$OS" = "linux" ]; then
            sudo ldconfig
        fi

        # Cleanup
        cd ../../..
        rm -rf "$TEMP_DIR"

        print_success "liboqs $LIBOQS_VERSION installed successfully"
    fi
else
    print_step $CURRENT_STEP $TOTAL_STEPS "Skipping liboqs installation (use --install-liboqs)"
fi
echo ""

# Step 2: Check prerequisites
((CURRENT_STEP++))
print_step $CURRENT_STEP $TOTAL_STEPS "Checking prerequisites..."

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

# Check for liboqs
if pkg-config --exists liboqs 2>/dev/null; then
    LIBOQS_VERSION_INSTALLED=$(pkg-config --modversion liboqs)
    print_success "liboqs $LIBOQS_VERSION_INSTALLED found"
else
    print_warning "liboqs not found - will use stub implementation"
    print_info "For production: run with --install-liboqs"
fi

# Check Qt
if [ $WITH_QT -eq 1 ]; then
    if pkg-config --exists Qt5Core 2>/dev/null; then
        QT_VERSION=$(pkg-config --modversion Qt5Core)
        print_success "Qt $QT_VERSION found"
    else
        print_warning "Qt5 not found. GUI wallet will be disabled."
        WITH_QT=0
    fi
fi

echo ""

# Step 3: Clean if requested
((CURRENT_STEP++))
if [ $CLEAN -eq 1 ] && [ -d "build" ]; then
    print_step $CURRENT_STEP $TOTAL_STEPS "Cleaning build directory..."
    rm -rf build
    print_success "Build directory cleaned"
else
    print_step $CURRENT_STEP $TOTAL_STEPS "Skipping clean (use --clean)"
fi
echo ""

# Step 4: Create build directory
((CURRENT_STEP++))
print_step $CURRENT_STEP $TOTAL_STEPS "Creating build directory..."
mkdir -p build
print_success "Build directory ready"
echo ""

# Step 5: Configure
((CURRENT_STEP++))
print_step $CURRENT_STEP $TOTAL_STEPS "Configuring with CMake..."
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

# Step 6: Build
((CURRENT_STEP++))
print_step $CURRENT_STEP $TOTAL_STEPS "Building ($BUILD_TYPE configuration)..."
echo -e "${CYAN}Using $JOBS parallel jobs${NC}"

make -j$JOBS

if [ $? -ne 0 ]; then
    print_error "Build failed"
    exit 1
fi

print_success "Build complete"
echo ""

# Step 7: Run tests
((CURRENT_STEP++))
if [ $WITH_TESTS -eq 1 ]; then
    print_step $CURRENT_STEP $TOTAL_STEPS "Running tests..."
    make test || print_warning "Some tests failed"
else
    print_step $CURRENT_STEP $TOTAL_STEPS "Skipping tests"
fi
echo ""

# Step 8: Install or package
((CURRENT_STEP++))
if [ $INSTALL -eq 1 ]; then
    print_step $CURRENT_STEP $TOTAL_STEPS "Installing..."
    sudo make install
    print_success "Installation complete"
elif [ $CREATE_PACKAGE -eq 1 ]; then
    print_step $CURRENT_STEP $TOTAL_STEPS "Creating package..."

    # Detect package type
    if [ "$OS" = "linux" ]; then
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
    elif [ "$OS" = "macos" ]; then
        cpack -G DragNDrop
        print_success "macOS DMG created"
    elif [ "$OS" = "freebsd" ]; then
        cpack -G TGZ
        print_success "FreeBSD package created"
    fi
else
    print_step $CURRENT_STEP $TOTAL_STEPS "Skipping installation (use --install or --package)"
fi

cd ..

echo ""
echo -e "${CYAN}========================================${NC}"
echo -e "${CYAN}Build Summary${NC}"
echo -e "${CYAN}========================================${NC}"
echo -e "Platform: ${BLUE}$OS${NC}"
echo -e "Build type: ${BLUE}$BUILD_TYPE${NC}"
echo -e "Build directory: ${BLUE}build/${NC}"
echo ""

echo -e "${YELLOW}Executables:${NC}"
echo -e "  ${GREEN}✓${NC} build/intcoind (Daemon)"
echo -e "  ${GREEN}✓${NC} build/intcoin-cli (CLI)"
[ $WITH_QT -eq 1 ] && echo -e "  ${GREEN}✓${NC} build/intcoin-qt (GUI Wallet)"
echo -e "  ${GREEN}✓${NC} build/intcoin-miner (CPU Miner)"
echo ""

if [ $INSTALL -eq 1 ]; then
    echo -e "${YELLOW}Installation:${NC}"
    echo -e "  Binaries: ${BLUE}/usr/local/bin/${NC}"
    echo -e "  Config: ${BLUE}/usr/local/share/intcoin/${NC}"
    echo -e "  Libraries: ${BLUE}/usr/local/lib/${NC}"
    echo ""
fi

if [ $CREATE_PACKAGE -eq 1 ]; then
    echo -e "${YELLOW}Package:${NC}"
    ls -1 build/INTcoin-*.{deb,rpm,tar.gz,dmg} 2>/dev/null || true
    echo ""
fi

echo -e "${YELLOW}To run:${NC}"
if [ $WITH_QT -eq 1 ]; then
    if [ "$OS" = "macos" ]; then
        echo -e "  ${CYAN}open build/intcoin-qt.app${NC}"
    else
        echo -e "  ${CYAN}./build/intcoin-qt${NC}"
    fi
fi
echo -e "  ${CYAN}./build/intcoind${NC}"
echo -e "  ${CYAN}./build/intcoin-cli help${NC}"
echo ""

if pkg-config --exists liboqs 2>/dev/null; then
    print_success "Build completed with liboqs (production-ready)"
else
    print_warning "Build completed with crypto stubs (NOT FOR PRODUCTION)"
    print_info "For production deployment: ./build.sh --install-liboqs --clean"
fi
echo ""
