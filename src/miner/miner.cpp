// Copyright (c) 2025 INTcoin Core (Maddison Lane)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "intcoin/miner.h"
#include "intcoin/crypto.h"
#include <chrono>
#include <algorithm>

namespace intcoin {

Miner::Miner(Blockchain& blockchain, Mempool& mempool)
    : blockchain_(blockchain)
    , mempool_(mempool)
    , mining_(false)
    , num_threads_(0)
{
}

Miner::~Miner() {
    stop();
}

bool Miner::start(const DilithiumPubKey& reward_address, size_t num_threads) {
    if (mining_) {
        return false;
    }

    reward_address_ = reward_address;

    // Auto-detect thread count if not specified
    if (num_threads == 0) {
        num_threads_ = std::thread::hardware_concurrency();
        if (num_threads_ == 0) num_threads_ = 1;
    } else {
        num_threads_ = num_threads;
    }

    mining_ = true;

    // Start mining threads
    for (size_t i = 0; i < num_threads_; ++i) {
        mining_threads_.emplace_back(&Miner::mining_thread, this, i);
    }

    return true;
}

void Miner::stop() {
    if (!mining_) {
        return;
    }

    mining_ = false;

    // Wait for all threads to finish
    for (auto& thread : mining_threads_) {
        if (thread.joinable()) {
            thread.join();
        }
    }

    mining_threads_.clear();
}

void Miner::set_extra_nonce(const std::string& extra_nonce) {
    extra_nonce_ = extra_nonce;
}

void Miner::set_threads(size_t count) {
    if (mining_) {
        stop();
        num_threads_ = count;
        start(reward_address_, count);
    } else {
        num_threads_ = count;
    }
}

MiningStats Miner::get_stats() const {
    return stats_;
}

void Miner::mining_thread(size_t thread_id) {
    const uint64_t nonce_range = 0xFFFFFFFFFFFFFFFF / num_threads_;
    const uint64_t start_nonce = thread_id * nonce_range;
    const uint64_t end_nonce = (thread_id + 1) * nonce_range;

    auto last_hash_count = std::chrono::steady_clock::now();
    uint64_t hashes_this_period = 0;

    while (mining_) {
        // Create new block template
        Block block = create_block_template();

        // Try to mine the block
        if (try_mine_block(block, start_nonce, end_nonce)) {
            // Block found!
            stats_.blocks_found++;
            stats_.last_block_time = std::chrono::system_clock::now().time_since_epoch().count();

            if (block_found_callback_) {
                block_found_callback_(block);
            }
        }

        // Update hashrate every second
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - last_hash_count).count();
        if (elapsed >= 1) {
            stats_.hashes_per_second = hashes_this_period / elapsed;
            hashes_this_period = 0;
            last_hash_count = now;
        }
    }
}

Block Miner::create_block_template() {
    Block block;

    // Get best block
    Hash256 prev_hash = blockchain_.get_best_block_hash();
    uint32_t height = blockchain_.get_height() + 1;

    // Set header
    block.header.version = 1;
    block.header.previous_block_hash = prev_hash;
    block.header.timestamp = std::chrono::system_clock::now().time_since_epoch().count() / 1000000000;
    block.header.bits = get_next_difficulty();
    block.header.nonce = 0;

    // Create coinbase transaction
    Transaction coinbase = create_coinbase_transaction(height);
    block.transactions.push_back(coinbase);

    // Add transactions from mempool
    std::vector<Transaction> txs = select_transactions();
    block.transactions.insert(block.transactions.end(), txs.begin(), txs.end());

    // Calculate merkle root
    block.header.merkle_root = block.calculate_merkle_root();

    return block;
}

bool Miner::try_mine_block(Block& block, uint64_t start_nonce, uint64_t end_nonce) {
    for (uint64_t nonce = start_nonce; nonce < end_nonce && mining_; ++nonce) {
        block.header.nonce = nonce;

        stats_.total_hashes++;

        if (check_proof_of_work(block)) {
            return true;
        }

        // Check for new block every 1000 hashes
        if (nonce % 1000 == 0) {
            // Check if blockchain has advanced
            if (block.header.previous_block_hash != blockchain_.get_best_block_hash()) {
                return false;  // Start fresh with new template
            }
        }
    }

    return false;
}

bool Miner::check_proof_of_work(const Block& block) const {
    return block.header.check_proof_of_work();
}

Transaction Miner::create_coinbase_transaction(uint32_t height) {
    uint64_t reward = Blockchain::calculate_block_reward(height);
    uint64_t fees = 0;

    // Calculate total fees from mempool transactions
    for (const auto& tx : mempool_.get_all_transactions()) {
        fees += tx.get_fee();
    }

    return Transaction::create_coinbase(height, reward + fees, reward_address_, extra_nonce_);
}

std::vector<Transaction> Miner::select_transactions() {
    // Get transactions sorted by fee rate
    // Limit to reasonable block size
    const size_t MAX_BLOCK_SIZE = 1024 * 1024;  // 1 MB
    const size_t MAX_BLOCK_TXS = 2000;

    return mempool_.get_transactions_for_mining(MAX_BLOCK_TXS, MAX_BLOCK_SIZE);
}

Hash256 Miner::calculate_merkle_root(const std::vector<Transaction>& transactions) {
    std::vector<Hash256> hashes;
    hashes.reserve(transactions.size());

    for (const auto& tx : transactions) {
        hashes.push_back(tx.get_hash());
    }

    return MerkleTree::calculate_root(hashes);
}

uint32_t Miner::get_next_difficulty() const {
    Hash256 best_block = blockchain_.get_best_block_hash();
    return blockchain_.calculate_next_difficulty(best_block);
}

bool Miner::meets_difficulty_target(const Hash256& hash, uint32_t bits) const {
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
        if (hash[i] < target[i]) return true;
        if (hash[i] > target[i]) return false;
    }

    return true;
}

} // namespace intcoin
