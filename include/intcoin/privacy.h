/*
 * Copyright (c) 2025 INTcoin Team (Neil Adamson)
 * MIT License
 * Privacy & Anonymous Networking (Tor/I2P Integration)
 */

#ifndef INTCOIN_PRIVACY_H
#define INTCOIN_PRIVACY_H

#include "types.h"
#include "network.h"
#include <string>
#include <vector>
#include <memory>
#include <optional>
#include <chrono>
#include <functional>

namespace intcoin {
namespace privacy {

// ============================================================================
// Anonymous Network Types
// ============================================================================

enum class AnonymousNetworkType {
    NONE,      // Clearnet only
    TOR,       // The Onion Router
    I2P,       // Invisible Internet Project
    HYBRID     // Both Tor and I2P
};

/// Address type for anonymous networks
enum class PrivacyAddressType {
    CLEARNET,       // Regular IPv4/IPv6
    TOR_V3,         // Tor v3 onion address (.onion)
    I2P_B32,        // I2P base32 address (.b32.i2p)
    I2P_B64,        // I2P base64 address
};

// ============================================================================
// Tor Integration
// ============================================================================

/// Tor configuration
struct TorConfig {
    std::string tor_proxy_host = "127.0.0.1";
    uint16_t tor_proxy_port = 9050;            // Default SOCKS5 port
    uint16_t tor_control_port = 9051;          // Control port
    std::string tor_password;                  // Control port password
    std::string tor_cookie_auth_file;          // Cookie auth file path
    bool use_tor_stream_isolation = true;      // Stream isolation
    bool tor_only_mode = false;                // Disable clearnet
    std::string onion_service_dir;             // Hidden service directory
    std::vector<uint16_t> onion_service_ports; // Ports to expose
    int circuit_build_timeout = 60;            // Seconds
    int stream_close_timeout = 10;             // Seconds
};

/// Tor connection manager
class TorManager {
public:
    TorManager(const TorConfig& config);
    ~TorManager();

    /// Initialize Tor connection
    Result<void> Initialize();

    /// Shutdown Tor connection
    void Shutdown();

    /// Check if Tor is available and working
    bool IsAvailable() const;

    /// Check if connected to Tor network
    bool IsConnected() const;

    /// Get Tor version
    std::optional<std::string> GetTorVersion() const;

    /// Create hidden service (onion address)
    Result<std::string> CreateHiddenService(uint16_t port);

    /// Remove hidden service
    Result<void> RemoveHiddenService(const std::string& onion_address);

    /// Get list of active hidden services
    std::vector<std::string> GetHiddenServices() const;

    /// Connect to onion address via SOCKS5 proxy
    Result<int> ConnectToOnion(const std::string& onion_address, uint16_t port);

    /// Check if address is Tor onion address
    static bool IsOnionAddress(const std::string& address);

    /// Validate Tor v3 address format
    static bool ValidateOnionV3Address(const std::string& address);

    /// Get current Tor circuit info
    struct CircuitInfo {
        std::string circuit_id;
        std::string status;
        std::vector<std::string> relay_path;
        std::chrono::system_clock::time_point created_at;
        size_t streams_attached;
    };
    std::vector<CircuitInfo> GetCircuitInfo() const;

    /// Request new Tor circuit (for privacy)
    Result<void> NewCircuit();

    /// Get Tor network status
    struct TorStatus {
        bool connected;
        size_t circuits_built;
        size_t circuits_active;
        size_t streams_active;
        double bandwidth_read_kbps;
        double bandwidth_write_kbps;
        std::chrono::system_clock::time_point last_updated;
    };
    TorStatus GetStatus() const;

    /// Send command to Tor control port
    Result<std::string> SendControlCommand(const std::string& command);

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

// ============================================================================
// I2P Integration
// ============================================================================

/// I2P configuration
struct I2PConfig {
    std::string i2p_sam_host = "127.0.0.1";
    uint16_t i2p_sam_port = 7656;              // Default SAM bridge port
    std::string session_id;                     // SAM session ID
    std::string destination_private_key;        // Local destination private key
    std::string destination_public_key;         // Local destination public key
    int tunnel_length = 3;                      // Number of hops (1-7)
    int tunnel_quantity = 2;                    // Number of tunnels
    int tunnel_backup_quantity = 1;             // Backup tunnels
    bool i2p_only_mode = false;                 // Disable clearnet
    std::chrono::seconds connection_timeout{30};
};

/// I2P connection manager (SAM v3 protocol)
class I2PManager {
public:
    I2PManager(const I2PConfig& config);
    ~I2PManager();

    /// Initialize I2P connection (SAM handshake)
    Result<void> Initialize();

    /// Shutdown I2P connection
    void Shutdown();

    /// Check if I2P is available
    bool IsAvailable() const;

    /// Check if connected to I2P network
    bool IsConnected() const;

    /// Get I2P router version
    std::optional<std::string> GetI2PVersion() const;

    /// Create new I2P destination (address)
    Result<std::string> CreateDestination();

    /// Get local I2P destination
    std::optional<std::string> GetLocalDestination() const;

    /// Connect to I2P destination
    Result<int> ConnectToDestination(const std::string& destination);

    /// Accept incoming I2P connections
    Result<int> AcceptConnection();

    /// Check if address is I2P address
    static bool IsI2PAddress(const std::string& address);

    /// Validate I2P base32 address format
    static bool ValidateI2PB32Address(const std::string& address);

    /// Convert I2P base64 to base32
    static std::string Base64ToBase32(const std::string& base64);

    /// Get I2P network status
    struct I2PStatus {
        bool connected;
        size_t active_tunnels;
        size_t participating_tunnels;
        double bandwidth_in_kbps;
        double bandwidth_out_kbps;
        size_t known_peers;
        std::chrono::system_clock::time_point last_updated;
    };
    I2PStatus GetStatus() const;

    /// Get tunnel statistics
    struct TunnelStats {
        size_t inbound_tunnels;
        size_t outbound_tunnels;
        size_t participating_tunnels;
        double success_rate;
        std::chrono::milliseconds avg_build_time;
    };
    TunnelStats GetTunnelStats() const;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

// ============================================================================
// Privacy Address Manager
// ============================================================================

/// Unified address for clearnet, Tor, and I2P
struct PrivacyAddress {
    PrivacyAddressType type;
    std::string address;
    uint16_t port;
    std::chrono::system_clock::time_point last_seen;

    /// Convert to string
    std::string ToString() const;

    /// Parse from string
    static Result<PrivacyAddress> Parse(const std::string& addr_str);

    /// Check if address is routable
    bool IsRoutable() const;

    /// Get network type
    AnonymousNetworkType GetNetworkType() const;
};

class PrivacyAddressManager {
public:
    /// Add address to address book
    void AddAddress(const PrivacyAddress& address);

    /// Remove address from address book
    void RemoveAddress(const PrivacyAddress& address);

    /// Get addresses by type
    std::vector<PrivacyAddress> GetAddresses(PrivacyAddressType type) const;

    /// Get all addresses
    std::vector<PrivacyAddress> GetAllAddresses() const;

    /// Get random address of type
    std::optional<PrivacyAddress> GetRandomAddress(PrivacyAddressType type) const;

    /// Mark address as seen
    void MarkAddressSeen(const PrivacyAddress& address);

    /// Remove old addresses
    void RemoveStaleAddresses(std::chrono::seconds max_age);

    /// Get address count
    size_t GetAddressCount() const;

    /// Get address count by type
    size_t GetAddressCount(PrivacyAddressType type) const;

private:
    std::map<std::string, PrivacyAddress> addresses_;
    mutable std::mutex mutex_;
};

// ============================================================================
// Privacy Network Manager (Unified)
// ============================================================================

struct PrivacyConfig {
    AnonymousNetworkType network_type = AnonymousNetworkType::NONE;
    TorConfig tor_config;
    I2PConfig i2p_config;
    bool prefer_privacy_network = true;        // Prefer Tor/I2P over clearnet
    bool allow_clearnet_fallback = true;       // Allow clearnet if privacy unavailable
    double privacy_network_peer_ratio = 0.5;   // 50% of peers from privacy networks
};

class PrivacyNetworkManager {
public:
    PrivacyNetworkManager(const PrivacyConfig& config);
    ~PrivacyNetworkManager();

    /// Initialize privacy networks
    Result<void> Initialize();

    /// Shutdown privacy networks
    void Shutdown();

    /// Check if any privacy network is available
    bool IsPrivacyAvailable() const;

    /// Get active network types
    std::vector<AnonymousNetworkType> GetActiveNetworks() const;

    /// Connect to address (auto-select network)
    Result<int> Connect(const PrivacyAddress& address);

    /// Get Tor manager (if enabled)
    TorManager* GetTorManager();

    /// Get I2P manager (if enabled)
    I2PManager* GetI2PManager();

    /// Get privacy address manager
    PrivacyAddressManager& GetAddressManager();

    /// Get combined status
    struct PrivacyStatus {
        bool tor_available;
        bool i2p_available;
        TorManager::TorStatus tor_status;
        I2PManager::I2PStatus i2p_status;
        size_t total_privacy_peers;
        size_t total_clearnet_peers;
    };
    PrivacyStatus GetStatus() const;

    /// Announce our addresses (clearnet + privacy)
    std::vector<PrivacyAddress> GetAnnouncedAddresses() const;

    /// Check if we should prefer privacy network for this connection
    bool ShouldUsePrivacyNetwork(const PrivacyAddress& address) const;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

// ============================================================================
// SOCKS5 Proxy Client (for Tor)
// ============================================================================

class SOCKS5Client {
public:
    SOCKS5Client(const std::string& proxy_host, uint16_t proxy_port);
    ~SOCKS5Client();

    /// Connect through SOCKS5 proxy
    Result<int> Connect(const std::string& target_host, uint16_t target_port);

    /// Connect with authentication
    Result<int> ConnectWithAuth(const std::string& target_host,
                                uint16_t target_port,
                                const std::string& username,
                                const std::string& password);

    /// Check if connected
    bool IsConnected() const;

    /// Get socket descriptor
    int GetSocket() const;

    /// Close connection
    void Close();

private:
    Result<void> SendAuthenticationRequest();
    Result<void> SendConnectionRequest(const std::string& host, uint16_t port);

    std::string proxy_host_;
    uint16_t proxy_port_;
    int socket_fd_ = -1;
};

// ============================================================================
// Utility Functions
// ============================================================================

/// Parse onion address (extract host and port)
Result<std::pair<std::string, uint16_t>> ParseOnionAddress(const std::string& address);

/// Parse I2P destination
Result<std::string> ParseI2PDestination(const std::string& address);

/// Check if string contains onion address
bool ContainsOnionAddress(const std::string& text);

/// Check if string contains I2P address
bool ContainsI2PAddress(const std::string& text);

/// Encode tor v3 onion address (from public key)
std::string EncodeOnionV3Address(const std::vector<uint8_t>& pubkey);

/// Decode tor v3 onion address (to public key)
Result<std::vector<uint8_t>> DecodeOnionV3Address(const std::string& onion_address);

} // namespace privacy
} // namespace intcoin

#endif // INTCOIN_PRIVACY_H
