// Copyright (c) 2025 INTcoin Core (Maddison Lane)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "intcoin/block.h"
#include "intcoin/crypto.h"
#include "intcoin/merkle.h"
#include "intcoin/serialization.h"
#include <cstring>

namespace intcoin {

// BlockHeader implementation

std::vector<uint8_t> BlockHeader::serialize() const {
    std::vector<uint8_t> buffer;
    buffer.reserve(80);

    // Version (4 bytes)
    buffer.push_back(static_cast<uint8_t>(version & 0xFF));
    buffer.push_back(static_cast<uint8_t>((version >> 8) & 0xFF));
    buffer.push_back(static_cast<uint8_t>((version >> 16) & 0xFF));
    buffer.push_back(static_cast<uint8_t>((version >> 24) & 0xFF));

    // Previous block hash (32 bytes)
    buffer.insert(buffer.end(), previous_block_hash.begin(), previous_block_hash.end());

    // Merkle root (32 bytes)
    buffer.insert(buffer.end(), merkle_root.begin(), merkle_root.end());

    // Timestamp (8 bytes)
    for (int i = 0; i < 8; ++i) {
        buffer.push_back(static_cast<uint8_t>((timestamp >> (i * 8)) & 0xFF));
    }

    // Bits (4 bytes)
    buffer.push_back(static_cast<uint8_t>(bits & 0xFF));
    buffer.push_back(static_cast<uint8_t>((bits >> 8) & 0xFF));
    buffer.push_back(static_cast<uint8_t>((bits >> 16) & 0xFF));
    buffer.push_back(static_cast<uint8_t>((bits >> 24) & 0xFF));

    // Nonce (8 bytes)
    for (int i = 0; i < 8; ++i) {
        buffer.push_back(static_cast<uint8_t>((nonce >> (i * 8)) & 0xFF));
    }

    return buffer;
}

BlockHeader BlockHeader::deserialize(const std::vector<uint8_t>& data) {
    BlockHeader header;

    if (data.size() < 88) {
        return header;
    }

    size_t offset = 0;

    // Version
    header.version = static_cast<uint32_t>(data[offset]) |
                     (static_cast<uint32_t>(data[offset + 1]) << 8) |
                     (static_cast<uint32_t>(data[offset + 2]) << 16) |
                     (static_cast<uint32_t>(data[offset + 3]) << 24);
    offset += 4;

    // Previous block hash
    std::copy(data.begin() + offset, data.begin() + offset + 32, header.previous_block_hash.begin());
    offset += 32;

    // Merkle root
    std::copy(data.begin() + offset, data.begin() + offset + 32, header.merkle_root.begin());
    offset += 32;

    // Timestamp
    header.timestamp = 0;
    for (int i = 0; i < 8; ++i) {
        header.timestamp |= static_cast<uint64_t>(data[offset + i]) << (i * 8);
    }
    offset += 8;

    // Bits
    header.bits = static_cast<uint32_t>(data[offset]) |
                  (static_cast<uint32_t>(data[offset + 1]) << 8) |
                  (static_cast<uint32_t>(data[offset + 2]) << 16) |
                  (static_cast<uint32_t>(data[offset + 3]) << 24);
    offset += 4;

    // Nonce
    header.nonce = 0;
    for (int i = 0; i < 8; ++i) {
        header.nonce |= static_cast<uint64_t>(data[offset + i]) << (i * 8);
    }

    return header;
}

Hash256 BlockHeader::get_hash() const {
    std::vector<uint8_t> serialized = serialize();
    return crypto::SHA3_256::hash(serialized.data(), serialized.size());
}

bool BlockHeader::check_proof_of_work() const {
    std::vector<uint8_t> serialized = serialize();
    Hash256 block_hash = crypto::SHA256_PoW::hash(serialized.data(), serialized.size());

    // Convert bits to target
    uint32_t exponent = bits >> 24;
    uint32_t mantissa = bits & 0x00FFFFFF;

    // Target = mantissa * 2^(8 * (exponent - 3))
    Hash256 target{};
    if (exponent <= 3) {
        uint32_t shifted = mantissa >> (8 * (3 - exponent));
        target[31] = shifted & 0xFF;
        target[30] = (shifted >> 8) & 0xFF;
        target[29] = (shifted >> 16) & 0xFF;
    } else {
        size_t shift_bytes = exponent - 3;
        if (shift_bytes < 29) {
            target[31 - shift_bytes] = mantissa & 0xFF;
            target[31 - shift_bytes - 1] = (mantissa >> 8) & 0xFF;
            target[31 - shift_bytes - 2] = (mantissa >> 16) & 0xFF;
        }
    }

    // Check if hash <= target (comparing as big-endian)
    for (size_t i = 0; i < 32; ++i) {
        if (block_hash[i] < target[i]) {
            return true;
        }
        if (block_hash[i] > target[i]) {
            return false;
        }
    }

    return true;
}

// Block implementation

std::vector<uint8_t> Block::serialize() const {
    serialization::Serializer s(serialization::MAX_BLOCK_SIZE);

    // Write version header
    serialization::VersionHeader version_header{
        serialization::SERIALIZATION_VERSION,
        serialization::VersionHeader::TYPE_BLOCK
    };
    version_header.serialize(s);

    // Serialize block header
    std::vector<uint8_t> header_data = header.serialize();
    s.write_bytes(header_data.data(), header_data.size());

    // Transaction count (with bounds checking via varint)
    s.write_varint(transactions.size());

    // Serialize each transaction
    for (const auto& tx : transactions) {
        std::vector<uint8_t> tx_data = tx.serialize();
        s.write_bytes(tx_data.data(), tx_data.size());
    }

    return s.data();
}

Block Block::deserialize(const std::vector<uint8_t>& data) {
    Block block;
    serialization::Deserializer d(data);

    // Read and validate version header
    auto version_header = serialization::VersionHeader::deserialize(d);
    if (!version_header) {
        return block;  // Invalid version header
    }

    if (version_header->type != serialization::VersionHeader::TYPE_BLOCK) {
        return block;  // Wrong object type
    }

    if (version_header->version != serialization::SERIALIZATION_VERSION) {
        // Handle version migration here if needed
        return block;
    }

    // Deserialize block header (88 bytes)
    auto header_bytes = d.read_bytes(88);
    if (!header_bytes) {
        return block;
    }
    block.header = BlockHeader::deserialize(*header_bytes);

    // Read transaction count
    auto tx_count = d.read_varint();
    if (!tx_count) {
        return block;
    }

    // Validate transaction count (prevent DOS)
    if (*tx_count > 1000000) {  // Reasonable max transactions per block
        return block;
    }

    // Deserialize each transaction
    block.transactions.clear();
    block.transactions.reserve(static_cast<size_t>(*tx_count));

    for (uint64_t i = 0; i < *tx_count; ++i) {
        if (d.remaining() == 0) {
            // Not enough data for remaining transactions
            return Block{};  // Return empty block on error
        }

        // Read transaction data
        std::vector<uint8_t> remaining_data(d.remaining());
        std::copy(data.data() + d.offset(), data.data() + data.size(), remaining_data.begin());

        Transaction tx = Transaction::deserialize(remaining_data);
        block.transactions.push_back(tx);

        // Skip the bytes we just read
        size_t tx_size = tx.serialize().size();
        d.skip(tx_size);
    }

    return block;
}

Hash256 Block::calculate_merkle_root() const {
    if (transactions.empty()) {
        return Hash256{};
    }

    std::vector<Hash256> hashes;
    hashes.reserve(transactions.size());
    for (const auto& tx : transactions) {
        hashes.push_back(tx.get_hash());
    }

    return MerkleTree::calculate_root(hashes);
}

bool Block::validate() const {
    // Check there's at least one transaction
    if (transactions.empty()) {
        return false;
    }

    // Check merkle root matches
    if (header.merkle_root != calculate_merkle_root()) {
        return false;
    }

    // Check proof of work
    if (!header.check_proof_of_work()) {
        return false;
    }

    return true;
}

size_t Block::get_size() const {
    return serialize().size();
}

size_t Block::get_weight() const {
    // For now, weight == size (can be modified for SegWit-style weight calculation)
    return get_size();
}

uint64_t Block::get_total_fees() const {
    uint64_t total_fees = 0;

    // Skip coinbase (first transaction)
    for (size_t i = 1; i < transactions.size(); ++i) {
        uint64_t fee = transactions[i].get_fee();
        total_fees += fee;
    }

    return total_fees;
}

uint64_t Block::get_block_reward(uint32_t height) {
    // 221 Trillion INT emission schedule
    // Block time: 2 minutes (262,800 blocks/year)
    // Halving: 50% every 4 years (traditional Bitcoin-style)
    // Halving interval: 1,051,200 blocks (~4 years)
    // Initial reward: 105,113,636 INT
    // Max supply: 221 Trillion INT

    const uint64_t initial_reward = 105113636ULL * COIN;
    const uint32_t halving_interval = 1051200;  // 4 years (262,800 * 4)

    uint32_t halvings = height / halving_interval;

    // After 64 halvings, reward becomes 0
    if (halvings >= 64) {
        return 0;
    }

    // Traditional Bitcoin-style halving: reward = initial_reward / (2^halvings)
    return initial_reward >> halvings;
}

bool Block::verify_block_reward(uint32_t height) const {
    if (transactions.empty()) {
        return false;
    }

    const Transaction& coinbase = transactions[0];
    if (!coinbase.is_coinbase()) {
        return false;
    }

    uint64_t expected_reward = get_block_reward(height);
    uint64_t total_fees = get_total_fees();
    uint64_t coinbase_value = coinbase.get_output_value();

    return coinbase_value <= expected_reward + total_fees;
}

// GenesisBlock implementation

Block GenesisBlock::create_mainnet() {
    const std::string message = "The Times 01/Jan/2025 Quantum-Resistant Cryptocurrency Era Begins";
    const uint64_t timestamp = 1735689600;  // January 1, 2025 00:00:00 UTC
    const uint64_t nonce = 0;  // Will need to be mined
    const uint32_t bits = 0x1d00ffff;  // Initial difficulty

    return create_genesis(message, timestamp, nonce, bits);
}

Block GenesisBlock::create_testnet() {
    const std::string message = "INTcoin Testnet Genesis Block";
    const uint64_t timestamp = 1735689600;
    const uint64_t nonce = 0;
    const uint32_t bits = 0x1d00ffff;  // Easier difficulty

    return create_genesis(message, timestamp, nonce, bits);
}

Block GenesisBlock::create_genesis(
    const std::string& message,
    uint64_t timestamp,
    uint64_t nonce,
    uint32_t bits
) {
    Block genesis;

    // Create coinbase transaction
    Transaction coinbase;
    coinbase.version = 1;
    coinbase.lock_time = 0;

    // Coinbase input
    TxInput coinbase_input;
    coinbase_input.previous_output = OutPoint(Hash256{}, 0xFFFFFFFF);
    coinbase_input.sequence = 0xFFFFFFFF;
    coinbase_input.script_sig.assign(message.begin(), message.end());

    coinbase.inputs.push_back(coinbase_input);

    // Coinbase output
    TxOutput coinbase_output;
    coinbase_output.value = 50 * COIN;
    coinbase_output.script_pubkey = {0x76, 0xa9, 0x14};  // Placeholder script

    coinbase.outputs.push_back(coinbase_output);

    // Add transaction to block
    genesis.transactions.push_back(coinbase);

    // Set header
    genesis.header.version = 1;
    genesis.header.previous_block_hash = Hash256{};
    genesis.header.merkle_root = genesis.calculate_merkle_root();
    genesis.header.timestamp = timestamp;
    genesis.header.bits = bits;
    genesis.header.nonce = nonce;

    return genesis;
}

} // namespace intcoin
