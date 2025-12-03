#!/usr/bin/env bash
# INTcoin Linux Installation Script
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
INSTALL_PREFIX="${INSTALL_PREFIX:-/usr/local}"
BUILD_JOBS="${BUILD_JOBS:-$(nproc)}"
BUILD_TYPE="${BUILD_TYPE:-Release}"
TEMP_DIR="/tmp/intcoin-build-$$"

# Version requirements
LIBOQS_VERSION="0.15.0"
RANDOMX_VERSION="v1.2.1"

# Detect distribution
if [ -f /etc/os-release ]; then
    . /etc/os-release
    DISTRO=$ID
    DISTRO_VERSION=$VERSION_ID
else
    echo -e "${RED}Error: Cannot detect Linux distribution${NC}"
    exit 1
fi

# Print banner
print_banner() {
    echo -e "${BLUE}"
    echo "╔═══════════════════════════════════════════════════════╗"
    echo "║          INTcoin Linux Installation Script           ║"
    echo "║                                                       ║"
    echo "║  This script will install INTcoin and dependencies   ║"
    echo "║  on your Linux system                                ║"
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

# Check if running as root
check_root() {
    if [ "$EUID" -ne 0 ]; then
        error "This script must be run as root (use sudo)"
    fi
}

# Install dependencies based on distribution
install_dependencies() {
    info "Installing dependencies for $DISTRO..."

    case "$DISTRO" in
        ubuntu|debian)
            apt-get update
            apt-get install -y \
                build-essential \
                cmake \
                pkg-config \
                git \
                wget \
                curl \
                libboost-all-dev \
                libssl-dev \
                librocksdb-dev \
                libzmq3-dev \
                libevent-dev \
                astyle \
                ninja-build \
                doxygen
            ;;

        fedora|rhel|centos)
            dnf install -y \
                gcc-c++ \
                cmake \
                make \
                git \
                pkg-config \
                wget \
                curl \
                boost-devel \
                openssl-devel \
                rocksdb-devel \
                zeromq-devel \
                libevent-devel \
                astyle \
                ninja-build \
                doxygen
            ;;

        arch|manjaro)
            pacman -Sy --noconfirm \
                base-devel \
                cmake \
                git \
                wget \
                curl \
                boost \
                openssl \
                rocksdb \
                zeromq \
                libevent \
                astyle \
                ninja \
                doxygen
            ;;

        *)
            error "Unsupported distribution: $DISTRO"
            ;;
    esac

    success "Dependencies installed"
}

# Build and install liboqs
install_liboqs() {
    info "Building and installing liboqs $LIBOQS_VERSION..."

    cd "$TEMP_DIR"

    if [ ! -d "liboqs" ]; then
        git clone --depth 1 --branch "$LIBOQS_VERSION" \
            https://github.com/open-quantum-safe/liboqs.git
    fi

    cd liboqs
    mkdir -p build && cd build

    cmake \
        -DCMAKE_INSTALL_PREFIX="$INSTALL_PREFIX" \
        -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
        -DBUILD_SHARED_LIBS=OFF \
        -DOQS_BUILD_ONLY_LIB=ON \
        ..

    make -j"$BUILD_JOBS"
    make install

    success "liboqs installed"
}

# Build and install RandomX
install_randomx() {
    info "Building and installing RandomX $RANDOMX_VERSION..."

    cd "$TEMP_DIR"

    if [ ! -d "RandomX" ]; then
        git clone --depth 1 --branch "$RANDOMX_VERSION" \
            https://github.com/tevador/RandomX.git
    fi

    cd RandomX
    mkdir -p build && cd build

    cmake \
        -DCMAKE_INSTALL_PREFIX="$INSTALL_PREFIX" \
        -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
        ..

    make -j"$BUILD_JOBS"
    make install

    success "RandomX installed"
}

# Build and install INTcoin
install_intcoin() {
    info "Building INTcoin..."

    # Assume we're running from the intcoin source directory
    INTCOIN_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
    cd "$INTCOIN_DIR"

    # Create build directory
    mkdir -p build && cd build

    # Configure
    cmake \
        -DCMAKE_INSTALL_PREFIX="$INSTALL_PREFIX" \
        -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
        ..

    # Build
    make -j"$BUILD_JOBS"

    # Run tests
    info "Running tests..."
    ctest --output-on-failure || warn "Some tests failed"

    # Install
    make install

    success "INTcoin installed"
}

# Create configuration directory
setup_config() {
    info "Setting up configuration..."

    # Create config directory
    mkdir -p /etc/intcoin

    # Create default config if it doesn't exist
    if [ ! -f /etc/intcoin/intcoin.conf ]; then
        cat > /etc/intcoin/intcoin.conf <<'EOF'
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
#datadir=/var/lib/intcoin

# Daemon settings
#daemon=1
#server=1

# Mining
#gen=0
#genproclimit=-1

# Logging
#debug=0
#printtoconsole=0

# Network optimization
#maxconnections=125
#maxuploadtarget=0
EOF
        success "Created default configuration at /etc/intcoin/intcoin.conf"
    else
        info "Configuration file already exists"
    fi
}

# Create systemd service
setup_systemd() {
    info "Setting up systemd service..."

    # Create intcoin user if it doesn't exist
    if ! id -u intcoin >/dev/null 2>&1; then
        useradd -r -m -d /var/lib/intcoin -s /bin/bash intcoin
        success "Created intcoin user"
    fi

    # Create data directory
    mkdir -p /var/lib/intcoin
    chown intcoin:intcoin /var/lib/intcoin
    chmod 750 /var/lib/intcoin

    # Create log directory
    mkdir -p /var/log/intcoin
    chown intcoin:intcoin /var/log/intcoin
    chmod 750 /var/log/intcoin

    # Create systemd service file
    cat > /etc/systemd/system/intcoind.service <<'EOF'
[Unit]
Description=INTcoin Daemon
After=network.target
Documentation=https://code.intcoin.org/intcoin/core

[Service]
Type=forking
User=intcoin
Group=intcoin

ExecStart=/usr/local/bin/intcoind -daemon -conf=/etc/intcoin/intcoin.conf -datadir=/var/lib/intcoin
ExecStop=/usr/local/bin/intcoin-cli -conf=/etc/intcoin/intcoin.conf stop

Restart=on-failure
RestartSec=10

# Security hardening
PrivateTmp=true
ProtectSystem=full
NoNewPrivileges=true
PrivateDevices=true
MemoryDenyWriteExecute=true

# Resource limits
LimitNOFILE=4096

# Logging
StandardOutput=journal
StandardError=journal

[Install]
WantedBy=multi-user.target
EOF

    # Reload systemd
    systemctl daemon-reload

    success "Systemd service created"
    info "Enable with: systemctl enable intcoind"
    info "Start with: systemctl start intcoind"
}

# Update library cache
update_ldconfig() {
    info "Updating library cache..."

    # Create ld.so.conf.d entry if needed
    if [ "$INSTALL_PREFIX" != "/usr" ] && [ "$INSTALL_PREFIX" != "/usr/local" ]; then
        echo "$INSTALL_PREFIX/lib" > /etc/ld.so.conf.d/intcoin.conf
    fi

    ldconfig

    success "Library cache updated"
}

# Print installation summary
print_summary() {
    echo ""
    echo -e "${GREEN}╔═══════════════════════════════════════════════════════╗${NC}"
    echo -e "${GREEN}║          Installation Complete!                       ║${NC}"
    echo -e "${GREEN}╚═══════════════════════════════════════════════════════╝${NC}"
    echo ""
    echo -e "${BLUE}Installed binaries:${NC}"
    echo "  - intcoind      : $INSTALL_PREFIX/bin/intcoind"
    echo "  - intcoin-cli   : $INSTALL_PREFIX/bin/intcoin-cli"
    echo "  - intcoin-miner : $INSTALL_PREFIX/bin/intcoin-miner"
    echo ""
    echo -e "${BLUE}Configuration:${NC}"
    echo "  - Config file   : /etc/intcoin/intcoin.conf"
    echo "  - Data directory: /var/lib/intcoin"
    echo "  - Log directory : /var/log/intcoin"
    echo ""
    echo -e "${BLUE}Next steps:${NC}"
    echo "  1. Edit configuration: sudo nano /etc/intcoin/intcoin.conf"
    echo "  2. Enable service: sudo systemctl enable intcoind"
    echo "  3. Start daemon: sudo systemctl start intcoind"
    echo "  4. Check status: sudo systemctl status intcoind"
    echo "  5. View logs: sudo journalctl -u intcoind -f"
    echo ""
    echo -e "${BLUE}CLI usage:${NC}"
    echo "  intcoin-cli getinfo"
    echo "  intcoin-cli getblockchaininfo"
    echo "  intcoin-cli help"
    echo ""
    echo -e "${YELLOW}Documentation: https://code.intcoin.org/intcoin/core/wiki${NC}"
    echo ""
}

# Main installation function
main() {
    print_banner

    # Check prerequisites
    check_root

    # Create temp directory
    mkdir -p "$TEMP_DIR"

    # Install steps
    install_dependencies
    install_liboqs
    install_randomx
    install_intcoin

    # Configuration and services
    setup_config
    setup_systemd
    update_ldconfig

    # Cleanup
    info "Cleaning up temporary files..."
    rm -rf "$TEMP_DIR"

    # Print summary
    print_summary
}

# Run main function
main "$@"
