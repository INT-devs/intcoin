#!/bin/sh
# INTcoin FreeBSD Installation Script
# Supports: FreeBSD 13.0+

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

error() { printf "${RED}ERROR: %s${NC}\n" "$1" >&2; exit 1; }
success() { printf "${GREEN}✓ %s${NC}\n" "$1"; }
info() { printf "${CYAN}%s${NC}\n" "$1"; }
warning() { printf "${YELLOW}⚠ %s${NC}\n" "$1"; }

printf "${CYAN}=========================================${NC}\n"
printf "${CYAN}INTcoin FreeBSD Installation Script${NC}\n"
printf "${CYAN}=========================================${NC}\n"
printf "\n"

# Detect FreeBSD version
if [ ! -f /bin/freebsd-version ]; then
    error "This script is for FreeBSD only"
fi

FREEBSD_VERSION=$(/bin/freebsd-version | cut -d'-' -f1)
FREEBSD_MAJOR=$(echo "$FREEBSD_VERSION" | cut -d'.' -f1)

info "Detected: FreeBSD $FREEBSD_VERSION"

# Enforce FreeBSD 13+ only (do not support out-of-date operating systems)
if [ "$FREEBSD_MAJOR" -lt 13 ]; then
    error "FreeBSD $FREEBSD_VERSION is not supported. Minimum required: FreeBSD 13.0+"
fi

printf "\n"

# Check if running as root
if [ "$(id -u)" -eq 0 ]; then
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
NUM_CORES=$(sysctl -n hw.ncpu)

info "Build Configuration:"
info "  Install Prefix: $INSTALL_PREFIX"
info "  Build Type: $BUILD_TYPE"
info "  Build Tests: $BUILD_TESTS"
info "  Build Qt Wallet: $BUILD_QT"
info "  CPU Cores: $NUM_CORES"
printf "\n"

# Install dependencies
install_dependencies() {
    info "Installing dependencies..."

    if [ "$INSTALL_AS_ROOT" = true ]; then
        PKG_CMD="pkg"
    else
        PKG_CMD="sudo pkg"
    fi

    # Update package repository
    $PKG_CMD update

    # Core dependencies
    $PKG_CMD install -y \
        cmake \
        ninja \
        git \
        pkgconf \
        openssl \
        rocksdb \
        curl \
        sqlite3 \
        nlohmann-json \
        autoconf \
        automake \
        libtool \
        gmake

    # Compiler (LLVM/Clang is default on FreeBSD)
    $PKG_CMD install -y llvm14

    # Qt dependencies (optional)
    if [ "$BUILD_QT" = "ON" ]; then
        $PKG_CMD install -y \
            qt5-buildtools \
            qt5-qmake \
            qt5-core \
            qt5-gui \
            qt5-widgets \
            qt5-network \
            libqrencode
    fi

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
        -DCMAKE_C_COMPILER=clang14 \
        -DCMAKE_CXX_COMPILER=clang++14 \
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
        -DCMAKE_C_COMPILER=clang14 \
        -DCMAKE_CXX_COMPILER=clang++14 \
        ..

    ninja
    ninja install

    success "RandomX built and installed"
}

# Build INTcoin
build_intcoin() {
    info "Building INTcoin..."

    SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
    BUILD_DIR="$SCRIPT_DIR/build-freebsd"

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
        -DCMAKE_C_COMPILER=clang14 \
        -DCMAKE_CXX_COMPILER=clang++14 \
        ..

    ninja -j$NUM_CORES

    if [ "$BUILD_TESTS" = "ON" ]; then
        info "Running tests..."
        ctest --output-on-failure || warning "Some tests failed"
    fi

    ninja install

    success "INTcoin built and installed"
}

# Create rc.d service
create_rc_service() {
    if [ "$INSTALL_AS_ROOT" = true ]; then
        info "Creating rc.d service..."

        # Create intcoin user
        if ! pw usershow intcoin > /dev/null 2>&1; then
            pw useradd intcoin -c "INTcoin Daemon" -d /var/db/intcoin -s /bin/sh
        fi

        # Create directories
        mkdir -p /usr/local/etc/intcoin
        mkdir -p /var/db/intcoin
        chown -R intcoin:intcoin /var/db/intcoin

        # Create default config
        if [ ! -f /usr/local/etc/intcoin/intcoin.conf ]; then
            cat > /usr/local/etc/intcoin/intcoin.conf <<EOF
# INTcoin Configuration
datadir=/var/db/intcoin
rpcuser=intcoinrpc
rpcpassword=$(openssl rand -hex 32)
rpcport=2211
port=2210
server=1
daemon=1
EOF
            chmod 600 /usr/local/etc/intcoin/intcoin.conf
            chown intcoin:intcoin /usr/local/etc/intcoin/intcoin.conf
        fi

        # Create rc.d script
        cat > /usr/local/etc/rc.d/intcoind <<'EOF'
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
rcvar="intcoind_enable"

load_rc_config $name

: ${intcoind_enable:="NO"}
: ${intcoind_user:="intcoin"}
: ${intcoind_group:="intcoin"}
: ${intcoind_datadir:="/var/db/intcoin"}
: ${intcoind_config:="/usr/local/etc/intcoin/intcoin.conf"}

command="/usr/local/bin/intcoind"
command_args="-daemon -conf=${intcoind_config} -datadir=${intcoind_datadir}"
pidfile="${intcoind_datadir}/intcoind.pid"

start_precmd="intcoind_prestart"
stop_cmd="intcoind_stop"

intcoind_prestart()
{
    if [ ! -d "${intcoind_datadir}" ]; then
        mkdir -p "${intcoind_datadir}"
        chown ${intcoind_user}:${intcoind_group} "${intcoind_datadir}"
    fi
}

intcoind_stop()
{
    /usr/local/bin/intcoin-cli stop
}

run_rc_command "$1"
EOF

        chmod 755 /usr/local/etc/rc.d/intcoind

        success "rc.d service created"
        info "To enable on boot: sysrc intcoind_enable=YES"
        info "To start service: service intcoind start"
        info "To view logs: tail -f /var/db/intcoin/debug.log"
    else
        info "Skipping rc.d service creation (not running as root)"
    fi
}

# Update PATH
update_path() {
    if [ "$INSTALL_AS_ROOT" = false ]; then
        BIN_DIR="$INSTALL_PREFIX/bin"

        # Add to PATH if not already there
        case ":$PATH:" in
            *":$BIN_DIR:"*) ;;
            *)
                info "Adding $BIN_DIR to PATH..."

                # Determine shell config file
                SHELL_NAME=$(basename "$SHELL")
                case "$SHELL_NAME" in
                    bash)
                        SHELL_RC="$HOME/.bashrc"
                        ;;
                    zsh)
                        SHELL_RC="$HOME/.zshrc"
                        ;;
                    tcsh|csh)
                        SHELL_RC="$HOME/.cshrc"
                        ;;
                    *)
                        SHELL_RC="$HOME/.profile"
                        ;;
                esac

                printf "\n" >> "$SHELL_RC"
                printf "# INTcoin\n" >> "$SHELL_RC"
                printf "export PATH=\"%s:\$PATH\"\n" "$BIN_DIR" >> "$SHELL_RC"

                success "Updated $SHELL_RC"
                info "Run: source $SHELL_RC  (or restart your shell)"
                ;;
        esac
    fi
}

# Main installation flow
main() {
    info "Starting installation..."
    printf "\n"

    # Install system dependencies
    install_dependencies
    printf "\n"

    # Build liboqs
    build_liboqs
    printf "\n"

    # Build RandomX
    build_randomx
    printf "\n"

    # Build INTcoin
    build_intcoin
    printf "\n"

    # Create rc.d service (if root)
    create_rc_service
    printf "\n"

    # Update PATH (if user install)
    update_path
    printf "\n"

    # Installation complete
    printf "${GREEN}=========================================${NC}\n"
    printf "${GREEN}Installation Complete!${NC}\n"
    printf "${GREEN}=========================================${NC}\n"
    printf "\n"
    printf "${CYAN}Installed executables:${NC}\n"
    printf "  - intcoind          : Full node daemon\n"
    printf "  - intcoin-cli       : Command-line interface\n"
    printf "  - intcoin-wallet    : Wallet management\n"
    printf "  - intcoin-miner     : Standalone miner\n"
    printf "  - intcoin-pool-server : Mining pool server\n"
    if [ "$BUILD_QT" = "ON" ]; then
        printf "  - intcoin-qt        : Qt graphical wallet\n"
    fi
    printf "\n"
    printf "${CYAN}Installation location:${NC} %s/bin\n" "$INSTALL_PREFIX"
    printf "\n"
    printf "${CYAN}Quick start:${NC}\n"
    if [ "$INSTALL_AS_ROOT" = true ]; then
        printf "  service intcoind start        # Start daemon\n"
        printf "  intcoin-cli getinfo           # Check status\n"
    else
        printf "  intcoind -daemon              # Start daemon\n"
        printf "  intcoin-cli getinfo           # Check status\n"
    fi
    printf "\n"
    printf "${CYAN}Documentation:${NC}\n"
    printf "  %s/docs/\n" "$(dirname "$0")"
    printf "\n"
    success "Done!"
}

# Run main installation
main
