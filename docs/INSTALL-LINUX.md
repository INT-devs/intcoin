# Installing INTcoin on Linux

**Copyright (c) 2025 INTcoin Core (Maddison Lane)**

This guide covers building and installing INTcoin on various Linux distributions.

## Quick Start (Ubuntu/Debian)

```bash
# Install dependencies
sudo apt update
sudo apt install -y build-essential cmake libboost-all-dev libssl-dev \
    qtbase5-dev qttools5-dev libqt5svg5-dev python3 python3-pip git

# Clone repository
git clone https://gitlab.com/intcoin/crypto.git
cd crypto

# Build
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)

# Install (optional)
sudo make install

# Run
./intcoin-qt  # GUI wallet
# or
./intcoind    # Daemon
```

---

## Detailed Installation by Distribution

### Ubuntu / Debian / Linux Mint

#### Ubuntu 22.04 LTS / Debian 12 (Bookworm)

```bash
# Update package list
sudo apt update && sudo apt upgrade -y

# Install build tools
sudo apt install -y \
    build-essential \
    cmake \
    git \
    pkg-config \
    autoconf \
    automake \
    libtool

# Install dependencies
sudo apt install -y \
    libboost-system-dev \
    libboost-filesystem-dev \
    libboost-thread-dev \
    libboost-program-options-dev \
    libboost-chrono-dev \
    libssl-dev \
    libevent-dev \
    libdb++-dev \
    libminiupnpc-dev \
    libzmq3-dev

# Install Qt5 (for GUI wallet)
sudo apt install -y \
    qtbase5-dev \
    qttools5-dev \
    qttools5-dev-tools \
    libqt5svg5-dev

# Install Python (for tests)
sudo apt install -y \
    python3 \
    python3-pip \
    python3-dev

# Install Python test dependencies
pip3 install pytest pytest-asyncio
```

### Fedora / RHEL / CentOS

#### Fedora 38+

```bash
# Install build tools
sudo dnf groupinstall -y "Development Tools"
sudo dnf install -y cmake git

# Install dependencies
sudo dnf install -y \
    boost-devel \
    openssl-devel \
    libevent-devel \
    libdb-cxx-devel \
    miniupnpc-devel \
    zeromq-devel

# Install Qt5
sudo dnf install -y \
    qt5-qtbase-devel \
    qt5-qttools-devel \
    qt5-qtsvg-devel

# Install Python
sudo dnf install -y \
    python3 \
    python3-pip \
    python3-devel
```

#### RHEL 9 / Rocky Linux 9 / AlmaLinux 9

```bash
# Enable EPEL repository
sudo dnf install -y epel-release

# Install build tools
sudo dnf groupinstall -y "Development Tools"
sudo dnf install -y cmake git

# Install dependencies
sudo dnf install -y \
    boost-devel \
    openssl-devel \
    libevent-devel \
    zeromq-devel \
    qt5-qtbase-devel \
    qt5-qttools-devel \
    python3 \
    python3-pip
```

### Arch Linux / Manjaro

```bash
# Update system
sudo pacman -Syu

# Install dependencies
sudo pacman -S --needed \
    base-devel \
    cmake \
    git \
    boost \
    openssl \
    libevent \
    db \
    miniupnpc \
    zeromq \
    qt5-base \
    qt5-tools \
    qt5-svg \
    python \
    python-pip

# Install Python test dependencies
pip install pytest pytest-asyncio
```

### openSUSE

```bash
# Install build tools
sudo zypper install -y -t pattern devel_basis

# Install dependencies
sudo zypper install -y \
    cmake \
    git \
    boost-devel \
    libopenssl-devel \
    libevent-devel \
    libdb-4_8-devel \
    miniupnpc-devel \
    zeromq-devel \
    libQt5Core-devel \
    libQt5Gui-devel \
    libQt5Widgets-devel \
    python3 \
    python3-pip
```

### Gentoo

```bash
# Install dependencies
sudo emerge --ask \
    dev-util/cmake \
    dev-vcs/git \
    dev-libs/boost \
    dev-libs/openssl \
    dev-libs/libevent \
    sys-libs/db:5.3 \
    net-libs/miniupnpc \
    net-libs/zeromq \
    dev-qt/qtcore \
    dev-qt/qtgui \
    dev-qt/qtwidgets \
    dev-qt/qtsvg \
    dev-lang/python
```

---

## Building INTcoin

### Standard Build

```bash
# Clone repository
git clone https://gitlab.com/intcoin/crypto.git
cd crypto

# Initialize submodules (for external dependencies)
git submodule update --init --recursive

# Create build directory
mkdir build && cd build

# Configure
cmake .. -DCMAKE_BUILD_TYPE=Release

# Build (using all CPU cores)
make -j$(nproc)

# Optional: Run tests
make test

# Optional: Install system-wide
sudo make install
```

### Custom Install Location

```bash
# Install to /opt/intcoin instead of /usr/local
cmake .. -DCMAKE_INSTALL_PREFIX=/opt/intcoin
make -j$(nproc)
sudo make install

# Add to PATH
echo 'export PATH=/opt/intcoin/bin:$PATH' >> ~/.bashrc
source ~/.bashrc
```

### Build Options

```bash
# Minimal build (daemon only, no GUI)
cmake .. -DBUILD_QT_WALLET=OFF \
         -DBUILD_MINER=OFF \
         -DBUILD_EXPLORER=OFF

# Full build (all components)
cmake .. -DBUILD_QT_WALLET=ON \
         -DBUILD_DAEMON=ON \
         -DBUILD_CLI=ON \
         -DBUILD_MINER=ON \
         -DBUILD_EXPLORER=ON \
         -DBUILD_TESTS=ON

# Debug build
cmake .. -DCMAKE_BUILD_TYPE=Debug

# With specific compiler
cmake .. -DCMAKE_C_COMPILER=gcc-13 \
         -DCMAKE_CXX_COMPILER=g++-13
```

---

## Installation

### System-wide Installation

```bash
cd build
sudo make install

# Verify installation
which intcoind
intcoind --version
```

Default install locations:
- Binaries: `/usr/local/bin/`
- Headers: `/usr/local/include/intcoin/`
- Config: `/usr/local/share/intcoin/`
- Docs: `/usr/local/share/doc/intcoin/`

### User Installation

```bash
# Install to home directory
cmake .. -DCMAKE_INSTALL_PREFIX=$HOME/.local
make install

# Add to PATH if needed
echo 'export PATH=$HOME/.local/bin:$PATH' >> ~/.bashrc
```

### Create .deb Package (Debian/Ubuntu)

```bash
cd build
cpack -G DEB

# Install the package
sudo dpkg -i INTcoin-*.deb

# Uninstall
sudo apt remove intcoin
```

### Create .rpm Package (Fedora/RHEL)

```bash
cd build
cpack -G RPM

# Install the package
sudo rpm -i INTcoin-*.rpm

# Uninstall
sudo rpm -e intcoin
```

---

## Running INTcoin

### First Run

```bash
# Create data directory
mkdir -p ~/.intcoin

# Copy config file
cp /usr/local/share/intcoin/config/mainnet/intcoin.conf ~/.intcoin/

# Edit configuration (optional)
nano ~/.intcoin/intcoin.conf
```

### Start Daemon

```bash
# Start daemon
intcoind -daemon

# Check status
intcoin-cli getblockchaininfo

# Stop daemon
intcoin-cli stop
```

### Start GUI Wallet

```bash
intcoin-qt
```

### Testnet

```bash
# Start testnet daemon
intcoind -testnet -daemon

# Or copy testnet config
cp /usr/local/share/intcoin/config/testnet/intcoin.conf ~/.intcoin/intcoin-testnet.conf
intcoind -testnet -daemon -conf=$HOME/.intcoin/intcoin-testnet.conf
```

---

## Systemd Service

Create a systemd service for automatic startup:

```bash
# Create service file
sudo nano /etc/systemd/system/intcoind.service
```

Add this content:

```ini
[Unit]
Description=INTcoin Daemon
After=network.target

[Service]
Type=forking
User=intcoin
Group=intcoin
ExecStart=/usr/local/bin/intcoind -daemon -conf=/etc/intcoin/intcoin.conf -datadir=/var/lib/intcoin
ExecStop=/usr/local/bin/intcoin-cli stop
Restart=on-failure
RestartSec=10

# Hardening
PrivateTmp=true
ProtectSystem=full
NoNewPrivileges=true
PrivateDevices=true

[Install]
WantedBy=multi-user.target
```

Enable and start:

```bash
# Create user
sudo useradd -r -m -d /var/lib/intcoin -s /bin/bash intcoin

# Create config directory
sudo mkdir -p /etc/intcoin
sudo cp config/mainnet/intcoin.conf /etc/intcoin/
sudo chown -R intcoin:intcoin /etc/intcoin /var/lib/intcoin

# Enable service
sudo systemctl daemon-reload
sudo systemctl enable intcoind
sudo systemctl start intcoind

# Check status
sudo systemctl status intcoind

# View logs
sudo journalctl -u intcoind -f
```

---

## CPU Mining

### From Wallet GUI

1. Open INTcoin wallet (`intcoin-qt`)
2. Go to Settings → Options → Mining
3. Check "Enable CPU mining"
4. Set number of threads (recommended: CPU cores - 1)
5. Click "Start Mining"

### Standalone Miner

```bash
# Mine with 4 threads
intcoin-miner -t 4

# Mine with all threads except 2
intcoin-miner -t $(expr $(nproc) - 2)

# Mine to specific address
intcoin-miner -t 4 -a INT1YourAddressHere
```

---

## Performance Tuning

### Compiler Optimizations

```bash
# Build with native CPU optimizations
cmake .. -DCMAKE_BUILD_TYPE=Release \
         -DCMAKE_CXX_FLAGS="-O3 -march=native -mtune=native"

# Link-time optimization
cmake .. -DCMAKE_BUILD_TYPE=Release \
         -DCMAKE_INTERPROCEDURAL_OPTIMIZATION=ON
```

### Huge Pages (for mining)

```bash
# Enable huge pages
echo 128 | sudo tee /sys/kernel/mm/hugepages/hugepages-2048kB/nr_hugepages

# Make permanent
echo "vm.nr_hugepages=128" | sudo tee -a /etc/sysctl.conf
sudo sysctl -p
```

---

## Troubleshooting

### Boost Version Too Old

```bash
# Download and build Boost manually
wget https://boostorg.jfrog.io/artifactory/main/release/1.82.0/source/boost_1_82_0.tar.gz
tar xzf boost_1_82_0.tar.gz
cd boost_1_82_0
./bootstrap.sh --prefix=/usr/local
sudo ./b2 install -j$(nproc)
```

### Qt Not Found

```bash
# Set Qt path manually
cmake .. -DCMAKE_PREFIX_PATH=/usr/lib/x86_64-linux-gnu/cmake/Qt5

# Or disable Qt wallet
cmake .. -DBUILD_QT_WALLET=OFF
```

### OpenSSL 3.0 Compatibility

```bash
# If using OpenSSL 3.0+, you may need legacy provider
export OPENSSL_CONF=/path/to/openssl_legacy.cnf
```

### Low Disk Space

```bash
# Enable pruning (keeps only recent blocks)
echo "prune=2000" >> ~/.intcoin/intcoin.conf
```

---

## Uninstallation

### Manual Removal

```bash
# Remove binaries
sudo rm /usr/local/bin/intcoin*

# Remove headers
sudo rm -rf /usr/local/include/intcoin

# Remove shared files
sudo rm -rf /usr/local/share/intcoin

# Remove user data (WARNING: includes wallet!)
rm -rf ~/.intcoin
```

### Package Removal

```bash
# Debian/Ubuntu
sudo apt remove intcoin

# Fedora/RHEL
sudo dnf remove intcoin

# Arch
sudo pacman -R intcoin
```

---

## Security Recommendations

### Firewall Configuration

```bash
# Allow P2P port (mainnet)
sudo ufw allow 8333/tcp

# Allow RPC port (only from localhost)
sudo ufw allow from 127.0.0.1 to any port 9332
```

### Wallet Security

```bash
# Encrypt wallet
intcoin-cli encryptwallet "your-secure-passphrase"

# Backup wallet
intcoin-cli backupwallet ~/intcoin-backup.dat

# Set proper permissions
chmod 600 ~/.intcoin/wallet.dat
```

---

## Additional Resources

- Build documentation: https://gitlab.com/intcoin/crypto/-/wikis/Build
- Forum: https://international-coin.org/forum
- Support: team@international-coin.org

---

**Last Updated**: January 2025
