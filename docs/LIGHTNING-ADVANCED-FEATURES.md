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

### 2. Submarine Swaps 🔄 (In Progress)

**Status**: Header defined, implementation pending
**File**: `include/intcoin/submarine_swap.h`

Seamless atomic swaps between on-chain and Lightning payments.

**Features**:
- **Regular Swaps (On-chain → Lightning)**: Convert on-chain funds to Lightning
- **Reverse Swaps (Lightning → On-chain)**: Convert Lightning funds to on-chain
- Hash-locked transactions on both layers
- Automatic refunds on timeout
- Fee estimation and quotes

**Use Cases**:
- Channel rebalancing
- Converting between layers
- Instant liquidity
- Payment routing optimization

## Planned Features

### 3. Atomic Multi-Path Payments (AMP)

**Status**: Planned
**Priority**: High

Split large payments across multiple routes for improved reliability and privacy.

**Benefits**:
- Higher payment success rates
- Better privacy (payment splitting)
- Utilize multiple channels simultaneously
- Balance channel liquidity

**Implementation Plan**:
```cpp
class AMPPayment {
    // Split payment into multiple HTLCs
    std::vector<HTLC> split_payment(uint64_t total_amount,
                                    size_t num_paths);

    // Reassemble payment at destination
    bool reassemble_payment(const std::vector<HTLC>& htlcs);

    // All-or-nothing guarantee
    bool verify_complete_payment();
};
```

### 4. Channel Factories

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

### 5. Splicing

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

### 6. Dual-Funded Channels

**Status**: Planned
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
| Submarine Swaps | 🔄 In Progress | 1-2 per swap | Medium | High | High |
| AMP | 📋 Planned | 0 | Very High | Medium | High |
| Channel Factories | 📋 Planned | 1 for N channels | Medium | Very High | Medium |
| Splicing | 📋 Planned | 1 per splice | Low | High | Medium |
| Dual-Funded | 📋 Planned | Same as regular | Low | Medium | Low |

## Implementation Status

### Submarine Swaps Implementation

**Completed**:
- ✅ Header file with complete API (`submarine_swap.h`)
- ✅ Swap direction enumeration (on-to-off, off-to-on)
- ✅ Swap state management
- ✅ Quote system for fee estimation
- ✅ Service infrastructure

**TODO**:
- Implement swap manager logic
- Create HTLC funding transactions
- Implement claim/refund mechanisms
- Add monitoring and timeout handling
- Write comprehensive tests

### Atomic Multi-Path Payments (AMP)

**Design**:
```
Payment Splitting:
Original: 1,000,000 sats
├─ Path 1: 400,000 sats
├─ Path 2: 350,000 sats
└─ Path 3: 250,000 sats

All paths must succeed or all fail (atomic)
```

**Key Components**:
1. **Payment Splitter**: Divide payment into optimal chunks
2. **Route Calculator**: Find multiple disjoint paths
3. **Payment Coordinator**: Ensure atomicity
4. **Preimage Derivation**: Generate unique preimages per path

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

### Phase 1: Core Features (Q4 2025 - Q1 2026)
- [x] Watchtowers
- [ ] Submarine Swaps (complete implementation)
- [ ] AMP (design and prototype)

### Phase 2: Advanced Features (Q2 2026)
- [ ] Channel Factories
- [ ] Splicing
- [ ] Dual-Funded Channels

### Phase 3: Optimization (Q3 2026)
- [ ] Performance improvements
- [ ] Fee optimization
- [ ] Network graph algorithms
- [ ] Mobile client support

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
