/*
 * Copyright (c) 2025 INTcoin Team (Neil Adamson)
 * SPDX-License-Identifier: MIT License
 * Block Implementation
 */

#include "intcoin/block.h"
#include "intcoin/crypto.h"
#include "intcoin/util.h"
#include "intcoin/consensus.h"
#include <algorithm>

namespace intcoin {

// ============================================================================
// BlockHeader Implementation
// ============================================================================

uint256 BlockHeader::GetHash() const {
    auto serialized = Serialize();
    return SHA3::Hash(serialized);
}

std::vector<uint8_t> BlockHeader::Serialize() const {
    std::vector<uint8_t> result;
    SerializeUint32(result, version);
    SerializeUint256(result, prev_block_hash);
    SerializeUint256(result, merkle_root);
    SerializeUint64(result, timestamp);
    SerializeUint32(result, bits);
    SerializeUint64(result, nonce);
    SerializeUint256(result, randomx_hash);
    SerializeUint256(result, randomx_key);
    return result;
}

Result<BlockHeader> BlockHeader::Deserialize(const std::vector<uint8_t>& data) {
    size_t pos = 0;
    BlockHeader header;

    // Deserialize version (4 bytes)
    auto version_result = DeserializeUint32(data, pos);
    if (version_result.IsError()) {
        return Result<BlockHeader>::Error("Failed to deserialize version: " + version_result.error);
    }
    header.version = *version_result.value;

    // Deserialize prev_block_hash (32 bytes)
    auto prev_hash_result = DeserializeUint256(data, pos);
    if (prev_hash_result.IsError()) {
        return Result<BlockHeader>::Error("Failed to deserialize prev_block_hash: " + prev_hash_result.error);
    }
    header.prev_block_hash = *prev_hash_result.value;

    // Deserialize merkle_root (32 bytes)
    auto merkle_result = DeserializeUint256(data, pos);
    if (merkle_result.IsError()) {
        return Result<BlockHeader>::Error("Failed to deserialize merkle_root: " + merkle_result.error);
    }
    header.merkle_root = *merkle_result.value;

    // Deserialize timestamp (8 bytes)
    auto timestamp_result = DeserializeUint64(data, pos);
    if (timestamp_result.IsError()) {
        return Result<BlockHeader>::Error("Failed to deserialize timestamp: " + timestamp_result.error);
    }
    header.timestamp = *timestamp_result.value;

    // Deserialize bits (4 bytes)
    auto bits_result = DeserializeUint32(data, pos);
    if (bits_result.IsError()) {
        return Result<BlockHeader>::Error("Failed to deserialize bits: " + bits_result.error);
    }
    header.bits = *bits_result.value;

    // Deserialize nonce (8 bytes)
    auto nonce_result = DeserializeUint64(data, pos);
    if (nonce_result.IsError()) {
        return Result<BlockHeader>::Error("Failed to deserialize nonce: " + nonce_result.error);
    }
    header.nonce = *nonce_result.value;

    // Deserialize randomx_hash (32 bytes)
    auto randomx_hash_result = DeserializeUint256(data, pos);
    if (randomx_hash_result.IsError()) {
        return Result<BlockHeader>::Error("Failed to deserialize randomx_hash: " + randomx_hash_result.error);
    }
    header.randomx_hash = *randomx_hash_result.value;

    // Deserialize randomx_key (32 bytes)
    auto randomx_key_result = DeserializeUint256(data, pos);
    if (randomx_key_result.IsError()) {
        return Result<BlockHeader>::Error("Failed to deserialize randomx_key: " + randomx_key_result.error);
    }
    header.randomx_key = *randomx_key_result.value;

    return Result<BlockHeader>::Ok(std::move(header));
}

size_t BlockHeader::GetSerializedSize() const {
    return 4 + 32 + 32 + 8 + 4 + 8 + 32 + 32; // 152 bytes
}

// ============================================================================
// Block Implementation
// ============================================================================

Block::Block(BlockHeader hdr, std::vector<Transaction> txs)
    : header(std::move(hdr)), transactions(std::move(txs)) {
    // Calculate and set merkle root
    header.merkle_root = CalculateMerkleRoot();
}

uint256 Block::GetHash() const {
    if (cached_hash_.has_value()) {
        return *cached_hash_;
    }

    uint256 hash = header.GetHash();
    cached_hash_ = hash;
    return hash;
}

uint256 Block::CalculateMerkleRoot() const {
    if (transactions.empty()) {
        return uint256();
    }

    std::vector<uint256> tx_hashes;
    for (const auto& tx : transactions) {
        tx_hashes.push_back(tx.GetHash());
    }

    return intcoin::CalculateMerkleRoot(tx_hashes);
}

Result<void> Block::Verify() const {
    // TODO: Implement full block verification
    // 1. Verify block header
    // 2. Verify PoW
    // 3. Verify merkle root
    // 4. Verify transactions
    return Result<void>::Error("Not implemented");
}

Result<void> Block::VerifyTransactions() const {
    // TODO: Implement transaction verification
    return Result<void>::Error("Not implemented");
}

uint64_t Block::GetTotalFees() const {
    uint64_t total_fees = 0;
    // TODO: Calculate total fees (requires UTXO set)
    return total_fees;
}

const Transaction& Block::GetCoinbase() const {
    if (transactions.empty()) {
        throw std::runtime_error("Block has no transactions");
    }
    return transactions[0];
}

bool Block::IsGenesis() const {
    // Check if prev_block_hash is all zeros
    return std::all_of(header.prev_block_hash.begin(), header.prev_block_hash.end(),
                      [](uint8_t b) { return b == 0; });
}

std::vector<uint8_t> Block::Serialize() const {
    std::vector<uint8_t> result;

    // Serialize header
    auto header_bytes = header.Serialize();
    result.insert(result.end(), header_bytes.begin(), header_bytes.end());

    // Serialize transaction count
    SerializeUint64(result, transactions.size());

    // Serialize transactions
    for (const auto& tx : transactions) {
        auto tx_bytes = tx.Serialize();
        result.insert(result.end(), tx_bytes.begin(), tx_bytes.end());
    }

    return result;
}

Result<Block> Block::Deserialize(const std::vector<uint8_t>& data) {
    size_t pos = 0;
    Block block;

    // Deserialize header (152 bytes)
    if (data.size() < 152) {
        return Result<Block>::Error("Buffer underflow: not enough bytes for block header");
    }
    std::vector<uint8_t> header_bytes(data.begin(), data.begin() + 152);
    auto header_result = BlockHeader::Deserialize(header_bytes);
    if (header_result.IsError()) {
        return Result<Block>::Error("Failed to deserialize block header: " + header_result.error);
    }
    block.header = *header_result.value;
    pos += 152;

    // Deserialize transaction count (8 bytes)
    auto tx_count_result = DeserializeUint64(data, pos);
    if (tx_count_result.IsError()) {
        return Result<Block>::Error("Failed to deserialize transaction count: " + tx_count_result.error);
    }
    uint64_t tx_count = *tx_count_result.value;

    // Deserialize each transaction
    block.transactions.reserve(tx_count);
    for (uint64_t i = 0; i < tx_count; ++i) {
        // For each transaction, we need to deserialize it inline
        // because we don't know the size beforehand
        Transaction tx;

        // Deserialize version (4 bytes)
        auto version_result = DeserializeUint32(data, pos);
        if (version_result.IsError()) {
            return Result<Block>::Error("Failed to deserialize transaction " + std::to_string(i) + " version: " + version_result.error);
        }
        tx.version = *version_result.value;

        // Deserialize inputs count (8 bytes)
        auto inputs_count_result = DeserializeUint64(data, pos);
        if (inputs_count_result.IsError()) {
            return Result<Block>::Error("Failed to deserialize transaction " + std::to_string(i) + " inputs count: " + inputs_count_result.error);
        }
        uint64_t inputs_count = *inputs_count_result.value;

        // Deserialize each input
        tx.inputs.reserve(inputs_count);
        for (uint64_t j = 0; j < inputs_count; ++j) {
            TxIn txin;

            // Deserialize prev_tx_hash (32 bytes)
            auto hash_result = DeserializeUint256(data, pos);
            if (hash_result.IsError()) {
                return Result<Block>::Error("Failed to deserialize tx " + std::to_string(i) + " input " + std::to_string(j) + " prev_tx_hash: " + hash_result.error);
            }
            txin.prev_tx_hash = *hash_result.value;

            // Deserialize prev_tx_index (4 bytes)
            auto index_result = DeserializeUint32(data, pos);
            if (index_result.IsError()) {
                return Result<Block>::Error("Failed to deserialize tx " + std::to_string(i) + " input " + std::to_string(j) + " prev_tx_index: " + index_result.error);
            }
            txin.prev_tx_index = *index_result.value;

            // Deserialize script_sig length (8 bytes)
            auto script_len_result = DeserializeUint64(data, pos);
            if (script_len_result.IsError()) {
                return Result<Block>::Error("Failed to deserialize tx " + std::to_string(i) + " input " + std::to_string(j) + " script_sig length: " + script_len_result.error);
            }
            uint64_t script_len = *script_len_result.value;

            // Deserialize script_sig bytes
            if (pos + script_len > data.size()) {
                return Result<Block>::Error("Buffer underflow: not enough bytes for tx " + std::to_string(i) + " input " + std::to_string(j) + " script_sig");
            }
            std::vector<uint8_t> script_bytes(data.begin() + pos, data.begin() + pos + script_len);
            txin.script_sig = Script::Deserialize(script_bytes);
            pos += script_len;

            // Deserialize sequence (4 bytes)
            auto seq_result = DeserializeUint32(data, pos);
            if (seq_result.IsError()) {
                return Result<Block>::Error("Failed to deserialize tx " + std::to_string(i) + " input " + std::to_string(j) + " sequence: " + seq_result.error);
            }
            txin.sequence = *seq_result.value;

            tx.inputs.push_back(std::move(txin));
        }

        // Deserialize outputs count (8 bytes)
        auto outputs_count_result = DeserializeUint64(data, pos);
        if (outputs_count_result.IsError()) {
            return Result<Block>::Error("Failed to deserialize transaction " + std::to_string(i) + " outputs count: " + outputs_count_result.error);
        }
        uint64_t outputs_count = *outputs_count_result.value;

        // Deserialize each output
        tx.outputs.reserve(outputs_count);
        for (uint64_t j = 0; j < outputs_count; ++j) {
            TxOut txout;

            // Deserialize value (8 bytes)
            auto value_result = DeserializeUint64(data, pos);
            if (value_result.IsError()) {
                return Result<Block>::Error("Failed to deserialize tx " + std::to_string(i) + " output " + std::to_string(j) + " value: " + value_result.error);
            }
            txout.value = *value_result.value;

            // Deserialize script_pubkey length (8 bytes)
            auto script_len_result = DeserializeUint64(data, pos);
            if (script_len_result.IsError()) {
                return Result<Block>::Error("Failed to deserialize tx " + std::to_string(i) + " output " + std::to_string(j) + " script_pubkey length: " + script_len_result.error);
            }
            uint64_t script_len = *script_len_result.value;

            // Deserialize script_pubkey bytes
            if (pos + script_len > data.size()) {
                return Result<Block>::Error("Buffer underflow: not enough bytes for tx " + std::to_string(i) + " output " + std::to_string(j) + " script_pubkey");
            }
            std::vector<uint8_t> script_bytes(data.begin() + pos, data.begin() + pos + script_len);
            txout.script_pubkey = Script::Deserialize(script_bytes);
            pos += script_len;

            tx.outputs.push_back(std::move(txout));
        }

        // Deserialize locktime (8 bytes)
        auto locktime_result = DeserializeUint64(data, pos);
        if (locktime_result.IsError()) {
            return Result<Block>::Error("Failed to deserialize transaction " + std::to_string(i) + " locktime: " + locktime_result.error);
        }
        tx.locktime = *locktime_result.value;

        // Deserialize signature (DILITHIUM3_BYTES = 3293 bytes)
        if (pos + DILITHIUM3_BYTES > data.size()) {
            return Result<Block>::Error("Buffer underflow: not enough bytes for transaction " + std::to_string(i) + " signature");
        }
        std::copy(data.begin() + pos, data.begin() + pos + DILITHIUM3_BYTES, tx.signature.begin());
        pos += DILITHIUM3_BYTES;

        block.transactions.push_back(std::move(tx));
    }

    return Result<Block>::Ok(std::move(block));
}

size_t Block::GetSerializedSize() const {
    size_t size = header.GetSerializedSize() + 8; // header + tx count
    for (const auto& tx : transactions) {
        size += tx.GetSerializedSize();
    }
    return size;
}

// ============================================================================
// Genesis Block
// ============================================================================

Block CreateGenesisBlock() {
    BlockHeader header;
    header.version = 1;
    header.prev_block_hash = uint256(); // Zero hash
    header.timestamp = 1735171200; // January 1, 2025 00:00:00 UTC
    header.bits = consensus::MIN_DIFFICULTY_BITS;
    header.nonce = 0;

    // Create coinbase transaction
    // TODO: Use actual genesis address
    PublicKey genesis_pubkey; // Placeholder
    Transaction coinbase = CreateCoinbaseTransaction(
        0, consensus::INITIAL_BLOCK_REWARD, genesis_pubkey);

    std::vector<Transaction> transactions;
    transactions.push_back(coinbase);

    Block genesis(header, transactions);
    return genesis;
}

const uint256& GetGenesisBlockHash() {
    static uint256 genesis_hash = CreateGenesisBlock().GetHash();
    return genesis_hash;
}

// ============================================================================
// Block Validation
// ============================================================================

Result<void> ValidateBlockHeader(const BlockHeader& header) {
    // TODO: Implement header validation
    return Result<void>::Error("Not implemented");
}

Result<void> ValidateBlockStructure(const Block& block) {
    // TODO: Implement structure validation
    return Result<void>::Error("Not implemented");
}

Result<void> ValidateBlockTransactions(const Block& block) {
    // TODO: Implement transaction validation
    return Result<void>::Error("Not implemented");
}

Result<void> ValidateProofOfWork(const BlockHeader& header) {
    // TODO: Implement PoW validation
    return Result<void>::Error("Not implemented");
}

// ============================================================================
// Merkle Tree
// ============================================================================

uint256 CalculateMerkleRoot(const std::vector<uint256>& tx_hashes) {
    if (tx_hashes.empty()) {
        return uint256();
    }

    std::vector<uint256> hashes = tx_hashes;

    // Build merkle tree bottom-up
    while (hashes.size() > 1) {
        std::vector<uint256> next_level;

        for (size_t i = 0; i < hashes.size(); i += 2) {
            std::vector<uint8_t> combined;
            SerializeUint256(combined, hashes[i]);

            // If odd number, duplicate last hash
            if (i + 1 < hashes.size()) {
                SerializeUint256(combined, hashes[i + 1]);
            } else {
                SerializeUint256(combined, hashes[i]);
            }

            next_level.push_back(SHA3::Hash(combined));
        }

        hashes = std::move(next_level);
    }

    return hashes[0];
}

std::vector<uint256> BuildMerkleTree(const std::vector<uint256>& tx_hashes) {
    // TODO: Implement full merkle tree building
    std::vector<uint256> tree;
    return tree;
}

std::vector<uint256> GetMerkleBranch(const std::vector<uint256>& tx_hashes,
                                    size_t index) {
    // TODO: Implement merkle branch generation
    std::vector<uint256> branch;
    return branch;
}

bool VerifyMerkleProof(const uint256& tx_hash, const uint256& merkle_root,
                       const std::vector<uint256>& branch, size_t index) {
    // TODO: Implement merkle proof verification
    return false;
}

} // namespace intcoin
