# INTcoin Exchange Integration Guide

**Version:** 1.0
**Status:** Production Ready
**Last Updated:** November 25, 2025

## Overview

This guide provides comprehensive documentation for integrating INTcoin with cryptocurrency exchanges. It covers security requirements, API specifications, wallet management, and compliance considerations for exchange platform integration.

## Table of Contents

- [Exchange Architecture](#exchange-architecture)
- [Security Requirements](#security-requirements)
- [Wallet Management](#wallet-management)
- [API Specifications](#api-specifications)
- [Deposit Processing](#deposit-processing)
- [Withdrawal Processing](#withdrawal-processing)
- [Compliance & KYC/AML](#compliance--kycaml)
- [Testing & Validation](#testing--validation)
- [Troubleshooting](#troubleshooting)

## Exchange Architecture

### System Components

```
┌─────────────────────────────────────────────────┐
│         Exchange Platform                       │
│  ┌──────────────────────────────────────────┐   │
│  │  API Layer (HTTP/HTTPS)                  │   │
│  │  - Authentication                        │   │
│  │  - Rate Limiting                         │   │
│  │  - Request Validation                    │   │
│  └──────────────────────────────────────────┘   │
│                    │                             │
│  ┌─────────────────┴──────────────────────────┐ │
│  │  Exchange Integration Layer                │ │
│  │  - Deposit Handling                        │ │
│  │  - Withdrawal Processing                  │ │
│  │  - Balance Management                     │ │
│  │  - Transaction Tracking                   │ │
│  └──────────────────────────────────────────┘  │
│                    │                             │
├────────────────────┴──────────────────────────┤
│         INTcoin Node Operations               │
│  ┌──────────────────────────────────────────┐ │
│  │  Wallet Management                       │ │
│  │  - Hot Wallet (Operational)              │ │
│  │  - Cold Wallet (Storage)                 │ │
│  │  - Multi-Signature Wallets               │ │
│  └──────────────────────────────────────────┘ │
│  ┌──────────────────────────────────────────┐ │
│  │  Transaction Management                  │ │
│  │  - Address Generation                    │ │
│  │  - Transaction Signing                   │ │
│  │  - Broadcasting                          │ │
│  │  - Confirmation Tracking                 │ │
│  └──────────────────────────────────────────┘ │
└────────────────────────────────────────────────┘
```

### Key Principles

1. **Security**: Multi-signature enforcement, encryption, access controls
2. **Auditability**: Complete transaction history and logging
3. **Compliance**: KYC/AML integration, regulatory reporting
4. **Reliability**: Fault tolerance, automatic recovery, monitoring

## Security Requirements

### Wallet Segregation

**Hot Wallet (Operational)**
- Holds 1-5% of total funds
- Automated operations
- Accessible to API layer
- Monitored for suspicious activity
- Hourly security audits

**Cold Wallet (Storage)**
- Holds 95-99% of total funds
- Manual access only
- Offline or air-gapped
- Multi-signature enforcement (2-of-3 minimum)
- Quarterly access audits

### Cryptographic Requirements

**Key Management:**
- All keys use Dilithium5 (NIST Level 5, quantum-resistant)
- Key encapsulation using Kyber1024 (NIST Level 5)
- Keys encrypted with AES-256-GCM
- No plaintext key storage
- Secure key backup with encryption

**Transaction Signing:**
- All transactions signed with Dilithium5
- Signature verification before broadcasting
- Signature size: 4,627 bytes per transaction
- Post-quantum security maintained throughout

### Access Control

```
User Access Levels:

1. Admin (Full Control)
   - Wallet management
   - Configuration changes
   - User management
   - Audit log access

2. Operator (Operational Control)
   - Deposit processing
   - Withdrawal initiation
   - Balance inquiry
   - Transaction status

3. Auditor (Read-Only)
   - Audit log access
   - Transaction history
   - No operational capabilities

4. Monitor (System Monitoring)
   - Performance metrics
   - Error logs
   - Notifications
```

### Network Security

- **TLS 1.3+** required for all API communications
- **Rate Limiting**: 100 requests/second per API key
- **IP Whitelisting**: Recommended for institutional clients
- **DDoS Protection**: Automatic mitigation enabled
- **WAF Rules**: Query validation and injection prevention

## Wallet Management

### Wallet Types

#### 1. Deposit Wallet (Hot)
```
Purpose: Receive customer deposits
Security: Single-signature (API-accessible)
Address Generation: Sequential HD derivation
Monitoring: Real-time balance tracking
Rebalancing: Automatic to cold storage when threshold exceeded
```

**Configuration:**
```
Max Balance: 10,000 INT
Rebalance Threshold: 8,000 INT
Rebalance Target: 1,000 INT
Monitoring Interval: 1 minute
```

#### 2. Withdrawal Wallet (Hot)
```
Purpose: Execute customer withdrawals
Security: Multi-signature (2-of-3)
Signing Keys: Distributed among operators
Monitoring: Every withdrawal transaction
Approval Process: 2 signatures required
```

**Multi-Signature Setup:**
- Key 1: Operator A (in hardware wallet)
- Key 2: Operator B (in hardware wallet)
- Key 3: Backup (cold storage)
- Threshold: 2-of-3 signatures required

#### 3. Cold Storage Wallet
```
Purpose: Long-term fund storage
Security: Multi-signature (3-of-5 or higher)
Access: Manual only, via physical process
Monitoring: Monthly audit
Key Distribution: Geographic separation
```

### Address Generation

**Hierarchical Deterministic (HD) Wallet:**
- Master key derived from seed phrase (24 words)
- Derivation path: `m/44'/1998'/0'/0/n` (BIP 44 standard)
- Each customer receives unique address
- Addresses never reused
- Address tracking for customer verification

**Example Flow:**
```
Seed Phrase (256-bit entropy)
    │
    ├─→ Master Private Key (XPRV)
    │       │
    │       ├─→ Account 0 (m/44'/1998'/0')
    │       │       │
    │       │       ├─→ External Chain (deposits)
    │       │       │       ├─→ Address 0
    │       │       │       ├─→ Address 1
    │       │       │       └─→ Address N
    │       │       │
    │       │       └─→ Internal Chain (change)
    │       │
    │       └─→ Account 1 (reserved for future)
```

### Balance Management

**Balance Tracking:**
- Real-time UTXO tracking
- Mempool monitoring for pending transactions
- Confirmation counting (6 confirmations standard)
- Orphaned transaction handling

**Balance Types:**
1. **Available Balance**: Confirmed and spendable
2. **Pending Balance**: Unconfirmed deposits
3. **Reserved Balance**: Pending withdrawals
4. **Total Balance**: Sum of all above

## API Specifications

### Exchange API Endpoints

#### 1. Account Management

**Get Account Balance**
```
GET /api/v1/account/balance

Response:
{
  "available": "10500.00",
  "pending": "250.50",
  "reserved": "1000.00",
  "total": "11750.50",
  "currency": "INT"
}
```

**Get Deposit Address**
```
GET /api/v1/account/deposit-address

Response:
{
  "address": "int1q...",
  "label": "Customer deposit #1",
  "created_at": "2025-11-25T10:30:00Z",
  "currency": "INT"
}
```

#### 2. Deposit Management

**Create Deposit Address**
```
POST /api/v1/deposits/address
Content-Type: application/json

Request:
{
  "customer_id": "cust_12345",
  "label": "Customer deposit"
}

Response:
{
  "address": "int1q...",
  "derivation_path": "m/44'/1998'/0'/0/42",
  "currency": "INT",
  "status": "active"
}
```

**List Deposits**
```
GET /api/v1/deposits?status=completed&limit=100

Response:
{
  "deposits": [
    {
      "id": "dep_123",
      "address": "int1q...",
      "amount": "100.00",
      "currency": "INT",
      "status": "confirmed",
      "confirmations": 6,
      "txid": "0x...",
      "created_at": "2025-11-25T10:30:00Z"
    }
  ]
}
```

#### 3. Withdrawal Management

**Create Withdrawal**
```
POST /api/v1/withdrawals
Content-Type: application/json

Request:
{
  "customer_id": "cust_12345",
  "amount": "50.00",
  "address": "int1r...",
  "fee_level": "standard"
}

Response:
{
  "id": "wdl_456",
  "status": "pending_approval",
  "amount": "50.00",
  "fee": "0.01",
  "total": "50.01",
  "signatures_required": 2,
  "signatures_obtained": 0,
  "txid": null
}
```

**Approve Withdrawal**
```
POST /api/v1/withdrawals/{id}/approve
Content-Type: application/json
Authorization: Bearer {signature}

Request:
{
  "operator_id": "op_789",
  "signature": "dilithium5_signature_base64"
}

Response:
{
  "id": "wdl_456",
  "status": "signed",
  "signatures_obtained": 1,
  "signatures_required": 2
}
```

### Security Validators

#### 1. Transaction Validator
- Verifies sender/recipient addresses
- Validates transaction amount
- Checks fee reasonableness
- Ensures output scripts are valid
- Verifies no double-spending

#### 2. Compliance Validator
- KYC status verification
- AML risk scoring
- Transaction limit enforcement
- Geographic restrictions
- Sanction list checking

#### 3. Operational Validator
- Rate limit enforcement
- API key validation
- Request signature verification
- Nonce checking for replay prevention
- Timestamp validation (±5 minutes)

## Deposit Processing

### Deposit Flow

```
1. Customer initiates deposit on exchange
   └─→ Exchange creates deposit address (HD derivation)

2. Customer sends INT to address
   └─→ Transaction broadcast to INTcoin network
   └─→ Included in mempool

3. Transaction confirmed (≥6 confirmations)
   └─→ Exchange credit customer account
   └─→ Audit log entry created
   └─→ Customer notification sent

4. Periodic rebalancing (if hot wallet threshold exceeded)
   └─→ Automatic transfer to cold storage
   └─→ Multi-signature cold storage transaction
   └─→ Completion logged
```

### Confirmation Requirements

**Standard Process (Most Users):**
- 6 confirmations required
- ~30 minutes on INTcoin (5-minute blocks)
- Single confirmation database update
- Immediate trading availability

**Fast-Track (VIP/Large Orders):**
- 1 confirmation for trading
- 6 confirmations for withdrawal
- Reduced risk exposure

**Extra Security (For Institutional):**
- 12 confirmations required
- ~60 minutes
- Additional manual verification
- Optional insurance coverage

### Error Handling

**Double-Spend Detection:**
- Automatic detection in mempool
- Transaction replacement handling (RBF)
- Customer notification
- Optional automatic refund processing

**Orphaned Transactions:**
- Detection: Blocks not in main chain
- Action: Automatic redetection
- Confirmation: Counts reset on main chain inclusion
- Notification: Customer informed

## Withdrawal Processing

### Withdrawal Flow

```
1. Customer initiates withdrawal
   └─→ Compliance validator checks KYC/AML
   └─→ Rate limits verified
   └─→ Address validation performed

2. Exchange prepares transaction
   └─→ Inputs selected (UTXO consolidation)
   └─→ Outputs created
   └─→ Fee calculated
   └─→ Transaction hashed for signing

3. Multi-signature approval
   └─→ Operator 1 signs transaction
   └─→ Operator 2 signs transaction
   └─→ Threshold met (2-of-3)
   └─→ Final transaction constructed

4. Broadcasting
   └─→ Transaction broadcast to network
   └─→ Mempool inclusion verified
   └─→ Tracking enabled
   └─→ Customer notified

5. Confirmation
   └─→ 6 confirmations monitoring
   └─→ Final customer notification
   └─→ Account update completed
   └─→ Audit log entry
```

### Fee Calculation

**Fee Levels:**

| Level | Multiplier | Est. Time | Use Case |
|-------|-----------|-----------|----------|
| **Economy** | 1.0x | 30-60 min | Low priority |
| **Standard** | 1.5x | 10-30 min | Normal |
| **Fast** | 2.0x | 5-10 min | Time-sensitive |
| **Instant** | 3.0x | <5 min | Emergency |

**Example Calculation:**
```
Base Fee: 0.0001 INT/byte
Transaction Size: 256 bytes
Standard Multiplier: 1.5x

Fee = 256 * 0.0001 * 1.5 = 0.0384 INT

Customer Pays: Withdrawal Amount + 0.0384 INT
```

### Multi-Signature Approval Process

**Required Setup:**
1. **Operator A**: Hardware wallet (Key 1)
2. **Operator B**: Hardware wallet (Key 2)
3. **Backup**: Cold storage (Key 3)

**Approval Workflow:**
```
Exchange System Prepares Withdrawal
    │
    ├─→ Generate transaction hash
    │
    ├─→ Send to Operator A for signing
    │   └─→ Operator A physically signs with hardware wallet
    │   └─→ Signature returned to system
    │
    ├─→ Send to Operator B for signing
    │   └─→ Operator B physically signs with hardware wallet
    │   └─→ Signature returned to system
    │
    ├─→ Combine signatures (2-of-3 threshold met)
    │   └─→ Construct final signed transaction
    │
    └─→ Broadcast to network
        └─→ Transaction confirmed on blockchain
```

**Timeout Handling:**
- Operator response timeout: 15 minutes
- Automatic escalation to backup operator (Key 3)
- Re-attempt with different operator
- If no response: Notify admin and hold transaction

## Compliance & KYC/AML

### KYC Integration Points

**Customer Onboarding:**
```
1. Identity Verification
   - Government ID validation
   - Facial recognition (optional)
   - Address verification
   - Phone verification

2. Risk Assessment
   - Beneficial ownership identification
   - Source of funds verification
   - PEP (Politically Exposed Person) checks
   - Sanction list screening

3. Account Setup
   - Tier assignment (Basic/Standard/Premium)
   - Transaction limits based on tier
   - Notification thresholds
```

**Tier-Based Limits:**

| Tier | Daily Limit | Monthly Limit | Annual Limit |
|------|------------|---------------|-------------|
| **Basic** | $2,500 | $10,000 | $100,000 |
| **Standard** | $25,000 | $100,000 | $1,000,000 |
| **Premium** | No Limit | No Limit | No Limit |

### AML Monitoring

**Transaction Monitoring:**
- Unusual amount detection
- Pattern analysis
- Velocity checking
- Geographic risk assessment

**Alerts:**
- Automatic alerts on suspicious transactions
- Manual review queue
- Escalation procedures
- Regulatory reporting triggers

**Enhanced Due Diligence (EDD):**
- Large transaction investigation (>$50,000)
- High-risk jurisdiction detection
- Multiple transaction correlation
- Industry risk assessment

## Testing & Validation

### Integration Testing

**Test Scenarios:**
1. Deposit address generation and verification
2. Deposit processing with various amounts
3. Withdrawal approval workflow
4. Multi-signature transaction handling
5. Error scenarios and recovery
6. Rate limiting enforcement
7. Compliance validator integration

**Test Data:**
- Testnet INT coins (faucet available)
- Test API keys with rate limit bypass
- Sandbox environment for safe testing

### Performance Testing

**Benchmarks:**
- Deposit processing: 1,000+ ops/sec
- Withdrawal processing: 500+ ops/sec
- Address generation: 10,000+ ops/sec
- Balance queries: 5,000+ ops/sec
- API response time: <100ms (p95)

**Load Testing:**
- Concurrent deposits: 100+ simultaneous
- Concurrent API calls: 1,000+ simultaneous
- Mempool capacity: No impact on 1,000+ transactions
- Database performance: Sustained throughout

### Security Testing

**Penetration Testing:**
- API endpoint fuzzing
- Authentication bypass attempts
- Authorization testing
- Injection attack testing
- Rate limit bypass attempts

**Compliance Testing:**
- KYC/AML integration verification
- Sanction list checks
- Transaction limit enforcement
- Audit logging verification

## Troubleshooting

### Common Issues

**Issue: Deposits Not Appearing**
```
Diagnosis:
1. Verify deposit address is correct
2. Check blockchain for transaction
3. Verify transaction has ≥1 confirmation
4. Check exchange database for record

Solution:
- If on blockchain: Wait for 6 confirmations
- If not on blockchain: Verify address to sender
- If record missing: Manual database sync required
```

**Issue: Withdrawal Stuck in Pending**
```
Diagnosis:
1. Check signature count (need 2-of-3)
2. Verify operator availability
3. Check for timeout (15 min threshold)
4. Verify transaction format

Solution:
- Get second operator signature
- Re-send signing request if timeout
- Verify address is valid
- Check fee level appropriateness
```

**Issue: Slow Confirmation Timing**
```
Diagnosis:
1. Check network congestion (mempool size)
2. Verify fee level matches network conditions
3. Check INTcoin node synchronization

Solution:
- Increase fee level for faster confirmation
- Wait for network to clear
- Restart node if out of sync
```

### Support Contacts

- **Technical Support**: support@intcoin.org
- **Security Issues**: security@intcoin.org
- **Compliance**: compliance@intcoin.org
- **24/7 Hotline**: [phone number]

## Resources

- [INTcoin RPC API Documentation](RPC-API.md)
- [Wallet Management Guide](../README.md#wallet-management)
- [Security Best Practices](SECURITY-AUDIT.md)
- [Testing Guide](TESTING.md)
- [Fuzz Testing Infrastructure](FUZZ-TESTING.md)

## Version History

| Version | Date | Changes |
|---------|------|---------|
| 1.0 | 2025-11-25 | Initial release with 12 API tests, security validators, and compliance integration |

---

**License**: MIT License
**Copyright**: (c) 2025 INTcoin Core
**Status**: Production Ready for Integration
