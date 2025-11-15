# INTcoin v1.1.0 Release Notes

**Release Date:** November 15, 2025
**Project:** INTcoin Core - Quantum-Resistant Cryptocurrency
**Lead Developer:** Maddison Lane

---

## Overview

INTcoin v1.1.0 introduces significant network privacy enhancements through Tor integration, comprehensive Lightning Network backend implementation, and extensive documentation improvements.

## Major Features

### 🧅 Tor Network Integration

- **Hardcoded Tor Seed Nodes**: Added built-in Tor hidden service seed node for enhanced privacy and censorship resistance
  - Primary Tor seed: `2nrhdp7i4dricaf362hwnajj27lscbmimggvjetwjhuwgtdnfcurxzyd.onion:9333`
  - Automatic bootstrap via Tor network for users behind restrictive firewalls
  - Support for .onion addresses in peer discovery

- **Tor Bridge Relay**: Official Tor bridge relay now operational
  - Bridge address: 192.168.1.186:9001
  - Helps users bypass censorship and access the network anonymously
  - SOCKS5 proxy support on port 9050

### ⚡ Lightning Network Backend (Complete Implementation)

Complete backend protocol implementation for Lightning Network payment channels:

- **Channel Management**
  - Multi-signature channel creation and funding
  - Commitment transaction management
  - Revocation key handling
  - Channel state updates and synchronization

- **Payment Routing**
  - Hash Time-Locked Contracts (HTLCs)
  - Source-based routing with path finding
  - Invoice generation and payment processing
  - Multi-hop payment forwarding

- **Network Protocol**
  - BOLT (Basis of Lightning Technology) protocol compliance
  - Gossip protocol for network topology
  - Channel announcement and updates
  - Node discovery and peer management

- **Security Features**
  - Penalty transactions for fraudulent channel closures
  - Watchtower support for offline protection
  - Atomic multi-path payments (AMP)
  - Quantum-resistant channel signatures using CRYSTALS-Dilithium

### 📚 Documentation Improvements

- **Lightning Network Documentation**
  - Comprehensive implementation guide
  - Protocol specifications
  - Security considerations
  - Future roadmap for GUI development

- **Website Updates**
  - Enhanced GDPR compliance with cookie consent management
  - Improved theme switcher with WCAG 2.1 Level AA accessibility
  - Professional whitepaper PDF generation scripts
  - Updated repository links and references

## Technical Improvements

### Network Layer
- Enhanced P2P network bootstrapping with Tor support
- Improved peer discovery mechanisms
- Better handling of .onion addresses in network stack

### Build System
- Updated CMake configuration for Lightning Network components
- Improved dependency management
- Better cross-platform compatibility

### Code Quality
- Extensive code documentation
- Implementation status tracking
- Clear separation of completed vs. planned features

## Installation & Upgrade

### New Installations

Download the appropriate binary for your platform from the [Releases page](https://gitlab.com/intcoin/crypto/-/releases).

**macOS:**
```bash
chmod +x intcoind
./intcoind -h
```

**Linux:**
```bash
chmod +x intcoind
./intcoind -server -gen
```

### Upgrading from v1.0.0

Simply replace your existing `intcoind` binary with the new version. Your wallet and blockchain data will remain compatible.

**Important:** If upgrading, consider enabling Tor support in your configuration:

```bash
# Add to ~/.intcoin/intcoin.conf
proxy=127.0.0.1:9050
onlynet=onion
```

## Configuration

### Tor Configuration (Recommended)

To use INTcoin over Tor for maximum privacy:

1. Install Tor on your system
2. Configure INTcoin to use Tor proxy:

```bash
# ~/.intcoin/intcoin.conf
proxy=127.0.0.1:9050
onlynet=onion  # Optional: only connect via Tor
```

3. Restart intcoind

The built-in Tor seed node will automatically be used for bootstrap.

### Lightning Network (Backend Only)

The Lightning Network backend is implemented but does not yet have a user interface. GUI development is planned for future releases. Advanced users can interact with the Lightning protocol through the codebase.

## Known Issues

- Lightning Network GUI not yet available (backend only)
- Watchtower implementation is partial
- Atomic multi-path payments require additional testing

## Security Notes

- This release includes quantum-resistant cryptographic signatures
- All Lightning Network channels use CRYSTALS-Dilithium for commitment transactions
- Tor integration provides network-layer privacy
- Always verify binary signatures before installation

## Project Links

- **Website:** https://international-coin.org
- **Source Code:** https://gitlab.com/intcoin/crypto
- **Documentation:** https://international-coin.org/docs/
- **Whitepaper:** https://international-coin.org/docs/whitepaper.pdf
- **License:** MIT License

## Contributors

- **Maddison Lane** - Lead Developer & Project Creator

## Changelog

### Added
- Tor hidden service seed node integration
- Complete Lightning Network backend protocol
- Tor bridge relay configuration
- Enhanced documentation for Lightning Network
- GDPR cookie consent management on website
- Accessibility-compliant theme switcher
- Professional whitepaper PDF generation

### Changed
- Updated network bootstrapping to prioritize Tor seeds
- Improved P2P peer discovery
- Enhanced website user experience
- Updated repository references

### Fixed
- Various code quality improvements
- Build system enhancements

---

**Full Changelog:** https://gitlab.com/intcoin/crypto/-/compare/v1.0.0...v1.1.0

## Support

For questions, issues, or contributions, please visit our GitLab repository or contact the development team through the project website.

---

🔐 **Verify This Release**

Always verify the GPG signature of downloaded binaries:
```bash
gpg --verify intcoind-v1.1.0-macos-arm64.sig intcoind
```

🌐 **Stay Connected**

- Follow development updates on our website
- Join the community discussions
- Report bugs through GitLab Issues

---

*INTcoin - Quantum-Resistant Cryptocurrency for the Future*
