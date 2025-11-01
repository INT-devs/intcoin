// Copyright (c) 2025 INTcoin Core (Maddison Lane)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
//
// CPU miner using SHA-256 Proof of Work.

#ifndef INTCOIN_MINER_H
#define INTCOIN_MINER_H

#include "block.h"
#include "transaction.h"
#include "primitives.h"
#include "blockchain.h"
#include "mempool.h"
#include <atomic>
#include <thread>
#include <functional>
#include <memory>

namespace intcoin {

/**
 * Mining statistics
 */
struct MiningStats {
    uint64_t hashes_per_second;
    uint64_t total_hashes;
    uint64_t blocks_found;
    uint64_t last_block_time;
    uint32_t current_difficulty;

    MiningStats()
        : hashes_per_second(0)
        , total_hashes(0)
        , blocks_found(0)
        , last_block_time(0)
        , current_difficulty(0)
    {}
};

/**
 * CPU Miner using SHA-256 PoW
 */
class Miner {
public:
    Miner(Blockchain& blockchain, Mempool& mempool);
    ~Miner();

    // Mining control
    bool start(const DilithiumPubKey& reward_address, size_t num_threads = 0);
    void stop();
    bool is_mining() const { return mining_; }

    // Mining configuration
    void set_extra_nonce(const std::string& extra_nonce);
    void set_threads(size_t count);

    // Statistics
    MiningStats get_stats() const;
    uint64_t get_hashrate() const { return stats_.hashes_per_second; }

    // Callbacks
    using BlockFoundCallback = std::function<void(const Block&)>;
    void set_block_found_callback(BlockFoundCallback cb) { block_found_callback_ = cb; }

private:
    Blockchain& blockchain_;
    Mempool& mempool_;

    std::atomic<bool> mining_;
    std::vector<std::thread> mining_threads_;
    size_t num_threads_;

    DilithiumPubKey reward_address_;
    std::string extra_nonce_;

    mutable MiningStats stats_;
    BlockFoundCallback block_found_callback_;

    // Mining functions
    void mining_thread(size_t thread_id);
    Block create_block_template();
    bool try_mine_block(Block& block, uint64_t start_nonce, uint64_t end_nonce);
    bool check_proof_of_work(const Block& block) const;

    // Block construction
    Transaction create_coinbase_transaction(uint32_t height);
    std::vector<Transaction> select_transactions();
    Hash256 calculate_merkle_root(const std::vector<Transaction>& transactions);

    // Difficulty
    uint32_t get_next_difficulty() const;
    bool meets_difficulty_target(const Hash256& hash, uint32_t bits) const;
};

/**
 * Mining pool support (for future implementation)
 */
namespace pool {
    struct PoolConfig {
        std::string pool_url;
        std::string worker_name;
        std::string password;
        bool use_stratum;
    };

    class PoolMiner {
    public:
        PoolMiner(const PoolConfig& config);
        bool connect();
        void disconnect();
        bool submit_share(const Block& block);
    private:
        PoolConfig config_;
    };
}

} // namespace intcoin

#endif // INTCOIN_MINER_H
