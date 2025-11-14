// Copyright (c) 2025 INTcoin Core (Maddison Lane)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "intcoin/lightning.h"
#include <algorithm>
#include <cstring>
#include <limits>

namespace intcoin {
namespace lightning {

// HTLC implementation

std::vector<uint8_t> HTLC::serialize() const {
    std::vector<uint8_t> buffer;

    // ID (8 bytes)
    for (int i = 0; i < 8; i++) {
        buffer.push_back(static_cast<uint8_t>((id >> (i * 8)) & 0xFF));
    }

    // Amount (8 bytes)
    for (int i = 0; i < 8; i++) {
        buffer.push_back(static_cast<uint8_t>((amount_sat >> (i * 8)) & 0xFF));
    }

    // Payment hash (32 bytes)
    buffer.insert(buffer.end(), payment_hash.begin(), payment_hash.end());

    // CLTV expiry (4 bytes)
    buffer.push_back(static_cast<uint8_t>(cltv_expiry & 0xFF));
    buffer.push_back(static_cast<uint8_t>((cltv_expiry >> 8) & 0xFF));
    buffer.push_back(static_cast<uint8_t>((cltv_expiry >> 16) & 0xFF));
    buffer.push_back(static_cast<uint8_t>((cltv_expiry >> 24) & 0xFF));

    // Direction (1 byte)
    buffer.push_back(direction == HTLCDirection::OFFERED ? 0 : 1);

    // Onion routing length (4 bytes) and data
    uint32_t onion_len = static_cast<uint32_t>(onion_routing.size());
    buffer.push_back(static_cast<uint8_t>(onion_len & 0xFF));
    buffer.push_back(static_cast<uint8_t>((onion_len >> 8) & 0xFF));
    buffer.push_back(static_cast<uint8_t>((onion_len >> 16) & 0xFF));
    buffer.push_back(static_cast<uint8_t>((onion_len >> 24) & 0xFF));
    buffer.insert(buffer.end(), onion_routing.begin(), onion_routing.end());

    return buffer;
}

HTLC HTLC::deserialize(const std::vector<uint8_t>& data) {
    HTLC htlc;
    if (data.size() < 53) return htlc;  // Minimum size

    size_t offset = 0;

    // ID (8 bytes)
    htlc.id = 0;
    for (int i = 0; i < 8; i++) {
        htlc.id |= (static_cast<uint64_t>(data[offset + i]) << (i * 8));
    }
    offset += 8;

    // Amount (8 bytes)
    htlc.amount_sat = 0;
    for (int i = 0; i < 8; i++) {
        htlc.amount_sat |= (static_cast<uint64_t>(data[offset + i]) << (i * 8));
    }
    offset += 8;

    // Payment hash (32 bytes)
    std::copy(data.begin() + offset, data.begin() + offset + 32, htlc.payment_hash.begin());
    offset += 32;

    // CLTV expiry (4 bytes)
    htlc.cltv_expiry = static_cast<uint32_t>(data[offset]) |
                       (static_cast<uint32_t>(data[offset + 1]) << 8) |
                       (static_cast<uint32_t>(data[offset + 2]) << 16) |
                       (static_cast<uint32_t>(data[offset + 3]) << 24);
    offset += 4;

    // Direction (1 byte)
    htlc.direction = (data[offset] == 0) ? HTLCDirection::OFFERED : HTLCDirection::RECEIVED;
    offset += 1;

    // Onion routing
    if (data.size() < offset + 4) return htlc;
    uint32_t onion_len = static_cast<uint32_t>(data[offset]) |
                         (static_cast<uint32_t>(data[offset + 1]) << 8) |
                         (static_cast<uint32_t>(data[offset + 2]) << 16) |
                         (static_cast<uint32_t>(data[offset + 3]) << 24);
    offset += 4;

    if (data.size() >= offset + onion_len) {
        htlc.onion_routing.assign(data.begin() + offset, data.begin() + offset + onion_len);
    }

    return htlc;
}

// CommitmentTransaction implementation

Hash256 CommitmentTransaction::get_hash() const {
    std::vector<uint8_t> data = serialize();
    return crypto::SHA3_256::hash(data.data(), data.size());
}

std::vector<uint8_t> CommitmentTransaction::serialize() const {
    std::vector<uint8_t> buffer;

    // Commitment number (8 bytes)
    for (int i = 0; i < 8; i++) {
        buffer.push_back(static_cast<uint8_t>((commitment_number >> (i * 8)) & 0xFF));
    }

    // Local balance (8 bytes)
    for (int i = 0; i < 8; i++) {
        buffer.push_back(static_cast<uint8_t>((to_local_sat >> (i * 8)) & 0xFF));
    }

    // Remote balance (8 bytes)
    for (int i = 0; i < 8; i++) {
        buffer.push_back(static_cast<uint8_t>((to_remote_sat >> (i * 8)) & 0xFF));
    }

    // HTLC count (4 bytes)
    uint32_t htlc_count = static_cast<uint32_t>(htlcs.size());
    buffer.push_back(static_cast<uint8_t>(htlc_count & 0xFF));
    buffer.push_back(static_cast<uint8_t>((htlc_count >> 8) & 0xFF));
    buffer.push_back(static_cast<uint8_t>((htlc_count >> 16) & 0xFF));
    buffer.push_back(static_cast<uint8_t>((htlc_count >> 24) & 0xFF));

    // HTLCs
    for (const auto& htlc : htlcs) {
        std::vector<uint8_t> htlc_data = htlc.serialize();
        buffer.insert(buffer.end(), htlc_data.begin(), htlc_data.end());
    }

    // Revocation pubkey (2592 bytes - Dilithium5)
    buffer.insert(buffer.end(), revocation_pubkey.begin(), revocation_pubkey.end());

    // Revocation hash (32 bytes)
    buffer.insert(buffer.end(), revocation_hash.begin(), revocation_hash.end());

    return buffer;
}

CommitmentTransaction CommitmentTransaction::deserialize(const std::vector<uint8_t>& data) {
    CommitmentTransaction ct;
    if (data.size() < 28) return ct;  // Minimum size

    size_t offset = 0;

    // Commitment number (8 bytes)
    ct.commitment_number = 0;
    for (int i = 0; i < 8; i++) {
        ct.commitment_number |= (static_cast<uint64_t>(data[offset + i]) << (i * 8));
    }
    offset += 8;

    // Local balance (8 bytes)
    ct.to_local_sat = 0;
    for (int i = 0; i < 8; i++) {
        ct.to_local_sat |= (static_cast<uint64_t>(data[offset + i]) << (i * 8));
    }
    offset += 8;

    // Remote balance (8 bytes)
    ct.to_remote_sat = 0;
    for (int i = 0; i < 8; i++) {
        ct.to_remote_sat |= (static_cast<uint64_t>(data[offset + i]) << (i * 8));
    }
    offset += 8;

    // HTLC count (4 bytes)
    // uint32_t htlc_count = static_cast<uint32_t>(data[offset]) |
    //                       (static_cast<uint32_t>(data[offset + 1]) << 8) |
    //                       (static_cast<uint32_t>(data[offset + 2]) << 16) |
    //                       (static_cast<uint32_t>(data[offset + 3]) << 24);
    offset += 4;

    // HTLCs (simplified - would need proper length tracking)
    // Skipping detailed deserialization for now

    return ct;
}

// Channel implementation

Channel::Channel()
    : channel_id{}
    , funding_txid{}
    , funding_output_index(0)
    , capacity_sat(0)
    , local_balance_sat(0)
    , remote_balance_sat(0)
    , dust_limit_sat(546)
    , max_htlc_value_in_flight_sat(1000000000)  // 10 INT
    , max_accepted_htlcs(30)
    , state(ChannelState::OPENING)
    , commitment_number(0)
    , next_htlc_id(0)
    , to_self_delay(144)  // ~1 day
    , channel_reserve_sat(1000)
{
}

bool Channel::open(const DilithiumPubKey& remote_key, uint64_t capacity) {
    if (state != ChannelState::OPENING) {
        return false;
    }

    remote_pubkey = remote_key;
    capacity_sat = capacity;
    local_balance_sat = capacity;
    remote_balance_sat = 0;

    state = ChannelState::OPEN;
    return true;
}

bool Channel::close_cooperative() {
    if (state != ChannelState::OPEN) {
        return false;
    }

    // Check no pending HTLCs
    if (!pending_htlcs.empty()) {
        return false;
    }

    state = ChannelState::CLOSING;
    return true;
}

bool Channel::close_unilateral() {
    if (state != ChannelState::OPEN && state != ChannelState::CLOSING) {
        return false;
    }

    state = ChannelState::FORCE_CLOSING;
    return true;
}

bool Channel::can_send(uint64_t amount_sat) const {
    return available_to_send() >= amount_sat;
}

bool Channel::can_receive(uint64_t amount_sat) const {
    return available_to_receive() >= amount_sat;
}

uint64_t Channel::available_to_send() const {
    if (local_balance_sat < channel_reserve_sat) {
        return 0;
    }
    return local_balance_sat - channel_reserve_sat;
}

uint64_t Channel::available_to_receive() const {
    if (remote_balance_sat < channel_reserve_sat) {
        return 0;
    }
    return capacity_sat - local_balance_sat - channel_reserve_sat;
}

bool Channel::add_htlc(uint64_t amount_sat, const Hash256& payment_hash,
                      uint32_t cltv_expiry, const std::vector<uint8_t>& onion) {
    if (state != ChannelState::OPEN) {
        return false;
    }

    if (!can_send(amount_sat)) {
        return false;
    }

    if (pending_htlcs.size() >= max_accepted_htlcs) {
        return false;
    }

    HTLC htlc;
    htlc.id = next_htlc_id++;
    htlc.amount_sat = amount_sat;
    htlc.payment_hash = payment_hash;
    htlc.cltv_expiry = cltv_expiry;
    htlc.direction = HTLCDirection::OFFERED;
    htlc.onion_routing = onion;

    pending_htlcs[htlc.id] = htlc;

    return true;
}

bool Channel::settle_htlc(uint64_t htlc_id, const std::vector<uint8_t>& preimage) {
    auto it = pending_htlcs.find(htlc_id);
    if (it == pending_htlcs.end()) {
        return false;
    }

    // Verify preimage
    Hash256 hash = crypto::SHA3_256::hash(preimage.data(), preimage.size());
    if (hash != it->second.payment_hash) {
        return false;
    }

    uint64_t amount = it->second.amount_sat;

    if (it->second.direction == HTLCDirection::OFFERED) {
        // We offered, they're settling - they get the funds
        local_balance_sat -= amount;
        remote_balance_sat += amount;
    } else {
        // They offered, we're settling - we get the funds
        local_balance_sat += amount;
        remote_balance_sat -= amount;
    }

    pending_htlcs.erase(it);
    return true;
}

bool Channel::fail_htlc(uint64_t htlc_id) {
    auto it = pending_htlcs.find(htlc_id);
    if (it == pending_htlcs.end()) {
        return false;
    }

    // HTLC failed, funds return to sender
    pending_htlcs.erase(it);
    return true;
}

bool Channel::create_new_commitment() {
    if (state != ChannelState::OPEN) {
        return false;
    }

    auto new_commitment = std::make_shared<CommitmentTransaction>();
    new_commitment->commitment_number = commitment_number + 1;
    new_commitment->to_local_sat = local_balance_sat;
    new_commitment->to_remote_sat = remote_balance_sat;

    // Copy pending HTLCs
    for (const auto& [id, htlc] : pending_htlcs) {
        new_commitment->htlcs.push_back(htlc);
    }

    latest_commitment = new_commitment;
    commitment_number++;

    return true;
}

bool Channel::sign_commitment(const crypto::DilithiumKeyPair& keypair) {
    if (!latest_commitment) {
        return false;
    }

    // Sign the commitment transaction hash
    Hash256 commitment_hash = latest_commitment->get_hash();
    latest_commitment->local_sig = crypto::Dilithium::sign(
        std::vector<uint8_t>(commitment_hash.begin(), commitment_hash.end()),
        keypair
    );

    return true;
}

bool Channel::verify_remote_signature(const DilithiumSignature& sig) {
    if (!latest_commitment) {
        return false;
    }

    Hash256 commitment_hash = latest_commitment->get_hash();
    DilithiumPubKey remote_key = remote_pubkey;

    return crypto::Dilithium::verify(
        std::vector<uint8_t>(commitment_hash.begin(), commitment_hash.end()),
        sig,
        remote_key
    );
}

bool Channel::revoke_previous_commitment() {
    if (!latest_commitment) {
        return false;
    }

    // Store revoked commitment for penalty enforcement
    if (latest_commitment->commitment_number > 0) {
        revoked_commitments.push_back(latest_commitment);
    }

    return true;
}

std::vector<uint8_t> Channel::serialize() const {
    std::vector<uint8_t> buffer;

    // Channel ID (32 bytes)
    buffer.insert(buffer.end(), channel_id.begin(), channel_id.end());

    // Funding TXID (32 bytes)
    buffer.insert(buffer.end(), funding_txid.begin(), funding_txid.end());

    // Funding output index (4 bytes)
    buffer.push_back(static_cast<uint8_t>(funding_output_index & 0xFF));
    buffer.push_back(static_cast<uint8_t>((funding_output_index >> 8) & 0xFF));
    buffer.push_back(static_cast<uint8_t>((funding_output_index >> 16) & 0xFF));
    buffer.push_back(static_cast<uint8_t>((funding_output_index >> 24) & 0xFF));

    // Public keys (2592 bytes each - Dilithium5)
    buffer.insert(buffer.end(), local_pubkey.begin(), local_pubkey.end());
    buffer.insert(buffer.end(), remote_pubkey.begin(), remote_pubkey.end());

    // Balances and parameters
    for (int i = 0; i < 8; i++) {
        buffer.push_back(static_cast<uint8_t>((capacity_sat >> (i * 8)) & 0xFF));
    }
    for (int i = 0; i < 8; i++) {
        buffer.push_back(static_cast<uint8_t>((local_balance_sat >> (i * 8)) & 0xFF));
    }
    for (int i = 0; i < 8; i++) {
        buffer.push_back(static_cast<uint8_t>((remote_balance_sat >> (i * 8)) & 0xFF));
    }

    // State (1 byte)
    buffer.push_back(static_cast<uint8_t>(state));

    return buffer;
}

Channel Channel::deserialize(const std::vector<uint8_t>& data) {
    Channel channel;
    if (data.size() < 5248) return channel;  // Minimum size

    size_t offset = 0;

    // Channel ID (32 bytes)
    std::copy(data.begin() + offset, data.begin() + offset + 32, channel.channel_id.begin());
    offset += 32;

    // Funding TXID (32 bytes)
    std::copy(data.begin() + offset, data.begin() + offset + 32, channel.funding_txid.begin());
    offset += 32;

    // Funding output index (4 bytes)
    channel.funding_output_index = static_cast<uint32_t>(data[offset]) |
                                   (static_cast<uint32_t>(data[offset + 1]) << 8) |
                                   (static_cast<uint32_t>(data[offset + 2]) << 16) |
                                   (static_cast<uint32_t>(data[offset + 3]) << 24);
    offset += 4;

    // Public keys (2592 bytes each)
    std::copy(data.begin() + offset, data.begin() + offset + 2592, channel.local_pubkey.begin());
    offset += 2592;
    std::copy(data.begin() + offset, data.begin() + offset + 2592, channel.remote_pubkey.begin());
    offset += 2592;

    // Balances
    channel.capacity_sat = 0;
    for (int i = 0; i < 8; i++) {
        channel.capacity_sat |= (static_cast<uint64_t>(data[offset + i]) << (i * 8));
    }
    offset += 8;

    channel.local_balance_sat = 0;
    for (int i = 0; i < 8; i++) {
        channel.local_balance_sat |= (static_cast<uint64_t>(data[offset + i]) << (i * 8));
    }
    offset += 8;

    channel.remote_balance_sat = 0;
    for (int i = 0; i < 8; i++) {
        channel.remote_balance_sat |= (static_cast<uint64_t>(data[offset + i]) << (i * 8));
    }
    offset += 8;

    // State (1 byte)
    channel.state = static_cast<ChannelState>(data[offset]);

    return channel;
}

// LightningNode implementation

LightningNode::LightningNode(const crypto::DilithiumKeyPair& keypair)
    : keypair_(keypair)
    , successful_payments_(0)
    , failed_payments_(0)
    , total_fees_earned_sat_(0)
{
}

std::optional<Hash256> LightningNode::open_channel(
    const DilithiumPubKey& remote_pubkey,
    uint64_t capacity_sat,
    uint64_t push_amount_sat)
{
    // Create new channel
    auto channel = std::make_shared<Channel>();
    channel->local_pubkey = keypair_.public_key;

    if (!channel->open(remote_pubkey, capacity_sat)) {
        return std::nullopt;
    }

    // Handle push amount
    if (push_amount_sat > 0) {
        if (push_amount_sat > capacity_sat) {
            return std::nullopt;
        }
        channel->local_balance_sat -= push_amount_sat;
        channel->remote_balance_sat += push_amount_sat;
    }

    // Generate channel ID (simplified - would use funding tx in real implementation)
    Hash256 channel_id;
    auto now = std::chrono::system_clock::now().time_since_epoch().count();
    std::vector<uint8_t> id_data;
    for (int i = 0; i < 8; i++) {
        id_data.push_back(static_cast<uint8_t>((now >> (i * 8)) & 0xFF));
    }
    channel_id = crypto::SHA3_256::hash(id_data.data(), id_data.size());

    channel->channel_id = channel_id;
    channels_[channel_id] = channel;

    return channel_id;
}

bool LightningNode::close_channel(const Hash256& channel_id, bool force) {
    auto it = channels_.find(channel_id);
    if (it == channels_.end()) {
        return false;
    }

    if (force) {
        return it->second->close_unilateral();
    } else {
        return it->second->close_cooperative();
    }
}

std::shared_ptr<Channel> LightningNode::get_channel(const Hash256& channel_id) {
    auto it = channels_.find(channel_id);
    if (it == channels_.end()) {
        return nullptr;
    }
    return it->second;
}

std::vector<std::shared_ptr<Channel>> LightningNode::get_all_channels() const {
    std::vector<std::shared_ptr<Channel>> result;
    for (const auto& [id, channel] : channels_) {
        result.push_back(channel);
    }
    return result;
}

size_t LightningNode::active_channel_count() const {
    size_t count = 0;
    for (const auto& [id, channel] : channels_) {
        if (channel->is_open()) {
            count++;
        }
    }
    return count;
}

LightningNode::Invoice LightningNode::create_invoice(
    uint64_t amount_sat,
    const std::string& description)
{
    Invoice invoice;

    // Generate random preimage
    std::vector<uint8_t> preimage(32);
    for (size_t i = 0; i < 32; i++) {
        preimage[i] = static_cast<uint8_t>(rand() % 256);
    }

    invoice.preimage = preimage;
    invoice.payment_hash = crypto::SHA3_256::hash(preimage.data(), preimage.size());
    invoice.amount_sat = amount_sat;
    invoice.description = description;
    invoice.expiry_time = static_cast<uint32_t>(
        std::chrono::system_clock::now().time_since_epoch().count() / 1000000000 + 3600
    );

    // Store invoice
    invoices_[invoice.payment_hash] = invoice;

    // Generate encoded invoice (simplified)
    invoice.encoded_invoice = "lnint1" + std::to_string(amount_sat);

    return invoice;
}

LightningNode::NodeStats LightningNode::get_stats() const {
    NodeStats stats;
    stats.total_channels = channels_.size();
    stats.active_channels = active_channel_count();
    stats.total_capacity_sat = 0;
    stats.total_local_balance_sat = 0;
    stats.total_remote_balance_sat = 0;
    stats.successful_payments = successful_payments_;
    stats.failed_payments = failed_payments_;
    stats.total_fees_earned_sat = total_fees_earned_sat_;

    for (const auto& [id, channel] : channels_) {
        if (channel->is_open()) {
            stats.total_capacity_sat += channel->capacity_sat;
            stats.total_local_balance_sat += channel->local_balance_sat;
            stats.total_remote_balance_sat += channel->remote_balance_sat;
        }
    }

    return stats;
}

Hash256 LightningNode::generate_channel_id(const Hash256& funding_txid, uint32_t output_index) {
    std::vector<uint8_t> data;
    data.insert(data.end(), funding_txid.begin(), funding_txid.end());
    for (int i = 0; i < 4; i++) {
        data.push_back(static_cast<uint8_t>((output_index >> (i * 8)) & 0xFF));
    }
    return crypto::SHA3_256::hash(data.data(), data.size());
}

// Network graph and routing implementation

void LightningNode::add_channel_to_graph(const ChannelInfo& info) {
    network_graph_[info.channel_id] = info;
}

void LightningNode::remove_channel_from_graph(const Hash256& channel_id) {
    network_graph_.erase(channel_id);
}

std::vector<DilithiumPubKey> LightningNode::find_route(
    const DilithiumPubKey& destination,
    uint64_t amount_sat)
{
    // Dijkstra's shortest path algorithm for Lightning routing

    // Distance map: node -> (distance, previous_node)
    std::map<DilithiumPubKey, std::pair<uint64_t, DilithiumPubKey>> distances;

    // Visited nodes tracked in the distances map (if finalized, distance is set)
    std::vector<DilithiumPubKey> visited_nodes;

    // Priority queue: (distance, node)
    std::vector<std::pair<uint64_t, DilithiumPubKey>> queue;

    // Initialize: start from our node
    distances[keypair_.public_key] = {0, keypair_.public_key};
    queue.push_back({0, keypair_.public_key});

    while (!queue.empty()) {
        // Find node with minimum distance
        auto min_it = std::min_element(queue.begin(), queue.end(),
            [](const auto& a, const auto& b) { return a.first < b.first; });

        auto [current_distance, current_node] = *min_it;
        queue.erase(min_it);

        // Skip if already visited
        auto is_visited = std::find_if(visited_nodes.begin(), visited_nodes.end(),
            [&current_node](const DilithiumPubKey& n) {
                return std::equal(n.begin(), n.end(), current_node.begin());
            }) != visited_nodes.end();

        if (is_visited) {
            continue;
        }
        visited_nodes.push_back(current_node);

        // Found destination
        if (current_node == destination) {
            break;
        }

        // Explore neighbors
        for (const auto& [channel_id, channel_info] : network_graph_) {
            if (!channel_info.enabled) {
                continue;
            }

            // Check if this channel connects to current node
            DilithiumPubKey neighbor;
            bool is_neighbor = false;

            if (channel_info.node1 == current_node) {
                neighbor = channel_info.node2;
                is_neighbor = true;
            } else if (channel_info.node2 == current_node) {
                neighbor = channel_info.node1;
                is_neighbor = true;
            }

            // Check if neighbor is visited
            auto neighbor_visited = std::find_if(visited_nodes.begin(), visited_nodes.end(),
                [&neighbor](const DilithiumPubKey& n) {
                    return std::equal(n.begin(), n.end(), neighbor.begin());
                }) != visited_nodes.end();

            if (!is_neighbor || neighbor_visited) {
                continue;
            }

            // Check if channel has enough capacity
            if (channel_info.capacity_sat < amount_sat) {
                continue;
            }

            // Calculate cost (distance): base_fee + (amount * fee_rate)
            uint64_t fee_msat = channel_info.fee_base_msat +
                               (amount_sat * 1000 * channel_info.fee_rate_ppm) / 1000000;
            uint64_t edge_cost = current_distance + fee_msat / 1000;  // Convert to sat

            // Update distance if better path found
            if (!distances.count(neighbor) || edge_cost < distances[neighbor].first) {
                distances[neighbor] = {edge_cost, current_node};
                queue.push_back({edge_cost, neighbor});
            }
        }
    }

    // Reconstruct path
    std::vector<DilithiumPubKey> route;

    if (!distances.count(destination)) {
        // No route found
        return route;
    }

    DilithiumPubKey current = destination;
    while (current != keypair_.public_key) {
        route.push_back(current);
        current = distances[current].second;
    }
    route.push_back(keypair_.public_key);

    // Reverse to get path from source to destination
    std::reverse(route.begin(), route.end());

    return route;
}

bool LightningNode::send_payment(uint64_t amount_sat, const Hash256& payment_hash,
                                const std::vector<DilithiumPubKey>& route)
{
    if (route.empty() || route[0] != keypair_.public_key) {
        failed_payments_++;
        return false;
    }

    // Validate route
    if (!validate_route(route, amount_sat)) {
        failed_payments_++;
        return false;
    }

    // Find channel to first hop
    std::shared_ptr<Channel> first_channel;
    for (const auto& [id, channel] : channels_) {
        if (channel->remote_pubkey == route[1] && channel->is_open()) {
            first_channel = channel;
            break;
        }
    }

    if (!first_channel) {
        failed_payments_++;
        return false;
    }

    // Add HTLC to first channel
    uint32_t cltv_expiry = 500000;  // Block height + safety margin
    std::vector<uint8_t> onion_packet;  // Would contain encrypted routing info

    if (!first_channel->add_htlc(amount_sat, payment_hash, cltv_expiry, onion_packet)) {
        failed_payments_++;
        return false;
    }

    // In a full implementation, would:
    // 1. Create onion-encrypted routing packet
    // 2. Send UPDATE_ADD_HTLC message to next hop
    // 3. Wait for preimage or failure
    // 4. Settle or fail HTLC accordingly

    successful_payments_++;
    return true;
}

bool LightningNode::receive_payment(uint64_t amount_sat, std::string& invoice_out) {
    Invoice invoice = create_invoice(amount_sat, "Payment request");
    invoice_out = invoice.encoded_invoice;
    return true;
}

bool LightningNode::pay_invoice(const std::string& encoded_invoice) {
    // Parse invoice (simplified)
    // Format: lnint1<amount>

    if (encoded_invoice.substr(0, 6) != "lnint1") {
        return false;
    }

    // uint64_t amount_sat = std::stoull(encoded_invoice.substr(6));  // Placeholder parsing

    // In full implementation, would:
    // 1. Extract payment hash from invoice
    // 2. Find route to destination
    // 3. Send payment via HTLCs
    // 4. Wait for settlement

    // For now, just mark as attempted
    failed_payments_++;
    return false;
}

bool LightningNode::forward_htlc(const Hash256& incoming_channel,
                                const Hash256& outgoing_channel,
                                uint64_t htlc_id)
{
    auto in_chan = get_channel(incoming_channel);
    auto out_chan = get_channel(outgoing_channel);

    if (!in_chan || !out_chan) {
        return false;
    }

    // Find HTLC in incoming channel
    auto htlc_it = in_chan->pending_htlcs.find(htlc_id);
    if (htlc_it == in_chan->pending_htlcs.end()) {
        return false;
    }

    const HTLC& incoming_htlc = htlc_it->second;

    // Calculate fee
    uint64_t fee_sat = 1;  // Minimal fee for now
    uint64_t forward_amount = incoming_htlc.amount_sat - fee_sat;

    // Add HTLC to outgoing channel
    if (!out_chan->add_htlc(forward_amount, incoming_htlc.payment_hash,
                           incoming_htlc.cltv_expiry - 40,  // Reduce CLTV
                           incoming_htlc.onion_routing)) {
        return false;
    }

    total_fees_earned_sat_ += fee_sat;
    return true;
}

bool LightningNode::validate_route(const std::vector<DilithiumPubKey>& route,
                                   uint64_t amount_sat)
{
    if (route.size() < 2) {
        return false;  // Must have at least source and destination
    }

    // Verify we have channels to adjacent nodes
    for (size_t i = 0; i < route.size() - 1; ++i) {
        bool found_channel = false;

        // Check if we have a channel to the next hop
        for (const auto& [id, channel] : channels_) {
            if (channel->remote_pubkey == route[i + 1] && channel->is_open()) {
                // Verify channel has sufficient balance
                if (channel->can_send(amount_sat)) {
                    found_channel = true;
                    break;
                }
            }
        }

        // Check network graph for intermediate hops
        if (!found_channel && i > 0) {
            for (const auto& [id, info] : network_graph_) {
                if ((info.node1 == route[i] && info.node2 == route[i + 1]) ||
                    (info.node2 == route[i] && info.node1 == route[i + 1])) {
                    if (info.enabled && info.capacity_sat >= amount_sat) {
                        found_channel = true;
                        break;
                    }
                }
            }
        }

        if (!found_channel) {
            return false;
        }
    }

    return true;
}

uint64_t LightningNode::calculate_route_fees(const std::vector<DilithiumPubKey>& route,
                                             uint64_t amount_sat)
{
    uint64_t total_fees = 0;

    for (size_t i = 0; i < route.size() - 1; ++i) {
        // Find channel info for this hop
        for (const auto& [id, info] : network_graph_) {
            if ((info.node1 == route[i] && info.node2 == route[i + 1]) ||
                (info.node2 == route[i] && info.node1 == route[i + 1])) {

                uint64_t fee_msat = info.fee_base_msat +
                                   (amount_sat * 1000 * info.fee_rate_ppm) / 1000000;
                total_fees += fee_msat / 1000;
                break;
            }
        }
    }

    return total_fees;
}

// Protocol messages implementation

namespace messages {

std::vector<uint8_t> Message::serialize() const {
    std::vector<uint8_t> buffer;

    // Type (2 bytes)
    uint16_t type_val = static_cast<uint16_t>(type);
    buffer.push_back(static_cast<uint8_t>(type_val & 0xFF));
    buffer.push_back(static_cast<uint8_t>((type_val >> 8) & 0xFF));

    // Payload length (4 bytes)
    uint32_t len = static_cast<uint32_t>(payload.size());
    buffer.push_back(static_cast<uint8_t>(len & 0xFF));
    buffer.push_back(static_cast<uint8_t>((len >> 8) & 0xFF));
    buffer.push_back(static_cast<uint8_t>((len >> 16) & 0xFF));
    buffer.push_back(static_cast<uint8_t>((len >> 24) & 0xFF));

    // Payload
    buffer.insert(buffer.end(), payload.begin(), payload.end());

    return buffer;
}

Message Message::deserialize(const std::vector<uint8_t>& data) {
    Message msg;
    if (data.size() < 6) return msg;

    // Type (2 bytes)
    uint16_t type_val = static_cast<uint16_t>(data[0]) |
                       (static_cast<uint16_t>(data[1]) << 8);
    msg.type = static_cast<MessageType>(type_val);

    // Payload length (4 bytes)
    uint32_t len = static_cast<uint32_t>(data[2]) |
                   (static_cast<uint32_t>(data[3]) << 8) |
                   (static_cast<uint32_t>(data[4]) << 16) |
                   (static_cast<uint32_t>(data[5]) << 24);

    // Payload
    if (data.size() >= 6 + len) {
        msg.payload.assign(data.begin() + 6, data.begin() + 6 + len);
    }

    return msg;
}

// OpenChannel message implementation
Message OpenChannel::to_message() const {
    std::vector<uint8_t> payload;

    // Chain hash (32 bytes)
    payload.insert(payload.end(), chain_hash.begin(), chain_hash.end());

    // Temporary channel ID (32 bytes)
    payload.insert(payload.end(), temporary_channel_id.begin(), temporary_channel_id.end());

    // Funding satoshis (8 bytes)
    for (int i = 0; i < 8; i++) {
        payload.push_back(static_cast<uint8_t>((funding_satoshis >> (i * 8)) & 0xFF));
    }

    // Push msat (8 bytes)
    for (int i = 0; i < 8; i++) {
        payload.push_back(static_cast<uint8_t>((push_msat >> (i * 8)) & 0xFF));
    }

    // Dust limit satoshis (8 bytes)
    for (int i = 0; i < 8; i++) {
        payload.push_back(static_cast<uint8_t>((dust_limit_satoshis >> (i * 8)) & 0xFF));
    }

    // Max HTLC value in flight msat (8 bytes)
    for (int i = 0; i < 8; i++) {
        payload.push_back(static_cast<uint8_t>((max_htlc_value_in_flight_msat >> (i * 8)) & 0xFF));
    }

    // Channel reserve satoshis (8 bytes)
    for (int i = 0; i < 8; i++) {
        payload.push_back(static_cast<uint8_t>((channel_reserve_satoshis >> (i * 8)) & 0xFF));
    }

    // HTLC minimum msat (4 bytes)
    payload.push_back(static_cast<uint8_t>(htlc_minimum_msat & 0xFF));
    payload.push_back(static_cast<uint8_t>((htlc_minimum_msat >> 8) & 0xFF));
    payload.push_back(static_cast<uint8_t>((htlc_minimum_msat >> 16) & 0xFF));
    payload.push_back(static_cast<uint8_t>((htlc_minimum_msat >> 24) & 0xFF));

    // Feerate per kw (4 bytes)
    payload.push_back(static_cast<uint8_t>(feerate_per_kw & 0xFF));
    payload.push_back(static_cast<uint8_t>((feerate_per_kw >> 8) & 0xFF));
    payload.push_back(static_cast<uint8_t>((feerate_per_kw >> 16) & 0xFF));
    payload.push_back(static_cast<uint8_t>((feerate_per_kw >> 24) & 0xFF));

    // To self delay (2 bytes)
    payload.push_back(static_cast<uint8_t>(to_self_delay & 0xFF));
    payload.push_back(static_cast<uint8_t>((to_self_delay >> 8) & 0xFF));

    // Max accepted HTLCs (2 bytes)
    payload.push_back(static_cast<uint8_t>(max_accepted_htlcs & 0xFF));
    payload.push_back(static_cast<uint8_t>((max_accepted_htlcs >> 8) & 0xFF));

    // Public keys (each 2592 bytes for Dilithium5)
    payload.insert(payload.end(), funding_pubkey.begin(), funding_pubkey.end());
    payload.insert(payload.end(), revocation_basepoint.begin(), revocation_basepoint.end());
    payload.insert(payload.end(), payment_basepoint.begin(), payment_basepoint.end());
    payload.insert(payload.end(), delayed_payment_basepoint.begin(), delayed_payment_basepoint.end());
    payload.insert(payload.end(), htlc_basepoint.begin(), htlc_basepoint.end());
    payload.insert(payload.end(), first_per_commitment_point.begin(), first_per_commitment_point.end());

    return Message(MessageType::OPEN_CHANNEL, payload);
}

OpenChannel OpenChannel::from_message(const Message& msg) {
    OpenChannel oc;
    if (msg.type != MessageType::OPEN_CHANNEL) {
        return oc;
    }

    const auto& data = msg.payload;
    if (data.size() < 15648) return oc;  // Minimum size with 6 Dilithium keys

    size_t offset = 0;

    // Chain hash (32 bytes)
    std::copy(data.begin() + offset, data.begin() + offset + 32, oc.chain_hash.begin());
    offset += 32;

    // Temporary channel ID (32 bytes)
    std::copy(data.begin() + offset, data.begin() + offset + 32, oc.temporary_channel_id.begin());
    offset += 32;

    // Funding satoshis (8 bytes)
    oc.funding_satoshis = 0;
    for (int i = 0; i < 8; i++) {
        oc.funding_satoshis |= (static_cast<uint64_t>(data[offset + i]) << (i * 8));
    }
    offset += 8;

    // Push msat (8 bytes)
    oc.push_msat = 0;
    for (int i = 0; i < 8; i++) {
        oc.push_msat |= (static_cast<uint64_t>(data[offset + i]) << (i * 8));
    }
    offset += 8;

    // Dust limit satoshis (8 bytes)
    oc.dust_limit_satoshis = 0;
    for (int i = 0; i < 8; i++) {
        oc.dust_limit_satoshis |= (static_cast<uint64_t>(data[offset + i]) << (i * 8));
    }
    offset += 8;

    // Max HTLC value in flight msat (8 bytes)
    oc.max_htlc_value_in_flight_msat = 0;
    for (int i = 0; i < 8; i++) {
        oc.max_htlc_value_in_flight_msat |= (static_cast<uint64_t>(data[offset + i]) << (i * 8));
    }
    offset += 8;

    // Channel reserve satoshis (8 bytes)
    oc.channel_reserve_satoshis = 0;
    for (int i = 0; i < 8; i++) {
        oc.channel_reserve_satoshis |= (static_cast<uint64_t>(data[offset + i]) << (i * 8));
    }
    offset += 8;

    // HTLC minimum msat (4 bytes)
    oc.htlc_minimum_msat = static_cast<uint32_t>(data[offset]) |
                           (static_cast<uint32_t>(data[offset + 1]) << 8) |
                           (static_cast<uint32_t>(data[offset + 2]) << 16) |
                           (static_cast<uint32_t>(data[offset + 3]) << 24);
    offset += 4;

    // Feerate per kw (4 bytes)
    oc.feerate_per_kw = static_cast<uint32_t>(data[offset]) |
                        (static_cast<uint32_t>(data[offset + 1]) << 8) |
                        (static_cast<uint32_t>(data[offset + 2]) << 16) |
                        (static_cast<uint32_t>(data[offset + 3]) << 24);
    offset += 4;

    // To self delay (2 bytes)
    oc.to_self_delay = static_cast<uint16_t>(data[offset]) |
                       (static_cast<uint16_t>(data[offset + 1]) << 8);
    offset += 2;

    // Max accepted HTLCs (2 bytes)
    oc.max_accepted_htlcs = static_cast<uint16_t>(data[offset]) |
                            (static_cast<uint16_t>(data[offset + 1]) << 8);
    offset += 2;

    // Public keys (each 2592 bytes)
    std::copy(data.begin() + offset, data.begin() + offset + 2592, oc.funding_pubkey.begin());
    offset += 2592;
    std::copy(data.begin() + offset, data.begin() + offset + 2592, oc.revocation_basepoint.begin());
    offset += 2592;
    std::copy(data.begin() + offset, data.begin() + offset + 2592, oc.payment_basepoint.begin());
    offset += 2592;
    std::copy(data.begin() + offset, data.begin() + offset + 2592, oc.delayed_payment_basepoint.begin());
    offset += 2592;
    std::copy(data.begin() + offset, data.begin() + offset + 2592, oc.htlc_basepoint.begin());
    offset += 2592;
    std::copy(data.begin() + offset, data.begin() + offset + 2592, oc.first_per_commitment_point.begin());

    return oc;
}

// AcceptChannel message implementation
Message AcceptChannel::to_message() const {
    std::vector<uint8_t> payload;

    // Temporary channel ID (32 bytes)
    payload.insert(payload.end(), temporary_channel_id.begin(), temporary_channel_id.end());

    // Dust limit satoshis (8 bytes)
    for (int i = 0; i < 8; i++) {
        payload.push_back(static_cast<uint8_t>((dust_limit_satoshis >> (i * 8)) & 0xFF));
    }

    // Max HTLC value in flight msat (8 bytes)
    for (int i = 0; i < 8; i++) {
        payload.push_back(static_cast<uint8_t>((max_htlc_value_in_flight_msat >> (i * 8)) & 0xFF));
    }

    // Channel reserve satoshis (8 bytes)
    for (int i = 0; i < 8; i++) {
        payload.push_back(static_cast<uint8_t>((channel_reserve_satoshis >> (i * 8)) & 0xFF));
    }

    // HTLC minimum msat (4 bytes)
    payload.push_back(static_cast<uint8_t>(htlc_minimum_msat & 0xFF));
    payload.push_back(static_cast<uint8_t>((htlc_minimum_msat >> 8) & 0xFF));
    payload.push_back(static_cast<uint8_t>((htlc_minimum_msat >> 16) & 0xFF));
    payload.push_back(static_cast<uint8_t>((htlc_minimum_msat >> 24) & 0xFF));

    // Minimum depth (4 bytes)
    payload.push_back(static_cast<uint8_t>(minimum_depth & 0xFF));
    payload.push_back(static_cast<uint8_t>((minimum_depth >> 8) & 0xFF));
    payload.push_back(static_cast<uint8_t>((minimum_depth >> 16) & 0xFF));
    payload.push_back(static_cast<uint8_t>((minimum_depth >> 24) & 0xFF));

    // To self delay (2 bytes)
    payload.push_back(static_cast<uint8_t>(to_self_delay & 0xFF));
    payload.push_back(static_cast<uint8_t>((to_self_delay >> 8) & 0xFF));

    // Max accepted HTLCs (2 bytes)
    payload.push_back(static_cast<uint8_t>(max_accepted_htlcs & 0xFF));
    payload.push_back(static_cast<uint8_t>((max_accepted_htlcs >> 8) & 0xFF));

    // Public keys (each 2592 bytes for Dilithium5)
    payload.insert(payload.end(), funding_pubkey.begin(), funding_pubkey.end());
    payload.insert(payload.end(), revocation_basepoint.begin(), revocation_basepoint.end());
    payload.insert(payload.end(), payment_basepoint.begin(), payment_basepoint.end());
    payload.insert(payload.end(), delayed_payment_basepoint.begin(), delayed_payment_basepoint.end());
    payload.insert(payload.end(), htlc_basepoint.begin(), htlc_basepoint.end());
    payload.insert(payload.end(), first_per_commitment_point.begin(), first_per_commitment_point.end());

    return Message(MessageType::ACCEPT_CHANNEL, payload);
}

AcceptChannel AcceptChannel::from_message(const Message& msg) {
    AcceptChannel ac;
    if (msg.type != MessageType::ACCEPT_CHANNEL) {
        return ac;
    }

    const auto& data = msg.payload;
    if (data.size() < 15594) return ac;  // Minimum size

    size_t offset = 0;

    // Temporary channel ID (32 bytes)
    std::copy(data.begin() + offset, data.begin() + offset + 32, ac.temporary_channel_id.begin());
    offset += 32;

    // Dust limit satoshis (8 bytes)
    ac.dust_limit_satoshis = 0;
    for (int i = 0; i < 8; i++) {
        ac.dust_limit_satoshis |= (static_cast<uint64_t>(data[offset + i]) << (i * 8));
    }
    offset += 8;

    // Max HTLC value in flight msat (8 bytes)
    ac.max_htlc_value_in_flight_msat = 0;
    for (int i = 0; i < 8; i++) {
        ac.max_htlc_value_in_flight_msat |= (static_cast<uint64_t>(data[offset + i]) << (i * 8));
    }
    offset += 8;

    // Channel reserve satoshis (8 bytes)
    ac.channel_reserve_satoshis = 0;
    for (int i = 0; i < 8; i++) {
        ac.channel_reserve_satoshis |= (static_cast<uint64_t>(data[offset + i]) << (i * 8));
    }
    offset += 8;

    // HTLC minimum msat (4 bytes)
    ac.htlc_minimum_msat = static_cast<uint32_t>(data[offset]) |
                           (static_cast<uint32_t>(data[offset + 1]) << 8) |
                           (static_cast<uint32_t>(data[offset + 2]) << 16) |
                           (static_cast<uint32_t>(data[offset + 3]) << 24);
    offset += 4;

    // Minimum depth (4 bytes)
    ac.minimum_depth = static_cast<uint32_t>(data[offset]) |
                       (static_cast<uint32_t>(data[offset + 1]) << 8) |
                       (static_cast<uint32_t>(data[offset + 2]) << 16) |
                       (static_cast<uint32_t>(data[offset + 3]) << 24);
    offset += 4;

    // To self delay (2 bytes)
    ac.to_self_delay = static_cast<uint16_t>(data[offset]) |
                       (static_cast<uint16_t>(data[offset + 1]) << 8);
    offset += 2;

    // Max accepted HTLCs (2 bytes)
    ac.max_accepted_htlcs = static_cast<uint16_t>(data[offset]) |
                            (static_cast<uint16_t>(data[offset + 1]) << 8);
    offset += 2;

    // Public keys (each 2592 bytes)
    std::copy(data.begin() + offset, data.begin() + offset + 2592, ac.funding_pubkey.begin());
    offset += 2592;
    std::copy(data.begin() + offset, data.begin() + offset + 2592, ac.revocation_basepoint.begin());
    offset += 2592;
    std::copy(data.begin() + offset, data.begin() + offset + 2592, ac.payment_basepoint.begin());
    offset += 2592;
    std::copy(data.begin() + offset, data.begin() + offset + 2592, ac.delayed_payment_basepoint.begin());
    offset += 2592;
    std::copy(data.begin() + offset, data.begin() + offset + 2592, ac.htlc_basepoint.begin());
    offset += 2592;
    std::copy(data.begin() + offset, data.begin() + offset + 2592, ac.first_per_commitment_point.begin());

    return ac;
}

// FundingCreated message implementation
Message FundingCreated::to_message() const {
    std::vector<uint8_t> payload;

    // Temporary channel ID (32 bytes)
    payload.insert(payload.end(), temporary_channel_id.begin(), temporary_channel_id.end());

    // Funding TXID (32 bytes)
    payload.insert(payload.end(), funding_txid.begin(), funding_txid.end());

    // Funding output index (2 bytes)
    payload.push_back(static_cast<uint8_t>(funding_output_index & 0xFF));
    payload.push_back(static_cast<uint8_t>((funding_output_index >> 8) & 0xFF));

    // Signature (4595 bytes for Dilithium5)
    payload.insert(payload.end(), signature.begin(), signature.end());

    return Message(MessageType::FUNDING_CREATED, payload);
}

FundingCreated FundingCreated::from_message(const Message& msg) {
    FundingCreated fc;
    if (msg.type != MessageType::FUNDING_CREATED || msg.payload.size() < 4661) {
        return fc;
    }

    const auto& data = msg.payload;
    size_t offset = 0;

    std::copy(data.begin() + offset, data.begin() + offset + 32, fc.temporary_channel_id.begin());
    offset += 32;

    std::copy(data.begin() + offset, data.begin() + offset + 32, fc.funding_txid.begin());
    offset += 32;

    fc.funding_output_index = static_cast<uint16_t>(data[offset]) |
                              (static_cast<uint16_t>(data[offset + 1]) << 8);
    offset += 2;

    std::copy(data.begin() + offset, data.begin() + offset + 4595, fc.signature.begin());

    return fc;
}

// FundingSigned message implementation
Message FundingSigned::to_message() const {
    std::vector<uint8_t> payload;

    // Channel ID (32 bytes)
    payload.insert(payload.end(), channel_id.begin(), channel_id.end());

    // Signature (4595 bytes for Dilithium5)
    payload.insert(payload.end(), signature.begin(), signature.end());

    return Message(MessageType::FUNDING_SIGNED, payload);
}

FundingSigned FundingSigned::from_message(const Message& msg) {
    FundingSigned fs;
    if (msg.type != MessageType::FUNDING_SIGNED || msg.payload.size() < 4627) {
        return fs;
    }

    const auto& data = msg.payload;
    size_t offset = 0;

    std::copy(data.begin() + offset, data.begin() + offset + 32, fs.channel_id.begin());
    offset += 32;

    std::copy(data.begin() + offset, data.begin() + offset + 4595, fs.signature.begin());

    return fs;
}

// UpdateAddHTLC message implementation
Message UpdateAddHTLC::to_message() const {
    std::vector<uint8_t> payload;

    // Channel ID (32 bytes)
    payload.insert(payload.end(), channel_id.begin(), channel_id.end());

    // HTLC ID (8 bytes)
    for (int i = 0; i < 8; i++) {
        payload.push_back(static_cast<uint8_t>((htlc_id >> (i * 8)) & 0xFF));
    }

    // Amount msat (8 bytes)
    for (int i = 0; i < 8; i++) {
        payload.push_back(static_cast<uint8_t>((amount_msat >> (i * 8)) & 0xFF));
    }

    // Payment hash (32 bytes)
    payload.insert(payload.end(), payment_hash.begin(), payment_hash.end());

    // CLTV expiry (4 bytes)
    payload.push_back(static_cast<uint8_t>(cltv_expiry & 0xFF));
    payload.push_back(static_cast<uint8_t>((cltv_expiry >> 8) & 0xFF));
    payload.push_back(static_cast<uint8_t>((cltv_expiry >> 16) & 0xFF));
    payload.push_back(static_cast<uint8_t>((cltv_expiry >> 24) & 0xFF));

    // Onion routing packet (1366 bytes fixed)
    if (onion_routing_packet.size() >= 1366) {
        payload.insert(payload.end(), onion_routing_packet.begin(), onion_routing_packet.begin() + 1366);
    } else {
        payload.insert(payload.end(), onion_routing_packet.begin(), onion_routing_packet.end());
        // Pad to 1366 bytes
        payload.insert(payload.end(), 1366 - onion_routing_packet.size(), 0);
    }

    return Message(MessageType::UPDATE_ADD_HTLC, payload);
}

UpdateAddHTLC UpdateAddHTLC::from_message(const Message& msg) {
    UpdateAddHTLC uah;
    if (msg.type != MessageType::UPDATE_ADD_HTLC || msg.payload.size() < 1450) {
        return uah;
    }

    const auto& data = msg.payload;
    size_t offset = 0;

    std::copy(data.begin() + offset, data.begin() + offset + 32, uah.channel_id.begin());
    offset += 32;

    uah.htlc_id = 0;
    for (int i = 0; i < 8; i++) {
        uah.htlc_id |= (static_cast<uint64_t>(data[offset + i]) << (i * 8));
    }
    offset += 8;

    uah.amount_msat = 0;
    for (int i = 0; i < 8; i++) {
        uah.amount_msat |= (static_cast<uint64_t>(data[offset + i]) << (i * 8));
    }
    offset += 8;

    std::copy(data.begin() + offset, data.begin() + offset + 32, uah.payment_hash.begin());
    offset += 32;

    uah.cltv_expiry = static_cast<uint32_t>(data[offset]) |
                      (static_cast<uint32_t>(data[offset + 1]) << 8) |
                      (static_cast<uint32_t>(data[offset + 2]) << 16) |
                      (static_cast<uint32_t>(data[offset + 3]) << 24);
    offset += 4;

    uah.onion_routing_packet.assign(data.begin() + offset, data.begin() + offset + 1366);

    return uah;
}

// UpdateFulfillHTLC message implementation
Message UpdateFulfillHTLC::to_message() const {
    std::vector<uint8_t> payload;

    // Channel ID (32 bytes)
    payload.insert(payload.end(), channel_id.begin(), channel_id.end());

    // HTLC ID (8 bytes)
    for (int i = 0; i < 8; i++) {
        payload.push_back(static_cast<uint8_t>((htlc_id >> (i * 8)) & 0xFF));
    }

    // Payment preimage (32 bytes)
    if (payment_preimage.size() >= 32) {
        payload.insert(payload.end(), payment_preimage.begin(), payment_preimage.begin() + 32);
    } else {
        payload.insert(payload.end(), payment_preimage.begin(), payment_preimage.end());
        payload.insert(payload.end(), 32 - payment_preimage.size(), 0);
    }

    return Message(MessageType::UPDATE_FULFILL_HTLC, payload);
}

UpdateFulfillHTLC UpdateFulfillHTLC::from_message(const Message& msg) {
    UpdateFulfillHTLC ufh;
    if (msg.type != MessageType::UPDATE_FULFILL_HTLC || msg.payload.size() < 72) {
        return ufh;
    }

    const auto& data = msg.payload;
    size_t offset = 0;

    std::copy(data.begin() + offset, data.begin() + offset + 32, ufh.channel_id.begin());
    offset += 32;

    ufh.htlc_id = 0;
    for (int i = 0; i < 8; i++) {
        ufh.htlc_id |= (static_cast<uint64_t>(data[offset + i]) << (i * 8));
    }
    offset += 8;

    ufh.payment_preimage.assign(data.begin() + offset, data.begin() + offset + 32);

    return ufh;
}

// UpdateFailHTLC message implementation
Message UpdateFailHTLC::to_message() const {
    std::vector<uint8_t> payload;

    // Channel ID (32 bytes)
    payload.insert(payload.end(), channel_id.begin(), channel_id.end());

    // HTLC ID (8 bytes)
    for (int i = 0; i < 8; i++) {
        payload.push_back(static_cast<uint8_t>((htlc_id >> (i * 8)) & 0xFF));
    }

    // Reason length (2 bytes)
    uint16_t reason_len = static_cast<uint16_t>(reason.size());
    payload.push_back(static_cast<uint8_t>(reason_len & 0xFF));
    payload.push_back(static_cast<uint8_t>((reason_len >> 8) & 0xFF));

    // Reason data
    payload.insert(payload.end(), reason.begin(), reason.end());

    return Message(MessageType::UPDATE_FAIL_HTLC, payload);
}

UpdateFailHTLC UpdateFailHTLC::from_message(const Message& msg) {
    UpdateFailHTLC ufh;
    if (msg.type != MessageType::UPDATE_FAIL_HTLC || msg.payload.size() < 42) {
        return ufh;
    }

    const auto& data = msg.payload;
    size_t offset = 0;

    std::copy(data.begin() + offset, data.begin() + offset + 32, ufh.channel_id.begin());
    offset += 32;

    ufh.htlc_id = 0;
    for (int i = 0; i < 8; i++) {
        ufh.htlc_id |= (static_cast<uint64_t>(data[offset + i]) << (i * 8));
    }
    offset += 8;

    uint16_t reason_len = static_cast<uint16_t>(data[offset]) |
                          (static_cast<uint16_t>(data[offset + 1]) << 8);
    offset += 2;

    if (data.size() >= offset + reason_len) {
        ufh.reason.assign(data.begin() + offset, data.begin() + offset + reason_len);
    }

    return ufh;
}

// CommitmentSigned message implementation
Message CommitmentSigned::to_message() const {
    std::vector<uint8_t> payload;

    // Channel ID (32 bytes)
    payload.insert(payload.end(), channel_id.begin(), channel_id.end());

    // Signature (4595 bytes for Dilithium5)
    payload.insert(payload.end(), signature.begin(), signature.end());

    // HTLC signature count (2 bytes)
    uint16_t htlc_count = static_cast<uint16_t>(htlc_signatures.size());
    payload.push_back(static_cast<uint8_t>(htlc_count & 0xFF));
    payload.push_back(static_cast<uint8_t>((htlc_count >> 8) & 0xFF));

    // HTLC signatures (each 4595 bytes)
    for (const auto& sig : htlc_signatures) {
        payload.insert(payload.end(), sig.begin(), sig.end());
    }

    return Message(MessageType::COMMITMENT_SIGNED, payload);
}

CommitmentSigned CommitmentSigned::from_message(const Message& msg) {
    CommitmentSigned cs;
    if (msg.type != MessageType::COMMITMENT_SIGNED || msg.payload.size() < 4629) {
        return cs;
    }

    const auto& data = msg.payload;
    size_t offset = 0;

    std::copy(data.begin() + offset, data.begin() + offset + 32, cs.channel_id.begin());
    offset += 32;

    std::copy(data.begin() + offset, data.begin() + offset + 4595, cs.signature.begin());
    offset += 4595;

    uint16_t htlc_count = static_cast<uint16_t>(data[offset]) |
                          (static_cast<uint16_t>(data[offset + 1]) << 8);
    offset += 2;

    for (uint16_t i = 0; i < htlc_count && (offset + 4595) <= data.size(); i++) {
        DilithiumSignature sig;
        std::copy(data.begin() + offset, data.begin() + offset + 4595, sig.begin());
        cs.htlc_signatures.push_back(sig);
        offset += 4595;
    }

    return cs;
}

// RevokeAndAck message implementation
Message RevokeAndAck::to_message() const {
    std::vector<uint8_t> payload;

    // Channel ID (32 bytes)
    payload.insert(payload.end(), channel_id.begin(), channel_id.end());

    // Per commitment secret (32 bytes)
    if (per_commitment_secret.size() >= 32) {
        payload.insert(payload.end(), per_commitment_secret.begin(), per_commitment_secret.begin() + 32);
    } else {
        payload.insert(payload.end(), per_commitment_secret.begin(), per_commitment_secret.end());
        payload.insert(payload.end(), 32 - per_commitment_secret.size(), 0);
    }

    // Next per commitment point (2592 bytes for Dilithium5 pubkey)
    payload.insert(payload.end(), next_per_commitment_point.begin(), next_per_commitment_point.end());

    return Message(MessageType::REVOKE_AND_ACK, payload);
}

RevokeAndAck RevokeAndAck::from_message(const Message& msg) {
    RevokeAndAck raa;
    if (msg.type != MessageType::REVOKE_AND_ACK || msg.payload.size() < 2656) {
        return raa;
    }

    const auto& data = msg.payload;
    size_t offset = 0;

    std::copy(data.begin() + offset, data.begin() + offset + 32, raa.channel_id.begin());
    offset += 32;

    raa.per_commitment_secret.assign(data.begin() + offset, data.begin() + offset + 32);
    offset += 32;

    std::copy(data.begin() + offset, data.begin() + offset + 2592, raa.next_per_commitment_point.begin());

    return raa;
}

// Shutdown message implementation
Message Shutdown::to_message() const {
    std::vector<uint8_t> payload;

    // Channel ID (32 bytes)
    payload.insert(payload.end(), channel_id.begin(), channel_id.end());

    // Scriptpubkey length (2 bytes)
    uint16_t script_len = static_cast<uint16_t>(scriptpubkey.size());
    payload.push_back(static_cast<uint8_t>(script_len & 0xFF));
    payload.push_back(static_cast<uint8_t>((script_len >> 8) & 0xFF));

    // Scriptpubkey data
    payload.insert(payload.end(), scriptpubkey.begin(), scriptpubkey.end());

    return Message(MessageType::SHUTDOWN, payload);
}

Shutdown Shutdown::from_message(const Message& msg) {
    Shutdown sd;
    if (msg.type != MessageType::SHUTDOWN || msg.payload.size() < 34) {
        return sd;
    }

    const auto& data = msg.payload;
    size_t offset = 0;

    std::copy(data.begin() + offset, data.begin() + offset + 32, sd.channel_id.begin());
    offset += 32;

    uint16_t script_len = static_cast<uint16_t>(data[offset]) |
                          (static_cast<uint16_t>(data[offset + 1]) << 8);
    offset += 2;

    if (data.size() >= offset + script_len) {
        sd.scriptpubkey.assign(data.begin() + offset, data.begin() + offset + script_len);
    }

    return sd;
}

// ClosingSigned message implementation
Message ClosingSigned::to_message() const {
    std::vector<uint8_t> payload;

    // Channel ID (32 bytes)
    payload.insert(payload.end(), channel_id.begin(), channel_id.end());

    // Fee satoshis (8 bytes)
    for (int i = 0; i < 8; i++) {
        payload.push_back(static_cast<uint8_t>((fee_satoshis >> (i * 8)) & 0xFF));
    }

    // Signature (4595 bytes for Dilithium5)
    payload.insert(payload.end(), signature.begin(), signature.end());

    return Message(MessageType::CLOSING_SIGNED, payload);
}

ClosingSigned ClosingSigned::from_message(const Message& msg) {
    ClosingSigned cs;
    if (msg.type != MessageType::CLOSING_SIGNED || msg.payload.size() < 4635) {
        return cs;
    }

    const auto& data = msg.payload;
    size_t offset = 0;

    std::copy(data.begin() + offset, data.begin() + offset + 32, cs.channel_id.begin());
    offset += 32;

    cs.fee_satoshis = 0;
    for (int i = 0; i < 8; i++) {
        cs.fee_satoshis |= (static_cast<uint64_t>(data[offset + i]) << (i * 8));
    }
    offset += 8;

    std::copy(data.begin() + offset, data.begin() + offset + 4595, cs.signature.begin());

    return cs;
}

} // namespace messages

} // namespace lightning
} // namespace intcoin
