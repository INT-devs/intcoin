#!/bin/sh
# Copyright (c) 2025 INTcoin Core (Maddison Lane)
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

# INTcoin FreeBSD Installation Script
# This script installs INTcoin and its dependencies on FreeBSD

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
    printf "${CYAN}================================${NC}\n"
    printf "${CYAN}INTcoin FreeBSD Installer${NC}\n"
    printf "${CYAN}================================${NC}\n"
    printf "\n"
}

print_step() {
    printf "${CYAN}[$1/$2] $3${NC}\n"
}

print_success() {
    printf "${GREEN}✓ $1${NC}\n"
}

print_warning() {
    printf "${YELLOW}⚠ $1${NC}\n"
}

print_error() {
    printf "${RED}✗ $1${NC}\n"
}

# Usage
usage() {
    cat << EOF
INTcoin FreeBSD Installation Script

Usage: $0 [OPTIONS]

Options:
    -s, --source          Build from source instead of using binaries
    -d DIR                Install directory (default: /usr/local)
    -u USER               System user to run daemon (default: intcoin)
    --no-qt               Skip Qt GUI wallet installation
    --no-deps             Skip dependency installation
    --no-service          Don't create rc.d service
    -h, --help            Show this help message

Examples:
    $0                    # Standard installation
    $0 --source           # Build and install from source
    $0 --no-qt            # Install without GUI wallet
    $0 -d /opt/intcoin    # Custom install directory

EOF
    exit 0
}

# Parse arguments
while [ $# -gt 0 ]; do
    case $1 in
        -s|--source)
            BUILD_FROM_SOURCE=1
            shift
            ;;
        -d)
            INSTALL_DIR="$2"
            shift 2
            ;;
        -u)
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
            printf "Unknown option: $1\n"
            usage
            ;;
    esac
done

# Check if running as root
check_root() {
    if [ "$(id -u)" -ne 0 ]; then
        print_error "This script must be run as root"
        exit 1
    fi
}

# Install dependencies
install_dependencies() {
    print_step 1 6 "Installing dependencies..."

    pkg install -y \
        cmake \
        git \
        gmake \
        boost-libs \
        libevent \
        libsodium \
        pkgconf \
        autoconf \
        automake \
        libtool \
        db5

    if [ $INSTALL_QT -eq 1 ]; then
        pkg install -y \
            qt5-core \
            qt5-gui \
            qt5-widgets \
            qt5-network \
            qt5-buildtools \
            qt5-qmake \
            qt5-linguisttools \
            libqrencode
    fi

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
    gmake -j$(sysctl -n hw.ncpu)
    gmake install
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

    QT_FLAG="OFF"
    [ $INSTALL_QT -eq 1 ] && QT_FLAG="ON"

    cmake .. \
        -DCMAKE_BUILD_TYPE=Release \
        -DBUILD_QT_WALLET=$QT_FLAG \
        -DBUILD_DAEMON=ON \
        -DBUILD_CLI=ON \
        -DBUILD_MINER=ON \
        -DCMAKE_INSTALL_PREFIX=$INSTALL_DIR \
        -DCMAKE_PREFIX_PATH=/usr/local

    gmake -j$(sysctl -n hw.ncpu)
    gmake install

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

    if ! pw usershow "$SYSTEM_USER" >/dev/null 2>&1; then
        pw useradd "$SYSTEM_USER" -m -s /bin/sh -d /var/db/intcoin -c "INTcoin daemon user"
        print_success "User '$SYSTEM_USER' created"
    else
        print_warning "User '$SYSTEM_USER' already exists"
    fi

    # Create data directory
    mkdir -p /var/db/intcoin/.intcoin
    chown -R $SYSTEM_USER:$SYSTEM_USER /var/db/intcoin

    # Create log directory
    mkdir -p /var/log/intcoin
    chown -R $SYSTEM_USER:$SYSTEM_USER /var/log/intcoin
}

# Create rc.d service
create_rc_service() {
    print_step 5 6 "Creating rc.d service..."

    cat > /usr/local/etc/rc.d/intcoind << 'EOF'
#!/bin/sh
#
# PROVIDE: intcoind
# REQUIRE: LOGIN cleanvar
# KEYWORD: shutdown
#
# Add the following lines to /etc/rc.conf to enable intcoind:
#
# intcoind_enable="YES"
#

. /etc/rc.subr

name="intcoind"
rcvar=intcoind_enable

load_rc_config $name

: ${intcoind_enable:=NO}
: ${intcoind_user:=intcoin}
: ${intcoind_group:=intcoin}
: ${intcoind_datadir:=/var/db/intcoin/.intcoin}
: ${intcoind_config:=/var/db/intcoin/.intcoin/intcoin.conf}
: ${intcoind_pidfile:=/var/run/intcoind.pid}

command="/usr/local/bin/intcoind"
command_args="-daemon -conf=${intcoind_config} -datadir=${intcoind_datadir} -pid=${intcoind_pidfile}"
pidfile="${intcoind_pidfile}"

start_precmd="intcoind_precmd"
stop_cmd="intcoind_stop"

intcoind_precmd() {
    if [ ! -f "${intcoind_config}" ]; then
        err 1 "Configuration file ${intcoind_config} does not exist"
    fi
    install -o ${intcoind_user} -g ${intcoind_group} /dev/null ${pidfile}
}

intcoind_stop() {
    if [ -f "${pidfile}" ]; then
        /usr/local/bin/intcoin-cli stop
        wait_for_pids $(cat ${pidfile})
    fi
}

run_rc_command "$1"
EOF

    chmod 755 /usr/local/etc/rc.d/intcoind
    print_success "rc.d service created"
}

# Create default configuration
create_config() {
    print_step 6 6 "Creating default configuration..."

    if [ ! -f "/var/db/intcoin/.intcoin/intcoin.conf" ]; then
        # Generate random RPC password
        RPC_PASS=$(openssl rand -hex 32)

        cat > /var/db/intcoin/.intcoin/intcoin.conf << EOF
# INTcoin Configuration File

# Network settings
listen=1
maxconnections=125

# RPC settings
server=1
rpcuser=intcoinrpc
rpcpassword=$RPC_PASS
rpcallowip=127.0.0.1
rpcport=8332

# Mining (disabled by default)
gen=0

# Logging
debug=0
logtimestamps=1

# Data directory
datadir=/var/db/intcoin/.intcoin
EOF

        chown $SYSTEM_USER:$SYSTEM_USER /var/db/intcoin/.intcoin/intcoin.conf
        chmod 600 /var/db/intcoin/.intcoin/intcoin.conf

        print_success "Configuration created at /var/db/intcoin/.intcoin/intcoin.conf"
    else
        print_warning "Configuration already exists, skipping"
    fi
}

# Create desktop entry for Qt wallet
create_desktop_entry() {
    if [ $INSTALL_QT -eq 1 ]; then
        mkdir -p /usr/local/share/applications

        cat > /usr/local/share/applications/intcoin-qt.desktop << EOF
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

    printf "Install directory: ${CYAN}$INSTALL_DIR${NC}\n"
    printf "Build from source: ${CYAN}"
    [ $BUILD_FROM_SOURCE -eq 1 ] && printf "Yes" || printf "No"
    printf "${NC}\n"
    printf "Install Qt wallet: ${CYAN}"
    [ $INSTALL_QT -eq 1 ] && printf "Yes" || printf "No"
    printf "${NC}\n"
    printf "\n"

    printf "Proceed with installation? (y/N) "
    read -r REPLY
    case "$REPLY" in
        [Yy]*)
            # Continue
            ;;
        *)
            printf "Installation cancelled.\n"
            exit 0
            ;;
    esac

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
        create_rc_service
    fi

    create_config
    create_desktop_entry

    printf "\n"
    printf "${GREEN}================================${NC}\n"
    printf "${GREEN}Installation Complete!${NC}\n"
    printf "${GREEN}================================${NC}\n"
    printf "\n"
    printf "Installed binaries:\n"
    printf "  - ${CYAN}$INSTALL_DIR/bin/intcoind${NC} (daemon)\n"
    printf "  - ${CYAN}$INSTALL_DIR/bin/intcoin-cli${NC} (CLI)\n"
    [ $INSTALL_QT -eq 1 ] && printf "  - ${CYAN}$INSTALL_DIR/bin/intcoin-qt${NC} (GUI wallet)\n"
    printf "  - ${CYAN}$INSTALL_DIR/bin/intcoin-miner${NC} (miner)\n"
    printf "\n"
    printf "Configuration:\n"
    printf "  - ${CYAN}/var/db/intcoin/.intcoin/intcoin.conf${NC}\n"
    printf "\n"

    if [ $CREATE_SERVICE -eq 1 ]; then
        printf "To enable and start the daemon:\n"
        printf "  ${CYAN}echo 'intcoind_enable=\"YES\"' >> /etc/rc.conf${NC}\n"
        printf "  ${CYAN}service intcoind start${NC}\n"
        printf "\n"
    fi

    printf "To run the Qt wallet (as regular user):\n"
    printf "  ${CYAN}intcoin-qt${NC}\n"
    printf "\n"

    print_success "INTcoin installation complete!"
}

main "$@"
