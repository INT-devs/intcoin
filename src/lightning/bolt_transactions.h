// Copyright (c) 2025 INTcoin Team (Neil Adamson)
// MIT License
// BOLT #3: Bitcoin Transaction and Script Formats (Adapted for INTcoin)
// BOLT #5: Recommendations for On-chain Transaction Handling

#ifndef INTCOIN_BOLT_TRANSACTIONS_H
#define INTCOIN_BOLT_TRANSACTIONS_H

#include "intcoin/types.h"
#include "intcoin/crypto.h"
#include "intcoin/transaction.h"
#include <vector>
#include <optional>

namespace intcoin {
namespace bolt {

// ============================================================================
// BOLT #3: Transaction and Script Formats
// ============================================================================

// Key derivation for commitment transactions
class CommitmentKeys {
public:
    PublicKey funding_pubkey;
    PublicKey revocation_basepoint;
    PublicKey payment_basepoint;
    PublicKey delayed_payment_basepoint;
    PublicKey htlc_basepoint;

    // Derive per-commitment keys
    static PublicKey DeriveRevocationPubkey(
        const PublicKey& revocation_basepoint,
        const PublicKey& per_commitment_point
    );

    static PublicKey DerivePaymentPubkey(
        const PublicKey& payment_basepoint,
        const PublicKey& per_commitment_point
    );

    static PublicKey DeriveDelayedPaymentPubkey(
        const PublicKey& delayed_payment_basepoint,
        const PublicKey& per_commitment_point
    );

    static PublicKey DeriveHTLCPubkey(
        const PublicKey& htlc_basepoint,
        const PublicKey& per_commitment_point
    );

    // Derive secret keys (for local keys only)
    static SecretKey DeriveRevocationPrivkey(
        const SecretKey& revocation_basepoint_secret,
        const uint256& per_commitment_secret
    );
};

// HTLC script types
enum class HTLCType {
    OFFERED,   // We offered the HTLC
    RECEIVED   // We received the HTLC
};

// HTLC transaction output
struct HTLCOutput {
    uint64_t amount;
    HTLCType htlc_type;
    uint256 payment_hash;
    uint32_t cltv_expiry;
    PublicKey local_htlc_key;
    PublicKey remote_htlc_key;
    PublicKey revocation_key;

    // Generate HTLC script
    Script CreateScript() const;

    // Output index in commitment transaction
    uint32_t output_index;
};

// Commitment transaction builder (BOLT #3)
class CommitmentTransactionBuilder {
public:
    CommitmentTransactionBuilder();

    // Set funding transaction
    CommitmentTransactionBuilder& WithFundingTxid(const uint256& txid, uint32_t vout);

    // Set commitment number
    CommitmentTransactionBuilder& WithCommitmentNumber(uint64_t num);

    // Set balances
    CommitmentTransactionBuilder& WithLocalBalance(uint64_t amount);
    CommitmentTransactionBuilder& WithRemoteBalance(uint64_t amount);

    // Set keys
    CommitmentTransactionBuilder& WithLocalKeys(const CommitmentKeys& keys);
    CommitmentTransactionBuilder& WithRemoteKeys(const CommitmentKeys& keys);

    // Set per-commitment point
    CommitmentTransactionBuilder& WithPerCommitmentPoint(const PublicKey& point);

    // Add HTLC
    CommitmentTransactionBuilder& AddHTLC(const HTLCOutput& htlc);

    // Set fee
    CommitmentTransactionBuilder& WithFee(uint64_t fee_satoshis);

    // Set CSV delay
    CommitmentTransactionBuilder& WithToSelfDelay(uint16_t delay);

    // Build commitment transaction
    Result<Transaction> Build();

    // Build commitment transaction and extract HTLC outputs
    Result<std::pair<Transaction, std::vector<HTLCOutput>>> BuildWithHTLCs();

private:
    uint256 funding_txid_;
    uint32_t funding_vout_;
    uint64_t commitment_number_;
    uint64_t local_balance_;
    uint64_t remote_balance_;
    CommitmentKeys local_keys_;
    CommitmentKeys remote_keys_;
    PublicKey per_commitment_point_;
    std::vector<HTLCOutput> htlcs_;
    uint64_t fee_;
    uint16_t to_self_delay_;

    // Generate obscured commitment number
    uint64_t ObscureCommitmentNumber(
        const PublicKey& local_payment_basepoint,
        const PublicKey& remote_payment_basepoint,
        uint64_t commitment_number
    );

    // Create to_local output script
    Script CreateToLocalScript(
        const PublicKey& revocation_key,
        uint16_t to_self_delay,
        const PublicKey& delayed_payment_key
    );

    // Create to_remote output script
    Script CreateToRemoteScript(const PublicKey& remote_payment_key);

    // Sort outputs (BIP69 lexicographic ordering)
    void SortOutputs(Transaction& tx, std::vector<HTLCOutput>& htlcs);
};

// HTLC transaction builder (HTLC-timeout and HTLC-success)
class HTLCTransactionBuilder {
public:
    enum class HTLCTxType {
        TIMEOUT,   // HTLC-timeout (for offered HTLCs)
        SUCCESS    // HTLC-success (for received HTLCs)
    };

    HTLCTransactionBuilder(HTLCTxType type);

    // Set commitment transaction
    HTLCTransactionBuilder& WithCommitmentTxid(const uint256& txid);

    // Set HTLC output
    HTLCTransactionBuilder& WithHTLCOutput(const HTLCOutput& htlc);

    // Set per-commitment point
    HTLCTransactionBuilder& WithPerCommitmentPoint(const PublicKey& point);

    // Set feerate
    HTLCTransactionBuilder& WithFeeRate(uint64_t fee_satoshis);

    // Set payment preimage (for HTLC-success only)
    HTLCTransactionBuilder& WithPaymentPreimage(const uint256& preimage);

    // Build HTLC transaction
    Result<Transaction> Build();

private:
    HTLCTxType type_;
    uint256 commitment_txid_;
    HTLCOutput htlc_;
    PublicKey per_commitment_point_;
    uint64_t fee_;
    std::optional<uint256> payment_preimage_;

    // Create HTLC-timeout script
    Script CreateHTLCTimeoutWitness(
        const Signature& local_sig,
        const Signature& remote_sig
    );

    // Create HTLC-success script
    Script CreateHTLCSuccessWitness(
        const uint256& payment_preimage,
        const Signature& local_sig,
        const Signature& remote_sig
    );
};

// ============================================================================
// BOLT #5: On-chain Transaction Handling
// ============================================================================

// Channel closure types
enum class ClosureType {
    MUTUAL,             // Cooperative close
    LOCAL_FORCE,        // Local force close
    REMOTE_FORCE,       // Remote force close
    REVOKED_REMOTE      // Remote published revoked commitment
};

// On-chain transaction monitor
class OnChainMonitor {
public:
    OnChainMonitor();

    // Monitor commitment transaction
    void MonitorCommitment(
        const uint256& channel_id,
        const Transaction& commitment_tx,
        uint64_t commitment_number,
        const std::vector<HTLCOutput>& htlcs
    );

    // Monitor HTLC transactions
    void MonitorHTLC(
        const uint256& channel_id,
        const Transaction& htlc_tx,
        HTLCType htlc_type
    );

    // Detect channel closure
    struct ClosureDetection {
        ClosureType type;
        uint256 closing_txid;
        uint64_t block_height;
        std::vector<HTLCOutput> pending_htlcs;
    };

    Result<ClosureDetection> DetectClosure(
        const uint256& channel_id,
        const std::vector<Transaction>& recent_blocks
    );

    // Handle revoked commitment broadcast
    Result<Transaction> CreateJusticeTx(
        const uint256& channel_id,
        const uint256& revoked_commitment_txid,
        const uint256& revocation_secret
    );

private:
    struct MonitoredChannel {
        uint256 channel_id;
        std::vector<Transaction> commitment_txs;
        std::vector<uint64_t> commitment_numbers;
        std::map<uint256, std::vector<HTLCOutput>> htlc_outputs;
    };

    std::map<uint256, MonitoredChannel> monitored_channels_;
};

// Penalty transaction builder (for revoked commitments)
class PenaltyTransactionBuilder {
public:
    PenaltyTransactionBuilder();

    // Set revoked commitment transaction
    PenaltyTransactionBuilder& WithRevokedCommitment(const Transaction& tx);

    // Set revocation secret
    PenaltyTransactionBuilder& WithRevocationSecret(const uint256& secret);

    // Set destination address
    PenaltyTransactionBuilder& WithDestination(const std::vector<uint8_t>& script_pubkey);

    // Set feerate
    PenaltyTransactionBuilder& WithFeeRate(uint64_t fee_satoshis);

    // Build penalty (justice) transaction
    Result<Transaction> Build();

private:
    Transaction revoked_commitment_;
    uint256 revocation_secret_;
    std::vector<uint8_t> destination_script_;
    uint64_t fee_;
};

// Mutual close transaction builder
class MutualCloseTransactionBuilder {
public:
    MutualCloseTransactionBuilder();

    // Set funding transaction
    MutualCloseTransactionBuilder& WithFundingTxid(const uint256& txid, uint32_t vout);

    // Set final balances
    MutualCloseTransactionBuilder& WithLocalBalance(uint64_t amount);
    MutualCloseTransactionBuilder& WithRemoteBalance(uint64_t amount);

    // Set destination scripts
    MutualCloseTransactionBuilder& WithLocalScript(const std::vector<uint8_t>& script);
    MutualCloseTransactionBuilder& WithRemoteScript(const std::vector<uint8_t>& script);

    // Set fee
    MutualCloseTransactionBuilder& WithFee(uint64_t fee_satoshis);

    // Build closing transaction
    Result<Transaction> Build();

private:
    uint256 funding_txid_;
    uint32_t funding_vout_;
    uint64_t local_balance_;
    uint64_t remote_balance_;
    std::vector<uint8_t> local_script_;
    std::vector<uint8_t> remote_script_;
    uint64_t fee_;
};

// ============================================================================
// BOLT #10: DNS Bootstrap and Assisted Node Location
// ============================================================================

// DNS seed record
struct DNSSeedRecord {
    std::string hostname;          // DNS hostname
    std::string ip_address;        // Resolved IP address
    uint16_t port;                 // Lightning port
    PublicKey node_id;             // Node public key (if known)
    std::vector<uint8_t> features; // Node features
};

// DNS bootstrap
class DNSBootstrap {
public:
    DNSBootstrap();

    // Add DNS seed
    void AddSeed(const std::string& dns_seed);

    // Query DNS seeds for nodes
    Result<std::vector<DNSSeedRecord>> QuerySeeds(bool testnet = false);

    // Parse SRV record
    static Result<std::vector<DNSSeedRecord>> ParseSRVRecords(
        const std::string& domain,
        const std::vector<std::string>& srv_records
    );

    // Default DNS seeds for INTcoin Lightning Network
    static std::vector<std::string> GetDefaultSeeds(bool testnet);

private:
    std::vector<std::string> dns_seeds_;

    // Resolve DNS record
    Result<std::vector<std::string>> ResolveDNS(const std::string& hostname);
};

} // namespace bolt
} // namespace intcoin

#endif // INTCOIN_BOLT_TRANSACTIONS_H
