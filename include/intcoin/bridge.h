// Copyright (c) 2024-2026 The INTcoin Core developers
// Distributed under the MIT software license

#ifndef INTCOIN_BRIDGE_H
#define INTCOIN_BRIDGE_H

#include <intcoin/types.h>
#include <intcoin/blockchain_monitor.h>

#include <cstdint>
#include <string>
#include <vector>
#include <map>
#include <functional>
#include <memory>

namespace intcoin {
namespace bridge {

/// Supported bridge chains
enum class BridgeChain : uint8_t {
    INTCOIN = 0,
    BITCOIN = 1,
    ETHEREUM = 2,
    LITECOIN = 3,
    TESTNET_INT = 10,
    TESTNET_BTC = 11,
    TESTNET_ETH = 12,
    TESTNET_LTC = 13
};

/// Bridge transaction status
enum class BridgeStatus {
    PENDING,            // Submitted but not processed
    CONFIRMING,         // Waiting for confirmations
    VALIDATED,          // Validator signatures collected
    EXECUTED,           // Bridge transaction executed
    FAILED,             // Transaction failed
    EXPIRED             // Timeout expired
};

/// Bridge operation type
enum class BridgeOperation {
    DEPOSIT,            // Lock native tokens, mint wrapped
    WITHDRAW,           // Burn wrapped tokens, unlock native
    VALIDATOR_ADD,      // Add new validator
    VALIDATOR_REMOVE,   // Remove validator
    EMERGENCY_PAUSE,    // Emergency circuit breaker
    EMERGENCY_RESUME    // Resume after pause
};

/// Wrapped token metadata
struct WrappedToken {
    /// Token symbol (e.g., "wBTC", "wETH")
    std::string symbol;

    /// Origin chain where native token exists
    BridgeChain origin_chain;

    /// Token address on origin chain (for smart contract tokens)
    std::vector<uint8_t> origin_address;

    /// Decimals (18 for ETH, 8 for BTC, etc.)
    uint8_t decimals;

    /// Total supply of wrapped tokens
    uint64_t total_supply;

    /// Contract address on INTcoin (for tracking)
    uint256 contract_id;
};

/// Bridge deposit proof
struct DepositProof {
    /// Transaction hash on source chain
    uint256 source_tx_hash;

    /// Block number containing the deposit
    uint64_t block_number;

    /// Merkle proof (for SPV verification)
    std::vector<uint256> merkle_proof;

    /// Depositor address on source chain
    std::vector<uint8_t> depositor_address;

    /// Recipient address on INTcoin
    std::vector<uint8_t> recipient_address;

    /// Deposit amount
    uint64_t amount;

    /// Token type
    WrappedToken token;

    /// Validator signatures (M-of-N threshold)
    std::vector<std::vector<uint8_t>> validator_signatures;

    /// Timestamp of deposit
    uint64_t timestamp;
};

/// Bridge withdrawal request
struct WithdrawalRequest {
    /// Unique withdrawal ID
    uint256 withdrawal_id;

    /// Requester address on INTcoin
    std::vector<uint8_t> requester_address;

    /// Destination address on target chain
    std::vector<uint8_t> destination_address;

    /// Withdrawal amount
    uint64_t amount;

    /// Token to withdraw
    WrappedToken token;

    /// Fee for bridge validators
    uint64_t bridge_fee;

    /// Current status
    BridgeStatus status;

    /// Validator signatures collected
    std::vector<std::vector<uint8_t>> validator_signatures;

    /// Transaction hash on target chain (once executed)
    uint256 target_tx_hash;

    /// Timestamp of request
    uint64_t timestamp;

    /// Expiration time
    uint64_t expiration;
};

/// Bridge validator information
struct BridgeValidator {
    /// Validator public key (for signing)
    std::vector<uint8_t> public_key;

    /// Validator address (for rewards)
    std::vector<uint8_t> address;

    /// Stake amount (for security)
    uint64_t stake;

    /// Active status
    bool is_active;

    /// Join timestamp
    uint64_t joined_at;

    /// Validator reputation score
    uint32_t reputation;

    /// Number of signatures provided
    uint64_t signatures_count;
};

/// Bridge configuration
struct BridgeConfig {
    /// Minimum validators required (M in M-of-N)
    uint32_t min_validators;

    /// Total validators in set (N in M-of-N)
    uint32_t total_validators;

    /// Minimum confirmations on source chain
    uint32_t min_confirmations_btc;
    uint32_t min_confirmations_eth;
    uint32_t min_confirmations_ltc;

    /// Bridge fee percentage (basis points, e.g., 30 = 0.3%)
    uint32_t fee_basis_points;

    /// Emergency pause enabled
    bool emergency_paused;

    /// Minimum stake required for validators
    uint64_t min_validator_stake;

    /// Withdrawal timeout (seconds)
    uint64_t withdrawal_timeout;
};

/// Bridge event callback types
using DepositDetectedCallback = std::function<void(const DepositProof&)>;
using WithdrawalRequestedCallback = std::function<void(const WithdrawalRequest&)>;
using ValidatorSignedCallback = std::function<void(const uint256&, const std::vector<uint8_t>&)>;
using BridgeExecutedCallback = std::function<void(const uint256&, bool success)>;

/// Bridge contract interface
class BridgeContract {
public:
    virtual ~BridgeContract() = default;

    /// Initialize bridge
    virtual Result<void> Initialize(const BridgeConfig& config) = 0;

    /// Shutdown bridge
    virtual Result<void> Shutdown() = 0;

    // ========================================
    // Deposit Operations (Lock & Mint)
    // ========================================

    /// Submit deposit proof (validators call this)
    virtual Result<uint256> SubmitDepositProof(const DepositProof& proof) = 0;

    /// Verify deposit proof validity
    virtual Result<bool> VerifyDepositProof(const DepositProof& proof) = 0;

    /// Mint wrapped tokens (after proof validation)
    virtual Result<void> MintWrappedTokens(
        const uint256& proof_id,
        const std::vector<uint8_t>& recipient,
        uint64_t amount,
        const WrappedToken& token
    ) = 0;

    // ========================================
    // Withdrawal Operations (Burn & Unlock)
    // ========================================

    /// Request withdrawal (burn wrapped tokens)
    virtual Result<uint256> RequestWithdrawal(
        const std::vector<uint8_t>& destination,
        uint64_t amount,
        const WrappedToken& token,
        const std::vector<uint8_t>& requester_signature
    ) = 0;

    /// Sign withdrawal request (validators call this)
    virtual Result<void> SignWithdrawal(
        const uint256& withdrawal_id,
        const std::vector<uint8_t>& validator_signature
    ) = 0;

    /// Execute withdrawal on target chain (once threshold reached)
    virtual Result<uint256> ExecuteWithdrawal(const uint256& withdrawal_id) = 0;

    /// Get withdrawal request details
    virtual Result<WithdrawalRequest> GetWithdrawal(const uint256& withdrawal_id) = 0;

    // ========================================
    // Token Management
    // ========================================

    /// Register new wrapped token
    virtual Result<void> RegisterWrappedToken(const WrappedToken& token) = 0;

    /// Get wrapped token balance
    virtual Result<uint64_t> GetWrappedBalance(
        const std::vector<uint8_t>& address,
        const std::string& token_symbol
    ) = 0;

    /// Get total supply of wrapped token
    virtual Result<uint64_t> GetWrappedSupply(const std::string& token_symbol) = 0;

    /// Get all registered wrapped tokens
    virtual Result<std::vector<WrappedToken>> GetWrappedTokens() = 0;

    // ========================================
    // Validator Management
    // ========================================

    /// Add new validator
    virtual Result<void> AddValidator(const BridgeValidator& validator) = 0;

    /// Remove validator
    virtual Result<void> RemoveValidator(const std::vector<uint8_t>& validator_pubkey) = 0;

    /// Get all active validators
    virtual Result<std::vector<BridgeValidator>> GetValidators() = 0;

    /// Check if address is validator
    virtual Result<bool> IsValidator(const std::vector<uint8_t>& pubkey) = 0;

    // ========================================
    // Security & Monitoring
    // ========================================

    /// Emergency pause (stops all bridge operations)
    virtual Result<void> EmergencyPause() = 0;

    /// Resume after emergency pause
    virtual Result<void> EmergencyResume() = 0;

    /// Get bridge status
    virtual Result<bool> IsPaused() = 0;

    /// Get bridge configuration
    virtual Result<BridgeConfig> GetConfig() = 0;

    /// Update bridge configuration
    virtual Result<void> UpdateConfig(const BridgeConfig& config) = 0;

    // ========================================
    // Event Callbacks
    // ========================================

    /// Register callback for deposit detected
    virtual void OnDepositDetected(DepositDetectedCallback callback) = 0;

    /// Register callback for withdrawal requested
    virtual void OnWithdrawalRequested(WithdrawalRequestedCallback callback) = 0;

    /// Register callback for validator signed
    virtual void OnValidatorSigned(ValidatorSignedCallback callback) = 0;

    /// Register callback for bridge executed
    virtual void OnBridgeExecuted(BridgeExecutedCallback callback) = 0;
};

/// INTcoin bridge implementation
class INTcoinBridge : public BridgeContract {
public:
    INTcoinBridge();
    ~INTcoinBridge() override;

    Result<void> Initialize(const BridgeConfig& config) override;
    Result<void> Shutdown() override;

    Result<uint256> SubmitDepositProof(const DepositProof& proof) override;
    Result<bool> VerifyDepositProof(const DepositProof& proof) override;
    Result<void> MintWrappedTokens(const uint256& proof_id, const std::vector<uint8_t>& recipient,
                                   uint64_t amount, const WrappedToken& token) override;

    Result<uint256> RequestWithdrawal(const std::vector<uint8_t>& destination, uint64_t amount,
                                      const WrappedToken& token, const std::vector<uint8_t>& requester_signature) override;
    Result<void> SignWithdrawal(const uint256& withdrawal_id, const std::vector<uint8_t>& validator_signature) override;
    Result<uint256> ExecuteWithdrawal(const uint256& withdrawal_id) override;
    Result<WithdrawalRequest> GetWithdrawal(const uint256& withdrawal_id) override;

    Result<void> RegisterWrappedToken(const WrappedToken& token) override;
    Result<uint64_t> GetWrappedBalance(const std::vector<uint8_t>& address, const std::string& token_symbol) override;
    Result<uint64_t> GetWrappedSupply(const std::string& token_symbol) override;
    Result<std::vector<WrappedToken>> GetWrappedTokens() override;

    Result<void> AddValidator(const BridgeValidator& validator) override;
    Result<void> RemoveValidator(const std::vector<uint8_t>& validator_pubkey) override;
    Result<std::vector<BridgeValidator>> GetValidators() override;
    Result<bool> IsValidator(const std::vector<uint8_t>& pubkey) override;

    Result<void> EmergencyPause() override;
    Result<void> EmergencyResume() override;
    Result<bool> IsPaused() override;
    Result<BridgeConfig> GetConfig() override;
    Result<void> UpdateConfig(const BridgeConfig& config) override;

    void OnDepositDetected(DepositDetectedCallback callback) override;
    void OnWithdrawalRequested(WithdrawalRequestedCallback callback) override;
    void OnValidatorSigned(ValidatorSignedCallback callback) override;
    void OnBridgeExecuted(BridgeExecutedCallback callback) override;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

/// Bridge helper functions
std::string BridgeChainToString(BridgeChain chain);
std::string BridgeStatusToString(BridgeStatus status);
std::string BridgeOperationToString(BridgeOperation op);

}  // namespace bridge
}  // namespace intcoin

#endif  // INTCOIN_BRIDGE_H
