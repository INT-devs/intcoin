// Copyright (c) 2025 INTcoin Core (Maddison Lane)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "intcoin/lightning_network.h"
#include "intcoin/serialization.h"
#include <cstring>
#include <algorithm>
#include <iostream>

namespace intcoin {
namespace lightning {

// ===== LightningPacket Implementation =====

std::vector<uint8_t> LightningPacket::serialize() const {
    std::vector<uint8_t> data;

    // Message type (4 bytes)
    uint32_t msg_type = static_cast<uint32_t>(type);
    for (int i = 0; i < 4; i++) {
        data.push_back(static_cast<uint8_t>((msg_type >> (i * 8)) & 0xFF));
    }

    // Sender public key (2592 bytes for Dilithium5)
    data.insert(data.end(), sender.begin(), sender.end());

    // Payload length (4 bytes)
    uint32_t payload_len = static_cast<uint32_t>(payload.size());
    for (int i = 0; i < 4; i++) {
        data.push_back(static_cast<uint8_t>((payload_len >> (i * 8)) & 0xFF));
    }

    // Payload
    data.insert(data.end(), payload.begin(), payload.end());

    return data;
}

std::optional<LightningPacket> LightningPacket::deserialize(const std::vector<uint8_t>& data) {
    if (data.size() < 2600) {  // 4 + 2592 + 4 minimum
        return std::nullopt;
    }

    LightningPacket packet;
    size_t offset = 0;

    // Message type (4 bytes)
    uint32_t msg_type = 0;
    for (int i = 0; i < 4; i++) {
        msg_type |= (static_cast<uint32_t>(data[offset++]) << (i * 8));
    }
    packet.type = static_cast<LightningMessageType>(msg_type);

    // Sender public key (2592 bytes)
    if (offset + 2592 > data.size()) return std::nullopt;
    std::copy(data.begin() + offset, data.begin() + offset + 2592, packet.sender.begin());
    offset += 2592;

    // Payload length (4 bytes)
    if (offset + 4 > data.size()) return std::nullopt;
    uint32_t payload_len = 0;
    for (int i = 0; i < 4; i++) {
        payload_len |= (static_cast<uint32_t>(data[offset++]) << (i * 8));
    }

    // Validate payload length
    if (payload_len > ln_protocol::MAX_LN_MESSAGE_SIZE) {
        return std::nullopt;
    }

    // Payload
    if (offset + payload_len > data.size()) return std::nullopt;
    packet.payload.assign(data.begin() + offset, data.begin() + offset + payload_len);

    return packet;
}

// ===== LightningNetworkManager Implementation =====

LightningNetworkManager::LightningNetworkManager(
    std::shared_ptr<LightningNode> ln_node,
    std::shared_ptr<p2p::Network> p2p_network)
    : lightning_node_(ln_node)
    , p2p_network_(p2p_network)
    , running_(false)
    , messages_sent_(0)
    , messages_received_(0)
{
}

LightningNetworkManager::~LightningNetworkManager() {
    stop();
}

bool LightningNetworkManager::start() {
    if (running_) {
        return true;
    }

    if (!lightning_node_ || !p2p_network_) {
        std::cerr << "Lightning node or P2P network not initialized" << std::endl;
        return false;
    }

    // Register P2P message handler
    // Note: In a real implementation, we'd need to extend P2P network to support
    // custom message handlers. For now, this is a placeholder for the architecture.

    running_ = true;
    std::cout << "Lightning Network Manager started" << std::endl;
    return true;
}

void LightningNetworkManager::stop() {
    if (!running_) {
        return;
    }

    running_ = false;

    // Disconnect all Lightning peers
    for (auto& [node_id, peer] : peers_) {
        if (peer.connected) {
            disconnect_peer(node_id);
        }
    }

    std::cout << "Lightning Network Manager stopped" << std::endl;
}

bool LightningNetworkManager::connect_to_peer(const DilithiumPubKey& node_id,
                                              const p2p::PeerAddress& addr) {
    if (!running_) {
        std::cerr << "Lightning Network Manager not running" << std::endl;
        return false;
    }

    // Check if already connected
    auto it = peers_.find(node_id);
    if (it != peers_.end() && it->second.connected) {
        return true;  // Already connected
    }

    // Create peer entry
    LightningPeer peer;
    peer.node_id = node_id;
    peer.address = addr;
    peer.connected = false;
    peer.last_seen = std::chrono::system_clock::now().time_since_epoch().count();

    // Establish P2P connection
    if (!p2p_network_->connect_to_peer(addr)) {
        std::cerr << "Failed to establish P2P connection to " << addr.to_string() << std::endl;
        return false;
    }

    peer.connected = true;
    peers_[node_id] = peer;

    // Send INIT message to establish Lightning connection
    messages::Message init_msg;
    init_msg.type = messages::MessageType::OPEN_CHANNEL;  // Placeholder
    // In real implementation, send proper INIT message with feature flags

    std::cout << "Connected to Lightning peer: " << addr.to_string() << std::endl;
    return true;
}

void LightningNetworkManager::disconnect_peer(const DilithiumPubKey& node_id) {
    auto it = peers_.find(node_id);
    if (it == peers_.end()) {
        return;
    }

    LightningPeer& peer = it->second;
    if (peer.connected) {
        p2p_network_->disconnect_peer(peer.address);
        peer.connected = false;
        std::cout << "Disconnected from Lightning peer: " << peer.address.to_string() << std::endl;
    }

    // Close all channels with this peer
    for (const auto& channel_id : peer.channels) {
        lightning_node_->close_channel(channel_id, true);  // Force close
    }

    peers_.erase(it);
}

std::vector<LightningPeer> LightningNetworkManager::get_connected_peers() const {
    std::vector<LightningPeer> connected;
    for (const auto& [node_id, peer] : peers_) {
        if (peer.connected) {
            connected.push_back(peer);
        }
    }
    return connected;
}

std::optional<LightningPeer> LightningNetworkManager::get_peer(const DilithiumPubKey& node_id) const {
    auto it = peers_.find(node_id);
    if (it != peers_.end()) {
        return it->second;
    }
    return std::nullopt;
}

size_t LightningNetworkManager::peer_count() const {
    size_t count = 0;
    for (const auto& [node_id, peer] : peers_) {
        if (peer.connected) {
            count++;
        }
    }
    return count;
}

void LightningNetworkManager::send_message(const DilithiumPubKey& node_id,
                                          const messages::Message& msg) {
    auto peer = find_peer(node_id);
    if (!peer || !peer->connected) {
        std::cerr << "Cannot send message: peer not connected" << std::endl;
        return;
    }

    // Create Lightning packet
    LightningPacket packet;
    packet.type = static_cast<LightningMessageType>(static_cast<uint32_t>(msg.type));
    packet.payload = msg.payload;
    packet.sender = lightning_node_->get_node_id();

    // Send through P2P network
    send_to_p2p(node_id, packet);
    messages_sent_++;
}

void LightningNetworkManager::broadcast_channel_announcement(const Hash256& channel_id) {
    // Create channel announcement message
    // In real implementation, this would construct proper BOLT #7 channel_announcement

    auto channel = lightning_node_->get_channel(channel_id);
    if (!channel) {
        std::cerr << "Channel not found for announcement: " << std::endl;
        return;
    }

    // Broadcast to all connected peers
    for (const auto& [node_id, peer] : peers_) {
        if (peer.connected) {
            // Send channel announcement
            messages::Message msg;
            msg.type = messages::MessageType::OPEN_CHANNEL;  // Placeholder
            send_message(node_id, msg);
        }
    }

    std::cout << "Broadcast channel announcement: " << std::endl;
}

void LightningNetworkManager::broadcast_node_announcement() {
    // Create node announcement with our identity
    // In real implementation, construct proper BOLT #7 node_announcement

    for (const auto& [node_id, peer] : peers_) {
        if (peer.connected) {
            messages::Message msg;
            msg.type = messages::MessageType::OPEN_CHANNEL;  // Placeholder
            send_message(node_id, msg);
        }
    }

    std::cout << "Broadcast node announcement" << std::endl;
}

bool LightningNetworkManager::open_channel_with_peer(const DilithiumPubKey& remote_node,
                                                     uint64_t capacity_sat,
                                                     uint64_t push_amount_sat) {
    // Verify peer is connected
    auto peer = find_peer(remote_node);
    if (!peer || !peer->connected) {
        std::cerr << "Cannot open channel: peer not connected" << std::endl;
        return false;
    }

    // Open channel through Lightning node
    auto channel_id = lightning_node_->open_channel(remote_node, capacity_sat, push_amount_sat);
    if (!channel_id) {
        std::cerr << "Failed to open channel" << std::endl;
        return false;
    }

    // Track channel-to-peer mapping
    channel_to_peer_[*channel_id] = remote_node;
    peer->channels.push_back(*channel_id);

    // Notify callback
    if (channel_open_callback_) {
        channel_open_callback_(*channel_id, remote_node);
    }

    std::cout << "Opened channel with peer, ID: "  << std::endl;
    return true;
}

bool LightningNetworkManager::close_channel_with_peer(const Hash256& channel_id, bool force) {
    // Find peer for this channel
    auto it = channel_to_peer_.find(channel_id);
    if (it == channel_to_peer_.end()) {
        std::cerr << "Channel not found: " << std::endl;
        return false;
    }

    DilithiumPubKey peer_node_id = it->second;

    // Close channel through Lightning node
    if (!lightning_node_->close_channel(channel_id, force)) {
        std::cerr << "Failed to close channel" << std::endl;
        return false;
    }

    // Update peer tracking
    auto peer = find_peer(peer_node_id);
    if (peer) {
        auto& channels = peer->channels;
        channels.erase(std::remove(channels.begin(), channels.end(), channel_id), channels.end());
    }

    channel_to_peer_.erase(it);

    std::cout << "Closed channel: " << std::endl;
    return true;
}

bool LightningNetworkManager::send_payment_through_network(
    const DilithiumPubKey& destination,
    uint64_t amount_sat,
    const Hash256& payment_hash) {

    // Find route to destination
    auto route = lightning_node_->find_route(destination, amount_sat);
    if (route.empty()) {
        std::cerr << "No route found to destination" << std::endl;
        return false;
    }

    // Send payment through Lightning node
    if (!lightning_node_->send_payment(amount_sat, payment_hash, route)) {
        std::cerr << "Failed to send payment" << std::endl;
        return false;
    }

    std::cout << "Payment sent: " << amount_sat << " satoshis" << std::endl;
    return true;
}

void LightningNetworkManager::sync_network_graph() {
    // Request network graph from all peers
    for (const auto& [node_id, peer] : peers_) {
        if (peer.connected) {
            // Send gossip query messages
            // In real implementation, send BOLT #7 gossip queries
        }
    }
}

void LightningNetworkManager::request_channel_announcements() {
    // Request channel announcements from peers
    sync_network_graph();
}

LightningNetworkManager::NetworkStats LightningNetworkManager::get_stats() const {
    NetworkStats stats;
    stats.connected_peers = peer_count();
    stats.announced_channels = node_directory_.size();
    stats.announced_nodes = node_directory_.size();
    stats.pending_htlcs = 0;
    stats.total_network_capacity = 0;
    stats.messages_sent = messages_sent_;
    stats.messages_received = messages_received_;

    // Calculate stats from Lightning node
    auto ln_stats = lightning_node_->get_stats();
    stats.pending_htlcs = ln_stats.active_channels;  // Approximation
    stats.total_network_capacity = ln_stats.total_capacity_sat;

    return stats;
}

// ===== Internal Message Handlers =====

void LightningNetworkManager::handle_init(const LightningPacket& packet) {
    // Process INIT message
    // Extract feature flags and update peer info
    auto peer = find_peer(packet.sender);
    if (peer) {
        peer->features_announced = true;
        update_peer_last_seen(packet.sender);
    }
    messages_received_++;
}

void LightningNetworkManager::handle_error(const LightningPacket& packet) {
    std::cerr << "Received ERROR message from peer" << std::endl;
    messages_received_++;
}

void LightningNetworkManager::handle_ping(const LightningPacket& packet) {
    // Respond with PONG
    auto peer = find_peer(packet.sender);
    if (peer && peer->connected) {
        LightningPacket pong;
        pong.type = LightningMessageType::PONG_LIGHTNING;
        pong.sender = lightning_node_->get_node_id();
        send_to_p2p(packet.sender, pong);
    }
    messages_received_++;
}

void LightningNetworkManager::handle_pong(const LightningPacket& packet) {
    // Update peer last seen
    update_peer_last_seen(packet.sender);
    messages_received_++;
}

void LightningNetworkManager::handle_open_channel(const LightningPacket& packet) {
    // Forward to Lightning node for processing
    messages::Message msg;
    msg.type = messages::MessageType::OPEN_CHANNEL;
    msg.payload = packet.payload;

    // Deserialize and process
    // auto open_channel = messages::OpenChannel::from_message(msg);

    if (message_received_callback_) {
        message_received_callback_(packet.sender, msg);
    }
    messages_received_++;
}

void LightningNetworkManager::handle_accept_channel(const LightningPacket& packet) {
    messages::Message msg;
    msg.type = messages::MessageType::ACCEPT_CHANNEL;
    msg.payload = packet.payload;

    if (message_received_callback_) {
        message_received_callback_(packet.sender, msg);
    }
    messages_received_++;
}

void LightningNetworkManager::handle_funding_created(const LightningPacket& packet) {
    messages::Message msg;
    msg.type = messages::MessageType::FUNDING_CREATED;
    msg.payload = packet.payload;

    if (message_received_callback_) {
        message_received_callback_(packet.sender, msg);
    }
    messages_received_++;
}

void LightningNetworkManager::handle_funding_signed(const LightningPacket& packet) {
    messages::Message msg;
    msg.type = messages::MessageType::FUNDING_SIGNED;
    msg.payload = packet.payload;

    if (message_received_callback_) {
        message_received_callback_(packet.sender, msg);
    }
    messages_received_++;
}

void LightningNetworkManager::handle_update_add_htlc(const LightningPacket& packet) {
    messages::Message msg;
    msg.type = messages::MessageType::UPDATE_ADD_HTLC;
    msg.payload = packet.payload;

    if (message_received_callback_) {
        message_received_callback_(packet.sender, msg);
    }
    messages_received_++;
}

void LightningNetworkManager::handle_update_fulfill_htlc(const LightningPacket& packet) {
    messages::Message msg;
    msg.type = messages::MessageType::UPDATE_FULFILL_HTLC;
    msg.payload = packet.payload;

    if (message_received_callback_) {
        message_received_callback_(packet.sender, msg);
    }

    // Extract payment info and call callback
    auto fulfill = messages::UpdateFulfillHTLC::from_message(msg);
    if (payment_received_callback_) {
        // In real implementation, extract amount from HTLC
        payment_received_callback_(fulfill.channel_id, 0);
    }
    messages_received_++;
}

void LightningNetworkManager::handle_update_fail_htlc(const LightningPacket& packet) {
    messages::Message msg;
    msg.type = messages::MessageType::UPDATE_FAIL_HTLC;
    msg.payload = packet.payload;

    if (message_received_callback_) {
        message_received_callback_(packet.sender, msg);
    }
    messages_received_++;
}

void LightningNetworkManager::handle_commitment_signed(const LightningPacket& packet) {
    messages::Message msg;
    msg.type = messages::MessageType::COMMITMENT_SIGNED;
    msg.payload = packet.payload;

    if (message_received_callback_) {
        message_received_callback_(packet.sender, msg);
    }
    messages_received_++;
}

void LightningNetworkManager::handle_revoke_and_ack(const LightningPacket& packet) {
    messages::Message msg;
    msg.type = messages::MessageType::REVOKE_AND_ACK;
    msg.payload = packet.payload;

    if (message_received_callback_) {
        message_received_callback_(packet.sender, msg);
    }
    messages_received_++;
}

void LightningNetworkManager::handle_shutdown(const LightningPacket& packet) {
    messages::Message msg;
    msg.type = messages::MessageType::SHUTDOWN;
    msg.payload = packet.payload;

    if (message_received_callback_) {
        message_received_callback_(packet.sender, msg);
    }
    messages_received_++;
}

void LightningNetworkManager::handle_closing_signed(const LightningPacket& packet) {
    messages::Message msg;
    msg.type = messages::MessageType::CLOSING_SIGNED;
    msg.payload = packet.payload;

    if (message_received_callback_) {
        message_received_callback_(packet.sender, msg);
    }
    messages_received_++;
}

void LightningNetworkManager::handle_channel_announcement(const LightningPacket& packet) {
    // Update network graph
    // In real implementation, parse and validate channel announcement
    messages_received_++;
}

void LightningNetworkManager::handle_node_announcement(const LightningPacket& packet) {
    // Update node directory
    // In real implementation, parse and validate node announcement
    messages_received_++;
}

void LightningNetworkManager::handle_channel_update(const LightningPacket& packet) {
    // Update channel info in network graph
    // In real implementation, parse and validate channel update
    messages_received_++;
}

// ===== P2P Integration =====

void LightningNetworkManager::on_p2p_message_received(const p2p::Message& msg,
                                                      const p2p::PeerAddress& from) {
    // Deserialize Lightning packet from P2P message
    auto packet = LightningPacket::deserialize(msg.payload);
    if (!packet) {
        std::cerr << "Failed to deserialize Lightning packet" << std::endl;
        return;
    }

    // Route to appropriate handler based on message type
    switch (packet->type) {
        case LightningMessageType::INIT:
            handle_init(*packet);
            break;
        case LightningMessageType::ERROR_MSG:
            handle_error(*packet);
            break;
        case LightningMessageType::PING_LIGHTNING:
            handle_ping(*packet);
            break;
        case LightningMessageType::PONG_LIGHTNING:
            handle_pong(*packet);
            break;
        case LightningMessageType::OPEN_CHANNEL:
            handle_open_channel(*packet);
            break;
        case LightningMessageType::ACCEPT_CHANNEL:
            handle_accept_channel(*packet);
            break;
        case LightningMessageType::FUNDING_CREATED:
            handle_funding_created(*packet);
            break;
        case LightningMessageType::FUNDING_SIGNED:
            handle_funding_signed(*packet);
            break;
        case LightningMessageType::UPDATE_ADD_HTLC:
            handle_update_add_htlc(*packet);
            break;
        case LightningMessageType::UPDATE_FULFILL_HTLC:
            handle_update_fulfill_htlc(*packet);
            break;
        case LightningMessageType::UPDATE_FAIL_HTLC:
            handle_update_fail_htlc(*packet);
            break;
        case LightningMessageType::COMMITMENT_SIGNED:
            handle_commitment_signed(*packet);
            break;
        case LightningMessageType::REVOKE_AND_ACK:
            handle_revoke_and_ack(*packet);
            break;
        case LightningMessageType::SHUTDOWN:
            handle_shutdown(*packet);
            break;
        case LightningMessageType::CLOSING_SIGNED:
            handle_closing_signed(*packet);
            break;
        case LightningMessageType::CHANNEL_ANNOUNCEMENT:
            handle_channel_announcement(*packet);
            break;
        case LightningMessageType::NODE_ANNOUNCEMENT:
            handle_node_announcement(*packet);
            break;
        case LightningMessageType::CHANNEL_UPDATE:
            handle_channel_update(*packet);
            break;
        default:
            std::cerr << "Unknown Lightning message type: "
                      << static_cast<uint32_t>(packet->type) << std::endl;
            break;
    }
}

void LightningNetworkManager::send_to_p2p(const DilithiumPubKey& node_id,
                                          const LightningPacket& packet) {
    auto peer = find_peer(node_id);
    if (!peer || !peer->connected) {
        std::cerr << "Cannot send to P2P: peer not connected" << std::endl;
        return;
    }

    // Serialize Lightning packet
    auto data = packet.serialize();

    // Wrap in P2P message
    p2p::Message msg(p2p::MessageType::TX, data);  // Use TX as placeholder

    // Send through P2P network
    p2p_network_->send_message(peer->address, msg);
}

// ===== Helper Functions =====

LightningPeer* LightningNetworkManager::find_peer(const DilithiumPubKey& node_id) {
    auto it = peers_.find(node_id);
    if (it != peers_.end()) {
        return &it->second;
    }
    return nullptr;
}

void LightningNetworkManager::process_message_queue(const DilithiumPubKey& node_id) {
    auto it = pending_messages_.find(node_id);
    if (it == pending_messages_.end() || it->second.empty()) {
        return;
    }

    // Process all pending messages for this peer
    for (const auto& packet : it->second) {
        send_to_p2p(node_id, packet);
    }

    it->second.clear();
}

void LightningNetworkManager::update_peer_last_seen(const DilithiumPubKey& node_id) {
    auto peer = find_peer(node_id);
    if (peer) {
        peer->last_seen = std::chrono::system_clock::now().time_since_epoch().count();
    }
}

std::optional<p2p::PeerAddress> LightningNetworkManager::resolve_node_address(
    const DilithiumPubKey& node_id) {

    // Check node directory
    auto it = node_directory_.find(node_id);
    if (it != node_directory_.end()) {
        return it->second.last_known_address;
    }

    // Check connected peers
    auto peer = find_peer(node_id);
    if (peer) {
        return peer->address;
    }

    return std::nullopt;
}

} // namespace lightning
} // namespace intcoin
