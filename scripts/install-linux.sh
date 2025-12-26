#!/bin/bash
# INTcoin Linux Installation Script
# Supports: Ubuntu 20.04+, Debian 11+, Fedora 35+, CentOS 8+, Arch Linux

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

error() { echo -e "${RED}ERROR: $1${NC}" >&2; exit 1; }
success() { echo -e "${GREEN}✓ $1${NC}"; }
info() { echo -e "${CYAN}$1${NC}"; }
warning() { echo -e "${YELLOW}⚠ $1${NC}"; }

echo -e "${CYAN}=========================================${NC}"
echo -e "${CYAN}INTcoin Linux Installation Script${NC}"
echo -e "${CYAN}=========================================${NC}"
echo ""

# Detect Linux distribution
if [ -f /etc/os-release ]; then
    . /etc/os-release
    DISTRO=$ID
    VERSION=$VERSION_ID
else
    error "Cannot detect Linux distribution"
fi

info "Detected: $PRETTY_NAME"

# Enforce minimum OS versions (do not support out-of-date operating systems)
case $DISTRO in
    ubuntu)
        # Ubuntu 20.04+ only
        VERSION_NUM=$(echo "$VERSION" | cut -d'.' -f1)
        if [ "$VERSION_NUM" -lt 20 ]; then
            error "Ubuntu $VERSION is not supported. Minimum required: Ubuntu 20.04 LTS"
        fi
        ;;
    debian)
        # Debian 11+ only
        if [ "$VERSION_ID" -lt 11 ]; then
            error "Debian $VERSION is not supported. Minimum required: Debian 11 (Bullseye)"
        fi
        ;;
    fedora)
        # Fedora 38+ only (Fedora has short support cycles)
        if [ "$VERSION_ID" -lt 38 ]; then
            error "Fedora $VERSION_ID is end-of-life and not supported. Minimum required: Fedora 38"
        fi
        ;;
    centos|rhel)
        # CentOS/RHEL 8+ only
        VERSION_NUM=$(echo "$VERSION" | cut -d'.' -f1)
        if [ "$VERSION_NUM" -lt 8 ]; then
            error "$DISTRO $VERSION is not supported. Minimum required: version 8"
        fi
        ;;
esac

echo ""

# Check if running as root
if [ "$EUID" -eq 0 ]; then
    warning "Running as root. Installation will be system-wide."
    INSTALL_PREFIX="/usr/local"
    INSTALL_AS_ROOT=true
else
    info "Running as user. Installation will be in home directory."
    INSTALL_PREFIX="$HOME/.local"
    INSTALL_AS_ROOT=false
fi

# Configuration
BUILD_TYPE="${BUILD_TYPE:-Release}"
BUILD_TESTS="${BUILD_TESTS:-ON}"
BUILD_QT="${BUILD_QT:-ON}"
NUM_CORES=$(nproc)

info "Build Configuration:"
info "  Install Prefix: $INSTALL_PREFIX"
info "  Build Type: $BUILD_TYPE"
info "  Build Tests: $BUILD_TESTS"
info "  Build Qt Wallet: $BUILD_QT"
info "  CPU Cores: $NUM_CORES"
echo ""

# Install dependencies based on distribution
install_dependencies() {
    info "Installing dependencies..."

    case $DISTRO in
        ubuntu|debian)
            if [ "$INSTALL_AS_ROOT" = true ]; then
                SUDO=""
            else
                SUDO="sudo"
            fi

            $SUDO apt-get update

            # Core dependencies
            $SUDO apt-get install -y \
                build-essential \
                cmake \
                git \
                pkg-config \
                libssl-dev \
                librocksdb-dev \
                zlib1g-dev \
                libcurl4-openssl-dev \
                nlohmann-json3-dev \
                libsqlite3-dev \
                autoconf \
                automake \
                libtool \
                ninja-build

            # Qt dependencies (optional)
            if [ "$BUILD_QT" = "ON" ]; then
                $SUDO apt-get install -y \
                    qtbase5-dev \
                    qttools5-dev \
                    qttools5-dev-tools \
                    libqrencode-dev
            fi
            ;;

        fedora|centos|rhel)
            if [ "$INSTALL_AS_ROOT" = true ]; then
                SUDO=""
            else
                SUDO="sudo"
            fi

            # Core dependencies
            $SUDO dnf install -y \
                gcc \
                gcc-c++ \
                cmake \
                git \
                pkgconfig \
                openssl-devel \
                rocksdb-devel \
                zlib-devel \
                libcurl-devel \
                json-devel \
                sqlite-devel \
                autoconf \
                automake \
                libtool \
                ninja-build

            # Qt dependencies (optional)
            if [ "$BUILD_QT" = "ON" ]; then
                $SUDO dnf install -y \
                    qt5-qtbase-devel \
                    qt5-qttools-devel \
                    qrencode-devel
            fi
            ;;

        arch|manjaro)
            if [ "$INSTALL_AS_ROOT" = true ]; then
                SUDO=""
            else
                SUDO="sudo"
            fi

            # Core dependencies
            $SUDO pacman -Syu --noconfirm \
                base-devel \
                cmake \
                git \
                pkg-config \
                openssl \
                rocksdb \
                zlib \
                curl \
                nlohmann-json \
                sqlite \
                autoconf \
                automake \
                libtool \
                ninja

            # Qt dependencies (optional)
            if [ "$BUILD_QT" = "ON" ]; then
                $SUDO pacman -S --noconfirm \
                    qt5-base \
                    qt5-tools \
                    qrencode
            fi
            ;;

        *)
            warning "Unsupported distribution: $DISTRO"
            warning "Please install dependencies manually:"
            warning "  - GCC 11+ or Clang 14+"
            warning "  - CMake 3.20+"
            warning "  - OpenSSL 3.0+"
            warning "  - RocksDB 7.0+"
            warning "  - zlib, curl, nlohmann-json, sqlite3"
            read -p "Continue anyway? [y/N] " -n 1 -r
            echo
            if [[ ! $REPLY =~ ^[Yy]$ ]]; then
                exit 1
            fi
            ;;
    esac

    success "Dependencies installed"
}

# Build liboqs
build_liboqs() {
    info "Building liboqs 0.15.0..."

    LIBOQS_DIR="$HOME/.intcoin-build/liboqs"
    LIBOQS_INSTALL="$INSTALL_PREFIX"

    if [ ! -d "$LIBOQS_DIR" ]; then
        mkdir -p "$(dirname "$LIBOQS_DIR")"
        git clone --branch 0.15.0 https://github.com/open-quantum-safe/liboqs.git "$LIBOQS_DIR"
    fi

    cd "$LIBOQS_DIR"

    if [ -d build ]; then
        rm -rf build
    fi
    mkdir build && cd build

    cmake -GNinja \
        -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
        -DCMAKE_INSTALL_PREFIX="$LIBOQS_INSTALL" \
        -DBUILD_SHARED_LIBS=OFF \
        -DOQS_BUILD_ONLY_LIB=ON \
        ..

    ninja
    ninja install

    success "liboqs built and installed"
}

# Build RandomX
build_randomx() {
    info "Building RandomX 1.2.1..."

    RANDOMX_DIR="$HOME/.intcoin-build/randomx"
    RANDOMX_INSTALL="$INSTALL_PREFIX"

    if [ ! -d "$RANDOMX_DIR" ]; then
        mkdir -p "$(dirname "$RANDOMX_DIR")"
        git clone --branch v1.2.1 https://github.com/tevador/RandomX.git "$RANDOMX_DIR"
    fi

    cd "$RANDOMX_DIR"

    if [ -d build ]; then
        rm -rf build
    fi
    mkdir build && cd build

    cmake -GNinja \
        -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
        -DCMAKE_INSTALL_PREFIX="$RANDOMX_INSTALL" \
        -DBUILD_SHARED_LIBS=OFF \
        ..

    ninja
    ninja install

    success "RandomX built and installed"
}

# Build INTcoin
build_intcoin() {
    info "Building INTcoin..."

    SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
    BUILD_DIR="$SCRIPT_DIR/build-linux"

    if [ -d "$BUILD_DIR" ]; then
        rm -rf "$BUILD_DIR"
    fi
    mkdir -p "$BUILD_DIR"

    cd "$BUILD_DIR"

    cmake -GNinja \
        -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
        -DCMAKE_INSTALL_PREFIX="$INSTALL_PREFIX" \
        -DBUILD_TESTS=$BUILD_TESTS \
        -DBUILD_QT=$BUILD_QT \
        ..

    ninja -j$NUM_CORES

    if [ "$BUILD_TESTS" = "ON" ]; then
        info "Running tests..."
        ctest --output-on-failure || warning "Some tests failed"
    fi

    ninja install

    success "INTcoin built and installed"
}

# Create systemd service
create_systemd_service() {
    if [ "$INSTALL_AS_ROOT" = true ]; then
        info "Creating systemd service..."

        cat > /etc/systemd/system/intcoind.service <<EOF
[Unit]
Description=INTcoin Daemon
After=network.target

[Service]
Type=forking
User=intcoin
Group=intcoin
WorkingDirectory=/var/lib/intcoin
ExecStart=$INSTALL_PREFIX/bin/intcoind -daemon -conf=/etc/intcoin/intcoin.conf -datadir=/var/lib/intcoin
ExecStop=$INSTALL_PREFIX/bin/intcoin-cli stop
Restart=on-failure
RestartSec=10

# Process management
KillMode=mixed
TimeoutStopSec=600

# Security hardening
NoNewPrivileges=true
PrivateTmp=true
ProtectSystem=full
ProtectHome=true

[Install]
WantedBy=multi-user.target
EOF

        # Create intcoin user
        if ! id -u intcoin > /dev/null 2>&1; then
            useradd -r -m -d /var/lib/intcoin -s /bin/bash intcoin
        fi

        # Create directories
        mkdir -p /etc/intcoin
        mkdir -p /var/lib/intcoin
        chown -R intcoin:intcoin /var/lib/intcoin

        # Create default config
        if [ ! -f /etc/intcoin/intcoin.conf ]; then
            cat > /etc/intcoin/intcoin.conf <<EOF
# INTcoin Configuration
datadir=/var/lib/intcoin
rpcuser=intcoinrpc
rpcpassword=$(openssl rand -hex 32)
rpcport=2211
port=2210
server=1
daemon=1
EOF
            chmod 600 /etc/intcoin/intcoin.conf
            chown intcoin:intcoin /etc/intcoin/intcoin.conf
        fi

        systemctl daemon-reload
        systemctl enable intcoind

        success "Systemd service created"
        info "To start the service: systemctl start intcoind"
        info "To view logs: journalctl -u intcoind -f"
    else
        info "Skipping systemd service creation (not running as root)"
    fi
}

# Update PATH
update_path() {
    if [ "$INSTALL_AS_ROOT" = false ]; then
        BIN_DIR="$INSTALL_PREFIX/bin"

        # Add to PATH if not already there
        if [[ ":$PATH:" != *":$BIN_DIR:"* ]]; then
            info "Adding $BIN_DIR to PATH..."

            # Determine shell config file
            if [ -n "$BASH_VERSION" ]; then
                SHELL_RC="$HOME/.bashrc"
            elif [ -n "$ZSH_VERSION" ]; then
                SHELL_RC="$HOME/.zshrc"
            else
                SHELL_RC="$HOME/.profile"
            fi

            echo "" >> "$SHELL_RC"
            echo "# INTcoin" >> "$SHELL_RC"
            echo "export PATH=\"$BIN_DIR:\$PATH\"" >> "$SHELL_RC"

            success "Updated $SHELL_RC"
            info "Run: source $SHELL_RC  (or restart your shell)"
        fi
    fi
}

# Main installation flow
main() {
    info "Starting installation..."
    echo ""

    # Install system dependencies
    install_dependencies
    echo ""

    # Build liboqs
    build_liboqs
    echo ""

    # Build RandomX
    build_randomx
    echo ""

    # Build INTcoin
    build_intcoin
    echo ""

    # Create systemd service (if root)
    create_systemd_service
    echo ""

    # Update PATH (if user install)
    update_path
    echo ""

    # Installation complete
    echo -e "${GREEN}=========================================${NC}"
    echo -e "${GREEN}Installation Complete!${NC}"
    echo -e "${GREEN}=========================================${NC}"
    echo ""
    echo -e "${CYAN}Installed executables:${NC}"
    echo "  - intcoind          : Full node daemon"
    echo "  - intcoin-cli       : Command-line interface"
    echo "  - intcoin-wallet    : Wallet management"
    echo "  - intcoin-miner     : Standalone miner"
    echo "  - intcoin-pool-server : Mining pool server"
    if [ "$BUILD_QT" = "ON" ]; then
        echo "  - intcoin-qt        : Qt graphical wallet"
    fi
    echo ""
    echo -e "${CYAN}Installation location:${NC} $INSTALL_PREFIX/bin"
    echo ""
    echo -e "${CYAN}Quick start:${NC}"
    if [ "$INSTALL_AS_ROOT" = true ]; then
        echo "  systemctl start intcoind      # Start daemon"
        echo "  intcoin-cli getinfo           # Check status"
    else
        echo "  intcoind -daemon              # Start daemon"
        echo "  intcoin-cli getinfo           # Check status"
    fi
    echo ""
    echo -e "${CYAN}Documentation:${NC}"
    echo "  $(dirname "${BASH_SOURCE[0]}")/docs/"
    echo ""
    success "Done!"
}

# Run main installation
main
