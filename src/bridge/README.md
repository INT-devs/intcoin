// Copyright (c) 2025 INTcoin Core (Maddison Lane)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

# INTcoin Cross-Chain Bridge

## Overview

The Cross-Chain Bridge enables trustless atomic swaps between INTcoin and other blockchains, allowing users to exchange assets across different networks without intermediaries.

## Features

✅ **Atomic Swaps**
- Hash Time Locked Contracts (HTLC)
- Trustless peer-to-peer exchange
- Automatic refund on timeout
- No custody or escrow required

✅ **SPV Proof Verification**
- Simplified Payment Verification
- Merkle proof validation
- Block header chain verification
- Multi-confirmation security

✅ **Supported Chains**
- Bitcoin (BTC)
- Ethereum (ETH)
- Litecoin (LTC) - planned
- Monero (XMR) - planned

✅ **Bridge Features**
- Multi-signature security
- Automatic monitoring
- Chain synchronization
- Statistics tracking
- Event notifications

## Architecture

```
┌─────────────────────────────────────────┐
│         Bridge Manager                   │
│                                          │
│  ┌────────────┐  ┌────────────┐        │
│  │   Bitcoin  │  │  Ethereum  │        │
│  │   Bridge   │  │   Bridge   │        │
│  └─────┬──────┘  └─────┬──────┘        │
│        │                │               │
└────────┼────────────────┼───────────────┘
         │                │
         │                │
    ┌────▼────┐      ┌────▼────┐
    │  Bitcoin│      │ Ethereum│
    │   SPV   │      │   SPV   │
    │ Verifier│      │ Verifier│
    └────┬────┘      └────┬────┘
         │                │
    ┌────▼────────────────▼────┐
    │   Atomic Swap Manager     │
    │   - HTLC Creation         │
    │   - Secret Management     │
    │   - Timeout Monitoring    │
    └──────────────────────────┘
```

## Atomic Swap Process

### Step-by-Step Flow

1. **Initiation** (Alice wants BTC, Bob wants INT)
   ```
   Alice                              Bob
     │                                 │
     │  1. Generate secret preimage    │
     │  2. Create hash_lock = H(secret)│
     │  3. Lock INT with HTLC          │
     ├────────────────────────────────►│
     │     (hash_lock, timelock_A)     │
     │                                 │
     │  4. Verify INT is locked        │
     │  5. Lock BTC with same hash_lock│
     │◄────────────────────────────────┤
     │     (hash_lock, timelock_B)     │
     │                                 │
     │  6. Claim BTC with secret       │
     ├────────────────────────────────►│
     │     (reveal secret)             │
     │                                 │
     │  7. Claim INT with secret       │
     │◄────────────────────────────────┤
     │                                 │
   ```

2. **Security Guarantees**
   - Both parties either complete swap or get refunded
   - No party can steal funds
   - Secret revelation is atomic
   - Timelocks prevent indefinite locking

3. **Timeout Handling**
   ```
   If timelock_A expires:
     Alice can refund her INT

   If timelock_B expires:
     Bob can refund his BTC

   timelock_B < timelock_A (safety margin)
   ```

## Usage

### Creating a Bitcoin Bridge

```cpp
#include <intcoin/bridge/bridge.h>

using namespace intcoin::bridge;

// Initialize blockchain
Blockchain blockchain;

// Create Bitcoin bridge
BitcoinBridge btc_bridge(&blockchain, "http://localhost:8332");

// Start bridge
if (btc_bridge.start()) {
    std::cout << "Bitcoin bridge online" << std::endl;
    std::cout << "Chain height: " << btc_bridge.get_chain_height() << std::endl;
}
```

### Initiating a Swap

```cpp
// Initiate swap: Send 1 INT, receive BTC
PublicKey recipient_btc_address = /* Bitcoin address */;
uint64_t amount_to_send = 100000000;  // 1 INT

Hash256 swap_id = btc_bridge.initiate_swap(recipient_btc_address, amount_to_send);

std::cout << "Swap initiated: " << swap_id.to_string() << std::endl;
std::cout << "Waiting for counterparty to lock BTC..." << std::endl;
```

### Completing a Swap

```cpp
// Counterparty locked funds, we have the secret
Hash256 secret = /* received from counterparty */;

if (btc_bridge.complete_swap(swap_id, secret)) {
    std::cout << "Swap completed successfully!" << std::endl;
} else {
    std::cerr << "Swap completion failed" << std::endl;
}
```

### Managing Multiple Bridges

```cpp
// Create bridge manager
BridgeManager manager(&blockchain);

// Add bridges
auto btc_bridge = std::make_shared<BitcoinBridge>(&blockchain);
auto eth_bridge = std::make_shared<EthereumBridge>(&blockchain);

manager.add_bridge(ChainType::BITCOIN, btc_bridge);
manager.add_bridge(ChainType::ETHEREUM, eth_bridge);

// Start all bridges
manager.start_all();

// Get statistics
auto stats = manager.get_all_stats();
std::cout << "Total bridges: " << stats.total_bridges << std::endl;
std::cout << "Online bridges: " << stats.online_bridges << std::endl;
std::cout << "Total swaps: " << stats.total_swaps << std::endl;
```

### SPV Proof Verification

```cpp
// Verify a Bitcoin transaction is confirmed
CrossChainProof proof = /* received proof */;

if (btc_bridge.verify_lock_proof(swap_id, proof)) {
    std::cout << "Proof verified - BTC locked" << std::endl;

    // Get confirmations
    uint32_t confirmations = proof.get_spv_proof()
        .get_confirmations(btc_bridge.get_chain_height());

    std::cout << "Confirmations: " << confirmations << std::endl;
}
```

## Security Model

### Hash Time Locked Contracts (HTLC)

**Properties:**
- **Atomicity**: Both parties complete or both refund
- **Trustless**: No third party custody
- **Time-bound**: Automatic refund after timeout
- **Secret-based**: Only holder of preimage can claim

**Script Structure:**
```
IF
  // Claim branch (with secret)
  <recipient_pubkey> CHECKSIG
  <hash_lock> EQUAL
ELSE
  // Refund branch (after timeout)
  <timelock> CHECKLOCKTIMEVERIFY DROP
  <sender_pubkey> CHECKSIG
ENDIF
```

### SPV Security

**Merkle Proofs:**
- Prove transaction inclusion in block
- O(log n) proof size
- Efficient verification without full block

**Block Header Validation:**
- Verify proof-of-work
- Check difficulty adjustments
- Validate chain continuity
- Require multiple confirmations

**Recommended Confirmations:**
- Bitcoin: 6 confirmations (~60 minutes)
- Ethereum: 12 confirmations (~3 minutes)
- INTcoin: 6 confirmations (~12 minutes)

### Attack Mitigations

**Double-Spend Prevention:**
- Wait for sufficient confirmations
- Monitor for chain reorganizations
- Verify continuous chain of headers

**Front-Running Prevention:**
- Random secret generation
- Hash commitment before reveal
- Minimal time windows

**Denial of Service:**
- Rate limiting on swap creation
- Minimum swap amounts
- Automatic refund on timeout

## Configuration

### Bridge Parameters

```cpp
// Bitcoin bridge config
struct BitcoinBridgeConfig {
    std::string rpc_url = "http://localhost:8332";
    uint32_t min_confirmations = 6;
    uint32_t timelock_duration = 144;  // ~24 hours in blocks
    uint64_t min_swap_amount = 100000;  // 0.001 BTC
    uint64_t max_swap_amount = 10000000000;  // 100 BTC
};

// Ethereum bridge config
struct EthereumBridgeConfig {
    std::string rpc_url = "http://localhost:8545";
    std::string contract_address;
    uint32_t min_confirmations = 12;
    uint32_t timelock_duration = 5760;  // ~24 hours in blocks
    uint64_t gas_limit = 200000;
    uint64_t gas_price = 20000000000;  // 20 Gwei
};
```

### Timelock Guidelines

**Safety Margin:**
```
timelock_participant = timelock_initiator / 2

Example:
- Initiator timelock: 24 hours
- Participant timelock: 12 hours

This ensures participant can claim before initiator can refund.
```

## API Reference

### AtomicSwap

```cpp
class AtomicSwap {
    // Create new swap
    static std::shared_ptr<AtomicSwap> initiate(
        const PublicKey& initiator,
        const PublicKey& participant,
        uint64_t initiator_amount,
        uint64_t participant_amount,
        ChainType initiator_chain,
        ChainType participant_chain,
        uint32_t timelock_duration
    );

    // Lock funds in HTLC
    bool lock_funds(const Transaction& tx, ChainType chain);

    // Claim funds with secret
    bool claim_funds(const Hash256& secret);

    // Refund after timeout
    bool refund_funds();

    // Query state
    SwapState get_state() const;
    Hash256 get_hash_lock() const;
};
```

### SPVProof

```cpp
class SPVProof {
    // Create proof
    static SPVProof create(
        const Transaction& tx,
        const Block& block,
        uint32_t tx_index
    );

    // Verify proof
    bool verify(const Hash256& expected_block_hash) const;
    bool verify_confirmations(
        uint32_t required_confirmations,
        uint32_t current_height
    ) const;

    // Get proof data
    SPVBlockHeader get_block_header() const;
    MerkleProof get_merkle_proof() const;
    uint32_t get_confirmations(uint32_t current_height) const;
};
```

### Bridge

```cpp
class Bridge {
    // Start/stop bridge
    virtual bool start() = 0;
    virtual void stop() = 0;

    // Initiate swap
    virtual Hash256 initiate_swap(
        const PublicKey& recipient,
        uint64_t amount
    ) = 0;

    // Complete swap with secret
    virtual bool complete_swap(
        const Hash256& swap_id,
        const Hash256& secret
    ) = 0;

    // Verify proof
    virtual bool verify_lock_proof(
        const Hash256& swap_id,
        const CrossChainProof& proof
    ) = 0;

    // Get status
    virtual BridgeStatus get_status() const = 0;
    virtual BridgeStats get_stats() const = 0;
};
```

## Examples

### Complete Swap Flow

```cpp
// Alice wants to swap 1 INT for 0.01 BTC with Bob

// 1. Alice initiates swap
SwapBuilder builder;
auto swap = builder
    .initiator(alice_pubkey)
    .participant(bob_pubkey)
    .send_amount(100000000)  // 1 INT
    .receive_amount(1000000)  // 0.01 BTC
    .send_chain(ChainType::INTCOIN)
    .receive_chain(ChainType::BITCOIN)
    .timelock(144)  // 24 hours
    .build();

// 2. Alice locks INT
Transaction alice_lock_tx = create_htlc_transaction(
    swap->get_hash_lock(),
    bob_pubkey,
    100000000,
    swap->get_initiator_htlc().time_lock
);
blockchain.submit_transaction(alice_lock_tx);

// 3. Bob verifies and locks BTC
// (Bob's code)
bool verified = verify_intcoin_lock(swap->get_swap_id());
if (verified) {
    // Bob locks BTC with same hash_lock
    bitcoin_lock_tx = create_bitcoin_htlc(
        swap->get_hash_lock(),
        alice_btc_address,
        1000000,
        swap->get_participant_htlc().time_lock
    );
}

// 4. Alice claims BTC by revealing secret
Hash256 secret = swap->get_secret();
bitcoin_claim_tx = claim_bitcoin_htlc(secret);

// 5. Bob sees secret on Bitcoin chain and claims INT
Hash256 revealed_secret = extract_secret_from_bitcoin(bitcoin_claim_tx);
intcoin_claim_tx = claim_intcoin_htlc(revealed_secret);

std::cout << "Swap completed!" << std::endl;
```

## Testing

```bash
# Start test bridges
./intcoin-bridge-test --bitcoin-regtest --ethereum-ganache

# Run atomic swap test
./test_atomic_swap

# Run SPV proof test
./test_spv_proof

# Integration test
./test_bridge_integration
```

## Monitoring

### Bridge Statistics

```cpp
auto stats = bridge.get_stats();

std::cout << "Total swaps: " << stats.total_swaps << std::endl;
std::cout << "Completed: " << stats.completed_swaps << std::endl;
std::cout << "Failed: " << stats.failed_swaps << std::endl;
std::cout << "Success rate: " << (stats.success_rate * 100) << "%" << std::endl;
std::cout << "Avg swap time: " << stats.avg_swap_time << " seconds" << std::endl;
```

### Event Callbacks

```cpp
swap_manager.set_swap_completed_callback([](const Hash256& swap_id) {
    std::cout << "Swap completed: " << swap_id.to_string() << std::endl;
    // Send notification, update UI, etc.
});

swap_manager.set_swap_expired_callback([](const Hash256& swap_id) {
    std::cerr << "Swap expired: " << swap_id.to_string() << std::endl;
    // Initiate refund
});
```

## Future Enhancements

### Planned Features

1. **Additional Chain Support**
   - Litecoin bridge
   - Monero bridge (special techniques for privacy coins)
   - Cardano, Polkadot, etc.

2. **Liquidity Pools**
   - Automated market makers
   - Provide liquidity for instant swaps
   - Earn fees from swaps

3. **Relay Network**
   - Decentralized relayers
   - Incentivized proof submission
   - Multi-signature security

4. **Advanced Features**
   - Submarine swaps (on-chain ↔ Lightning)
   - Batch swaps for efficiency
   - Privacy-preserving swaps

5. **User Experience**
   - Swap marketplace
   - Price oracle integration
   - One-click swaps
   - Mobile wallet integration

## Security Considerations

⚠️ **Important Notes:**

1. **Confirmation Requirements**
   - Always wait for sufficient confirmations
   - Higher value swaps need more confirmations
   - Monitor for chain reorganizations

2. **Timelock Safety**
   - Set appropriate timelock durations
   - Consider network congestion
   - Participant timelock < Initiator timelock

3. **Secret Management**
   - Generate cryptographically secure secrets
   - Never reuse secrets
   - Store secrets securely

4. **Network Monitoring**
   - Monitor both chains continuously
   - Watch for unusual activity
   - Have fallback RPC nodes

5. **Testing**
   - Test on testnets first
   - Start with small amounts
   - Verify all security properties

## References

- [Atomic Swaps Whitepaper](https://en.bitcoin.it/wiki/Atomic_swap)
- [HTLC Specification](https://github.com/bitcoin/bips/blob/master/bip-0199.mediawiki)
- [SPV Security](https://bitcoin.org/en/operating-modes-guide#simplified-payment-verification-spv)
- [Ethereum HTLC](https://github.com/ethereum/EIPs/issues/1844)

## License

MIT License - See LICENSE file for details

## Authors

INTcoin Core Development Team
