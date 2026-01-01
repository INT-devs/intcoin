// Copyright (c) 2024-2025 The INTcoin Core developers
// Distributed under the MIT software license

#ifndef INTCOIN_HTLC_H
#define INTCOIN_HTLC_H

#include <intcoin/script.h>
#include <intcoin/transaction.h>
#include <intcoin/types.h>

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace intcoin {
namespace htlc {

/// HTLC (Hash Time-Locked Contract) implementation for atomic swaps
/// Enables trustless cross-chain cryptocurrency exchanges using hash locks and time locks

/// Hash algorithm used for HTLC
enum class HTLCHashAlgorithm : uint8_t {
    SHA3_256 = 0,  // SHA3-256 (INTcoin native)
    SHA256 = 1,    // SHA-256 (Bitcoin compatible)
    RIPEMD160 = 2  // RIPEMD-160 (Bitcoin compatible)
};

/// HTLC parameters
struct HTLCParameters {
    /// Recipient's public key (claims with preimage)
    std::vector<uint8_t> recipient_pubkey;

    /// Refund public key (refunds after timeout)
    std::vector<uint8_t> refund_pubkey;

    /// Hash of the secret (32 bytes for SHA3-256/SHA-256, 20 bytes for RIPEMD160)
    std::vector<uint8_t> hash_lock;

    /// Locktime (Unix timestamp or block height)
    uint64_t locktime;

    /// Hash algorithm to use
    HTLCHashAlgorithm hash_algorithm;

    /// Whether locktime is block height (true) or timestamp (false)
    bool is_block_height;
};

/// HTLC script builder
class HTLCScript {
public:
    /// Create HTLC script
    /// @param params HTLC parameters
    /// @return Script for HTLC output
    static Script CreateHTLCScript(const HTLCParameters& params);

    /// Create witness script for HTLC claim (with preimage)
    /// @param preimage Secret preimage
    /// @param signature Recipient's signature
    /// @return Witness script to claim HTLC
    static Script CreateClaimWitness(const std::vector<uint8_t>& preimage,
                                    const std::vector<uint8_t>& signature);

    /// Create witness script for HTLC refund (after timeout)
    /// @param signature Refund key's signature
    /// @return Witness script to refund HTLC
    static Script CreateRefundWitness(const std::vector<uint8_t>& signature);

    /// Verify HTLC script format
    /// @param script Script to verify
    /// @return True if valid HTLC script
    static bool IsHTLCScript(const Script& script);

    /// Extract HTLC parameters from script
    /// @param script HTLC script
    /// @return HTLC parameters if valid, error otherwise
    static Result<HTLCParameters> ExtractHTLCParameters(const Script& script);

    /// Generate payment hash from preimage
    /// @param preimage Secret preimage
    /// @param algorithm Hash algorithm
    /// @return Payment hash
    static std::vector<uint8_t> HashPreimage(const std::vector<uint8_t>& preimage,
                                             HTLCHashAlgorithm algorithm = HTLCHashAlgorithm::SHA3_256);

    /// Verify preimage matches hash
    /// @param preimage Secret preimage
    /// @param hash Expected hash
    /// @param algorithm Hash algorithm
    /// @return True if preimage is valid
    static bool VerifyPreimage(const std::vector<uint8_t>& preimage,
                              const std::vector<uint8_t>& hash,
                              HTLCHashAlgorithm algorithm = HTLCHashAlgorithm::SHA3_256);
};

/// HTLC transaction builder
class HTLCTransactionBuilder {
public:
    /// Constructor
    HTLCTransactionBuilder();

    /// Create HTLC funding transaction
    /// @param inputs Transaction inputs (UTXOs to fund HTLC)
    /// @param htlc_params HTLC parameters
    /// @param amount Amount to lock in HTLC (in INTS)
    /// @param change_address Address for change output
    /// @param fee_rate Fee rate in INTS per KB
    /// @return Unsigned HTLC funding transaction
    Result<Transaction> CreateFundingTransaction(
        const std::vector<TxIn>& inputs,
        const HTLCParameters& htlc_params,
        uint64_t amount,
        const std::string& change_address,
        uint64_t fee_rate = 2000
    );

    /// Create HTLC claim transaction (with preimage)
    /// @param htlc_outpoint HTLC output to claim
    /// @param htlc_amount HTLC amount
    /// @param htlc_script HTLC script
    /// @param preimage Secret preimage
    /// @param recipient_address Address to send claimed funds
    /// @param fee_rate Fee rate in INTS per KB
    /// @return Unsigned HTLC claim transaction
    Result<Transaction> CreateClaimTransaction(
        const OutPoint& htlc_outpoint,
        uint64_t htlc_amount,
        const Script& htlc_script,
        const std::vector<uint8_t>& preimage,
        const std::string& recipient_address,
        uint64_t fee_rate = 2000
    );

    /// Create HTLC refund transaction (after timeout)
    /// @param htlc_outpoint HTLC output to refund
    /// @param htlc_amount HTLC amount
    /// @param htlc_script HTLC script
    /// @param refund_address Address to refund to
    /// @param locktime Locktime for transaction (must match HTLC)
    /// @param fee_rate Fee rate in INTS per KB
    /// @return Unsigned HTLC refund transaction
    Result<Transaction> CreateRefundTransaction(
        const OutPoint& htlc_outpoint,
        uint64_t htlc_amount,
        const Script& htlc_script,
        const std::string& refund_address,
        uint64_t locktime,
        uint64_t fee_rate = 2000
    );

    /// Estimate HTLC transaction size
    /// @param num_inputs Number of inputs
    /// @param num_outputs Number of outputs
    /// @param is_claim True for claim, false for refund
    /// @return Estimated size in bytes
    static size_t EstimateHTLCTransactionSize(size_t num_inputs,
                                               size_t num_outputs,
                                               bool is_claim);

    /// Calculate HTLC transaction fee
    /// @param tx_size Transaction size in bytes
    /// @param fee_rate Fee rate in INTS per KB
    /// @return Fee in INTS
    static uint64_t CalculateFee(size_t tx_size, uint64_t fee_rate);
};

/// HTLC state for tracking
enum class HTLCState {
    PENDING,     // HTLC created, waiting for funding
    FUNDED,      // HTLC funded, waiting for claim or timeout
    CLAIMED,     // HTLC claimed with preimage
    REFUNDED,    // HTLC refunded after timeout
    EXPIRED      // HTLC expired without claim or refund
};

/// HTLC information
struct HTLCInfo {
    /// HTLC outpoint
    OutPoint outpoint;

    /// HTLC parameters
    HTLCParameters params;

    /// HTLC amount in INTS
    uint64_t amount;

    /// HTLC state
    HTLCState state;

    /// Block height when HTLC was created
    uint64_t creation_height;

    /// Block height when HTLC was claimed/refunded (0 if pending)
    uint64_t settlement_height;

    /// Transaction hash that settled HTLC (empty if pending)
    uint256 settlement_tx_hash;

    /// Preimage if claimed (empty if not revealed)
    std::vector<uint8_t> preimage;
};

/// HTLC manager for tracking HTLCs
class HTLCManager {
public:
    /// Constructor
    HTLCManager();

    /// Add HTLC to tracking
    /// @param info HTLC information
    void AddHTLC(const HTLCInfo& info);

    /// Update HTLC state
    /// @param outpoint HTLC outpoint
    /// @param state New state
    void UpdateHTLCState(const OutPoint& outpoint, HTLCState state);

    /// Get HTLC information
    /// @param outpoint HTLC outpoint
    /// @return HTLC info if found
    Result<HTLCInfo> GetHTLC(const OutPoint& outpoint) const;

    /// Get all HTLCs
    /// @return Vector of all tracked HTLCs
    std::vector<HTLCInfo> GetAllHTLCs() const;

    /// Get HTLCs by state
    /// @param state HTLC state to filter
    /// @return Vector of HTLCs with specified state
    std::vector<HTLCInfo> GetHTLCsByState(HTLCState state) const;

    /// Check for expired HTLCs
    /// @param current_height Current block height
    /// @param current_time Current timestamp
    /// @return Vector of expired HTLCs
    std::vector<HTLCInfo> GetExpiredHTLCs(uint64_t current_height,
                                         uint64_t current_time) const;

    /// Remove HTLC from tracking
    /// @param outpoint HTLC outpoint
    void RemoveHTLC(const OutPoint& outpoint);

    /// Get HTLC count
    /// @return Number of tracked HTLCs
    size_t GetHTLCCount() const;

    /// Get HTLC count by state
    /// @param state HTLC state
    /// @return Number of HTLCs in specified state
    size_t GetHTLCCountByState(HTLCState state) const;

private:
    /// Map of outpoint to HTLC info
    std::unordered_map<OutPoint, HTLCInfo, OutPointHash> htlcs_;
};

}  // namespace htlc
}  // namespace intcoin

#endif  // INTCOIN_HTLC_H
