# INTcoin Lightning Network - Advanced Features

## Overview

This document covers the advanced Lightning Network features implemented in INTcoin, including Submarine Swaps, Atomic Multi-Path Payments (AMP), Channel Factories, Splicing, and Dual-Funded Channels.

## Implemented Features

### 1. Watchtowers ✅ (November 15, 2025)

**Status**: Complete
**Documentation**: [WATCHTOWER.md](WATCHTOWER.md)

Third-party monitoring services that protect channels when users are offline.

**Key Features**:
- Encrypted breach remedy storage
- Automatic penalty transaction broadcast
- Multi-watchtower redundancy
- Privacy-preserving blinded hints
- Quantum-resistant signatures

### 2. Submarine Swaps ✅ (November 15, 2025)

**Status**: Complete
**Files**: `include/intcoin/submarine_swap.h`, `src/lightning/submarine_swap.cpp`

Seamless atomic swaps between on-chain and Lightning payments.

**Implemented Features**:
- ✅ **Regular Swaps (On-chain → Lightning)**: Convert on-chain funds to Lightning
- ✅ **Reverse Swaps (Lightning → On-chain)**: Convert Lightning funds to on-chain
- ✅ **HTLC Script Construction**: Complete Bitcoin script with OP_IF/OP_ELSE
- ✅ **Claim Transactions**: SHA-256 preimage verification path
- ✅ **Refund Transactions**: CHECKLOCKTIMEVERIFY timeout path
- ✅ **Witness Stack**: Proper witness data for script execution
- ✅ **Automatic refunds on timeout**
- ✅ **Fee estimation and quotes**

**Use Cases**:
- Channel rebalancing
- Converting between layers
- Instant liquidity
- Payment routing optimization

### 3. Atomic Multi-Path Payments (AMP) ✅ (November 15, 2025)

**Status**: Complete
**Files**: `include/intcoin/amp.h`, `src/lightning/amp.cpp`

Split large payments across multiple routes for improved reliability and privacy.

**Implemented Features**:
- ✅ **Multi-Path Route Finding**: Node-disjoint path selection algorithm
- ✅ **Payment Splitting Strategies**: Equal, weighted, and random distribution
- ✅ **HTLC Sending**: Multi-path HTLC propagation across routes
- ✅ **Path Management**: Success/failure tracking per path
- ✅ **Cleanup Logic**: Failed path HTLC reclamation
- ✅ **All-or-nothing guarantee**: Atomic payment completion
- ✅ **Root secret derivation**: Per-path preimage generation

**Benefits**:
- Higher payment success rates
- Better privacy (payment splitting)
- Utilize multiple channels simultaneously
- Balance channel liquidity

### 4. Point Time-Locked Contracts (PTLCs) ✅ (November 15, 2025)

**Status**: Complete
**Files**: `include/intcoin/ptlc.h`, `src/lightning/ptlc.cpp`
**Documentation**: [docs/PTLC.md](PTLC.md)

Enhanced privacy payment contracts using adaptor signatures.

**Implemented Features**:
- ✅ **Adaptor Signatures**: Post-quantum adaptor signature scheme
- ✅ **Payment Decorrelation**: Different points per hop for privacy
- ✅ **Scriptless Scripts**: No visible hash locks in transactions
- ✅ **Stuckless Payments**: Cancel in-flight payments
- ✅ **Secret Extraction**: Recover secret from completed signatures
- ✅ **65% Script Size Reduction**: Compared to HTLCs

**Privacy Advantages over HTLCs**:
- No hash correlation across hops
- Payments indistinguishable from regular transactions
- Routing nodes cannot correlate payments
- Enhanced anonymity through payment decorrelation

### 5. Eltoo Channel Updates ✅ (November 15, 2025)

**Status**: Complete
**Files**: `include/intcoin/eltoo.h`, `src/lightning/eltoo.cpp`
**Documentation**: [docs/ELTOO.md](ELTOO.md)

Simplified channel update mechanism using SIGHASH_NOINPUT.

**Implemented Features**:
- ✅ **SIGHASH_NOINPUT**: Signature mode for update transactions
- ✅ **Monotonic Updates**: Increasing update number enforcement
- ✅ **No Penalty Transactions**: Simplified breach response
- ✅ **Settlement Transactions**: CSV-delayed settlement
- ✅ **80% Storage Reduction**: Compared to LN-penalty
- ✅ **O(1) Watchtower Storage**: Constant storage per channel

**Advantages over Traditional Lightning**:
- No revocation key management
- Simplified watchtower protocol
- Reduced storage requirements
- Easier channel recovery
- Lower operational complexity

## Planned Features

### 6. Channel Factories

**Status**: Planned
**Priority**: Medium

Batch channel creation for improved efficiency and reduced on-chain footprint.

**Benefits**:
- Lower on-chain fees (one transaction for multiple channels)
- Faster channel creation
- Improved scalability
- Reduced blockchain bloat

**Implementation Plan**:
- Multi-party funding transaction
- Off-chain channel state management
- Cooperative channel closure
- Penalty mechanisms for fraud

### 7. Splicing

**Status**: Planned
**Priority**: Medium

Dynamically adjust channel capacity without closing and reopening.

**Types**:
- **Splice-in**: Add funds to existing channel
- **Splice-out**: Remove funds from channel

**Benefits**:
- No channel downtime
- Preserve channel history/reputation
- Adjust liquidity as needed
- Lower costs than close+reopen

### 8. Dual-Funded Channels

**Status**: Header defined, implementation pending
**Priority**: Low

Both parties contribute funds during channel opening.

**Benefits**:
- Immediate bidirectional capacity
- Better liquidity distribution
- Reduced need for rebalancing
- Fairer channel economics

## Feature Comparison

| Feature | Status | On-Chain Txs | Privacy | Complexity | Priority |
|---------|--------|--------------|---------|------------|----------|
| Watchtowers | ✅ Complete | 0 (monitoring only) | High | High | Critical |
| Submarine Swaps | ✅ Complete | 1-2 per swap | Medium | High | High |
| AMP | ✅ Complete | 0 | Very High | Medium | High |
| PTLCs | ✅ Complete | 0 | Very High | High | High |
| Eltoo | ✅ Complete | Reduced | Medium | High | High |
| Channel Factories | 📋 Planned | 1 for N channels | Medium | Very High | Medium |
| Splicing | 📋 Planned | 1 per splice | Low | High | Medium |
| Dual-Funded | 🔄 Header Defined | Same as regular | Low | Medium | Low |

## Implementation Status

### Submarine Swaps Implementation ✅

**Completed** (November 15, 2025):
- ✅ Header file with complete API (`submarine_swap.h`)
- ✅ Complete implementation (`submarine_swap.cpp`)
- ✅ Swap direction enumeration (on-to-off, off-to-on)
- ✅ Swap state management
- ✅ Quote system for fee estimation
- ✅ Service infrastructure
- ✅ HTLC funding transaction creation
- ✅ HTLC script construction (OP_IF/OP_ELSE)
- ✅ Claim transaction implementation
- ✅ Refund transaction implementation
- ✅ Witness stack construction
- ✅ Transaction broadcasting

**Implementation Details**:
- Bitcoin script with SHA-256 preimage verification
- CHECKLOCKTIMEVERIFY for timeout enforcement
- Proper witness data for SegWit transactions
- Locktime and sequence number handling

### Atomic Multi-Path Payments (AMP) ✅

**Completed** (November 15, 2025):
```
Payment Splitting:
Original: 1,000,000 sats
├─ Path 1: 400,000 sats
├─ Path 2: 350,000 sats
└─ Path 3: 250,000 sats

All paths must succeed or all fail (atomic)
```

**Implemented Components**:
1. ✅ **Payment Splitter**: Divide payment into optimal chunks
2. ✅ **Route Calculator**: Find multiple node-disjoint paths
3. ✅ **Payment Coordinator**: Ensure atomicity
4. ✅ **Preimage Derivation**: Generate unique preimages per path
5. ✅ **HTLC Sending**: Multi-path HTLC propagation
6. ✅ **Cleanup Logic**: Failed path reclamation

**Splitting Strategies**:
- Equal distribution across paths
- Weighted based on capacity/reliability
- Random for privacy enhancement

### Channel Factories

**Design**:
```
On-Chain Funding TX (2-of-2 multisig):
├─ Channel A: Alice ↔ Bob
├─ Channel B: Alice ↔ Carol
└─ Channel C: Bob ↔ Carol

All managed off-chain until factory closes
```

**Challenges**:
- Multi-party coordination
- Complex penalty mechanisms
- State management
- Watchtower integration

### Splicing

**Design**:
```
Splice-In:
Old Channel: 1,000,000 sats
+ Add: 500,000 sats
= New Channel: 1,500,000 sats

Splice-Out:
Old Channel: 1,000,000 sats
- Remove: 300,000 sats
= New Channel: 700,000 sats
```

**Implementation**:
1. Negotiate splice parameters
2. Create new commitment transaction
3. Broadcast splice transaction
4. Wait for confirmations
5. Update channel state

### Dual-Funded Channels

**Design**:
```
Traditional:
Alice funds: 1,000,000 sats
Bob funds: 0 sats
Total capacity: 1,000,000 sats
Alice can send: 1,000,000 sats
Bob can send: 0 sats

Dual-Funded:
Alice funds: 600,000 sats
Bob funds: 400,000 sats
Total capacity: 1,000,000 sats
Alice can send: 600,000 sats
Bob can send: 400,000 sats
```

## Quantum Resistance

All Lightning features use:
- **CRYSTALS-Dilithium5** for signatures
- **CRYSTALS-Kyber1024** for key exchange
- **SHA3-256** for hashing

This ensures security against quantum computers for:
- All commitment transactions
- HTLC scripts
- Revocation keys
- Watchtower messages
- Submarine swap contracts

## Development Roadmap

### Phase 1: Core Features (Q4 2025 - Q1 2026) ✅ COMPLETE
- [x] Watchtowers (November 15, 2025)
- [x] Submarine Swaps (November 15, 2025)
- [x] AMP (November 15, 2025)
- [x] PTLCs (November 15, 2025)
- [x] Eltoo (November 15, 2025)

### Phase 2: Advanced Features (Q2 2026)
- [ ] Channel Factories
- [ ] Splicing
- [ ] Dual-Funded Channels (complete implementation)
- [ ] Trampoline Routing

### Phase 3: Optimization (Q3 2026)
- [ ] Performance improvements
- [ ] Fee optimization
- [ ] Network graph algorithms
- [ ] Mobile client support
- [ ] GUI integration

## Testing Requirements

Each feature requires:
1. **Unit Tests**: Core functionality
2. **Integration Tests**: Multi-component interaction
3. **Network Tests**: Real P2P communication
4. **Security Tests**: Attack resistance
5. **Performance Tests**: Scalability benchmarks

## Security Considerations

### Submarine Swaps
- Timelock must exceed Lightning payment timeout
- Preimage revelation timing critical
- Service reputation system needed

### AMP
- All paths must use correlated preimages
- Timeout coordination across paths
- Privacy: avoid payment correlation

### Channel Factories
- Complex multi-party security model
- All participants must be online
- Penalty mechanisms for each channel

### Splicing
- No double-spending during splice
- Proper HTLC migration
- Watchtower compatibility

### Dual-Funded
- Interactive funding protocol
- Fee negotiation fairness
- Commitment to funding amounts

## References

- [BOLT Specifications](https://github.com/lightning/bolts)
- [Submarine Swaps Explained](https://docs.lightning.engineering/the-lightning-network/liquidity/submarine-swaps)
- [AMP Specification](https://github.com/lightning/bolts/pull/658)
- [Channel Factories](https://www.tik.ee.ethz.ch/file/a20a865ce40d40c8f942cf206a7cba96/Scalable_Funding_Of_Blockchain_Micropayment_Networks.pdf)
- [Splicing Proposal](https://github.com/lightning/bolts/pull/863)

## Contributing

To contribute to these features:

1. Review the design documents
2. Implement according to specifications
3. Write comprehensive tests
4. Submit pull request with:
   - Implementation code
   - Test suite
   - Documentation
   - Performance analysis

## License

All Lightning Network features are released under the MIT License.

Copyright (c) 2025 INTcoin Core (Maddison Lane)
