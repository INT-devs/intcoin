# INTcoin Monitoring Tutorial

**Version**: 1.2.0-beta
**Last Updated**: January 2, 2026
**Difficulty**: Intermediate
**Time Required**: 45 minutes

Learn how to set up professional monitoring for your INTcoin node using Prometheus and Grafana.

---

## What You'll Learn

- ✅ Enable Prometheus metrics on your INTcoin node
- ✅ Install and configure Prometheus
- ✅ Install and configure Grafana
- ✅ Import pre-built INTcoin dashboards
- ✅ Set up alerts for node issues

---

## Prerequisites

- INTcoin node running v1.2.0-beta or later
- Ubuntu/Debian server (or similar Linux distribution)
- Basic command line knowledge
- 2 GB free RAM
- 10 GB free disk space

---

## Part 1: Enable INTcoin Metrics

### Step 1: Configure INTcoin

Edit your INTcoin configuration:

```bash
nano ~/.intcoin/intcoin.conf
```

Add these lines:

```ini
# Prometheus Metrics
metrics.enabled=1                  # Enable metrics server
metrics.bind=127.0.0.1            # Bind to localhost (secure)
metrics.port=9090                  # HTTP port for metrics
metrics.threads=2                  # Worker threads
```

### Step 2: Restart INTcoin

```bash
# Stop node
intcoin-cli stop

# Wait for shutdown
sleep 10

# Start node
intcoind -daemon

# Verify metrics server started
tail -f ~/.intcoin/debug.log | grep -i metrics
```

Expected output:
```
2026-01-02 12:00:00 Starting Prometheus metrics server on 127.0.0.1:9090
2026-01-02 12:00:00 Metrics server started successfully
```

### Step 3: Test Metrics Endpoint

```bash
# Fetch metrics
curl http://localhost:9090/metrics

# Should see output like:
# # HELP intcoin_blocks_processed_total Total blocks processed
# # TYPE intcoin_blocks_processed_total counter
# intcoin_blocks_processed_total 150234.00
# ...
```

✅ **Checkpoint**: If you see metrics output, you're ready for Part 2!

---

## Part 2: Install Prometheus

### Step 1: Download Prometheus

```bash
# Download latest version
cd /tmp
wget https://github.com/prometheus/prometheus/releases/download/v2.48.0/prometheus-2.48.0.linux-amd64.tar.gz

# Extract
tar -xzf prometheus-2.48.0.linux-amd64.tar.gz

# Move to /opt
sudo mv prometheus-2.48.0.linux-amd64 /opt/prometheus
```

### Step 2: Create Prometheus User

```bash
# Create prometheus user
sudo useradd --no-create-home --shell /bin/false prometheus

# Create directories
sudo mkdir -p /var/lib/prometheus
sudo mkdir -p /etc/prometheus

# Set ownership
sudo chown prometheus:prometheus /var/lib/prometheus
sudo chown prometheus:prometheus /etc/prometheus
```

### Step 3: Configure Prometheus

Create configuration file:

```bash
sudo nano /etc/prometheus/prometheus.yml
```

Add this configuration:

```yaml
global:
  scrape_interval: 15s              # Scrape targets every 15 seconds
  evaluation_interval: 15s          # Evaluate rules every 15 seconds
  external_labels:
    monitor: 'intcoin-monitor'

# Alertmanager configuration (optional, for Part 5)
alerting:
  alertmanagers:
    - static_configs:
        - targets: ['localhost:9093']

# Load rules (for alerts in Part 5)
rule_files:
  - "alerts.yml"

# Scrape configurations
scrape_configs:
  # INTcoin node metrics
  - job_name: 'intcoin'
    static_configs:
      - targets: ['localhost:9090']
        labels:
          instance: 'mainnet-node-1'
          network: 'mainnet'

  # Prometheus self-monitoring
  - job_name: 'prometheus'
    static_configs:
      - targets: ['localhost:9091']  # Prometheus metrics port
```

Set permissions:

```bash
sudo chown prometheus:prometheus /etc/prometheus/prometheus.yml
```

### Step 4: Create Systemd Service

```bash
sudo nano /etc/systemd/system/prometheus.service
```

Add:

```ini
[Unit]
Description=Prometheus Monitoring System
Documentation=https://prometheus.io/docs/
After=network-online.target

[Service]
User=prometheus
Group=prometheus
Type=simple
ExecStart=/opt/prometheus/prometheus \
  --config.file=/etc/prometheus/prometheus.yml \
  --storage.tsdb.path=/var/lib/prometheus/ \
  --web.console.templates=/opt/prometheus/consoles \
  --web.console.libraries=/opt/prometheus/console_libraries \
  --web.listen-address=0.0.0.0:9091 \
  --storage.tsdb.retention.time=30d
Restart=on-failure
RestartSec=5

[Install]
WantedBy=multi-user.target
```

### Step 5: Start Prometheus

```bash
# Reload systemd
sudo systemctl daemon-reload

# Start Prometheus
sudo systemctl start prometheus

# Enable on boot
sudo systemctl enable prometheus

# Check status
sudo systemctl status prometheus
```

### Step 6: Verify Prometheus

```bash
# Check Prometheus is scraping INTcoin
curl http://localhost:9091/api/v1/targets

# Access web UI
# Open browser: http://your-server-ip:9091
```

In Prometheus UI:
1. Go to Status → Targets
2. You should see `intcoin` job with state "UP"

✅ **Checkpoint**: If INTcoin target is "UP", Prometheus is working!

---

## Part 3: Install Grafana

### Step 1: Install Grafana

```bash
# Add Grafana repository
sudo apt-get install -y software-properties-common
sudo add-apt-repository "deb https://packages.grafana.com/oss/deb stable main"
wget -q -O - https://packages.grafana.com/gpg.key | sudo apt-key add -

# Update and install
sudo apt-get update
sudo apt-get install grafana
```

### Step 2: Start Grafana

```bash
# Start Grafana
sudo systemctl start grafana-server

# Enable on boot
sudo systemctl enable grafana-server

# Check status
sudo systemctl status grafana-server
```

### Step 3: Access Grafana

Open browser: `http://your-server-ip:3000`

**Default credentials**:
- Username: `admin`
- Password: `admin`

**First login**: You'll be prompted to change the password.

✅ **Checkpoint**: If you can log in to Grafana, proceed to Part 4!

---

## Part 4: Configure Grafana

### Step 1: Add Prometheus Data Source

1. Click **⚙️ Configuration** (gear icon) → **Data Sources**
2. Click **Add data source**
3. Select **Prometheus**
4. Configure:
   - **Name**: INTcoin Prometheus
   - **URL**: `http://localhost:9091`
   - **Access**: Server (default)
5. Click **Save & Test**

Expected: ✅ "Data source is working"

### Step 2: Import INTcoin Dashboard

**Method 1: Import from file**

```bash
# Download pre-built dashboard
cd /tmp
wget https://raw.githubusercontent.com/INT-devs/intcoin/main/grafana/intcoin-dashboard.json
```

In Grafana:
1. Click **+** → **Import**
2. Click **Upload JSON file**
3. Select `intcoin-dashboard.json`
4. Select data source: **INTcoin Prometheus**
5. Click **Import**

**Method 2: Manual creation**

Create dashboard manually (see template below).

### Step 3: INTcoin Dashboard Panels

Your dashboard should include these panels:

**Blockchain Metrics**:
- Block Height (time series)
- Blocks Processed Rate (graph)
- Block Processing Duration (histogram)
- Difficulty (graph)

**Mempool Metrics**:
- Mempool Size (gauge + time series)
- Mempool Bytes (gauge)
- Transaction Priority Breakdown (pie chart)
- Accept/Reject Rate (graph)

**Network Metrics**:
- Peer Count (gauge)
- Bytes Sent/Received (graph)
- Message Processing Latency (histogram)

**Mining Metrics** (if mining):
- Hashrate (gauge + graph)
- Blocks Mined (counter)
- Mining Duration (histogram)

### Example Panel Configuration

**Panel: Block Height**

PromQL Query:
```promql
intcoin_blockchain_height
```

Visualization: Stat (showing current value)

**Panel: Mempool Size Over Time**

PromQL Query:
```promql
intcoin_mempool_size
```

Visualization: Time series (graph)

**Panel: Priority Breakdown**

PromQL Queries:
```promql
intcoin_mempool_priority_low
intcoin_mempool_priority_normal
intcoin_mempool_priority_high
intcoin_mempool_priority_htlc
intcoin_mempool_priority_bridge
intcoin_mempool_priority_critical
```

Visualization: Pie chart

---

## Part 5: Set Up Alerts

### Step 1: Create Alert Rules

Create Prometheus alert rules:

```bash
sudo nano /etc/prometheus/alerts.yml
```

Add these rules:

```yaml
groups:
  - name: intcoin_alerts
    interval: 30s
    rules:
      # Blockchain not progressing
      - alert: BlockchainStuck
        expr: rate(intcoin_blocks_processed_total[5m]) == 0
        for: 10m
        labels:
          severity: critical
        annotations:
          summary: "INTcoin blockchain stuck"
          description: "No blocks processed in 10 minutes on {{ $labels.instance }}"

      # Mempool congestion
      - alert: MempoolCongestion
        expr: intcoin_mempool_size > 10000
        for: 15m
        labels:
          severity: warning
        annotations:
          summary: "INTcoin mempool congested"
          description: "Mempool has {{ $value }} transactions on {{ $labels.instance }}"

      # Low peer count
      - alert: LowPeerCount
        expr: intcoin_peer_count < 3
        for: 5m
        labels:
          severity: warning
        annotations:
          summary: "Low peer count"
          description: "Only {{ $value }} peers connected on {{ $labels.instance }}"

      # High block processing time
      - alert: SlowBlockProcessing
        expr: |
          rate(intcoin_block_processing_duration_sum[5m]) /
          rate(intcoin_block_processing_duration_count[5m]) > 5
        for: 15m
        labels:
          severity: warning
        annotations:
          summary: "Slow block processing"
          description: "Average block processing: {{ $value }}s on {{ $labels.instance }}"

      # Node down
      - alert: NodeDown
        expr: up{job="intcoin"} == 0
        for: 2m
        labels:
          severity: critical
        annotations:
          summary: "INTcoin node down"
          description: "Node {{ $labels.instance }} is unreachable"
```

Set permissions:

```bash
sudo chown prometheus:prometheus /etc/prometheus/alerts.yml
```

### Step 2: Reload Prometheus

```bash
# Reload configuration
sudo systemctl reload prometheus

# Check alerts are loaded
curl http://localhost:9091/api/v1/rules
```

### Step 3: Configure Grafana Alerts

In Grafana:

1. Open your INTcoin dashboard
2. Edit any panel (e.g., "Block Height")
3. Go to **Alert** tab
4. Click **Create Alert**
5. Configure conditions:
   - **When**: `avg()` of query `A`
   - **Is Above**: `0` (customize as needed)
   - **For**: `5m`
6. Add notification channel (email, Slack, etc.)
7. Save

---

## Part 6: Advanced Configuration

### Monitor Multiple Nodes

Edit `/etc/prometheus/prometheus.yml`:

```yaml
scrape_configs:
  - job_name: 'intcoin-cluster'
    static_configs:
      - targets:
          - 'node1.example.com:9090'
          - 'node2.example.com:9090'
          - 'node3.example.com:9090'
        labels:
          network: 'mainnet'
```

### Use Service Discovery (Consul)

```yaml
scrape_configs:
  - job_name: 'intcoin-auto'
    consul_sd_configs:
      - server: 'localhost:8500'
        services: ['intcoin']
```

### Secure Metrics Endpoint

**Use Nginx reverse proxy with authentication**:

```bash
sudo apt install nginx apache2-utils

# Create password
sudo htpasswd -c /etc/nginx/.htpasswd prometheus

# Configure Nginx
sudo nano /etc/nginx/sites-available/intcoin-metrics
```

Add:

```nginx
server {
    listen 8443 ssl;
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

Enable and restart:

```bash
sudo ln -s /etc/nginx/sites-available/intcoin-metrics /etc/nginx/sites-enabled/
sudo systemctl restart nginx
```

---

## Troubleshooting

### Prometheus can't reach INTcoin

**Check**:
```bash
# Metrics endpoint responding?
curl http://localhost:9090/metrics

# Firewall blocking?
sudo ufw status

# INTcoin metrics enabled?
grep "metrics.enabled" ~/.intcoin/intcoin.conf
```

### Grafana shows "No Data"

**Check**:
1. Data source configured correctly?
2. Time range includes data? (adjust time picker)
3. Query syntax correct? (test in Prometheus UI first)

### Alerts not firing

**Check**:
```bash
# Alert rules loaded?
curl http://localhost:9091/api/v1/rules

# Alerts triggering?
curl http://localhost:9091/api/v1/alerts
```

---

## Example Queries

### PromQL Queries for INTcoin

**Block height growth rate** (blocks per hour):
```promql
rate(intcoin_blockchain_height[1h]) * 3600
```

**Average mempool size** (24-hour average):
```promql
avg_over_time(intcoin_mempool_size[24h])
```

**Network bandwidth** (bytes per second):
```promql
rate(intcoin_bytes_sent_total[5m]) + rate(intcoin_bytes_received_total[5m])
```

**P99 block processing time**:
```promql
histogram_quantile(0.99, rate(intcoin_block_processing_duration_bucket[5m]))
```

**Transaction throughput** (tx per second):
```promql
rate(intcoin_transactions_processed_total[1m])
```

---

## Next Steps

**Learn More**:
- [Prometheus Metrics Documentation](PROMETHEUS_METRICS.md)
- [Grafana Dashboards](GRAFANA_DASHBOARDS.md)
- [Alert Rules Best Practices](https://prometheus.io/docs/practices/alerting/)

**Advanced Topics**:
- Long-term storage with Thanos or Cortex
- Distributed tracing with Jaeger
- Log aggregation with Loki

---

**Congratulations!** 🎉 You now have professional monitoring for your INTcoin node!

**Last Updated**: January 2, 2026
