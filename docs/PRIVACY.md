# Privacy & Anonymous Networking

**Version**: 1.2.0-beta
**Last Updated**: January 2, 2026
**Status**: Production Beta

Complete guide to INTcoin's privacy features including Tor, I2P, and mobile SPV privacy.

## What's New in v1.2.0-beta

- 📱 **Mobile SPV Privacy**: Bloom filter privacy considerations for lightweight clients
- 🔐 **Enhanced Filtering**: Privacy-preserving transaction filtering
- 📲 **Mobile Tor/I2P**: Support for privacy networks on mobile platforms

See also: [SPV & Bloom Filters](SPV_AND_BLOOM_FILTERS.md), [Mobile SDK](MOBILE_SDK.md)

---

## Table of Contents

- [Overview](#overview)
- [Mobile SPV Privacy](#mobile-spv-privacy)
- [Tor Integration](#tor-integration)
- [I2P Integration](#i2p-integration)
- [Configuration](#configuration)
- [Usage](#usage)
- [Security Considerations](#security-considerations)

## Overview

INTcoin provides optional privacy features for users who want to enhance their anonymity:

- **Tor (The Onion Router)**: Route connections through the Tor network
- **I2P (Invisible Internet Project)**: Use I2P for anonymous networking
- **Hybrid Mode**: Combine both Tor and I2P for maximum privacy

### Privacy Levels

| Feature | Privacy Level | Description |
|---------|---------------|-------------|
| **Clearnet Only** | Low | Standard internet connection (IP visible) |
| **Tor Only** | High | All connections through Tor (.onion addresses) |
| **I2P Only** | High | All connections through I2P (.i2p addresses) |
| **Tor + I2P Hybrid** | Very High | Mix of both networks for enhanced privacy |

---

## Mobile SPV Privacy

### Overview

INTcoin v1.2.0-beta introduces mobile SPV wallets using Bloom filters (BIP37) for lightweight blockchain verification. While SPV provides convenience, users should understand the privacy implications.

### Bloom Filter Privacy

**How Bloom Filters Affect Privacy:**

| Aspect | Privacy Level | Explanation |
|--------|---------------|-------------|
| **False Positives** | Medium | Bloom filters intentionally include false positives to obscure which transactions you own |
| **Full Node Knowledge** | Medium-Low | Full nodes can see which transactions match your filter (though with false positives) |
| **Network Observation** | Low | Network observers can correlate filter patterns with transactions |
| **Address Linkage** | Medium | Multiple addresses in the same filter may be linked to the same wallet |

### Privacy Best Practices for Mobile

**1. Use Lower False Positive Rates**
```swift
// iOS SDK example
let wallet = try Wallet.create(mnemonic: mnemonic)
wallet.bloomFilterFPR = 0.001  // Lower FPR = more privacy (default: 0.0001)
```

**2. Rotate Bloom Filters Regularly**
```kotlin
// Android SDK example
wallet.rotateBloomFilter()  // Creates new filter with different randomization
```

**3. Combine with Tor/I2P**
```ini
# Mobile node configuration
tor=1
onlynet=onion

# SPV with Tor
spv.enabled=1
spv.tor=1
```

**4. Use Multiple Wallets**
- Separate wallets for different purposes prevent address linkage
- Don't reuse addresses across wallets

### Mobile Privacy Modes

**Standard SPV** (Default):
- FPR: 0.0001 (0.01%)
- ~100 false positives per 1M transactions
- Good balance of privacy and bandwidth

**High Privacy SPV**:
- FPR: 0.001 (0.1%)
- ~1,000 false positives per 1M transactions
- Better privacy, higher bandwidth

**Maximum Privacy** (Full Node):
- Run full node instead of SPV
- Download entire blockchain
- No bloom filter privacy leaks

### Privacy Comparison

| Wallet Type | Privacy | Bandwidth | Storage |
|-------------|---------|-----------|---------|
| **Mobile SPV (Standard)** | Medium | Low (~50 MB/month) | Minimal (~10 MB) |
| **Mobile SPV (High Privacy)** | Medium-High | Medium (~200 MB/month) | Minimal (~10 MB) |
| **Mobile SPV + Tor** | High | Low-Medium | Minimal (~10 MB) |
| **Desktop Full Node** | Very High | High (~10 GB/month) | High (~50 GB) |
| **Desktop Full Node + Tor** | Maximum | High | High (~50 GB) |

### SPV Privacy Limitations

**Known Privacy Leaks:**

1. **Transaction Broadcasting**: When you broadcast a transaction, nodes know your IP (unless using Tor)
2. **Timing Correlation**: Requesting specific blocks immediately after broadcasting reveals ownership
3. **Filter Updates**: Changing filters after receiving coins reveals which transactions you care about
4. **IP Address**: SPV nodes reveal their IP to full nodes (unless using Tor/I2P)

**Mitigations:**

```ini
# Desktop SPV configuration
spv.enabled=1
spv.tor=1                    # Use Tor for all SPV connections
spv.rotate_filter=3600       # Rotate filter every hour
spv.delay_broadcast=60       # Random delay before broadcasting (0-60 seconds)
spv.fetch_extra_blocks=10    # Fetch extra blocks to obscure interest
```

### Mobile SDK Privacy Features

**iOS (Swift):**
```swift
import INTcoinKit

// High privacy configuration
let config = WalletConfig(
    network: .mainnet,
    bloomFilterFPR: 0.001,      // Higher FPR for more false positives
    torEnabled: true,            // Use Tor (requires Tor app)
    rotateFilterInterval: 3600   // Rotate every hour
)

let wallet = try Wallet.create(mnemonic: mnemonic, config: config)
```

**Android (Kotlin):**
```kotlin
import org.intcoin.sdk.Wallet

// High privacy configuration
val config = WalletConfig(
    network = Network.MAINNET,
    bloomFilterFPR = 0.001,      // Higher FPR
    torEnabled = true,            // Use Orbot
    rotateFilterInterval = 3600   // Rotate every hour
)

val wallet = Wallet.create(mnemonic, config)
```

### Further Reading

- [SPV & Bloom Filters Technical Guide](SPV_AND_BLOOM_FILTERS.md)
- [Mobile SDK Documentation](MOBILE_SDK.md)
- [BIP37: Connection Bloom Filtering](https://github.com/bitcoin/bips/blob/master/bip-0037.mediawiki)

---

## Tor Integration

### Prerequisites

Install Tor on your system:

**Ubuntu/Debian:**
```bash
sudo apt install tor
sudo systemctl start tor
sudo systemctl enable tor
```

**macOS (Homebrew):**
```bash
brew install tor
brew services start tor
```

**FreeBSD:**
```bash
sudo pkg install tor
sudo service tor start
sudo sysrc tor_enable="YES"
```

### Configuration

Edit `~/.intcoin/intcoin.conf`:

```ini
# Enable Tor
tor=1

# Tor SOCKS5 proxy
proxy=127.0.0.1:9050

# Tor control port (optional)
torcontrol=127.0.0.1:9051

# Tor password (if configured)
# torpassword=your_password

# Create hidden service
listenonion=1

# Tor-only mode (no clearnet)
onlynet=onion
```

### Hidden Service

INTcoin can create a Tor hidden service automatically:

```bash
# Start with hidden service
intcoind -listenonion=1

# Check your .onion address
intcoin-cli getnetworkinfo | grep onion
```

Your node will be accessible via:
- `abcdef1234567890.onion:2211`

### Tor Features

- **SOCKS5 Proxy**: Route all connections through Tor
- **Control Port**: Manage Tor circuits programmatically
- **Hidden Services**: Run node as .onion address
- **Stream Isolation**: Each connection uses different Tor circuit
- **Circuit Rotation**: Automatically rotate circuits for privacy

## I2P Integration

### Prerequisites

Install I2P router:

**Java I2P (i2pd):**
```bash
# Ubuntu/Debian
sudo apt install i2pd
sudo systemctl start i2pd
sudo systemctl enable i2pd

# macOS
brew install i2pd
brew services start i2pd
```

### Configuration

Edit `~/.intcoin/intcoin.conf`:

```ini
# Enable I2P
i2p=1

# I2P SAM bridge
i2psam=127.0.0.1:7656

# I2P-only mode
onlynet=i2p
```

### I2P Destination

INTcoin creates an I2P destination automatically:

```bash
# Start with I2P
intcoind -i2p=1

# Check your .i2p address
intcoin-cli getnetworkinfo | grep i2p
```

Your node will be accessible via:
- `abcdef1234567890abcdef1234567890.b32.i2p`

### I2P Features

- **SAM v3 Protocol**: Standard I2P interface
- **Tunnel Creation**: Automatic inbound/outbound tunnels
- **Destination Management**: Persistent I2P identity
- **Garlic Routing**: I2P's packet encryption system

## Configuration

### Hybrid Tor + I2P

Use both networks simultaneously:

```ini
# Enable both
tor=1
i2p=1

# SOCKS5 proxy for Tor
proxy=127.0.0.1:9050

# SAM bridge for I2P
i2psam=127.0.0.1:7656

# Prefer privacy networks
preferonion=1
preferi2p=1

# Allow clearnet fallback
allowclearnet=1

# Privacy peer ratio (50% from Tor/I2P)
privacypeerratio=0.5
```

### Advanced Settings

```ini
# Tor settings
torcontrol=127.0.0.1:9051
torpassword=your_password

# I2P settings
i2p.tunnel.length=3          # Number of hops (1-7)
i2p.tunnel.quantity=2        # Number of tunnels
i2p.tunnel.backup=1          # Backup tunnels

# Privacy options
onlynet=onion                # Tor only
onlynet=i2p                  # I2P only
# Or allow both:
onlynet=onion
onlynet=i2p
```

## Usage

### Starting with Privacy

```bash
# Tor only
intcoind -tor=1 -onlynet=onion

# I2P only
intcoind -i2p=1 -onlynet=i2p

# Both Tor and I2P
intcoind -tor=1 -i2p=1

# With hidden service
intcoind -tor=1 -listenonion=1
```

### Connecting to Privacy Nodes

```bash
# Connect to .onion address
intcoin-cli addnode "abcdef1234567890.onion:2211" "add"

# Connect to .i2p address
intcoin-cli addnode "abcdef1234567890.b32.i2p" "add"
```

### Checking Privacy Status

```bash
# Get network info
intcoin-cli getnetworkinfo

# Check peer connections
intcoin-cli getpeerinfo | grep addr

# Verify Tor status
intcoin-cli getnetworkinfo | grep onion

# Verify I2P status
intcoin-cli getnetworkinfo | grep i2p
```

### RPC Commands

```bash
# Get Tor status
intcoin-cli tor_getstatus

# Create new Tor circuit
intcoin-cli tor_newcircuit

# Get I2P tunnel stats
intcoin-cli i2p_gettunnelstats
```

## Security Considerations

### Best Practices

1. **Use Tor Browser**: Access block explorer via Tor Browser
2. **Don't Mix**: Avoid mixing Tor and clearnet for same wallet
3. **Stream Isolation**: Keep different activities on different circuits
4. **Update Regularly**: Keep Tor/I2P software up to date
5. **No Personal Info**: Don't leak personal information

### Privacy Threats

**Address Correlation:**
- Reusing addresses links transactions
- Use new address for each transaction
- Enable HD wallet auto-generation

**Network Analysis:**
- Timing attacks can correlate transactions
- Use Tor for all connections
- Add random delays

**IP Leaks:**
- Ensure Tor/I2P is running before starting node
- Use `onlynet` to prevent clearnet leaks
- Check for DNS leaks

### Tor-Specific Security

```ini
# Strict Tor-only mode
tor=1
onlynet=onion
bind=127.0.0.1
listen=1

# No clearnet DNS
proxy=127.0.0.1:9050
dns=0

# Strict stream isolation
tor.streamisolation=1
```

### I2P-Specific Security

```ini
# Strict I2P-only mode
i2p=1
onlynet=i2p
bind=127.0.0.1
listen=1

# Tunnel security
i2p.tunnel.length=5          # More hops = more privacy
i2p.tunnel.quantity=3        # More tunnels = more reliability
```

## Troubleshooting

### Tor Not Connecting

```bash
# Check Tor is running
systemctl status tor

# Check SOCKS5 port
netstat -an | grep 9050

# Test Tor connection
curl --socks5 127.0.0.1:9050 https://check.torproject.org
```

### I2P Not Connecting

```bash
# Check I2P is running
systemctl status i2pd

# Check SAM port
netstat -an | grep 7656

# Check I2P console
# Open: http://127.0.0.1:7070
```

### Hidden Service Not Created

```bash
# Check Tor control port
intcoin-cli getnetworkinfo | grep torcontrol

# Check logs
tail -f ~/.intcoin/debug.log | grep -i tor

# Verify torrc configuration
cat /etc/tor/torrc | grep ControlPort
```

### Performance Issues

**Tor is slow:**
- Normal for Tor (3-5x slower than clearnet)
- Consider Tor bridges if censored
- Use faster exit nodes

**I2P is slow:**
- Tunnels need time to establish (30-60 seconds)
- Increase tunnel quantity for better performance
- Wait for router integration

## Privacy Checklist

- [ ] Tor/I2P installed and running
- [ ] INTcoin configured for privacy network
- [ ] Hidden service/destination created
- [ ] No clearnet connections (check `getpeerinfo`)
- [ ] Using new address for each transaction
- [ ] HD wallet enabled
- [ ] No personal information in transactions
- [ ] Regular Tor circuit rotation
- [ ] Updated Tor/I2P software

## Resources

- **Tor Project**: https://www.torproject.org
- **I2P Project**: https://geti2p.net
- **Privacy Best Practices**: https://international-coin.org/privacy
- **Security Guide**: [SECURITY.md](SECURITY.md)

---

**Remember**: Privacy is not binary. Use multiple layers and best practices for maximum anonymity.
