// Copyright (c) 2025 INTcoin Core (Maddison Lane)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
//
// P2P networking layer for node communication.

#ifndef INTCOIN_P2P_H
#define INTCOIN_P2P_H

#include "primitives.h"
#include "block.h"
#include "transaction.h"
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <chrono>
#include <thread>

namespace intcoin {
namespace p2p {

/**
 * Network message types
 */
enum class MessageType : uint32_t {
    VERSION = 1,        // Initial handshake
    VERACK,            // Acknowledge version
    PING,              // Keepalive
    PONG,              // Pong response
    GETADDR,           // Request peer addresses
    ADDR,              // Peer addresses
    INV,               // Inventory advertisement
    GETDATA,           // Request data
    BLOCK,             // Block data
    TX,                // Transaction data
    GETBLOCKS,         // Request block hashes
    GETHEADERS,        // Request block headers
    HEADERS,           // Block headers
    MEMPOOL,           // Request mempool
    REJECT,            // Reject message
    NOTFOUND           // Data not found
};

/**
 * Peer address information
 */
struct PeerAddress {
    std::string ip;
    uint16_t port;
    uint64_t timestamp;
    uint64_t services;  // Service flags

    PeerAddress() : ip(""), port(0), timestamp(0), services(0) {}
    PeerAddress(const std::string& addr, uint16_t p)
        : ip(addr), port(p), timestamp(0), services(1) {}

    std::string to_string() const {
        return ip + ":" + std::to_string(port);
    }
};

/**
 * Network message header
 */
struct MessageHeader {
    uint32_t magic;         // Network magic bytes
    MessageType type;       // Message type
    uint32_t length;        // Payload length
    Hash256 checksum;       // Payload checksum

    MessageHeader() : magic(0), type(MessageType::VERSION), length(0), checksum{} {}

    std::vector<uint8_t> serialize() const;
    static MessageHeader deserialize(const std::vector<uint8_t>& data);
};

/**
 * Network message
 */
class Message {
public:
    MessageHeader header;
    std::vector<uint8_t> payload;

    Message() = default;
    Message(MessageType type, const std::vector<uint8_t>& data);

    std::vector<uint8_t> serialize() const;
    static Message deserialize(const std::vector<uint8_t>& data);

    Hash256 get_checksum() const;
};

/**
 * Inventory vector (announces objects)
 */
struct InvVector {
    enum class Type : uint32_t {
        ERROR = 0,
        TX = 1,
        BLOCK = 2,
        FILTERED_BLOCK = 3
    };

    Type type;
    Hash256 hash;

    InvVector() : type(Type::ERROR), hash{} {}
    InvVector(Type t, const Hash256& h) : type(t), hash(h) {}

    std::vector<uint8_t> serialize() const;
    static InvVector deserialize(const std::vector<uint8_t>& data);
};

/**
 * Peer connection
 */
class Peer {
public:
    PeerAddress address;
    bool connected;
    bool inbound;           // True if peer connected to us
    uint64_t last_seen;
    uint32_t version;
    std::string user_agent;
    uint32_t start_height;
    int socket_fd;          // Socket file descriptor
    uint32_t protocol_version;
    uint64_t services;

    Peer() : connected(false), inbound(false), last_seen(0), version(0), start_height(0),
             socket_fd(-1), protocol_version(0), services(0) {}
    explicit Peer(const PeerAddress& addr)
        : address(addr), connected(false), inbound(false), last_seen(0), version(0), start_height(0),
          socket_fd(-1), protocol_version(0), services(0) {}

    bool is_alive() const;
    void update_last_seen();
};

/**
 * P2P Network manager
 */
class Network {
public:
    Network(uint16_t port, bool is_testnet = false);
    ~Network();

    // Connection management
    bool start();
    void stop();
    bool connect_to_peer(const PeerAddress& addr);
    void disconnect_peer(const PeerAddress& addr);

    // Message handling
    void broadcast_block(const Block& block);
    void broadcast_transaction(const Transaction& tx);
    void send_message(const PeerAddress& addr, const Message& msg);

    // Peer discovery
    std::vector<PeerAddress> get_peers() const;
    void add_seed_node(const PeerAddress& addr);

    // Callbacks
    using BlockCallback = std::function<void(const Block&, const PeerAddress&)>;
    using TxCallback = std::function<void(const Transaction&, const PeerAddress&)>;
    using BlockLookupCallback = std::function<std::optional<Block>(const Hash256&)>;
    using TxLookupCallback = std::function<std::optional<Transaction>(const Hash256&)>;

    void set_block_callback(BlockCallback cb) { block_callback_ = cb; }
    void set_tx_callback(TxCallback cb) { tx_callback_ = cb; }
    void set_block_lookup_callback(BlockLookupCallback cb) { block_lookup_callback_ = cb; }
    void set_tx_lookup_callback(TxLookupCallback cb) { tx_lookup_callback_ = cb; }

    // Status
    size_t peer_count() const;
    bool is_running() const { return running_; }

private:
    uint16_t port_;
    bool is_testnet_;
    bool running_;

    int listen_socket_;
    std::thread discovery_thread_;
    std::thread maintenance_thread_;
    std::thread accept_thread_;

    std::vector<Peer> peers_;
    std::vector<PeerAddress> seed_nodes_;

    BlockCallback block_callback_;
    TxCallback tx_callback_;
    BlockLookupCallback block_lookup_callback_;
    TxLookupCallback tx_lookup_callback_;

    // Internal message handlers
    void handle_version(const Message& msg, const PeerAddress& from);
    void handle_inv(const Message& msg, const PeerAddress& from);
    void handle_getdata(const Message& msg, const PeerAddress& from);
    void handle_block(const Message& msg, const PeerAddress& from);
    void handle_tx(const Message& msg, const PeerAddress& from);

    // Peer management
    void discover_peers();
    void maintain_connections();
    Peer* find_peer(const PeerAddress& addr);
};

/**
 * Peer scoring for prioritization
 */
struct PeerScore {
    double reliability_score;       // 0.0-1.0 based on uptime
    double latency_score;          // 0.0-1.0 based on response time
    double bandwidth_score;        // 0.0-1.0 based on throughput
    uint32_t successful_requests;  // Successful message exchanges
    uint32_t failed_requests;      // Failed message exchanges
    uint64_t bytes_sent;
    uint64_t bytes_received;
    uint64_t avg_latency_ms;
    uint32_t misbehavior_score;    // Higher = worse behavior

    PeerScore()
        : reliability_score(0.5)
        , latency_score(0.5)
        , bandwidth_score(0.5)
        , successful_requests(0)
        , failed_requests(0)
        , bytes_sent(0)
        , bytes_received(0)
        , avg_latency_ms(0)
        , misbehavior_score(0)
    {}

    double get_overall_score() const {
        return (reliability_score + latency_score + bandwidth_score) / 3.0
               - (misbehavior_score * 0.01);
    }

    bool should_ban() const {
        return misbehavior_score >= 100;
    }
};

/**
 * Ban information for misbehaving peers
 */
struct BanEntry {
    std::string ip;
    uint64_t banned_until;          // Unix timestamp
    std::string reason;
    uint32_t ban_count;             // Number of times banned

    BanEntry() : banned_until(0), ban_count(0) {}

    bool is_banned() const {
        return banned_until > static_cast<uint64_t>(std::chrono::system_clock::now().time_since_epoch().count());
    }
};

/**
 * Connection quality metrics
 */
struct ConnectionQuality {
    uint64_t established_at;
    uint64_t last_message_sent;
    uint64_t last_message_received;
    uint32_t messages_sent;
    uint32_t messages_received;
    uint64_t bytes_sent;
    uint64_t bytes_received;
    double avg_rtt_ms;              // Round-trip time
    double packet_loss_rate;
    bool supports_compression;
    bool supports_ipv6;
    std::string protocol_version;

    ConnectionQuality()
        : established_at(0)
        , last_message_sent(0)
        , last_message_received(0)
        , messages_sent(0)
        , messages_received(0)
        , bytes_sent(0)
        , bytes_received(0)
        , avg_rtt_ms(0.0)
        , packet_loss_rate(0.0)
        , supports_compression(false)
        , supports_ipv6(false)
    {}
};

/**
 * Advanced peer discovery
 */
class PeerDiscovery {
public:
    PeerDiscovery();
    ~PeerDiscovery();

    /**
     * Discover peers via DNS seeds
     */
    std::vector<PeerAddress> discover_via_dns(
        const std::vector<std::string>& dns_seeds
    );

    /**
     * Discover peers via peer exchange
     */
    std::vector<PeerAddress> discover_via_peer_exchange();

    /**
     * Discover peers via local network broadcast
     */
    std::vector<PeerAddress> discover_via_local_broadcast();

    /**
     * Get recommended peers (sorted by score)
     */
    std::vector<PeerAddress> get_recommended_peers(size_t count = 10);

    /**
     * Add known peer
     */
    void add_known_peer(const PeerAddress& addr, const PeerScore& score);

    /**
     * Update peer score
     */
    void update_peer_score(const PeerAddress& addr, const PeerScore& score);

    /**
     * Get peer score
     */
    std::optional<PeerScore> get_peer_score(const PeerAddress& addr) const;

private:
    std::unordered_map<std::string, PeerScore> peer_scores_;
    mutable std::mutex discovery_mutex_;
};

/**
 * Bandwidth manager for rate limiting
 */
class BandwidthManager {
public:
    BandwidthManager(uint64_t max_upload_bps, uint64_t max_download_bps);
    ~BandwidthManager();

    /**
     * Check if can send data (rate limiting)
     */
    bool can_send(size_t bytes);

    /**
     * Check if can receive data
     */
    bool can_receive(size_t bytes);

    /**
     * Record sent data
     */
    void record_sent(size_t bytes);

    /**
     * Record received data
     */
    void record_received(size_t bytes);

    /**
     * Get current bandwidth usage
     */
    struct BandwidthStats {
        uint64_t bytes_sent_last_second;
        uint64_t bytes_received_last_second;
        uint64_t bytes_sent_total;
        uint64_t bytes_received_total;
        double upload_utilization;      // 0.0-1.0
        double download_utilization;    // 0.0-1.0
    };

    BandwidthStats get_stats() const;

    /**
     * Set bandwidth limits
     */
    void set_upload_limit(uint64_t bytes_per_second);
    void set_download_limit(uint64_t bytes_per_second);

private:
    uint64_t max_upload_bps_;
    uint64_t max_download_bps_;
    uint64_t bytes_sent_current_second_;
    uint64_t bytes_received_current_second_;
    uint64_t bytes_sent_total_;
    uint64_t bytes_received_total_;
    uint64_t current_second_;
    mutable std::mutex bandwidth_mutex_;

    void reset_second_counters();
};

/**
 * Message compression for bandwidth efficiency
 */
class MessageCompressor {
public:
    /**
     * Compress message payload
     */
    static std::vector<uint8_t> compress(const std::vector<uint8_t>& data);

    /**
     * Decompress message payload
     */
    static std::optional<std::vector<uint8_t>> decompress(
        const std::vector<uint8_t>& compressed_data
    );

    /**
     * Check if data should be compressed
     */
    static bool should_compress(const std::vector<uint8_t>& data);
};

/**
 * DDoS protection
 */
class DDoSProtection {
public:
    DDoSProtection();
    ~DDoSProtection();

    /**
     * Check if peer is flooding
     */
    bool is_flooding(const std::string& ip, MessageType msg_type);

    /**
     * Record message from peer
     */
    void record_message(const std::string& ip, MessageType msg_type);

    /**
     * Check if IP should be banned
     */
    bool should_ban(const std::string& ip);

    /**
     * Get flood statistics
     */
    struct FloodStats {
        uint32_t messages_last_second;
        uint32_t messages_last_minute;
        double messages_per_second;
        bool is_likely_flooding;
    };

    FloodStats get_flood_stats(const std::string& ip) const;

private:
    struct MessageCounter {
        std::vector<uint64_t> timestamps;
        uint32_t total_count;

        MessageCounter() : total_count(0) {}
    };

    std::unordered_map<std::string, MessageCounter> message_counters_;
    mutable std::mutex ddos_mutex_;

    static constexpr uint32_t MAX_MESSAGES_PER_SECOND = 100;
    static constexpr uint32_t MAX_MESSAGES_PER_MINUTE = 1000;
};

/**
 * Enhanced peer with advanced features
 */
class EnhancedPeer : public Peer {
public:
    PeerScore score;
    ConnectionQuality quality;
    bool banned;
    bool supports_compression;
    bool supports_bloom_filters;
    bool is_spv_node;               // Simplified Payment Verification node
    std::vector<std::string> supported_services;

    EnhancedPeer() : Peer(), banned(false), supports_compression(false),
                     supports_bloom_filters(false), is_spv_node(false) {}

    /**
     * Update peer score based on behavior
     */
    void update_score(bool success);

    /**
     * Check if peer should be disconnected
     */
    bool should_disconnect() const;

    /**
     * Get peer priority (higher = better)
     */
    double get_priority() const;
};

/**
 * Peer ban manager
 */
class PeerBanManager {
public:
    PeerBanManager();
    ~PeerBanManager();

    /**
     * Ban peer
     */
    void ban_peer(const std::string& ip, const std::string& reason, uint32_t duration_seconds = 86400);

    /**
     * Unban peer
     */
    void unban_peer(const std::string& ip);

    /**
     * Check if peer is banned
     */
    bool is_banned(const std::string& ip) const;

    /**
     * Get ban info
     */
    std::optional<BanEntry> get_ban_info(const std::string& ip) const;

    /**
     * List all banned peers
     */
    std::vector<BanEntry> list_banned_peers() const;

    /**
     * Clear expired bans
     */
    void clear_expired_bans();

private:
    std::unordered_map<std::string, BanEntry> banned_peers_;
    mutable std::mutex ban_mutex_;
};

/**
 * Protocol constants
 */
namespace protocol {
    static constexpr uint32_t PROTOCOL_VERSION = 1;
    static constexpr size_t MAX_MESSAGE_SIZE = 32 * 1024 * 1024;  // 32 MB
    static constexpr size_t MAX_PEERS = 125;
    static constexpr size_t MIN_PEERS = 8;
    static constexpr size_t MAX_OUTBOUND_CONNECTIONS = 8;
    static constexpr uint64_t TIMEOUT_SECONDS = 20;
    static constexpr uint64_t PING_INTERVAL_SECONDS = 120;
    static constexpr uint16_t DEFAULT_PORT = 9333;  // INTcoin P2P port (unique, not Bitcoin's 8333)
    static constexpr uint16_t DEFAULT_PORT_TESTNET = 19333;

    // New features (v1.3.0+)
    static constexpr uint64_t MAX_UPLOAD_BPS = 10 * 1024 * 1024;      // 10 MB/s
    static constexpr uint64_t MAX_DOWNLOAD_BPS = 50 * 1024 * 1024;    // 50 MB/s
    static constexpr uint32_t DEFAULT_BAN_TIME_SECONDS = 86400;       // 24 hours
    static constexpr uint32_t MAX_MISBEHAVIOR_SCORE = 100;
    static constexpr size_t MIN_COMPRESS_SIZE = 1024;                 // Compress if >= 1KB
}

} // namespace p2p
} // namespace intcoin

#endif // INTCOIN_P2P_H
