// Copyright (c) 2025 INTcoin Core (Maddison Lane)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef INTCOIN_UNDO_H
#define INTCOIN_UNDO_H

#include "primitives.h"
#include "transaction.h"
#include "serialization.h"
#include <vector>
#include <optional>

namespace intcoin {

/**
 * Information needed to undo spending a transaction output
 */
struct TxOutUndo {
    Transaction::Output output;     // The output that was spent
    uint32_t height;                // Block height where it was created
    bool coinbase;                  // Was this from a coinbase transaction
    uint32_t tx_version;            // Transaction version (for compatibility)

    void serialize(serialization::Serializer& s) const {
        // Write output value
        s.write_uint64(output.value);

        // Write script pubkey
        s.write_vector(output.script_pubkey);

        // Write metadata
        s.write_uint32(height);
        s.write_uint8(coinbase ? 1 : 0);
        s.write_uint32(tx_version);
    }

    static std::optional<TxOutUndo> deserialize(serialization::Deserializer& d) {
        TxOutUndo undo;

        // Read output value
        auto value = d.read_uint64();
        if (!value) return std::nullopt;
        undo.output.value = *value;

        // Read script pubkey
        auto script = d.read_vector();
        if (!script) return std::nullopt;
        undo.output.script_pubkey = *script;

        // Read metadata
        auto height = d.read_uint32();
        if (!height) return std::nullopt;
        undo.height = *height;

        auto coinbase_byte = d.read_uint8();
        if (!coinbase_byte) return std::nullopt;
        undo.coinbase = (*coinbase_byte != 0);

        auto tx_version = d.read_uint32();
        if (!tx_version) return std::nullopt;
        undo.tx_version = *tx_version;

        return undo;
    }
};

/**
 * Undo information for a single transaction
 */
struct TxUndo {
    std::vector<TxOutUndo> outputs_spent;  // All outputs spent by this transaction

    void serialize(serialization::Serializer& s) const {
        s.write_varint(outputs_spent.size());
        for (const auto& undo : outputs_spent) {
            undo.serialize(s);
        }
    }

    static std::optional<TxUndo> deserialize(serialization::Deserializer& d) {
        TxUndo undo;

        auto count = d.read_varint();
        if (!count) return std::nullopt;

        undo.outputs_spent.reserve(static_cast<size_t>(*count));
        for (size_t i = 0; i < *count; ++i) {
            auto output_undo = TxOutUndo::deserialize(d);
            if (!output_undo) return std::nullopt;
            undo.outputs_spent.push_back(*output_undo);
        }

        return undo;
    }
};

/**
 * Undo information for an entire block
 * Contains all data needed to revert the block's effects
 */
class BlockUndo {
public:
    std::vector<TxUndo> tx_undo;  // Undo data for each non-coinbase transaction

    BlockUndo() = default;

    /**
     * Serialize block undo data with version header
     */
    std::vector<uint8_t> serialize() const {
        serialization::Serializer s(serialization::MAX_BLOCK_SIZE);

        // Write version header
        serialization::VersionHeader header{
            serialization::SERIALIZATION_VERSION,
            serialization::VersionHeader::TYPE_BLOCK_UNDO
        };
        header.serialize(s);

        // Write transaction count
        s.write_varint(tx_undo.size());

        // Write each transaction's undo data
        for (const auto& undo : tx_undo) {
            undo.serialize(s);
        }

        return s.data();
    }

    /**
     * Deserialize block undo data
     */
    static std::optional<BlockUndo> deserialize(const std::vector<uint8_t>& data) {
        serialization::Deserializer d(data);

        // Read and verify version header
        auto header = serialization::VersionHeader::deserialize(d);
        if (!header) return std::nullopt;

        if (header->type != serialization::VersionHeader::TYPE_BLOCK_UNDO) {
            return std::nullopt;
        }

        if (header->version != serialization::SERIALIZATION_VERSION) {
            // Handle version migration here if needed
            return std::nullopt;
        }

        BlockUndo undo;

        // Read transaction count
        auto tx_count = d.read_varint();
        if (!tx_count) return std::nullopt;

        // Read each transaction's undo data
        undo.tx_undo.reserve(static_cast<size_t>(*tx_count));
        for (size_t i = 0; i < *tx_count; ++i) {
            auto tx_undo_data = TxUndo::deserialize(d);
            if (!tx_undo_data) return std::nullopt;
            undo.tx_undo.push_back(*tx_undo_data);
        }

        return undo;
    }

    /**
     * Check if undo data is valid
     */
    bool is_valid() const {
        // Undo data should have entries for all non-coinbase transactions
        for (const auto& undo : tx_undo) {
            // Each transaction must spend at least one output (no value in undo for tx with no inputs)
            if (undo.outputs_spent.empty()) {
                return false;
            }
        }
        return true;
    }

    /**
     * Get size in bytes
     */
    size_t get_size() const {
        return serialize().size();
    }
};

/**
 * UTXO (Unspent Transaction Output) entry
 * Stored in the UTXO set
 */
struct UTXOEntry {
    Transaction::Output output;
    uint32_t height;                // Block height where created
    bool coinbase;                  // Is this from a coinbase transaction
    uint32_t tx_version;            // Transaction version

    // Check if this UTXO can be spent at given height
    bool is_spendable(uint32_t current_height) const {
        if (!coinbase) {
            return true;  // Regular transactions are immediately spendable
        }

        // Coinbase outputs require COINBASE_MATURITY confirmations
        constexpr uint32_t COINBASE_MATURITY = 100;
        return current_height >= height + COINBASE_MATURITY;
    }

    void serialize(serialization::Serializer& s) const {
        s.write_uint64(output.value);
        s.write_vector(output.script_pubkey);
        s.write_uint32(height);
        s.write_uint8(coinbase ? 1 : 0);
        s.write_uint32(tx_version);
    }

    static std::optional<UTXOEntry> deserialize(serialization::Deserializer& d) {
        UTXOEntry entry;

        auto value = d.read_uint64();
        if (!value) return std::nullopt;
        entry.output.value = *value;

        auto script = d.read_vector();
        if (!script) return std::nullopt;
        entry.output.script_pubkey = *script;

        auto height = d.read_uint32();
        if (!height) return std::nullopt;
        entry.height = *height;

        auto coinbase = d.read_uint8();
        if (!coinbase) return std::nullopt;
        entry.coinbase = (*coinbase != 0);

        auto tx_version = d.read_uint32();
        if (!tx_version) return std::nullopt;
        entry.tx_version = *tx_version;

        return entry;
    }
};

/**
 * Reorg detector and handler
 */
class ReorgManager {
public:
    struct ReorgInfo {
        Hash256 fork_point;             // Common ancestor block
        uint32_t fork_height;           // Height of fork point
        std::vector<Hash256> old_chain; // Blocks to disconnect
        std::vector<Hash256> new_chain; // Blocks to connect
        uint32_t depth;                 // Reorg depth
    };

    /**
     * Detect if a reorg is needed
     *
     * @param current_tip Current blockchain tip
     * @param new_block_hash Hash of new block to potentially add
     * @return ReorgInfo if reorg needed, nullopt otherwise
     */
    static std::optional<ReorgInfo> detect_reorg(
        const Hash256& current_tip,
        const Hash256& new_block_hash
    );

    /**
     * Apply undo data to disconnect a block
     *
     * @param block_undo Undo data for the block
     * @param block The block to disconnect
     * @return true if successfully disconnected
     */
    static bool disconnect_block(
        const BlockUndo& block_undo,
        const class Block& block
    );

    /**
     * Maximum safe reorg depth
     * Reorgs deeper than this require manual intervention
     */
    static constexpr uint32_t MAX_REORG_DEPTH = 100;

    /**
     * Check if reorg is safe to perform
     */
    static bool is_safe_reorg(const ReorgInfo& info) {
        return info.depth <= MAX_REORG_DEPTH;
    }
};

} // namespace intcoin

#endif // INTCOIN_UNDO_H
