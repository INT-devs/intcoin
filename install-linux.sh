#!/bin/bash
# Copyright (c) 2025 INTcoin Core (Maddison Lane)
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

# INTcoin Linux Installation Script
# This script installs INTcoin and its dependencies on Linux systems

set -e

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# Default options
INSTALL_DIR="/usr/local"
BUILD_FROM_SOURCE=0
INSTALL_QT=1
INSTALL_DEPS=1
CREATE_SERVICE=1
SYSTEM_USER="intcoin"

# Print functions
print_banner() {
    echo -e "${CYAN}================================${NC}"
    echo -e "${CYAN}INTcoin Linux Installer${NC}"
    echo -e "${CYAN}================================${NC}"
    echo ""
}

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
INTcoin Linux Installation Script

Usage: $0 [OPTIONS]

Options:
    -s, --source          Build from source instead of using binaries
    -d, --install-dir DIR Install directory (default: /usr/local)
    -u, --user USER       System user to run daemon (default: intcoin)
    --no-qt               Skip Qt GUI wallet installation
    --no-deps             Skip dependency installation
    --no-service          Don't create systemd service
    -h, --help            Show this help message

Examples:
    $0                    # Standard installation
    $0 --source           # Build and install from source
    $0 --no-qt            # Install without GUI wallet
    $0 --install-dir /opt/intcoin  # Custom install directory

EOF
    exit 0
}

# Parse arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        -s|--source)
            BUILD_FROM_SOURCE=1
            shift
            ;;
        -d|--install-dir)
            INSTALL_DIR="$2"
            shift 2
            ;;
        -u|--user)
            SYSTEM_USER="$2"
            shift 2
            ;;
        --no-qt)
            INSTALL_QT=0
            shift
            ;;
        --no-deps)
            INSTALL_DEPS=0
            shift
            ;;
        --no-service)
            CREATE_SERVICE=0
            shift
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

# Detect distribution
detect_distro() {
    if [ -f /etc/os-release ]; then
        . /etc/os-release
        DISTRO=$ID
        DISTRO_VERSION=$VERSION_ID
    elif [ -f /etc/redhat-release ]; then
        DISTRO="rhel"
    elif [ -f /etc/debian_version ]; then
        DISTRO="debian"
    else
        DISTRO="unknown"
    fi
}

# Check if running as root
check_root() {
    if [ "$EUID" -ne 0 ]; then
        print_error "This script must be run as root (use sudo)"
        exit 1
    fi
}

# Install dependencies based on distribution
install_dependencies() {
    print_step 1 6 "Installing dependencies..."

    case $DISTRO in
        ubuntu|debian|linuxmint|pop)
            apt-get update
            apt-get install -y \
                build-essential \
                cmake \
                git \
                libboost-all-dev \
                libssl-dev \
                libevent-dev \
                libdb++-dev \
                libsodium-dev \
                pkg-config \
                autoconf \
                automake \
                libtool

            if [ $INSTALL_QT -eq 1 ]; then
                apt-get install -y \
                    qtbase5-dev \
                    qttools5-dev \
                    qttools5-dev-tools \
                    libqrencode-dev
            fi
            ;;

        fedora|rhel|centos|rocky|almalinux)
            dnf install -y \
                gcc \
                gcc-c++ \
                cmake \
                git \
                boost-devel \
                openssl-devel \
                libevent-devel \
                libdb-cxx-devel \
                libsodium-devel \
                pkgconfig \
                autoconf \
                automake \
                libtool

            if [ $INSTALL_QT -eq 1 ]; then
                dnf install -y \
                    qt5-qtbase-devel \
                    qt5-qttools-devel \
                    qrencode-devel
            fi
            ;;

        arch|manjaro)
            pacman -Sy --noconfirm \
                base-devel \
                cmake \
                git \
                boost \
                openssl \
                libevent \
                db \
                libsodium \
                pkg-config

            if [ $INSTALL_QT -eq 1 ]; then
                pacman -Sy --noconfirm \
                    qt5-base \
                    qt5-tools \
                    qrencode
            fi
            ;;

        opensuse*)
            zypper install -y \
                gcc \
                gcc-c++ \
                cmake \
                git \
                boost-devel \
                libopenssl-devel \
                libevent-devel \
                libdb-4_8-devel \
                libsodium-devel \
                pkg-config

            if [ $INSTALL_QT -eq 1 ]; then
                zypper install -y \
                    libqt5-qtbase-devel \
                    libqt5-qttools-devel \
                    qrencode-devel
            fi
            ;;

        *)
            print_warning "Unknown distribution. Please install dependencies manually."
            print_warning "Required: cmake, git, boost, openssl, libevent, libdb++, libsodium"
            if [ $INSTALL_QT -eq 1 ]; then
                print_warning "For Qt wallet: qt5-base, qt5-tools, qrencode"
            fi
            read -p "Press Enter to continue or Ctrl+C to abort..."
            ;;
    esac

    print_success "Dependencies installed"
}

# Install liboqs (quantum-resistant crypto library)
install_liboqs() {
    print_step 2 6 "Installing liboqs..."

    if [ ! -d "/tmp/liboqs" ]; then
        cd /tmp
        git clone --depth 1 --branch main https://github.com/open-quantum-safe/liboqs.git
    fi

    cd /tmp/liboqs
    mkdir -p build
    cd build
    cmake -DCMAKE_INSTALL_PREFIX=/usr/local \
          -DBUILD_SHARED_LIBS=ON \
          -DCMAKE_BUILD_TYPE=Release ..
    make -j$(nproc)
    make install
    ldconfig

    cd /tmp
    rm -rf /tmp/liboqs

    print_success "liboqs installed"
}

# Build from source
build_from_source() {
    print_step 3 6 "Building INTcoin from source..."

    CURRENT_DIR=$(pwd)

    # Build
    mkdir -p build
    cd build

    cmake .. \
        -DCMAKE_BUILD_TYPE=Release \
        -DBUILD_QT_WALLET=$([ $INSTALL_QT -eq 1 ] && echo "ON" || echo "OFF") \
        -DBUILD_DAEMON=ON \
        -DBUILD_CLI=ON \
        -DBUILD_MINER=ON \
        -DCMAKE_INSTALL_PREFIX=$INSTALL_DIR

    make -j$(nproc)
    make install

    cd "$CURRENT_DIR"

    print_success "Build complete"
}

# Install binaries
install_binaries() {
    print_step 3 6 "Installing binaries..."

    if [ -f "build/intcoind" ]; then
        install -m 755 build/intcoind "$INSTALL_DIR/bin/"
        install -m 755 build/intcoin-cli "$INSTALL_DIR/bin/"
        install -m 755 build/intcoin-miner "$INSTALL_DIR/bin/"

        if [ $INSTALL_QT -eq 1 ] && [ -f "build/intcoin-qt" ]; then
            install -m 755 build/intcoin-qt "$INSTALL_DIR/bin/"
        fi

        print_success "Binaries installed to $INSTALL_DIR/bin"
    else
        print_error "Binaries not found. Please build first or use --source option."
        exit 1
    fi
}

# Create system user
create_user() {
    print_step 4 6 "Creating system user..."

    if ! id -u "$SYSTEM_USER" >/dev/null 2>&1; then
        useradd -r -m -s /bin/bash -d /var/lib/intcoin "$SYSTEM_USER"
        print_success "User '$SYSTEM_USER' created"
    else
        print_warning "User '$SYSTEM_USER' already exists"
    fi

    # Create data directory
    mkdir -p /var/lib/intcoin/.intcoin
    chown -R $SYSTEM_USER:$SYSTEM_USER /var/lib/intcoin

    # Create log directory
    mkdir -p /var/log/intcoin
    chown -R $SYSTEM_USER:$SYSTEM_USER /var/log/intcoin
}

# Create systemd service
create_systemd_service() {
    print_step 5 6 "Creating systemd service..."

    cat > /etc/systemd/system/intcoind.service << EOF
[Unit]
Description=INTcoin Daemon
After=network.target

[Service]
Type=forking
User=$SYSTEM_USER
Group=$SYSTEM_USER
WorkingDirectory=/var/lib/intcoin
ExecStart=$INSTALL_DIR/bin/intcoind -daemon -conf=/var/lib/intcoin/.intcoin/intcoin.conf -datadir=/var/lib/intcoin/.intcoin
ExecStop=$INSTALL_DIR/bin/intcoin-cli stop
Restart=on-failure
RestartSec=10
StandardOutput=append:/var/log/intcoin/intcoind.log
StandardError=append:/var/log/intcoin/intcoind.error.log

# Hardening
PrivateTmp=true
ProtectSystem=full
NoNewPrivileges=true
PrivateDevices=true
MemoryDenyWriteExecute=true

[Install]
WantedBy=multi-user.target
EOF

    systemctl daemon-reload
    print_success "Systemd service created"
}

# Create default configuration
create_config() {
    print_step 6 6 "Creating default configuration..."

    if [ ! -f "/var/lib/intcoin/.intcoin/intcoin.conf" ]; then
        cat > /var/lib/intcoin/.intcoin/intcoin.conf << EOF
# INTcoin Configuration File

# Network settings
listen=1
maxconnections=125

# RPC settings
server=1
rpcuser=intcoinrpc
rpcpassword=$(openssl rand -hex 32)
rpcallowip=127.0.0.1
rpcport=9334

# Mining (disabled by default)
gen=0

# Logging
debug=0
logtimestamps=1

# Data directory
datadir=/var/lib/intcoin/.intcoin
EOF

        chown $SYSTEM_USER:$SYSTEM_USER /var/lib/intcoin/.intcoin/intcoin.conf
        chmod 600 /var/lib/intcoin/.intcoin/intcoin.conf

        print_success "Configuration created at /var/lib/intcoin/.intcoin/intcoin.conf"
    else
        print_warning "Configuration already exists, skipping"
    fi
}

# Create desktop entry for Qt wallet
create_desktop_entry() {
    if [ $INSTALL_QT -eq 1 ]; then
        cat > /usr/share/applications/intcoin-qt.desktop << EOF
[Desktop Entry]
Version=1.0
Type=Application
Name=INTcoin Wallet
Comment=Quantum-resistant cryptocurrency wallet
Icon=intcoin
Exec=$INSTALL_DIR/bin/intcoin-qt
Terminal=false
Categories=Finance;Network;
EOF
        print_success "Desktop entry created"
    fi
}

# Main installation
main() {
    print_banner

    check_root
    detect_distro

    echo -e "Distribution: ${CYAN}$DISTRO${NC}"
    echo -e "Install directory: ${CYAN}$INSTALL_DIR${NC}"
    echo -e "Build from source: ${CYAN}$([ $BUILD_FROM_SOURCE -eq 1 ] && echo 'Yes' || echo 'No')${NC}"
    echo -e "Install Qt wallet: ${CYAN}$([ $INSTALL_QT -eq 1 ] && echo 'Yes' || echo 'No')${NC}"
    echo ""

    read -p "Proceed with installation? (y/N) " -n 1 -r
    echo
    if [[ ! $REPLY =~ ^[Yy]$ ]]; then
        echo "Installation cancelled."
        exit 0
    fi

    # Install dependencies
    if [ $INSTALL_DEPS -eq 1 ]; then
        install_dependencies
        install_liboqs
    fi

    # Build or install
    if [ $BUILD_FROM_SOURCE -eq 1 ]; then
        build_from_source
    else
        install_binaries
    fi

    # System setup
    create_user

    if [ $CREATE_SERVICE -eq 1 ]; then
        create_systemd_service
    fi

    create_config
    create_desktop_entry

    echo ""
    echo -e "${GREEN}================================${NC}"
    echo -e "${GREEN}Installation Complete!${NC}"
    echo -e "${GREEN}================================${NC}"
    echo ""
    echo -e "Installed binaries:"
    echo -e "  - ${CYAN}$INSTALL_DIR/bin/intcoind${NC} (daemon)"
    echo -e "  - ${CYAN}$INSTALL_DIR/bin/intcoin-cli${NC} (CLI)"
    [ $INSTALL_QT -eq 1 ] && echo -e "  - ${CYAN}$INSTALL_DIR/bin/intcoin-qt${NC} (GUI wallet)"
    echo -e "  - ${CYAN}$INSTALL_DIR/bin/intcoin-miner${NC} (miner)"
    echo ""
    echo -e "Configuration:"
    echo -e "  - ${CYAN}/var/lib/intcoin/.intcoin/intcoin.conf${NC}"
    echo ""

    if [ $CREATE_SERVICE -eq 1 ]; then
        echo -e "To start the daemon:"
        echo -e "  ${CYAN}sudo systemctl start intcoind${NC}"
        echo -e "  ${CYAN}sudo systemctl enable intcoind${NC}  # Start on boot"
        echo ""
    fi

    echo -e "To run the Qt wallet (as regular user):"
    echo -e "  ${CYAN}intcoin-qt${NC}"
    echo ""

    print_success "INTcoin installation complete!"
}

main "$@"
