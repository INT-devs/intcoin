# INTcoin Bridge Web UI

Web interface for the INTcoin cross-chain bridge, enabling trustless asset transfers between Bitcoin, Ethereum, Litecoin, and INTcoin.

## Features

- **Deposit to Bridge**: Submit deposit proofs from Bitcoin, Ethereum, or Litecoin
- **Withdraw from Bridge**: Request withdrawals to destination chains
- **Transaction Monitoring**: View deposit/withdrawal history and status
- **Real-time Stats**: Monitor bridge health, validator status, and balances
- **Wrapped Token Support**: wBTC, wETH, wLTC on INTcoin

## Tech Stack

- **React 18** - UI framework
- **TypeScript** - Type safety
- **Vite** - Build tool
- **TanStack Query** - Data fetching and caching
- **Zustand** - State management (if needed)

## Prerequisites

- Node.js >= 18.0.0
- npm >= 9.0.0
- INTcoin node running with RPC enabled (port 12212)

## Installation

```bash
# Install dependencies
npm install

# Start development server
npm run dev

# Build for production
npm run build

# Preview production build
npm run preview
```

## Configuration

The app proxies API requests to the INTcoin RPC server:

- **Development**: `http://localhost:3000` → `http://localhost:12212` (via Vite proxy)
- **Production**: Configure reverse proxy or update `RPC_ENDPOINT` in `src/api/bridge.ts`

## Project Structure

```
bridge-ui/
├── src/
│   ├── api/           # API client (RPC calls)
│   ├── components/    # React components
│   ├── hooks/         # React hooks (TanStack Query)
│   ├── types/         # TypeScript types
│   ├── utils/         # Utility functions
│   ├── App.tsx        # Main app component
│   ├── App.css        # Styles
│   └── main.tsx       # Entry point
├── package.json
├── tsconfig.json
├── vite.config.ts
└── README.md
```

## Available RPC Methods

The UI integrates with the following bridge RPC methods:

### Information
- `getbridgeinfo` - Get bridge configuration and status

### Deposits
- `bridgedeposit` - Submit deposit proof from external chain

### Withdrawals
- `bridgewithdraw` - Request withdrawal to external chain

### Balances
- `getbridgebalance` - Get wrapped token balances

### Transactions
- `listbridgetransactions` - List bridge transaction history

## Usage

### 1. Check Bridge Status

View real-time bridge statistics:
- Bridge status (active/paused)
- Validator count and threshold
- Required confirmations per chain
- Bridge fees

### 2. Deposit to Bridge

To deposit assets from Bitcoin/Ethereum/Litecoin:

1. Send funds to the bridge contract on the source chain
2. Wait for required confirmations (BTC: 6, ETH: 12, LTC: 24)
3. Submit deposit proof via the UI:
   - Transaction hash
   - Block number
   - Depositor address (source chain)
   - Recipient address (INTcoin)
   - Amount (in satoshis/wei)
4. Wait for validator signatures
5. Wrapped tokens will be minted to your INTcoin address

### 3. Withdraw from Bridge

To withdraw wrapped tokens back to the origin chain:

1. Ensure you have wrapped tokens (wBTC/wETH/wLTC)
2. Create and sign a withdrawal request
3. Submit withdrawal via the UI:
   - Destination chain
   - Token symbol
   - Destination address
   - Amount
   - Signature
4. Wait for M-of-N validator signatures
5. Funds will be released on the destination chain

### 4. Monitor Transactions

View your transaction history:
- Filter by type (deposit/withdrawal/all)
- Check status and confirmations
- Track validator signatures

## Security Considerations

⚠️ **Beta Software Warning**

This is beta software. Use at your own risk:

- Always verify transaction details before signing
- Double-check addresses (wrong address = permanent loss)
- Start with small amounts for testing
- Ensure your INTcoin node is fully synced
- Verify validator signatures meet threshold
- Monitor transaction status until completion

### Best Practices

- **Backup Recovery Phrase**: Always backup your wallet recovery phrase
- **Verify Contracts**: Verify bridge contract addresses on each chain
- **Test Amounts**: Use small test amounts first
- **Check Status**: Monitor bridge status before large transfers
- **Wait for Confirmations**: Don't trust unconfirmed deposits
- **Secure Environment**: Use the UI on a trusted, secure device

## Development

### Type Checking

```bash
npm run type-check
```

### Linting

```bash
npm run lint
```

### Building

```bash
# Development build
npm run dev

# Production build
npm run build

# The build output will be in dist/
```

## API Integration

The UI uses JSON-RPC 2.0 to communicate with the INTcoin node:

```typescript
// Example: Get bridge info
const response = await fetch('http://localhost:12212', {
  method: 'POST',
  headers: { 'Content-Type': 'application/json' },
  body: JSON.stringify({
    jsonrpc: '2.0',
    id: 1,
    method: 'getbridgeinfo',
    params: [],
  }),
})
```

## Deployment

### Production Build

```bash
npm run build
```

### Serve with Nginx

```nginx
server {
    listen 80;
    server_name bridge.international-coin.org;

    root /var/www/bridge-ui/dist;
    index index.html;

    location / {
        try_files $uri $uri/ /index.html;
    }

    # Proxy RPC requests to INTcoin node
    location /api {
        proxy_pass http://localhost:12212;
        proxy_set_header Host $host;
        proxy_set_header X-Real-IP $remote_addr;
    }
}
```

## Troubleshooting

### Connection Issues

**Problem**: Cannot connect to RPC server

**Solution**:
- Ensure INTcoin node is running
- Check RPC port (default: 12212)
- Verify firewall allows connections
- Check `intcoin.conf` for RPC settings

### Transaction Failures

**Problem**: Transaction submission fails

**Solution**:
- Verify all required fields are filled
- Check transaction hash format
- Ensure sufficient validator signatures
- Verify bridge is not paused
- Check you have sufficient balance (for withdrawals)

### Balance Not Updating

**Problem**: Balance doesn't update after deposit

**Solution**:
- Wait for required confirmations
- Check deposit proof status
- Verify validator signatures collected
- Refresh the page
- Check transaction in the transactions tab

## Support

- **GitHub**: https://github.com/INT-devs/intcoin
- **Discord**: https://discord.gg/jCy3eNgx
- **Email**: team@international-coin.org
- **Documentation**: https://international-coin.org/docs

## License

MIT License - see [LICENSE](../LICENSE) for details

## Version

INTcoin Bridge UI v1.2.0-beta

**Status**: Beta - Use with caution

---

**Maintainer**: INTcoin Development Team
**Contact**: team@international-coin.org
**Last Updated**: January 1, 2026
