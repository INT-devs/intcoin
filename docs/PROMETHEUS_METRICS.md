# Prometheus Metrics Documentation

**Version**: 1.2.0-beta
**Last Updated**: January 2, 2026
**Status**: Production

---

## Overview

INTcoin v1.2.0-beta includes a comprehensive **Prometheus-compatible metrics system** for monitoring node performance, blockchain statistics, and network health. Metrics are exposed via HTTP endpoint for scraping by Prometheus and visualization in Grafana.

### Features

- 📊 **40+ Standard Metrics** - Comprehensive blockchain monitoring
- 🔌 **HTTP Endpoint** - GET /metrics at port 9090
- 🎯 **Prometheus Compatible** - Text exposition format v0.0.4
- 🔒 **Thread-Safe** - Concurrent access support
- ⚡ **Low Overhead** - Minimal performance impact
- 📈 **Grafana Integration** - Dashboard templates included

---

## Quick Start

### Enable Metrics

Edit `intcoin.conf`:
```conf
# Enable metrics HTTP server
metrics.enabled=1

# Bind address (default: 127.0.0.1)
metrics.bind=127.0.0.1

# Port (default: 9090)
metrics.port=9090

# Worker threads (default: 2)
metrics.threads=2
```

Restart node:
```bash
intcoind -daemon
```

### Test Metrics Endpoint

```bash
curl http://localhost:9090/metrics
```

### Configure Prometheus

Add to `prometheus.yml`:
```yaml
scrape_configs:
  - job_name: 'intcoin'
    scrape_interval: 15s
    static_configs:
      - targets: ['localhost:9090']
```

---

## Available Metrics

### Blockchain Metrics

#### blocks_processed_total
**Type**: Counter
**Description**: Total number of blocks processed by node
```
intcoin_blocks_processed_total 150234
```

#### transactions_processed_total
**Type**: Counter
**Description**: Total number of transactions processed
```
intcoin_transactions_processed_total 5234567
```

#### blockchain_height
**Type**: Gauge
**Description**: Current blockchain height
```
intcoin_blockchain_height 150234
```

#### blockchain_difficulty
**Type**: Gauge
**Description**: Current mining difficulty
```
intcoin_blockchain_difficulty 1234567.89
```

#### block_processing_duration
**Type**: Histogram
**Description**: Time to process blocks (seconds)
**Buckets**: 0.1, 0.5, 1, 2, 5, 10, 30, 60
```
intcoin_block_processing_duration_bucket{le="0.5"} 145000
intcoin_block_processing_duration_bucket{le="1.0"} 149500
intcoin_block_processing_duration_sum 75234.5
intcoin_block_processing_duration_count 150234
```

#### block_size
**Type**: Histogram
**Description**: Block size distribution (bytes)
**Buckets**: 1KB, 10KB, 100KB, 500KB, 1MB, 2MB
```
intcoin_block_size_bucket{le="10000"} 50000
intcoin_block_size_sum 5234567890
intcoin_block_size_count 150234
```

### Mempool Metrics

#### mempool_size
**Type**: Gauge
**Description**: Current number of transactions in mempool
```
intcoin_mempool_size 1523
```

#### mempool_bytes
**Type**: Gauge
**Description**: Total mempool size in bytes
```
intcoin_mempool_bytes 456789
```

#### mempool_accepted_total
**Type**: Counter
**Description**: Total transactions accepted to mempool
```
intcoin_mempool_accepted_total 8234567
```

#### mempool_rejected_total
**Type**: Counter
**Description**: Total transactions rejected from mempool
```
intcoin_mempool_rejected_total 12345
```

#### mempool_tx_fee
**Type**: Histogram
**Description**: Transaction fee distribution (satoshis)
**Buckets**: 1000, 5000, 10000, 50000, 100000, 500000, 1000000
```
intcoin_mempool_tx_fee_bucket{le="10000"} 1200
intcoin_mempool_tx_fee_sum 15500000
intcoin_mempool_tx_fee_count 1523
```

### Network Metrics

#### peer_count
**Type**: Gauge
**Description**: Number of connected peers
```
intcoin_peer_count 42
```

#### bytes_sent_total
**Type**: Counter
**Description**: Total bytes sent to network
```
intcoin_bytes_sent_total 1234567890
```

#### bytes_received_total
**Type**: Counter
**Description**: Total bytes received from network
```
intcoin_bytes_received_total 9876543210
```

#### messages_sent_total
**Type**: Counter
**Description**: Total network messages sent
```
intcoin_messages_sent_total 234567
```

#### messages_received_total
**Type**: Counter
**Description**: Total network messages received
```
intcoin_messages_received_total 345678
```

#### message_processing_duration
**Type**: Histogram
**Description**: Message processing time (seconds)
```
intcoin_message_processing_duration_bucket{le="0.001"} 300000
intcoin_message_processing_duration_sum 123.45
intcoin_message_processing_duration_count 345678
```

### Mining Metrics

#### blocks_mined_total
**Type**: Counter
**Description**: Total blocks mined by this node
```
intcoin_blocks_mined_total 42
```

#### hashes_computed_total
**Type**: Counter
**Description**: Total hashes computed
```
intcoin_hashes_computed_total 98765432109876
```

#### hashrate
**Type**: Gauge
**Description**: Current hashrate (hashes/second)
```
intcoin_hashrate 1234567.89
```

#### mining_duration
**Type**: Histogram
**Description**: Time to mine a block (seconds)
```
intcoin_mining_duration_bucket{le="60"} 5
intcoin_mining_duration_bucket{le="300"} 20
intcoin_mining_duration_sum 8765.4
intcoin_mining_duration_count 42
```

### Wallet Metrics

#### wallet_balance
**Type**: Gauge
**Description**: Total wallet balance (satoshis)
```
intcoin_wallet_balance 123456789000
```

#### wallet_transactions_total
**Type**: Counter
**Description**: Total wallet transactions
```
intcoin_wallet_transactions_total 567
```

#### wallet_utxo_count
**Type**: Gauge
**Description**: Number of UTXOs in wallet
```
intcoin_wallet_utxo_count 234
```

### SPV/P2P Metrics

#### spv_best_height
**Type**: Gauge
**Description**: Best known header height (SPV)
```
intcoin_spv_best_height 150250
```

#### bloom_filters_loaded_total
**Type**: Counter
**Description**: Total bloom filters loaded
```
intcoin_bloom_filters_loaded_total 1523
```

#### header_sync_duration
**Type**: Histogram
**Description**: Header synchronization time (seconds)
```
intcoin_header_sync_duration_bucket{le="10"} 1200
intcoin_header_sync_duration_sum 8456.7
intcoin_header_sync_duration_count 1523
```

---

## HTTP API

### Endpoint

**URL**: `http://<bind_address>:<port>/metrics`
**Method**: GET
**Format**: Prometheus text exposition format v0.0.4

### Request

```bash
curl http://localhost:9090/metrics
```

### Response

```
# HELP intcoin_blocks_processed_total Total number of blocks processed
# TYPE intcoin_blocks_processed_total counter
intcoin_blocks_processed_total 150234.00

# HELP intcoin_blockchain_height Current blockchain height
# TYPE intcoin_blockchain_height gauge
intcoin_blockchain_height 150234.00

# HELP intcoin_mempool_size Current mempool transaction count
# TYPE intcoin_mempool_size gauge
intcoin_mempool_size 1523.00

...
```

### Error Responses

**404 Not Found**:
```
GET /invalid-path
HTTP/1.1 404 Not Found
Content-Type: text/plain
```

**405 Method Not Allowed**:
```
POST /metrics
HTTP/1.1 405 Method Not Allowed
Content-Type: text/plain
```

---

## Prometheus Configuration

### Basic Setup

`prometheus.yml`:
```yaml
global:
  scrape_interval: 15s
  evaluation_interval: 15s

scrape_configs:
  - job_name: 'intcoin-mainnet'
    static_configs:
      - targets: ['localhost:9090']
        labels:
          network: 'mainnet'
          instance: 'node-1'
```

### Multiple Nodes

```yaml
scrape_configs:
  - job_name: 'intcoin-cluster'
    static_configs:
      - targets:
        - 'node1.example.com:9090'
        - 'node2.example.com:9090'
        - 'node3.example.com:9090'
```

### Service Discovery

```yaml
scrape_configs:
  - job_name: 'intcoin-auto'
    consul_sd_configs:
      - server: 'localhost:8500'
        services: ['intcoin']
```

---

## Grafana Dashboards

### Import Dashboard

1. Open Grafana → Dashboards → Import
2. Upload JSON: `grafana/intcoin-dashboard.json`
3. Select Prometheus data source
4. Click Import

### Dashboard Panels

**Blockchain Overview**:
- Block height (time series)
- Blocks processed rate
- Transaction rate
- Block processing time (histogram)

**Mempool**:
- Mempool size over time
- Transaction fee distribution
- Accept/reject rates
- Priority breakdown

**Network**:
- Peer count
- Bandwidth usage (sent/received)
- Message processing latency
- Geographic peer distribution

**Mining** (if mining):
- Hashrate
- Blocks mined
- Mining duration
- Accepted vs stale blocks

**Alerts** (example):
- Blockchain height not increasing (stuck sync)
- High mempool size (congestion)
- Low peer count (network issues)
- High block processing time (performance issue)

---

## Alert Rules

### Prometheus Alert Rules

`alerts.yml`:
```yaml
groups:
  - name: intcoin
    interval: 30s
    rules:
      # Blockchain stuck
      - alert: BlockchainStuck
        expr: rate(intcoin_blocks_processed_total[5m]) == 0
        for: 10m
        labels:
          severity: critical
        annotations:
          summary: "Blockchain not processing blocks"
          description: "No blocks processed in 10 minutes"

      # Mempool congestion
      - alert: MempoolCongestion
        expr: intcoin_mempool_size > 10000
        for: 15m
        labels:
          severity: warning
        annotations:
          summary: "Mempool highly congested"
          description: "Mempool has {{ $value }} transactions"

      # Low peer count
      - alert: LowPeerCount
        expr: intcoin_peer_count < 3
        for: 5m
        labels:
          severity: warning
        annotations:
          summary: "Low peer count"
          description: "Only {{ $value }} peers connected"

      # High block processing time
      - alert: SlowBlockProcessing
        expr: rate(intcoin_block_processing_duration_sum[5m]) / rate(intcoin_block_processing_duration_count[5m]) > 5
        for: 15m
        labels:
          severity: warning
        annotations:
          summary: "Slow block processing"
          description: "Average block processing time: {{ $value }}s"
```

### Alertmanager Configuration

```yaml
route:
  receiver: 'intcoin-team'
  group_by: ['alertname', 'instance']
  group_wait: 30s
  group_interval: 5m
  repeat_interval: 4h

receivers:
  - name: 'intcoin-team'
    email_configs:
      - to: 'ops@example.com'
    slack_configs:
      - api_url: 'https://hooks.slack.com/services/XXX'
        channel: '#intcoin-alerts'
```

---

## Security Considerations

### Bind to Localhost

**Default configuration** (secure):
```conf
metrics.bind=127.0.0.1  # Localhost only
```

**Public exposure** (NOT recommended):
```conf
metrics.bind=0.0.0.0  # Accessible from anywhere
```

### Use Reverse Proxy with Authentication

Nginx example:
```nginx
server {
    listen 443 ssl;
    server_name metrics.example.com;

    ssl_certificate /path/to/cert.pem;
    ssl_certificate_key /path/to/key.pem;

    location /metrics {
        auth_basic "Metrics Access";
        auth_basic_user_file /etc/nginx/.htpasswd;
        proxy_pass http://127.0.0.1:9090/metrics;
    }
}
```

Create password file:
```bash
htpasswd -c /etc/nginx/.htpasswd prometheus
```

### Firewall Rules

**Allow only Prometheus server**:
```bash
# UFW
sudo ufw allow from <prometheus_ip> to any port 9090

# iptables
sudo iptables -A INPUT -p tcp -s <prometheus_ip> --dport 9090 -j ACCEPT
sudo iptables -A INPUT -p tcp --dport 9090 -j DROP
```

### TLS/SSL

Use reverse proxy (Nginx, Apache, Caddy) for TLS termination.

### Information Disclosure

Metrics reveal:
- Node uptime
- Peer count and IPs
- Mempool contents
- Block processing times

**Mitigation**: Limit access to trusted monitoring infrastructure.

---

## Performance Impact

### Resource Usage

**Memory**: ~1-5 MB for metrics storage
**CPU**: <0.1% additional load
**Network**: ~10-50 KB per scrape (15s intervals = 2-20 MB/day)

### Optimization

**Reduce scrape frequency**:
```yaml
scrape_interval: 60s  # Instead of 15s
```

**Disable if not needed**:
```conf
metrics.enabled=0
```

**Reduce worker threads** (if low traffic):
```conf
metrics.threads=1
```

---

## Developer Guide

### Adding Custom Metrics

Example in C++:
```cpp
#include "intcoin/metrics.h"

// Register custom counter
static Counter& my_counter =
    MetricsRegistry::Instance().RegisterCounter(
        "my_custom_total",
        "Description of my metric"
    );

// Increment counter
my_counter.Inc();

// Add value
my_counter.Add(5.0);

// Custom gauge
static Gauge& my_gauge =
    MetricsRegistry::Instance().RegisterGauge(
        "my_gauge",
        "Current value of something"
    );

my_gauge.Set(42.0);

// Custom histogram
std::vector<double> buckets = {0.1, 1.0, 10.0, 100.0};
static Histogram& my_histogram =
    MetricsRegistry::Instance().RegisterHistogram(
        "my_duration",
        "Duration of operation",
        buckets
    );

my_histogram.Observe(5.23);

// Timer (RAII pattern)
{
    Timer timer(my_histogram);
    // Code to measure
    // Timer automatically records duration on destruction
}
```

### Metric Naming

Follow Prometheus conventions:
- **Lowercase** with underscores
- **Prefix** with `intcoin_`
- **Suffix** counters with `_total`
- **Unit suffix**: `_seconds`, `_bytes`, etc.
- **Base units**: seconds (not milliseconds), bytes (not KB)

Examples:
- ✅ `intcoin_blocks_processed_total`
- ✅ `intcoin_sync_duration_seconds`
- ❌ `INTcoin_BlockCount`
- ❌ `blocks` (no prefix)

---

## Troubleshooting

### Metrics Endpoint Not Responding

**Check**:
```bash
# Is service running?
ps aux | grep intcoind

# Is port open?
netstat -an | grep 9090

# Test locally
curl http://localhost:9090/metrics
```

**Solutions**:
1. Verify `metrics.enabled=1` in config
2. Check firewall rules
3. Verify bind address/port
4. Check logs for errors

### Prometheus Not Scraping

**Check Prometheus**:
```bash
# View targets
http://localhost:9090/targets

# Check logs
journalctl -u prometheus -f
```

**Common issues**:
- Firewall blocking
- Wrong target address in prometheus.yml
- TLS/SSL certificate errors (if using HTTPS)

### Metrics Stale or Not Updating

**Check**:
- Node is syncing (`intcoin-cli getblockchaininfo`)
- Metrics endpoint returns fresh data
- Prometheus scrape interval

**Restart**:
```bash
systemctl restart intcoind
systemctl restart prometheus
```

---

## Resources

- **Source Code**: [src/metrics/](../src/metrics/)
- **Tests**: [tests/test_metrics.cpp](../tests/test_metrics.cpp) (10/10 ✅)
- **Tests**: [tests/test_metrics_server.cpp](../tests/test_metrics_server.cpp) (8/8 ✅)
- **Prometheus**: https://prometheus.io
- **Grafana**: https://grafana.com

---

**Maintained by**: INTcoin Core Development Team
**Last Updated**: January 2, 2026
**Version**: 1.2.0-beta
