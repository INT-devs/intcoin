# INTcoin Cross-Chain Bridge Guide

**Version**: 1.2.0-beta
**Last Updated**: January 2, 2026
**Status**: Testnet Only - Mainnet Pending Security Audit

---

## Table of Contents

1. [Introduction](#introduction)
2. [Supported Assets](#supported-assets)
3. [Bridge Architecture](#bridge-architecture)
4. [User Guide - Deposits](#user-guide---deposits)
5. [User Guide - Withdrawals](#user-guide---withdrawals)
6. [Bridge Web UI](#bridge-web-ui)
7. [RPC API Reference](#rpc-api-reference)
8. [Wrapped Tokens](#wrapped-tokens)
9. [Security Model](#security-model)
10. [Fees and Limits](#fees-and-limits)
11. [Troubleshooting](#troubleshooting)
12. [FAQ](#faq)

---

## Introduction

The INTcoin Cross-Chain Bridge enables trustless transfer of assets between INTcoin and other blockchains. Using a **federated multi-signature model**, the bridge allows users to:

- **Lock** assets on origin chain (e.g., Bitcoin)
- **Mint** equivalent wrapped tokens on INTcoin (e.g., wBTC)
- **Burn** wrapped tokens on INTcoin
- **Unlock** original assets on origin chain

### Key Features

- 🌉 **Multi-Chain Support** - Bitcoin, Ethereum, Litecoin
- 🔒 **Federated Security** - M-of-N validator signatures required
- 💰 **1:1 Peg** - Wrapped tokens backed by locked assets
- 🌐 **Web UI** - User-friendly interface for deposits/withdrawals
- 📊 **Transparent** - Public audit trail, supply verification
- ⚡ **Fast** - <30 minute finality for most transactions
- 🛡️ **Secure** - Multi-layer validation, emergency pause mechanism

### Use Cases

- **DeFi on INTcoin** - Use BTC/ETH/LTC in INTcoin DeFi protocols
- **Cross-chain arbitrage** - Trade assets across chains
- **Portfolio diversification** - Hold multiple assets on single chain
- **Lightning Network** - Use wBTC for Lightning channels
- **Yield farming** - Earn with wrapped assets

---

## Supported Assets

### Wrapped Bitcoin (wBTC)

- **Origin Chain**: Bitcoin (BTC)
- **Contract Address**: INT_wBTC_CONTRACT (testnet)
- **Decimals**: 8 (same as BTC)
- **Minimum Deposit**: 0.001 BTC (~$40)
- **Confirmations Required**: 6 blocks (~60 minutes)
- **Withdrawal Fee**: 0.0005 BTC
- **Status**: ✅ Testnet Active

### Wrapped Ethereum (wETH)

- **Origin Chain**: Ethereum (ETH)
- **Contract Address**: INT_wETH_CONTRACT (testnet)
- **Decimals**: 18 (same as ETH)
- **Minimum Deposit**: 0.01 ETH (~$30)
- **Confirmations Required**: 12 blocks (~3 minutes)
- **Withdrawal Fee**: 0.005 ETH (includes gas)
- **Status**: ✅ Testnet Active

### Wrapped Litecoin (wLTC)

- **Origin Chain**: Litecoin (LTC)
- **Contract Address**: INT_wLTC_CONTRACT (testnet)
- **Decimals**: 8 (same as LTC)
- **Minimum Deposit**: 0.1 LTC
- **Confirmations Required**: 24 blocks (~60 minutes)
- **Withdrawal Fee**: 0.01 LTC
- **Status**: ✅ Testnet Active

### Upcoming Assets

- Wrapped Monero (wXMR) - Q2 2026
- Wrapped Cardano (wADA) - Q3 2026
- More assets based on community demand

---

## Bridge Architecture

### Federated Multi-Signature Model

The bridge uses a **federated multi-sig** approach where multiple independent validators must approve transactions.

**Current Configuration (Testnet)**:
- **Total Validators**: 5
- **Required Signatures**: 3 (3-of-5)
- **Validator Selection**: Reputable community members

**Planned Mainnet Configuration**:
- **Total Validators**: 11+
- **Required Signatures**: 7+ (7-of-11)
- **Validator Selection**: Elected by community, rotating set

### How It Works

**Deposit Flow**:
```
1. User sends BTC to bridge address on Bitcoin
   ↓
2. Bitcoin validators monitor and verify transaction (6 confirmations)
   ↓
3. Validators submit deposit proof to INTcoin bridge contract
   ↓
4. Once M-of-N validators approve, wBTC is minted to user's INT address
   ↓
5. User receives wBTC on INTcoin (typically <30 minutes total)
```

**Withdrawal Flow**:
```
1. User initiates withdrawal request, burning wBTC on INTcoin
   ↓
2. Validators verify burn transaction
   ↓
3. Validators create and sign Bitcoin unlock transaction
   ↓
4. Once M-of-N signatures collected, Bitcoin transaction broadcast
   ↓
5. User receives BTC on Bitcoin (typically <30 minutes total)
```

### Security Features

- **Multi-sig validation** - No single validator can move funds
- **Supply verification** - Automated checks ensure wrapped supply = locked assets
- **Emergency pause** - Circuit breaker for security incidents
- **Validator reputation** - Track record of validator performance
- **Time locks** - Large transactions have mandatory delay
- **Audit trail** - All bridge operations publicly verifiable

---

## User Guide - Deposits

### Depositing Bitcoin (BTC → wBTC)

**Step 1: Get Deposit Address**

Using RPC:
```bash
intcoin-cli bridgedeposit \
  '{
    "asset": "BTC",
    "destination_address": "int1qxy2kgdygjrsqtzq2n0yrf2493p83kkfjhx0wlh"
  }'
```

Response:
```json
{
  "deposit_id": "d7a8b9c0-1234-5678-abcd-ef9012345678",
  "deposit_address": "bc1qbridge...",
  "min_amount": "0.001",
  "max_amount": "10.0",
  "confirmations_required": 6,
  "estimated_time": "60 minutes",
  "expires_at": "2026-01-03T12:00:00Z"
}
```

Using Web UI:
1. Go to https://bridge.intcoin.org
2. Connect wallet (MetaMask, Keplr, or INTcoin wallet)
3. Select "Deposit"
4. Choose "Bitcoin (BTC)"
5. Enter deposit amount
6. Click "Generate Deposit Address"
7. Copy deposit address

**Step 2: Send BTC to Deposit Address**

```bash
bitcoin-cli sendtoaddress bc1qbridge... 0.1
```

Or use any Bitcoin wallet to send to the deposit address.

**Important**:
- Send exact amount or more (change is not returned)
- Include sufficient fee for timely confirmation
- Deposit address expires in 24 hours (generate new if expired)

**Step 3: Submit Deposit Proof**

After your Bitcoin transaction confirms:

```bash
intcoin-cli bridgedeposit \
  '{
    "deposit_id": "d7a8b9c0-1234-5678-abcd-ef9012345678",
    "tx_hash": "bitcoin_transaction_hash",
    "tx_index": 0,
    "amount": "0.1"
  }'
```

Or the Web UI will auto-detect and submit proof.

**Step 4: Wait for Validator Signatures**

Monitor progress:
```bash
intcoin-cli getbridgeinfo d7a8b9c0-1234-5678-abcd-ef9012345678
```

Status progression:
```
PENDING → CONFIRMED (6 blocks) → VALIDATOR_SIGNING → MINTING → COMPLETED
```

**Step 5: Receive wBTC**

Once validators approve (3-of-5 signatures), wBTC is automatically minted to your INT address.

Check balance:
```bash
intcoin-cli getbridgebalance
```

### Depositing Ethereum (ETH → wETH)

Similar process to Bitcoin:

1. Get deposit address (ETH smart contract)
2. Send ETH to contract address
3. Wait for 12 confirmations (~3 minutes)
4. Submit deposit proof
5. Wait for validator signatures
6. Receive wETH on INTcoin

**Note**: Ethereum deposits may cost $5-50 in gas fees depending on network congestion.

### Depositing Litecoin (LTC → wLTC)

1. Get deposit address
2. Send LTC
3. Wait for 24 confirmations (~60 minutes)
4. Submit proof
5. Receive wLTC

---

## User Guide - Withdrawals

### Withdrawing to Bitcoin (wBTC → BTC)

**Step 1: Initiate Withdrawal**

Using RPC:
```bash
intcoin-cli bridgewithdraw \
  '{
    "asset": "wBTC",
    "amount": "0.1",
    "destination_address": "bc1qyour_bitcoin_address..."
  }'
```

Response:
```json
{
  "withdrawal_id": "w8b9c0d1-5678-9012-cdef-345678901234",
  "status": "PENDING",
  "amount": "0.1",
  "fee": "0.0005",
  "net_amount": "0.0995",
  "destination": "bc1q...",
  "estimated_time": "30 minutes"
}
```

Using Web UI:
1. Go to https://bridge.intcoin.org
2. Select "Withdraw"
3. Choose "Bitcoin (BTC)"
4. Enter amount and Bitcoin address
5. Review fee and net amount
6. Click "Withdraw"
7. Confirm transaction in wallet

**Step 2: Burn wBTC**

The withdrawal process automatically burns your wBTC:
- wBTC is removed from circulation
- Burn transaction is verified by validators
- Cannot be reversed after burning

**Step 3: Wait for Validator Signatures**

```bash
intcoin-cli getbridgeinfo w8b9c0d1-5678-9012-cdef-345678901234
```

Status:
```
PENDING → BURN_CONFIRMED → VALIDATOR_SIGNING → UNLOCKING → COMPLETED
```

Validators will:
1. Verify wBTC burn transaction (6 confirmations)
2. Create Bitcoin unlock transaction
3. Sign with their validator keys
4. Once 3-of-5 signatures collected, broadcast to Bitcoin

**Step 4: Receive BTC**

Bitcoin transaction is automatically broadcast when threshold reached.

Track on Bitcoin block explorer:
```
https://blockstream.info/tx/<tx_hash>
```

**Typical Timeline**:
- Burn confirmation: 30 minutes (6 INT blocks)
- Validator signing: 5-15 minutes
- Bitcoin confirmation: 60 minutes (6 BTC blocks)
- **Total**: 1.5-2 hours

### Withdrawing to Ethereum (wETH → ETH)

Similar process:
1. Burn wETH on INTcoin
2. Validators sign Ethereum unlock transaction
3. Transaction broadcast to Ethereum
4. Receive ETH after 12 confirmations

**Gas fees**: Paid from withdrawal amount (typically $10-100 depending on congestion).

### Withdrawing to Litecoin (wLTC → LTC)

1. Burn wLTC
2. Validators sign
3. Broadcast to Litecoin
4. Receive after 24 confirmations

---

## Bridge Web UI

### Accessing the UI

**Testnet**: https://testnet-bridge.intcoin.org
**Mainnet** (when available): https://bridge.intcoin.org

### Connecting Wallet

Supported wallets:
- **INTcoin Wallet** (desktop or mobile)
- **MetaMask** (for Ethereum operations)
- **Keplr** (for Cosmos ecosystem)
- **WalletConnect** (mobile wallets)

1. Click "Connect Wallet"
2. Select wallet type
3. Approve connection in wallet
4. Your INTcoin address displays in top right

### Deposit Interface

**Features**:
- Asset selection dropdown (BTC, ETH, LTC)
- Amount input with min/max limits
- Real-time fee calculation
- QR code for deposit address
- Automatic transaction detection
- Status tracking with progress bar

**Steps**:
1. Select asset to deposit
2. Enter amount
3. Click "Generate Deposit Address"
4. Copy address or scan QR code
5. Send from origin chain
6. Monitor progress in "Transactions" tab

### Withdrawal Interface

**Features**:
- Asset selection for withdrawal
- Destination address validation
- Fee breakdown (bridge fee + network fee)
- Net amount calculation
- Confirmation dialog

**Steps**:
1. Select wrapped asset (wBTC, wETH, wLTC)
2. Enter withdrawal amount
3. Enter destination address on origin chain
4. Review fees and net amount
5. Click "Withdraw"
6. Confirm in wallet
7. Monitor in "Transactions" tab

### Transaction Monitoring

**Transaction List** shows:
- Transaction ID
- Type (Deposit/Withdrawal)
- Asset
- Amount
- Status with progress indicators
- Timestamp
- Estimated completion time

**Status Indicators**:
- 🔄 Pending
- ⏱️ Confirming
- 🔏 Validator Signing
- ✅ Completed
- ❌ Failed

Click transaction for details:
- Block confirmations
- Validator signatures (X/5)
- Transaction hashes (both chains)
- Fee breakdown

### Bridge Statistics Dashboard

**Displays**:
- Total Value Locked (TVL)
- Total wBTC supply / locked BTC
- Total wETH supply / locked ETH
- Total wLTC supply / locked LTC
- 24h deposit/withdrawal volume
- Active validators
- Average completion time

---

## RPC API Reference

### bridgedeposit

Generate deposit address or submit deposit proof.

**Generate Deposit Address**:
```bash
intcoin-cli bridgedeposit \
  '{
    "asset": "BTC|ETH|LTC",
    "destination_address": "int1q..."
  }'
```

**Submit Deposit Proof**:
```bash
intcoin-cli bridgedeposit \
  '{
    "deposit_id": "uuid",
    "tx_hash": "origin_chain_tx_hash",
    "tx_index": 0,
    "amount": "decimal_string"
  }'
```

### bridgewithdraw

Initiate withdrawal (burn wrapped tokens, unlock on origin chain).

```bash
intcoin-cli bridgewithdraw \
  '{
    "asset": "wBTC|wETH|wLTC",
    "amount": "decimal_string",
    "destination_address": "origin_chain_address"
  }'
```

### getbridgebalance

Get balance of all wrapped tokens.

```bash
intcoin-cli getbridgebalance
```

Response:
```json
{
  "wBTC": "1.5",
  "wETH": "10.25",
  "wLTC": "100.0",
  "total_usd": "75000.00"
}
```

### listbridgetransactions

List bridge transaction history.

```bash
intcoin-cli listbridgetransactions
```

Optional parameters:
- `asset`: Filter by asset
- `type`: "deposit" or "withdrawal"
- `status`: Filter by status
- `limit`: Max results (default: 100)
- `offset`: Pagination offset

### getbridgeinfo

Get detailed info about a bridge transaction.

```bash
intcoin-cli getbridgeinfo <transaction_id>
```

---

## Wrapped Tokens

### Token Metadata

Each wrapped token includes:
- **Name**: e.g., "Wrapped Bitcoin"
- **Symbol**: e.g., "wBTC"
- **Decimals**: Matches origin chain
- **Total Supply**: Equals locked assets
- **Origin Chain**: Source blockchain
- **Contract Address**: INT blockchain address

### Supply Verification

**Public Audit**:
```bash
intcoin-cli getbridgestats
```

Shows:
- Total wBTC minted
- Total BTC locked in bridge contract
- Discrepancy (should always be 0)
- Last verification block

**Automated Verification**:
- Runs every 100 blocks (~50 minutes)
- Compares wrapped supply to locked assets
- Alerts validators if mismatch detected
- Public verification log

### Using Wrapped Tokens

wBTC, wETH, wLTC can be used like any INT asset:

**Send**:
```bash
intcoin-cli sendtoaddress <address> <amount> <asset=wBTC>
```

**Lightning Network**:
- Open channels with wBTC
- Make payments with wrapped assets
- Earn routing fees

**DeFi** (coming in v1.3.0):
- Provide liquidity in DEX pools
- Collateralize loans
- Yield farming

---

## Security Model

### Multi-Sig Validators

**Current Testnet (3-of-5)**:
- 5 independent validators
- 3 signatures required for any action
- Validators: Community-selected, geographically distributed

**Planned Mainnet (7-of-11)**:
- 11+ validators
- 7+ signatures required
- Elected by community
- Regular rotation (every 6 months)
- Performance-based reputation

### Validator Requirements

- **Hardware Security Module (HSM)** for key storage
- **99.9% uptime** requirement
- **Geographic diversity** (no concentration)
- **Transparent operations** (public logs)
- **Stake requirement** (to be determined)
- **Background checks** for mainnet

### Attack Scenarios & Mitigations

**Scenario 1: Validator Collusion**
- **Risk**: 3-of-5 validators collude to steal funds
- **Mitigation**:
  - Diverse, reputable validator selection
  - Public transparency of all actions
  - Insurance fund for losses
  - Rapid validator rotation if suspicious

**Scenario 2: Minting Bug**
- **Risk**: Bug allows minting without locking assets
- **Mitigation**:
  - Multi-layer validation
  - Supply verification every 100 blocks
  - Code audits before mainnet
  - Emergency pause mechanism

**Scenario 3: Chain Reorganization**
- **Risk**: Deep reorg invalidates deposit/withdrawal
- **Mitigation**:
  - High confirmation requirements (BTC: 6, ETH: 12, LTC: 24)
  - Monitoring for reorgs
  - Refund mechanism if detected

**Scenario 4: Validator Key Compromise**
- **Risk**: Attacker obtains validator private key
- **Mitigation**:
  - HSM requirement (keys never in memory)
  - Rate limiting on validator operations
  - Immediate key rotation if compromise suspected
  - Multi-sig prevents single key compromise

### Emergency Procedures

**Emergency Pause**:
- Activated if anomaly detected
- Halts all deposits and withdrawals
- Requires 3-of-5 validator approval OR automated trigger
- Transparent public announcement
- Resume requires 4-of-5 validator approval

**Validator Rotation**:
- Scheduled rotation every 6 months
- Emergency rotation if performance/security issues
- Smooth handover with overlap period
- No service interruption

### Insurance Fund

- **Size**: 10% of Total Value Locked
- **Purpose**: Compensate users for bridge failures
- **Funding**: Community treasury
- **Claims Process**: Transparent, community-reviewed

---

## Fees and Limits

### Deposit Fees

- **Bitcoin**: Bridge fee 0%, network fee paid by user
- **Ethereum**: Gas fee paid by user (varies)
- **Litecoin**: Bridge fee 0%, network fee paid by user

### Withdrawal Fees

| Asset | Bridge Fee | Network Fee | Total Fee |
|-------|-----------|-------------|-----------|
| wBTC → BTC | 0.0005 BTC | Varies | ~0.0005-0.001 BTC |
| wETH → ETH | 0.005 ETH | Varies | ~0.005-0.05 ETH |
| wLTC → LTC | 0.01 LTC | Varies | ~0.01-0.02 LTC |

Network fees vary based on congestion. Bridge fee goes to validator operations.

### Transaction Limits

**Testnet Limits**:
| Asset | Min Deposit | Max Deposit | Max Withdrawal |
|-------|------------|-------------|----------------|
| BTC | 0.001 BTC | 10 BTC | 5 BTC |
| ETH | 0.01 ETH | 100 ETH | 50 ETH |
| LTC | 0.1 LTC | 1000 LTC | 500 LTC |

**Mainnet Limits** (planned):
- Higher limits after security audit
- Graduated limits based on age/reputation
- Institutional limits available

### Time Locks

Large transactions have mandatory delays:
- **> 10 BTC**: 24-hour delay
- **> 100 ETH**: 24-hour delay
- **> 10,000 LTC**: 24-hour delay

Allows time for anomaly detection and emergency response.

---

## Troubleshooting

### Deposit Not Appearing

**Check**:
1. Sufficient confirmations on origin chain
2. Sent to correct deposit address
3. Deposit amount above minimum
4. Deposit address hasn't expired

**Solutions**:
1. Wait for confirmations
2. Check transaction on origin chain block explorer
3. Submit deposit proof manually if auto-detection failed
4. Contact support with transaction details

### Withdrawal Delayed

**Common Causes**:
- Insufficient validator signatures
- Origin chain congestion
- Large transaction time lock

**Check Status**:
```bash
intcoin-cli getbridgeinfo <withdrawal_id>
```

### "Insufficient Wrapped Token Balance"

**Cause**: Not enough wBTC/wETH/wLTC

**Solutions**:
1. Check balance: `intcoin-cli getbridgebalance`
2. Complete deposit first
3. Reduce withdrawal amount

### Validator Signature Timeout

**If validators aren't signing**:
1. Check validator status
2. Wait (may be temporary)
3. If > 4 hours, contact support
4. Emergency pause may be active

---

## FAQ

### Are my assets safe in the bridge?

Assets are secured by:
- Multi-sig validation (3-of-5 testnet, 7-of-11 mainnet)
- Transparent operations
- Supply verification
- Emergency pause mechanism
- Insurance fund

**Risks**: Validator collusion, smart contract bugs
**Recommendation**: Start with small amounts, use testnet first

### Can I reverse a deposit/withdrawal?

**Deposits**: Cannot reverse after submitting to origin chain
**Withdrawals**: Cannot reverse after burning wrapped tokens

### How long does it take?

**Deposits**: 30-120 minutes (varies by chain)
**Withdrawals**: 30-120 minutes

### What if a validator goes offline?

With M-of-N (3-of-5), system continues with remaining validators. If too many offline, operations pause until restored.

### Can the bridge be shut down?

Emergency pause can halt operations. Existing locked assets remain secure. Resume requires validator approval.

### Are wrapped tokens the same as original?

Wrapped tokens are **pegged 1:1** to originals and backed by locked assets. Functionally similar but on INTcoin chain.

### Can I earn yield with wrapped tokens?

Currently: Use in Lightning Network for routing fees
Coming v1.3.0: DeFi protocols (DEX, lending, yield farming)

---

## Resources

- **Bridge Web UI**: https://bridge.intcoin.org (mainnet TBA)
- **Testnet Bridge**: https://testnet-bridge.intcoin.org
- **Bridge Validator Guide**: [BRIDGE_VALIDATOR.md](BRIDGE_VALIDATOR.md)
- **Source Code**: [src/bridge/](../src/bridge/)
- **Block Explorer**: https://explorer.international-coin.org

---

## Support

- **GitHub Issues**: https://github.com/InternationalCoin/intcoin/issues
- **Bridge Support**: bridge@international-coin.org
- **Security Issues**: security@international-coin.org (PGP encrypted)

---

**Maintained by**: INTcoin Core Development Team
**Last Updated**: January 2, 2026
**Version**: 1.2.0-beta
**Status**: Testnet Only - Mainnet Pending Audit
