// Copyright (c) 2025 INTcoin Core (Maddison Lane)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
//
// Network Security - Buffer overflow protection, secure parsing, DoS prevention

#ifndef INTCOIN_NETWORK_SECURITY_H
#define INTCOIN_NETWORK_SECURITY_H

#include "primitives.h"
#include <cstdint>
#include <string>
#include <vector>
#include <optional>
#include <memory>
#include <chrono>
#include <map>
#include <set>

namespace intcoin {
namespace network {

/**
 * Protocol version constants
 */
constexpr uint32_t PROTOCOL_VERSION = 70016;  // Current protocol version
constexpr uint32_t MIN_PROTOCOL_VERSION = 70015;  // Minimum supported version
constexpr uint32_t MAX_PROTOCOL_VERSION = 70020;  // Maximum accepted version

/**
 * Message size limits (prevent buffer overflows)
 */
constexpr size_t MAX_MESSAGE_SIZE = 32 * 1024 * 1024;  // 32 MB max message
constexpr size_t MAX_HEADER_SIZE = 24;  // Message header size
constexpr size_t MAX_PAYLOAD_SIZE = MAX_MESSAGE_SIZE - MAX_HEADER_SIZE;
constexpr size_t MAX_INV_SIZE = 50000;  // Max inventory items per message
constexpr size_t MAX_ADDR_SIZE = 1000;  // Max addresses per message
constexpr size_t MAX_GETDATA_SIZE = 50000;  // Max getdata items
constexpr size_t MAX_HEADERS_SIZE = 2000;  // Max headers per message
constexpr size_t MAX_TX_SIZE = 1 * 1024 * 1024;  // 1 MB max transaction
constexpr size_t MAX_BLOCK_SIZE = 4 * 1024 * 1024;  // 4 MB max block

/**
 * Rate limiting constants (prevent DoS)
 */
constexpr uint32_t MAX_MESSAGES_PER_SECOND = 100;
constexpr uint32_t MAX_BYTES_PER_SECOND = 1 * 1024 * 1024;  // 1 MB/s per peer
constexpr uint32_t MAX_CONNECTIONS_PER_IP = 8;
constexpr uint32_t MAX_OUTBOUND_CONNECTIONS = 8;
constexpr uint32_t MAX_INBOUND_CONNECTIONS = 125;

/**
 * Timeout constants
 */
constexpr uint32_t VERSION_HANDSHAKE_TIMEOUT = 60;  // seconds
constexpr uint32_t PING_TIMEOUT = 20;  // seconds
constexpr uint32_t IDLE_TIMEOUT = 90 * 60;  // 90 minutes

/**
 * Safe buffer with bounds checking
 * Prevents buffer overflow attacks
 */
class SafeBuffer {
public:
    SafeBuffer() : position_(0) {}
    explicit SafeBuffer(const std::vector<uint8_t>& data) : data_(data), position_(0) {}
    explicit SafeBuffer(size_t reserve_size) : position_(0) {
        data_.reserve(reserve_size);
    }

    // Read operations (with bounds checking)
    bool read_uint8(uint8_t& value);
    bool read_uint16(uint16_t& value);
    bool read_uint32(uint32_t& value);
    bool read_uint64(uint64_t& value);
    bool read_var_int(uint64_t& value);
    bool read_bytes(std::vector<uint8_t>& bytes, size_t length);
    bool read_string(std::string& str, size_t max_length);
    bool read_hash256(Hash256& hash);

    // Write operations (with overflow protection)
    bool write_uint8(uint8_t value);
    bool write_uint16(uint16_t value);
    bool write_uint32(uint32_t value);
    bool write_uint64(uint64_t value);
    bool write_var_int(uint64_t value);
    bool write_bytes(const std::vector<uint8_t>& bytes);
    bool write_string(const std::string& str);
    bool write_hash256(const Hash256& hash);

    // Position management
    size_t position() const { return position_; }
    size_t remaining() const { return data_.size() - position_; }
    size_t size() const { return data_.size(); }
    bool eof() const { return position_ >= data_.size(); }
    void reset() { position_ = 0; }
    bool seek(size_t pos);

    // Data access
    const std::vector<uint8_t>& data() const { return data_; }
    std::vector<uint8_t>& data() { return data_; }

    // Validation
    bool validate_remaining(size_t required) const {
        return remaining() >= required;
    }

private:
    std::vector<uint8_t> data_;
    size_t position_;
};

/**
 * Message header with validation
 */
struct MessageHeader {
    uint32_t magic;           // Network magic bytes
    char command[12];         // Command name (null-terminated)
    uint32_t payload_length;  // Payload size
    uint32_t checksum;        // Payload checksum

    // Validation
    bool is_valid() const;
    bool validate_size() const;
    bool validate_command() const;
    std::string get_command() const;

    // Serialization
    bool serialize(SafeBuffer& buffer) const;
    bool deserialize(SafeBuffer& buffer);
};

/**
 * Secure message parser with overflow protection
 */
class SecureMessageParser {
public:
    SecureMessageParser();

    // Parse message with full validation
    struct ParseResult {
        bool success;
        std::string error;
        std::string command;
        std::vector<uint8_t> payload;
        uint32_t bytes_consumed;
    };

    ParseResult parse_message(const std::vector<uint8_t>& data, size_t max_size);

    // Validation
    bool validate_header(const MessageHeader& header) const;
    bool validate_payload_size(const std::string& command, size_t size) const;
    bool validate_checksum(const std::vector<uint8_t>& payload, uint32_t checksum) const;

    // Command-specific limits
    size_t get_max_payload_size(const std::string& command) const;

private:
    std::map<std::string, size_t> command_size_limits_;

    void initialize_size_limits();
    uint32_t calculate_checksum(const std::vector<uint8_t>& data) const;
};

/**
 * Protocol version negotiation with security checks
 */
class ProtocolVersionNegotiator {
public:
    ProtocolVersionNegotiator();

    struct VersionInfo {
        uint32_t protocol_version;
        uint64_t services;
        int64_t timestamp;
        std::string user_agent;
        uint32_t start_height;
        bool relay;
    };

    // Version negotiation
    bool validate_version(const VersionInfo& version, std::string& error) const;
    bool is_compatible(uint32_t peer_version) const;
    uint32_t get_common_version(uint32_t peer_version) const;

    // Security checks
    bool validate_timestamp(int64_t timestamp) const;
    bool validate_user_agent(const std::string& user_agent) const;
    bool validate_services(uint64_t services) const;

    // Information leakage prevention
    VersionInfo create_version_message(bool minimal = false) const;

private:
    uint32_t our_version_;
    std::set<std::string> blocked_user_agents_;

    static constexpr int64_t MAX_TIMESTAMP_DRIFT = 2 * 60 * 60;  // 2 hours
};

/**
 * Rate limiter to prevent DoS attacks
 */
class RateLimiter {
public:
    RateLimiter(uint32_t max_per_second, uint32_t burst_size = 0);

    // Check if action is allowed
    bool allow();
    bool allow_bytes(size_t bytes);

    // Reset (for testing or peer disconnect)
    void reset();

    // Statistics
    uint64_t get_total_allowed() const { return total_allowed_; }
    uint64_t get_total_rejected() const { return total_rejected_; }
    double get_current_rate() const;

private:
    uint32_t max_per_second_;
    uint32_t burst_size_;
    uint64_t tokens_;
    std::chrono::steady_clock::time_point last_refill_;
    uint64_t total_allowed_;
    uint64_t total_rejected_;

    void refill_tokens();
};

/**
 * Per-peer statistics and DoS prevention
 */
class PeerSecurityTracker {
public:
    explicit PeerSecurityTracker(const std::string& peer_id);

    // Message tracking
    bool record_message(const std::string& command, size_t size);
    bool is_misbehaving() const;
    void add_misbehavior(uint32_t score, const std::string& reason);

    // Rate limiting
    bool check_message_rate();
    bool check_bandwidth(size_t bytes);

    // Statistics
    struct Stats {
        uint64_t messages_sent;
        uint64_t messages_received;
        uint64_t bytes_sent;
        uint64_t bytes_received;
        uint32_t misbehavior_score;
        std::vector<std::string> misbehavior_reasons;
        double messages_per_second;
        double bytes_per_second;
    };
    Stats get_stats() const;

    // Thresholds
    static constexpr uint32_t MISBEHAVIOR_BAN_THRESHOLD = 100;
    static constexpr uint32_t MISBEHAVIOR_DISCONNECT_THRESHOLD = 50;

private:
    std::string peer_id_;
    uint64_t messages_sent_;
    uint64_t messages_received_;
    uint64_t bytes_sent_;
    uint64_t bytes_received_;
    uint32_t misbehavior_score_;
    std::vector<std::string> misbehavior_reasons_;

    RateLimiter message_rate_limiter_;
    RateLimiter bandwidth_limiter_;

    std::chrono::steady_clock::time_point creation_time_;
    std::chrono::steady_clock::time_point last_message_time_;
};

/**
 * Proof-of-Work validation for spam prevention
 */
class ProofOfWorkValidator {
public:
    ProofOfWorkValidator();

    // Validate block PoW
    bool validate_block_pow(const Hash256& block_hash, const Hash256& target) const;

    // Validate transaction PoW (optional anti-spam)
    bool validate_transaction_pow(const Hash256& tx_hash, uint32_t difficulty) const;

    // Check if hash meets difficulty target
    bool meets_target(const Hash256& hash, const Hash256& target) const;
    bool meets_difficulty(const Hash256& hash, uint32_t difficulty) const;

    // Calculate difficulty from target
    static uint32_t target_to_difficulty(const Hash256& target);
    static Hash256 difficulty_to_target(uint32_t difficulty);

private:
    bool compare_hashes(const Hash256& a, const Hash256& b) const;
};

/**
 * Amplification attack prevention
 */
class AmplificationAttackPrevention {
public:
    AmplificationAttackPrevention();

    // Check if response would amplify request
    bool validate_response_size(size_t request_size, size_t response_size) const;

    // Limit response based on request
    size_t get_max_response_size(const std::string& command, size_t request_size) const;

    // Track amplification attempts
    void record_amplification_attempt(const std::string& peer_id);
    bool is_amplification_attacker(const std::string& peer_id) const;

private:
    static constexpr double MAX_AMPLIFICATION_FACTOR = 10.0;
    std::map<std::string, uint32_t> amplification_attempts_;
};

/**
 * Resource exhaustion prevention
 */
class ResourceExhaustionPrevention {
public:
    ResourceExhaustionPrevention();

    // Memory limits
    struct MemoryLimits {
        size_t max_mempool_size;        // 300 MB
        size_t max_orphan_tx_size;      // 100 MB
        size_t max_peer_buffers;        // 10 MB per peer
        size_t max_signature_cache;     // 40 MB
        size_t max_script_cache;        // 40 MB
    };

    // Check resource usage
    bool check_memory_usage(size_t current, size_t limit) const;
    bool check_cpu_usage() const;
    bool check_disk_usage() const;

    // Limit enforcement
    bool can_accept_transaction(size_t tx_size) const;
    bool can_accept_block(size_t block_size) const;
    bool can_allocate_peer_buffer(size_t size) const;

    // Get limits
    MemoryLimits get_memory_limits() const;

    // Update resource usage
    void update_mempool_size(size_t size);
    void update_orphan_tx_size(size_t size);

private:
    MemoryLimits limits_;
    size_t current_mempool_size_;
    size_t current_orphan_tx_size_;
};

/**
 * Information leakage prevention
 */
class InformationLeakagePrevention {
public:
    InformationLeakagePrevention();

    // Sanitize data before sending
    std::string sanitize_error_message(const std::string& error) const;
    std::vector<uint8_t> sanitize_timestamp(int64_t timestamp) const;
    std::string sanitize_user_agent() const;

    // Prevent timing attacks
    template<typename Func>
    auto constant_time_execute(Func&& func, std::chrono::microseconds target_time) const;

    // Privacy-preserving responses
    bool should_reveal_peer_count() const;
    bool should_reveal_mempool_size() const;
    bool should_reveal_balance() const;

private:
    // Blacklist of sensitive information patterns
    std::vector<std::string> sensitive_patterns_;

    void initialize_sensitive_patterns();
};

/**
 * Complete network security manager
 */
class NetworkSecurityManager {
public:
    static NetworkSecurityManager& instance();

    // Initialize security subsystems
    void initialize();
    void shutdown();

    // Access components
    SecureMessageParser& message_parser() { return *message_parser_; }
    ProtocolVersionNegotiator& version_negotiator() { return *version_negotiator_; }
    ProofOfWorkValidator& pow_validator() { return *pow_validator_; }
    AmplificationAttackPrevention& amplification_prevention() { return *amplification_prevention_; }
    ResourceExhaustionPrevention& resource_prevention() { return *resource_prevention_; }
    InformationLeakagePrevention& leakage_prevention() { return *leakage_prevention_; }

    // Peer management
    std::shared_ptr<PeerSecurityTracker> get_peer_tracker(const std::string& peer_id);
    void remove_peer_tracker(const std::string& peer_id);

    // Global security checks
    bool validate_incoming_message(const std::string& peer_id,
                                     const std::vector<uint8_t>& data,
                                     std::string& error);

    bool validate_outgoing_message(const std::string& peer_id,
                                     const std::string& command,
                                     const std::vector<uint8_t>& payload,
                                     std::string& error);

    // Ban management
    void ban_peer(const std::string& peer_id, const std::string& reason, uint32_t duration_seconds = 86400);
    bool is_banned(const std::string& peer_id) const;
    void unban_peer(const std::string& peer_id);

    // Statistics
    struct SecurityStats {
        uint64_t messages_validated;
        uint64_t messages_rejected;
        uint64_t buffer_overflows_prevented;
        uint64_t dos_attempts_blocked;
        uint64_t banned_peers;
        std::map<std::string, uint64_t> rejection_reasons;
    };
    SecurityStats get_stats() const;

private:
    NetworkSecurityManager();
    ~NetworkSecurityManager() = default;
    NetworkSecurityManager(const NetworkSecurityManager&) = delete;
    NetworkSecurityManager& operator=(const NetworkSecurityManager&) = delete;

    std::unique_ptr<SecureMessageParser> message_parser_;
    std::unique_ptr<ProtocolVersionNegotiator> version_negotiator_;
    std::unique_ptr<ProofOfWorkValidator> pow_validator_;
    std::unique_ptr<AmplificationAttackPrevention> amplification_prevention_;
    std::unique_ptr<ResourceExhaustionPrevention> resource_prevention_;
    std::unique_ptr<InformationLeakagePrevention> leakage_prevention_;

    std::map<std::string, std::shared_ptr<PeerSecurityTracker>> peer_trackers_;
    std::map<std::string, std::chrono::steady_clock::time_point> banned_peers_;

    mutable std::mutex mutex_;

    // Statistics
    uint64_t messages_validated_;
    uint64_t messages_rejected_;
    uint64_t buffer_overflows_prevented_;
    std::map<std::string, uint64_t> rejection_reasons_;
};

} // namespace network
} // namespace intcoin

#endif // INTCOIN_NETWORK_SECURITY_H
