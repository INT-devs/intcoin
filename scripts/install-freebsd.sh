#!/bin/sh
# Copyright (c) 2025 INTcoin Core (Maddison Lane)
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

# INTcoin Installation Script for FreeBSD
# Supports: FreeBSD 13.0+

set -e

# Colors for output (FreeBSD sh compatible)
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m'

# Configuration
INSTALL_DIR="/usr/local/bin"
DATA_DIR="$HOME/.intcoin"
RC_FILE="/usr/local/etc/rc.d/intcoind"

# Print functions
print_header() {
    printf "${CYAN}================================${NC}\n"
    printf "${CYAN}%s${NC}\n" "$1"
    printf "${CYAN}================================${NC}\n"
}

print_step() {
    printf "${YELLOW}[%s] %s${NC}\n" "$1" "$2"
}

print_success() {
    printf "${GREEN}✓ %s${NC}\n" "$1"
}

print_error() {
    printf "${RED}✗ %s${NC}\n" "$1"
}

print_info() {
    printf "${BLUE}ℹ %s${NC}\n" "$1"
}

# Check if running as root
check_root() {
    if [ "$(id -u)" -eq 0 ]; then
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
    if [ "$(uname -s)" != "FreeBSD" ]; then
        print_error "This script is for FreeBSD only. Detected: $(uname -s)"
        print_info "Use install-debian.sh for Debian/Ubuntu, install-fedora.sh for Fedora/RHEL"
        exit 1
    fi

    FREEBSD_VERSION=$(freebsd-version | cut -d'-' -f1 | cut -d'.' -f1)
    if [ "$FREEBSD_VERSION" -lt 13 ]; then
        print_error "FreeBSD 13.0 or later is required. Detected: $(freebsd-version)"
        exit 1
    fi

    print_success "Detected: FreeBSD $(freebsd-version)"
}

# Install dependencies
install_dependencies() {
    print_step "2/8" "Installing dependencies..."

    # Update pkg database
    sudo pkg update -q

    # Build tools
    sudo pkg install -y -q \
        cmake \
        git \
        pkgconf \
        autoconf \
        automake \
        libtool \
        gmake \
        gcc \
        curl \
        wget

    # Libraries
    sudo pkg install -y -q \
        boost-all \
        openssl \
        libevent \
        miniupnpc \
        libzmq4 \
        sqlite3

    # Qt5 (for GUI wallet)
    sudo pkg install -y -q \
        qt5-buildtools \
        qt5-qmake \
        qt5-core \
        qt5-gui \
        qt5-widgets \
        qt5-network \
        qt5-svg

    # Python (for tests)
    sudo pkg install -y -q \
        python3 \
        py39-pip

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
        gmake -j$(sysctl -n hw.ncpu)
        sudo gmake install
        sudo ldconfig

        # Cleanup
        cd "$HOME"
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
    gmake -j$(sysctl -n hw.ncpu)

    print_success "Build complete"
}

# Run tests
run_tests() {
    print_step "5/8" "Running tests..."

    if gmake test; then
        print_success "All tests passed"
    else
        print_error "Some tests failed (non-critical, continuing installation)"
    fi
}

# Install binaries
install_binaries() {
    print_step "6/8" "Installing binaries..."

    sudo install -m 0755 -o root -g wheel intcoind "$INSTALL_DIR/"
    sudo install -m 0755 -o root -g wheel intcoin-cli "$INSTALL_DIR/"
    sudo install -m 0755 -o root -g wheel intcoin-wallet "$INSTALL_DIR/"
    sudo install -m 0755 -o root -g wheel intcoin-miner "$INSTALL_DIR/"

    if [ -f "intcoin-qt" ]; then
        sudo install -m 0755 -o root -g wheel intcoin-qt "$INSTALL_DIR/"
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
rpcport=9332

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

# Create rc.d service script
create_service() {
    print_step "8/8" "Creating rc.d service script..."

    sudo tee "$RC_FILE" > /dev/null << 'EOF'
#!/bin/sh

# PROVIDE: intcoind
# REQUIRE: LOGIN cleanvar
# KEYWORD: shutdown

#
# Add the following lines to /etc/rc.conf to enable intcoind:
#
# intcoind_enable="YES"
# intcoind_user="<username>"  # User to run intcoind as
#

. /etc/rc.subr

name="intcoind"
rcvar=intcoind_enable

load_rc_config $name

: ${intcoind_enable:="NO"}
: ${intcoind_user:="intcoin"}
: ${intcoind_group:="intcoin"}
: ${intcoind_config:="/home/${intcoind_user}/.intcoin/intcoin.conf"}
: ${intcoind_datadir:="/home/${intcoind_user}/.intcoin"}

pidfile="/var/run/${name}.pid"
command="/usr/local/bin/intcoind"
command_args="-daemon -conf=${intcoind_config} -datadir=${intcoind_datadir} -pid=${pidfile}"
start_cmd="intcoind_start"
stop_cmd="intcoind_stop"

intcoind_start()
{
    echo "Starting ${name}..."
    su -m ${intcoind_user} -c "${command} ${command_args}"
}

intcoind_stop()
{
    echo "Stopping ${name}..."
    su -m ${intcoind_user} -c "/usr/local/bin/intcoin-cli stop"
    wait_for_pids $(cat ${pidfile} 2>/dev/null)
}

run_rc_command "$1"
EOF

    sudo chmod +x "$RC_FILE"
    print_success "RC service script created"
    print_info "To enable at boot: Add intcoind_enable=\"YES\" to /etc/rc.conf"
    print_info "Set user: Add intcoind_user=\"$USER\" to /etc/rc.conf"
    print_info "To start now: sudo service intcoind start"
}

# Print summary
print_summary() {
    printf "\n"
    print_header "Installation Complete!"
    printf "\n"
    printf "${GREEN}INTcoin has been successfully installed!${NC}\n"
    printf "\n"
    printf "${YELLOW}Installed binaries:${NC}\n"
    printf "  • intcoind         - Daemon\n"
    printf "  • intcoin-cli      - Command-line interface\n"
    printf "  • intcoin-wallet   - Wallet management tool\n"
    printf "  • intcoin-miner    - CPU miner\n"
    if [ -f "$INSTALL_DIR/intcoin-qt" ]; then
        printf "  • intcoin-qt       - GUI wallet\n"
    fi
    printf "\n"
    printf "${YELLOW}Configuration:${NC}\n"
    printf "  • Data directory: %s\n" "$DATA_DIR"
    printf "  • Config file: %s/intcoin.conf\n" "$DATA_DIR"
    printf "\n"
    printf "${YELLOW}Quick start commands:${NC}\n"
    printf "  # Start GUI wallet\n"
    printf "  $ intcoin-qt\n"
    printf "\n"
    printf "  # Or start daemon\n"
    printf "  $ intcoind -daemon\n"
    printf "\n"
    printf "  # Check status\n"
    printf "  $ intcoin-cli getblockchaininfo\n"
    printf "\n"
    printf "  # Get new address\n"
    printf "  $ intcoin-cli getnewaddress\n"
    printf "\n"
    printf "  # Start mining\n"
    printf "  $ intcoin-cli startmining 4\n"
    printf "\n"
    printf "${YELLOW}RC service (FreeBSD):${NC}\n"
    printf "  # Add to /etc/rc.conf:\n"
    printf "  intcoind_enable=\"YES\"\n"
    printf "  intcoind_user=\"%s\"\n" "$USER"
    printf "\n"
    printf "  # Start service\n"
    printf "  $ sudo service intcoind start\n"
    printf "\n"
    printf "  # Check status\n"
    printf "  $ sudo service intcoind status\n"
    printf "\n"
    printf "${YELLOW}Documentation:${NC}\n"
    printf "  • Website: https://international-coin.org\n"
    printf "  • Repository: https://gitlab.com/intcoin/crypto\n"
    printf "  • Wiki: https://gitlab.com/intcoin/crypto/-/wikis/home\n"
    printf "\n"
    printf "${GREEN}Thank you for installing INTcoin!${NC}\n"
    printf "\n"
}

# Cleanup function
cleanup() {
    cd "$HOME"
    if [ -d "intcoin-build" ]; then
        print_info "Build directory preserved at: ~/intcoin-build"
        print_info "You can safely remove it with: rm -rf ~/intcoin-build"
    fi
}

# Main installation flow
main() {
    print_header "INTcoin Installation Script"
    printf "\n"
    print_info "This script will install INTcoin on your system"
    print_info "Supported: FreeBSD 13.0+"
    printf "\n"

    print_step "1/8" "Checking system requirements..."
    check_root
    check_sudo
    detect_os

    # Change to home directory for build
    cd "$HOME"

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
