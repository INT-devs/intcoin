# INTcoin TOR Quick Start Guide

Get up and running with TOR support in 5 minutes.

## Step 1: Install TOR

### Linux
```bash
sudo apt-get install tor
sudo systemctl start tor
sudo systemctl enable tor
```

### macOS
```bash
brew install tor
brew services start tor
```

### FreeBSD
```bash
sudo pkg install tor
sudo service tor start
```

## Step 2: Verify TOR is Running

```bash
# Check TOR status
systemctl status tor

# Or test SOCKS proxy
curl --socks5 localhost:9050 https://check.torproject.org/api/ip
```

## Step 3: Configure INTcoin for TOR

Create or edit `~/.intcoin/intcoin.conf`:

```ini
# Enable TOR
tor=1

# TOR proxy (default)
torproxy=127.0.0.1:9050

# Enable hidden service
hiddenservice=1
hiddensrvdir=/var/lib/intcoin/onion_service
hiddensrvport=8333:8333
```

## Step 4: Start INTcoin

```bash
./intcoind -daemon
```

## Step 5: Verify TOR Connection

```bash
# Check network info
./intcoin-cli getnetworkinfo

# Get your .onion address
./intcoin-cli gethiddenserviceaddress

# View connected peers
./intcoin-cli getpeerinfo | grep onion
```

## That's It!

Your INTcoin node is now running over TOR with anonymous networking.

## Optional: Onion-Only Mode

For maximum privacy, run in onion-only mode:

```ini
# Only connect via TOR
onlynet=onion
nodnsseed=1
bind=127.0.0.1
```

## Need Help?

- Full documentation: [TOR-GUIDE.md](TOR-GUIDE.md)
- Check logs: `tail -f ~/.intcoin/debug.log`
- TOR logs: `sudo journalctl -u tor -f`

## Security Tips

1. ✅ Change default RPC password in config
2. ✅ Keep TOR updated: `sudo apt-get update && sudo apt-get upgrade tor`
3. ✅ Backup your `.onion` private keys from hidden service directory
4. ✅ Use a firewall if running onion-only mode
5. ✅ Monitor connections regularly

## Common Issues

**TOR connection failed:**
```bash
# Restart TOR
sudo systemctl restart tor

# Check if port 9050 is open
netstat -an | grep 9050
```

**No onion peers:**
```bash
# Add seed nodes manually
./intcoin-cli addnode "seed.intcoin.onion:8333" add
```

**Hidden service not working:**
```bash
# Check permissions
sudo chown -R tor:tor /var/lib/tor/intcoin/
sudo chmod 700 /var/lib/tor/intcoin/
```

---

For advanced configuration and troubleshooting, see [TOR-GUIDE.md](TOR-GUIDE.md)
