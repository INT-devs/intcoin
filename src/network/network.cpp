/*
 * Copyright (c) 2025 INTcoin Team (Neil Adamson)
 * P2P Network Protocol Implementation
 */

#include "intcoin/network.h"
#include "intcoin/blockchain.h"
#include "intcoin/crypto.h"
#include "intcoin/util.h"
#include <sstream>
#include <algorithm>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <cerrno>
#include <mutex>
#include <unordered_map>
#include <fstream>
#include <sys/stat.h>

namespace intcoin {

// ============================================================================
// Network Utilities
// ============================================================================

Result<std::array<uint8_t, 16>> ParseIPAddress(const std::string& ip) {
    std::array<uint8_t, 16> result = {0};

    // Try IPv4 first
    struct sockaddr_in sa4;
    if (inet_pton(AF_INET, ip.c_str(), &(sa4.sin_addr)) == 1) {
        // IPv4-mapped IPv6 address (::ffff:x.x.x.x)
        result[10] = 0xff;
        result[11] = 0xff;
        std::memcpy(&result[12], &sa4.sin_addr, 4);
        return Result<std::array<uint8_t, 16>>::Ok(result);
    }

    // Try IPv6
    struct sockaddr_in6 sa6;
    if (inet_pton(AF_INET6, ip.c_str(), &(sa6.sin6_addr)) == 1) {
        std::memcpy(result.data(), &sa6.sin6_addr, 16);
        return Result<std::array<uint8_t, 16>>::Ok(result);
    }

    return Result<std::array<uint8_t, 16>>::Error("Invalid IP address: " + ip);
}

std::string IPAddressToString(const std::array<uint8_t, 16>& ip) {
    // Check if IPv4-mapped IPv6
    bool is_ipv4_mapped = true;
    for (int i = 0; i < 10; i++) {
        if (ip[i] != 0) {
            is_ipv4_mapped = false;
            break;
        }
    }
    if (is_ipv4_mapped && ip[10] == 0xff && ip[11] == 0xff) {
        // IPv4
        char buffer[INET_ADDRSTRLEN];
        struct in_addr addr;
        std::memcpy(&addr, &ip[12], 4);
        inet_ntop(AF_INET, &addr, buffer, INET_ADDRSTRLEN);
        return std::string(buffer);
    }

    // IPv6
    char buffer[INET6_ADDRSTRLEN];
    struct in6_addr addr;
    std::memcpy(&addr, ip.data(), 16);
    inet_ntop(AF_INET6, &addr, buffer, INET6_ADDRSTRLEN);
    return std::string(buffer);
}

bool IsValidPort(uint16_t port) {
    return port > 0;  // uint16_t is always < 65536
}

std::vector<NetworkAddress> GetLocalAddresses() {
    std::vector<NetworkAddress> addresses;

    // Get hostname
    char hostname[256];
    if (gethostname(hostname, sizeof(hostname)) != 0) {
        return addresses;
    }

    // Get address info
    struct addrinfo hints;
    std::memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    struct addrinfo* result;
    if (getaddrinfo(hostname, nullptr, &hints, &result) != 0) {
        return addresses;
    }

    // Process results
    for (struct addrinfo* rp = result; rp != nullptr; rp = rp->ai_next) {
        if (rp->ai_family == AF_INET) {
            struct sockaddr_in* sa = (struct sockaddr_in*)rp->ai_addr;
            char ip_str[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &(sa->sin_addr), ip_str, INET_ADDRSTRLEN);
            addresses.emplace_back(std::string(ip_str), network::MAINNET_P2P_PORT);
        } else if (rp->ai_family == AF_INET6) {
            struct sockaddr_in6* sa = (struct sockaddr_in6*)rp->ai_addr;
            char ip_str[INET6_ADDRSTRLEN];
            inet_ntop(AF_INET6, &(sa->sin6_addr), ip_str, INET6_ADDRSTRLEN);
            addresses.emplace_back(std::string(ip_str), network::MAINNET_P2P_PORT);
        }
    }

    freeaddrinfo(result);
    return addresses;
}

// ============================================================================
// NetworkAddress Implementation
// ============================================================================

NetworkAddress::NetworkAddress()
    : services(0), port(0), timestamp(0) {
    ip.fill(0);
}

NetworkAddress::NetworkAddress(const std::string& ip_str, uint16_t p)
    : services(static_cast<uint64_t>(ServiceFlags::NODE_NETWORK)),
      port(p),
      timestamp(std::chrono::system_clock::now().time_since_epoch().count()) {

    auto result = ParseIPAddress(ip_str);
    if (result.IsOk()) {
        ip = result.value.value();
    } else {
        ip.fill(0);
    }
}

std::vector<uint8_t> NetworkAddress::Serialize() const {
    std::vector<uint8_t> data;

    // Timestamp (8 bytes)
    for (int i = 0; i < 8; i++) {
        data.push_back((timestamp >> (i * 8)) & 0xFF);
    }

    // Services (8 bytes)
    for (int i = 0; i < 8; i++) {
        data.push_back((services >> (i * 8)) & 0xFF);
    }

    // IP address (16 bytes)
    data.insert(data.end(), ip.begin(), ip.end());

    // Port (2 bytes, big-endian)
    data.push_back((port >> 8) & 0xFF);
    data.push_back(port & 0xFF);

    return data;
}

Result<NetworkAddress> NetworkAddress::Deserialize(const std::vector<uint8_t>& data) {
    if (data.size() < 34) {
        return Result<NetworkAddress>::Error("NetworkAddress data too short");
    }

    NetworkAddress addr;
    size_t pos = 0;

    // Timestamp (8 bytes)
    addr.timestamp = 0;
    for (int i = 0; i < 8; i++) {
        addr.timestamp |= (static_cast<uint64_t>(data[pos++]) << (i * 8));
    }

    // Services (8 bytes)
    addr.services = 0;
    for (int i = 0; i < 8; i++) {
        addr.services |= (static_cast<uint64_t>(data[pos++]) << (i * 8));
    }

    // IP address (16 bytes)
    std::copy(data.begin() + pos, data.begin() + pos + 16, addr.ip.begin());
    pos += 16;

    // Port (2 bytes, big-endian)
    addr.port = (static_cast<uint16_t>(data[pos]) << 8) | data[pos + 1];

    return Result<NetworkAddress>::Ok(addr);
}

std::string NetworkAddress::ToString() const {
    return IPAddressToString(ip) + ":" + std::to_string(port);
}

bool NetworkAddress::IsIPv4() const {
    // Check for IPv4-mapped IPv6 (::ffff:x.x.x.x)
    for (int i = 0; i < 10; i++) {
        if (ip[i] != 0) return false;
    }
    return ip[10] == 0xff && ip[11] == 0xff;
}

bool NetworkAddress::IsIPv6() const {
    return !IsIPv4();
}

bool NetworkAddress::IsTor() const {
    // Tor addresses are .onion (not directly represented in IP)
    return false;
}

bool NetworkAddress::IsLocal() const {
    if (IsIPv4()) {
        // 127.0.0.0/8
        return ip[12] == 127;
    } else {
        // ::1
        for (int i = 0; i < 15; i++) {
            if (ip[i] != 0) return false;
        }
        return ip[15] == 1;
    }
}

bool NetworkAddress::IsRoutable() const {
    if (IsLocal()) return false;

    if (IsIPv4()) {
        uint8_t b1 = ip[12];
        uint8_t b2 = ip[13];

        // Private networks
        if (b1 == 10) return false;                    // 10.0.0.0/8
        if (b1 == 172 && (b2 >= 16 && b2 <= 31)) return false;  // 172.16.0.0/12
        if (b1 == 192 && b2 == 168) return false;      // 192.168.0.0/16
        if (b1 == 169 && b2 == 254) return false;      // 169.254.0.0/16 (link-local)
        if (b1 >= 224) return false;                    // Multicast/reserved
    }

    return true;
}

// ============================================================================
// NetworkMessage Implementation
// ============================================================================

NetworkMessage::NetworkMessage()
    : magic(network::MAINNET_MAGIC), length(0), checksum(0) {}

NetworkMessage::NetworkMessage(uint32_t m, const std::string& cmd,
                              const std::vector<uint8_t>& data)
    : magic(m), command(cmd), length(data.size()), payload(data) {
    checksum = CalculateChecksum(payload);
}

std::vector<uint8_t> NetworkMessage::Serialize() const {
    std::vector<uint8_t> data;

    // Magic (4 bytes)
    for (int i = 0; i < 4; i++) {
        data.push_back((magic >> (i * 8)) & 0xFF);
    }

    // Command (12 bytes, null-padded)
    std::string cmd = command;
    cmd.resize(12, '\0');
    data.insert(data.end(), cmd.begin(), cmd.end());

    // Length (4 bytes)
    for (int i = 0; i < 4; i++) {
        data.push_back((length >> (i * 8)) & 0xFF);
    }

    // Checksum (4 bytes)
    for (int i = 0; i < 4; i++) {
        data.push_back((checksum >> (i * 8)) & 0xFF);
    }

    // Payload
    data.insert(data.end(), payload.begin(), payload.end());

    return data;
}

Result<NetworkMessage> NetworkMessage::Deserialize(const std::vector<uint8_t>& data) {
    if (data.size() < 24) {
        return Result<NetworkMessage>::Error("Message data too short");
    }

    NetworkMessage msg;
    size_t pos = 0;

    // Magic (4 bytes)
    msg.magic = 0;
    for (int i = 0; i < 4; i++) {
        msg.magic |= (static_cast<uint32_t>(data[pos++]) << (i * 8));
    }

    // Command (12 bytes)
    msg.command = std::string(data.begin() + pos, data.begin() + pos + 12);
    // Remove null padding
    size_t null_pos = msg.command.find('\0');
    if (null_pos != std::string::npos) {
        msg.command = msg.command.substr(0, null_pos);
    }
    pos += 12;

    // Length (4 bytes)
    msg.length = 0;
    for (int i = 0; i < 4; i++) {
        msg.length |= (static_cast<uint32_t>(data[pos++]) << (i * 8));
    }

    // Check length
    if (msg.length > network::MAX_MESSAGE_SIZE) {
        return Result<NetworkMessage>::Error("Message too large");
    }

    // Checksum (4 bytes)
    msg.checksum = 0;
    for (int i = 0; i < 4; i++) {
        msg.checksum |= (static_cast<uint32_t>(data[pos++]) << (i * 8));
    }

    // Payload
    if (data.size() < 24 + msg.length) {
        return Result<NetworkMessage>::Error("Incomplete message payload");
    }
    msg.payload = std::vector<uint8_t>(data.begin() + pos, data.begin() + pos + msg.length);

    // Verify checksum
    if (!msg.VerifyChecksum()) {
        return Result<NetworkMessage>::Error("Invalid message checksum");
    }

    return Result<NetworkMessage>::Ok(msg);
}

bool NetworkMessage::VerifyChecksum() const {
    return checksum == CalculateChecksum(payload);
}

uint32_t NetworkMessage::CalculateChecksum(const std::vector<uint8_t>& data) {
    uint256 hash = SHA3::Hash(data);
    // Use first 4 bytes
    return (hash[0] << 24) | (hash[1] << 16) | (hash[2] << 8) | hash[3];
}

// ============================================================================
// InvVector Implementation
// ============================================================================

std::vector<uint8_t> InvVector::Serialize() const {
    std::vector<uint8_t> data;

    // Type (4 bytes)
    uint32_t type_val = static_cast<uint32_t>(type);
    for (int i = 0; i < 4; i++) {
        data.push_back((type_val >> (i * 8)) & 0xFF);
    }

    // Hash (32 bytes)
    data.insert(data.end(), hash.begin(), hash.end());

    return data;
}

Result<InvVector> InvVector::Deserialize(const std::vector<uint8_t>& data) {
    if (data.size() < 36) {
        return Result<InvVector>::Error("InvVector data too short");
    }

    InvVector inv;
    size_t pos = 0;

    // Type (4 bytes)
    uint32_t type_val = 0;
    for (int i = 0; i < 4; i++) {
        type_val |= (static_cast<uint32_t>(data[pos++]) << (i * 8));
    }
    inv.type = static_cast<InvType>(type_val);

    // Hash (32 bytes)
    std::copy(data.begin() + pos, data.begin() + pos + 32, inv.hash.begin());

    return Result<InvVector>::Ok(inv);
}

// ============================================================================
// Peer Implementation
// ============================================================================

// Internal peer socket state
struct PeerSocket {
    int socket_fd;
    bool connected;
    std::vector<uint8_t> recv_buffer;
    std::vector<uint8_t> send_buffer;
    std::chrono::system_clock::time_point last_ping_time;
    std::chrono::milliseconds ping_time;

    PeerSocket() : socket_fd(-1), connected(false), ping_time(0) {}

    ~PeerSocket() {
        if (socket_fd >= 0) {
            close(socket_fd);
        }
    }
};

// Global peer socket map (peer_id -> socket state)
static std::unordered_map<uint64_t, std::shared_ptr<PeerSocket>> g_peer_sockets;
static std::mutex g_peer_sockets_mutex;

Result<void> Peer::SendMessage(const NetworkMessage& msg) {
    std::lock_guard<std::mutex> lock(g_peer_sockets_mutex);

    auto it = g_peer_sockets.find(id);
    if (it == g_peer_sockets.end() || !it->second->connected) {
        return Result<void>::Error("Peer not connected");
    }

    auto socket_state = it->second;

    // Serialize message
    std::vector<uint8_t> data = msg.Serialize();

    // Send data
    ssize_t total_sent = 0;
    while (total_sent < static_cast<ssize_t>(data.size())) {
        ssize_t sent = send(socket_state->socket_fd,
                           data.data() + total_sent,
                           data.size() - total_sent,
                           0);
        if (sent < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // Would block, add to send buffer
                socket_state->send_buffer.insert(
                    socket_state->send_buffer.end(),
                    data.begin() + total_sent,
                    data.end()
                );
                break;
            }
            return Result<void>::Error("Send failed: " + std::string(strerror(errno)));
        }
        total_sent += sent;
        bytes_sent += sent;
    }

    last_message_time = std::chrono::system_clock::now();
    return Result<void>::Ok();
}

Result<NetworkMessage> Peer::ReceiveMessage() {
    std::lock_guard<std::mutex> lock(g_peer_sockets_mutex);

    auto it = g_peer_sockets.find(id);
    if (it == g_peer_sockets.end() || !it->second->connected) {
        return Result<NetworkMessage>::Error("Peer not connected");
    }

    auto socket_state = it->second;

    // Try to receive data
    uint8_t buffer[4096];
    ssize_t received = recv(socket_state->socket_fd, buffer, sizeof(buffer), MSG_DONTWAIT);

    if (received < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return Result<NetworkMessage>::Error("No data available");
        }
        return Result<NetworkMessage>::Error("Receive failed: " + std::string(strerror(errno)));
    }

    if (received == 0) {
        // Connection closed
        socket_state->connected = false;
        return Result<NetworkMessage>::Error("Connection closed");
    }

    // Add to receive buffer
    socket_state->recv_buffer.insert(
        socket_state->recv_buffer.end(),
        buffer,
        buffer + received
    );
    bytes_received += received;

    // Try to parse a complete message (need at least 24 bytes for header)
    if (socket_state->recv_buffer.size() < 24) {
        return Result<NetworkMessage>::Error("Incomplete message");
    }

    // Try to deserialize
    auto result = NetworkMessage::Deserialize(socket_state->recv_buffer);
    if (result.IsOk()) {
        // Message parsed successfully
        auto msg = result.value.value();
        size_t msg_size = 24 + msg.length;

        // Remove parsed message from buffer
        socket_state->recv_buffer.erase(
            socket_state->recv_buffer.begin(),
            socket_state->recv_buffer.begin() + msg_size
        );

        last_message_time = std::chrono::system_clock::now();
        return Result<NetworkMessage>::Ok(msg);
    }

    // Not enough data yet
    return Result<NetworkMessage>::Error("Incomplete message");
}

void Peer::Disconnect() {
    std::lock_guard<std::mutex> lock(g_peer_sockets_mutex);

    auto it = g_peer_sockets.find(id);
    if (it != g_peer_sockets.end()) {
        if (it->second->socket_fd >= 0) {
            close(it->second->socket_fd);
            it->second->socket_fd = -1;
        }
        it->second->connected = false;
        g_peer_sockets.erase(it);
    }
}

bool Peer::IsConnected() const {
    std::lock_guard<std::mutex> lock(g_peer_sockets_mutex);

    auto it = g_peer_sockets.find(id);
    return it != g_peer_sockets.end() && it->second->connected;
}

std::chrono::milliseconds Peer::GetPingTime() const {
    std::lock_guard<std::mutex> lock(g_peer_sockets_mutex);

    auto it = g_peer_sockets.find(id);
    if (it != g_peer_sockets.end()) {
        return it->second->ping_time;
    }
    return std::chrono::milliseconds(0);
}

void Peer::IncreaseBanScore(int points) {
    ban_score += points;
}

// ============================================================================
// P2PNode Implementation
// ============================================================================

class P2PNode::Impl {
public:
    uint32_t network_magic;
    uint16_t listen_port;
    int listen_socket;
    bool running;
    std::vector<std::shared_ptr<Peer>> peers;
    std::mutex peers_mutex;
    uint64_t next_peer_id;
    std::unordered_map<std::string, std::chrono::system_clock::time_point> banned_peers;
    std::mutex banned_peers_mutex;

    Impl(uint32_t magic, uint16_t port)
        : network_magic(magic), listen_port(port), listen_socket(-1),
          running(false), next_peer_id(1) {}

    ~Impl() {
        if (listen_socket >= 0) {
            close(listen_socket);
        }
    }
};

P2PNode::P2PNode(uint32_t network_magic, uint16_t listen_port)
    : impl_(std::make_unique<Impl>(network_magic, listen_port)) {}

P2PNode::~P2PNode() {
    Stop();
}

Result<void> P2PNode::Start() {
    if (impl_->running) {
        return Result<void>::Error("Node already running");
    }

    // Create socket
    impl_->listen_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (impl_->listen_socket < 0) {
        return Result<void>::Error("Failed to create socket: " + std::string(strerror(errno)));
    }

    // Set socket options
    int opt = 1;
    if (setsockopt(impl_->listen_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        close(impl_->listen_socket);
        impl_->listen_socket = -1;
        return Result<void>::Error("Failed to set SO_REUSEADDR: " + std::string(strerror(errno)));
    }

    // Set non-blocking
    int flags = fcntl(impl_->listen_socket, F_GETFL, 0);
    if (fcntl(impl_->listen_socket, F_SETFL, flags | O_NONBLOCK) < 0) {
        close(impl_->listen_socket);
        impl_->listen_socket = -1;
        return Result<void>::Error("Failed to set non-blocking: " + std::string(strerror(errno)));
    }

    // Bind
    struct sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(impl_->listen_port);

    if (bind(impl_->listen_socket, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        close(impl_->listen_socket);
        impl_->listen_socket = -1;
        return Result<void>::Error("Failed to bind to port " +
                                   std::to_string(impl_->listen_port) + ": " +
                                   std::string(strerror(errno)));
    }

    // Listen
    if (listen(impl_->listen_socket, network::MAX_INBOUND_CONNECTIONS) < 0) {
        close(impl_->listen_socket);
        impl_->listen_socket = -1;
        return Result<void>::Error("Failed to listen: " + std::string(strerror(errno)));
    }

    impl_->running = true;
    return Result<void>::Ok();
}

void P2PNode::Stop() {
    if (!impl_->running) {
        return;
    }

    impl_->running = false;

    // Close all peer connections
    std::lock_guard<std::mutex> lock(impl_->peers_mutex);
    for (auto& peer : impl_->peers) {
        peer->Disconnect();
    }
    impl_->peers.clear();

    // Close listen socket
    if (impl_->listen_socket >= 0) {
        close(impl_->listen_socket);
        impl_->listen_socket = -1;
    }
}

Result<std::shared_ptr<Peer>> P2PNode::ConnectToPeer(const NetworkAddress& address) {
    // Check if banned
    {
        std::lock_guard<std::mutex> lock(impl_->banned_peers_mutex);
        std::string addr_str = address.ToString();
        auto it = impl_->banned_peers.find(addr_str);
        if (it != impl_->banned_peers.end()) {
            auto now = std::chrono::system_clock::now();
            if (now < it->second) {
                return Result<std::shared_ptr<Peer>>::Error("Peer is banned");
            }
            // Ban expired
            impl_->banned_peers.erase(it);
        }
    }

    // Create socket
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        return Result<std::shared_ptr<Peer>>::Error("Failed to create socket: " +
                                                    std::string(strerror(errno)));
    }

    // Set non-blocking
    int flags = fcntl(sock, F_GETFL, 0);
    fcntl(sock, F_SETFL, flags | O_NONBLOCK);

    // Convert NetworkAddress to sockaddr_in
    struct sockaddr_in peer_addr;
    std::memset(&peer_addr, 0, sizeof(peer_addr));
    peer_addr.sin_family = AF_INET;
    peer_addr.sin_port = htons(address.port);

    // Get IP address
    if (address.IsIPv4()) {
        // Extract IPv4 from IPv4-mapped IPv6
        std::memcpy(&peer_addr.sin_addr, &address.ip[12], 4);
    } else {
        close(sock);
        return Result<std::shared_ptr<Peer>>::Error("IPv6 not yet supported");
    }

    // Connect (non-blocking)
    int result = connect(sock, (struct sockaddr*)&peer_addr, sizeof(peer_addr));
    if (result < 0 && errno != EINPROGRESS) {
        close(sock);
        return Result<std::shared_ptr<Peer>>::Error("Failed to connect: " +
                                                    std::string(strerror(errno)));
    }

    // Create peer object
    auto peer = std::make_shared<Peer>();
    peer->id = impl_->next_peer_id++;
    peer->address = address;
    peer->version = 0;  // Set during handshake
    peer->services = 0;
    peer->connect_time = std::chrono::system_clock::now();
    peer->last_message_time = peer->connect_time;
    peer->inbound = false;
    peer->bytes_sent = 0;
    peer->bytes_received = 0;
    peer->ban_score = 0;

    // Create socket state
    auto socket_state = std::make_shared<PeerSocket>();
    socket_state->socket_fd = sock;
    socket_state->connected = true;

    {
        std::lock_guard<std::mutex> lock(g_peer_sockets_mutex);
        g_peer_sockets[peer->id] = socket_state;
    }

    // Add to peers list
    {
        std::lock_guard<std::mutex> lock(impl_->peers_mutex);
        impl_->peers.push_back(peer);
    }

    return Result<std::shared_ptr<Peer>>::Ok(peer);
}

void P2PNode::DisconnectPeer(uint64_t peer_id) {
    std::lock_guard<std::mutex> lock(impl_->peers_mutex);

    auto it = std::find_if(impl_->peers.begin(), impl_->peers.end(),
                          [peer_id](const std::shared_ptr<Peer>& p) {
                              return p->id == peer_id;
                          });

    if (it != impl_->peers.end()) {
        (*it)->Disconnect();
        impl_->peers.erase(it);
    }
}

void P2PNode::BroadcastMessage(const NetworkMessage& msg) {
    std::lock_guard<std::mutex> lock(impl_->peers_mutex);

    for (auto& peer : impl_->peers) {
        peer->SendMessage(msg);  // Ignore errors
    }
}

Result<void> P2PNode::SendToPeer(uint64_t peer_id, const NetworkMessage& msg) {
    std::lock_guard<std::mutex> lock(impl_->peers_mutex);

    auto it = std::find_if(impl_->peers.begin(), impl_->peers.end(),
                          [peer_id](const std::shared_ptr<Peer>& p) {
                              return p->id == peer_id;
                          });

    if (it == impl_->peers.end()) {
        return Result<void>::Error("Peer not found");
    }

    return (*it)->SendMessage(msg);
}

std::vector<std::shared_ptr<Peer>> P2PNode::GetPeers() const {
    std::lock_guard<std::mutex> lock(impl_->peers_mutex);
    return impl_->peers;
}

size_t P2PNode::GetPeerCount() const {
    std::lock_guard<std::mutex> lock(impl_->peers_mutex);
    return impl_->peers.size();
}

void P2PNode::BanPeer(const NetworkAddress& address, std::chrono::seconds duration) {
    std::lock_guard<std::mutex> lock(impl_->banned_peers_mutex);
    auto ban_until = std::chrono::system_clock::now() + duration;
    impl_->banned_peers[address.ToString()] = ban_until;

    // Disconnect if currently connected
    std::lock_guard<std::mutex> peers_lock(impl_->peers_mutex);
    for (auto it = impl_->peers.begin(); it != impl_->peers.end();) {
        if ((*it)->address.ToString() == address.ToString()) {
            (*it)->Disconnect();
            it = impl_->peers.erase(it);
        } else {
            ++it;
        }
    }
}

bool P2PNode::IsBanned(const NetworkAddress& address) const {
    std::lock_guard<std::mutex> lock(impl_->banned_peers_mutex);
    auto it = impl_->banned_peers.find(address.ToString());
    if (it == impl_->banned_peers.end()) {
        return false;
    }
    return std::chrono::system_clock::now() < it->second;
}

void P2PNode::AddSeedNode(const NetworkAddress& address) {
    // Connect to seed node
    auto result = ConnectToPeer(address);
    if (result.IsError()) {
        // Silently ignore connection failures to seed nodes
        return;
    }
}

Result<void> P2PNode::DiscoverPeers() {
    std::vector<NetworkAddress> all_peers;

    // 1. Load previously discovered peers from peers.dat
    auto load_result = PeerDiscovery::LoadPeerAddresses();
    if (load_result.IsOk()) {
        all_peers = load_result.value.value();
    }

    // 2. Add hardcoded seed nodes
    auto seeds = PeerDiscovery::GetSeedNodes();
    for (const auto& seed : seeds) {
        all_peers.push_back(seed);
    }

    // 3. DNS seed discovery (enabled)
    std::vector<std::string> dns_seeds;
    if (impl_->network_magic == network::MAINNET_MAGIC) {
        // Mainnet P2P DNS seeds
        dns_seeds = {
            "seed-uk.international-coin.org",
            "seed-us.international-coin.org"
        };
    } else {
        // Testnet P2P DNS seeds
        dns_seeds = {
            "test-uk.international-coin.org",
            "test-us.international-coin.org"
        };
    }

    for (const auto& dns_seed : dns_seeds) {
        auto dns_result = PeerDiscovery::DNSSeedQuery(dns_seed);
        if (dns_result.IsOk()) {
            auto dns_peers = dns_result.value.value();
            all_peers.insert(all_peers.end(), dns_peers.begin(), dns_peers.end());
        }
    }

    // 4. Connect to discovered peers
    for (const auto& peer : all_peers) {
        if (impl_->peers.size() >= network::MAX_OUTBOUND_CONNECTIONS) {
            break;
        }

        // Skip if already connected
        bool already_connected = false;
        for (const auto& existing : impl_->peers) {
            if (existing->address.ToString() == peer.ToString()) {
                already_connected = true;
                break;
            }
        }

        if (!already_connected) {
            AddSeedNode(peer);
        }
    }

    return Result<void>::Ok();
}

void P2PNode::BroadcastBlock(const uint256& block_hash) {
    // Create INV message for the new block
    InvVector inv;
    inv.type = InvType::BLOCK;
    inv.hash = block_hash;

    // Serialize INV vector
    std::vector<uint8_t> inv_payload;
    inv_payload.push_back(1); // count = 1
    auto serialized = inv.Serialize();
    inv_payload.insert(inv_payload.end(), serialized.begin(), serialized.end());

    // Create and broadcast INV message
    NetworkMessage inv_msg(impl_->network_magic, "inv", inv_payload);
    BroadcastMessage(inv_msg);
}

void P2PNode::BroadcastTransaction(const uint256& tx_hash) {
    // Create INV message for the new transaction
    InvVector inv;
    inv.type = InvType::TX;
    inv.hash = tx_hash;

    // Serialize INV vector
    std::vector<uint8_t> inv_payload;
    inv_payload.push_back(1); // count = 1
    auto serialized = inv.Serialize();
    inv_payload.insert(inv_payload.end(), serialized.begin(), serialized.end());

    // Create and broadcast INV message
    NetworkMessage inv_msg(impl_->network_magic, "inv", inv_payload);
    BroadcastMessage(inv_msg);
}

// ============================================================================
// MessageHandler Implementation
// ============================================================================

// VERSION message payload structure
struct VersionMessage {
    uint32_t version;
    uint64_t services;
    int64_t timestamp;
    NetworkAddress addr_recv;
    NetworkAddress addr_from;
    uint64_t nonce;
    std::string user_agent;
    int32_t start_height;
    bool relay;

    std::vector<uint8_t> Serialize() const {
        std::vector<uint8_t> data;

        // Version (4 bytes)
        for (int i = 0; i < 4; i++) {
            data.push_back((version >> (i * 8)) & 0xFF);
        }

        // Services (8 bytes)
        for (int i = 0; i < 8; i++) {
            data.push_back((services >> (i * 8)) & 0xFF);
        }

        // Timestamp (8 bytes)
        for (int i = 0; i < 8; i++) {
            data.push_back((timestamp >> (i * 8)) & 0xFF);
        }

        // Addr_recv (without timestamp - 26 bytes)
        auto recv_data = addr_recv.Serialize();
        data.insert(data.end(), recv_data.begin() + 8, recv_data.end()); // Skip timestamp

        // Addr_from (without timestamp - 26 bytes)
        auto from_data = addr_from.Serialize();
        data.insert(data.end(), from_data.begin() + 8, from_data.end()); // Skip timestamp

        // Nonce (8 bytes)
        for (int i = 0; i < 8; i++) {
            data.push_back((nonce >> (i * 8)) & 0xFF);
        }

        // User agent (variable length string)
        uint8_t ua_len = static_cast<uint8_t>(user_agent.size());
        data.push_back(ua_len);
        data.insert(data.end(), user_agent.begin(), user_agent.end());

        // Start height (4 bytes)
        for (int i = 0; i < 4; i++) {
            data.push_back((start_height >> (i * 8)) & 0xFF);
        }

        // Relay (1 byte)
        data.push_back(relay ? 1 : 0);

        return data;
    }

    static Result<VersionMessage> Deserialize(const std::vector<uint8_t>& data) {
        if (data.size() < 85) { // Minimum size
            return Result<VersionMessage>::Error("VERSION payload too short");
        }

        VersionMessage msg;
        size_t pos = 0;

        // Version (4 bytes)
        msg.version = 0;
        for (int i = 0; i < 4; i++) {
            msg.version |= (static_cast<uint32_t>(data[pos++]) << (i * 8));
        }

        // Services (8 bytes)
        msg.services = 0;
        for (int i = 0; i < 8; i++) {
            msg.services |= (static_cast<uint64_t>(data[pos++]) << (i * 8));
        }

        // Timestamp (8 bytes)
        msg.timestamp = 0;
        for (int i = 0; i < 8; i++) {
            msg.timestamp |= (static_cast<int64_t>(data[pos++]) << (i * 8));
        }

        // Addr_recv (26 bytes without timestamp)
        std::vector<uint8_t> recv_addr_data(34, 0);
        std::copy(data.begin() + pos, data.begin() + pos + 26, recv_addr_data.begin() + 8);
        auto recv_result = NetworkAddress::Deserialize(recv_addr_data);
        if (recv_result.IsError()) {
            return Result<VersionMessage>::Error("Failed to parse addr_recv");
        }
        msg.addr_recv = recv_result.value.value();
        pos += 26;

        // Addr_from (26 bytes without timestamp)
        std::vector<uint8_t> from_addr_data(34, 0);
        std::copy(data.begin() + pos, data.begin() + pos + 26, from_addr_data.begin() + 8);
        auto from_result = NetworkAddress::Deserialize(from_addr_data);
        if (from_result.IsError()) {
            return Result<VersionMessage>::Error("Failed to parse addr_from");
        }
        msg.addr_from = from_result.value.value();
        pos += 26;

        // Nonce (8 bytes)
        msg.nonce = 0;
        for (int i = 0; i < 8; i++) {
            msg.nonce |= (static_cast<uint64_t>(data[pos++]) << (i * 8));
        }

        // User agent (variable length)
        if (pos >= data.size()) {
            return Result<VersionMessage>::Error("Incomplete VERSION message");
        }
        uint8_t ua_len = data[pos++];
        if (pos + ua_len > data.size()) {
            return Result<VersionMessage>::Error("Invalid user agent length");
        }
        msg.user_agent = std::string(data.begin() + pos, data.begin() + pos + ua_len);
        pos += ua_len;

        // Start height (4 bytes)
        if (pos + 4 > data.size()) {
            return Result<VersionMessage>::Error("Missing start height");
        }
        msg.start_height = 0;
        for (int i = 0; i < 4; i++) {
            msg.start_height |= (static_cast<int32_t>(data[pos++]) << (i * 8));
        }

        // Relay (1 byte, optional)
        if (pos < data.size()) {
            msg.relay = data[pos] != 0;
        } else {
            msg.relay = true; // Default to true for older protocol versions
        }

        return Result<VersionMessage>::Ok(msg);
    }
};

Result<void> MessageHandler::HandleVersion(Peer& peer,
                                          const std::vector<uint8_t>& payload) {
    // Parse VERSION message
    auto result = VersionMessage::Deserialize(payload);
    if (result.IsError()) {
        peer.IncreaseBanScore(10);
        return Result<void>::Error("Invalid VERSION message: " + result.error);
    }

    auto version_msg = result.value.value();

    // Check protocol version
    if (version_msg.version < network::MIN_PROTOCOL_VERSION) {
        return Result<void>::Error("Peer protocol version too old: " +
                                   std::to_string(version_msg.version));
    }

    // Update peer information
    peer.version = version_msg.version;
    peer.services = version_msg.services;

    // Send VERACK response
    NetworkMessage verack(network::MAINNET_MAGIC, "verack", {});
    auto send_result = peer.SendMessage(verack);
    if (send_result.IsError()) {
        return Result<void>::Error("Failed to send VERACK: " + send_result.error);
    }

    return Result<void>::Ok();
}

Result<void> MessageHandler::HandleVerack(Peer& peer) {
    // VERACK is just an acknowledgment with no payload
    // Connection is now fully established
    peer.last_message_time = std::chrono::system_clock::now();
    return Result<void>::Ok();
}

Result<void> MessageHandler::HandleAddr(const std::vector<uint8_t>& payload) {
    // ADDR message format:
    // - count (varint): number of addresses
    // - addresses (count * NetworkAddress): peer addresses

    if (payload.empty()) {
        return Result<void>::Error("Empty ADDR payload");
    }

    size_t pos = 0;

    // Read count (simple varint, max 255 for now)
    uint8_t count = payload[pos++];

    if (count > 100) {
        // Sanity check: reject unreasonably large ADDR messages
        return Result<void>::Error("ADDR message contains too many addresses");
    }

    std::vector<NetworkAddress> addresses;
    addresses.reserve(count);

    // Parse each address
    for (uint8_t i = 0; i < count; i++) {
        if (pos + 34 > payload.size()) {
            return Result<void>::Error("Truncated ADDR message");
        }

        // Extract 34 bytes for NetworkAddress
        std::vector<uint8_t> addr_data(payload.begin() + pos,
                                       payload.begin() + pos + 34);
        pos += 34;

        auto result = NetworkAddress::Deserialize(addr_data);
        if (result.IsError()) {
            // Skip invalid addresses but continue processing
            continue;
        }

        auto addr = result.value.value();

        // Filter out non-routable addresses
        if (!addr.IsRoutable()) {
            continue;
        }

        addresses.push_back(addr);
    }

    // Save the received addresses to peers.dat
    auto save_result = PeerDiscovery::SavePeerAddresses(addresses);
    if (save_result.IsError()) {
        // Log error but don't fail the message handling
        // The addresses are still in memory
    }

    return Result<void>::Ok();
}

Result<void> MessageHandler::HandleInv(Peer& peer,
                                      const std::vector<uint8_t>& payload) {
    // INV message format:
    // - count (varint): number of inventory items
    // - inventory[] (count * InvVector): inventory items

    if (payload.empty()) {
        return Result<void>::Error("Empty INV payload");
    }

    size_t pos = 0;

    // Read count (simple varint, max 255 for now)
    uint8_t count = payload[pos++];

    if (count > 50) {
        // Sanity check: limit INV messages
        return Result<void>::Error("INV message contains too many items");
    }

    std::vector<InvVector> inv_items;
    inv_items.reserve(count);

    // Parse each inventory item
    for (uint8_t i = 0; i < count; i++) {
        if (pos + 36 > payload.size()) {
            return Result<void>::Error("Truncated INV message");
        }

        // Extract 36 bytes for InvVector (4 + 32)
        std::vector<uint8_t> inv_data(payload.begin() + pos,
                                      payload.begin() + pos + 36);
        pos += 36;

        auto result = InvVector::Deserialize(inv_data);
        if (result.IsError()) {
            peer.IncreaseBanScore(5);
            return Result<void>::Error("Invalid inventory item: " + result.error);
        }

        inv_items.push_back(result.value.value());
    }

    // Process inventory items
    std::vector<InvVector> items_to_request;

    for (const auto& inv : inv_items) {
        if (inv.type == InvType::BLOCK) {
            // TODO: Check if we already have this block
            // For now, request all blocks we don't have
            items_to_request.push_back(inv);
        } else if (inv.type == InvType::TX) {
            // TODO: Check if we already have this transaction
            // For now, request all transactions we don't have
            items_to_request.push_back(inv);
        }
    }

    // Send GETDATA message to request the items
    if (!items_to_request.empty()) {
        std::vector<uint8_t> getdata_payload;

        // Count
        getdata_payload.push_back(static_cast<uint8_t>(items_to_request.size()));

        // Serialize each item
        for (const auto& inv : items_to_request) {
            auto serialized = inv.Serialize();
            getdata_payload.insert(getdata_payload.end(),
                                 serialized.begin(), serialized.end());
        }

        NetworkMessage getdata(network::MAINNET_MAGIC, "getdata", getdata_payload);
        auto send_result = peer.SendMessage(getdata);
        if (send_result.IsError()) {
            return Result<void>::Error("Failed to send GETDATA: " + send_result.error);
        }
    }

    return Result<void>::Ok();
}

Result<void> MessageHandler::HandleGetData(Peer& peer,
                                          const std::vector<uint8_t>& payload,
                                          Blockchain* blockchain) {
    // GETDATA message format (same as INV):
    // - count (varint): number of inventory items
    // - inventory[] (count * InvVector): requested items

    if (payload.empty()) {
        return Result<void>::Error("Empty GETDATA payload");
    }

    // If blockchain not provided, we can't serve data
    if (!blockchain) {
        return Result<void>::Error("Blockchain not available");
    }

    size_t pos = 0;

    // Read count
    uint8_t count = payload[pos++];

    if (count > 50) {
        peer.IncreaseBanScore(5);
        return Result<void>::Error("GETDATA message contains too many items");
    }

    // Collect items not found for batch NOTFOUND response
    std::vector<InvVector> not_found_items;

    // Parse and serve each requested item
    for (uint8_t i = 0; i < count; i++) {
        if (pos + 36 > payload.size()) {
            peer.IncreaseBanScore(5);
            return Result<void>::Error("Truncated GETDATA message");
        }

        // Extract InvVector
        std::vector<uint8_t> inv_data(payload.begin() + pos,
                                      payload.begin() + pos + 36);
        pos += 36;

        auto result = InvVector::Deserialize(inv_data);
        if (result.IsError()) {
            peer.IncreaseBanScore(5);
            continue; // Skip invalid items
        }

        auto inv = result.value.value();

        if (inv.type == InvType::BLOCK) {
            // Look up block in blockchain
            auto block_result = blockchain->GetBlock(inv.hash);
            if (block_result.IsOk()) {
                // Serialize and send block
                auto block = block_result.value.value();
                auto block_payload = block.Serialize();

                NetworkMessage block_msg(network::MAINNET_MAGIC, "block", block_payload);
                auto send_result = peer.SendMessage(block_msg);
                if (send_result.IsError()) {
                    // Failed to send, but don't fail the whole handler
                    continue;
                }
            } else {
                // Block not found
                not_found_items.push_back(inv);
            }

        } else if (inv.type == InvType::TX) {
            // First check mempool for unconfirmed transactions
            bool found = false;
            auto& mempool = blockchain->GetMempool();
            auto mempool_txs = mempool.GetAllTransactions();

            for (const auto& tx : mempool_txs) {
                if (tx.GetHash() == inv.hash) {
                    // Found in mempool - send it
                    auto tx_payload = tx.Serialize();
                    NetworkMessage tx_msg(network::MAINNET_MAGIC, "tx", tx_payload);
                    auto send_result = peer.SendMessage(tx_msg);
                    if (send_result.IsOk()) {
                        found = true;
                    }
                    break;
                }
            }

            // If not in mempool, check blockchain
            if (!found) {
                auto tx_result = blockchain->GetTransaction(inv.hash);
                if (tx_result.IsOk()) {
                    // Found in blockchain - send it
                    auto tx = tx_result.value.value();
                    auto tx_payload = tx.Serialize();
                    NetworkMessage tx_msg(network::MAINNET_MAGIC, "tx", tx_payload);
                    auto send_result = peer.SendMessage(tx_msg);
                    if (send_result.IsError()) {
                        continue;
                    }
                } else {
                    // Transaction not found anywhere
                    not_found_items.push_back(inv);
                }
            }
        }
    }

    // Send NOTFOUND message for items we couldn't find (batch)
    if (!not_found_items.empty()) {
        std::vector<uint8_t> notfound_payload;
        notfound_payload.push_back(static_cast<uint8_t>(not_found_items.size()));

        for (const auto& inv : not_found_items) {
            auto serialized = inv.Serialize();
            notfound_payload.insert(notfound_payload.end(),
                                  serialized.begin(), serialized.end());
        }

        NetworkMessage notfound_msg(network::MAINNET_MAGIC, "notfound", notfound_payload);
        peer.SendMessage(notfound_msg);
    }

    return Result<void>::Ok();
}

Result<void> MessageHandler::HandleBlock(Peer& peer,
                                        const std::vector<uint8_t>& payload) {
    // BLOCK message format:
    // - block (Block): full block data

    if (payload.empty()) {
        return Result<void>::Error("Empty BLOCK payload");
    }

    // Deserialize block
    auto result = Block::Deserialize(payload);
    if (result.IsError()) {
        peer.IncreaseBanScore(10);
        return Result<void>::Error("Invalid block: " + result.error);
    }

    auto block = result.value.value();

    // Calculate block hash
    [[maybe_unused]] auto block_hash = block.GetHash();

    // Basic validation
    // 1. Verify block structure and PoW
    auto verify_result = block.Verify();
    if (verify_result.IsError()) {
        peer.IncreaseBanScore(100); // Severe violation
        return Result<void>::Error("Block verification failed: " + verify_result.error);
    }

    // 2. Check block timestamp (not too far in the future)
    auto now = std::chrono::system_clock::now();
    auto block_time = std::chrono::system_clock::time_point(
        std::chrono::seconds(block.header.timestamp));
    auto time_diff = std::chrono::duration_cast<std::chrono::seconds>(
        block_time - now);

    if (time_diff.count() > 7200) { // More than 2 hours in the future
        peer.IncreaseBanScore(10);
        return Result<void>::Error("Block timestamp too far in the future");
    }

    // TODO: Additional validation:
    // - Check if we already have this block
    // - Verify block connects to the chain (check prev_block_hash)
    // - If parent is missing, add to orphan pool
    // - Validate all transactions in the block
    // - Check merkle root matches transactions
    // - Add to blockchain if valid

    // For now, we just acknowledge receipt
    // In a full implementation, we would:
    // 1. Add block to blockchain
    // 2. Relay to other peers
    // 3. Remove transactions from mempool

    return Result<void>::Ok();
}

Result<void> MessageHandler::HandleTx(Peer& peer,
                                     const std::vector<uint8_t>& payload) {
    // TX message format:
    // - transaction (Transaction): full transaction data

    if (payload.empty()) {
        return Result<void>::Error("Empty TX payload");
    }

    // Deserialize transaction
    auto result = Transaction::Deserialize(payload);
    if (result.IsError()) {
        peer.IncreaseBanScore(10);
        return Result<void>::Error("Invalid transaction: " + result.error);
    }

    auto tx = result.value.value();

    // Calculate transaction hash
    [[maybe_unused]] auto tx_hash = tx.GetHash();

    // Basic validation
    // 1. Check transaction format
    if (tx.inputs.empty()) {
        peer.IncreaseBanScore(10);
        return Result<void>::Error("Transaction has no inputs");
    }

    if (tx.outputs.empty()) {
        peer.IncreaseBanScore(10);
        return Result<void>::Error("Transaction has no outputs");
    }

    // TODO: Additional validation:
    // - Verify signatures
    // - Check inputs exist and are unspent
    // - Verify input amounts >= output amounts
    // - Check for double-spending
    // - Add to mempool if valid
    // - Relay to other peers

    // For now, we just acknowledge receipt
    return Result<void>::Ok();
}

Result<void> MessageHandler::HandleGetHeaders(Peer& peer,
                                             const std::vector<uint8_t>& payload) {
    // TODO: Implement GETHEADERS message handling
    return Result<void>::Error("Not implemented");
}

Result<void> MessageHandler::HandleHeaders(Peer& peer,
                                          const std::vector<uint8_t>& payload) {
    // TODO: Implement HEADERS message handling
    return Result<void>::Error("Not implemented");
}

Result<void> MessageHandler::HandlePing(Peer& peer,
                                       const std::vector<uint8_t>& payload) {
    // PING message format:
    // - nonce (8 bytes): random nonce for matching with PONG

    if (payload.size() != 8) {
        peer.IncreaseBanScore(5);
        return Result<void>::Error("Invalid PING message size");
    }

    // Read nonce
    uint64_t nonce = 0;
    for (int i = 0; i < 8; i++) {
        nonce |= (static_cast<uint64_t>(payload[i]) << (i * 8));
    }

    // Send PONG response with same nonce
    std::vector<uint8_t> pong_payload;
    for (int i = 0; i < 8; i++) {
        pong_payload.push_back((nonce >> (i * 8)) & 0xFF);
    }

    NetworkMessage pong(network::MAINNET_MAGIC, "pong", pong_payload);
    auto send_result = peer.SendMessage(pong);
    if (send_result.IsError()) {
        return Result<void>::Error("Failed to send PONG: " + send_result.error);
    }

    // Update last message time
    peer.last_message_time = std::chrono::system_clock::now();

    return Result<void>::Ok();
}

Result<void> MessageHandler::HandlePong(Peer& peer,
                                       const std::vector<uint8_t>& payload) {
    // PONG message format:
    // - nonce (8 bytes): nonce from PING

    if (payload.size() != 8) {
        peer.IncreaseBanScore(5);
        return Result<void>::Error("Invalid PONG message size");
    }

    // Read nonce
    [[maybe_unused]] uint64_t nonce = 0;
    for (int i = 0; i < 8; i++) {
        nonce |= (static_cast<uint64_t>(payload[i]) << (i * 8));
    }

    // TODO: Verify nonce matches our PING
    // For now, just update last message time

    peer.last_message_time = std::chrono::system_clock::now();

    return Result<void>::Ok();
}

// ============================================================================
// PeerDiscovery Implementation
// ============================================================================

Result<std::vector<NetworkAddress>> PeerDiscovery::DNSSeedQuery(
    const std::string& dns_seed) {
    std::vector<NetworkAddress> addresses;

    struct addrinfo hints;
    std::memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    struct addrinfo* result;
    if (getaddrinfo(dns_seed.c_str(), nullptr, &hints, &result) != 0) {
        return Result<std::vector<NetworkAddress>>::Error("DNS query failed for: " + dns_seed);
    }

    // Process results
    for (struct addrinfo* rp = result; rp != nullptr; rp = rp->ai_next) {
        if (rp->ai_family == AF_INET) {
            struct sockaddr_in* sa = (struct sockaddr_in*)rp->ai_addr;
            char ip_str[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &(sa->sin_addr), ip_str, INET_ADDRSTRLEN);
            addresses.emplace_back(std::string(ip_str), network::MAINNET_P2P_PORT);
        } else if (rp->ai_family == AF_INET6) {
            struct sockaddr_in6* sa = (struct sockaddr_in6*)rp->ai_addr;
            char ip_str[INET6_ADDRSTRLEN];
            inet_ntop(AF_INET6, &(sa->sin6_addr), ip_str, INET6_ADDRSTRLEN);
            addresses.emplace_back(std::string(ip_str), network::MAINNET_P2P_PORT);
        }
    }

    freeaddrinfo(result);

    if (addresses.empty()) {
        return Result<std::vector<NetworkAddress>>::Error("No addresses found for: " + dns_seed);
    }

    return Result<std::vector<NetworkAddress>>::Ok(addresses);
}

std::vector<NetworkAddress> PeerDiscovery::GetSeedNodes(bool testnet) {
    // Hardcoded seed nodes for INTcoin mainnet and testnet
    // DNS: seed-uk.international-coin.org, seed-us.international-coin.org (mainnet)
    //      test-uk.international-coin.org, test-us.international-coin.org (testnet)
    std::vector<NetworkAddress> seeds;

    if (testnet) {
        // Testnet seed nodes (IPs for seed resolution testing)
        seeds.emplace_back("192.168.100.2", network::TESTNET_P2P_PORT);  // test-uk.international-coin.org
        seeds.emplace_back("192.168.100.3", network::TESTNET_P2P_PORT);  // test-us.international-coin.org
    } else {
        // Mainnet seed nodes (IPs for seed resolution testing)
        seeds.emplace_back("51.155.97.192", network::MAINNET_P2P_PORT);  // seed-uk.international-coin.org
        seeds.emplace_back("74.208.112.43", network::MAINNET_P2P_PORT);  // seed-us.international-coin.org

        // Tor hidden service seed node
        // Note: Tor addresses need special handling via SOCKS5 proxy
        // seeds.emplace_back("2nrhdp7i4dricaf362hwnajj27lscbmimggvjetwjhuwgtdnfcurxzyd.onion", 9333);
    }

    return seeds;
}

Result<void> PeerDiscovery::SavePeerAddresses(
    const std::vector<NetworkAddress>& addresses) {
    // Get data directory (use home directory if no data dir is set)
    const char* home = std::getenv("HOME");
    if (!home) {
        return Result<void>::Error("Cannot determine home directory");
    }

    std::string peers_file = std::string(home) + "/.intcoin/peers.dat";

    // Create directory if it doesn't exist
    std::string data_dir = std::string(home) + "/.intcoin";
    #ifdef _WIN32
        _mkdir(data_dir.c_str());
    #else
        mkdir(data_dir.c_str(), 0755);
    #endif

    // Load existing addresses
    auto load_result = LoadPeerAddresses();
    std::vector<NetworkAddress> all_addresses;

    if (load_result.IsOk()) {
        all_addresses = load_result.value.value();
    }

    // Add new addresses (avoid duplicates)
    for (const auto& new_addr : addresses) {
        bool duplicate = false;
        for (const auto& existing : all_addresses) {
            if (existing.ToString() == new_addr.ToString()) {
                duplicate = true;
                break;
            }
        }
        if (!duplicate) {
            all_addresses.push_back(new_addr);
        }
    }

    // Limit to 10,000 addresses
    if (all_addresses.size() > 10000) {
        all_addresses.resize(10000);
    }

    // Open file for writing
    std::ofstream file(peers_file, std::ios::binary);
    if (!file) {
        return Result<void>::Error("Failed to open peers.dat for writing");
    }

    // Write version number (4 bytes)
    uint32_t version = 1;
    file.write(reinterpret_cast<const char*>(&version), sizeof(version));

    // Write count (4 bytes)
    uint32_t count = static_cast<uint32_t>(all_addresses.size());
    file.write(reinterpret_cast<const char*>(&count), sizeof(count));

    // Write each address
    for (const auto& addr : all_addresses) {
        auto serialized = addr.Serialize();
        file.write(reinterpret_cast<const char*>(serialized.data()),
                  serialized.size());
    }

    file.close();

    return Result<void>::Ok();
}

Result<std::vector<NetworkAddress>> PeerDiscovery::LoadPeerAddresses() {
    // Get data directory
    const char* home = std::getenv("HOME");
    if (!home) {
        return Result<std::vector<NetworkAddress>>::Error(
            "Cannot determine home directory");
    }

    std::string peers_file = std::string(home) + "/.intcoin/peers.dat";

    // Open file for reading
    std::ifstream file(peers_file, std::ios::binary);
    if (!file) {
        // File doesn't exist yet - not an error
        return Result<std::vector<NetworkAddress>>::Ok(
            std::vector<NetworkAddress>());
    }

    // Read version number (4 bytes)
    uint32_t version;
    file.read(reinterpret_cast<char*>(&version), sizeof(version));
    if (!file || version != 1) {
        return Result<std::vector<NetworkAddress>>::Error(
            "Invalid peers.dat version");
    }

    // Read count (4 bytes)
    uint32_t count;
    file.read(reinterpret_cast<char*>(&count), sizeof(count));
    if (!file) {
        return Result<std::vector<NetworkAddress>>::Error(
            "Failed to read peer count");
    }

    // Sanity check
    if (count > 100000) {
        return Result<std::vector<NetworkAddress>>::Error(
            "Peer count too large");
    }

    std::vector<NetworkAddress> addresses;
    addresses.reserve(count);

    // Read each address
    for (uint32_t i = 0; i < count; i++) {
        std::vector<uint8_t> addr_data(34);
        file.read(reinterpret_cast<char*>(addr_data.data()), 34);
        if (!file) {
            // Partial read - file may be corrupted
            break;
        }

        auto result = NetworkAddress::Deserialize(addr_data);
        if (result.IsOk()) {
            addresses.push_back(result.value.value());
        }
    }

    file.close();

    return Result<std::vector<NetworkAddress>>::Ok(addresses);
}

} // namespace intcoin
