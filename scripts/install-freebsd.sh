#!/usr/bin/env sh
# INTcoin FreeBSD Installation Script
# Copyright (c) 2025 INTcoin Team (Neil Adamson)

set -e  # Exit on error
set -u  # Exit on undefined variable

# Configuration
INSTALL_PREFIX="${INSTALL_PREFIX:-/usr/local}"
BUILD_JOBS="${BUILD_JOBS:-$(sysctl -n hw.ncpu)}"
BUILD_TYPE="${BUILD_TYPE:-Release}"
TEMP_DIR="/tmp/intcoin-build-$$"

# Version requirements
LIBOQS_VERSION="0.15.0"
RANDOMX_VERSION="v1.2.1"

# Print banner
print_banner() {
    echo ""
    echo "╔═══════════════════════════════════════════════════════╗"
    echo "║         INTcoin FreeBSD Installation Script          ║"
    echo "║                                                       ║"
    echo "║  This script will install INTcoin and dependencies   ║"
    echo "║  on your FreeBSD system                              ║"
    echo "╚═══════════════════════════════════════════════════════╝"
    echo ""
}

# Print status message
info() {
    echo "[INFO] $1"
}

# Print success message
success() {
    echo "[SUCCESS] $1"
}

# Print warning message
warn() {
    echo "[WARNING] $1"
}

# Print error message and exit
error() {
    echo "[ERROR] $1"
    exit 1
}

# Check if running as root
check_root() {
    if [ "$(id -u)" -ne 0 ]; then
        error "This script must be run as root (use sudo)"
    fi
}

# Install dependencies
install_dependencies() {
    info "Installing dependencies..."

    pkg install -y \
        cmake \
        git \
        pkgconf \
        gmake \
        wget \
        curl \
        boost-all \
        openssl \
        rocksdb \
        zeromq \
        libevent \
        astyle \
        ninja \
        doxygen

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

    gmake -j"$BUILD_JOBS"
    gmake install

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

    gmake -j"$BUILD_JOBS"
    gmake install

    success "RandomX installed"
}

# Build and install INTcoin
install_intcoin() {
    info "Building INTcoin..."

    # Assume we're running from the intcoin source directory
    INTCOIN_DIR="$(cd "$(dirname "$0")" && pwd)"
    cd "$INTCOIN_DIR"

    # Create build directory
    mkdir -p build && cd build

    # Configure
    cmake \
        -DCMAKE_INSTALL_PREFIX="$INSTALL_PREFIX" \
        -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
        ..

    # Build
    gmake -j"$BUILD_JOBS"

    # Run tests
    info "Running tests..."
    ctest --output-on-failure || warn "Some tests failed"

    # Install
    gmake install

    success "INTcoin installed"
}

# Create configuration directory
setup_config() {
    info "Setting up configuration..."

    # Create config directory
    mkdir -p /usr/local/etc/intcoin

    # Create default config if it doesn't exist
    if [ ! -f /usr/local/etc/intcoin/intcoin.conf ]; then
        cat > /usr/local/etc/intcoin/intcoin.conf <<'EOF'
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
#datadir=/var/db/intcoin

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
        success "Created default configuration at /usr/local/etc/intcoin/intcoin.conf"
    else
        info "Configuration file already exists"
    fi
}

# Create rc.d service script
setup_rc_service() {
    info "Setting up rc.d service..."

    # Create intcoin user if it doesn't exist
    if ! pw usershow intcoin >/dev/null 2>&1; then
        pw useradd intcoin -m -d /var/db/intcoin -s /usr/sbin/nologin -c "INTcoin Daemon"
        success "Created intcoin user"
    fi

    # Create data directory
    mkdir -p /var/db/intcoin
    chown intcoin:intcoin /var/db/intcoin
    chmod 750 /var/db/intcoin

    # Create log directory
    mkdir -p /var/log/intcoin
    chown intcoin:intcoin /var/log/intcoin
    chmod 750 /var/log/intcoin

    # Create rc.d service script
    cat > /usr/local/etc/rc.d/intcoind <<'EOF'
#!/bin/sh

# PROVIDE: intcoind
# REQUIRE: LOGIN cleanvar
# KEYWORD: shutdown

# Add the following lines to /etc/rc.conf to enable intcoind:
#
# intcoind_enable="YES"
# intcoind_user="intcoin"
# intcoind_group="intcoin"
# intcoind_conf="/usr/local/etc/intcoin/intcoin.conf"
# intcoind_datadir="/var/db/intcoin"

. /etc/rc.subr

name="intcoind"
rcvar="intcoind_enable"

load_rc_config $name

: ${intcoind_enable:="NO"}
: ${intcoind_user:="intcoin"}
: ${intcoind_group:="intcoin"}
: ${intcoind_conf:="/usr/local/etc/intcoin/intcoin.conf"}
: ${intcoind_datadir:="/var/db/intcoin"}
: ${intcoind_pidfile:="/var/run/intcoind.pid"}

pidfile="${intcoind_pidfile}"
command="/usr/local/bin/intcoind"
command_args="-daemon -conf=${intcoind_conf} -datadir=${intcoind_datadir} -pid=${pidfile}"

start_precmd="intcoind_prestart"
stop_cmd="intcoind_stop"

intcoind_prestart()
{
    if [ ! -d "${intcoind_datadir}" ]; then
        install -d -o ${intcoind_user} -g ${intcoind_group} -m 750 ${intcoind_datadir}
    fi

    if [ ! -f "${intcoind_conf}" ]; then
        echo "Configuration file ${intcoind_conf} not found"
        return 1
    fi

    return 0
}

intcoind_stop()
{
    echo "Stopping ${name}..."
    /usr/local/bin/intcoin-cli -conf=${intcoind_conf} stop
    wait_for_pids $(cat ${pidfile} 2>/dev/null)
}

run_rc_command "$1"
EOF

    # Make executable
    chmod +x /usr/local/etc/rc.d/intcoind

    success "rc.d service created"
    info "Enable with: sysrc intcoind_enable=YES"
    info "Start with: service intcoind start"
}

# Update library cache
update_ldconfig() {
    info "Updating library cache..."

    /etc/rc.d/ldconfig restart

    success "Library cache updated"
}

# Print installation summary
print_summary() {
    echo ""
    echo "╔═══════════════════════════════════════════════════════╗"
    echo "║          Installation Complete!                       ║"
    echo "╚═══════════════════════════════════════════════════════╝"
    echo ""
    echo "Installed binaries:"
    echo "  - intcoind      : $INSTALL_PREFIX/bin/intcoind"
    echo "  - intcoin-cli   : $INSTALL_PREFIX/bin/intcoin-cli"
    echo "  - intcoin-miner : $INSTALL_PREFIX/bin/intcoin-miner"
    echo ""
    echo "Configuration:"
    echo "  - Config file   : /usr/local/etc/intcoin/intcoin.conf"
    echo "  - Data directory: /var/db/intcoin"
    echo "  - Log directory : /var/log/intcoin"
    echo ""
    echo "Next steps:"
    echo "  1. Edit configuration: ee /usr/local/etc/intcoin/intcoin.conf"
    echo "  2. Enable service: sysrc intcoind_enable=YES"
    echo "  3. Start daemon: service intcoind start"
    echo "  4. Check status: service intcoind status"
    echo "  5. View logs: tail -f /var/log/intcoin/debug.log"
    echo ""
    echo "CLI usage:"
    echo "  intcoin-cli getinfo"
    echo "  intcoin-cli getblockchaininfo"
    echo "  intcoin-cli help"
    echo ""
    echo "Documentation: https://code.intcoin.org/intcoin/core/wiki"
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
    setup_rc_service
    update_ldconfig

    # Cleanup
    info "Cleaning up temporary files..."
    rm -rf "$TEMP_DIR"

    # Print summary
    print_summary
}

# Run main function
main "$@"
