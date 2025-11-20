#!/bin/sh
# Copyright (c) 2025 INTcoin Core (Maddison Lane)
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
#
# INTcoin FreeBSD Installation Script

set -e

echo "╔════════════════════════════════════════════╗"
echo "║   INTcoin FreeBSD Installation Script   ║"
echo "╚════════════════════════════════════════════╝"
echo ""

# Colors (FreeBSD sh compatible)
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Check if running as root
if [ "$(id -u)" -eq 0 ]; then
    printf "${YELLOW}Warning: Running as root. Consider running as normal user.${NC}\n"
    echo ""
fi

# Detect FreeBSD version
FREEBSD_VERSION=$(freebsd-version)
echo "Detected: FreeBSD $FREEBSD_VERSION"
echo ""

# Configuration
INSTALL_PREFIX="/usr/local"
BUILD_TYPE="Release"
ENABLE_LIGHTNING="ON"
ENABLE_I2P="ON"
ENABLE_ML="ON"
ENABLE_GUI="ON"
INSTALL_SYSTEM_WIDE=false
BUILD_FROM_SOURCE=true

# Parse arguments (POSIX-compatible)
while [ $# -gt 0 ]; do
    case $1 in
        --prefix)
            INSTALL_PREFIX="$2"
            shift 2
            ;;
        --system)
            INSTALL_SYSTEM_WIDE=true
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
        --no-gui)
            ENABLE_GUI="OFF"
            shift
            ;;
        --help)
            echo "Usage: $0 [OPTIONS]"
            echo ""
            echo "Options:"
            echo "  --prefix PATH      Install to PATH (default: /usr/local)"
            echo "  --system           Install system-wide"
            echo "  --no-lightning     Disable Lightning Network"
            echo "  --no-i2p           Disable I2P network"
            echo "  --no-ml            Disable Machine Learning"
            echo "  --no-gui           Disable Qt GUI"
            echo "  --help             Show this help"
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            echo "Use --help for usage information"
            exit 1
            ;;
    esac
done

if [ "$INSTALL_SYSTEM_WIDE" = "true" ]; then
    INSTALL_PREFIX="/usr/local"
fi

echo "Installation Configuration:"
echo "  Install Prefix: $INSTALL_PREFIX"
echo "  Build Type: $BUILD_TYPE"
echo "  Lightning Network: $ENABLE_LIGHTNING"
echo "  I2P Network: $ENABLE_I2P"
echo "  Machine Learning: $ENABLE_ML"
echo "  Qt GUI: $ENABLE_GUI"
echo "  Build from Source: $BUILD_FROM_SOURCE"
echo ""

# Install dependencies using pkg
printf "${BLUE}Step 1: Installing dependencies...${NC}\n"

echo "Updating pkg database..."
pkg update

echo "Installing build dependencies..."
pkg install -y \
    cmake \
    git \
    boost-all \
    openssl \
    rocksdb \
    liboqs \
    python3 \
    pkgconf \
    libusb \
    hidapi

if [ "$ENABLE_GUI" = "ON" ]; then
    echo "Installing Qt5 for GUI..."
    pkg install -y \
        qt5-buildtools \
        qt5-qmake \
        qt5-core \
        qt5-gui \
        qt5-widgets \
        qt5-network
fi

printf "${GREEN}✓${NC} Dependencies installed\n"
echo ""

# Build from source
if [ "$BUILD_FROM_SOURCE" = "true" ]; then
    printf "${BLUE}Step 2: Building from source...${NC}\n"

    # Check if we're in the source directory
    if [ ! -f "CMakeLists.txt" ]; then
        printf "${RED}Error: Not in INTcoin source directory${NC}\n"
        echo "Please cd to the intcoin directory first"
        exit 1
    fi

    # Create build directory
    BUILD_DIR="build"
    if [ -d "$BUILD_DIR" ]; then
        echo "Cleaning previous build..."
        rm -rf "$BUILD_DIR"
    fi

    mkdir -p "$BUILD_DIR"
    cd "$BUILD_DIR"

    # Configure
    echo "Configuring build..."
    cmake .. \
        -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
        -DCMAKE_INSTALL_PREFIX="$INSTALL_PREFIX" \
        -DENABLE_LIGHTNING="$ENABLE_LIGHTNING" \
        -DENABLE_I2P="$ENABLE_I2P" \
        -DENABLE_ML="$ENABLE_ML" \
        -DBUILD_QT_WALLET="$ENABLE_GUI" \
        -DBUILD_TESTS=ON

    # Build
    echo ""
    echo "Building INTcoin..."
    NPROC=$(sysctl -n hw.ncpu)
    echo "Using $NPROC parallel jobs"
    gmake -j"$NPROC" || make -j"$NPROC"

    echo ""
    printf "${GREEN}✓${NC} Build successful\n"
    echo ""

    # Run tests
    printf "${BLUE}Step 3: Running tests...${NC}\n"
    if [ -f "tests/test_crypto" ]; then
        echo "Running crypto tests..."
        ./tests/test_crypto || echo "${YELLOW}Warning: Some tests failed${NC}"
    fi

    # Install
    echo ""
    printf "${BLUE}Step 4: Installing...${NC}\n"
    gmake install || make install

    cd ..
fi

printf "${GREEN}✓${NC} Installation complete\n"
echo ""

# Create config directory
printf "${BLUE}Step 5: Setting up configuration...${NC}\n"

CONFIG_DIR="$HOME/.intcoin"
mkdir -p "$CONFIG_DIR"

# Create default configuration file
if [ ! -f "$CONFIG_DIR/intcoin.conf" ]; then
    cat > "$CONFIG_DIR/intcoin.conf" <<EOF
# INTcoin Configuration File (FreeBSD)
# Generated by install_freebsd.sh

[network]
port=9333
maxconnections=125

[rpc]
port=9334
user=intcoin
password=$(openssl rand -hex 32)
allowip=127.0.0.1

[lightning]
enabled=$( [ "$ENABLE_LIGHTNING" = "ON" ] && echo "1" || echo "0" )
port=9335

[i2p]
enabled=$( [ "$ENABLE_I2P" = "ON" ] && echo "1" || echo "0" )
sam.port=9336
router.port=9337

[ml]
enabled=$( [ "$ENABLE_ML" = "ON" ] && echo "1" || echo "0" )
enable_fee_prediction=1
enable_anomaly_detection=1
EOF

    printf "${GREEN}✓${NC} Created configuration: $CONFIG_DIR/intcoin.conf\n"
else
    echo "Configuration file already exists: $CONFIG_DIR/intcoin.conf"
fi

# Create rc.d service script (FreeBSD service)
echo ""
printf "Install rc.d service? (y/N): "
read -r REPLY
if [ "$REPLY" = "y" ] || [ "$REPLY" = "Y" ]; then
    cat > /tmp/intcoind <<EOF
#!/bin/sh
#
# PROVIDE: intcoind
# REQUIRE: DAEMON
# KEYWORD: shutdown
#
# Add the following lines to /etc/rc.conf to enable intcoind:
# intcoind_enable="YES"
# intcoind_user="$USER"
#

. /etc/rc.subr

name="intcoind"
rcvar=intcoind_enable

load_rc_config \$name

: \${intcoind_enable:=NO}
: \${intcoind_user:=$USER}
: \${intcoind_group:=$USER}
: \${intcoind_config:=$CONFIG_DIR/intcoin.conf}
: \${intcoind_data:=$CONFIG_DIR}

pidfile="\${intcoind_data}/intcoind.pid"
command="$INSTALL_PREFIX/bin/intcoind"
command_args="-daemon -pid=\${pidfile} -conf=\${intcoind_config} -datadir=\${intcoind_data}"

run_rc_command "\$1"
EOF

    chmod +x /tmp/intcoind
    mv /tmp/intcoind /usr/local/etc/rc.d/intcoind

    printf "${GREEN}✓${NC} rc.d service installed\n"
    echo "  Enable: sysrc intcoind_enable=YES"
    echo "  Start:  service intcoind start"
    echo "  Status: service intcoind status"
fi

# Set up firewall rules
echo ""
printf "Configure firewall rules (IPFW)? (y/N): "
read -r REPLY
if [ "$REPLY" = "y" ] || [ "$REPLY" = "Y" ]; then
    echo "Adding IPFW rules..."

    # Check if IPFW is enabled
    if sysrc -n firewall_enable 2>/dev/null | grep -q "YES"; then
        # Add rules to /etc/rc.firewall.local
        if [ ! -f /etc/rc.firewall.local ]; then
            cat > /etc/rc.firewall.local <<'EOF'
#!/bin/sh
# INTcoin firewall rules
EOF
            chmod +x /etc/rc.firewall.local
        fi

        cat >> /etc/rc.firewall.local <<EOF

# INTcoin P2P
\${fwcmd} add allow tcp from any to me 9333 in

# INTcoin Lightning
\${fwcmd} add allow tcp from any to me 9335 in
EOF

        # Reload firewall
        service ipfw restart
        printf "${GREEN}✓${NC} Firewall rules added\n"
    else
        printf "${YELLOW}Warning: IPFW not enabled. Enable with:${NC}\n"
        echo "  sysrc firewall_enable=YES"
        echo "  sysrc firewall_type=workstation"
        echo "  service ipfw start"
    fi
fi

# Set up PF firewall (alternative)
echo ""
printf "Configure PF firewall? (y/N): "
read -r REPLY
if [ "$REPLY" = "y" ] || [ "$REPLY" = "Y" ]; then
    if [ ! -f /etc/pf.conf.intcoin ]; then
        cat > /etc/pf.conf.intcoin <<'EOF'
# INTcoin PF rules
# Add to /etc/pf.conf:
# include "/etc/pf.conf.intcoin"

# INTcoin P2P
pass in proto tcp to port 9333

# INTcoin Lightning
pass in proto tcp to port 9335
EOF

        printf "${GREEN}✓${NC} PF rules template created: /etc/pf.conf.intcoin\n"
        echo "Add to /etc/pf.conf:"
        echo "  echo 'include \"/etc/pf.conf.intcoin\"' >> /etc/pf.conf"
        echo "  pfctl -f /etc/pf.conf"
    fi
fi

echo ""
echo "╔════════════════════════════════════════════╗"
echo "║   Installation Complete!                 ║"
echo "╚════════════════════════════════════════════╝"
echo ""
echo "Installed binaries:"
ls -1 "$INSTALL_PREFIX/bin/intcoin"* 2>/dev/null || echo "  (check $INSTALL_PREFIX/bin/)"
echo ""
echo "Configuration: $CONFIG_DIR/intcoin.conf"
echo "Data directory: $CONFIG_DIR/"
echo ""
echo "Quick Start:"
echo "  Start daemon:  intcoind"
if [ "$ENABLE_GUI" = "ON" ]; then
    echo "  Start GUI:     intcoin-qt"
fi
echo "  CLI commands:  intcoin-cli help"
echo ""
echo "FreeBSD Specific:"
echo "  Service:       service intcoind start"
echo "  Enable boot:   sysrc intcoind_enable=YES"
echo "  View logs:     tail -f $CONFIG_DIR/debug.log"
echo ""
echo "Documentation:"
echo "  Quick Start:   $PWD/QUICK-START.md"
echo "  Full docs:     $PWD/docs/"
echo ""
echo "Support:"
echo "  Website: https://international-coin.org"
echo "  Email:   team@international-coin.org"
echo ""
