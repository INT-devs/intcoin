// Copyright (c) 2025 INTcoin Core (Maddison Lane)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "intcoin/p2p.h"
#include "intcoin/crypto.h"
#include <cstring>
#include <algorithm>

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
{
}

Network::~Network() {
    stop();
}

bool Network::start() {
    if (running_) return false;

    running_ = true;

    // TODO: Start listening socket
    // TODO: Start peer discovery thread
    // TODO: Start connection maintenance thread

    return true;
}

void Network::stop() {
    if (!running_) return;

    running_ = false;

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

    // TODO: Create actual TCP connection
    Peer peer(addr);
    peer.connected = true;
    peer.inbound = false;
    peer.update_last_seen();

    peers_.push_back(peer);

    // Send version message
    // TODO: Implement version handshake

    return true;
}

void Network::disconnect_peer(const PeerAddress& addr) {
    auto it = std::remove_if(peers_.begin(), peers_.end(),
        [&addr](const Peer& p) {
            return p.address.ip == addr.ip && p.address.port == addr.port;
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
    // TODO: Implement actual TCP send
    (void)addr;
    (void)msg;
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
    (void)msg;
    (void)from;
    // TODO: Parse version message and send verack
}

void Network::handle_inv(const Message& msg, const PeerAddress& from) {
    (void)msg;
    (void)from;
    // TODO: Process inventory and request data
}

void Network::handle_getdata(const Message& msg, const PeerAddress& from) {
    (void)msg;
    (void)from;
    // TODO: Send requested data
}

void Network::handle_block(const Message& msg, const PeerAddress& from) {
    // TODO: Deserialize block and call callback
    if (block_callback_) {
        // block_callback_(block, from);
    }
}

void Network::handle_tx(const Message& msg, const PeerAddress& from) {
    // TODO: Deserialize transaction and call callback
    if (tx_callback_) {
        // tx_callback_(tx, from);
    }
}

void Network::discover_peers() {
    // TODO: DNS seed lookup
    // TODO: Connect to seed nodes
}

void Network::maintain_connections() {
    // TODO: Remove dead peers
    // TODO: Ensure minimum connections
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
