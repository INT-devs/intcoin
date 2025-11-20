// Copyright (c) 2025 INTcoin Core (Maddison Lane)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
//
// I2P (Invisible Internet Project) Network Integration
// SAM v3.1 Protocol Implementation

#ifndef INTCOIN_I2P_H
#define INTCOIN_I2P_H

#include "primitives.h"
#include <string>
#include <vector>
#include <memory>
#include <optional>
#include <functional>
#include <map>
#include <mutex>

namespace intcoin {
namespace i2p {

/**
 * I2P network ports (independent from Bitcoin and other projects)
 */
constexpr uint16_t DEFAULT_SAM_PORT = 9336;      // SAM bridge (not 7656)
constexpr uint16_t DEFAULT_ROUTER_PORT = 9337;   // I2P router
constexpr uint16_t DEFAULT_MAINNET_PORT = 9333;  // P2P over I2P

/**
 * I2P destination (like an IP address in clearnet)
 * Base32 encoded public key: xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx.b32.i2p
 */
struct I2PDestination {
    std::string base32_address;  // xxx.b32.i2p
    std::vector<uint8_t> public_key;  // Full I2P destination key (387+ bytes)
    bool is_transient;  // Temporary or persistent

    I2PDestination() : is_transient(true) {}

    std::string to_string() const { return base32_address; }
    bool is_valid() const { return !base32_address.empty() && base32_address.find(".b32.i2p") != std::string::npos; }
};

/**
 * I2P session configuration
 */
struct I2PSessionConfig {
    std::string session_name;
    uint16_t sam_port;
    uint16_t router_port;
    std::string sam_host;

    // Tunnel configuration
    uint32_t tunnel_length;     // Hops (default: 3)
    uint32_t tunnel_quantity;   // Backup tunnels (default: 2)
    uint32_t tunnel_variance;   // Randomness (default: 0)
    uint32_t tunnel_backup_quantity;

    // Bandwidth limits (KB/s, 0 = unlimited)
    uint32_t inbound_bandwidth;
    uint32_t outbound_bandwidth;

    // Privacy settings
    bool transient;  // Generate new keys each session
    bool reduce_idle;
    uint32_t idle_timeout;  // Seconds before closing idle tunnels

    I2PSessionConfig()
        : session_name("intcoin-mainnet")
        , sam_port(DEFAULT_SAM_PORT)
        , router_port(DEFAULT_ROUTER_PORT)
        , sam_host("127.0.0.1")
        , tunnel_length(3)
        , tunnel_quantity(2)
        , tunnel_variance(0)
        , tunnel_backup_quantity(1)
        , inbound_bandwidth(0)
        , outbound_bandwidth(0)
        , transient(false)
        , reduce_idle(true)
        , idle_timeout(300)
    {}
};

/**
 * I2P connection state
 */
enum class I2PConnectionState {
    DISCONNECTED,
    CONNECTING,
    CONNECTED,
    SESSION_CREATED,
    READY,
    ERROR
};

/**
 * SAM v3.1 Protocol Implementation
 * Simple Anonymous Messaging interface to I2P router
 */
class SAMSession {
public:
    SAMSession(const I2PSessionConfig& config);
    ~SAMSession();

    // Session lifecycle
    bool connect();
    bool create_session();
    bool close_session();
    void disconnect();

    // Connection management
    std::optional<int> stream_connect(const std::string& destination, uint16_t port);
    std::optional<int> stream_accept();
    bool stream_forward(uint16_t local_port);

    // Destination management
    I2PDestination get_my_destination() const { return my_destination_; }
    std::string get_destination_base32() const;
    std::vector<uint8_t> get_destination_key() const;
    bool save_destination_keys(const std::string& filepath) const;
    bool load_destination_keys(const std::string& filepath);

    // Status
    I2PConnectionState get_state() const { return state_; }
    bool is_ready() const { return state_ == I2PConnectionState::READY; }
    std::string get_error() const { return last_error_; }

    // Statistics
    struct TunnelStats {
        uint32_t inbound_tunnels;
        uint32_t outbound_tunnels;
        uint64_t bytes_sent;
        uint64_t bytes_received;
        uint32_t peer_count;
    };
    TunnelStats get_stats() const;

private:
    I2PSessionConfig config_;
    I2PConnectionState state_;
    std::string last_error_;
    int sam_socket_;
    I2PDestination my_destination_;
    std::string session_id_;

    // SAM protocol commands
    bool send_command(const std::string& command, std::string& response);
    bool handshake();
    bool generate_destination();

    // Protocol helpers
    std::string build_session_options() const;
    bool parse_destination_reply(const std::string& reply);
};

/**
 * I2P Network Manager
 * High-level interface for I2P networking
 */
class I2PManager {
public:
    explicit I2PManager(const I2PSessionConfig& config);
    ~I2PManager();

    // Initialization
    bool initialize();
    void shutdown();

    // Connection to I2P network
    bool connect_to_i2p();
    void disconnect_from_i2p();
    bool is_connected() const;

    // Peer connections
    std::optional<int> connect_to_peer(const std::string& i2p_address, uint16_t port);
    bool accept_incoming_connection(int& socket_fd, std::string& peer_address);

    // Local destination
    I2PDestination get_my_destination() const;
    std::string get_my_address() const;  // Returns xxx.b32.i2p

    // Configuration
    void set_config(const I2PSessionConfig& config) { config_ = config; }
    I2PSessionConfig get_config() const { return config_; }

    // Status and monitoring
    bool is_ready() const;
    std::string get_status() const;
    SAMSession::TunnelStats get_tunnel_stats() const;

    // Error handling
    std::string get_last_error() const { return last_error_; }

private:
    I2PSessionConfig config_;
    std::unique_ptr<SAMSession> sam_session_;
    std::string last_error_;
    mutable std::mutex mutex_;

    // Connection tracking
    struct PeerConnection {
        int socket_fd;
        std::string destination;
        uint64_t connected_time;
        uint64_t bytes_sent;
        uint64_t bytes_received;
    };
    std::map<int, PeerConnection> active_connections_;

    // Internal helpers
    bool verify_i2p_router_running();
    void cleanup_dead_connections();
};

/**
 * I2P Address Resolver
 * Convert between I2P formats and validate addresses
 */
class I2PAddressResolver {
public:
    // Address validation
    static bool is_valid_b32_address(const std::string& address);
    static bool is_valid_base64_destination(const std::string& dest);

    // Conversion
    static std::optional<std::string> base64_to_b32(const std::string& base64_dest);
    static std::optional<std::string> b32_to_base64(const std::string& b32_address);

    // Address book (persistent name -> destination mapping)
    bool add_address(const std::string& name, const std::string& destination);
    std::optional<std::string> resolve(const std::string& name);
    bool remove_address(const std::string& name);
    std::vector<std::pair<std::string, std::string>> list_addresses() const;

    // Load/save address book
    bool load_from_file(const std::string& filepath);
    bool save_to_file(const std::string& filepath) const;

private:
    std::map<std::string, std::string> address_book_;
    mutable std::mutex mutex_;
};

/**
 * I2P Service Discovery
 * Find other INTcoin nodes on I2P network
 */
class I2PServiceDiscovery {
public:
    I2PServiceDiscovery(I2PManager* manager);

    // Node discovery
    void announce_node(const I2PDestination& dest, uint16_t port);
    std::vector<std::string> discover_nodes();
    void update_peer_list();

    // Seed nodes
    void add_seed_node(const std::string& i2p_address);
    std::vector<std::string> get_seed_nodes() const;

    // Statistics
    size_t get_known_node_count() const { return known_nodes_.size(); }

private:
    I2PManager* manager_;
    std::vector<std::string> known_nodes_;
    std::vector<std::string> seed_nodes_;
    mutable std::mutex mutex_;

    // Discovery protocol
    void send_announce_message();
    void handle_discover_response(const std::string& response);
};

/**
 * I2P Configuration File Parser
 */
class I2PConfigParser {
public:
    static I2PSessionConfig parse_config_file(const std::string& filepath);
    static bool write_config_file(const std::string& filepath, const I2PSessionConfig& config);

    // Default configs
    static I2PSessionConfig default_mainnet_config();
    static I2PSessionConfig default_testnet_config();
    static I2PSessionConfig default_privacy_config();  // Maximum privacy
};

/**
 * I2P Network Statistics
 */
struct I2PNetworkStats {
    uint64_t total_connections;
    uint64_t active_connections;
    uint64_t bytes_sent;
    uint64_t bytes_received;
    uint32_t tunnel_build_success;
    uint32_t tunnel_build_failures;
    double average_latency_ms;
    uint32_t known_peers;

    I2PNetworkStats()
        : total_connections(0)
        , active_connections(0)
        , bytes_sent(0)
        , bytes_received(0)
        , tunnel_build_success(0)
        , tunnel_build_failures(0)
        , average_latency_ms(0.0)
        , known_peers(0)
    {}
};

/**
 * I2P Utility Functions
 */
namespace util {
    // Base32 encoding/decoding (I2P uses custom alphabet)
    std::string base32_encode(const std::vector<uint8_t>& data);
    std::vector<uint8_t> base32_decode(const std::string& encoded);

    // Base64 encoding/decoding
    std::string base64_encode(const std::vector<uint8_t>& data);
    std::vector<uint8_t> base64_decode(const std::string& encoded);

    // Address formatting
    std::string format_i2p_address(const std::string& base32, uint16_t port);
    bool parse_i2p_address(const std::string& addr_with_port, std::string& base32, uint16_t& port);

    // Destination key generation
    std::vector<uint8_t> generate_destination_keys();
    std::string destination_to_base32(const std::vector<uint8_t>& dest_key);
}

} // namespace i2p
} // namespace intcoin

#endif // INTCOIN_I2P_H
