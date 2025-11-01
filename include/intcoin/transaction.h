// Copyright (c) 2025 INTcoin Core (Maddison Lane)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
//
// Transaction structures and UTXO model.

#ifndef INTCOIN_TRANSACTION_H
#define INTCOIN_TRANSACTION_H

#include "primitives.h"
#include <vector>
#include <string>
#include <optional>

namespace intcoin {

/**
 * Transaction output point
 * References a specific output in a transaction
 */
struct OutPoint {
    Hash256 tx_hash;       // Transaction hash
    uint32_t index;        // Output index

    OutPoint() : tx_hash{}, index(0) {}

    OutPoint(const Hash256& hash, uint32_t idx)
        : tx_hash(hash), index(idx) {}

    bool operator==(const OutPoint& other) const {
        return tx_hash == other.tx_hash && index == other.index;
    }

    bool operator!=(const OutPoint& other) const {
        return !(*this == other);
    }

    // Serialize for hashing
    std::vector<uint8_t> serialize() const;
};

/**
 * Transaction input
 * Spends a previous output
 */
struct TxInput {
    OutPoint previous_output;           // Output being spent
    std::vector<uint8_t> script_sig;    // Signature script
    DilithiumSignature signature;       // Quantum-resistant signature
    uint32_t sequence;                  // Sequence number (for relative lock time)

    TxInput()
        : previous_output()
        , script_sig()
        , signature{}
        , sequence(0xFFFFFFFF)
    {}

    // Serialize for hashing
    std::vector<uint8_t> serialize() const;

    // Check if this is a coinbase input
    bool is_coinbase() const {
        return previous_output.tx_hash == Hash256{} && previous_output.index == 0xFFFFFFFF;
    }
};

/**
 * Transaction output
 * Defines amount and spending conditions
 */
struct TxOutput {
    uint64_t value;                     // Amount in base units
    std::vector<uint8_t> script_pubkey; // Public key script
    DilithiumPubKey pubkey;             // Recipient's public key (quantum-resistant)

    TxOutput()
        : value(0)
        , script_pubkey()
        , pubkey{}
    {}

    TxOutput(uint64_t val, const std::vector<uint8_t>& script, const DilithiumPubKey& pk)
        : value(val), script_pubkey(script), pubkey(pk) {}

    // Serialize for hashing
    std::vector<uint8_t> serialize() const;

    // Check if output is dust (too small to be economical)
    bool is_dust() const {
        return value < 1000;  // Minimum 1000 base units
    }
};

/**
 * Transaction
 * Transfers value from inputs to outputs
 */
class Transaction {
public:
    uint32_t version;                   // Transaction version
    std::vector<TxInput> inputs;        // Transaction inputs
    std::vector<TxOutput> outputs;      // Transaction outputs
    uint32_t lock_time;                 // Lock time (0 = no lock)
    uint64_t timestamp;                 // Transaction timestamp

    Transaction()
        : version(1)
        , inputs()
        , outputs()
        , lock_time(0)
        , timestamp(0)
    {}

    // Serialize transaction
    std::vector<uint8_t> serialize() const;

    // Deserialize from bytes
    static Transaction deserialize(const std::vector<uint8_t>& data);

    // Calculate transaction hash
    Hash256 get_hash() const;

    // Get transaction ID (hex string)
    std::string get_txid() const;

    // Get transaction size in bytes
    size_t get_size() const;

    // Get transaction weight
    size_t get_weight() const;

    // Check if this is a coinbase transaction
    bool is_coinbase() const {
        return inputs.size() == 1 && inputs[0].is_coinbase();
    }

    // Get total input value (requires UTXO set lookup)
    uint64_t get_input_value() const;

    // Get total output value
    uint64_t get_output_value() const;

    // Calculate transaction fee
    uint64_t get_fee() const {
        if (is_coinbase()) return 0;
        return get_input_value() - get_output_value();
    }

    // Validate transaction structure
    bool validate_structure() const;

    // Sign transaction with private key
    bool sign(const std::vector<uint8_t>& private_key, size_t input_index);

    // Verify signature for input
    bool verify_signature(size_t input_index) const;

    // Verify all signatures
    bool verify_all_signatures() const;

    // Create coinbase transaction
    static Transaction create_coinbase(
        uint32_t height,
        uint64_t reward,
        const DilithiumPubKey& miner_pubkey,
        const std::string& extra_data = ""
    );
};

/**
 * UTXO (Unspent Transaction Output)
 * Represents a spendable output
 */
struct UTXO {
    OutPoint outpoint;
    TxOutput output;
    uint32_t height;        // Block height where it was created
    bool is_coinbase;       // Is this from a coinbase transaction?

    UTXO() : outpoint(), output(), height(0), is_coinbase(false) {}

    UTXO(const OutPoint& op, const TxOutput& out, uint32_t h, bool cb)
        : outpoint(op), output(out), height(h), is_coinbase(cb) {}

    // Check if UTXO is mature (can be spent)
    bool is_mature(uint32_t current_height) const {
        if (!is_coinbase) return true;
        return (current_height - height) >= consensus::COINBASE_MATURITY;
    }

    // Serialize for database storage
    std::vector<uint8_t> serialize() const;

    // Deserialize from database
    static UTXO deserialize(const std::vector<uint8_t>& data);
};

/**
 * Transaction builder
 * Helper class for constructing transactions
 */
class TransactionBuilder {
public:
    TransactionBuilder() : tx_() {}

    // Add input
    TransactionBuilder& add_input(const OutPoint& outpoint);

    // Add output
    TransactionBuilder& add_output(uint64_t value, const DilithiumPubKey& pubkey);

    // Set lock time
    TransactionBuilder& set_lock_time(uint32_t lock_time);

    // Build the transaction
    Transaction build();

private:
    Transaction tx_;
};

} // namespace intcoin

// Hash specialization for OutPoint for unordered containers
namespace std {
    template<>
    struct hash<intcoin::OutPoint> {
        size_t operator()(const intcoin::OutPoint& op) const noexcept {
            // Combine hash of tx_hash and index
            size_t h1 = hash<intcoin::Hash256>{}(op.tx_hash);
            size_t h2 = hash<uint32_t>{}(op.index);
            return h1 ^ (h2 + 0x9e3779b9 + (h1 << 6) + (h1 >> 2));
        }
    };
}

#endif // INTCOIN_TRANSACTION_H
