# INTcoin TOR Support Guide

## Overview

INTcoin provides comprehensive support for The Onion Router (TOR) network, enabling anonymous and privacy-preserving peer-to-peer connections. This guide covers setup, configuration, and usage of TOR features in INTcoin.

## Features

### 1. SOCKS5 Proxy Support
- Connect to peers through TOR using SOCKS5 proxy
- Automatic proxy detection and configuration
- Support for authenticated SOCKS5 connections
- Connection timeout and retry logic

### 2. Hidden Service (v3 Onion Addresses)
- Host your node as a TOR hidden service
- Generate and manage .onion addresses
- Automatic key generation and management
- Support for modern v3 onion addresses (56 characters)

### 3. Onion-Only Mode
- Operate exclusively over TOR network
- Reject all clearnet connections
- Enhanced privacy and anonymity

### 4. Mixed Network Support
- Simultaneously accept clearnet and TOR connections
- Automatic peer discovery over both networks
- Smart routing between TOR and clearnet peers

## Prerequisites

### Installing TOR

#### Linux (Debian/Ubuntu)
```bash
sudo apt-get update
sudo apt-get install tor
```

#### Linux (Fedora/RHEL)
```bash
sudo dnf install tor
```

#### FreeBSD
```bash
sudo pkg install tor
```

#### macOS
```bash
brew install tor
```

#### Windows
Download TOR Expert Bundle from https://www.torproject.org/download/tor/

### Starting TOR

#### Linux/FreeBSD
```bash
sudo systemctl start tor
sudo systemctl enable tor
```

#### macOS
```bash
brew services start tor
```

#### Manual Start
```bash
tor &
```

### Verifying TOR is Running

```bash
# Check if TOR SOCKS proxy is listening
netstat -an | grep 9050

# Or try connecting
curl --socks5 localhost:9050 https://check.torproject.org
```

## Configuration

### Basic TOR Configuration

Add the following to your `intcoin.conf` file:

```ini
# Enable TOR
tor=1

# TOR SOCKS5 proxy settings
torproxy=127.0.0.1:9050

# Optional: SOCKS5 authentication
# torproxyuser=username
# torproxypass=password

# TOR control port (for advanced features)
torcontrol=127.0.0.1:9051

# Optional: Control port password
# torpassword=your_password
```

### Hidden Service Configuration

To run your node as a TOR hidden service:

```ini
# Enable hidden service
hiddenservice=1

# Hidden service data directory
hiddensrvdir=/var/lib/intcoin/onion_service

# Port mappings (virtual_port:local_port)
hiddensrvport=8333:9333

# Auto-publish hidden service address to peers
discoverhiddenservice=1
```

### Onion-Only Mode

For maximum privacy, configure onion-only mode:

```ini
# Only connect through TOR
onlynet=onion

# Disable DNS lookups
nodnsseed=1

# Listen only on localhost (TOR will handle external connections)
bind=127.0.0.1
```

### Advanced Configuration

```ini
# TOR connection timeout (milliseconds)
tortimeout=30000

# Maximum onion peers
maxonionpeers=64

# Enable TOR stream isolation
torisolation=1

# Prefer onion peers over clearnet
preferonion=1
```

## Usage

### Command-Line Options

```bash
# Run with TOR enabled
./intcoind -tor

# Run as hidden service
./intcoind -tor -hiddenservice

# Run in onion-only mode
./intcoind -onlynet=onion

# Specify custom TOR proxy
./intcoind -torproxy=127.0.0.1:9050

# Enable TOR control
./intcoind -torcontrol=127.0.0.1:9051
```

### Connecting to Onion Peers

#### Manual Connection
```bash
# Connect to specific .onion address
./intcoin-cli addnode "abc...xyz.onion:9333" add

# Get info about onion peers
./intcoin-cli getpeerinfo | grep onion
```

#### Automatic Discovery
INTcoin will automatically discover and connect to .onion addresses advertised by peers.

### Checking TOR Status

```bash
# Get TOR network info
./intcoin-cli getnetworkinfo

# Expected output includes:
# "tor_active": true
# "tor_onion_address": "your_address.onion"
# "tor_proxy": "127.0.0.1:9050"
```

### Getting Your Onion Address

```bash
# Get your hidden service address
./intcoin-cli gethiddenserviceaddress

# Example output:
# "abcdef1234567890abcdef1234567890abcdef1234567890abcdef.onion:9333"
```

## TOR Configuration File (torrc)

For advanced users, you can configure TOR directly via `/etc/tor/torrc`:

```
# SOCKS5 proxy
SOCKSPort 9050

# Control port
ControlPort 9051

# Cookie authentication file
CookieAuthentication 1

# Hidden service for INTcoin
HiddenServiceDir /var/lib/tor/intcoin/
HiddenServicePort 8333 127.0.0.1:9333

# Optional: Circuit isolation
IsolateDestAddr 1
IsolateDestPort 1
```

After editing torrc, restart TOR:
```bash
sudo systemctl restart tor
```

## Security Considerations

### Best Practices

1. **Onion-Only Mode**: For maximum privacy, use onion-only mode
2. **No Leaks**: Ensure all connections go through TOR (disable UPnP, etc.)
3. **Regular Updates**: Keep TOR updated to the latest version
4. **Stream Isolation**: Enable TOR stream isolation for better privacy
5. **Firewall Rules**: Configure firewall to block non-TOR connections if using onion-only mode

### Privacy Features

- **No IP Leakage**: Your real IP is never exposed to peers
- **Anonymous Transactions**: Broadcast transactions anonymously
- **Hidden Location**: Node location cannot be determined
- **Resistant to Analysis**: Traffic analysis is significantly harder

### Potential Issues

1. **Performance**: TOR introduces latency (3-5 second ping times)
2. **Bandwidth**: Lower bandwidth compared to clearnet
3. **Peer Discovery**: Slower initial peer discovery
4. **Block Propagation**: Slightly delayed block propagation

## Troubleshooting

### TOR Connection Failed

```bash
# Check if TOR is running
systemctl status tor

# Check TOR logs
sudo journalctl -u tor -f

# Verify SOCKS port is open
telnet 127.0.0.1 9050
```

### Hidden Service Not Working

```bash
# Check hidden service directory permissions
ls -la /var/lib/tor/intcoin/

# Should be owned by TOR user
sudo chown -R tor:tor /var/lib/tor/intcoin/
sudo chmod 700 /var/lib/tor/intcoin/

# Check TOR logs for errors
sudo tail -f /var/log/tor/log
```

### No Onion Peers

```bash
# Manually add onion seed nodes
./intcoin-cli addnode "seed1.intcoin.onion:9333" add
./intcoin-cli addnode "seed2.intcoin.onion:9333" add

# Check if TOR is properly configured
./intcoin-cli getnetworkinfo
```

### High Latency

```bash
# Check your TOR circuit
curl --socks5 localhost:9050 https://check.torproject.org/api/ip

# Request new TOR circuit
echo -e 'AUTHENTICATE ""\r\nSIGNAL NEWNYM\r\n' | nc 127.0.0.1 9051
```

## Testing TOR Connectivity

### Test Script

```bash
#!/bin/bash
# test-tor.sh - Verify TOR functionality

echo "Testing TOR proxy..."
curl --socks5 localhost:9050 https://check.torproject.org/api/ip

echo -e "\nTesting INTcoin TOR connection..."
./intcoin-cli getnetworkinfo | grep -A 5 tor

echo -e "\nGetting onion peers..."
./intcoin-cli getpeerinfo | grep -B 2 ".onion"

echo -e "\nDone!"
```

## Performance Tuning

### TOR Configuration for Better Performance

```
# /etc/tor/torrc

# Increase circuit build timeout
CircuitBuildTimeout 60

# Use faster guard nodes
NumEntryGuards 8

# Prefer high-bandwidth nodes
UseMicrodescriptors 0
```

### INTcoin Configuration

```ini
# Reduce connection count for TOR
maxconnections=64

# Increase timeout for TOR
timeout=300

# Reduce block size to improve propagation
maxblocksize=500000
```

## Monitoring

### Log Analysis

```bash
# Watch INTcoin TOR connections
tail -f ~/.intcoin/debug.log | grep -i tor

# Monitor TOR circuits
watch "echo -e 'AUTHENTICATE \"\"\r\nGETINFO circuit-status\r\n' | nc 127.0.0.1 9051"
```

### Metrics

```bash
# Get TOR statistics
./intcoin-cli getnetworkinfo | jq '.tor_stats'

# Example output:
{
  "onion_peers": 8,
  "clearnet_peers": 0,
  "connections_through_tor": 8,
  "hidden_service_active": true,
  "our_onion_address": "abc...xyz.onion:9333"
}
```

## Advanced Features

### TOR Control Protocol

INTcoin can use TOR's control protocol for advanced features:

- **Circuit Management**: Create new circuits on demand
- **Stream Isolation**: Separate streams for different connections
- **Hidden Service Control**: Dynamically manage hidden services
- **Event Monitoring**: Monitor TOR events and status

### Multiple Hidden Services

You can run multiple hidden services on different ports:

```ini
# Primary hidden service
hiddensrvdir=/var/lib/intcoin/onion1
hiddensrvport=8333:9333

# Secondary hidden service (testnet)
hiddensrvdir2=/var/lib/intcoin/onion2
hiddensrvport2=18333:18333
```

## Resources

- [TOR Project](https://www.torproject.org/)
- [TOR Hidden Services](https://community.torproject.org/onion-services/)
- [SOCKS5 Protocol](https://datatracker.ietf.org/doc/html/rfc1928)
- [TOR Control Protocol](https://spec.torproject.org/control-spec)

## FAQ

### Q: Is TOR support mandatory?
A: No, TOR support is optional. INTcoin works fine without TOR.

### Q: Can I use both TOR and clearnet simultaneously?
A: Yes, mixed mode is supported and recommended for better connectivity.

### Q: How secure is TOR with INTcoin?
A: TOR provides strong anonymity when configured correctly. Always use onion-only mode for maximum privacy.

### Q: Does TOR affect mining performance?
A: Mining performance is unaffected. Only network communication goes through TOR.

### Q: Can I run a public node with TOR?
A: Yes, you can run a hidden service accessible to all TOR users.

### Q: What's the difference between v2 and v3 onion addresses?
A: V3 addresses are longer (56 chars), more secure, and use modern crypto. V2 is deprecated.

## Support

For TOR-related issues, please:
1. Check the troubleshooting section above
2. Review TOR logs: `sudo journalctl -u tor -f`
3. Review INTcoin logs: `tail -f ~/.intcoin/debug.log`
4. Report issues on GitHub with relevant logs

## License

This documentation is released under the MIT License.
Copyright (c) 2025 INTcoin Core (Maddison Lane)
