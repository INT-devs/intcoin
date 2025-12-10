/*
 * Copyright (c) 2025 INTcoin Team (Neil Adamson)
 * MIT License
 * P2P Network Protocol
 */

#ifndef INTCOIN_NETWORK_H
#define INTCOIN_NETWORK_H

#include "types.h"
#include "block.h"
#include "transaction.h"
#include <string>
#include <vector>
#include <cstdint>
#include <chrono>
#include <memory>
#include <map>
#include <mutex>

namespace intcoin {

// ============================================================================
// Network Constants
// ============================================================================

namespace network {

/// Network magic bytes (0x494E5443 = "INTC")
constexpr uint32_t MAINNET_MAGIC = 0x494E5443;
constexpr uint32_t TESTNET_MAGIC = 0x54494E54; // "TINT"
constexpr uint32_t REGTEST_MAGIC = 0x52494E54; // "RINT"

/// Default ports (using 2210-2220 range to avoid conflicts)
constexpr uint16_t MAINNET_P2P_PORT = 2210;
constexpr uint16_t MAINNET_RPC_PORT = 2211;
constexpr uint16_t TESTNET_P2P_PORT = 2212;
constexpr uint16_t TESTNET_RPC_PORT = 2213;
constexpr uint16_t REGTEST_P2P_PORT = 2214;
constexpr uint16_t REGTEST_RPC_PORT = 2215;

/// Protocol version
constexpr uint32_t PROTOCOL_VERSION = 70001;

/// Minimum protocol version we support
constexpr uint32_t MIN_PROTOCOL_VERSION = 70001;

/// Maximum message size (32 MB)
constexpr size_t MAX_MESSAGE_SIZE = 32 * 1024 * 1024;

/// Maximum headers in single message
constexpr size_t MAX_HEADERS_COUNT = 2000;

/// Maximum inventory items
constexpr size_t MAX_INV_COUNT = 50000;

/// Connection timeout (30 seconds)
constexpr auto CONNECTION_TIMEOUT = std::chrono::seconds(30);

/// Ping interval (2 minutes)
constexpr auto PING_INTERVAL = std::chrono::seconds(120);

/// Maximum connected peers
constexpr size_t MAX_OUTBOUND_CONNECTIONS = 8;
constexpr size_t MAX_INBOUND_CONNECTIONS = 125;

} // namespace network

// ============================================================================
// Network Address
// ============================================================================

struct NetworkAddress {
    /// Services provided by this node
    uint64_t services;

    /// IP address (IPv4 or IPv6)
    std::array<uint8_t, 16> ip;

    /// Port
    uint16_t port;

    /// Timestamp (when address was last seen)
    uint64_t timestamp;

    /// Default constructor
    NetworkAddress();

    /// Constructor from IP and port
    NetworkAddress(const std::string& ip_str, uint16_t port);

    /// Serialize
    std::vector<uint8_t> Serialize() const;

    /// Deserialize
    static Result<NetworkAddress> Deserialize(const std::vector<uint8_t>& data);

    /// Convert to string (IP:port)
    std::string ToString() const;

    /// Check if IPv4
    bool IsIPv4() const;

    /// Check if IPv6
    bool IsIPv6() const;

    /// Check if Tor
    bool IsTor() const;

    /// Check if local
    bool IsLocal() const;

    /// Check if routable
    bool IsRoutable() const;
};

// ============================================================================
// Service Flags
// ============================================================================

enum class ServiceFlags : uint64_t {
    NONE = 0,
    NODE_NETWORK = (1 << 0),      // Full node (can serve blocks)
    NODE_BLOOM = (1 << 2),         // Supports bloom filtering
    NODE_WITNESS = (1 << 3),       // Supports segregated witness
    NODE_COMPACT_FILTERS = (1 << 6), // Supports compact filters
    NODE_NETWORK_LIMITED = (1 << 10), // Pruned node with recent blocks
};

// ============================================================================
// Message Types
// ============================================================================

enum class MessageType {
    VERSION,        // Protocol version handshake
    VERACK,        // Version acknowledgment
    ADDR,          // Peer address announcement
    INV,           // Inventory (blocks, transactions)
    GETDATA,       // Request data
    NOTFOUND,      // Data not found
    GETBLOCKS,     // Request block hashes
    GETHEADERS,    // Request block headers
    HEADERS,       // Block headers
    BLOCK,         // Full block
    TX,            // Transaction
    MEMPOOL,       // Request mempool transactions
    PING,          // Keep-alive ping
    PONG,          // Keep-alive pong
    REJECT,        // Reject message
    SENDHEADERS,   // Request headers instead of inv
    FEEFILTER,     // Set minimum fee filter
    SENDCMPCT,     // Compact block relay
    CMPCTBLOCK,    // Compact block
    GETBLOCKTXN,   // Request block transactions
    BLOCKTXN,      // Block transactions
};

// ============================================================================
// Network Message
// ============================================================================

struct NetworkMessage {
    /// Magic bytes
    uint32_t magic;

    /// Command name
    std::string command;

    /// Payload length
    uint32_t length;

    /// Checksum (first 4 bytes of SHA3-256 hash)
    uint32_t checksum;

    /// Payload data
    std::vector<uint8_t> payload;

    /// Default constructor
    NetworkMessage();

    /// Constructor with command and payload
    NetworkMessage(uint32_t magic, const std::string& cmd,
                  const std::vector<uint8_t>& data);

    /// Serialize message
    std::vector<uint8_t> Serialize() const;

    /// Deserialize message
    static Result<NetworkMessage> Deserialize(const std::vector<uint8_t>& data);

    /// Verify checksum
    bool VerifyChecksum() const;

    /// Calculate checksum
    static uint32_t CalculateChecksum(const std::vector<uint8_t>& data);
};

// ============================================================================
// Inventory Vector (for INV messages)
// ============================================================================

enum class InvType : uint32_t {
    ERROR = 0,
    TX = 1,
    BLOCK = 2,
    FILTERED_BLOCK = 3,
    COMPACT_BLOCK = 4,
};

struct InvVector {
    InvType type;
    uint256 hash;

    /// Serialize
    std::vector<uint8_t> Serialize() const;

    /// Deserialize
    static Result<InvVector> Deserialize(const std::vector<uint8_t>& data);
};

// ============================================================================
// Peer Connection
// ============================================================================

class Peer {
public:
    /// Peer ID
    uint64_t id;

    /// Network address
    NetworkAddress address;

    /// Protocol version
    uint32_t version;

    /// Services
    uint64_t services;

    /// Connection time
    std::chrono::system_clock::time_point connect_time;

    /// Last message time
    std::chrono::system_clock::time_point last_message_time;

    /// Inbound or outbound
    bool inbound;

    /// Bytes sent
    uint64_t bytes_sent;

    /// Bytes received
    uint64_t bytes_received;

    /// Ban score (misbehavior)
    int ban_score;

    /// Peer reputation score (0-100, higher is better)
    int reputation_score;

    /// Message rate limit (messages per second)
    double message_rate;

    /// Last message timestamps for rate limiting
    std::vector<std::chrono::system_clock::time_point> recent_messages;

    /// Send message to peer
    Result<void> SendMessage(const NetworkMessage& msg);

    /// Receive message from peer
    Result<NetworkMessage> ReceiveMessage();

    /// Disconnect
    void Disconnect();

    /// Check if connected
    bool IsConnected() const;

    /// Get ping time
    std::chrono::milliseconds GetPingTime() const;

    /// Increase ban score
    void IncreaseBanScore(int points);

    /// Decrease ban score (reward good behavior)
    void DecreaseBanScore(int points);

    /// Update reputation score
    void UpdateReputation(int delta);

    /// Check if rate limited
    bool IsRateLimited() const;

    /// Record message for rate limiting
    void RecordMessage();

    /// Get reputation level (0-5, higher is better)
    int GetReputationLevel() const;
};

// ============================================================================
// P2P Node
// ============================================================================

class P2PNode {
public:
    /// Constructor
    P2PNode(uint32_t network_magic, uint16_t listen_port);

    /// Destructor
    ~P2PNode();

    /// Start P2P node
    Result<void> Start();

    /// Stop P2P node
    void Stop();

    /// Connect to peer
    Result<std::shared_ptr<Peer>> ConnectToPeer(const NetworkAddress& address);

    /// Disconnect from peer
    void DisconnectPeer(uint64_t peer_id);

    /// Broadcast message to all peers
    void BroadcastMessage(const NetworkMessage& msg);

    /// Send message to specific peer
    Result<void> SendToPeer(uint64_t peer_id, const NetworkMessage& msg);

    /// Get connected peers
    std::vector<std::shared_ptr<Peer>> GetPeers() const;

    /// Get peer count
    size_t GetPeerCount() const;

    /// Ban peer
    void BanPeer(const NetworkAddress& address, std::chrono::seconds duration);

    /// Check if peer is banned
    bool IsBanned(const NetworkAddress& address) const;

    /// Add seed node
    void AddSeedNode(const NetworkAddress& address);

    /// Discover peers
    Result<void> DiscoverPeers();

    /// Broadcast new block to all peers (sends INV message)
    void BroadcastBlock(const uint256& block_hash);

    /// Broadcast new transaction to all peers (sends INV message)
    void BroadcastTransaction(const uint256& tx_hash);

    // ------------------------------------------------------------------------
    // Network Security
    // ------------------------------------------------------------------------

    /// Enable/disable rate limiting
    void SetRateLimiting(bool enabled);

    /// Set rate limit (messages per second per peer)
    void SetRateLimit(double messages_per_second);

    /// Check if peer is rate limited
    bool IsPeerRateLimited(uint64_t peer_id) const;

    /// Get peer reputation
    int GetPeerReputation(uint64_t peer_id) const;

    /// Update peer reputation
    void UpdatePeerReputation(uint64_t peer_id, int delta);

    /// Get trusted peers (reputation >= 80)
    std::vector<uint64_t> GetTrustedPeers() const;

    /// Get suspicious peers (reputation < 40)
    std::vector<uint64_t> GetSuspiciousPeers() const;

    /// Auto-ban suspicious peers
    void AutoBanSuspiciousPeers();

    // ------------------------------------------------------------------------
    // DDoS Protection
    // ------------------------------------------------------------------------

    /// Check for DoS attack
    bool IsUnderDoSAttack() const;

    /// Get connection rate (connections per minute)
    double GetConnectionRate() const;

    /// Get message rate (messages per second)
    double GetMessageRate() const;

    /// Enable/disable connection throttling
    void SetConnectionThrottling(bool enabled);

    /// Set max connections per IP
    void SetMaxConnectionsPerIP(size_t max);

    /// Block IP address
    void BlockIP(const std::string& ip, std::chrono::seconds duration);

    /// Unblock IP address
    void UnblockIP(const std::string& ip);

    /// Check if IP is blocked
    bool IsIPBlocked(const std::string& ip) const;

    /// Get blocked IPs
    std::vector<std::string> GetBlockedIPs() const;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

// ============================================================================
// Message Handlers
// ============================================================================

class MessageHandler {
public:
    /// Handle VERSION message
    static Result<void> HandleVersion(Peer& peer,
                                     const std::vector<uint8_t>& payload);

    /// Handle VERACK message
    static Result<void> HandleVerack(Peer& peer);

    /// Handle ADDR message
    static Result<void> HandleAddr(const std::vector<uint8_t>& payload);

    /// Handle INV message
    static Result<void> HandleInv(Peer& peer,
                                 const std::vector<uint8_t>& payload,
                                 class Blockchain* blockchain);

    /// Handle GETDATA message
    static Result<void> HandleGetData(Peer& peer,
                                     const std::vector<uint8_t>& payload,
                                     class Blockchain* blockchain);

    /// Handle BLOCK message
    static Result<void> HandleBlock(Peer& peer,
                                   const std::vector<uint8_t>& payload);

    /// Handle TX message
    static Result<void> HandleTx(Peer& peer,
                                const std::vector<uint8_t>& payload);

    /// Handle GETHEADERS message
    static Result<void> HandleGetHeaders(Peer& peer,
                                        const std::vector<uint8_t>& payload,
                                        class Blockchain* blockchain);

    /// Handle HEADERS message
    static Result<void> HandleHeaders(Peer& peer,
                                     const std::vector<uint8_t>& payload,
                                     class Blockchain* blockchain);

    /// Handle PING message
    static Result<void> HandlePing(Peer& peer,
                                  const std::vector<uint8_t>& payload);

    /// Handle PONG message
    static Result<void> HandlePong(Peer& peer,
                                  const std::vector<uint8_t>& payload);
};

// ============================================================================
// Peer Reputation Manager
// ============================================================================

class PeerReputationManager {
public:
    /// Constructor
    PeerReputationManager();

    /// Update peer reputation
    void UpdateReputation(uint64_t peer_id, int delta);

    /// Get peer reputation
    int GetReputation(uint64_t peer_id) const;

    /// Get reputation level (0-5)
    int GetReputationLevel(uint64_t peer_id) const;

    /// Record good behavior
    void RecordGoodBehavior(uint64_t peer_id);

    /// Record bad behavior
    void RecordBadBehavior(uint64_t peer_id);

    /// Get trusted peers
    std::vector<uint64_t> GetTrustedPeers() const;

    /// Get suspicious peers
    std::vector<uint64_t> GetSuspiciousPeers() const;

    /// Clear reputation for peer
    void ClearReputation(uint64_t peer_id);

    /// Get all reputations
    std::map<uint64_t, int> GetAllReputations() const;

private:
    std::map<uint64_t, int> reputations_;
    mutable std::mutex mutex_;
};

// ============================================================================
// Rate Limiter
// ============================================================================

class RateLimiter {
public:
    /// Constructor
    RateLimiter(double max_rate);  // max_rate = messages per second

    /// Check if action is allowed
    bool AllowAction(uint64_t id);

    /// Record action
    void RecordAction(uint64_t id);

    /// Get current rate
    double GetRate(uint64_t id) const;

    /// Check if rate limited
    bool IsRateLimited(uint64_t id) const;

    /// Set max rate
    void SetMaxRate(double max_rate);

    /// Clear history for ID
    void Clear(uint64_t id);

    /// Clear all
    void ClearAll();

private:
    double max_rate_;
    std::map<uint64_t, std::vector<std::chrono::system_clock::time_point>> actions_;
    mutable std::mutex mutex_;
};

// ============================================================================
// IP Blocker
// ============================================================================

class IPBlocker {
public:
    /// Constructor
    IPBlocker();

    /// Block IP
    void BlockIP(const std::string& ip, std::chrono::seconds duration);

    /// Unblock IP
    void UnblockIP(const std::string& ip);

    /// Check if blocked
    bool IsBlocked(const std::string& ip) const;

    /// Get blocked IPs
    std::vector<std::string> GetBlockedIPs() const;

    /// Clean expired blocks
    void CleanExpired();

    /// Clear all blocks
    void ClearAll();

private:
    struct BlockInfo {
        std::chrono::system_clock::time_point block_time;
        std::chrono::seconds duration;
    };

    std::map<std::string, BlockInfo> blocked_ips_;
    mutable std::mutex mutex_;
};

// ============================================================================
// Connection Throttler
// ============================================================================

class ConnectionThrottler {
public:
    /// Constructor
    ConnectionThrottler(size_t max_per_ip, std::chrono::seconds window);

    /// Check if connection allowed
    bool AllowConnection(const std::string& ip);

    /// Record connection
    void RecordConnection(const std::string& ip);

    /// Get connection count for IP
    size_t GetConnectionCount(const std::string& ip) const;

    /// Set max connections per IP
    void SetMaxPerIP(size_t max);

    /// Clean old connections
    void CleanOld();

    /// Clear all
    void ClearAll();

private:
    size_t max_per_ip_;
    std::chrono::seconds window_;
    std::map<std::string, std::vector<std::chrono::system_clock::time_point>> connections_;
    mutable std::mutex mutex_;
};

// ============================================================================
// DoS Detector
// ============================================================================

class DoSDetector {
public:
    /// Constructor
    DoSDetector();

    /// Record connection attempt
    void RecordConnection();

    /// Record message
    void RecordMessage();

    /// Check if under attack
    bool IsUnderAttack() const;

    /// Get connection rate (per minute)
    double GetConnectionRate() const;

    /// Get message rate (per second)
    double GetMessageRate() const;

    /// Set attack thresholds
    void SetThresholds(double max_conn_rate, double max_msg_rate);

    /// Reset statistics
    void Reset();

private:
    std::vector<std::chrono::system_clock::time_point> connections_;
    std::vector<std::chrono::system_clock::time_point> messages_;
    double max_connection_rate_;  // per minute
    double max_message_rate_;     // per second
    mutable std::mutex mutex_;
};

// ============================================================================
// Peer Discovery
// ============================================================================

class PeerDiscovery {
public:
    /// DNS seed discovery
    static Result<std::vector<NetworkAddress>> DNSSeedQuery(
        const std::string& dns_seed);

    /// Get hardcoded seed nodes
    static std::vector<NetworkAddress> GetSeedNodes(bool testnet = false);

    /// Save peer addresses to disk
    static Result<void> SavePeerAddresses(
        const std::vector<NetworkAddress>& addresses);

    /// Load peer addresses from disk
    static Result<std::vector<NetworkAddress>> LoadPeerAddresses();
};

// ============================================================================
// Network Utilities
// ============================================================================

/// Convert IP string to bytes
Result<std::array<uint8_t, 16>> ParseIPAddress(const std::string& ip);

/// Convert IP bytes to string
std::string IPAddressToString(const std::array<uint8_t, 16>& ip);

/// Check if port is valid
bool IsValidPort(uint16_t port);

/// Get local IP addresses
std::vector<NetworkAddress> GetLocalAddresses();

} // namespace intcoin

#endif // INTCOIN_NETWORK_H
