#!/bin/bash
# Copyright (c) 2025 INTcoin Core (Maddison Lane)
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

# INTcoin Installation Script for Debian/Ubuntu
# Supports: Debian 11+, Ubuntu 20.04+

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# Configuration
INSTALL_DIR="/usr/local/bin"
DATA_DIR="$HOME/.intcoin"
SERVICE_FILE="/etc/systemd/system/intcoind.service"

# Print functions
print_header() {
    echo -e "${CYAN}================================${NC}"
    echo -e "${CYAN}$1${NC}"
    echo -e "${CYAN}================================${NC}"
}

print_step() {
    echo -e "${YELLOW}[$1] $2${NC}"
}

print_success() {
    echo -e "${GREEN}✓ $1${NC}"
}

print_error() {
    echo -e "${RED}✗ $1${NC}"
}

print_info() {
    echo -e "${BLUE}ℹ $1${NC}"
}

# Check if running as root
check_root() {
    if [[ $EUID -eq 0 ]]; then
        print_error "This script should not be run as root. Run as normal user with sudo privileges."
        exit 1
    fi
}

# Check sudo access
check_sudo() {
    if ! sudo -n true 2>/dev/null; then
        print_info "This script requires sudo privileges for system-level installation."
        sudo -v
    fi
}

# Detect OS
detect_os() {
    if [ -f /etc/os-release ]; then
        . /etc/os-release
        OS=$ID
        VER=$VERSION_ID
    else
        print_error "Cannot detect OS. This script supports Debian and Ubuntu only."
        exit 1
    fi

    if [[ "$OS" != "debian" && "$OS" != "ubuntu" ]]; then
        print_error "This script is for Debian/Ubuntu. Detected: $OS"
        print_info "Use install-fedora.sh for Fedora/RHEL or install-arch.sh for Arch Linux"
        exit 1
    fi

    print_success "Detected: $OS $VER"
}

# Install dependencies
install_dependencies() {
    print_step "2/8" "Installing dependencies..."

    sudo apt-get update -qq

    # Build tools
    sudo apt-get install -y -qq \
        build-essential \
        cmake \
        git \
        pkg-config \
        autoconf \
        automake \
        libtool \
        curl \
        wget

    # Libraries
    sudo apt-get install -y -qq \
        libboost-all-dev \
        libssl-dev \
        libevent-dev \
        libminiupnpc-dev \
        libzmq3-dev \
        libsqlite3-dev

    # Qt5 (for GUI wallet)
    sudo apt-get install -y -qq \
        qtbase5-dev \
        qttools5-dev \
        qttools5-dev-tools \
        libqt5svg5-dev

    # Python (for tests)
    sudo apt-get install -y -qq \
        python3 \
        python3-pip \
        python3-dev

    # Install liboqs for quantum-resistant crypto
    if ! pkg-config --exists liboqs; then
        print_info "Building liboqs from source..."

        # Create temp directory
        TEMP_DIR=$(mktemp -d)
        cd "$TEMP_DIR"

        # Clone and build liboqs
        git clone --depth 1 --branch main https://github.com/open-quantum-safe/liboqs.git
        cd liboqs
        mkdir build && cd build
        cmake -DCMAKE_INSTALL_PREFIX=/usr/local \
              -DCMAKE_BUILD_TYPE=Release \
              -DBUILD_SHARED_LIBS=ON \
              ..
        make -j$(nproc)
        sudo make install
        sudo ldconfig

        # Cleanup
        cd ~
        rm -rf "$TEMP_DIR"

        print_success "liboqs installed"
    else
        print_success "liboqs already installed"
    fi

    print_success "Dependencies installed"
}

# Clone repository
clone_repo() {
    print_step "3/8" "Cloning INTcoin repository..."

    if [ -d "intcoin-build" ]; then
        print_info "Repository already exists. Pulling latest changes..."
        cd intcoin-build
        git pull
    else
        git clone https://gitlab.com/intcoin/crypto.git intcoin-build
        cd intcoin-build
    fi

    print_success "Repository ready"
}

# Build INTcoin
build_intcoin() {
    print_step "4/8" "Building INTcoin..."

    # Clean previous build
    if [ -d "build" ]; then
        rm -rf build
    fi

    mkdir build && cd build

    # Configure
    print_info "Configuring with CMake..."
    cmake .. \
        -DCMAKE_BUILD_TYPE=Release \
        -DBUILD_QT_WALLET=ON \
        -DENABLE_LIGHTNING=ON \
        -DBUILD_TESTS=ON

    # Build
    print_info "Compiling (this may take several minutes)..."
    make -j$(nproc)

    print_success "Build complete"
}

# Run tests
run_tests() {
    print_step "5/8" "Running tests..."

    if make test; then
        print_success "All tests passed"
    else
        print_error "Some tests failed (non-critical, continuing installation)"
    fi
}

# Install binaries
install_binaries() {
    print_step "6/8" "Installing binaries..."

    sudo install -m 0755 -o root -g root -t "$INSTALL_DIR" intcoind
    sudo install -m 0755 -o root -g root -t "$INSTALL_DIR" intcoin-cli
    sudo install -m 0755 -o root -g root -t "$INSTALL_DIR" intcoin-wallet
    sudo install -m 0755 -o root -g root -t "$INSTALL_DIR" intcoin-miner

    if [ -f "intcoin-qt" ]; then
        sudo install -m 0755 -o root -g root -t "$INSTALL_DIR" intcoin-qt
        print_success "GUI wallet installed"
    fi

    print_success "Binaries installed to $INSTALL_DIR"
}

# Create data directory
create_data_dir() {
    print_step "7/8" "Creating data directory..."

    if [ ! -d "$DATA_DIR" ]; then
        mkdir -p "$DATA_DIR"
        print_success "Created $DATA_DIR"
    else
        print_info "Data directory already exists: $DATA_DIR"
    fi

    # Create default config if it doesn't exist
    if [ ! -f "$DATA_DIR/intcoin.conf" ]; then
        cat > "$DATA_DIR/intcoin.conf" << EOF
# INTcoin Configuration File
# Generated by install script

# Server mode
server=1

# RPC settings
rpcuser=intcoinrpc
rpcpassword=$(openssl rand -hex 32)
rpcallowip=127.0.0.1
rpcport=8332

# Network settings
listen=1
maxconnections=125

# Mining settings
# gen=0
# genproclimit=4

# Data directory
datadir=$DATA_DIR

# Logging
debug=0
printtoconsole=0
EOF
        print_success "Created default configuration file"
    else
        print_info "Configuration file already exists"
    fi
}

# Create systemd service
create_service() {
    print_step "8/8" "Creating systemd service..."

    sudo tee "$SERVICE_FILE" > /dev/null << EOF
[Unit]
Description=INTcoin Daemon
After=network.target

[Service]
Type=forking
User=$USER
Group=$USER
ExecStart=$INSTALL_DIR/intcoind -daemon -conf=$DATA_DIR/intcoin.conf -datadir=$DATA_DIR
ExecStop=$INSTALL_DIR/intcoin-cli stop
Restart=on-failure
RestartSec=10
TimeoutStopSec=60

# Hardening
PrivateTmp=true
ProtectSystem=full
NoNewPrivileges=true
PrivateDevices=true
MemoryDenyWriteExecute=true

[Install]
WantedBy=multi-user.target
EOF

    sudo systemctl daemon-reload
    print_success "Systemd service created"
    print_info "To enable at boot: sudo systemctl enable intcoind"
    print_info "To start now: sudo systemctl start intcoind"
}

# Print summary
print_summary() {
    echo ""
    print_header "Installation Complete!"
    echo ""
    echo -e "${GREEN}INTcoin has been successfully installed!${NC}"
    echo ""
    echo -e "${YELLOW}Installed binaries:${NC}"
    echo "  • intcoind         - Daemon"
    echo "  • intcoin-cli      - Command-line interface"
    echo "  • intcoin-wallet   - Wallet management tool"
    echo "  • intcoin-miner    - CPU miner"
    if [ -f "$INSTALL_DIR/intcoin-qt" ]; then
        echo "  • intcoin-qt       - GUI wallet"
    fi
    echo ""
    echo -e "${YELLOW}Configuration:${NC}"
    echo "  • Data directory: $DATA_DIR"
    echo "  • Config file: $DATA_DIR/intcoin.conf"
    echo ""
    echo -e "${YELLOW}Quick start commands:${NC}"
    echo "  # Start GUI wallet"
    echo "  $ intcoin-qt"
    echo ""
    echo "  # Or start daemon"
    echo "  $ intcoind -daemon"
    echo ""
    echo "  # Check status"
    echo "  $ intcoin-cli getblockchaininfo"
    echo ""
    echo "  # Get new address"
    echo "  $ intcoin-cli getnewaddress"
    echo ""
    echo "  # Start mining"
    echo "  $ intcoin-cli startmining 4"
    echo ""
    echo -e "${YELLOW}Systemd service:${NC}"
    echo "  # Enable and start service"
    echo "  $ sudo systemctl enable intcoind"
    echo "  $ sudo systemctl start intcoind"
    echo ""
    echo "  # Check status"
    echo "  $ sudo systemctl status intcoind"
    echo ""
    echo -e "${YELLOW}Documentation:${NC}"
    echo "  • Website: https://international-coin.org"
    echo "  • Repository: https://gitlab.com/intcoin/crypto"
    echo "  • Wiki: https://gitlab.com/intcoin/crypto/-/wikis/home"
    echo ""
    echo -e "${GREEN}Thank you for installing INTcoin!${NC}"
    echo ""
}

# Cleanup function
cleanup() {
    cd ~
    if [ -d "intcoin-build" ]; then
        print_info "Build directory preserved at: ~/intcoin-build"
        print_info "You can safely remove it with: rm -rf ~/intcoin-build"
    fi
}

# Main installation flow
main() {
    print_header "INTcoin Installation Script"
    echo ""
    print_info "This script will install INTcoin on your system"
    print_info "Supported: Debian 11+, Ubuntu 20.04+"
    echo ""

    print_step "1/8" "Checking system requirements..."
    check_root
    check_sudo
    detect_os

    # Change to home directory for build
    cd ~

    install_dependencies
    clone_repo
    build_intcoin
    run_tests
    install_binaries
    create_data_dir
    create_service

    cleanup
    print_summary
}

# Run main function
main "$@"
