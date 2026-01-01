// Copyright (c) 2024-2025 The INTcoin Core developers
// Distributed under the MIT software license

#ifndef INTCOIN_ATOMIC_SWAP_H
#define INTCOIN_ATOMIC_SWAP_H

#include <intcoin/htlc.h>
#include <intcoin/types.h>

#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace intcoin {
namespace atomic_swap {

/// Supported blockchain networks for atomic swaps
enum class SwapChain : uint8_t {
    INTCOIN,     // INTcoin (this chain)
    BITCOIN,     // Bitcoin mainnet
    LITECOIN,    // Litecoin mainnet
    TESTNET_INT, // INTcoin testnet
    TESTNET_BTC, // Bitcoin testnet
    TESTNET_LTC  // Litecoin testnet
};

/// Swap role (initiator or participant)
enum class SwapRole {
    INITIATOR,   // Initiates the swap (Alice)
    PARTICIPANT  // Accepts the swap (Bob)
};

/// Swap state machine states
enum class SwapState {
    // Pre-swap states
    OFFER_CREATED,       // Swap offer created by initiator
    OFFER_SENT,          // Offer sent to participant
    OFFER_RECEIVED,      // Offer received by participant
    OFFER_ACCEPTED,      // Participant accepted the offer

    // Contract deployment states
    INITIATOR_HTLC_PENDING,   // Initiator creating HTLC
    INITIATOR_HTLC_FUNDED,    // Initiator HTLC on-chain
    PARTICIPANT_HTLC_PENDING, // Participant creating HTLC
    PARTICIPANT_HTLC_FUNDED,  // Participant HTLC on-chain

    // Execution states
    PARTICIPANT_CLAIMED,  // Participant claimed with preimage
    INITIATOR_CLAIMED,    // Initiator claimed with preimage
    COMPLETED,            // Both parties claimed successfully

    // Failure states
    CANCELLED,            // Swap cancelled before execution
    EXPIRED,              // Swap expired without completion
    REFUNDED,             // One or both parties refunded
    FAILED                // Swap failed for other reasons
};

/// Swap offer parameters
struct SwapOffer {
    /// Unique swap ID (hash of offer parameters)
    uint256 swap_id;

    /// Initiator's chain
    SwapChain initiator_chain;

    /// Participant's chain
    SwapChain participant_chain;

    /// Amount initiator sends (in smallest unit: INTS for INTcoin, satoshis for BTC)
    uint64_t initiator_amount;

    /// Amount participant sends
    uint64_t participant_amount;

    /// Initiator's public key
    std::vector<uint8_t> initiator_pubkey;

    /// Participant's public key (empty in initial offer)
    std::vector<uint8_t> participant_pubkey;

    /// Payment hash (SHA3-256 for INTcoin, SHA-256 for Bitcoin)
    std::vector<uint8_t> payment_hash;

    /// Locktime for initiator's HTLC (Unix timestamp)
    uint64_t initiator_locktime;

    /// Locktime for participant's HTLC (must be < initiator_locktime)
    uint64_t participant_locktime;

    /// Offer expiration time
    uint64_t offer_expires_at;

    /// Optional contact info for negotiation
    std::string contact_info;

    /// Offer signature (signed by initiator)
    std::vector<uint8_t> signature;
};

/// Swap contract details (HTLC on each chain)
struct SwapContract {
    /// HTLC transaction hash
    uint256 htlc_tx_hash;

    /// HTLC output index
    uint32_t htlc_output_index;

    /// HTLC script
    std::vector<uint8_t> htlc_script;

    /// HTLC amount
    uint64_t amount;

    /// Locktime
    uint64_t locktime;

    /// Block height when HTLC was created
    uint64_t creation_height;

    /// Number of confirmations required
    uint32_t required_confirmations;
};

/// Complete swap information
struct SwapInfo {
    /// Swap offer
    SwapOffer offer;

    /// Current swap state
    SwapState state;

    /// Swap role (initiator or participant)
    SwapRole role;

    /// Initiator's HTLC contract
    SwapContract initiator_contract;

    /// Participant's HTLC contract
    SwapContract participant_contract;

    /// Secret preimage (32 bytes, revealed when claiming)
    std::vector<uint8_t> preimage;

    /// Timestamp when swap was created
    uint64_t created_at;

    /// Timestamp when swap state last changed
    uint64_t updated_at;

    /// Error message (if swap failed)
    std::string error_message;
};

/// Swap event types for callbacks
enum class SwapEventType {
    OFFER_RECEIVED,           // New swap offer received
    OFFER_ACCEPTED,           // Offer accepted by participant
    INITIATOR_HTLC_DETECTED,  // Initiator's HTLC detected on-chain
    PARTICIPANT_HTLC_DETECTED,// Participant's HTLC detected on-chain
    PREIMAGE_REVEALED,        // Secret preimage revealed
    SWAP_COMPLETED,           // Swap completed successfully
    SWAP_FAILED,              // Swap failed
    SWAP_REFUNDED             // Swap refunded
};

/// Swap event callback
struct SwapEvent {
    SwapEventType type;
    uint256 swap_id;
    SwapState new_state;
    std::string message;
};

/// Atomic swap coordinator
/// Manages the swap state machine and coordinates HTLC operations on both chains
class AtomicSwapCoordinator {
public:
    /// Constructor
    AtomicSwapCoordinator();

    /// Destructor
    ~AtomicSwapCoordinator();

    // ========================================
    // Swap Creation & Negotiation
    // ========================================

    /// Create a new swap offer (initiator)
    /// @param initiator_chain Initiator's blockchain
    /// @param participant_chain Participant's blockchain
    /// @param initiator_amount Amount to send
    /// @param participant_amount Amount to receive
    /// @param initiator_pubkey Initiator's public key
    /// @param locktime_hours Hours until locktime (default: 48)
    /// @return Swap offer
    Result<SwapOffer> CreateSwapOffer(
        SwapChain initiator_chain,
        SwapChain participant_chain,
        uint64_t initiator_amount,
        uint64_t participant_amount,
        const std::vector<uint8_t>& initiator_pubkey,
        uint32_t locktime_hours = 48
    );

    /// Accept a swap offer (participant)
    /// @param offer Swap offer to accept
    /// @param participant_pubkey Participant's public key
    /// @return Updated swap offer with participant info
    Result<SwapOffer> AcceptSwapOffer(
        const SwapOffer& offer,
        const std::vector<uint8_t>& participant_pubkey
    );

    /// Cancel a swap offer
    /// @param swap_id Swap ID
    /// @return Success/failure result
    Result<void> CancelSwap(const uint256& swap_id);

    // ========================================
    // Swap Execution
    // ========================================

    /// Start swap execution (creates HTLCs)
    /// @param swap_id Swap ID
    /// @return Success/failure result
    Result<void> StartSwapExecution(const uint256& swap_id);

    /// Create initiator's HTLC
    /// @param swap_id Swap ID
    /// @param funding_inputs UTXOs to fund HTLC
    /// @return HTLC transaction
    Result<Transaction> CreateInitiatorHTLC(
        const uint256& swap_id,
        const std::vector<TxIn>& funding_inputs
    );

    /// Create participant's HTLC
    /// @param swap_id Swap ID
    /// @param funding_inputs UTXOs to fund HTLC
    /// @return HTLC transaction
    Result<Transaction> CreateParticipantHTLC(
        const uint256& swap_id,
        const std::vector<TxIn>& funding_inputs
    );

    /// Claim HTLC with preimage
    /// @param swap_id Swap ID
    /// @param is_initiator True if claiming as initiator
    /// @return Claim transaction
    Result<Transaction> ClaimHTLC(
        const uint256& swap_id,
        bool is_initiator
    );

    /// Refund expired HTLC
    /// @param swap_id Swap ID
    /// @param is_initiator True if refunding as initiator
    /// @return Refund transaction
    Result<Transaction> RefundHTLC(
        const uint256& swap_id,
        bool is_initiator
    );

    // ========================================
    // Swap Monitoring
    // ========================================

    /// Monitor swap progress (call periodically)
    /// @param swap_id Swap ID
    /// @return Updated swap state
    Result<SwapState> MonitorSwap(const uint256& swap_id);

    /// Check if HTLC is funded on chain
    /// @param contract HTLC contract details
    /// @return Number of confirmations (0 if not found)
    uint32_t CheckHTLCConfirmations(const SwapContract& contract);

    /// Watch for preimage revelation
    /// @param swap_id Swap ID
    /// @return Preimage if revealed, empty otherwise
    std::vector<uint8_t> WatchForPreimage(const uint256& swap_id);

    /// Check if swap has expired
    /// @param swap_id Swap ID
    /// @return True if swap can be refunded
    bool IsSwapExpired(const uint256& swap_id);

    // ========================================
    // Swap Query
    // ========================================

    /// Get swap information
    /// @param swap_id Swap ID
    /// @return Swap info
    Result<SwapInfo> GetSwapInfo(const uint256& swap_id) const;

    /// Get all swaps
    /// @return Vector of all swaps
    std::vector<SwapInfo> GetAllSwaps() const;

    /// Get swaps by state
    /// @param state Swap state to filter
    /// @return Vector of swaps with specified state
    std::vector<SwapInfo> GetSwapsByState(SwapState state) const;

    /// Get swap count
    /// @return Number of tracked swaps
    size_t GetSwapCount() const;

    // ========================================
    // Callbacks
    // ========================================

    /// Set swap event callback
    /// @param callback Function to call on swap events
    void SetSwapEventCallback(std::function<void(const SwapEvent&)> callback);

    // ========================================
    // Utilities
    // ========================================

    /// Generate secret preimage
    /// @return 32-byte random preimage
    static std::vector<uint8_t> GeneratePreimage();

    /// Compute payment hash from preimage
    /// @param preimage Secret preimage
    /// @param chain Blockchain (determines hash algorithm)
    /// @return Payment hash
    static std::vector<uint8_t> ComputePaymentHash(
        const std::vector<uint8_t>& preimage,
        SwapChain chain
    );

    /// Get chain name
    /// @param chain Blockchain
    /// @return Chain name string
    static std::string GetChainName(SwapChain chain);

    /// Get state name
    /// @param state Swap state
    /// @return State name string
    static std::string GetStateName(SwapState state);

private:
    /// Map of swap_id to swap info
    std::unordered_map<uint256, SwapInfo, uint256_hash> swaps_;

    /// Swap event callback
    std::function<void(const SwapEvent&)> event_callback_;

    /// HTLC manager
    std::shared_ptr<htlc::HTLCManager> htlc_manager_;

    /// Update swap state
    void UpdateSwapState(const uint256& swap_id, SwapState new_state);

    /// Trigger swap event
    void TriggerEvent(SwapEventType type, const uint256& swap_id,
                     SwapState new_state, const std::string& message);

    /// Validate swap offer
    Result<void> ValidateSwapOffer(const SwapOffer& offer) const;

    /// Calculate swap ID from offer
    uint256 CalculateSwapID(const SwapOffer& offer) const;
};

}  // namespace atomic_swap
}  // namespace intcoin

#endif  // INTCOIN_ATOMIC_SWAP_H
