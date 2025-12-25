/*
 * Copyright (c) 2025 INTcoin Team (Neil Adamson)
 * MIT License
 * Mining Pool HTTP API Server (Stub Implementation)
 */

#include "intcoin/pool.h"
#include "intcoin/rpc.h"
#include <sstream>
#include <iomanip>
#include <algorithm>

namespace intcoin {
namespace pool {

// ============================================================================
// HTTP API Server for Pool Dashboard
// ============================================================================

/**
 * Simple HTTP API server for mining pool statistics
 *
 * This is a minimal implementation providing the RPC endpoints required
 * by the pool dashboard (see web/pool-dashboard/README.md)
 *
 * Full implementation would use a proper HTTP server framework
 * (similar to RPCServer in src/rpc/rpc.cpp)
 */
class HttpApiServer {
public:
    HttpApiServer(uint16_t port, MiningPoolServer& pool)
        : port_(port), pool_(pool), is_running_(false) {
        (void)port_;  // TODO: Use when implementing HTTP server
    }

    Result<void> Start() {
        // TODO: Implement HTTP server
        // For now, just mark as running
        is_running_ = true;
        return Result<void>::Ok();
    }

    void Stop() {
        is_running_ = false;
    }

    bool IsRunning() const {
        return is_running_;
    }

    // ========================================================================
    // API Endpoints (matching pool dashboard requirements)
    // ========================================================================

    /**
     * GET /api/pool/stats
     * Returns pool statistics
     */
    rpc::JSONValue GetPoolStats() {
        auto stats = pool_.GetStatistics();

        std::map<std::string, rpc::JSONValue> response;
        response["hashrate"] = rpc::JSONValue(static_cast<int64_t>(stats.pool_hashrate));
        response["difficulty"] = rpc::JSONValue(static_cast<int64_t>(stats.network_difficulty));
        response["miners"] = rpc::JSONValue(static_cast<int64_t>(stats.active_miners));
        response["blocks_found"] = rpc::JSONValue(static_cast<int64_t>(stats.blocks_found));
        response["total_shares"] = rpc::JSONValue(static_cast<int64_t>(stats.total_shares));
        response["valid_shares_24h"] = rpc::JSONValue(static_cast<int64_t>(stats.shares_last_day));

        return rpc::JSONValue(response);
    }

    /**
     * GET /api/pool/blocks?limit=10
     * Returns recent blocks found by pool
     */
    rpc::JSONValue GetRecentBlocks(int limit) {
        auto rounds = pool_.GetRoundHistory(limit);

        std::vector<rpc::JSONValue> blocks;

        for (const auto& round : rounds) {
            if (!round.is_complete) continue;

            std::map<std::string, rpc::JSONValue> block;
            block["height"] = rpc::JSONValue(static_cast<int64_t>(round.block_height));
            block["hash"] = rpc::JSONValue(ToHex(round.block_hash));
            block["timestamp"] = rpc::JSONValue(static_cast<int64_t>(
                std::chrono::duration_cast<std::chrono::milliseconds>(
                    round.ended_at.time_since_epoch()).count()));
            block["finder"] = rpc::JSONValue("pool");  // TODO: Get actual finder
            block["reward"] = rpc::JSONValue(static_cast<int64_t>(round.block_reward));
            block["status"] = rpc::JSONValue("confirmed");  // TODO: Check actual status

            blocks.push_back(rpc::JSONValue(block));
        }

        return rpc::JSONValue(blocks);
    }

    /**
     * GET /api/pool/payments?limit=20
     * Returns recent payments to miners
     */
    rpc::JSONValue GetRecentPayments(int limit) {
        // TODO: Implement payment tracking
        std::vector<rpc::JSONValue> payments;
        return rpc::JSONValue(payments);
    }

    /**
     * GET /api/pool/topminers?limit=10
     * Returns top miners by hashrate (24h)
     */
    rpc::JSONValue GetTopMiners(int limit) {
        auto miners = pool_.GetAllMiners();

        // Sort by hashrate (descending)
        std::sort(miners.begin(), miners.end(),
            [this](const intcoin::Miner& a, const intcoin::Miner& b) {
                return pool_.CalculateMinerHashrate(a.miner_id) >
                       pool_.CalculateMinerHashrate(b.miner_id);
            });

        std::vector<rpc::JSONValue> top_miners;

        for (size_t i = 0; i < std::min(static_cast<size_t>(limit), miners.size()); i++) {
            const auto& miner = miners[i];

            std::map<std::string, rpc::JSONValue> miner_obj;
            miner_obj["rank"] = rpc::JSONValue(static_cast<int64_t>(i + 1));
            miner_obj["address"] = rpc::JSONValue(miner.payout_address);
            miner_obj["hashrate"] = rpc::JSONValue(static_cast<int64_t>(pool_.CalculateMinerHashrate(miner.miner_id)));
            miner_obj["shares"] = rpc::JSONValue(static_cast<int64_t>(miner.total_shares_accepted));

            top_miners.push_back(rpc::JSONValue(miner_obj));
        }

        return rpc::JSONValue(top_miners);
    }

    /**
     * GET /api/pool/worker?address=intc1...
     * Returns statistics for specific worker/miner
     */
    rpc::JSONValue GetWorkerStats(const std::string& address) {
        // Find miner by payout address
        auto miners = pool_.GetAllMiners();

        for (const auto& miner : miners) {
            if (miner.payout_address == address) {
                std::map<std::string, rpc::JSONValue> stats;
                stats["address"] = rpc::JSONValue(miner.payout_address);
                stats["hashrate"] = rpc::JSONValue(static_cast<int64_t>(pool_.CalculateMinerHashrate(miner.miner_id)));
                stats["shares"] = rpc::JSONValue(static_cast<int64_t>(miner.total_shares_accepted));
                stats["balance"] = rpc::JSONValue(static_cast<int64_t>(miner.unpaid_balance));
                stats["total_paid"] = rpc::JSONValue(static_cast<int64_t>(miner.paid_balance));

                return rpc::JSONValue(stats);
            }
        }

        // Worker not found
        std::map<std::string, rpc::JSONValue> error;
        error["error"] = rpc::JSONValue("Worker not found");
        return rpc::JSONValue(error);
    }

private:
    uint16_t port_;
    MiningPoolServer& pool_;
    std::atomic<bool> is_running_;
};

} // namespace pool
} // namespace intcoin
