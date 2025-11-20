#!/bin/bash
# Copyright (c) 2025 INTcoin Core (Maddison Lane)
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
#
# INTcoin Linux Installation Script
# Supports: Ubuntu, Debian, Fedora, CentOS, Arch Linux

set -e

echo "╔════════════════════════════════════════════╗"
echo "║   INTcoin Linux Installation Script     ║"
echo "╚════════════════════════════════════════════╝"
echo ""

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Check if running as root
if [ "$EUID" -eq 0 ]; then
    echo -e "${YELLOW}Warning: Running as root. Consider running as normal user with sudo.${NC}"
    echo ""
fi

# Detect Linux distribution
if [ -f /etc/os-release ]; then
    . /etc/os-release
    DISTRO=$ID
    VERSION=$VERSION_ID
else
    echo -e "${RED}Error: Cannot detect Linux distribution${NC}"
    exit 1
fi

echo "Detected: $NAME $VERSION"
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

# Parse arguments
while [[ $# -gt 0 ]]; do
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
        --binary)
            BUILD_FROM_SOURCE=false
            shift
            ;;
        --help)
            echo "Usage: $0 [OPTIONS]"
            echo ""
            echo "Options:"
            echo "  --prefix PATH      Install to PATH (default: /usr/local)"
            echo "  --system           Install system-wide to /usr"
            echo "  --no-lightning     Disable Lightning Network"
            echo "  --no-i2p           Disable I2P network"
            echo "  --no-ml            Disable Machine Learning"
            echo "  --no-gui           Disable Qt GUI"
            echo "  --binary           Install from pre-built binary (not implemented yet)"
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

if $INSTALL_SYSTEM_WIDE; then
    INSTALL_PREFIX="/usr"
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

# Install dependencies based on distribution
echo -e "${BLUE}Step 1: Installing dependencies...${NC}"

case $DISTRO in
    ubuntu|debian)
        echo "Installing dependencies for Ubuntu/Debian..."
        sudo apt update
        sudo apt install -y \
            build-essential \
            cmake \
            git \
            libboost-all-dev \
            libssl-dev \
            librocksdb-dev \
            liboqs-dev \
            python3 \
            pkg-config \
            libusb-1.0-0-dev \
            libhidapi-dev

        if [ "$ENABLE_GUI" = "ON" ]; then
            echo "Installing Qt for GUI..."
            sudo apt install -y \
                qtbase5-dev \
                qttools5-dev \
                qttools5-dev-tools
        fi
        ;;

    fedora|rhel|centos)
        echo "Installing dependencies for Fedora/RHEL/CentOS..."
        sudo dnf install -y \
            gcc \
            gcc-c++ \
            cmake \
            git \
            boost-devel \
            openssl-devel \
            rocksdb-devel \
            liboqs-devel \
            python3 \
            pkg-config \
            libusb-devel \
            hidapi-devel

        if [ "$ENABLE_GUI" = "ON" ]; then
            sudo dnf install -y \
                qt5-qtbase-devel \
                qt5-qttools-devel
        fi
        ;;

    arch|manjaro)
        echo "Installing dependencies for Arch Linux..."
        sudo pacman -Syu --noconfirm \
            base-devel \
            cmake \
            git \
            boost \
            openssl \
            rocksdb \
            liboqs \
            python \
            pkg-config \
            libusb \
            hidapi

        if [ "$ENABLE_GUI" = "ON" ]; then
            sudo pacman -S --noconfirm qt5-base qt5-tools
        fi
        ;;

    *)
        echo -e "${YELLOW}Warning: Unsupported distribution. Please install dependencies manually.${NC}"
        echo "Required packages:"
        echo "  - build-essential/gcc/g++"
        echo "  - cmake (>= 4.0)"
        echo "  - boost (>= 1.70)"
        echo "  - openssl (>= 3.0)"
        echo "  - rocksdb"
        echo "  - liboqs"
        echo "  - libusb-1.0"
        echo "  - hidapi"
        echo "  - qt5 (optional, for GUI)"
        echo ""
        read -p "Press Enter to continue anyway, or Ctrl+C to abort..."
        ;;
esac

echo -e "${GREEN}✓${NC} Dependencies installed"
echo ""

# Build from source
if $BUILD_FROM_SOURCE; then
    echo -e "${BLUE}Step 2: Building from source...${NC}"

    # Check if we're in the source directory
    if [ ! -f "CMakeLists.txt" ]; then
        echo -e "${RED}Error: Not in INTcoin source directory${NC}"
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
    NPROC=$(nproc)
    echo "Using $NPROC parallel jobs"
    make -j"$NPROC"

    echo ""
    echo -e "${GREEN}✓${NC} Build successful"
    echo ""

    # Run tests
    echo -e "${BLUE}Step 3: Running tests...${NC}"
    if [ -f "tests/test_crypto" ]; then
        echo "Running crypto tests..."
        ./tests/test_crypto || echo -e "${YELLOW}Warning: Some tests failed${NC}"
    fi

    # Install
    echo ""
    echo -e "${BLUE}Step 4: Installing...${NC}"
    sudo make install

    cd ..
else
    echo -e "${BLUE}Step 2: Installing from binary...${NC}"
    echo -e "${RED}Error: Binary installation not yet implemented${NC}"
    echo "Please use --build-from-source or omit --binary flag"
    exit 1
fi

echo -e "${GREEN}✓${NC} Installation complete"
echo ""

# Create config directory
echo -e "${BLUE}Step 5: Setting up configuration...${NC}"

CONFIG_DIR="$HOME/.intcoin"
mkdir -p "$CONFIG_DIR"

# Create default configuration file
if [ ! -f "$CONFIG_DIR/intcoin.conf" ]; then
    cat > "$CONFIG_DIR/intcoin.conf" <<EOF
# INTcoin Configuration File
# Generated by install_linux.sh

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

    echo -e "${GREEN}✓${NC} Created configuration: $CONFIG_DIR/intcoin.conf"
else
    echo "Configuration file already exists: $CONFIG_DIR/intcoin.conf"
fi

# Create systemd service (optional)
if command -v systemctl &> /dev/null; then
    echo ""
    read -p "Install systemd service? (y/N): " -n 1 -r
    echo
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        sudo tee /etc/systemd/system/intcoind.service > /dev/null <<EOF
[Unit]
Description=INTcoin Daemon
After=network.target

[Service]
Type=simple
User=$USER
ExecStart=$INSTALL_PREFIX/bin/intcoind
Restart=on-failure
RestartSec=5s

# Security hardening
PrivateTmp=true
ProtectSystem=full
NoNewPrivileges=true
ProtectHome=read-only

[Install]
WantedBy=multi-user.target
EOF

        sudo systemctl daemon-reload
        echo -e "${GREEN}✓${NC} Systemd service installed"
        echo "  Enable: sudo systemctl enable intcoind"
        echo "  Start:  sudo systemctl start intcoind"
        echo "  Status: sudo systemctl status intcoind"
    fi
fi

# Set up firewall rules
echo ""
read -p "Configure firewall rules? (y/N): " -n 1 -r
echo
if [[ $REPLY =~ ^[Yy]$ ]]; then
    if command -v ufw &> /dev/null; then
        echo "Configuring UFW firewall..."
        sudo ufw allow 9333/tcp comment "INTcoin P2P"
        sudo ufw allow 9335/tcp comment "INTcoin Lightning"
        echo -e "${GREEN}✓${NC} Firewall rules added"
    elif command -v firewall-cmd &> /dev/null; then
        echo "Configuring firewalld..."
        sudo firewall-cmd --permanent --add-port=9333/tcp
        sudo firewall-cmd --permanent --add-port=9335/tcp
        sudo firewall-cmd --reload
        echo -e "${GREEN}✓${NC} Firewall rules added"
    else
        echo -e "${YELLOW}Warning: No firewall detected (ufw/firewalld)${NC}"
        echo "Manually open ports 9333 and 9335"
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
echo "Documentation:"
echo "  Quick Start:   $PWD/QUICK-START.md"
echo "  Full docs:     $PWD/docs/"
echo ""
echo "Support:"
echo "  Website: https://international-coin.org"
echo "  Email:   team@international-coin.org"
echo ""
