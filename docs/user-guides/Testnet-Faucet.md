# INTcoin Testnet Faucet Guide

Complete guide to running and using the INTcoin testnet faucet server.

## Table of Contents

- [What is a Faucet?](#what-is-a-faucet)
- [Using the Public Faucet](#using-the-public-faucet)
- [Running Your Own Faucet](#running-your-own-faucet)
- [Configuration](#configuration)
- [API Reference](#api-reference)
- [Troubleshooting](#troubleshooting)

---

## What is a Faucet?

A cryptocurrency faucet is a service that distributes small amounts of testnet coins for free. Faucets are essential for:

- **Testing**: Developers need testnet coins to test their applications
- **Learning**: New users can experiment without risking real value
- **Development**: Testing transactions, mining, and wallet functionality

### INTcoin Faucet Features

- **Free testnet INT**: Get 10 INT per request (configurable)
- **Rate Limiting**: Prevents abuse with IP and address cooldowns
- **Web Interface**: Easy-to-use HTML form
- **REST API**: Programmatic access for automation
- **Real-time Stats**: Monitor distributions and balance
- **Queue System**: Automatic processing of requests

---

## Using the Public Faucet

### Web Interface

1. **Get Your Address**
   ```bash
   # Using CLI
   intcoin-cli --testnet getnewaddress

   # Using Qt GUI
   # Open intcoin-qt → Receive tab → Generate address
   ```

2. **Visit Faucet**
   - Open: http://testnet-faucet.intcoin.org:2215
   - Paste your address in the form
   - Click "Request Testnet Coins"

3. **Wait for Confirmation**
   - Request is queued
   - Processing takes ~1-5 minutes
   - Check your wallet for incoming coins

### Rate Limits

- **Per IP Address**: 1 request per hour
- **Per INT Address**: 1 request per 24 hours

### Amount

- **Default**: 10 INT per request
- Check faucet stats for current drip amount

---

## Running Your Own Faucet

### Prerequisites

- INTcoin daemon running on testnet
- Wallet with testnet INT balance
- Open port 2215 (or custom port)
- Recommended: 100+ INT in faucet wallet

### Installation

```bash
# Build with faucet support
cd intcoin/build
cmake .. -DBUILD_FAUCET=ON
make intcoin-faucet
sudo make install
```

### Initial Setup

**1. Create Data Directory**

```bash
mkdir -p ~/intcoin-faucet/blockchain
mkdir -p ~/intcoin-faucet/faucet_wallet
```

**2. Start Testnet Daemon**

```bash
# Start intcoind on testnet
intcoind --testnet --datadir=~/intcoin-faucet/blockchain

# Wait for sync to complete
intcoin-cli --testnet getblockchaininfo
```

**3. Fund Faucet Wallet**

The faucet will create a wallet automatically on first run, or you can prepare one:

```bash
# Option 1: Let faucet create wallet (recommended)
# Start faucet and note the receiving address
intcoin-faucet --datadir=~/intcoin-faucet

# Option 2: Create wallet manually
intcoin-cli --testnet createwallet "faucet"
intcoin-cli --testnet getnewaddress
# Send testnet INT to this address
```

### Starting the Faucet

**Basic Start**

```bash
intcoin-faucet --datadir=~/intcoin-faucet
```

**With Custom Options**

```bash
intcoin-faucet \
  --datadir=~/intcoin-faucet \
  --port=2215 \
  --drip=10 \
  --ip-cooldown=3600 \
  --addr-cooldown=86400 \
  --bind=0.0.0.0 \
  --fee=1000
```

**Output**

```
========================================
INTcoin Testnet Faucet Server
Version: 1.0.0-alpha
========================================

Initializing blockchain...
Initializing faucet wallet...
Faucet wallet balance: 150.00 INT

Starting faucet server...
  HTTP Port: 2215
  Drip Amount: 10.0 INT
  IP Cooldown: 3600 seconds
  Address Cooldown: 86400 seconds
  Bind Address: 0.0.0.0

Faucet server running!
Web interface: http://0.0.0.0:2215/
Press Ctrl+C to stop
```

### Running as Service

**systemd (Linux)**

Create `/etc/systemd/system/intcoin-faucet.service`:

```ini
[Unit]
Description=INTcoin Testnet Faucet
After=network.target

[Service]
Type=simple
User=intcoin
ExecStart=/usr/local/bin/intcoin-faucet \
  --datadir=/var/lib/intcoin-faucet \
  --port=2215 \
  --drip=10
Restart=on-failure
RestartSec=10

[Install]
WantedBy=multi-user.target
```

Enable and start:

```bash
sudo systemctl daemon-reload
sudo systemctl enable intcoin-faucet
sudo systemctl start intcoin-faucet
sudo systemctl status intcoin-faucet
```

**rc.d (FreeBSD)**

Create `/usr/local/etc/rc.d/intcoin_faucet`:

```bash
#!/bin/sh
# PROVIDE: intcoin_faucet
# REQUIRE: DAEMON
# KEYWORD: shutdown

. /etc/rc.subr

name="intcoin_faucet"
rcvar=intcoin_faucet_enable

command="/usr/local/bin/intcoin-faucet"
command_args="--datadir=/var/db/intcoin-faucet --port=2215"

load_rc_config $name
run_rc_command "$1"
```

Enable and start:

```bash
chmod +x /usr/local/etc/rc.d/intcoin_faucet
sudo sysrc intcoin_faucet_enable="YES"
sudo service intcoin_faucet start
```

---

## Configuration

### Command-line Options

| Option | Description | Default |
|--------|-------------|---------|
| `--datadir=<dir>` | Data directory path | `./data` |
| `--port=<port>` | HTTP server port | `2215` |
| `--drip=<amount>` | Amount per request (INT) | `10` |
| `--ip-cooldown=<s>` | IP cooldown in seconds | `3600` (1 hour) |
| `--addr-cooldown=<s>` | Address cooldown in seconds | `86400` (24 hours) |
| `--bind=<addr>` | Bind address | `0.0.0.0` |
| `--fee=<amount>` | Transaction fee (satoshis) | `1000` |
| `--help` | Show help message | - |
| `--version` | Show version | - |

### Examples

**High-traffic Faucet**

```bash
intcoin-faucet \
  --drip=5 \
  --ip-cooldown=7200 \
  --addr-cooldown=172800
```

**Generous Faucet**

```bash
intcoin-faucet \
  --drip=50 \
  --ip-cooldown=1800 \
  --addr-cooldown=43200
```

**Development Faucet**

```bash
intcoin-faucet \
  --drip=100 \
  --ip-cooldown=60 \
  --addr-cooldown=300 \
  --port=3000
```

---

## API Reference

### Web Interface

**GET /**

Returns HTML page with request form and statistics.

```bash
curl http://localhost:2215/
```

### REST API

**POST /request**

Submit a distribution request.

**Request**:
```bash
curl -X POST http://localhost:2215/request \
  -H "Content-Type: application/x-www-form-urlencoded" \
  -d "address=int1qyouraddresshere"
```

**Response** (Success):
```json
{
  "status": "success",
  "message": "Request queued successfully"
}
```

**Response** (Rate Limited):
```json
{
  "status": "error",
  "message": "IP rate limited. Try again in 3456 seconds"
}
```

**Response** (Invalid Address):
```json
{
  "status": "error",
  "message": "Invalid address"
}
```

**GET /stats**

Get faucet statistics.

```bash
curl http://localhost:2215/stats
```

**Response**:
```json
{
  "total_distributions": 1234,
  "total_amount": 12340000000000,
  "pending_requests": 5,
  "failed_requests": 12,
  "rate_limited": 45,
  "faucet_balance": 15000000000000,
  "uptime": 86400
}
```

### Integration Example

**Python**

```python
import requests

def request_testnet_coins(address):
    url = "http://localhost:2215/request"
    data = {"address": address}

    response = requests.post(url, data=data)
    result = response.json()

    if result["status"] == "success":
        print(f"Success: {result['message']}")
    else:
        print(f"Error: {result['message']}")

# Usage
request_testnet_coins("int1qyouraddresshere")
```

**JavaScript**

```javascript
async function requestTestnetCoins(address) {
  const response = await fetch('http://localhost:2215/request', {
    method: 'POST',
    headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
    body: `address=${encodeURIComponent(address)}`
  });

  const result = await response.json();

  if (result.status === 'success') {
    console.log('Success:', result.message);
  } else {
    console.error('Error:', result.message);
  }
}

// Usage
requestTestnetCoins('int1qyouraddresshere');
```

**Bash**

```bash
#!/bin/bash
ADDRESS="int1qyouraddresshere"
FAUCET_URL="http://localhost:2215/request"

response=$(curl -s -X POST "$FAUCET_URL" \
  -H "Content-Type: application/x-www-form-urlencoded" \
  -d "address=$ADDRESS")

echo $response | jq .
```

---

## Monitoring

### Real-time Statistics

The faucet prints statistics every 60 seconds:

```
[1670252400] Stats:
  Total Distributions: 1234
  Total Amount: 12340.0 INT
  Pending Requests: 5
  Failed Requests: 12
  Rate Limited: 45
  Faucet Balance: 150.00 INT
  Uptime: 86400 seconds
```

### Log File

Logs are printed to stdout. Redirect to file:

```bash
intcoin-faucet --datadir=~/intcoin-faucet > ~/faucet.log 2>&1
```

### Monitoring Scripts

**Check Balance**

```bash
#!/bin/bash
BALANCE=$(curl -s http://localhost:2215/stats | jq -r '.faucet_balance')
BALANCE_INT=$((BALANCE / 100000000))

if [ $BALANCE_INT -lt 50 ]; then
  echo "WARNING: Faucet balance low: $BALANCE_INT INT"
  # Send alert (email, SMS, etc.)
fi
```

**Check Uptime**

```bash
#!/bin/bash
UPTIME=$(curl -s http://localhost:2215/stats | jq -r '.uptime')
UPTIME_HOURS=$((UPTIME / 3600))

echo "Faucet uptime: $UPTIME_HOURS hours"
```

---

## Troubleshooting

### Faucet Won't Start

**Error**: `Failed to bind to port 2215`

```bash
# Check if port is in use
sudo lsof -i :2215

# Use different port
intcoin-faucet --port=3000
```

**Error**: `Failed to open blockchain database`

```bash
# Ensure intcoind is running
intcoind --testnet

# Check data directory
ls ~/intcoin-faucet/blockchain
```

### Zero Balance

**Problem**: Faucet has no balance

```bash
# Get faucet address
# (shown when faucet starts)

# Send testnet INT
intcoin-cli --testnet sendtoaddress <faucet_address> 100
```

### Transactions Not Processing

**Problem**: Requests stuck in queue

1. **Check blockchain sync**
   ```bash
   intcoin-cli --testnet getblockchaininfo
   ```

2. **Check wallet balance**
   ```bash
   # Restart faucet, check balance in output
   ```

3. **Check logs for errors**
   ```bash
   tail -f ~/faucet.log
   ```

### Rate Limit Too Strict

**Problem**: Users complaining about cooldowns

```bash
# Reduce cooldown times
intcoin-faucet \
  --ip-cooldown=1800 \
  --addr-cooldown=21600
```

### High Traffic

**Problem**: Faucet overwhelmed with requests

```bash
# Reduce drip amount
intcoin-faucet \
  --drip=5 \
  --ip-cooldown=7200
```

---

## Security Considerations

### Best Practices

1. **Firewall**: Only expose port 2215
2. **HTTPS**: Use reverse proxy (nginx) with SSL
3. **Rate Limiting**: Keep default cooldowns
4. **Balance Monitoring**: Alert when balance < 50 INT
5. **Log Monitoring**: Watch for abuse patterns
6. **Backup Wallet**: Regular wallet backups
7. **Update Software**: Keep faucet updated

### Reverse Proxy (nginx)

```nginx
server {
    listen 80;
    server_name testnet-faucet.intcoin.org;

    location / {
        proxy_pass http://127.0.0.1:2215;
        proxy_set_header Host $host;
        proxy_set_header X-Real-IP $remote_addr;
        proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
    }
}
```

### CAPTCHA Integration

The faucet has CAPTCHA framework (future enhancement):

```bash
# When implemented:
intcoin-faucet \
  --enable-captcha \
  --captcha-secret=your_secret_key
```

---

## FAQ

**Q: How much does it cost to run a faucet?**

A: Minimal. Server costs + initial testnet INT balance (free from another faucet).

**Q: Can I run multiple faucets?**

A: Yes, use different ports and data directories.

**Q: How do I get initial testnet INT?**

A: Request from public faucet or mine testnet blocks.

**Q: Can I change drip amount while running?**

A: No, restart faucet with new `--drip` value.

**Q: Is this safe for mainnet?**

A: **NO**. Only use for testnet. Mainnet faucets aren't practical.

**Q: How do I stop the faucet?**

A: Press Ctrl+C or send SIGTERM signal.

---

## Next Steps

- [Mining Guide](Mining.md)
- [Running Daemon](Running-Daemon.md)
- [API Reference](../api-reference/RPC-API.md)
- [Developer Guides](../developer-guides/)

---

*Last Updated: December 5, 2025*
