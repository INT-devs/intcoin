// Copyright (c) 2025 INTcoin Core (Maddison Lane)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
//
// TOR (The Onion Router) support for anonymous networking.

#ifndef INTCOIN_TOR_H
#define INTCOIN_TOR_H

#include "primitives.h"
#include "p2p.h"
#include <string>
#include <vector>
#include <memory>
#include <optional>
#include <chrono>
#include <functional>

namespace intcoin {
namespace tor {

/**
 * TOR address types
 */
enum class AddressType {
    NONE,
    V2,         // Legacy 16-character .onion addresses (deprecated)
    V3          // Modern 56-character .onion addresses
};

/**
 * TOR onion address
 */
struct OnionAddress {
    std::string address;     // Full .onion address
    uint16_t port;
    AddressType type;

    OnionAddress() : address(""), port(0), type(AddressType::NONE) {}
    OnionAddress(const std::string& addr, uint16_t p)
        : address(addr), port(p), type(detect_type(addr)) {}

    bool is_valid() const;
    std::string to_string() const { return address + ":" + std::to_string(port); }

    static AddressType detect_type(const std::string& addr);
    static bool is_onion_address(const std::string& addr);
};

/**
 * SOCKS5 proxy configuration
 */
struct SOCKS5Config {
    std::string host;
    uint16_t port;
    std::string username;
    std::string password;
    bool use_auth;
    uint32_t timeout_ms;

    SOCKS5Config()
        : host("127.0.0.1")
        , port(9050)  // Default TOR SOCKS port
        , username("")
        , password("")
        , use_auth(false)
        , timeout_ms(30000) {}
};

/**
 * SOCKS5 authentication methods
 */
enum class SOCKS5Auth : uint8_t {
    NO_AUTH = 0x00,
    GSSAPI = 0x01,
    USERNAME_PASSWORD = 0x02,
    NO_ACCEPTABLE = 0xFF
};

/**
 * SOCKS5 command types
 */
enum class SOCKS5Command : uint8_t {
    CONNECT = 0x01,
    BIND = 0x02,
    UDP_ASSOCIATE = 0x03
};

/**
 * SOCKS5 address types
 */
enum class SOCKS5AddressType : uint8_t {
    IPV4 = 0x01,
    DOMAIN = 0x03,
    IPV6 = 0x04
};

/**
 * SOCKS5 reply codes
 */
enum class SOCKS5Reply : uint8_t {
    SUCCESS = 0x00,
    GENERAL_FAILURE = 0x01,
    CONNECTION_NOT_ALLOWED = 0x02,
    NETWORK_UNREACHABLE = 0x03,
    HOST_UNREACHABLE = 0x04,
    CONNECTION_REFUSED = 0x05,
    TTL_EXPIRED = 0x06,
    COMMAND_NOT_SUPPORTED = 0x07,
    ADDRESS_TYPE_NOT_SUPPORTED = 0x08
};

/**
 * SOCKS5 proxy connector
 * Handles SOCKS5 protocol for connecting through TOR
 */
class SOCKS5Proxy {
public:
    explicit SOCKS5Proxy(const SOCKS5Config& config);
    ~SOCKS5Proxy();

    // Connect to target through SOCKS5 proxy
    int connect(const std::string& host, uint16_t port);
    int connect(const OnionAddress& onion_addr);

    // Close connection
    void disconnect(int socket_fd);

    // Check if proxy is reachable
    bool test_connection();

    // Get configuration
    const SOCKS5Config& get_config() const { return config_; }

private:
    SOCKS5Config config_;

    // SOCKS5 protocol methods
    bool socks5_handshake(int socket_fd);
    bool socks5_authenticate(int socket_fd);
    bool socks5_connect_command(int socket_fd, const std::string& host, uint16_t port);

    // Helper methods
    int create_proxy_socket();
    bool send_data(int socket_fd, const std::vector<uint8_t>& data);
    std::vector<uint8_t> receive_data(int socket_fd, size_t expected_size);
};

/**
 * TOR hidden service configuration
 */
struct HiddenServiceConfig {
    std::string data_dir;           // Directory for hidden service keys
    std::string private_key_file;   // Path to private key (ed25519)
    std::string hostname_file;      // Path to hostname file
    uint16_t virtual_port;          // External port advertised
    uint16_t target_port;           // Internal port to forward to
    bool enabled;

    HiddenServiceConfig()
        : data_dir("")
        , private_key_file("")
        , hostname_file("")
        , virtual_port(8333)
        , target_port(8333)
        , enabled(false) {}
};

/**
 * TOR hidden service manager
 * Manages .onion hidden service for this node
 */
class HiddenService {
public:
    HiddenService();
    explicit HiddenService(const HiddenServiceConfig& config);

    // Initialize hidden service
    bool initialize();

    // Start/stop hidden service
    bool start();
    void stop();

    // Get our .onion address
    std::optional<OnionAddress> get_onion_address() const;

    // Check if hidden service is running
    bool is_running() const { return running_; }

    // Generate new hidden service keys
    bool generate_keys();

    // Load existing keys
    bool load_keys();

private:
    HiddenServiceConfig config_;
    bool running_;
    std::optional<OnionAddress> onion_address_;

    // Key generation and management
    bool generate_ed25519_keypair();
    bool derive_onion_address();
    bool save_keys();
};

/**
 * TOR controller for managing TOR process
 * Communicates with TOR via control port
 */
class TORController {
public:
    TORController();
    ~TORController();

    // Connect to TOR control port
    bool connect(const std::string& host = "127.0.0.1", uint16_t port = 9051);
    void disconnect();

    // Authenticate with TOR
    bool authenticate(const std::string& password = "");
    bool authenticate_cookie(const std::string& cookie_path = "");

    // TOR control commands
    bool send_command(const std::string& command, std::string& response);
    bool get_info(const std::string& keyword, std::string& value);
    bool set_config(const std::string& key, const std::string& value);

    // Circuit management
    bool new_circuit();
    bool close_circuit(const std::string& circuit_id);

    // Hidden service management via control port
    bool add_onion(const std::string& private_key, uint16_t port, std::string& onion_address);
    bool del_onion(const std::string& onion_address);

    // Status
    bool is_connected() const { return connected_; }
    bool is_authenticated() const { return authenticated_; }

private:
    int control_socket_;
    bool connected_;
    bool authenticated_;

    bool send_line(const std::string& line);
    std::string receive_response();
    bool parse_response(const std::string& response, std::string& data);
};

/**
 * TOR network manager
 * Integrates TOR functionality with P2P network
 */
class TORNetwork {
public:
    TORNetwork();
    ~TORNetwork();

    // Configuration
    void set_socks5_config(const SOCKS5Config& config);
    void set_hidden_service_config(const HiddenServiceConfig& config);
    void enable_onion_only(bool enabled) { onion_only_ = enabled; }

    // Initialize TOR networking
    bool initialize();

    // Start/stop TOR networking
    bool start();
    void stop();

    // Connection through TOR
    int connect_through_tor(const std::string& host, uint16_t port);
    int connect_to_onion(const OnionAddress& addr);

    // Hidden service
    bool start_hidden_service();
    void stop_hidden_service();
    std::optional<OnionAddress> get_our_onion_address() const;

    // Peer management
    void add_onion_peer(const OnionAddress& addr);
    std::vector<OnionAddress> get_onion_peers() const;

    // Convert between regular and TOR addresses
    p2p::PeerAddress onion_to_peer_address(const OnionAddress& onion) const;
    std::optional<OnionAddress> peer_address_to_onion(const p2p::PeerAddress& addr) const;

    // Status
    bool is_tor_available() const;
    bool is_hidden_service_running() const { return hidden_service_ && hidden_service_->is_running(); }
    bool is_onion_only() const { return onion_only_; }

    // Statistics
    struct TORStats {
        size_t onion_peers;
        size_t clearnet_peers;
        size_t connections_through_tor;
        bool hidden_service_active;
        std::string our_onion_address;
    };
    TORStats get_stats() const;

private:
    std::unique_ptr<SOCKS5Proxy> proxy_;
    std::unique_ptr<HiddenService> hidden_service_;
    std::unique_ptr<TORController> controller_;

    SOCKS5Config socks5_config_;
    HiddenServiceConfig hs_config_;
    bool onion_only_;  // Only connect to .onion addresses
    bool initialized_;
    bool running_;

    std::vector<OnionAddress> onion_peers_;
    mutable size_t connections_through_tor_;

    // Helper methods
    bool check_tor_available();
    bool validate_onion_address(const std::string& addr) const;
};

/**
 * TOR utilities
 */
namespace util {
    // Generate v3 .onion address from ed25519 public key
    std::string generate_v3_onion_address(const std::vector<uint8_t>& pubkey);

    // Parse .onion address
    bool parse_onion_address(const std::string& addr, std::string& onion, uint16_t& port);

    // Check if TOR is running on default port
    bool is_tor_running(const std::string& host = "127.0.0.1", uint16_t port = 9050);

    // Get default TOR data directory
    std::string get_default_tor_datadir();

    // Generate random TOR circuit ID
    std::string generate_circuit_id();
}

/**
 * TOR protocol constants
 */
namespace protocol {
    static constexpr uint8_t SOCKS5_VERSION = 0x05;
    static constexpr size_t SOCKS5_MAX_HOSTNAME_LEN = 255;
    static constexpr size_t V2_ONION_LEN = 16;  // chars (deprecated)
    static constexpr size_t V3_ONION_LEN = 56;  // chars
    static constexpr uint16_t DEFAULT_TOR_SOCKS_PORT = 9050;
    static constexpr uint16_t DEFAULT_TOR_CONTROL_PORT = 9051;
    static constexpr const char* ONION_SUFFIX = ".onion";
}

} // namespace tor
} // namespace intcoin

#endif // INTCOIN_TOR_H
