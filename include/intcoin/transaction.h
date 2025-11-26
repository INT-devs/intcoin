/*
 * Copyright (c) 2025 INTcoin Team (Neil Adamson)
 * SPDX-License-Identifier: MIT License
 * Transaction Structure and Operations
 */

#ifndef INTCOIN_TRANSACTION_H
#define INTCOIN_TRANSACTION_H

#include "types.h"
#include "script.h"
#include <vector>
#include <optional>

namespace intcoin {

// ============================================================================
// Transaction Input (TxIn)
// ============================================================================

struct TxIn {
    /// Previous transaction hash
    uint256 prev_tx_hash;

    /// Previous transaction output index
    uint32_t prev_tx_index;

    /// Signature script (unlocking script)
    Script script_sig;

    /// Sequence number (for timelocks)
    uint32_t sequence;

    /// Default constructor
    TxIn() : prev_tx_index(0), sequence(0xFFFFFFFF) {}

    /// Serialize to bytes
    std::vector<uint8_t> Serialize() const;

    /// Deserialize from bytes
    static Result<TxIn> Deserialize(const std::vector<uint8_t>& data);

    /// Get serialized size
    size_t GetSerializedSize() const;
};

// ============================================================================
// Transaction Output (TxOut)
// ============================================================================

struct TxOut {
    /// Value in INTS (1 INT = 1,000,000 INTS)
    uint64_t value;

    /// Public key script (locking script)
    Script script_pubkey;

    /// Default constructor
    TxOut() : value(0) {}

    /// Constructor with value and script
    TxOut(uint64_t val, Script script) : value(val), script_pubkey(std::move(script)) {}

    /// Serialize to bytes
    std::vector<uint8_t> Serialize() const;

    /// Deserialize from bytes
    static Result<TxOut> Deserialize(const std::vector<uint8_t>& data);

    /// Get serialized size
    size_t GetSerializedSize() const;
};

// ============================================================================
// OutPoint (Transaction Output Reference)
// ============================================================================

struct OutPoint {
    /// Transaction hash
    uint256 tx_hash;

    /// Output index
    uint32_t index;

    /// Default constructor
    OutPoint() : index(0) {}

    /// Constructor
    OutPoint(uint256 hash, uint32_t idx) : tx_hash(hash), index(idx) {}

    /// Equality comparison
    bool operator==(const OutPoint& other) const;

    /// Less-than comparison (for ordering)
    bool operator<(const OutPoint& other) const;

    /// Serialize to bytes
    std::vector<uint8_t> Serialize() const;

    /// Deserialize from bytes
    static Result<OutPoint> Deserialize(const std::vector<uint8_t>& data);
};

/// Hash function for OutPoint (for use in unordered containers)
struct OutPointHash {
    std::size_t operator()(const OutPoint& outpoint) const noexcept;
};

// ============================================================================
// Transaction
// ============================================================================

class Transaction {
public:
    /// Transaction version
    uint32_t version;

    /// Transaction inputs
    std::vector<TxIn> inputs;

    /// Transaction outputs
    std::vector<TxOut> outputs;

    /// Lock time (block height or Unix time)
    uint64_t locktime;

    /// Quantum-resistant signature (Dilithium3)
    Signature signature;

    /// Default constructor
    Transaction() : version(1), locktime(0) {}

    /// Get transaction hash (txid)
    uint256 GetHash() const;

    /// Calculate transaction hash without signature
    uint256 GetHashForSigning() const;

    /// Sign transaction with private key
    Result<void> Sign(const SecretKey& secret_key);

    /// Verify transaction signature
    Result<void> VerifySignature(const PublicKey& public_key) const;

    /// Check if this is a coinbase transaction
    bool IsCoinbase() const;

    /// Get total input value (requires UTXO set)
    uint64_t GetTotalInputValue(const class UTXOSet& utxo_set) const;

    /// Get total output value
    uint64_t GetTotalOutputValue() const;

    /// Calculate transaction fee
    uint64_t GetFee(const class UTXOSet& utxo_set) const;

    /// Verify transaction structure
    Result<void> VerifyStructure() const;

    /// Verify transaction against UTXO set
    Result<void> VerifyAgainstUTXO(const class UTXOSet& utxo_set) const;

    /// Serialize to bytes
    std::vector<uint8_t> Serialize() const;

    /// Deserialize from bytes
    static Result<Transaction> Deserialize(const std::vector<uint8_t>& data);

    /// Get serialized size
    size_t GetSerializedSize() const;

private:
    mutable std::optional<uint256> cached_hash_;
};

// ============================================================================
// Transaction Builder
// ============================================================================

class TransactionBuilder {
public:
    /// Add input
    TransactionBuilder& AddInput(const OutPoint& outpoint, const Script& script_sig);

    /// Add output
    TransactionBuilder& AddOutput(uint64_t value, const Script& script_pubkey);

    /// Set locktime
    TransactionBuilder& SetLocktime(uint64_t locktime);

    /// Build and sign transaction
    Result<Transaction> Build(const SecretKey& secret_key);

private:
    Transaction tx_;
};

// ============================================================================
// Coinbase Transaction
// ============================================================================

/// Create coinbase transaction
Transaction CreateCoinbaseTransaction(uint64_t height, uint64_t block_reward,
                                     const PublicKey& miner_pubkey);

} // namespace intcoin

#endif // INTCOIN_TRANSACTION_H
