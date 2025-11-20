# INTcoin I2P Network Integration

**Copyright (c) 2025 INTcoin Core (Maddison Lane)**

## Overview

INTcoin integrates with the **I2P (Invisible Internet Project)** network to provide anonymous peer-to-peer communication. This integration uses **independent ports** (9336-9337) to avoid conflicts with standard I2P implementations and other projects.

---

## What is I2P?

I2P is an anonymous overlay network that provides:
- **End-to-end encryption** for all connections
- **Distributed architecture** with no central points of failure
- **Garlic routing** (similar to Tor's onion routing, but bidirectional)
- **Hidden services** (destinations) that are hard to censor
- **NAT traversal** - works behind firewalls without port forwarding

### I2P vs Tor

| Feature | I2P | Tor |
|---------|-----|-----|
| **Architecture** | Fully distributed peer-to-peer | Directory-based |
| **Routing** | Garlic routing (bidirectional) | Onion routing (unidirectional) |
| **Use Case** | Hidden services, P2P apps | Exit to clearnet |
| **Speed** | Faster for P2P | Faster for web browsing |
| **NAT Traversal** | Built-in | Requires configuration |

**Why I2P for INTcoin**: I2P is specifically designed for peer-to-peer applications and hidden services, making it ideal for cryptocurrency networks.

---

## Port Configuration

### Independent Port Assignments

INTcoin uses **unique ports** to enable running alongside other I2P applications:

| Service | Standard I2P | INTcoin I2P | Purpose |
|---------|--------------|-------------|---------|
| **SAM Bridge** | 7656 | **9336** | Simple Anonymous Messaging API |
| **Router** | 7654-7660 | **9337** | I2P internal routing |
| **P2P over I2P** | N/A | **9333** | Blockchain P2P protocol |

**Benefits**:
- ✅ No port conflicts with existing I2P installations
- ✅ Can run both standard I2P and INTcoin simultaneously
- ✅ Easy to identify INTcoin-specific I2P traffic
- ✅ Firewall rules can target INTcoin specifically

---

## SAM Protocol (v3.1)

INTcoin uses the **SAM (Simple Anonymous Messaging)** protocol to communicate with the I2P router.

### Protocol Overview

```
Client (INTcoin)  <--TCP-->  SAM Bridge (9336)  <--I2P-->  I2P Network
```

**SAM Commands**:
1. `HELLO` - Establish connection and negotiate version
2. `SESSION CREATE` - Create a session with destination keys
3. `STREAM CONNECT` - Connect to another I2P destination
4. `STREAM ACCEPT` - Accept incoming connections
5. `STREAM FORWARD` - Forward local port to I2P

### Session Configuration

```cpp
I2PSessionConfig config;
config.session_name = "intcoin-mainnet";
config.sam_port = 9336;
config.router_port = 9337;
config.tunnel_length = 3;        // 3-hop tunnels
config.tunnel_quantity = 2;      // 2 backup tunnels
config.transient = false;        // Persistent keys
```

---

## I2P Addresses

### Destination Format

I2P destinations are 387+ byte public keys, displayed as:
- **Base64**: `abcd...xyz~` (516 characters)
- **Base32**: `abcdefghijklmnopqrstuvwxyz234567.b32.i2p` (52 characters)

Example INTcoin node:
```
w7i3dnoxu7feghlmwdjq5eugiq5rbm7t3uyapoqkkxpjhzeuioha.b32.i2p:9333
```

### Address Book

INTcoin maintains an address book mapping human-readable names to I2P destinations:

```ini
[addresses]
seed1=abcdefghijklmnopqrstuvwxyz234567.b32.i2p
seed2=bcdefghijklmnopqrstuvwxyza234567.b32.i2p
friend-node=cdefghijklmnopqrstuvwxyzab234567.b32.i2p
```

---

## Configuration

### Basic Configuration (`intcoin.conf`)

```ini
[i2p]
# Enable I2P network
enabled=1

# SAM bridge connection
sam.host=127.0.0.1
sam.port=9336

# I2P router port
router.port=9337

# Session settings
session.name=intcoin-mainnet
transient=false

# Tunnel configuration
tunnel.length=3
tunnel.quantity=2
tunnel.variance=0
tunnel.backup_quantity=1

# Bandwidth limits (KB/s, 0 = unlimited)
inbound_bandwidth=0
outbound_bandwidth=0

# Privacy
reduce_idle=true
idle_timeout=300
```

### Privacy-Enhanced Configuration

```ini
[i2p]
enabled=1
sam.port=9336
router.port=9337

# Maximum privacy settings
tunnel.length=4              # Longer tunnels (slower but more private)
tunnel.quantity=3            # More backup tunnels
tunnel.variance=1            # Randomize tunnel length
transient=true               # Generate new keys each session
reduce_idle=false            # Keep tunnels alive
```

### Performance-Optimized Configuration

```ini
[i2p]
enabled=1
sam.port=9336
router.port=9337

# Faster but less private
tunnel.length=2              # Shorter tunnels
tunnel.quantity=1            # Fewer tunnels
transient=false              # Persistent keys
reduce_idle=true             # Close idle tunnels
idle_timeout=60              # Quick cleanup
```

---

## Usage Examples

### Starting INTcoin with I2P

```bash
# Basic start with I2P
./intcoind -i2p

# Specify custom SAM port
./intcoind -i2p -i2psam=9336

# I2P-only mode (no clearnet)
./intcoind -onlynet=i2p

# Combined Tor + I2P
./intcoind -i2p -tor
```

### Connecting to I2P Nodes

```bash
# Add I2P seed node
./intcoin-cli addnode "w7i3dnoxu7feghlmwdjq5eugiq5rbm7t3uyapoqkkxpjhzeuioha.b32.i2p:9333" "add"

# List I2P peers
./intcoin-cli getpeerinfo | grep "i2p"

# Get my I2P destination
./intcoin-cli getnetworkinfo | grep "i2p"
```

### Programming Example

```cpp
#include "intcoin/i2p.h"

using namespace intcoin::i2p;

// Create I2P manager
I2PSessionConfig config;
config.sam_port = 9336;
config.tunnel_length = 3;

I2PManager manager(config);

// Initialize and connect
if (manager.initialize()) {
    if (manager.connect_to_i2p()) {
        std::cout << "My I2P address: " << manager.get_my_address() << std::endl;

        // Connect to peer
        auto socket = manager.connect_to_peer(
            "w7i3dnoxu7feghlmwdjq5eugiq5rbm7t3uyapoqkkxpjhzeuioha.b32.i2p",
            9333
        );

        if (socket) {
            std::cout << "Connected to peer!" << std::endl;
        }
    }
}
```

---

## Tunnel Configuration

### What are Tunnels?

I2P uses **tunnels** to route traffic anonymously. Each connection uses two tunnels:
- **Outbound tunnel**: Your node → Destination
- **Inbound tunnel**: Destination → Your node

### Tunnel Parameters

**Tunnel Length** (`tunnel.length`):
- **1 hop**: Fastest, least private
- **2 hops**: Balanced (default for performance)
- **3 hops**: Standard privacy (default for mainnet)
- **4+ hops**: Maximum privacy, slower

**Tunnel Quantity** (`tunnel.quantity`):
- **1**: Minimum (single point of failure)
- **2**: Standard (recommended)
- **3+**: High reliability, more resources

**Tunnel Variance** (`tunnel.variance`):
- **0**: Fixed tunnel length (predictable)
- **1**: Randomize ±1 hop (recommended for privacy)
- **2+**: More randomization

### Tunnel Statistics

```cpp
auto stats = manager.get_tunnel_stats();
std::cout << "Inbound tunnels: " << stats.inbound_tunnels << std::endl;
std::cout << "Outbound tunnels: " << stats.outbound_tunnels << std::endl;
std::cout << "Bytes sent: " << stats.bytes_sent << std::endl;
std::cout << "Bytes received: " << stats.bytes_received << std::endl;
```

---

## Security Considerations

### Destination Key Management

**Persistent Keys** (`transient=false`):
- ✅ Stable I2P address (like a static IP)
- ✅ Peers can reconnect to you
- ✅ Better for long-running nodes
- ❌ Linkable across sessions

**Transient Keys** (`transient=true`):
- ✅ New address each session (more private)
- ✅ No long-term tracking
- ❌ Peers cannot reconnect
- ❌ Not suitable for seed nodes

**Recommendation**: Use persistent keys for seed nodes, transient keys for privacy-focused wallets.

### Bandwidth Limits

Unlimited bandwidth can:
- Consume your network connection
- Reveal usage patterns to ISP
- Correlate with blockchain activity

**Recommended limits**:
```ini
inbound_bandwidth=500    # 500 KB/s = 4 Mbps
outbound_bandwidth=500   # 500 KB/s = 4 Mbps
```

### Firewall Configuration

I2P **does not require port forwarding** - it uses NAT traversal. However, you can optionally forward ports for better connectivity:

```bash
# Optional: Forward I2P router port (UDP)
sudo ufw allow 9337/udp

# Do NOT forward SAM bridge (internal use only)
# SAM port 9336 should NEVER be exposed to internet
```

---

## Troubleshooting

### Connection Issues

**Problem**: Cannot connect to I2P network

**Solutions**:
1. Check if I2P router is running:
   ```bash
   netstat -tuln | grep 9336
   ```

2. Verify SAM bridge is accessible:
   ```bash
   telnet 127.0.0.1 9336
   ```

3. Check INTcoin logs:
   ```bash
   tail -f ~/.intcoin/debug.log | grep "I2P"
   ```

4. Restart I2P router and INTcoin

### Slow Performance

**Problem**: I2P connections are very slow

**Solutions**:
1. Reduce tunnel length:
   ```ini
   tunnel.length=2
   ```

2. Reduce tunnel quantity:
   ```ini
   tunnel.quantity=1
   ```

3. Disable tunnel variance:
   ```ini
   tunnel.variance=0
   ```

4. Wait for tunnel build (can take 30-60 seconds)

### Address Resolution Failures

**Problem**: Cannot resolve .b32.i2p addresses

**Solutions**:
1. Ensure address is correct (52 characters + `.b32.i2p`)
2. Check address book file exists: `~/.intcoin/i2p_addresses.txt`
3. Manually add to address book:
   ```bash
   ./intcoin-cli i2p-addaddress "seed1" "w7i3dn...ioha.b32.i2p"
   ```

---

## Integration with Other Networks

### Tor + I2P Dual Stack

Run both Tor and I2P simultaneously:

```ini
[tor]
enabled=1
control.port=9338

[i2p]
enabled=1
sam.port=9336
router.port=9337

[network]
# Connect via both networks
onlynet=tor
onlynet=i2p
```

### Clearnet + I2P

Allow both clearnet and I2P:

```ini
[i2p]
enabled=1

[network]
# No onlynet restriction - use all networks
```

---

## Performance Benchmarks

### Latency Comparison

| Network | Avg Latency | Notes |
|---------|-------------|-------|
| Clearnet | 10-50 ms | Direct connection |
| Tor | 200-500 ms | 3-hop circuit |
| I2P | 300-800 ms | 6 hops total (3 in, 3 out) |

### Throughput

| Network | Download | Upload | Notes |
|---------|----------|--------|-------|
| Clearnet | Full bandwidth | Full bandwidth | No overhead |
| Tor | 1-5 MB/s | 1-5 MB/s | Depends on exit node |
| I2P | 500 KB/s - 2 MB/s | 500 KB/s - 2 MB/s | Peer-to-peer optimized |

**Note**: I2P performance improves over time as tunnels are built and the network graph is learned.

---

## Advanced Features

### Address Book Management

```bash
# Add address
./intcoin-cli i2p-addaddress "mynode" "w7i3dn...ioha.b32.i2p"

# Resolve address
./intcoin-cli i2p-resolve "mynode"

# List all addresses
./intcoin-cli i2p-listaddresses

# Remove address
./intcoin-cli i2p-removeaddress "mynode"
```

### Service Discovery

```bash
# Announce node on I2P
./intcoin-cli i2p-announce

# Discover other nodes
./intcoin-cli i2p-discover

# List seed nodes
./intcoin-cli i2p-getseedlist
```

### Statistics and Monitoring

```bash
# Get I2P statistics
./intcoin-cli i2p-stats

# Check tunnel status
./intcoin-cli i2p-tunnels

# View I2P peers
./intcoin-cli i2p-peers
```

---

## Development

### Building with I2P Support

```bash
cd build
cmake .. -DENABLE_I2P=ON
make -j$(nproc)
```

### API Reference

See [include/intcoin/i2p.h](../include/intcoin/i2p.h) for complete API documentation.

**Key Classes**:
- `I2PManager` - High-level I2P management
- `SAMSession` - SAM v3.1 protocol implementation
- `I2PAddressResolver` - Address book and resolution
- `I2PServiceDiscovery` - Node discovery protocol

---

## Comparison with Bitcoin I2P

| Feature | Bitcoin | INTcoin |
|---------|---------|---------|
| **SAM Port** | 7656 (standard) | **9336** (custom) |
| **Router Port** | 7654-7660 | **9337** (custom) |
| **P2P Port** | 8334 over I2P | **9333** over I2P |
| **Address Format** | .b32.i2p | .b32.i2p (same) |
| **Protocol** | Bitcoin P2P | INTcoin P2P (quantum-resistant) |

**Key Difference**: INTcoin uses **independent ports** to avoid conflicts and enable side-by-side operation with Bitcoin and other I2P applications.

---

## FAQ

**Q: Do I need to install I2P separately?**
A: No, INTcoin includes a built-in I2P client. However, you can connect to an existing I2P router if preferred.

**Q: Can I run INTcoin I2P alongside Java I2P?**
A: Yes! INTcoin uses different ports (9336/9337) so there are no conflicts.

**Q: Is I2P slower than Tor?**
A: For peer-to-peer applications like INTcoin, I2P is often faster than Tor because it's bidirectional and optimized for hidden services.

**Q: Do I need port forwarding?**
A: No, I2P has built-in NAT traversal. Port forwarding can improve connectivity but is not required.

**Q: How private is I2P?**
A: I2P provides strong anonymity with garlic routing. Use longer tunnels (3-4 hops) for maximum privacy.

**Q: Can I use I2P on mobile?**
A: Yes, but battery usage will be higher due to tunnel maintenance. Consider using transient keys and shorter tunnels.

---

## Resources

- **I2P Project**: https://geti2p.net/
- **SAM Protocol Documentation**: https://geti2p.net/en/docs/api/samv3
- **INTcoin I2P Header**: [include/intcoin/i2p.h](../include/intcoin/i2p.h)
- **Port Configuration**: [docs/NETWORK-PORTS.md](NETWORK-PORTS.md)

---

**Last Updated**: November 20, 2025
**Version**: 1.0
**Status**: Production Ready ✅
