// Copyright (c) 2025 INTcoin Core (Maddison Lane)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef INTCOIN_PEER_MANAGER_H
#define INTCOIN_PEER_MANAGER_H

#include "primitives.h"
#include "p2p.h"
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <mutex>
#include <string>

namespace intcoin {

/**
 * Peer scoring and reputation system
 */
class PeerScore {
public:
    /**
     * Misbehavior types
     */
    enum class Misbehavior {
        INVALID_BLOCK = 100,           // Sent invalid block
        INVALID_TX = 50,               // Sent invalid transaction
        DUPLICATE_INV = 10,            // Sent duplicate inventory
        SLOW_RESPONSE = 5,             // Slow to respond
        PROTOCOL_VIOLATION = 75,       // Protocol violation
        BAD_HEADERS = 80,              // Sent invalid headers
        TIMEOUT = 20,                  // Request timeout
        UNREQUESTED_DATA = 15          // Sent unrequested data
    };

private:
    int32_t score_;                    // Current score (starts at 0)
    uint32_t successful_requests_;     // Successful block/tx requests
    uint32_t failed_requests_;         // Failed requests
    uint64_t bytes_sent_;              // Total bytes sent to peer
    uint64_t bytes_received_;          // Total bytes received from peer
    uint64_t connection_time_;         // When connection was established
    uint64_t last_message_time_;       // Last message received
    std::unordered_map<Hash256, uint64_t> known_inventory_;  // Known inv items

    static constexpr int32_t MAX_SCORE = 100;
    static constexpr int32_t MIN_SCORE = -100;
    static constexpr int32_t BAN_THRESHOLD = -100;

public:
    PeerScore();

    /**
     * Record misbehavior
     * @return true if peer should be banned
     */
    bool record_misbehavior(Misbehavior type);

    /**
     * Record successful interaction
     */
    void record_success();

    /**
     * Record data transfer
     */
    void record_bytes(uint64_t sent, uint64_t received);

    /**
     * Check if peer should be banned
     */
    bool should_ban() const {
        return score_ <= BAN_THRESHOLD;
    }

    /**
     * Get current score
     */
    int32_t get_score() const {
        return score_;
    }

    /**
     * Get success rate
     */
    double get_success_rate() const {
        uint32_t total = successful_requests_ + failed_requests_;
        if (total == 0) return 1.0;
        return static_cast<double>(successful_requests_) / total;
    }

    /**
     * Get connection duration in seconds
     */
    uint64_t get_connection_duration() const;

    /**
     * Check if inventory item is known
     */
    bool is_known_inventory(const Hash256& inv_hash) const;

    /**
     * Add known inventory item
     */
    void add_known_inventory(const Hash256& inv_hash);

    /**
     * Update last message time
     */
    void update_last_message_time();

    /**
     * Get statistics
     */
    struct Stats {
        int32_t score;
        uint32_t successful_requests;
        uint32_t failed_requests;
        double success_rate;
        uint64_t bytes_sent;
        uint64_t bytes_received;
        uint64_t connection_duration;
        size_t known_inventory_size;
    };

    Stats get_stats() const;
};

/**
 * Banned peer information
 */
struct BannedPeer {
    std::string ip;
    uint64_t ban_time;              // When banned
    uint64_t ban_duration;          // Duration in seconds
    std::string reason;             // Ban reason

    bool is_expired() const;
};

/**
 * Peer Manager
 * Handles peer discovery, scoring, and banning
 */
class PeerManager {
private:
    // Peer tracking
    std::unordered_map<std::string, std::shared_ptr<p2p::Peer>> connected_peers_;
    std::unordered_map<std::string, PeerScore> peer_scores_;
    std::unordered_map<std::string, BannedPeer> banned_peers_;
    std::vector<p2p::PeerAddress> known_addresses_;

    // DNS seeds for peer discovery
    std::vector<std::string> dns_seeds_;

    // Configuration
    static constexpr size_t MAX_OUTBOUND_CONNECTIONS = 8;
    static constexpr size_t MAX_INBOUND_CONNECTIONS = 117;
    static constexpr size_t MAX_TOTAL_CONNECTIONS = 125;
    static constexpr uint64_t DEFAULT_BAN_DURATION = 24 * 60 * 60;  // 24 hours

    mutable std::mutex mutex_;

public:
    PeerManager();
    ~PeerManager() = default;

    /**
     * Add DNS seed
     */
    void add_dns_seed(const std::string& seed) {
        dns_seeds_.push_back(seed);
    }

    /**
     * Discover peers from DNS seeds
     * @return List of discovered peer addresses
     */
    std::vector<p2p::PeerAddress> discover_peers();

    /**
     * Add known peer address
     */
    void add_peer_address(const p2p::PeerAddress& addr);

    /**
     * Get peers to connect to
     * @param count Number of peers needed
     * @return List of peer addresses to try
     */
    std::vector<p2p::PeerAddress> get_peers_to_connect(size_t count);

    /**
     * Register connected peer
     */
    bool add_connected_peer(const std::shared_ptr<p2p::Peer>& peer);

    /**
     * Remove peer
     */
    void remove_peer(const std::string& peer_id);

    /**
     * Get peer by address
     */
    std::shared_ptr<p2p::Peer> get_peer(const std::string& peer_id);

    /**
     * Get all connected peers
     */
    std::vector<std::shared_ptr<p2p::Peer>> get_all_peers();

    /**
     * Get peer count
     */
    size_t get_peer_count() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return connected_peers_.size();
    }

    /**
     * Check if can accept more connections
     */
    bool can_accept_connection(bool inbound) const;

    /**
     * Record peer misbehavior
     * @return true if peer was banned
     */
    bool record_misbehavior(const std::string& peer_id, PeerScore::Misbehavior type);

    /**
     * Record successful peer interaction
     */
    void record_success(const std::string& peer_id);

    /**
     * Ban peer
     */
    void ban_peer(const std::string& peer_id, const std::string& reason,
                   uint64_t duration = DEFAULT_BAN_DURATION);

    /**
     * Unban peer
     */
    void unban_peer(const std::string& peer_id);

    /**
     * Check if peer is banned
     */
    bool is_banned(const std::string& peer_id) const;

    /**
     * Get peer score
     */
    PeerScore::Stats get_peer_stats(const std::string& peer_id) const;

    /**
     * Get all banned peers
     */
    std::vector<BannedPeer> get_banned_peers() const;

    /**
     * Clear expired bans
     */
    void clear_expired_bans();

    /**
     * Save peer addresses to disk
     */
    bool save_peers(const std::string& filename);

    /**
     * Load peer addresses from disk
     */
    bool load_peers(const std::string& filename);

    /**
     * Select best peers for a request
     * @param count Number of peers to select
     * @return List of best peers based on score
     */
    std::vector<std::shared_ptr<p2p::Peer>> select_best_peers(size_t count);

private:
    /**
     * Get or create peer score
     */
    PeerScore& get_peer_score(const std::string& peer_id);

    /**
     * Resolve DNS seed to IP addresses
     */
    std::vector<std::string> resolve_dns_seed(const std::string& seed);

    /**
     * Generate peer ID from address
     */
    static std::string make_peer_id(const p2p::PeerAddress& addr) {
        return addr.ip + ":" + std::to_string(addr.port);
    }
};

/**
 * DNS Seed configuration for different networks
 */
class DNSSeeds {
public:
    /**
     * Get mainnet DNS seeds
     */
    static std::vector<std::string> get_mainnet_seeds() {
        return {
            "seed.international-coin.org",
            "seed1.intcoin.network",
            "seed2.intcoin.network",
            "dnsseed.intcoin.io"
        };
    }

    /**
     * Get testnet DNS seeds
     */
    static std::vector<std::string> get_testnet_seeds() {
        return {
            "testnet-seed.international-coin.org",
            "testnet-seed.intcoin.network"
        };
    }

    /**
     * Get regtest fixed nodes (no DNS for regtest)
     */
    static std::vector<p2p::PeerAddress> get_regtest_nodes() {
        return {
            p2p::PeerAddress("127.0.0.1", 18444)
        };
    }
};

} // namespace intcoin

#endif // INTCOIN_PEER_MANAGER_H
