# INTcoin Bridge Monitoring Guide

Complete guide to monitoring and managing cross-chain bridges with real-time health checks, alerts, and analytics.

## Table of Contents

- [Overview](#overview)
- [Starting the Monitor](#starting-the-monitor)
- [Health Checks](#health-checks)
- [Performance Metrics](#performance-metrics)
- [Alert System](#alert-system)
- [Anomaly Detection](#anomaly-detection)
- [Analytics and Reporting](#analytics-and-reporting)
- [Dashboard API](#dashboard-api)
- [RPC Commands](#rpc-commands)
- [Best Practices](#best-practices)

## Overview

The Bridge Monitoring system provides:
- **Real-time Health Checks**: Monitor bridge status every 30 seconds
- **Performance Tracking**: Success rates, volumes, timing metrics
- **Alert System**: Multi-level alerts (INFO/WARNING/ERROR/CRITICAL)
- **Anomaly Detection**: Automatic detection of unusual patterns
- **Historical Analytics**: 30-day data retention with export capabilities
- **Dashboard API**: JSON/CSV output for visualization

## Starting the Monitor

### Automatic Start

Bridge monitoring starts automatically when `intcoind` launches with bridges enabled.

```bash
# Start daemon with bridges
intcoind -bridge=1
```

### Manual Control

```bash
# Start monitor
intcoin-cli startmonitor

# Stop monitor
intcoin-cli stopmonitor

# Check if monitor is running
intcoin-cli getmonitorstatus
```

## Health Checks

### Health Status Levels

| Status | Description | Action Required |
|--------|-------------|-----------------|
| **HEALTHY** | All systems operational | None |
| **DEGRADED** | Some issues but functional | Monitor closely |
| **UNHEALTHY** | Significant problems | Investigate immediately |
| **OFFLINE** | Bridge not responding | Critical action needed |

### Checking Bridge Health

**Single Bridge**:
```bash
intcoin-cli checkbridgehealth BITCOIN
```

**All Bridges**:
```bash
intcoin-cli checkallbridges
```

**Sample Output**:
```json
{
  "chain": "BITCOIN",
  "status": "HEALTHY",
  "timestamp": 1700000000,
  "response_time_ms": 45,
  "chain_height": 820000,
  "sync_height": 820000,
  "pending_swaps": 3,
  "status_message": "Bridge online and synced"
}
```

### Overall Health

```bash
intcoin-cli getoverallhealth
```

Returns: `HEALTHY`, `DEGRADED`, `UNHEALTHY`, or `OFFLINE`

## Performance Metrics

### Available Metrics

For each bridge chain:

**Timing Metrics**:
- Average swap time (seconds)
- Average confirmation time (seconds)
- Average sync time (seconds)

**Success Metrics**:
- Success rate (24 hours)
- Success rate (7 days)
- Total swaps (24 hours)
- Failed swaps (24 hours)

**Volume Metrics**:
- Volume (24 hours, 7 days, 30 days)
- Blocks behind
- Is syncing (boolean)

### Getting Metrics

**Single Chain**:
```bash
intcoin-cli getbridgemetrics ETHEREUM
```

**Sample Output**:
```json
{
  "chain": "ETHEREUM",
  "timestamp": 1700000000,
  "avg_swap_time_sec": 300,
  "avg_confirmation_time_sec": 180,
  "avg_sync_time_sec": 60,
  "success_rate_24h": 0.9850,
  "success_rate_7d": 0.9820,
  "total_swaps_24h": 142,
  "failed_swaps_24h": 2,
  "volume_24h": 5000000000000000000,
  "volume_7d": 35000000000000000000,
  "volume_30d": 150000000000000000000,
  "blocks_behind": 0,
  "is_syncing": false
}
```

**All Chains**:
```bash
intcoin-cli getallmetrics
```

## Alert System

### Alert Severity Levels

1. **INFO**: Informational messages
2. **WARNING**: Potential issues
3. **ERROR**: Definite problems
4. **CRITICAL**: Immediate action required

### Alert Types

| Type | Description | Severity |
|------|-------------|----------|
| BRIDGE_DOWN | Bridge is offline | CRITICAL |
| CHAIN_SYNC_FAILED | Cannot sync with chain | ERROR |
| SWAP_TIMEOUT | Swap took too long | WARNING |
| HIGH_FAILURE_RATE | >10% swaps failing | WARNING |
| LOW_LIQUIDITY | Insufficient funds | WARNING |
| PROOF_VERIFICATION_FAILED | Invalid proof detected | ERROR |
| UNUSUAL_VOLUME | Abnormal transaction volume | WARNING |
| STUCK_TRANSACTION | Transaction not confirming | WARNING |

### Viewing Alerts

**Active Alerts**:
```bash
intcoin-cli getactivealerts
```

**Critical Alerts Only**:
```bash
intcoin-cli getalertsbyseverity CRITICAL
```

**Sample Alert**:
```json
{
  "alert_id": "abc123...",
  "type": "BRIDGE_DOWN",
  "severity": "CRITICAL",
  "chain": "BITCOIN",
  "message": "Bridge is offline",
  "timestamp": 1700000000,
  "acknowledged": false,
  "details": "Failed to connect to Bitcoin RPC"
}
```

### Managing Alerts

**Acknowledge Alert**:
```bash
intcoin-cli acknowledgealert <alert_id>
```

**Clear Acknowledged Alerts**:
```bash
intcoin-cli clearacknowledgedalerts
```

### Alert Callbacks

Register custom alert handlers in C++:

```cpp
#include "include/intcoin/bridge/monitor.h"

void my_alert_handler(const bridge::MonitorAlert& alert) {
    if (alert.severity == bridge::AlertSeverity::CRITICAL) {
        // Send notification (email, SMS, etc.)
        send_notification(alert.message);
    }
}

// Register callback
monitor->register_alert_callback(my_alert_handler);
```

## Anomaly Detection

### Detection Algorithms

**Volume Anomalies**:
- Detects when current volume > 10x average
- Detects when volume drops to near zero unexpectedly

**Failure Rate Anomalies**:
- Detects when failure rate > 10%
- Severity score based on failure percentage

### Checking for Anomalies

**Single Chain**:
```bash
intcoin-cli detectanomalies LITECOIN
```

**Recent Anomalies (Last Hour)**:
```bash
intcoin-cli getrecentanomalies 3600
```

**Sample Anomaly**:
```json
{
  "anomaly_id": "def456...",
  "chain": "ETHEREUM",
  "type": "HIGH_FAILURE_RATE",
  "severity_score": 85.5,
  "description": "Unusually high swap failure rate detected",
  "timestamp": 1700000000,
  "metadata": {
    "failure_rate": "0.15",
    "threshold": "0.10"
  }
}
```

## Analytics and Reporting

### Recording Data

The monitor automatically records:
- All swap attempts (success/failure, amount, duration)
- Sync operations (blocks synced, duration)
- Health check results

### Generating Reports

**Swap Report (24 hours)**:
```bash
intcoin-cli generateswapreport BITCOIN 86400
```

**Sample Report**:
```json
{
  "chain": "BITCOIN",
  "period_start": 1699913600,
  "period_end": 1700000000,
  "total_swaps": 156,
  "successful_swaps": 153,
  "failed_swaps": 3,
  "success_rate": 0.9808,
  "total_volume": 1500000000,
  "avg_duration_sec": 320
}
```

**All Chains Report**:
```bash
intcoin-cli generateallswapreports 86400
```

**Sync Report**:
```bash
intcoin-cli generatesyncreport CARDANO 604800  # 7 days
```

### Export Options

**JSON Export**:
```bash
intcoin-cli exportanalyticsj son 86400 > bridge_analytics.json
```

**CSV Export**:
```bash
intcoin-cli exportanalyticscsv 86400 > bridge_analytics.csv
```

**CSV Format**:
```csv
Chain,Total Swaps,Successful,Failed,Success Rate,Volume,Avg Duration
Bitcoin,156,153,3,98.08%,1500000000,320
Ethereum,203,201,2,99.01%,2500000000000000000,180
```

## Dashboard API

### Getting Dashboard Data

```bash
intcoin-cli getdashboard
```

**Sample Output**:
```json
{
  "overall_health": "HEALTHY",
  "bridges_online": 5,
  "bridges_total": 5,
  "swaps_1h": 18,
  "swaps_24h": 432,
  "volume_24h_usd": 1250000,
  "active_alerts": 2,
  "critical_alerts": 0,
  "avg_success_rate": 98.5,
  "avg_swap_time_sec": 285,
  "chain_metrics": {
    "BITCOIN": { /* metrics */ },
    "ETHEREUM": { /* metrics */ },
    "LITECOIN": { /* metrics */ },
    "CARDANO": { /* metrics */ },
    "MONERO": { /* metrics */ }
  }
}
```

### Dashboard JSON

For web dashboards:

```bash
intcoin-cli getdashboardjson | jq .
```

## RPC Commands

### Monitor Control

```bash
# Start/stop monitor
intcoin-cli startmonitor
intcoin-cli stopmonitor
intcoin-cli getmonitorstatus

# Get monitor statistics
intcoin-cli getmonitorstats
```

**Monitor Stats Output**:
```json
{
  "total_alerts_24h": 15,
  "critical_alerts_24h": 1,
  "total_health_checks": 2880,
  "failed_health_checks": 12,
  "avg_response_time_ms": 52.3,
  "total_anomalies_detected": 3
}
```

### Health Commands

```bash
intcoin-cli checkbridgehealth <chain>
intcoin-cli checkallbridges
intcoin-cli getoverallhealth
```

### Metrics Commands

```bash
intcoin-cli getbridgemetrics <chain>
intcoin-cli getallmetrics
```

### Alert Commands

```bash
intcoin-cli getactivealerts
intcoin-cli getalertsbyseverity <severity>
intcoin-cli acknowledgealert <alert_id>
intcoin-cli clearacknowledgedalerts
```

### Anomaly Commands

```bash
intcoin-cli detectanomalies <chain>
intcoin-cli getrecentanomalies <max_age_seconds>
```

### Analytics Commands

```bash
intcoin-cli generateswapreport <chain> <period_seconds>
intcoin-cli generateallswapreports <period_seconds>
intcoin-cli generatesyncreport <chain> <period_seconds>
intcoin-cli exportanalyticsjson <period_seconds>
intcoin-cli exportanalyticscsv <period_seconds>
```

### Dashboard Commands

```bash
intcoin-cli getdashboard
intcoin-cli getdashboardjson
```

## Best Practices

### Monitoring Setup

1. **Automated Monitoring**: Let the monitor run continuously (auto-starts with daemon)
2. **Alert Handling**: Set up custom alert callbacks for critical alerts
3. **Regular Review**: Check dashboard daily for trends
4. **Response Time**: Respond to CRITICAL alerts within minutes

### Alert Management

1. **Acknowledge Alerts**: Mark alerts as acknowledged after addressing
2. **Clean Up**: Regularly clear old acknowledged alerts
3. **Escalation**: Critical alerts should trigger immediate investigation
4. **False Positives**: Tune thresholds if getting too many false alerts

### Performance Optimization

1. **Monitor Resource Usage**: Bridge monitoring is lightweight but check CPU/memory
2. **Data Retention**: 30-day retention is default, exports allow archival
3. **Network**: Ensure stable RPC connections to all chain nodes
4. **Redundancy**: Consider backup RPC endpoints for critical chains

### Anomaly Response

1. **High Failure Rate**:
   - Check chain node connectivity
   - Verify sufficient gas/fees
   - Check for chain congestion

2. **Unusual Volume**:
   - Investigate large swaps
   - Check for potential attacks
   - Verify transaction legitimacy

3. **Stuck Transactions**:
   - Check transaction status on chain
   - Consider rebroadcasting
   - Verify fee adequacy

### Data Analysis

1. **Export Regularly**: Weekly exports for long-term analysis
2. **Trend Analysis**: Look for patterns in success rates and volumes
3. **Capacity Planning**: Use volume trends to plan infrastructure
4. **Optimization**: Identify bottlenecks from timing metrics

## Troubleshooting

### Monitor Not Starting

**Symptoms**: `startmonitor` returns false or error

**Solutions**:
- Ensure bridges are enabled: `intcoind -bridge=1`
- Check bridge manager is initialized
- Verify no other monitor instance running

### No Alerts Generated

**Symptoms**: Known issues but no alerts

**Solutions**:
- Check monitor is running: `getmonitorstatus`
- Verify alert thresholds are configured
- Review monitor logs for errors

### High Alert Volume

**Symptoms**: Too many alerts, alert fatigue

**Solutions**:
- Tune severity thresholds
- Filter by severity (focus on ERROR/CRITICAL)
- Batch acknowledge INFO/WARNING alerts
- Improve infrastructure to reduce issues

### Missing Metrics

**Symptoms**: Some metrics show 0 or missing

**Solutions**:
- Ensure bridge has processed transactions
- Check data retention (30 days)
- Verify analytics recording is enabled

## Integration Examples

### Prometheus Integration

Export metrics for Prometheus:

```bash
#!/bin/bash
# prometheus_exporter.sh

while true; do
    intcoin-cli getallmetrics | \
    jq -r '
to_entries[] | 
"bridge_success_rate{chain=\"\(.key)\"} \(.value.success_rate_24h * 100)\n" +
"bridge_volume_24h{chain=\"\(.key)\"} \(.value.volume_24h)\n" +
"bridge_swaps_24h{chain=\"\(.key)\"} \(.value.total_swaps_24h)"
' > /var/lib/prometheus/bridge_metrics.prom
    
    sleep 60
done
```

### Grafana Dashboard

Sample Grafana queries:

```
# Success Rate
bridge_success_rate

# Volume Over Time
rate(bridge_volume_24h[1h])

# Alert Count
count(bridge_alerts{severity="CRITICAL"})
```

### Email Alerts

Python script for email alerts:

```python
import subprocess
import smtplib
from email.mime.text import MIMEText

def check_critical_alerts():
    result = subprocess.run(
        ['intcoin-cli', 'getalertsbyseverity', 'CRITICAL'],
        capture_output=True, text=True
    )
    
    alerts = json.loads(result.stdout)
    
    if alerts:
        send_email(f"CRITICAL: {len(alerts)} bridge alerts", 
                   json.dumps(alerts, indent=2))

def send_email(subject, body):
    msg = MIMEText(body)
    msg['Subject'] = subject
    msg['From'] = 'bridge-monitor@intcoin.org'
    msg['To'] = 'admin@intcoin.org'
    
    smtp = smtplib.SMTP('localhost')
    smtp.send_message(msg)
    smtp.quit()

if __name__ == '__main__':
    check_critical_alerts()
```

## Resources

- [Bridge Guide](BRIDGE-GUIDE.md)
- [DeFi Guide](DEFI-GUIDE.md)
- [RPC API Reference](RPC-API.md)
- [Bridge Implementation](../src/bridge/README.md)

## Support

For questions or issues:
- GitHub: https://github.com/intcoin/core
- GitLab: https://gitlab.com/intcoin/crypto
- Email: team@international-coin.org

---

**Lead Developer**: Maddison Lane  
**Last Updated**: November 2025
