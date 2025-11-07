// Copyright (c) 2025 INTcoin Core (Maddison Lane)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "intcoin/p2p.h"
#include "intcoin/crypto.h"
#include <cstring>
#include <algorithm>
#include <thread>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <fcntl.h>
#include <errno.h>

namespace intcoin {
namespace p2p {

// MessageHeader implementation

std::vector<uint8_t> MessageHeader::serialize() const {
    std::vector<uint8_t> buffer;

    // Magic (4 bytes)
    buffer.push_back(static_cast<uint8_t>(magic & 0xFF));
    buffer.push_back(static_cast<uint8_t>((magic >> 8) & 0xFF));
    buffer.push_back(static_cast<uint8_t>((magic >> 16) & 0xFF));
    buffer.push_back(static_cast<uint8_t>((magic >> 24) & 0xFF));

    // Type (4 bytes)
    uint32_t type_val = static_cast<uint32_t>(type);
    buffer.push_back(static_cast<uint8_t>(type_val & 0xFF));
    buffer.push_back(static_cast<uint8_t>((type_val >> 8) & 0xFF));
    buffer.push_back(static_cast<uint8_t>((type_val >> 16) & 0xFF));
    buffer.push_back(static_cast<uint8_t>((type_val >> 24) & 0xFF));

    // Length (4 bytes)
    buffer.push_back(static_cast<uint8_t>(length & 0xFF));
    buffer.push_back(static_cast<uint8_t>((length >> 8) & 0xFF));
    buffer.push_back(static_cast<uint8_t>((length >> 16) & 0xFF));
    buffer.push_back(static_cast<uint8_t>((length >> 24) & 0xFF));

    // Checksum (32 bytes)
    buffer.insert(buffer.end(), checksum.begin(), checksum.end());

    return buffer;
}

MessageHeader MessageHeader::deserialize(const std::vector<uint8_t>& data) {
    MessageHeader header;
    if (data.size() < 44) return header;

    size_t offset = 0;

    // Magic
    header.magic = static_cast<uint32_t>(data[offset]) |
                   (static_cast<uint32_t>(data[offset + 1]) << 8) |
                   (static_cast<uint32_t>(data[offset + 2]) << 16) |
                   (static_cast<uint32_t>(data[offset + 3]) << 24);
    offset += 4;

    // Type
    uint32_t type_val = static_cast<uint32_t>(data[offset]) |
                        (static_cast<uint32_t>(data[offset + 1]) << 8) |
                        (static_cast<uint32_t>(data[offset + 2]) << 16) |
                        (static_cast<uint32_t>(data[offset + 3]) << 24);
    header.type = static_cast<MessageType>(type_val);
    offset += 4;

    // Length
    header.length = static_cast<uint32_t>(data[offset]) |
                    (static_cast<uint32_t>(data[offset + 1]) << 8) |
                    (static_cast<uint32_t>(data[offset + 2]) << 16) |
                    (static_cast<uint32_t>(data[offset + 3]) << 24);
    offset += 4;

    // Checksum
    std::copy(data.begin() + offset, data.begin() + offset + 32, header.checksum.begin());

    return header;
}

// Message implementation

Message::Message(MessageType type, const std::vector<uint8_t>& data) {
    header.type = type;
    header.length = static_cast<uint32_t>(data.size());
    payload = data;
    header.checksum = get_checksum();
}

std::vector<uint8_t> Message::serialize() const {
    std::vector<uint8_t> buffer = header.serialize();
    buffer.insert(buffer.end(), payload.begin(), payload.end());
    return buffer;
}

Message Message::deserialize(const std::vector<uint8_t>& data) {
    Message msg;
    if (data.size() < 44) return msg;

    msg.header = MessageHeader::deserialize(data);

    if (data.size() >= 44 + msg.header.length) {
        msg.payload.assign(data.begin() + 44, data.begin() + 44 + msg.header.length);
    }

    return msg;
}

Hash256 Message::get_checksum() const {
    return crypto::SHA3_256::hash(payload.data(), payload.size());
}

// InvVector implementation

std::vector<uint8_t> InvVector::serialize() const {
    std::vector<uint8_t> buffer;

    // Type
    uint32_t type_val = static_cast<uint32_t>(type);
    buffer.push_back(static_cast<uint8_t>(type_val & 0xFF));
    buffer.push_back(static_cast<uint8_t>((type_val >> 8) & 0xFF));
    buffer.push_back(static_cast<uint8_t>((type_val >> 16) & 0xFF));
    buffer.push_back(static_cast<uint8_t>((type_val >> 24) & 0xFF));

    // Hash
    buffer.insert(buffer.end(), hash.begin(), hash.end());

    return buffer;
}

InvVector InvVector::deserialize(const std::vector<uint8_t>& data) {
    InvVector inv;
    if (data.size() < 36) return inv;

    // Type
    uint32_t type_val = static_cast<uint32_t>(data[0]) |
                        (static_cast<uint32_t>(data[1]) << 8) |
                        (static_cast<uint32_t>(data[2]) << 16) |
                        (static_cast<uint32_t>(data[3]) << 24);
    inv.type = static_cast<InvVector::Type>(type_val);

    // Hash
    std::copy(data.begin() + 4, data.begin() + 36, inv.hash.begin());

    return inv;
}

// Peer implementation

bool Peer::is_alive() const {
    if (!connected) return false;

    auto now = std::chrono::system_clock::now().time_since_epoch().count();
    auto diff = (now - last_seen) / 1000000000;  // Convert to seconds

    return diff < protocol::TIMEOUT_SECONDS;
}

void Peer::update_last_seen() {
    last_seen = std::chrono::system_clock::now().time_since_epoch().count();
}

// Network implementation

Network::Network(uint16_t port, bool is_testnet)
    : port_(port)
    , is_testnet_(is_testnet)
    , running_(false)
    , listen_socket_(-1)
{
}

Network::~Network() {
    stop();
}

bool Network::start() {
    if (running_) return false;

    running_ = true;

    // Start listening socket
    listen_socket_ = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_socket_ < 0) {
        running_ = false;
        return false;
    }

    // Set socket options
    int opt = 1;
    setsockopt(listen_socket_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // Bind to port
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port_);

    if (bind(listen_socket_, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        close(listen_socket_);
        listen_socket_ = -1;
        running_ = false;
        return false;
    }

    // Listen for connections
    if (listen(listen_socket_, 10) < 0) {
        close(listen_socket_);
        listen_socket_ = -1;
        running_ = false;
        return false;
    }

    // Set non-blocking
    fcntl(listen_socket_, F_SETFL, O_NONBLOCK);

    // Start peer discovery thread
    discovery_thread_ = std::thread([this]() {
        while (running_) {
            discover_peers();
            std::this_thread::sleep_for(std::chrono::seconds(60));
        }
    });

    // Start connection maintenance thread
    maintenance_thread_ = std::thread([this]() {
        while (running_) {
            maintain_connections();
            std::this_thread::sleep_for(std::chrono::seconds(30));
        }
    });

    // Start accept thread
    accept_thread_ = std::thread([this]() {
        while (running_) {
            struct sockaddr_in client_addr;
            socklen_t client_len = sizeof(client_addr);
            int client_socket = accept(listen_socket_, (struct sockaddr*)&client_addr, &client_len);

            if (client_socket >= 0) {
                // New inbound connection
                PeerAddress peer_addr;
                peer_addr.ip = inet_ntoa(client_addr.sin_addr);
                peer_addr.port = ntohs(client_addr.sin_port);

                if (peers_.size() < protocol::MAX_PEERS) {
                    Peer peer(peer_addr);
                    peer.connected = true;
                    peer.inbound = true;
                    peer.socket_fd = client_socket;
                    peer.update_last_seen();
                    peers_.push_back(peer);
                }
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    });

    return true;
}

void Network::stop() {
    if (!running_) return;

    running_ = false;

    // Wait for threads to finish
    if (discovery_thread_.joinable()) discovery_thread_.join();
    if (maintenance_thread_.joinable()) maintenance_thread_.join();
    if (accept_thread_.joinable()) accept_thread_.join();

    // Close all peer sockets
    for (auto& peer : peers_) {
        if (peer.socket_fd >= 0) {
            close(peer.socket_fd);
        }
    }

    // Close listening socket
    if (listen_socket_ >= 0) {
        close(listen_socket_);
        listen_socket_ = -1;
    }

    // Disconnect all peers
    peers_.clear();
}

bool Network::connect_to_peer(const PeerAddress& addr) {
    // Check if already connected
    if (find_peer(addr) != nullptr) {
        return false;
    }

    // Check peer limit
    if (peers_.size() >= protocol::MAX_PEERS) {
        return false;
    }

    // Create actual TCP connection
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        return false;
    }

    // Set non-blocking
    fcntl(sock, F_SETFL, O_NONBLOCK);

    // Set timeout
    struct timeval timeout;
    timeout.tv_sec = 10;
    timeout.tv_usec = 0;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));

    // Connect to peer
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(addr.port);

    if (inet_pton(AF_INET, addr.ip.c_str(), &server_addr.sin_addr) <= 0) {
        close(sock);
        return false;
    }

    int result = connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr));
    if (result < 0 && errno != EINPROGRESS) {
        close(sock);
        return false;
    }

    Peer peer(addr);
    peer.connected = true;
    peer.inbound = false;
    peer.socket_fd = sock;
    peer.update_last_seen();

    peers_.push_back(peer);

    // Send version message
    uint32_t version = protocol::PROTOCOL_VERSION;
    uint64_t services = 1;  // NODE_NETWORK
    uint64_t timestamp = std::chrono::system_clock::now().time_since_epoch().count() / 1000000000;

    std::vector<uint8_t> version_payload;
    // Version (4 bytes)
    version_payload.push_back(version & 0xFF);
    version_payload.push_back((version >> 8) & 0xFF);
    version_payload.push_back((version >> 16) & 0xFF);
    version_payload.push_back((version >> 24) & 0xFF);
    // Services (8 bytes)
    for (int i = 0; i < 8; i++) {
        version_payload.push_back((services >> (i * 8)) & 0xFF);
    }
    // Timestamp (8 bytes)
    for (int i = 0; i < 8; i++) {
        version_payload.push_back((timestamp >> (i * 8)) & 0xFF);
    }

    Message version_msg(MessageType::VERSION, version_payload);
    send_message(addr, version_msg);

    return true;
}

void Network::disconnect_peer(const PeerAddress& addr) {
    auto it = std::remove_if(peers_.begin(), peers_.end(),
        [&addr](Peer& p) {
            if (p.address.ip == addr.ip && p.address.port == addr.port) {
                if (p.socket_fd >= 0) {
                    close(p.socket_fd);
                    p.socket_fd = -1;
                }
                return true;
            }
            return false;
        });
    peers_.erase(it, peers_.end());
}

void Network::broadcast_block(const Block& block) {
    // Create inventory message
    InvVector inv(InvVector::Type::BLOCK, block.get_hash());
    std::vector<uint8_t> payload = inv.serialize();
    Message msg(MessageType::INV, payload);

    // Send to all connected peers
    for (const auto& peer : peers_) {
        if (peer.connected) {
            send_message(peer.address, msg);
        }
    }
}

void Network::broadcast_transaction(const Transaction& tx) {
    // Create inventory message
    InvVector inv(InvVector::Type::TX, tx.get_hash());
    std::vector<uint8_t> payload = inv.serialize();
    Message msg(MessageType::INV, payload);

    // Send to all connected peers
    for (const auto& peer : peers_) {
        if (peer.connected) {
            send_message(peer.address, msg);
        }
    }
}

void Network::send_message(const PeerAddress& addr, const Message& msg) {
    // Find peer
    Peer* peer = find_peer(addr);
    if (!peer || !peer->connected || peer->socket_fd < 0) {
        return;
    }

    // Serialize message
    std::vector<uint8_t> data = msg.serialize();

    // Send via TCP
    ssize_t sent = send(peer->socket_fd, data.data(), data.size(), 0);
    if (sent < 0) {
        // Connection failed, disconnect peer
        peer->connected = false;
    }
}

std::vector<PeerAddress> Network::get_peers() const {
    std::vector<PeerAddress> addrs;
    for (const auto& peer : peers_) {
        if (peer.connected) {
            addrs.push_back(peer.address);
        }
    }
    return addrs;
}

void Network::add_seed_node(const PeerAddress& addr) {
    seed_nodes_.push_back(addr);
}

size_t Network::peer_count() const {
    return std::count_if(peers_.begin(), peers_.end(),
        [](const Peer& p) { return p.connected; });
}

void Network::handle_version(const Message& msg, const PeerAddress& from) {
    // Parse version message
    if (msg.payload.size() < 20) return;

    size_t offset = 0;
    uint32_t version = msg.payload[offset] |
                      (msg.payload[offset + 1] << 8) |
                      (msg.payload[offset + 2] << 16) |
                      (msg.payload[offset + 3] << 24);
    offset += 4;

    uint64_t services = 0;
    for (int i = 0; i < 8; i++) {
        services |= (static_cast<uint64_t>(msg.payload[offset + i]) << (i * 8));
    }
    offset += 8;

    // Update peer info
    Peer* peer = find_peer(from);
    if (peer) {
        peer->protocol_version = version;
        peer->services = services;

        // Send verack
        Message verack(MessageType::VERACK, std::vector<uint8_t>());
        send_message(from, verack);
    }
}

void Network::handle_inv(const Message& msg, const PeerAddress& from) {
    // Parse inventory vectors
    if (msg.payload.size() < 36) return;

    size_t offset = 0;
    while (offset + 36 <= msg.payload.size()) {
        std::vector<uint8_t> inv_data(msg.payload.begin() + offset,
                                      msg.payload.begin() + offset + 36);
        InvVector inv = InvVector::deserialize(inv_data);

        // Request data we don't have
        Message getdata(MessageType::GETDATA, inv_data);
        send_message(from, getdata);

        offset += 36;
    }
}

void Network::handle_getdata(const Message& msg, const PeerAddress& from) {
    // Parse inventory vectors and send requested data
    if (msg.payload.size() < 36) return;

    size_t offset = 0;
    while (offset + 36 <= msg.payload.size()) {
        std::vector<uint8_t> inv_data(msg.payload.begin() + offset,
                                      msg.payload.begin() + offset + 36);
        InvVector inv = InvVector::deserialize(inv_data);

        // Look up and send requested data
        if (inv.type == InvVector::Type::BLOCK) {
            // Look up block from blockchain
            if (block_lookup_callback_) {
                auto block_opt = block_lookup_callback_(inv.hash);
                if (block_opt.has_value()) {
                    // Send block
                    std::vector<uint8_t> block_data = block_opt->serialize();
                    Message block_msg(MessageType::BLOCK, block_data);
                    send_message(from, block_msg);
                } else {
                    // Block not found - send NOTFOUND
                    Message notfound_msg(MessageType::NOTFOUND, inv_data);
                    send_message(from, notfound_msg);
                }
            }
        } else if (inv.type == InvVector::Type::TX) {
            // Look up transaction from mempool or blockchain
            if (tx_lookup_callback_) {
                auto tx_opt = tx_lookup_callback_(inv.hash);
                if (tx_opt.has_value()) {
                    // Send transaction
                    std::vector<uint8_t> tx_data = tx_opt->serialize();
                    Message tx_msg(MessageType::TX, tx_data);
                    send_message(from, tx_msg);
                } else {
                    // Transaction not found - send NOTFOUND
                    Message notfound_msg(MessageType::NOTFOUND, inv_data);
                    send_message(from, notfound_msg);
                }
            }
        }

        offset += 36;
    }
}

void Network::handle_block(const Message& msg, const PeerAddress& from) {
    // Deserialize block and call callback
    if (block_callback_ && msg.payload.size() > 0) {
        Block block = Block::deserialize(msg.payload);
        block_callback_(block, from);
    }
}

void Network::handle_tx(const Message& msg, const PeerAddress& from) {
    // Deserialize transaction and call callback
    if (tx_callback_ && msg.payload.size() > 0) {
        Transaction tx = Transaction::deserialize(msg.payload);
        tx_callback_(tx, from);
    }
}

void Network::discover_peers() {
    // Connect to seed nodes
    for (const auto& seed : seed_nodes_) {
        if (peer_count() < protocol::MAX_PEERS / 2) {
            connect_to_peer(seed);
        }
    }

    // DNS seed lookup
    const char* dns_seeds[] = {
        "seed.intcoin.org",
        "seed.intcoin.io",
        "dnsseed.intcoin.net"
    };

    for (const char* seed : dns_seeds) {
        struct addrinfo hints, *res;
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;

        if (getaddrinfo(seed, nullptr, &hints, &res) == 0) {
            struct addrinfo* p = res;
            while (p && peer_count() < protocol::MAX_PEERS / 2) {
                if (p->ai_family == AF_INET) {
                    struct sockaddr_in* addr = (struct sockaddr_in*)p->ai_addr;
                    PeerAddress peer_addr;
                    peer_addr.ip = inet_ntoa(addr->sin_addr);
                    peer_addr.port = is_testnet_ ? protocol::DEFAULT_PORT_TESTNET : protocol::DEFAULT_PORT;

                    connect_to_peer(peer_addr);
                }
                p = p->ai_next;
            }
            freeaddrinfo(res);
        }
    }
}

void Network::maintain_connections() {
    // Remove dead peers
    auto it = peers_.begin();
    while (it != peers_.end()) {
        if (!it->is_alive()) {
            if (it->socket_fd >= 0) {
                close(it->socket_fd);
            }
            it = peers_.erase(it);
        } else {
            ++it;
        }
    }

    // Ensure minimum connections
    if (peer_count() < protocol::MIN_PEERS) {
        discover_peers();
    }
}

Peer* Network::find_peer(const PeerAddress& addr) {
    for (auto& peer : peers_) {
        if (peer.address.ip == addr.ip && peer.address.port == addr.port) {
            return &peer;
        }
    }
    return nullptr;
}

} // namespace p2p
} // namespace intcoin
