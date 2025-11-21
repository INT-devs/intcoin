# INTcoin Seed Node Infrastructure

**Version**: 1.1.0
**Target Launch**: January 1, 2026
**Last Updated**: November 13, 2025

## Overview

Seed nodes are the initial entry points for new nodes joining the INTcoin network. They provide peer discovery services and help bootstrap the peer-to-peer network. This document outlines the seed node infrastructure for mainnet and testnet.

## Architecture

### Seed Node Types

1. **DNS Seed Nodes**: Domain-based seed servers that return IP addresses of active nodes
2. **Hardcoded Seed Nodes**: IP addresses embedded in the client software as fallback
3. **Tor Hidden Service Seeds**: .onion addresses for privacy-focused connections

### Geographic Distribution

Seed nodes will be strategically distributed across multiple continents:

- **North America**: 2 seed nodes (US East, US West)
- **Europe**: 2 seed nodes (London, Frankfurt)
- **Asia**: 2 seed nodes (Singapore, Tokyo)
- **Australia**: 1 seed node (Sydney)
- **South America**: 1 seed node (São Paulo)

**Total**: 8 seed nodes for global coverage and redundancy

## Mainnet Seed Nodes

### DNS Seeds

```
seed.intcoin.org
seed-eu.intcoin.org
seed-asia.intcoin.org
seed-au.intcoin.org
```

### Hardcoded Seeds (Planned)

```cpp
// Mainnet seed nodes (to be deployed)
static const char* pszMainnetSeeds[] = {
    "seed1.intcoin.org:9333",
    "seed2.intcoin.org:9333",
    "seed-eu1.intcoin.org:9333",
    "seed-eu2.intcoin.org:9333",
    "seed-asia1.intcoin.org:9333",
    "seed-asia2.intcoin.org:9333",
    "seed-au.intcoin.org:9333",
    "seed-sa.intcoin.org:9333",
};
```

### Tor Seeds (Planned)

```
intcoin[random].onion:9333
```

## Testnet Seed Nodes

### DNS Seeds

```
testnet-seed.intcoin.org
```

### Hardcoded Seeds

```cpp
// Testnet seed nodes
static const char* pszTestnetSeeds[] = {
    "testnet-seed1.intcoin.org:19333",
    "testnet-seed2.intcoin.org:19333",
};
```

## Seed Node Requirements

### Hardware Specifications

**Minimum Requirements**:
- CPU: 2 cores (4 cores recommended)
- RAM: 4 GB (8 GB recommended)
- Storage: 100 GB SSD (for blockchain data)
- Network: 100 Mbps symmetric (1 Gbps recommended)
- Uptime: 99.9% target

**Recommended Setup**:
- CPU: 4+ cores (Intel Xeon or AMD EPYC)
- RAM: 16 GB
- Storage: 500 GB NVMe SSD
- Network: 1 Gbps symmetric
- DDoS Protection: Cloudflare or similar
- Monitoring: 24/7 uptime monitoring

### Software Requirements

- **OS**: Ubuntu 24.04 LTS, Debian 12, or FreeBSD 14+
- **intcoind**: Latest stable version (v1.1.0+)
- **Dependencies**: liboqs 0.14+, Boost 1.85+, RocksDB 9.7+
- **Firewall**: UFW or iptables with port 9333/19333 open
- **Monitoring**: Prometheus + Grafana (optional)

## Deployment Process

### 1. Server Provisioning

```bash
# Update system
sudo apt update && sudo apt upgrade -y

# Install dependencies
sudo apt install -y build-essential cmake git \
    libboost-all-dev libssl-dev \
    librocksdb-dev liboqs-dev

# Create intcoin user
sudo useradd -r -m -s /bin/bash intcoin
```

### 2. Build INTcoin

```bash
# Clone repository
git clone https://gitlab.com/intcoin/crypto.git intcoin
cd intcoin

# Build release version
cmake -B build -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_TESTS=OFF \
    -DBUILD_QT=OFF \
    -DBUILD_EXPLORER=OFF
cmake --build build -j$(nproc)

# Install
sudo cmake --install build
```

### 3. Configure Seed Node

Create `/home/intcoin/.intcoin/intcoin.conf`:

```ini
# Network settings
listen=1
maxconnections=250
upnp=0

# Seed node specific
discover=1
dnsseed=1
seednode=1

# Performance
dbcache=2048
maxmempool=500

# Logging
debug=net
logips=1
logtimestamps=1

# RPC (for monitoring)
server=1
rpcuser=intcoin_rpc
rpcpassword=[STRONG_PASSWORD]
rpcallowip=127.0.0.1
rpcbind=127.0.0.1
```

### 4. Create Systemd Service

Create `/etc/systemd/system/intcoind.service`:

```ini
[Unit]
Description=INTcoin Daemon (Seed Node)
After=network.target
Wants=network-online.target

[Service]
Type=forking
User=intcoin
Group=intcoin

ExecStart=/usr/local/bin/intcoind -daemon -conf=/home/intcoin/.intcoin/intcoin.conf
ExecStop=/usr/local/bin/intcoin-cli stop

Restart=always
RestartSec=30
TimeoutStopSec=120

# Hardening
NoNewPrivileges=true
PrivateTmp=true
ProtectSystem=full
ProtectHome=true

[Install]
WantedBy=multi-user.target
```

### 5. Start and Enable Service

```bash
# Reload systemd
sudo systemctl daemon-reload

# Enable on boot
sudo systemctl enable intcoind

# Start service
sudo systemctl start intcoind

# Check status
sudo systemctl status intcoind
```

## Monitoring

### Health Checks

```bash
# Check peer connections
intcoin-cli getconnectioncount

# Check blockchain sync status
intcoin-cli getblockchaininfo

# Check network info
intcoin-cli getnetworkinfo

# Check peer info
intcoin-cli getpeerinfo | jq '.[0:5]'
```

### Metrics to Monitor

1. **Uptime**: Should be 99.9%+
2. **Peer Count**: Should maintain 100+ connections
3. **Blockchain Height**: Should be at tip (not lagging)
4. **Memory Usage**: Should be stable, not growing
5. **Disk I/O**: Should be reasonable (<80% utilization)
6. **Network Traffic**: Inbound/outbound balance
7. **CPU Usage**: Should be <50% average

### Alerting

Set up alerts for:
- Node goes offline (down for >5 minutes)
- Peer count drops below 50
- Blockchain falls behind (>10 blocks)
- Disk space below 20%
- Memory usage above 90%

## DNS Seed Server Setup

### Requirements

- Separate server from full node
- Python 3.10+ or Go 1.21+
- DNS server (BIND9 or similar)
- Access to intcoin node RPC

### Implementation

Create a DNS seeder that:
1. Connects to local intcoind RPC
2. Queries for active peers
3. Tests peer connectivity and version
4. Returns A records for healthy peers
5. Updates DNS records every 30 seconds

Example Python DNS seeder (simplified):

```python
#!/usr/bin/env python3
import requests
import time
from dnslib.server import DNSServer, DNSHandler, BaseResolver
from dnslib import RR, QTYPE, A

class INTcoinSeedResolver(BaseResolver):
    def __init__(self, rpc_url, rpc_user, rpc_pass):
        self.rpc_url = rpc_url
        self.rpc_auth = (rpc_user, rpc_pass)
        self.seeds = []
        self.update_seeds()

    def update_seeds(self):
        """Query intcoind for healthy peers"""
        try:
            response = requests.post(
                self.rpc_url,
                json={"jsonrpc": "1.0", "method": "getpeerinfo", "params": []},
                auth=self.rpc_auth
            )
            peers = response.json()['result']
            self.seeds = [peer['addr'].split(':')[0] for peer in peers
                         if peer['synced_blocks'] > 0][:25]
        except Exception as e:
            print(f"Error updating seeds: {e}")

    def resolve(self, request, handler):
        reply = request.reply()
        qname = request.q.qname
        qtype = request.q.qtype

        if qtype == QTYPE.A:
            for seed in self.seeds:
                reply.add_answer(RR(qname, QTYPE.A, rdata=A(seed), ttl=60))

        return reply

# Run DNS server
if __name__ == '__main__':
    resolver = INTcoinSeedResolver(
        'http://localhost:9334',
        'intcoin_rpc',
        'strong_password'
    )

    server = DNSServer(resolver, port=53, address='0.0.0.0')
    server.start_thread()

    while True:
        time.sleep(30)
        resolver.update_seeds()
```

## Security Considerations

### DDoS Protection

1. **Rate Limiting**: Limit connections per IP
2. **Firewall Rules**: Use iptables/nftables
3. **CloudFlare**: Proxy DNS queries through Cloudflare
4. **Fail2Ban**: Auto-ban abusive IPs
5. **Connection Limits**: Set maxconnections appropriately

### Access Control

- RPC interface should only bind to localhost
- Use strong RPC passwords (32+ characters)
- SSH key-only authentication
- Disable password authentication
- Enable UFW/firewall
- Regular security updates

### Backup and Recovery

- Daily blockchain backups (optional, can resync)
- Configuration backups
- Monitoring data retention
- Disaster recovery plan
- Automated failover (for DNS seeds)

## Cost Estimates

### Cloud Hosting (Annual)

- **AWS**: t3.large (2 vCPU, 8GB RAM, 100GB SSD) = ~$600/year
- **DigitalOcean**: Droplet ($48/month) = ~$576/year
- **Linode**: Dedicated 8GB = ~$480/year
- **Hetzner**: VPS CX31 = ~$180/year (best value)

### Total Infrastructure Cost

- 8 seed nodes × $300/year (avg) = $2,400/year
- DNS seed servers (2) × $120/year = $240/year
- DDoS protection = $500/year
- Monitoring/alerting = $200/year

**Total Estimated Cost**: ~$3,340/year for complete seed infrastructure

## Launch Checklist

- [ ] Deploy 8 mainnet seed nodes globally
- [ ] Configure DNS seed servers
- [ ] Set up monitoring and alerting
- [ ] Test peer discovery from fresh nodes
- [ ] Configure Tor hidden services
- [ ] Document emergency procedures
- [ ] Train operations team
- [ ] Set up 24/7 on-call rotation
- [ ] Prepare incident response plan
- [ ] Create runbooks for common issues

## Maintenance Schedule

### Daily

- Monitor uptime and peer counts
- Check for alerts
- Review logs for anomalies

### Weekly

- Review performance metrics
- Update security patches
- Check disk space usage
- Verify backup integrity

### Monthly

- Software version updates
- Security audit
- Capacity planning review
- Cost analysis

### Quarterly

- Hardware refresh evaluation
- Geographic distribution review
- Disaster recovery drill
- Performance optimization

## Support and Contact

### Emergency Contacts

- **Operations Lead**: ops@international-coin.org
- **Infrastructure Team**: infra@international-coin.org
- **24/7 Hotline**: [To be established]

### Documentation

- **Node Operation**: docs/NODE_OPERATION.md
- **Troubleshooting**: docs/TROUBLESHOOTING.md
- **API Reference**: docs/RPC_API.md

### Community

- **Discord**: #seed-nodes channel
- **GitHub**: gitlab.com/intcoin/seed-infrastructure
- **Forum**: forum.intcoin.org/category/infrastructure

## References

- **Bitcoin Seed Nodes**: https://bitcoin.org/en/full-node#seed-nodes
- **Ethereum Bootnodes**: https://github.com/ethereum/go-ethereum/wiki/Connecting-to-the-network
- **Network Architecture**: docs/NETWORK_ARCHITECTURE.md
- **Security Best Practices**: docs/SECURITY.md

---

**Lead Developer**: Maddison Lane
**Infrastructure Team**: INTcoin Foundation
**Documentation**: Claude AI (Anthropic)
**Date**: November 13, 2025
