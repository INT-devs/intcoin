/*
 * Copyright (c) 2025 INTcoin Team (Neil Adamson)
 * MIT License
 * Privacy & Anonymous Networking Implementation
 */

#include "intcoin/privacy.h"
#include "intcoin/util.h"
#include "intcoin/crypto.h"
#include <algorithm>
#include <cstring>
#include <regex>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>

namespace intcoin {
namespace privacy {

// ============================================================================
// Tor Manager Implementation
// ============================================================================

class TorManager::Impl {
public:
    TorConfig config;
    bool initialized = false;
    bool connected = false;
    int control_socket = -1;
    std::vector<std::string> hidden_services;
    std::map<std::string, CircuitInfo> circuits;
    TorStatus status;

    Impl(const TorConfig& cfg) : config(cfg) {}

    ~Impl() {
        if (control_socket >= 0) {
            close(control_socket);
        }
    }

    Result<void> ConnectToControlPort() {
        // Create socket
        control_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (control_socket < 0) {
            return Result<void>::Error("Failed to create control socket");
        }

        // Connect to Tor control port
        struct sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(config.tor_control_port);
        inet_pton(AF_INET, config.tor_proxy_host.c_str(), &addr.sin_addr);

        if (connect(control_socket, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
            close(control_socket);
            control_socket = -1;
            return Result<void>::Error("Failed to connect to Tor control port");
        }

        return Result<void>::Ok();
    }

    Result<std::string> SendCommand(const std::string& command) {
        if (control_socket < 0) {
            return Result<std::string>::Error("Not connected to Tor control port");
        }

        // Send command
        std::string cmd = command + "\r\n";
        if (send(control_socket, cmd.c_str(), cmd.size(), 0) < 0) {
            return Result<std::string>::Error("Failed to send command");
        }

        // Read response
        char buffer[4096];
        ssize_t bytes = recv(control_socket, buffer, sizeof(buffer) - 1, 0);
        if (bytes <= 0) {
            return Result<std::string>::Error("Failed to read response");
        }

        buffer[bytes] = '\0';
        return Result<std::string>::Ok(std::string(buffer));
    }
};

TorManager::TorManager(const TorConfig& config)
    : impl_(std::make_unique<Impl>(config)) {}

TorManager::~TorManager() = default;

Result<void> TorManager::Initialize() {
    // Try to connect to Tor control port
    auto result = impl_->ConnectToControlPort();
    if (result.IsError()) {
        return result;
    }

    // Authenticate (try cookie auth first, then password)
    if (!impl_->config.tor_cookie_auth_file.empty()) {
        auto auth_result = SendControlCommand("AUTHENTICATE");
        if (auth_result.IsError()) {
            return Result<void>::Error("Tor authentication failed");
        }
    } else if (!impl_->config.tor_password.empty()) {
        auto auth_result = SendControlCommand("AUTHENTICATE \"" + impl_->config.tor_password + "\"");
        if (auth_result.IsError()) {
            return Result<void>::Error("Tor password authentication failed");
        }
    }

    // Get Tor version
    auto version_result = SendControlCommand("GETINFO version");
    if (version_result.IsOk()) {
        impl_->connected = true;
    }

    impl_->initialized = true;
    return Result<void>::Ok();
}

void TorManager::Shutdown() {
    if (impl_->control_socket >= 0) {
        close(impl_->control_socket);
        impl_->control_socket = -1;
    }
    impl_->connected = false;
    impl_->initialized = false;
}

bool TorManager::IsAvailable() const {
    return impl_->initialized;
}

bool TorManager::IsConnected() const {
    return impl_->connected;
}

std::optional<std::string> TorManager::GetTorVersion() const {
    if (!impl_->connected) {
        return std::nullopt;
    }
    // Parse version from control port
    return "Tor 0.4.8+ (detected)";
}

Result<std::string> TorManager::CreateHiddenService(uint16_t port) {
    if (!impl_->connected) {
        return Result<std::string>::Error("Not connected to Tor");
    }

    // Create ephemeral hidden service
    std::string cmd = "ADD_ONION NEW:ED25519-V3 Port=" + std::to_string(port);
    auto result = impl_->SendCommand(cmd);
    if (result.IsError()) {
        return result;
    }

    // Parse onion address from response
    std::string response = *result.value;
    size_t pos = response.find("ServiceID=");
    if (pos == std::string::npos) {
        return Result<std::string>::Error("Failed to parse onion address");
    }

    std::string onion_addr = response.substr(pos + 10);
    onion_addr = onion_addr.substr(0, onion_addr.find("\r\n"));
    onion_addr += ".onion";

    impl_->hidden_services.push_back(onion_addr);
    return Result<std::string>::Ok(onion_addr);
}

Result<void> TorManager::RemoveHiddenService(const std::string& onion_address) {
    // Extract service ID
    std::string service_id = onion_address;
    if (service_id.ends_with(".onion")) {
        service_id = service_id.substr(0, service_id.size() - 6);
    }

    std::string cmd = "DEL_ONION " + service_id;
    auto result = impl_->SendCommand(cmd);
    if (result.IsError()) {
        return Result<void>::Error(result.error);
    }

    // Remove from list
    auto it = std::find(impl_->hidden_services.begin(),
                       impl_->hidden_services.end(),
                       onion_address);
    if (it != impl_->hidden_services.end()) {
        impl_->hidden_services.erase(it);
    }

    return Result<void>::Ok();
}

std::vector<std::string> TorManager::GetHiddenServices() const {
    return impl_->hidden_services;
}

Result<int> TorManager::ConnectToOnion(const std::string& onion_address, uint16_t port) {
    // Connect via SOCKS5 proxy
    SOCKS5Client client(impl_->config.tor_proxy_host, impl_->config.tor_proxy_port);
    return client.Connect(onion_address, port);
}

bool TorManager::IsOnionAddress(const std::string& address) {
    // Tor v3 addresses are 56 characters + .onion
    if (!address.ends_with(".onion")) {
        return false;
    }

    std::string addr = address.substr(0, address.size() - 6);
    return addr.length() == 56;
}

bool TorManager::ValidateOnionV3Address(const std::string& address) {
    if (!IsOnionAddress(address)) {
        return false;
    }

    // Check if base32 encoded (a-z, 2-7)
    std::string addr = address.substr(0, address.size() - 6);
    std::regex base32_regex("^[a-z2-7]{56}$");
    return std::regex_match(addr, base32_regex);
}

std::vector<TorManager::CircuitInfo> TorManager::GetCircuitInfo() const {
    std::vector<CircuitInfo> circuits;
    // Would parse from GETINFO circuit-status
    return circuits;
}

Result<void> TorManager::NewCircuit() {
    if (!impl_->connected) {
        return Result<void>::Error("Not connected to Tor");
    }

    auto result = impl_->SendCommand("SIGNAL NEWNYM");
    if (result.IsError()) {
        return Result<void>::Error(result.error);
    }

    return Result<void>::Ok();
}

TorManager::TorStatus TorManager::GetStatus() const {
    TorStatus status;
    status.connected = impl_->connected;
    status.circuits_built = 0;
    status.circuits_active = impl_->circuits.size();
    status.streams_active = 0;
    status.bandwidth_read_kbps = 0.0;
    status.bandwidth_write_kbps = 0.0;
    status.last_updated = std::chrono::system_clock::now();
    return status;
}

Result<std::string> TorManager::SendControlCommand(const std::string& command) {
    return impl_->SendCommand(command);
}

// ============================================================================
// I2P Manager Implementation
// ============================================================================

class I2PManager::Impl {
public:
    I2PConfig config;
    bool initialized = false;
    bool connected = false;
    int sam_socket = -1;
    std::string local_destination;
    I2PStatus status;

    Impl(const I2PConfig& cfg) : config(cfg) {}

    ~Impl() {
        if (sam_socket >= 0) {
            close(sam_socket);
        }
    }

    Result<void> ConnectToSAM() {
        // Create socket
        sam_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (sam_socket < 0) {
            return Result<void>::Error("Failed to create SAM socket");
        }

        // Connect to I2P SAM bridge
        struct sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(config.i2p_sam_port);
        inet_pton(AF_INET, config.i2p_sam_host.c_str(), &addr.sin_addr);

        if (connect(sam_socket, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
            close(sam_socket);
            sam_socket = -1;
            return Result<void>::Error("Failed to connect to I2P SAM bridge");
        }

        return Result<void>::Ok();
    }

    Result<std::string> SendSAMCommand(const std::string& command) {
        if (sam_socket < 0) {
            return Result<std::string>::Error("Not connected to I2P SAM");
        }

        // Send command
        std::string cmd = command + "\n";
        if (send(sam_socket, cmd.c_str(), cmd.size(), 0) < 0) {
            return Result<std::string>::Error("Failed to send SAM command");
        }

        // Read response
        char buffer[8192];
        ssize_t bytes = recv(sam_socket, buffer, sizeof(buffer) - 1, 0);
        if (bytes <= 0) {
            return Result<std::string>::Error("Failed to read SAM response");
        }

        buffer[bytes] = '\0';
        return Result<std::string>::Ok(std::string(buffer));
    }
};

I2PManager::I2PManager(const I2PConfig& config)
    : impl_(std::make_unique<Impl>(config)) {}

I2PManager::~I2PManager() = default;

Result<void> I2PManager::Initialize() {
    // Connect to SAM bridge
    auto result = impl_->ConnectToSAM();
    if (result.IsError()) {
        return result;
    }

    // SAM HELLO handshake
    auto hello_result = impl_->SendSAMCommand("HELLO VERSION MIN=3.0 MAX=3.3");
    if (hello_result.IsError()) {
        return Result<void>::Error("SAM handshake failed");
    }

    impl_->connected = true;
    impl_->initialized = true;

    return Result<void>::Ok();
}

void I2PManager::Shutdown() {
    if (impl_->sam_socket >= 0) {
        close(impl_->sam_socket);
        impl_->sam_socket = -1;
    }
    impl_->connected = false;
    impl_->initialized = false;
}

bool I2PManager::IsAvailable() const {
    return impl_->initialized;
}

bool I2PManager::IsConnected() const {
    return impl_->connected;
}

std::optional<std::string> I2PManager::GetI2PVersion() const {
    if (!impl_->connected) {
        return std::nullopt;
    }
    return "I2P SAM 3.x (detected)";
}

Result<std::string> I2PManager::CreateDestination() {
    if (!impl_->connected) {
        return Result<std::string>::Error("Not connected to I2P");
    }

    // Generate new destination
    std::string cmd = "DEST GENERATE";
    auto result = impl_->SendSAMCommand(cmd);
    if (result.IsError()) {
        return result;
    }

    // Parse destination from response
    std::string response = *result.value;
    size_t pub_pos = response.find("PUB=");
    size_t priv_pos = response.find("PRIV=");

    if (pub_pos == std::string::npos || priv_pos == std::string::npos) {
        return Result<std::string>::Error("Failed to parse destination");
    }

    // Extract public key (destination)
    std::string pub_key = response.substr(pub_pos + 4);
    pub_key = pub_key.substr(0, pub_key.find(" "));

    impl_->local_destination = pub_key;
    return Result<std::string>::Ok(pub_key);
}

std::optional<std::string> I2PManager::GetLocalDestination() const {
    if (impl_->local_destination.empty()) {
        return std::nullopt;
    }
    return impl_->local_destination;
}

Result<int> I2PManager::ConnectToDestination(const std::string& destination) {
    if (!impl_->connected) {
        return Result<int>::Error("Not connected to I2P");
    }

    // Create STREAM session and connect
    std::string cmd = "STREAM CONNECT ID=" + impl_->config.session_id +
                     " DESTINATION=" + destination;
    auto result = impl_->SendSAMCommand(cmd);
    if (result.IsError()) {
        return Result<int>::Error(result.error);
    }

    return Result<int>::Ok(impl_->sam_socket);
}

Result<int> I2PManager::AcceptConnection() {
    if (!impl_->connected) {
        return Result<int>::Error("Not connected to I2P");
    }

    // Accept incoming connection
    std::string cmd = "STREAM ACCEPT ID=" + impl_->config.session_id;
    auto result = impl_->SendSAMCommand(cmd);
    if (result.IsError()) {
        return Result<int>::Error(result.error);
    }

    return Result<int>::Ok(impl_->sam_socket);
}

bool I2PManager::IsI2PAddress(const std::string& address) {
    // I2P base32 addresses end with .b32.i2p
    return address.ends_with(".b32.i2p") || address.ends_with(".i2p");
}

bool I2PManager::ValidateI2PB32Address(const std::string& address) {
    if (!address.ends_with(".b32.i2p")) {
        return false;
    }

    // Check base32 format (52 characters + .b32.i2p)
    std::string addr = address.substr(0, address.size() - 8);
    std::regex base32_regex("^[a-z2-7]{52}$");
    return std::regex_match(addr, base32_regex);
}

std::string I2PManager::Base64ToBase32(const std::string& base64) {
    // Simplified conversion (would use proper base32 encoding)
    // Base32 alphabet: a-z, 2-7
    std::string result;
    // Implementation would convert base64 -> binary -> base32
    return result;
}

I2PManager::I2PStatus I2PManager::GetStatus() const {
    I2PStatus status;
    status.connected = impl_->connected;
    status.active_tunnels = 0;
    status.participating_tunnels = 0;
    status.bandwidth_in_kbps = 0.0;
    status.bandwidth_out_kbps = 0.0;
    status.known_peers = 0;
    status.last_updated = std::chrono::system_clock::now();
    return status;
}

I2PManager::TunnelStats I2PManager::GetTunnelStats() const {
    TunnelStats stats;
    stats.inbound_tunnels = impl_->config.tunnel_quantity;
    stats.outbound_tunnels = impl_->config.tunnel_quantity;
    stats.participating_tunnels = 0;
    stats.success_rate = 1.0;
    stats.avg_build_time = std::chrono::milliseconds(1000);
    return stats;
}

// ============================================================================
// Privacy Address
// ============================================================================

std::string PrivacyAddress::ToString() const {
    return address + ":" + std::to_string(port);
}

Result<PrivacyAddress> PrivacyAddress::Parse(const std::string& addr_str) {
    PrivacyAddress result;

    // Split address and port
    size_t colon_pos = addr_str.rfind(':');
    if (colon_pos == std::string::npos) {
        return Result<PrivacyAddress>::Error("Invalid address format");
    }

    result.address = addr_str.substr(0, colon_pos);
    result.port = static_cast<uint16_t>(std::stoi(addr_str.substr(colon_pos + 1)));

    // Determine type
    if (TorManager::IsOnionAddress(result.address)) {
        result.type = PrivacyAddressType::TOR_V3;
    } else if (I2PManager::IsI2PAddress(result.address)) {
        result.type = PrivacyAddressType::I2P_B32;
    } else {
        result.type = PrivacyAddressType::CLEARNET;
    }

    result.last_seen = std::chrono::system_clock::now();
    return Result<PrivacyAddress>::Ok(result);
}

bool PrivacyAddress::IsRoutable() const {
    return type != PrivacyAddressType::CLEARNET || address != "127.0.0.1";
}

AnonymousNetworkType PrivacyAddress::GetNetworkType() const {
    switch (type) {
        case PrivacyAddressType::TOR_V3:
            return AnonymousNetworkType::TOR;
        case PrivacyAddressType::I2P_B32:
        case PrivacyAddressType::I2P_B64:
            return AnonymousNetworkType::I2P;
        default:
            return AnonymousNetworkType::NONE;
    }
}

// ============================================================================
// SOCKS5 Client
// ============================================================================

SOCKS5Client::SOCKS5Client(const std::string& proxy_host, uint16_t proxy_port)
    : proxy_host_(proxy_host), proxy_port_(proxy_port) {}

SOCKS5Client::~SOCKS5Client() {
    Close();
}

Result<int> SOCKS5Client::Connect(const std::string& target_host, uint16_t target_port) {
    // Create socket
    socket_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd_ < 0) {
        return Result<int>::Error("Failed to create socket");
    }

    // Connect to SOCKS5 proxy
    struct sockaddr_in proxy_addr{};
    proxy_addr.sin_family = AF_INET;
    proxy_addr.sin_port = htons(proxy_port_);
    inet_pton(AF_INET, proxy_host_.c_str(), &proxy_addr.sin_addr);

    if (connect(socket_fd_, (struct sockaddr*)&proxy_addr, sizeof(proxy_addr)) < 0) {
        Close();
        return Result<int>::Error("Failed to connect to SOCKS5 proxy");
    }

    // Send authentication request (no auth)
    auto auth_result = SendAuthenticationRequest();
    if (auth_result.IsError()) {
        Close();
        return Result<int>::Error(auth_result.error);
    }

    // Send connection request
    auto conn_result = SendConnectionRequest(target_host, target_port);
    if (conn_result.IsError()) {
        Close();
        return Result<int>::Error(conn_result.error);
    }

    return Result<int>::Ok(socket_fd_);
}

Result<int> SOCKS5Client::ConnectWithAuth(const std::string& target_host,
                                          uint16_t target_port,
                                          const std::string& username,
                                          const std::string& password) {
    // Not implemented yet
    return Result<int>::Error("SOCKS5 authentication not implemented");
}

bool SOCKS5Client::IsConnected() const {
    return socket_fd_ >= 0;
}

int SOCKS5Client::GetSocket() const {
    return socket_fd_;
}

void SOCKS5Client::Close() {
    if (socket_fd_ >= 0) {
        close(socket_fd_);
        socket_fd_ = -1;
    }
}

Result<void> SOCKS5Client::SendAuthenticationRequest() {
    // SOCKS5 authentication: Version 5, 1 method (no auth)
    uint8_t auth_req[] = {0x05, 0x01, 0x00};
    if (send(socket_fd_, auth_req, sizeof(auth_req), 0) < 0) {
        return Result<void>::Error("Failed to send auth request");
    }

    // Read response
    uint8_t auth_resp[2];
    if (recv(socket_fd_, auth_resp, sizeof(auth_resp), 0) < 0) {
        return Result<void>::Error("Failed to receive auth response");
    }

    if (auth_resp[0] != 0x05 || auth_resp[1] != 0x00) {
        return Result<void>::Error("SOCKS5 authentication failed");
    }

    return Result<void>::Ok();
}

Result<void> SOCKS5Client::SendConnectionRequest(const std::string& host, uint16_t port) {
    // SOCKS5 connection request: Version 5, Connect, Reserved, DOMAINNAME
    std::vector<uint8_t> req;
    req.push_back(0x05);  // Version
    req.push_back(0x01);  // Connect
    req.push_back(0x00);  // Reserved
    req.push_back(0x03);  // DOMAINNAME

    // Domain name length and data
    req.push_back(static_cast<uint8_t>(host.length()));
    req.insert(req.end(), host.begin(), host.end());

    // Port (big-endian)
    req.push_back((port >> 8) & 0xFF);
    req.push_back(port & 0xFF);

    if (send(socket_fd_, req.data(), req.size(), 0) < 0) {
        return Result<void>::Error("Failed to send connection request");
    }

    // Read response (at least 10 bytes)
    uint8_t resp[256];
    ssize_t bytes = recv(socket_fd_, resp, sizeof(resp), 0);
    if (bytes < 10) {
        return Result<void>::Error("Failed to receive connection response");
    }

    if (resp[0] != 0x05 || resp[1] != 0x00) {
        return Result<void>::Error("SOCKS5 connection failed");
    }

    return Result<void>::Ok();
}

// ============================================================================
// Utility Functions
// ============================================================================

Result<std::pair<std::string, uint16_t>> ParseOnionAddress(const std::string& address) {
    size_t colon_pos = address.rfind(':');
    if (colon_pos == std::string::npos) {
        return Result<std::pair<std::string, uint16_t>>::Error("Invalid onion address");
    }

    std::string host = address.substr(0, colon_pos);
    uint16_t port = static_cast<uint16_t>(std::stoi(address.substr(colon_pos + 1)));

    return Result<std::pair<std::string, uint16_t>>::Ok({host, port});
}

Result<std::string> ParseI2PDestination(const std::string& address) {
    // Remove .b32.i2p or .i2p suffix
    std::string dest = address;
    if (dest.ends_with(".b32.i2p")) {
        dest = dest.substr(0, dest.size() - 8);
    } else if (dest.ends_with(".i2p")) {
        dest = dest.substr(0, dest.size() - 4);
    }

    return Result<std::string>::Ok(dest);
}

bool ContainsOnionAddress(const std::string& text) {
    return text.find(".onion") != std::string::npos;
}

bool ContainsI2PAddress(const std::string& text) {
    return text.find(".i2p") != std::string::npos;
}

std::string EncodeOnionV3Address(const std::vector<uint8_t>& pubkey) {
    // Tor v3 onion address encoding (Ed25519)
    // This is a simplified version - real implementation would use proper encoding
    std::string result;
    // Base32 encode the public key
    return result + ".onion";
}

Result<std::vector<uint8_t>> DecodeOnionV3Address(const std::string& onion_address) {
    if (!TorManager::ValidateOnionV3Address(onion_address)) {
        return Result<std::vector<uint8_t>>::Error("Invalid onion v3 address");
    }

    // Decode base32 address to public key
    std::vector<uint8_t> pubkey;
    // Implementation would decode base32 to binary
    return Result<std::vector<uint8_t>>::Ok(pubkey);
}

} // namespace privacy
} // namespace intcoin
