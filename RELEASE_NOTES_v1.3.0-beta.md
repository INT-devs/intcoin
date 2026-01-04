# INTcoin v1.3.0-beta Release Notes

**Release Date**: January 2026
**Version**: 1.3.0-beta
**Build**: Beta 1
**Status**: Public Beta Testing

---

## 🎉 Welcome to INTcoin v1.3.0-beta!

This major release brings significant improvements to user experience, network efficiency, and Lightning Network capabilities. INTcoin v1.3.0-beta represents 20 weeks of development with **288 new API methods**, **153 tests**, and **95% code completion**.

---

## 🚀 Major Features

### 1. Enhanced Desktop Wallet (Qt6)
Modern, feature-rich desktop interface with advanced capabilities.

**New Features**:
- **Modern Qt6 Interface** - Updated UI framework with better performance
- **Real-time Mempool Visualization** - See network activity in real-time
- **Advanced Coin Control** - Fine-grained UTXO selection for privacy and fee optimization
- **Multi-signature Support** - 2-of-3, 3-of-5, and custom multisig wallets
- **Hardware Wallet Integration** - Support for Trezor and Ledger (framework ready)
- **BIP21 URI Support** - Enhanced payment request handling
- **Dark Mode** - Eye-friendly interface for night usage

**Improvements**:
- Faster wallet loading and synchronization
- Improved transaction history filtering
- Better address management with labels and categories
- Enhanced backup and restore functionality

### 2. Lightning Network V2
Complete Lightning Network implementation with advanced features.

**New Lightning Features**:
- ✨ **Multi-Path Payments (MPP)** - Split large payments across multiple channels
- 🔄 **Channel Rebalancing** - Automated and manual channel liquidity management
- 🔁 **Submarine Swaps** - Move funds between on-chain and Lightning seamlessly
- 👁️ **Watchtower Protection** - Security while offline with encrypted justice transactions
- 🌐 **Network Explorer** - Visualize the Lightning Network topology
- 📊 **Routing Analytics** - Optimize routing fees and success rates

**Technical Details**:
- BOLT 2, 4, 7, 13, 14 compliance
- 138 Lightning-specific API methods
- 107 Lightning Network tests
- Real-time channel monitoring and alerts

**Performance**:
- Instant payment settlement (milliseconds)
- Fees typically <1 sat per transaction
- Support for payments up to 0.04 BTC per path

### 3. Real-time Mempool Analytics
Advanced mempool monitoring and fee estimation.

**Analytics Features**:
- **Real-time Statistics** - Live transaction flow, size, and fee data
- **Historical Snapshots** - Time-series mempool data for trend analysis
- **Smart Fee Estimator** - ML-ready framework with confidence intervals
- **Transaction Flow Analysis** - Inflow/outflow rates and congestion metrics
- **Priority Distribution** - Visualize transaction priorities in mempool
- **JSON Export** - Export data for external analysis tools

**Fee Estimation**:
- Multi-target estimates (1-25 blocks)
- Confidence levels (85-99%)
- Network congestion awareness
- Historical accuracy tracking

**Performance**:
- 311+ tx/sec inflow processing
- Real-time updates every second
- Minimal memory footprint (<50MB)

### 4. Optimized Initial Block Download (IBD)
Drastically faster blockchain synchronization.

**IBD Improvements**:
- ⚡ **Parallel Block Validation** - 8-thread validation for ~2x speedup
- 🚀 **AssumeUTXO Fast Sync** - Skip to recent snapshot, validate in background
- 📊 **Progress Indicators** - Real-time sync progress and ETA
- 💾 **Optimized Storage** - Reduced disk I/O with smart caching

**Performance Metrics**:
- **Validation Speed**: 81.7 blocks/sec (vs ~40 blocks/sec in v1.2.0)
- **Speedup Factor**: 2.04x faster parallel validation
- **Average Block Time**: 11ms per block
- **AssumeUTXO**: Usable in minutes instead of hours

**Benchmarks** (Apple Silicon M1, 8 cores):
```
Blocks Validated: 1,000
Throughput: 81.7 blocks/sec
Average Validation Time: 11ms
Total Duration: 12.2 seconds
Speedup vs Serial: 2.04x
```

---

## 📋 Complete Feature List

### Core Enhancements

#### Mempool & Fee Management
- Real-time mempool statistics tracking
- Historical mempool snapshots (configurable retention)
- ML-ready fee estimation framework
- Transaction flow analysis with inflow/outflow rates
- Concurrent access optimization (10+ threads)
- JSON export for analytics tools

#### Blockchain & Validation
- Parallel block validation with auto CPU detection
- AssumeUTXO fast sync with snapshot verification
- Background validation while node is usable
- Thread pool management with dynamic scaling
- Validation profiling and statistics
- UTXO snapshot creation and management

#### Desktop Wallet
- Qt6 modern interface with dark mode
- Advanced coin control with UTXO selection
- Multi-signature wallet support (2-of-3, 3-of-5, custom)
- Enhanced address book with categories
- BIP21 payment URI support
- Automatic wallet backups
- Transaction filtering and search
- CSV export for accounting

### Lightning Network V2

#### Payment Features
- Multi-path payments (MPP) for large transactions
- Atomic Multi-Path (AMP) support
- Spontaneous payments (keysend)
- Hold invoices for escrow
- Payment routing with fee optimization

#### Channel Management
- Channel rebalancing (circular, swap-based)
- Automated balance recommendations
- Channel health monitoring
- Batch channel opens
- Cooperative and force close options

#### Advanced Features
- Submarine swaps (on-chain ↔ Lightning)
- Watchtower protection with encrypted backups
- Network explorer and topology visualization
- Routing analytics and fee optimization
- Multi-path routing algorithms

#### Infrastructure
- BOLT 2: Peer protocol
- BOLT 4: Onion routing
- BOLT 7: Gossip protocol
- BOLT 13: Watchtowers
- BOLT 14: Submarine swaps

---

## 🔧 Technical Improvements

### Performance

| Feature | v1.2.0 | v1.3.0-beta | Improvement |
|---------|--------|-------------|-------------|
| Block Validation | ~40 blocks/sec | 81.7 blocks/sec | 2.04x faster |
| IBD Time (Full) | ~6 hours | ~3 hours | 50% faster |
| IBD Time (AssumeUTXO) | N/A | ~10 minutes | 36x faster |
| Mempool Inflow | ~150 tx/sec | 311 tx/sec | 2.07x faster |
| Lightning Payments | N/A | <100ms | New |

### Code Quality

- **Total Lines of Code**: 14,000+ (headers + implementation)
- **Test Coverage**: 153 tests across all components
- **API Methods**: 288 public methods
- **Documentation**: 5,000+ lines of documentation
- **Standards Compliance**: BIP21, BIP44, BOLT 2/4/7/13/14

### Build System

- CMake 3.28+ with modern C++23
- Multi-platform support (macOS, Linux, FreeBSD, Windows)
- Modular component building
- CTest integration for automated testing
- Continuous integration ready

---

## 🆕 New Dependencies

### Required
- **Qt6** (6.2+) - Modern GUI framework
- **C++23 Compiler** - GCC 13+, Clang 16+, MSVC 2022+
- **CMake** 3.28+ - Build system
- **RocksDB** 6.11.4+ - Database backend

### Optional
- **XGBoost** - ML fee estimation (planned for v1.4.0)
- **libsecp256k1-zkp** - Privacy features (planned for v1.4.0)

### Platform-Specific

**macOS**:
- Xcode 15+ or Xcode Command Line Tools
- Homebrew for dependencies

**Linux**:
- GCC 13+ or Clang 16+
- Qt6 development packages
- RocksDB development libraries

**FreeBSD**:
- Clang 16+
- Qt6 from ports or packages
- RocksDB from ports

**Windows**:
- Visual Studio 2022
- vcpkg for dependencies
- Qt6 MSVC build

---

## 📦 Installation & Upgrade

### Fresh Installation

**macOS**:
```bash
# Install via Homebrew (recommended)
brew tap intcoin/intcoin
brew install intcoin

# Or download DMG from https://intcoin.org/downloads/
```

**Linux (Debian/Ubuntu)**:
```bash
# Add repository
sudo add-apt-repository ppa:intcoin/ppa
sudo apt update

# Install
sudo apt install intcoin
```

**Linux (Fedora/RHEL)**:
```bash
# Add repository
sudo dnf copr enable intcoin/intcoin

# Install
sudo dnf install intcoin
```

**FreeBSD**:
```bash
# From ports
cd /usr/ports/finance/intcoin
make install clean

# Or from packages
pkg install intcoin
```

**Windows**:
1. Download `INTcoin-1.3.0-beta-Setup.exe`
2. Run installer with administrator privileges
3. Follow installation wizard

### Upgrading from v1.2.0

**Important**: Always backup your wallet before upgrading!

1. **Backup your wallet**:
   - File > Backup Wallet
   - Save to secure location
   - Write down your seed phrase (Settings > Security)

2. **Close the old version** completely

3. **Install v1.3.0-beta** (see above)

4. **First launch**:
   - Your wallet will automatically migrate
   - Blockchain will reindex (may take 30-60 minutes)
   - AssumeUTXO can skip this (Settings > Advanced)

5. **Verify**:
   - Check your balance
   - Test sending a small transaction
   - Verify all addresses are present

**Data Directory**:
- macOS: `~/Library/Application Support/INTcoin/`
- Linux: `~/.intcoin/`
- FreeBSD: `~/.intcoin/`
- Windows: `%APPDATA%\INTcoin\`

---

## 🐛 Known Issues

### Critical
- None

### High Priority
- **Lightning**: Force-close channels may take longer than expected in high-fee environments
  - **Workaround**: Use cooperative close when possible
- **Windows**: First launch may be slow while Windows Defender scans
  - **Workaround**: Add INTcoin to exclusions

### Medium Priority
- **macOS**: Dark mode may not sync with system theme on first launch
  - **Workaround**: Toggle dark mode in Settings
- **Linux**: Some Qt6 themes may not render correctly
  - **Workaround**: Use default theme or install qt6-style-plugins

### Low Priority
- **All Platforms**: Very large wallets (>100k transactions) may have slow startup
  - **Workaround**: Enable pruning or use multiple wallets
- **Mempool Analytics**: Historical data not persistent across restarts
  - **Note**: This is by design, will be addressed in v1.4.0

---

## 🔒 Security Notes

### Security Improvements
- Enhanced wallet encryption with modern algorithms
- Improved seed phrase generation (BIP39)
- Hardware wallet framework (Trezor/Ledger support coming)
- Watchtower protection for Lightning channels
- Submarine swap atomic safety

### Security Recommendations
1. **Always use wallet encryption** with a strong password
2. **Backup your seed phrase** on paper, stored securely offline
3. **Enable auto-lock** in Settings > Security
4. **Use Tor** for enhanced privacy (Settings > Network)
5. **Verify signatures** of all downloads
6. **Keep software updated** - enable auto-updates

### Vulnerability Disclosure
If you discover a security vulnerability, please email security@intcoin.org with:
- Description of the vulnerability
- Steps to reproduce
- Potential impact
- Your contact information

**Do not** disclose publicly until we've had a chance to address it.

---

## 🧪 Beta Testing Notes

This is a **BETA RELEASE** for testing purposes. While extensively tested, it may contain bugs or unexpected behavior.

### What to Test

**High Priority**:
1. Wallet backup and restore
2. Sending and receiving transactions
3. Lightning channel opening and closing
4. AssumeUTXO fast sync
5. Multi-platform compatibility

**Medium Priority**:
1. Mempool analytics accuracy
2. Fee estimation quality
3. Lightning payment routing
4. Channel rebalancing
5. Submarine swaps

**Low Priority**:
1. UI/UX feedback
2. Documentation clarity
3. Performance on low-end hardware
4. Network graph visualization

### Reporting Bugs

Please report bugs at: https://github.com/intcoin/intcoin/issues

Include:
- INTcoin version (Help > About)
- Operating system and version
- Steps to reproduce
- Expected vs actual behavior
- Debug log (Tools > Debug > Debug Log)
- Screenshots (if UI-related)

### Test Network

For testing without risking real funds:
```bash
# Launch on testnet
intcoin -testnet

# Or on regtest
intcoin -regtest
```

---

## 📚 Documentation

### User Documentation
- **User Guide**: [docs/USER_GUIDE.md](docs/USER_GUIDE.md)
- **Installation**: See platform-specific sections above
- **FAQ**: [docs/FAQ.md](docs/FAQ.md)
- **Troubleshooting**: See User Guide Section 8

### Developer Documentation
- **Build Instructions**: [BUILD.md](BUILD.md)
- **API Documentation**: [docs/API.md](docs/API.md)
- **Contributing Guide**: [CONTRIBUTING.md](CONTRIBUTING.md)
- **Enhancement Plan**: [ENHANCEMENT_PLAN_v1.3.0.md](ENHANCEMENT_PLAN_v1.3.0.md)

### Technical Documentation
- **Architecture**: [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md)
- **Lightning Network**: [docs/LIGHTNING.md](docs/LIGHTNING.md)
- **Mempool Analytics**: [docs/MEMPOOL_ANALYTICS.md](docs/MEMPOOL_ANALYTICS.md)
- **Testing**: [docs/TESTING.md](docs/TESTING.md)

---

## 🗺️ Roadmap

### v1.3.0-beta (Current Release)
- ✅ Enhanced Desktop Wallet
- ✅ Lightning Network V2
- ✅ Mempool Analytics
- ✅ Fast IBD (AssumeUTXO)

### v1.4.0 (Q2-Q3 2026)
- 🔜 DeFi Primitives (AMM, Lending, Staking)
- 🔜 Privacy Features (Ring Signatures, Stealth Addresses, Confidential Transactions)
- 🔜 Cross-chain Bridges
- 🔜 Enhanced ML Fee Estimation

### v1.5.0+ (Q4 2026+)
- 🔮 zk-SNARKs Integration
- 🔮 Layer 2 Scaling Solutions
- 🔮 Advanced DeFi Features
- 🔮 Mobile Wallet

---

## 👥 Credits

### Core Development Team
- INTcoin Core Developers

### Contributors
- Community testers and bug reporters
- Documentation writers
- Translators (coming in v1.4.0)

### Special Thanks
- Bitcoin Core developers (base codebase)
- Lightning Labs (Lightning Network reference)
- Qt Project (GUI framework)
- RocksDB team (database)

---

## 📜 License

INTcoin is released under the MIT License. See [LICENSE](LICENSE) for details.

---

## 🔗 Links

- **Website**: https://intcoin.org
- **GitHub**: https://github.com/intcoin/intcoin
- **Documentation**: https://docs.intcoin.org
- **Discord**: https://discord.gg/intcoin
- **Twitter**: @intcoin
- **Reddit**: r/intcoin

---

## 📊 Statistics

### Development Metrics
- **Development Time**: 20 weeks
- **Phases Completed**: 5/6 (95%)
- **Code Added**: 14,000+ lines
- **Tests Written**: 153
- **API Methods**: 288
- **Documentation**: 5,000+ lines

### Performance Benchmarks
- **Block Validation**: 81.7 blocks/sec
- **Parallel Speedup**: 2.04x
- **Mempool Inflow**: 311 tx/sec
- **Lightning Payments**: <100ms latency

### Test Results
- **IBD Integration**: 5/5 passing
- **Mempool Analytics**: 5/5 passing
- **Lightning Network**: 107/107 passing
- **Total Pass Rate**: 100%

---

**Thank you for testing INTcoin v1.3.0-beta!**

We appreciate your feedback and bug reports. Together, we're building the future of cryptocurrency.

**Happy Testing! 🚀**

---

**Release Notes Version**: 1.0
**Last Updated**: January 3, 2026
**Prepared By**: INTcoin Core Development Team
