// Copyright (c) 2024-2026 The INTcoin Core developers
// Distributed under the MIT software license

#ifndef INTCOIN_BLOCKCHAIN_MONITOR_H
#define INTCOIN_BLOCKCHAIN_MONITOR_H

#include <intcoin/types.h>
#include <intcoin/atomic_swap.h>

#include <cstdint>
#include <string>
#include <vector>
#include <functional>
#include <memory>

namespace intcoin {
namespace blockchain_monitor {

/// Blockchain type for monitoring
enum class BlockchainType {
    INTCOIN,
    BITCOIN,
    LITECOIN,
    TESTNET_INT,
    TESTNET_BTC,
    TESTNET_LTC
};

/// Transaction status
enum class TxStatus {
    NOT_FOUND,      // Transaction not found on blockchain
    PENDING,        // In mempool, not confirmed
    CONFIRMING,     // Has confirmations but below threshold
    CONFIRMED,      // Fully confirmed
    SPENT,          // Output has been spent
    FAILED          // Transaction failed/invalid
};

/// HTLC transaction information
struct HTLCTransaction {
    /// Transaction hash
    uint256 tx_hash;

    /// Output index containing HTLC
    uint32_t output_index;

    /// HTLC amount (in ints)
    uint64_t amount;

    /// Number of confirmations
    uint32_t confirmations;

    /// Block height (0 if unconfirmed)
    uint64_t block_height;

    /// Transaction status
    TxStatus status;

    /// Raw transaction hex (for parsing)
    std::string raw_tx_hex;

    /// HTLC script (extracted from output)
    std::vector<uint8_t> htlc_script;

    /// Payment hash (extracted from script)
    std::vector<uint8_t> payment_hash;

    /// Locktime (extracted from script)
    uint64_t locktime;

    /// Whether this HTLC has been claimed
    bool claimed;

    /// Whether this HTLC has been refunded
    bool refunded;

    /// Preimage (if claimed and revealed)
    std::vector<uint8_t> preimage;

    /// Claiming transaction hash (if claimed)
    uint256 claim_tx_hash;

    /// Refund transaction hash (if refunded)
    uint256 refund_tx_hash;
};

/// Blockchain monitor callback types
using HTLCDetectedCallback = std::function<void(const HTLCTransaction&)>;
using HTLCConfirmedCallback = std::function<void(const HTLCTransaction&)>;
using HTLCClaimedCallback = std::function<void(const HTLCTransaction&, const std::vector<uint8_t>& preimage)>;
using HTLCRefundedCallback = std::function<void(const HTLCTransaction&)>;

/// Blockchain monitor interface
class BlockchainMonitor {
public:
    virtual ~BlockchainMonitor() = default;

    /// Start monitoring
    virtual Result<void> Start() = 0;

    /// Stop monitoring
    virtual Result<void> Stop() = 0;

    /// Check if monitoring is active
    virtual bool IsActive() const = 0;

    /// Get blockchain type
    virtual BlockchainType GetBlockchainType() const = 0;

    /// Get current block height
    virtual Result<uint64_t> GetCurrentBlockHeight() = 0;

    /// Get current block hash
    virtual Result<uint256> GetCurrentBlockHash() = 0;

    // ========================================
    // Transaction Monitoring
    // ========================================

    /// Watch for HTLC funding transaction
    /// @param payment_hash Payment hash to watch for
    /// @param recipient_pubkey Recipient public key
    /// @param refund_pubkey Refund public key
    /// @param locktime Locktime value
    /// @return Success/failure
    virtual Result<void> WatchForHTLC(
        const std::vector<uint8_t>& payment_hash,
        const std::vector<uint8_t>& recipient_pubkey,
        const std::vector<uint8_t>& refund_pubkey,
        uint64_t locktime
    ) = 0;

    /// Stop watching for a specific HTLC
    /// @param payment_hash Payment hash to stop watching
    virtual Result<void> StopWatchingHTLC(const std::vector<uint8_t>& payment_hash) = 0;

    /// Get HTLC transaction information
    /// @param tx_hash Transaction hash
    /// @param output_index Output index
    /// @return HTLC transaction info
    virtual Result<HTLCTransaction> GetHTLCTransaction(
        const uint256& tx_hash,
        uint32_t output_index
    ) = 0;

    /// Check confirmations for a transaction
    /// @param tx_hash Transaction hash
    /// @return Number of confirmations
    virtual Result<uint32_t> GetConfirmations(const uint256& tx_hash) = 0;

    /// Watch for preimage revelation (claim transaction)
    /// @param htlc_tx_hash HTLC funding transaction hash
    /// @param htlc_output_index HTLC output index
    /// @return Preimage if found, error otherwise
    virtual Result<std::vector<uint8_t>> WatchForPreimage(
        const uint256& htlc_tx_hash,
        uint32_t htlc_output_index
    ) = 0;

    /// Check if HTLC has been spent (claimed or refunded)
    /// @param htlc_tx_hash HTLC transaction hash
    /// @param htlc_output_index HTLC output index
    /// @return True if spent
    virtual Result<bool> IsHTLCSpent(
        const uint256& htlc_tx_hash,
        uint32_t htlc_output_index
    ) = 0;

    // ========================================
    // Transaction Broadcasting
    // ========================================

    /// Broadcast raw transaction
    /// @param raw_tx_hex Raw transaction in hex format
    /// @return Transaction hash
    virtual Result<uint256> BroadcastTransaction(const std::string& raw_tx_hex) = 0;

    // ========================================
    // Event Callbacks
    // ========================================

    /// Register callback for HTLC detected
    virtual void OnHTLCDetected(HTLCDetectedCallback callback) = 0;

    /// Register callback for HTLC confirmed
    virtual void OnHTLCConfirmed(HTLCConfirmedCallback callback) = 0;

    /// Register callback for HTLC claimed
    virtual void OnHTLCClaimed(HTLCClaimedCallback callback) = 0;

    /// Register callback for HTLC refunded
    virtual void OnHTLCRefunded(HTLCRefundedCallback callback) = 0;
};

/// Bitcoin blockchain monitor
class BitcoinMonitor : public BlockchainMonitor {
public:
    /// Constructor
    /// @param rpc_url Bitcoin RPC URL (e.g., "http://localhost:8332")
    /// @param rpc_user RPC username
    /// @param rpc_password RPC password
    /// @param testnet Use testnet instead of mainnet
    BitcoinMonitor(const std::string& rpc_url,
                   const std::string& rpc_user,
                   const std::string& rpc_password,
                   bool testnet = false);

    ~BitcoinMonitor() override;

    Result<void> Start() override;
    Result<void> Stop() override;
    bool IsActive() const override;
    BlockchainType GetBlockchainType() const override;

    Result<uint64_t> GetCurrentBlockHeight() override;
    Result<uint256> GetCurrentBlockHash() override;

    Result<void> WatchForHTLC(
        const std::vector<uint8_t>& payment_hash,
        const std::vector<uint8_t>& recipient_pubkey,
        const std::vector<uint8_t>& refund_pubkey,
        uint64_t locktime
    ) override;

    Result<void> StopWatchingHTLC(const std::vector<uint8_t>& payment_hash) override;

    Result<HTLCTransaction> GetHTLCTransaction(
        const uint256& tx_hash,
        uint32_t output_index
    ) override;

    Result<uint32_t> GetConfirmations(const uint256& tx_hash) override;

    Result<std::vector<uint8_t>> WatchForPreimage(
        const uint256& htlc_tx_hash,
        uint32_t htlc_output_index
    ) override;

    Result<bool> IsHTLCSpent(
        const uint256& htlc_tx_hash,
        uint32_t htlc_output_index
    ) override;

    Result<uint256> BroadcastTransaction(const std::string& raw_tx_hex) override;

    void OnHTLCDetected(HTLCDetectedCallback callback) override;
    void OnHTLCConfirmed(HTLCConfirmedCallback callback) override;
    void OnHTLCClaimed(HTLCClaimedCallback callback) override;
    void OnHTLCRefunded(HTLCRefundedCallback callback) override;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

/// Litecoin blockchain monitor
class LitecoinMonitor : public BlockchainMonitor {
public:
    /// Constructor
    /// @param rpc_url Litecoin RPC URL
    /// @param rpc_user RPC username
    /// @param rpc_password RPC password
    /// @param testnet Use testnet instead of mainnet
    LitecoinMonitor(const std::string& rpc_url,
                    const std::string& rpc_user,
                    const std::string& rpc_password,
                    bool testnet = false);

    ~LitecoinMonitor() override;

    Result<void> Start() override;
    Result<void> Stop() override;
    bool IsActive() const override;
    BlockchainType GetBlockchainType() const override;

    Result<uint64_t> GetCurrentBlockHeight() override;
    Result<uint256> GetCurrentBlockHash() override;

    Result<void> WatchForHTLC(
        const std::vector<uint8_t>& payment_hash,
        const std::vector<uint8_t>& recipient_pubkey,
        const std::vector<uint8_t>& refund_pubkey,
        uint64_t locktime
    ) override;

    Result<void> StopWatchingHTLC(const std::vector<uint8_t>& payment_hash) override;

    Result<HTLCTransaction> GetHTLCTransaction(
        const uint256& tx_hash,
        uint32_t output_index
    ) override;

    Result<uint32_t> GetConfirmations(const uint256& tx_hash) override;

    Result<std::vector<uint8_t>> WatchForPreimage(
        const uint256& htlc_tx_hash,
        uint32_t htlc_output_index
    ) override;

    Result<bool> IsHTLCSpent(
        const uint256& htlc_tx_hash,
        uint32_t htlc_output_index
    ) override;

    Result<uint256> BroadcastTransaction(const std::string& raw_tx_hex) override;

    void OnHTLCDetected(HTLCDetectedCallback callback) override;
    void OnHTLCConfirmed(HTLCConfirmedCallback callback) override;
    void OnHTLCClaimed(HTLCClaimedCallback callback) override;
    void OnHTLCRefunded(HTLCRefundedCallback callback) override;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

/// Factory function to create blockchain monitor
/// @param type Blockchain type
/// @param rpc_url RPC URL
/// @param rpc_user RPC username
/// @param rpc_password RPC password
/// @return Blockchain monitor instance
std::unique_ptr<BlockchainMonitor> CreateBlockchainMonitor(
    BlockchainType type,
    const std::string& rpc_url,
    const std::string& rpc_user,
    const std::string& rpc_password
);

}  // namespace blockchain_monitor
}  // namespace intcoin

#endif  // INTCOIN_BLOCKCHAIN_MONITOR_H
