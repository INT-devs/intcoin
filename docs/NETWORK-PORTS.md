# INTcoin Network Port Assignments

**Copyright (c) 2025 INTcoin Core (Maddison Lane)**

## Overview

INTcoin uses a unique port range to avoid conflicts with Bitcoin and other cryptocurrencies. All network services run on independent ports to ensure complete isolation.

---

## Port Allocation Strategy

**Base Port Range**: 9330-9349 (20 ports reserved)

This range is:
- ✅ Not used by Bitcoin (8333, 8334, 18333, 18444)
- ✅ Not used by common I2P implementations (7656-7660, 4444-4447)
- ✅ Not used by Tor (9050, 9051, 9150, 9151)
- ✅ Not conflicting with other major cryptocurrencies

---

## Port Assignments

### Main Network Ports

| Service | Port | Protocol | Description |
|---------|------|----------|-------------|
| **P2P Network** | 9333 | TCP | Main peer-to-peer protocol (mainnet) |
| **RPC Server** | 9334 | TCP/HTTP | JSON-RPC API server |
| **Lightning P2P** | 9335 | TCP | Lightning Network peer protocol |
| **I2P SAM Bridge** | 9336 | TCP | I2P SAM (Simple Anonymous Messaging) |
| **I2P Router** | 9337 | TCP/UDP | I2P internal routing |
| **Tor Control** | 9338 | TCP | Tor control port (if using custom instance) |
| **Watchtower** | 9339 | TCP | Lightning watchtower protocol |
| **Block Explorer** | 9340 | HTTP | Web-based blockchain explorer |
| **WebSocket API** | 9341 | WebSocket | Real-time blockchain updates |
| **gRPC API** | 9342 | gRPC | High-performance API |

### Testnet Ports (Add +10000)

| Service | Port | Protocol | Description |
|---------|------|----------|-------------|
| **P2P Network (Testnet)** | 19333 | TCP | Testnet peer protocol |
| **RPC Server (Testnet)** | 19334 | TCP/HTTP | Testnet RPC |
| **Lightning P2P (Testnet)** | 19335 | TCP | Testnet Lightning |
| **I2P SAM (Testnet)** | 19336 | TCP | Testnet I2P |
| **I2P Router (Testnet)** | 19337 | TCP/UDP | Testnet I2P routing |
| **Watchtower (Testnet)** | 19339 | TCP | Testnet watchtowers |

### Regtest/Development Ports (Add +20000)

| Service | Port | Protocol | Description |
|---------|------|----------|-------------|
| **P2P Network (Regtest)** | 29333 | TCP | Regtest peer protocol |
| **RPC Server (Regtest)** | 29334 | TCP/HTTP | Regtest RPC |
| **Lightning P2P (Regtest)** | 29335 | TCP | Regtest Lightning |

---

## I2P Configuration

### SAM Bridge

INTcoin uses the **SAM v3.1 protocol** to connect to I2P networks.

**Default Configuration**:
```ini
[i2p]
enabled=1
sam.host=127.0.0.1
sam.port=9336
router.port=9337
session.name=intcoin-mainnet
transient=false
tunnel.length=3
tunnel.quantity=2
```

**Features**:
- Independent SAM bridge on port 9336 (not using standard 7656)
- Custom router port 9337 for I2P routing
- Persistent destination keys (non-transient)
- 3-hop tunnels for privacy
- 2 backup tunnels for reliability

### I2P Address Format

INTcoin nodes on I2P are identified by `.b32.i2p` addresses:
```
example: abcdefghijklmnopqrstuvwxyz234567.b32.i2p:9333
```

---

## Lightning Network Ports

### Complete Independence from Bitcoin Lightning

INTcoin Lightning Network is **100% independent** from Bitcoin's Lightning implementation:

| Aspect | Bitcoin LN | INTcoin LN |
|--------|-----------|------------|
| **Port** | 9735 | **9335** |
| **Cryptography** | ECDSA | **Dilithium5 + Kyber1024** |
| **Messages** | BOLT spec | **Modified for post-quantum** |
| **Invoice Format** | lnbc prefix | **lnint prefix** |
| **Network ID** | Bitcoin mainnet | **INTcoin mainnet** |

**No Bitcoin Dependency**: INTcoin Lightning runs entirely on INTcoin blockchain with zero interaction with Bitcoin Lightning Network.

### Lightning Service Ports

| Service | Port | Description |
|---------|------|-------------|
| Lightning P2P | 9335 | Channel operations, HTLCs |
| Watchtower | 9339 | Breach monitoring service |
| Lightning RPC | 9334 | Shared with main RPC (different endpoint) |

---

## Tor Integration

### Hidden Service Configuration

INTcoin can run as a Tor hidden service:

**Port Mapping**:
```
HiddenServicePort 9333 127.0.0.1:9333  # P2P
HiddenServicePort 9335 127.0.0.1:9335  # Lightning
HiddenServicePort 9339 127.0.0.1:9339  # Watchtower
```

**Onion Address Format**:
```
v3: abcdefghijklmnopqrstuvwxyz234567abcdefghijklmnopqrst.onion:9333
```

**Existing Seed Node**:
```
2nrhdp7i4dricaf362hwnajj27lscbmimggvjetwjhuwgtdnfcurxzyd.onion:9333
```

---

## Port Forwarding Guide

### Router Configuration

For mainnet node operation, forward these ports:

**Essential Ports**:
```
9333 TCP (P2P) - Required for incoming connections
9335 TCP (Lightning) - Required for Lightning channels
```

**Optional Ports**:
```
9339 TCP (Watchtower) - If running watchtower service
9340 HTTP (Explorer) - If running public explorer
```

### Firewall Rules (Linux)

```bash
# Allow P2P
sudo ufw allow 9333/tcp

# Allow Lightning
sudo ufw allow 9335/tcp

# Allow Watchtower (optional)
sudo ufw allow 9339/tcp
```

### macOS Firewall

```bash
# Add firewall rules
sudo /usr/libexec/ApplicationFirewall/socketfilterfw --add /path/to/intcoind
sudo /usr/libexec/ApplicationFirewall/socketfilterfw --unblockapp /path/to/intcoind
```

---

## Network Address Examples

### Mainnet Nodes

**IPv4**:
```
192.168.1.100:9333
203.0.113.50:9333
```

**IPv6**:
```
[2001:db8::1]:9333
[::1]:9333
```

**Tor Hidden Service**:
```
2nrhdp7i4dricaf362hwnajj27lscbmimggvjetwjhuwgtdnfcurxzyd.onion:9333
```

**I2P Destination**:
```
abcdefghijklmnopqrstuvwxyz234567.b32.i2p:9333
```

### Lightning Nodes

**Public Node**:
```
node_id@203.0.113.50:9335
node_id@[2001:db8::1]:9335
```

**Tor Lightning Node**:
```
node_id@example.onion:9335
```

**I2P Lightning Node**:
```
node_id@example.b32.i2p:9335
```

---

## Port Conflict Resolution

### Checking for Conflicts

```bash
# Linux/macOS - Check if port is in use
sudo lsof -i :9333
sudo netstat -tuln | grep 9333

# FreeBSD
sockstat -4 -l | grep 9333

# Windows
netstat -ano | findstr :9333
```

### Changing Ports

Edit `intcoin.conf`:
```ini
[network]
# Main P2P port
port=9333

# RPC port
rpcport=9334

[lightning]
# Lightning P2P port
port=9335

# Watchtower port
watchtower.port=9339

[i2p]
# I2P SAM bridge
sam.port=9336

# I2P router
router.port=9337
```

---

## Security Considerations

### Port Exposure

**Safe to Expose Publicly**:
- ✅ 9333 (P2P) - Required for full node
- ✅ 9335 (Lightning) - Required for Lightning routing
- ✅ 9339 (Watchtower) - Required for watchtower service

**Should NOT Expose Publicly**:
- ❌ 9334 (RPC) - Administrative access, use SSH tunnel
- ❌ 9336/9337 (I2P) - Internal use only
- ❌ 9342 (gRPC) - API access, use authentication

### RPC Security

**Never expose RPC to public internet**. Use:

1. **SSH Tunnel**:
```bash
ssh -L 9334:localhost:9334 user@remote-node
```

2. **VPN Access**:
```bash
# Configure wireguard/openvpn
# Access RPC via VPN IP
```

3. **Authentication**:
```ini
[rpc]
user=intcoin
password=strong-random-password-here
allowip=127.0.0.1
allowip=192.168.1.0/24
```

---

## Comparison with Other Projects

| Project | P2P Port | Lightning | RPC | I2P | Notes |
|---------|----------|-----------|-----|-----|-------|
| **Bitcoin** | 8333 | 9735 | 8332 | 8334 | Original implementation |
| **Litecoin** | 9333 | - | 9332 | - | ⚠️ Conflicts with INTcoin! |
| **Monero** | 18080 | - | 18081 | - | Privacy-focused |
| **Ethereum** | 30303 | - | 8545 | - | Smart contracts |
| **INTcoin** | **9333** | **9335** | **9334** | **9336-9337** | Quantum-resistant |

**Note**: INTcoin's port 9333 conflicts with Litecoin. Do not run both on the same machine without reconfiguring one of them.

---

## Configuration Examples

### Minimal Configuration (intcoin.conf)

```ini
[network]
port=9333
maxconnections=125

[rpc]
port=9334
user=intcoin
password=change-this-password
```

### Full Node with Lightning (intcoin.conf)

```ini
[network]
port=9333
maxconnections=125
listen=1

[rpc]
port=9334
user=intcoin
password=strong-password-here
allowip=127.0.0.1

[lightning]
enabled=1
port=9335
announce=1

[watchtower]
enabled=1
port=9339
```

### Privacy Node with Tor + I2P (intcoin.conf)

```ini
[network]
port=9333
onlynet=tor
onlynet=i2p

[tor]
enabled=1
control.port=9338
hidden_service=1

[i2p]
enabled=1
sam.port=9336
router.port=9337
transient=false
tunnel.length=3

[lightning]
enabled=1
port=9335
tor.enabled=1
i2p.enabled=1
```

---

## Version History

- **v1.0** (2025-11-20): Initial port allocation documentation
  - Defined unique port range 9330-9349
  - Complete isolation from Bitcoin and other projects
  - I2P integration with custom ports
  - Lightning Network independence documented

---

## References

- [IANA Port Registry](https://www.iana.org/assignments/service-names-port-numbers/)
- [Bitcoin Protocol Documentation](https://en.bitcoin.it/wiki/Protocol_documentation)
- [I2P SAM Protocol](https://geti2p.net/en/docs/api/samv3)
- [Lightning Network BOLT Specifications](https://github.com/lightning/bolts)

---

**Last Updated**: November 20, 2025
**Document Version**: 1.0
**Status**: Complete ✅
