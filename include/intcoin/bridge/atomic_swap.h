// Copyright (c) 2025 INTcoin Core (Maddison Lane)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef INTCOIN_BRIDGE_ATOMIC_SWAP_H
#define INTCOIN_BRIDGE_ATOMIC_SWAP_H

#include "../primitives.h"
#include "../transaction.h"
#include "../crypto.h"
#include <string>
#include <chrono>
#include <memory>
#include <optional>

namespace intcoin {
namespace bridge {

/**
 * Atomic swap state
 */
enum class SwapState {
    INITIATED,      // Swap initiated, waiting for counterparty
    LOCKED,         // Both parties locked funds
    CLAIMED,        // Swap completed successfully
    REFUNDED,       // Swap refunded (timeout)
    CANCELLED       // Swap cancelled
};

/**
 * Supported blockchain types
 */
enum class ChainType {
    BITCOIN,
    ETHEREUM,
    LITECOIN,
    MONERO,
    CARDANO,
    INTCOIN
};

/**
 * Hash Time Locked Contract (HTLC) for atomic swaps
 *
 * Enables trustless cross-chain atomic swaps using hash locks and time locks
 */
struct HTLC {
    Hash256 hash_lock;          // SHA-256 hash of secret
    Hash256 secret;             // Secret preimage (if revealed)
    DilithiumPubKey sender;           // Initiating party
    DilithiumPubKey receiver;         // Receiving party
    uint64_t amount;            // Amount locked
    uint32_t time_lock;         // Block height or timestamp for refund
    ChainType chain;            // Which blockchain
    std::string chain_txid;     // Transaction ID on the chain
    bool secret_revealed;       // Whether secret has been revealed

    HTLC() : amount(0), time_lock(0), chain(ChainType::INTCOIN),
             secret_revealed(false) {}

    // Verify secret matches hash
    bool verify_secret(const Hash256& preimage) const;

    // Check if timelocked
    bool is_timelocked(uint32_t current_height) const;
};

/**
 * Atomic swap between two blockchains
 *
 * Implements Hash Time Locked Contract (HTLC) based atomic swaps
 */
class AtomicSwap {
public:
    AtomicSwap();
    ~AtomicSwap();

    // Swap initiation
    static std::shared_ptr<AtomicSwap> initiate(
        const DilithiumPubKey& initiator,
        const DilithiumPubKey& participant,
        uint64_t initiator_amount,
        uint64_t participant_amount,
        ChainType initiator_chain,
        ChainType participant_chain,
        uint32_t timelock_duration
    );

    // Swap operations
    bool lock_funds(const Transaction& tx, ChainType chain);
    bool claim_funds(const Hash256& secret);
    bool refund_funds();

    // Swap state
    SwapState get_state() const { return state_; }
    Hash256 get_swap_id() const { return swap_id_; }
    Hash256 get_hash_lock() const { return hash_lock_; }
    std::optional<Hash256> get_secret() const;

    // Swap details
    DilithiumPubKey get_initiator() const { return initiator_; }
    DilithiumPubKey get_participant() const { return participant_; }
    uint64_t get_initiator_amount() const { return initiator_amount_; }
    uint64_t get_participant_amount() const { return participant_amount_; }
    ChainType get_initiator_chain() const { return initiator_chain_; }
    ChainType get_participant_chain() const { return participant_chain_; }

    // HTLCs
    HTLC get_initiator_htlc() const { return initiator_htlc_; }
    HTLC get_participant_htlc() const { return participant_htlc_; }

    // Validation
    bool verify_initiator_lock() const;
    bool verify_participant_lock() const;
    bool is_expired(uint32_t current_height) const;

    // Serialization
    std::vector<uint8_t> serialize() const;
    static std::optional<AtomicSwap> deserialize(const std::vector<uint8_t>& data);

private:
    Hash256 swap_id_;
    Hash256 hash_lock_;
    Hash256 secret_;
    DilithiumPubKey initiator_;
    DilithiumPubKey participant_;

    uint64_t initiator_amount_;
    uint64_t participant_amount_;
    ChainType initiator_chain_;
    ChainType participant_chain_;

    HTLC initiator_htlc_;
    HTLC participant_htlc_;

    SwapState state_;
    uint32_t timelock_duration_;
    std::chrono::system_clock::time_point created_at_;

    // Generate hash lock
    static Hash256 generate_secret();
    static Hash256 hash_secret(const Hash256& secret);
};

/**
 * Atomic swap manager
 *
 * Manages multiple atomic swaps and monitors their states
 */
class AtomicSwapManager {
public:
    AtomicSwapManager();
    ~AtomicSwapManager();

    // Swap creation
    std::shared_ptr<AtomicSwap> create_swap(
        const DilithiumPubKey& participant,
        uint64_t send_amount,
        uint64_t receive_amount,
        ChainType send_chain,
        ChainType receive_chain,
        uint32_t timelock_duration = 24 * 60  // 24 hours default
    );

    // Swap management
    bool add_swap(std::shared_ptr<AtomicSwap> swap);
    std::shared_ptr<AtomicSwap> get_swap(const Hash256& swap_id);
    bool remove_swap(const Hash256& swap_id);

    // Swap operations
    bool lock_funds(const Hash256& swap_id, const Transaction& tx, ChainType chain);
    bool claim_funds(const Hash256& swap_id, const Hash256& secret);
    bool refund_funds(const Hash256& swap_id);

    // Queries
    std::vector<std::shared_ptr<AtomicSwap>> get_all_swaps() const;
    std::vector<std::shared_ptr<AtomicSwap>> get_swaps_by_state(SwapState state) const;
    std::vector<std::shared_ptr<AtomicSwap>> get_initiated_swaps() const;
    std::vector<std::shared_ptr<AtomicSwap>> get_pending_swaps() const;

    // Monitoring
    void monitor_swaps(uint32_t current_height);
    std::vector<Hash256> get_expired_swaps(uint32_t current_height) const;
    void cleanup_completed_swaps(uint32_t max_age_seconds);

    // Statistics
    size_t get_swap_count() const;
    size_t get_active_swap_count() const;
    uint64_t get_total_volume() const;

    // Callbacks
    void set_swap_completed_callback(
        std::function<void(const Hash256&)> callback);
    void set_swap_expired_callback(
        std::function<void(const Hash256&)> callback);

private:
    std::unordered_map<Hash256, std::shared_ptr<AtomicSwap>> swaps_;
    mutable std::mutex swaps_mutex_;

    std::function<void(const Hash256&)> swap_completed_callback_;
    std::function<void(const Hash256&)> swap_expired_callback_;
};

/**
 * Swap builder for convenient creation
 */
class SwapBuilder {
public:
    SwapBuilder();

    SwapBuilder& initiator(const DilithiumPubKey& key);
    SwapBuilder& participant(const DilithiumPubKey& key);
    SwapBuilder& send_amount(uint64_t amount);
    SwapBuilder& receive_amount(uint64_t amount);
    SwapBuilder& send_chain(ChainType chain);
    SwapBuilder& receive_chain(ChainType chain);
    SwapBuilder& timelock(uint32_t blocks);

    std::shared_ptr<AtomicSwap> build();

private:
    DilithiumPubKey initiator_;
    DilithiumPubKey participant_;
    uint64_t send_amount_;
    uint64_t receive_amount_;
    ChainType send_chain_;
    ChainType receive_chain_;
    uint32_t timelock_;
};

} // namespace bridge
} // namespace intcoin

#endif // INTCOIN_BRIDGE_ATOMIC_SWAP_H
