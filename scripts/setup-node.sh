#!/bin/bash
# INTcoin Node Setup Script v1.4.1
# Complete mainnet and testnet installation for Ubuntu 24.04 / Debian 13+
# Excludes: Qt wallet, Tor, I2P
#
# Usage: sudo ./setup-node.sh [options]
#   --mainnet-only    Only install mainnet node
#   --testnet-only    Only install testnet node (includes faucet)
#   --with-mining     Enable CPU mining on testnet
#   --user USERNAME   Specify user (default: current sudo user or 'intcoin')
#   --skip-deps       Skip dependency installation (use if already installed)
#   --help            Show this help message

set -e

# =============================================================================
# Configuration
# =============================================================================

VERSION="1.4.2"
LIBOQS_VERSION="0.10.0"
RANDOMX_VERSION="v1.2.1"
INTCOIN_REPO="https://github.com/INT-devs/intcoin.git"

# Default ports
MAINNET_P2P_PORT=2212
MAINNET_RPC_PORT=2213
TESTNET_P2P_PORT=12212
TESTNET_RPC_PORT=12213
FAUCET_HTTP_PORT=8080

# Default settings
INSTALL_MAINNET=true
INSTALL_TESTNET=true
ENABLE_MINING=false
SKIP_DEPS=false
INTCOIN_USER=""

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# =============================================================================
# Helper Functions
# =============================================================================

log_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

log_success() {
    echo -e "${GREEN}[OK]${NC} $1"
}

log_warn() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

generate_password() {
    # Generate a secure 48-character hex password
    openssl rand -hex 24
}

show_help() {
    echo "INTcoin Node Setup Script v${VERSION}"
    echo ""
    echo "Usage: sudo ./setup-node.sh [options]"
    echo ""
    echo "Options:"
    echo "  --mainnet-only    Only install mainnet node"
    echo "  --testnet-only    Only install testnet node (includes faucet)"
    echo "  --with-mining     Enable CPU mining on testnet"
    echo "  --user USERNAME   Specify user (default: current sudo user or 'intcoin')"
    echo "  --skip-deps       Skip dependency installation"
    echo "  --help            Show this help message"
    echo ""
    echo "Examples:"
    echo "  sudo ./setup-node.sh                    # Full installation"
    echo "  sudo ./setup-node.sh --testnet-only    # Testnet only with faucet"
    echo "  sudo ./setup-node.sh --with-mining     # Enable testnet mining"
    exit 0
}

check_root() {
    if [ "$EUID" -ne 0 ]; then
        log_error "Please run with sudo: sudo ./setup-node.sh"
        exit 1
    fi
}

detect_os() {
    if [ -f /etc/os-release ]; then
        . /etc/os-release
        OS=$ID
        OS_VERSION=$VERSION_ID
    else
        log_error "Cannot detect OS. This script supports Ubuntu 24.04+ and Debian 13+"
        exit 1
    fi

    case $OS in
        ubuntu)
            if [[ "${OS_VERSION%%.*}" -lt 24 ]]; then
                log_warn "Ubuntu $OS_VERSION detected. Recommended: Ubuntu 24.04+"
            fi
            ;;
        debian)
            if [[ "${OS_VERSION%%.*}" -lt 13 ]]; then
                log_warn "Debian $OS_VERSION detected. Recommended: Debian 13+"
            fi
            ;;
        *)
            log_warn "Unsupported OS: $OS. Proceeding anyway..."
            ;;
    esac

    log_info "Detected OS: $OS $OS_VERSION"
}

# =============================================================================
# Parse Arguments
# =============================================================================

while [[ $# -gt 0 ]]; do
    case $1 in
        --mainnet-only)
            INSTALL_MAINNET=true
            INSTALL_TESTNET=false
            shift
            ;;
        --testnet-only)
            INSTALL_MAINNET=false
            INSTALL_TESTNET=true
            shift
            ;;
        --with-mining)
            ENABLE_MINING=true
            shift
            ;;
        --user)
            INTCOIN_USER="$2"
            shift 2
            ;;
        --skip-deps)
            SKIP_DEPS=true
            shift
            ;;
        --help|-h)
            show_help
            ;;
        *)
            log_error "Unknown option: $1"
            show_help
            ;;
    esac
done

# =============================================================================
# Main Installation
# =============================================================================

echo ""
echo "=============================================="
echo "  INTcoin Node Setup Script v${VERSION}"
echo "=============================================="
echo ""

check_root
detect_os

# Determine user
if [ -z "$INTCOIN_USER" ]; then
    INTCOIN_USER="${SUDO_USER:-intcoin}"
fi

# Create user if doesn't exist
if ! id "$INTCOIN_USER" &>/dev/null; then
    log_info "Creating user: $INTCOIN_USER"
    useradd -m -s /bin/bash "$INTCOIN_USER"
    log_success "User $INTCOIN_USER created"
fi

HOME_DIR=$(eval echo ~$INTCOIN_USER)
log_info "Installation user: $INTCOIN_USER"
log_info "Home directory: $HOME_DIR"
log_info "Install mainnet: $INSTALL_MAINNET"
log_info "Install testnet: $INSTALL_TESTNET"
log_info "Enable mining: $ENABLE_MINING"
echo ""

# Generate RPC passwords
MAINNET_RPC_PASSWORD=$(generate_password)
TESTNET_RPC_PASSWORD=$(generate_password)

# -----------------------------------------------------------------------------
# Step 1: Install Dependencies
# -----------------------------------------------------------------------------

if [ "$SKIP_DEPS" = false ]; then
    echo "=============================================="
    echo "[1/9] Installing build dependencies..."
    echo "=============================================="

    apt-get update
    apt-get install -y \
        build-essential \
        cmake \
        git \
        libboost-all-dev \
        libssl-dev \
        pkg-config \
        librocksdb-dev \
        ninja-build \
        curl \
        wget \
        libcurl4-openssl-dev \
        libjsoncpp-dev \
        libsodium-dev \
        libzmq3-dev \
        libevent-dev \
        jq \
        logrotate

    log_success "Build dependencies installed"

    # -----------------------------------------------------------------------------
    # Step 2: Install GCC 13
    # -----------------------------------------------------------------------------

    echo ""
    echo "=============================================="
    echo "[2/9] Installing GCC 13 for C++23 support..."
    echo "=============================================="

    apt-get install -y gcc-13 g++-13
    update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-13 130 \
        --slave /usr/bin/g++ g++ /usr/bin/g++-13

    GCC_VERSION=$(gcc --version | head -n1)
    log_success "GCC installed: $GCC_VERSION"

    # -----------------------------------------------------------------------------
    # Step 3: Build liboqs
    # -----------------------------------------------------------------------------

    echo ""
    echo "=============================================="
    echo "[3/9] Building liboqs (post-quantum crypto)..."
    echo "=============================================="

    cd /tmp
    if [ -d "liboqs" ]; then
        log_info "Removing existing liboqs directory..."
        rm -rf liboqs
    fi

    git clone --depth 1 --branch $LIBOQS_VERSION https://github.com/open-quantum-safe/liboqs.git
    cd liboqs
    mkdir -p build && cd build
    cmake -GNinja \
        -DCMAKE_INSTALL_PREFIX=/usr/local \
        -DBUILD_SHARED_LIBS=ON \
        -DOQS_BUILD_ONLY_LIB=ON \
        ..
    ninja
    ninja install
    ldconfig

    log_success "liboqs $LIBOQS_VERSION installed"

    # -----------------------------------------------------------------------------
    # Step 4: Build RandomX
    # -----------------------------------------------------------------------------

    echo ""
    echo "=============================================="
    echo "[4/9] Building RandomX (PoW algorithm)..."
    echo "=============================================="

    cd /tmp
    if [ -d "RandomX" ]; then
        log_info "Removing existing RandomX directory..."
        rm -rf RandomX
    fi

    git clone --depth 1 --branch $RANDOMX_VERSION https://github.com/tevador/RandomX.git
    cd RandomX
    mkdir -p build && cd build
    cmake -DCMAKE_INSTALL_PREFIX=/usr/local ..
    make -j$(nproc)
    make install
    ldconfig

    log_success "RandomX $RANDOMX_VERSION installed"
else
    log_info "Skipping dependency installation (--skip-deps)"
fi

# -----------------------------------------------------------------------------
# Step 5: Clone and Build INTcoin
# -----------------------------------------------------------------------------

echo ""
echo "=============================================="
echo "[5/9] Building INTcoin..."
echo "=============================================="

cd $HOME_DIR
if [ -d "intcoin" ]; then
    log_info "Updating existing INTcoin repository..."
    cd intcoin
    git fetch origin
    git reset --hard origin/main
else
    log_info "Cloning INTcoin repository..."
    git clone $INTCOIN_REPO
    cd intcoin
fi

# Clean previous build
rm -rf build
mkdir -p build && cd build

log_info "Configuring CMake..."
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_WALLET_QT=OFF \
    -DBUILD_MINER=OFF \
    -DBUILD_TESTS=OFF \
    -DBUILD_FAUCET=ON \
    -DENABLE_TOR=OFF \
    -DENABLE_I2P=OFF \
    -DWARNINGS_AS_ERRORS=OFF

log_info "Compiling (using $(nproc) cores)..."
make -j$(nproc)

log_success "INTcoin compiled successfully"

# -----------------------------------------------------------------------------
# Step 6: Install Binaries
# -----------------------------------------------------------------------------

echo ""
echo "=============================================="
echo "[6/9] Installing binaries..."
echo "=============================================="

# Stop services if running (with timeout and force-kill fallback)
stop_service() {
    local service=$1
    if systemctl is-active --quiet "$service" 2>/dev/null; then
        log_info "Stopping $service..."
        timeout 10 systemctl stop "$service" 2>/dev/null || {
            log_warn "$service didn't stop gracefully, force killing..."
            local pid=$(systemctl show -p MainPID --value "$service" 2>/dev/null)
            if [ -n "$pid" ] && [ "$pid" != "0" ]; then
                kill -9 "$pid" 2>/dev/null || true
            fi
            systemctl reset-failed "$service" 2>/dev/null || true
        }
    fi
}

stop_service intcoind-mainnet
stop_service intcoind-testnet
stop_service intcoin-faucet

# Also kill any orphaned processes
pkill -9 -f "intcoind" 2>/dev/null || true
pkill -9 -f "intcoin-faucet" 2>/dev/null || true
sleep 2

# Install binaries
cp bin/intcoind /usr/local/bin/
cp bin/intcoin-cli /usr/local/bin/
cp bin/intcoin-faucet /usr/local/bin/

chown root:root /usr/local/bin/intcoind /usr/local/bin/intcoin-cli /usr/local/bin/intcoin-faucet
chmod 755 /usr/local/bin/intcoind /usr/local/bin/intcoin-cli /usr/local/bin/intcoin-faucet

# Verify installation
INTCOIN_VERSION=$(/usr/local/bin/intcoind --version 2>/dev/null || echo "unknown")
log_success "Installed: intcoind, intcoin-cli, intcoin-faucet"

# -----------------------------------------------------------------------------
# Step 7: Create Data Directories and Configs
# -----------------------------------------------------------------------------

echo ""
echo "=============================================="
echo "[7/9] Creating data directories and configs..."
echo "=============================================="

# Mainnet setup
if [ "$INSTALL_MAINNET" = true ]; then
    log_info "Setting up mainnet..."

    mkdir -p $HOME_DIR/.intcoin-mainnet
    mkdir -p $HOME_DIR/.intcoin-mainnet/contracts

    cat > $HOME_DIR/.intcoin-mainnet/intcoin.conf << EOF
# INTcoin Mainnet Configuration
# Generated by setup-node.sh v${VERSION} on $(date)

# Network
network=mainnet
datadir=$HOME_DIR/.intcoin-mainnet

# RPC Settings
rpcuser=intcoinrpc
rpcpassword=${MAINNET_RPC_PASSWORD}
rpcbind=127.0.0.1
rpcport=${MAINNET_RPC_PORT}
rpcallowip=127.0.0.1

# P2P Settings
port=${MAINNET_P2P_PORT}
maxconnections=125
listen=1

# Performance
dbcache=512
maxmempool=300

# Logging
debug=0
printtoconsole=0
logfile=$HOME_DIR/.intcoin-mainnet/debug.log

# Security
disablewallet=0
EOF

    chown -R $INTCOIN_USER:$INTCOIN_USER $HOME_DIR/.intcoin-mainnet
    chmod 700 $HOME_DIR/.intcoin-mainnet
    chmod 600 $HOME_DIR/.intcoin-mainnet/intcoin.conf

    log_success "Mainnet configuration created"
fi

# Testnet setup
if [ "$INSTALL_TESTNET" = true ]; then
    log_info "Setting up testnet..."

    mkdir -p $HOME_DIR/.intcoin-testnet
    mkdir -p $HOME_DIR/.intcoin-testnet/contracts

    cat > $HOME_DIR/.intcoin-testnet/intcoin.conf << EOF
# INTcoin Testnet Configuration
# Generated by setup-node.sh v${VERSION} on $(date)

# Network
network=testnet
testnet=1
datadir=$HOME_DIR/.intcoin-testnet

# RPC Settings
rpcuser=intcoinrpc
rpcpassword=${TESTNET_RPC_PASSWORD}
rpcbind=127.0.0.1
rpcport=${TESTNET_RPC_PORT}
rpcallowip=127.0.0.1

# P2P Settings
port=${TESTNET_P2P_PORT}
maxconnections=50
listen=1

# Performance
dbcache=256
maxmempool=100

# Logging
debug=1
printtoconsole=0
logfile=$HOME_DIR/.intcoin-testnet/debug.log

# Security
disablewallet=0
EOF

    # Add mining settings if enabled
    if [ "$ENABLE_MINING" = true ]; then
        cat >> $HOME_DIR/.intcoin-testnet/intcoin.conf << EOF

# Mining (CPU)
gen=1
genproclimit=1
EOF
        log_info "Mining enabled in testnet config"
    fi

    chown -R $INTCOIN_USER:$INTCOIN_USER $HOME_DIR/.intcoin-testnet
    chmod 700 $HOME_DIR/.intcoin-testnet
    chmod 600 $HOME_DIR/.intcoin-testnet/intcoin.conf

    log_success "Testnet configuration created"
fi

# -----------------------------------------------------------------------------
# Step 8: Create Systemd Services
# -----------------------------------------------------------------------------

echo ""
echo "=============================================="
echo "[8/9] Creating systemd services..."
echo "=============================================="

# Mainnet service
if [ "$INSTALL_MAINNET" = true ]; then
    cat > /etc/systemd/system/intcoind-mainnet.service << EOF
[Unit]
Description=INTcoin Mainnet Daemon
Documentation=https://github.com/INT-devs/intcoin
After=network-online.target
Wants=network-online.target

[Service]
Type=simple
User=$INTCOIN_USER
Group=$INTCOIN_USER
Environment="HOME=$HOME_DIR"

ExecStart=/usr/local/bin/intcoind \\
    -datadir=$HOME_DIR/.intcoin-mainnet \\
    -rpcuser=intcoinrpc \\
    -rpcpassword=${MAINNET_RPC_PASSWORD} \\
    -rpcport=${MAINNET_RPC_PORT} \\
    -port=${MAINNET_P2P_PORT}

ExecStop=/usr/local/bin/intcoin-cli \\
    -rpcuser=intcoinrpc \\
    -rpcpassword=${MAINNET_RPC_PASSWORD} \\
    -rpcport=${MAINNET_RPC_PORT} \\
    stop

Restart=on-failure
RestartSec=30
TimeoutStartSec=infinity
TimeoutStopSec=600

# Resource limits
LimitNOFILE=65536
LimitNPROC=4096

# Security hardening
PrivateTmp=true
ProtectSystem=full
NoNewPrivileges=true
ProtectHome=false
ReadWritePaths=$HOME_DIR/.intcoin-mainnet

[Install]
WantedBy=multi-user.target
EOF

    log_success "Mainnet systemd service created"
fi

# Testnet service
if [ "$INSTALL_TESTNET" = true ]; then
    cat > /etc/systemd/system/intcoind-testnet.service << EOF
[Unit]
Description=INTcoin Testnet Daemon
Documentation=https://github.com/INT-devs/intcoin
After=network-online.target
Wants=network-online.target

[Service]
Type=simple
User=$INTCOIN_USER
Group=$INTCOIN_USER
Environment="HOME=$HOME_DIR"

ExecStart=/usr/local/bin/intcoind \\
    -testnet \\
    -datadir=$HOME_DIR/.intcoin-testnet \\
    -rpcuser=intcoinrpc \\
    -rpcpassword=${TESTNET_RPC_PASSWORD} \\
    -rpcport=${TESTNET_RPC_PORT} \\
    -port=${TESTNET_P2P_PORT}

ExecStop=/usr/local/bin/intcoin-cli \\
    -testnet \\
    -rpcuser=intcoinrpc \\
    -rpcpassword=${TESTNET_RPC_PASSWORD} \\
    -rpcport=${TESTNET_RPC_PORT} \\
    stop

Restart=on-failure
RestartSec=30
TimeoutStartSec=infinity
TimeoutStopSec=600

# Resource limits
LimitNOFILE=32768
LimitNPROC=2048

# Security hardening
PrivateTmp=true
ProtectSystem=full
NoNewPrivileges=true
ProtectHome=false
ReadWritePaths=$HOME_DIR/.intcoin-testnet

[Install]
WantedBy=multi-user.target
EOF

    log_success "Testnet systemd service created"

    # Faucet service
    cat > /etc/systemd/system/intcoin-faucet.service << EOF
[Unit]
Description=INTcoin Testnet Faucet
Documentation=https://github.com/INT-devs/intcoin
After=intcoind-testnet.service
Requires=intcoind-testnet.service

[Service]
Type=simple
User=$INTCOIN_USER
Group=$INTCOIN_USER
Environment="HOME=$HOME_DIR"

ExecStart=/usr/local/bin/intcoin-faucet \\
    --rpcconnect=127.0.0.1 \\
    --rpcport=${TESTNET_RPC_PORT} \\
    --rpcuser=intcoinrpc \\
    --rpcpassword=${TESTNET_RPC_PASSWORD} \\
    --httpport=${FAUCET_HTTP_PORT} \\
    --amount=10 \\
    --cooldown=3600

Restart=on-failure
RestartSec=30

# Resource limits
LimitNOFILE=4096

# Security hardening
PrivateTmp=true
ProtectSystem=full
NoNewPrivileges=true

[Install]
WantedBy=multi-user.target
EOF

    log_success "Faucet systemd service created"
fi

# Reload systemd
systemctl daemon-reload

# -----------------------------------------------------------------------------
# Step 9: Configure Firewall and Log Rotation
# -----------------------------------------------------------------------------

echo ""
echo "=============================================="
echo "[9/9] Configuring firewall and log rotation..."
echo "=============================================="

# UFW Firewall
if command -v ufw &> /dev/null; then
    log_info "Configuring UFW firewall rules..."

    if [ "$INSTALL_MAINNET" = true ]; then
        ufw allow ${MAINNET_P2P_PORT}/tcp comment 'INTcoin Mainnet P2P'
    fi

    if [ "$INSTALL_TESTNET" = true ]; then
        ufw allow ${TESTNET_P2P_PORT}/tcp comment 'INTcoin Testnet P2P'
        ufw allow ${FAUCET_HTTP_PORT}/tcp comment 'INTcoin Testnet Faucet'
    fi

    log_success "UFW rules configured"
else
    log_warn "UFW not found. Please configure firewall manually."
    echo "  Required ports:"
    [ "$INSTALL_MAINNET" = true ] && echo "    - ${MAINNET_P2P_PORT}/tcp (Mainnet P2P)"
    [ "$INSTALL_TESTNET" = true ] && echo "    - ${TESTNET_P2P_PORT}/tcp (Testnet P2P)"
    [ "$INSTALL_TESTNET" = true ] && echo "    - ${FAUCET_HTTP_PORT}/tcp (Faucet HTTP)"
fi

# Log rotation
cat > /etc/logrotate.d/intcoin << EOF
$HOME_DIR/.intcoin-mainnet/*.log
$HOME_DIR/.intcoin-testnet/*.log {
    daily
    rotate 14
    compress
    delaycompress
    missingok
    notifempty
    create 644 $INTCOIN_USER $INTCOIN_USER
    sharedscripts
    postrotate
        systemctl reload intcoind-mainnet 2>/dev/null || true
        systemctl reload intcoind-testnet 2>/dev/null || true
    endscript
}
EOF

log_success "Log rotation configured"

# =============================================================================
# Save Credentials
# =============================================================================

CREDS_FILE="$HOME_DIR/.intcoin-credentials"
cat > $CREDS_FILE << EOF
# INTcoin RPC Credentials
# Generated by setup-node.sh v${VERSION} on $(date)
# KEEP THIS FILE SECURE!

MAINNET_RPC_USER=intcoinrpc
MAINNET_RPC_PASSWORD=${MAINNET_RPC_PASSWORD}
MAINNET_RPC_PORT=${MAINNET_RPC_PORT}

TESTNET_RPC_USER=intcoinrpc
TESTNET_RPC_PASSWORD=${TESTNET_RPC_PASSWORD}
TESTNET_RPC_PORT=${TESTNET_RPC_PORT}
EOF

chown $INTCOIN_USER:$INTCOIN_USER $CREDS_FILE
chmod 600 $CREDS_FILE

# =============================================================================
# Create Helper Scripts
# =============================================================================

# Mainnet CLI helper
if [ "$INSTALL_MAINNET" = true ]; then
    cat > /usr/local/bin/intcoin-mainnet << EOF
#!/bin/bash
# INTcoin Mainnet CLI wrapper
exec intcoin-cli -rpcuser=intcoinrpc -rpcpassword=${MAINNET_RPC_PASSWORD} -rpcport=${MAINNET_RPC_PORT} "\$@"
EOF
    chmod 755 /usr/local/bin/intcoin-mainnet
fi

# Testnet CLI helper
if [ "$INSTALL_TESTNET" = true ]; then
    cat > /usr/local/bin/intcoin-testnet << EOF
#!/bin/bash
# INTcoin Testnet CLI wrapper
exec intcoin-cli -testnet -rpcuser=intcoinrpc -rpcpassword=${TESTNET_RPC_PASSWORD} -rpcport=${TESTNET_RPC_PORT} "\$@"
EOF
    chmod 755 /usr/local/bin/intcoin-testnet
fi

# =============================================================================
# Final Output
# =============================================================================

echo ""
echo "=============================================="
echo "  Installation Complete!"
echo "=============================================="
echo ""

log_success "INTcoin node setup completed successfully!"
echo ""

echo "CREDENTIALS SAVED TO: $CREDS_FILE"
echo "(Keep this file secure - it contains RPC passwords)"
echo ""

if [ "$INSTALL_MAINNET" = true ]; then
    echo "MAINNET:"
    echo "  Data directory: $HOME_DIR/.intcoin-mainnet"
    echo "  P2P port: ${MAINNET_P2P_PORT}"
    echo "  RPC port: ${MAINNET_RPC_PORT}"
    echo "  RPC password: ${MAINNET_RPC_PASSWORD}"
    echo ""
    echo "  Start:   sudo systemctl start intcoind-mainnet"
    echo "  Enable:  sudo systemctl enable intcoind-mainnet"
    echo "  Status:  sudo systemctl status intcoind-mainnet"
    echo "  CLI:     intcoin-mainnet getblockcount"
    echo ""
fi

if [ "$INSTALL_TESTNET" = true ]; then
    echo "TESTNET:"
    echo "  Data directory: $HOME_DIR/.intcoin-testnet"
    echo "  P2P port: ${TESTNET_P2P_PORT}"
    echo "  RPC port: ${TESTNET_RPC_PORT}"
    echo "  RPC password: ${TESTNET_RPC_PASSWORD}"
    if [ "$ENABLE_MINING" = true ]; then
        echo "  Mining: ENABLED (1 CPU thread)"
    fi
    echo ""
    echo "  Start:   sudo systemctl start intcoind-testnet"
    echo "  Enable:  sudo systemctl enable intcoind-testnet"
    echo "  Status:  sudo systemctl status intcoind-testnet"
    echo "  CLI:     intcoin-testnet getblockcount"
    echo ""

    echo "TESTNET FAUCET:"
    echo "  HTTP port: ${FAUCET_HTTP_PORT}"
    echo "  URL: http://$(hostname -I | awk '{print $1}'):${FAUCET_HTTP_PORT}"
    echo "  Dispenses: 10 INT per request (1-hour cooldown)"
    echo ""
    echo "  Start:   sudo systemctl start intcoin-faucet"
    echo "  Enable:  sudo systemctl enable intcoin-faucet"
    echo "  Status:  sudo systemctl status intcoin-faucet"
    echo "  Logs:    journalctl -u intcoin-faucet -f"
    echo ""
    echo "  NOTE: The faucet generates its own wallet on startup."
    echo "        Check faucet logs for the wallet address to fund."
    echo ""
fi

echo "QUICK START:"
if [ "$INSTALL_MAINNET" = true ] && [ "$INSTALL_TESTNET" = true ]; then
    echo "  # Start both nodes and faucet"
    echo "  sudo systemctl start intcoind-mainnet intcoind-testnet"
    echo "  sudo systemctl start intcoin-faucet"
    echo ""
    echo "  # Enable on boot"
    echo "  sudo systemctl enable intcoind-mainnet intcoind-testnet intcoin-faucet"
elif [ "$INSTALL_MAINNET" = true ]; then
    echo "  sudo systemctl start intcoind-mainnet"
    echo "  sudo systemctl enable intcoind-mainnet"
elif [ "$INSTALL_TESTNET" = true ]; then
    echo "  sudo systemctl start intcoind-testnet intcoin-faucet"
    echo "  sudo systemctl enable intcoind-testnet intcoin-faucet"
fi
echo ""

echo "LOGS:"
[ "$INSTALL_MAINNET" = true ] && echo "  Mainnet: tail -f $HOME_DIR/.intcoin-mainnet/debug.log"
[ "$INSTALL_TESTNET" = true ] && echo "  Testnet: tail -f $HOME_DIR/.intcoin-testnet/debug.log"
[ "$INSTALL_TESTNET" = true ] && echo "  Faucet:  journalctl -u intcoin-faucet -f"
echo ""

echo "DOCUMENTATION:"
echo "  https://github.com/INT-devs/intcoin"
echo "  https://international-coin.org"
echo ""

# Cleanup
log_info "Cleaning up build files..."
rm -rf /tmp/liboqs /tmp/RandomX

log_success "Setup complete!"
