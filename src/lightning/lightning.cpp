// Copyright (c) 2025 INTcoin Team (Neil Adamson)
// MIT License

#include "intcoin/lightning.h"
#include "intcoin/blockchain.h"
#include "intcoin/util.h"
#include "intcoin/crypto.h"
#include <queue>
#include <algorithm>
#include <cmath>
#include <sstream>
#include <iomanip>

namespace intcoin {

// ============================================================================
// Lightning Network Implementation
// ============================================================================

HTLC::HTLC() : id(0), amount(0), cltv_expiry(0), incoming(false), fulfilled(false) {}
HTLC::HTLC(uint64_t id_, uint64_t amt, const uint256& hash, uint32_t expiry, bool inc)
    : id(id_), amount(amt), payment_hash(hash), cltv_expiry(expiry), incoming(inc), fulfilled(false), preimage() {}
std::vector<uint8_t> HTLC::Serialize() const { return std::vector<uint8_t>(); }
Result<HTLC> HTLC::Deserialize(const std::vector<uint8_t>& data) { return Result<HTLC>::Ok(HTLC()); }

ChannelConfig::ChannelConfig() : dust_limit(lightning::DUST_LIMIT), max_htlc_value(lightning::MAX_CHANNEL_CAPACITY),
    channel_reserve(lightning::MIN_CHANNEL_CAPACITY / 100), htlc_minimum(1000), to_self_delay(144),
    max_accepted_htlcs(lightning::MAX_HTLC_COUNT) {}
ChannelConfig ChannelConfig::Default() { return ChannelConfig(); }

CommitmentTransaction::CommitmentTransaction() : commitment_number(0), local_balance(0), remote_balance(0), fee(0) {}
Result<CommitmentTransaction> CommitmentTransaction::Build(const uint256&, uint32_t, uint64_t, uint64_t, uint64_t,
    const std::vector<HTLC>&, uint64_t, const ChannelConfig&) {
    return Result<CommitmentTransaction>::Ok(CommitmentTransaction());
}
bool CommitmentTransaction::Verify(const PublicKey&, const PublicKey&) const { return true; }

Channel::Channel() : state(ChannelState::OPENING), capacity(0), local_balance(0), remote_balance(0),
    funding_vout(0), funding_confirmations(0), commitment_number(0), next_htlc_id(0) {}
Channel::Channel(const PublicKey& local, const PublicKey& remote, uint64_t cap)
    : local_node_id(local), remote_node_id(remote), state(ChannelState::OPENING), capacity(cap),
      local_balance(cap), remote_balance(0), funding_vout(0), funding_confirmations(0),
      commitment_number(0), next_htlc_id(0), local_config(ChannelConfig::Default()),
      remote_config(ChannelConfig::Default()), opened_at(std::chrono::system_clock::now()),
      last_update(std::chrono::system_clock::now()) { temporary_id = RandomGenerator::GetRandomUint256(); }
Result<void> Channel::Open(const Transaction& funding_tx, uint32_t vout) {
    funding_txid = funding_tx.GetHash(); funding_vout = vout; state = ChannelState::OPEN;
    return Result<void>::Ok();
}
Result<void> Channel::Close(bool force) {
    state = force ? ChannelState::CLOSING_FORCE : ChannelState::CLOSING_MUTUAL;
    return Result<void>::Ok();
}
Result<uint64_t> Channel::AddHTLC(uint64_t amount, const uint256& payment_hash, uint32_t expiry) {
    uint64_t htlc_id = next_htlc_id++; HTLC htlc(htlc_id, amount, payment_hash, expiry, false);
    pending_htlcs.push_back(htlc); return Result<uint64_t>::Ok(htlc_id);
}
Result<void> Channel::FulfillHTLC(uint64_t, const uint256&) { return Result<void>::Ok(); }
Result<void> Channel::FailHTLC(uint64_t) { return Result<void>::Ok(); }
Result<void> Channel::UpdateCommitment() { return Result<void>::Ok(); }
uint64_t Channel::GetLocalBalance() const { return local_balance; }
uint64_t Channel::GetRemoteBalance() const { return remote_balance; }
uint64_t Channel::GetAvailableBalance() const { return local_balance; }
bool Channel::CanSend(uint64_t amount) const { return local_balance >= amount; }
bool Channel::CanReceive(uint64_t amount) const { return remote_balance >= amount; }
std::vector<uint8_t> Channel::Serialize() const { return std::vector<uint8_t>(); }
Result<Channel> Channel::Deserialize(const std::vector<uint8_t>&) { return Result<Channel>::Ok(Channel()); }

RouteHop::RouteHop() : amount(0), cltv_expiry(0), fee(0) {}
RouteHop::RouteHop(const PublicKey& node, const uint256& chan, uint64_t amt, uint32_t expiry)
    : node_id(node), channel_id(chan), amount(amt), cltv_expiry(expiry), fee(0) {}
PaymentRoute::PaymentRoute() : total_amount(0), total_fees(0), total_cltv(0) {}
bool PaymentRoute::IsValid() const { return !hops.empty(); }
uint64_t PaymentRoute::CalculateTotalFees() const { return total_fees; }

Invoice::Invoice() : amount(0), expiry(3600), min_final_cltv(lightning::MIN_CLTV_EXPIRY),
    created_at(std::chrono::system_clock::now()) {}
Invoice::Invoice(uint64_t amt, const std::string& desc, const PublicKey& payee_key)
    : amount(amt), description(desc), expiry(3600), min_final_cltv(lightning::MIN_CLTV_EXPIRY),
      payee(payee_key), created_at(std::chrono::system_clock::now()) {}
uint256 Invoice::GeneratePaymentHash(const uint256& preimage) {
    return SHA3::Hash(preimage.data(), 32);
}

// BOLT #11 Invoice Encoding (simplified for INTcoin)
std::string Invoice::Encode() const {
    std::ostringstream oss;

    // Prefix: "lint" for INTcoin Lightning Network
    oss << "lint";

    // Encode amount (in INTS, hex)
    oss << std::hex << std::setfill('0') << std::setw(16) << amount;

    // Encode payment hash (hex)
    oss << Uint256ToHex(payment_hash);  // 64 hex chars (32 bytes)

    // Encode expiry (hex, 8 chars)
    oss << std::hex << std::setw(8) << expiry;

    // Add description length and description (simple encoding)
    std::string desc_hex;
    for (char c : description) {
        oss << std::hex << std::setw(2) << static_cast<int>(c);
    }

    // Note: Full BOLT #11 would include bech32 encoding, routing hints, etc.
    // This is a simplified version for INTcoin

    return oss.str();
}

// BOLT #11 Invoice Decoding (simplified)
Result<Invoice> Invoice::Decode(const std::string& bolt11) {
    if (bolt11.substr(0, 4) != "lint") {
        return Result<Invoice>::Error("Invalid invoice prefix");
    }

    if (bolt11.length() < 4 + 16 + 64 + 8) {
        return Result<Invoice>::Error("Invoice too short");
    }

    Invoice invoice;

    try {
        // Decode amount (16 hex chars after "lint")
        std::string amount_hex = bolt11.substr(4, 16);
        invoice.amount = std::stoull(amount_hex, nullptr, 16);

        // Decode payment hash (64 hex chars)
        std::string hash_hex = bolt11.substr(4 + 16, 64);
        // Note: Would need proper uint256 parsing in full implementation

        // Decode expiry (8 hex chars)
        std::string expiry_hex = bolt11.substr(4 + 16 + 64, 8);
        invoice.expiry = std::stoul(expiry_hex, nullptr, 16);

        // Decode description (remaining chars)
        std::string desc_hex = bolt11.substr(4 + 16 + 64 + 8);
        for (size_t i = 0; i + 1 < desc_hex.length(); i += 2) {
            std::string byte_hex = desc_hex.substr(i, 2);
            char c = static_cast<char>(std::stoi(byte_hex, nullptr, 16));
            invoice.description += c;
        }

        invoice.min_final_cltv = lightning::MIN_CLTV_EXPIRY;
        invoice.created_at = std::chrono::system_clock::now();

    } catch (const std::exception& e) {
        return Result<Invoice>::Error(std::string("Failed to decode invoice: ") + e.what());
    }

    return Result<Invoice>::Ok(invoice);
}
Result<void> Invoice::Sign(const SecretKey&) { return Result<void>::Ok(); }
bool Invoice::Verify() const { return true; }
bool Invoice::IsExpired() const { return false; }

ChannelInfo::ChannelInfo() : capacity(0), base_fee(lightning::BASE_FEE), fee_rate(lightning::FEE_RATE),
    cltv_expiry_delta(lightning::CLTV_EXPIRY_DELTA), enabled(true),
    last_update(std::chrono::system_clock::now()) {}
NodeInfo::NodeInfo() : last_update(std::chrono::system_clock::now()) {}
NetworkGraph::NetworkGraph() {}
void NetworkGraph::AddChannel(const ChannelInfo& channel) {
    std::lock_guard<std::mutex> lock(mutex_); channels_[channel.channel_id] = channel;
}
void NetworkGraph::RemoveChannel(const uint256& channel_id) {
    std::lock_guard<std::mutex> lock(mutex_); channels_.erase(channel_id);
}
void NetworkGraph::UpdateChannel(const uint256& channel_id, const ChannelInfo& info) {
    std::lock_guard<std::mutex> lock(mutex_); channels_[channel_id] = info;
}
void NetworkGraph::AddNode(const NodeInfo& node) {
    std::lock_guard<std::mutex> lock(mutex_); nodes_[node.node_id] = node;
}
void NetworkGraph::RemoveNode(const PublicKey& node_id) {
    std::lock_guard<std::mutex> lock(mutex_); nodes_.erase(node_id);
}
Result<ChannelInfo> NetworkGraph::GetChannel(const uint256& channel_id) const {
    std::lock_guard<std::mutex> lock(mutex_); auto it = channels_.find(channel_id);
    if (it == channels_.end()) return Result<ChannelInfo>::Error("Channel not found");
    return Result<ChannelInfo>::Ok(it->second);
}
Result<NodeInfo> NetworkGraph::GetNode(const PublicKey& node_id) const {
    std::lock_guard<std::mutex> lock(mutex_); auto it = nodes_.find(node_id);
    if (it == nodes_.end()) return Result<NodeInfo>::Error("Node not found");
    return Result<NodeInfo>::Ok(it->second);
}
std::vector<ChannelInfo> NetworkGraph::GetNodeChannels(const PublicKey&) const {
    return std::vector<ChannelInfo>();
}
// Dijkstra's algorithm for finding optimal payment route
Result<PaymentRoute> NetworkGraph::FindRoute(const PublicKey& source, const PublicKey& dest,
                                             uint64_t amount, uint32_t max_hops) const {
    std::lock_guard<std::mutex> lock(mutex_);

    // Check source and destination exist
    if (nodes_.find(source) == nodes_.end()) {
        return Result<PaymentRoute>::Error("Source node not found");
    }
    if (nodes_.find(dest) == nodes_.end()) {
        return Result<PaymentRoute>::Error("Destination node not found");
    }

    // Dijkstra's algorithm data structures
    std::map<PublicKey, uint64_t> distances;        // Best cost to reach node
    std::map<PublicKey, PublicKey> previous;        // Previous node in path
    std::map<PublicKey, uint256> previous_channel;  // Channel used to reach node

    // Priority queue: (cost, node_id)
    using QueueItem = std::pair<uint64_t, PublicKey>;
    std::priority_queue<QueueItem, std::vector<QueueItem>, std::greater<QueueItem>> pq;

    // Initialize
    distances[source] = 0;
    pq.push({0, source});

    // Dijkstra's main loop
    while (!pq.empty()) {
        auto [current_cost, current_node] = pq.top();
        pq.pop();

        // Found destination
        if (current_node == dest) {
            break;
        }

        // Skip if we've found a better path
        if (distances.count(current_node) && current_cost > distances[current_node]) {
            continue;
        }

        // Explore neighbors via channels
        for (const auto& [chan_id, channel] : channels_) {
            if (!channel.enabled) continue;

            // Determine which node is the neighbor
            PublicKey neighbor;
            if (channel.node1 == current_node) {
                neighbor = channel.node2;
            } else if (channel.node2 == current_node) {
                neighbor = channel.node1;
            } else {
                continue;  // Channel doesn't involve current_node
            }

            // Check channel capacity
            if (channel.capacity < amount) {
                continue;
            }

            // Calculate fee for this hop
            uint64_t fee = channel.base_fee + (amount * channel.fee_rate) / 1000000;
            uint64_t hop_cost = current_cost + fee + amount / 1000;  // Include routing cost

            // Update if better path found
            if (!distances.count(neighbor) || hop_cost < distances[neighbor]) {
                distances[neighbor] = hop_cost;
                previous[neighbor] = current_node;
                previous_channel[neighbor] = chan_id;
                pq.push({hop_cost, neighbor});
            }
        }
    }

    // Check if destination was reached
    if (!distances.count(dest)) {
        return Result<PaymentRoute>::Error("No route found to destination");
    }

    // Reconstruct path
    std::vector<RouteHop> hops;
    PublicKey current = dest;
    uint32_t total_cltv = lightning::MIN_CLTV_EXPIRY;

    while (current != source) {
        if (!previous.count(current)) {
            return Result<PaymentRoute>::Error("Failed to reconstruct route");
        }

        PublicKey prev_node = previous[current];
        uint256 chan_id = previous_channel[current];

        // Get channel info
        auto chan_it = channels_.find(chan_id);
        if (chan_it == channels_.end()) {
            return Result<PaymentRoute>::Error("Channel not found in route reconstruction");
        }

        const auto& channel = chan_it->second;
        uint64_t fee = channel.base_fee + (amount * channel.fee_rate) / 1000000;

        RouteHop hop;
        hop.node_id = current;
        hop.channel_id = chan_id;
        hop.amount = amount + fee;
        hop.cltv_expiry = total_cltv;
        hop.fee = fee;

        hops.insert(hops.begin(), hop);  // Prepend to reverse order

        total_cltv += channel.cltv_expiry_delta;
        current = prev_node;
    }

    // Check hop count
    if (hops.size() > max_hops) {
        return Result<PaymentRoute>::Error("Route exceeds maximum hop count");
    }

    // Build result
    PaymentRoute route;
    route.hops = hops;
    route.total_fees = route.CalculateTotalFees();
    route.total_amount = amount + route.total_fees;
    route.total_cltv = total_cltv;

    return Result<PaymentRoute>::Ok(route);
}
std::vector<uint8_t> NetworkGraph::Serialize() const { return std::vector<uint8_t>(); }
Result<std::unique_ptr<NetworkGraph>> NetworkGraph::Deserialize(const std::vector<uint8_t>&) {
    return Result<std::unique_ptr<NetworkGraph>>::Ok(std::make_unique<NetworkGraph>());
}

// ============================================================================
// Sphinx Onion Routing Implementation (Simplified)
// ============================================================================

OnionPacket::OnionPacket() : version(0) {}

// Create onion packet (Sphinx protocol foundation)
Result<OnionPacket> OnionPacket::Create(const std::vector<RouteHop>& route,
                                        const uint256& payment_hash,
                                        const std::vector<uint8_t>& session_key) {
    if (route.empty()) {
        return Result<OnionPacket>::Error("Route is empty");
    }

    OnionPacket packet;
    packet.version = 0;  // Protocol version

    // Generate ephemeral key pair for this payment
    // In full Sphinx: would use ECDH for each hop
    packet.public_key.resize(33);  // Compressed public key size
    std::copy(session_key.begin(),
              session_key.begin() + std::min(session_key.size(), size_t(33)),
              packet.public_key.begin());

    // Build hop data (encrypted layers)
    // Each layer contains: amount, outgoing_channel, cltv_expiry
    packet.hops_data.clear();
    for (const auto& hop : route) {
        // Encode hop data (simplified)
        // Real Sphinx: ChaCha20-Poly1305 encryption for each layer
        std::vector<uint8_t> hop_data;

        // Amount (8 bytes)
        for (int i = 7; i >= 0; i--) {
            hop_data.push_back((hop.amount >> (i * 8)) & 0xFF);
        }

        // CLTV expiry (4 bytes)
        for (int i = 3; i >= 0; i--) {
            hop_data.push_back((hop.cltv_expiry >> (i * 8)) & 0xFF);
        }

        // Channel ID (32 bytes)
        auto chan_bytes = hop.channel_id.data();
        hop_data.insert(hop_data.end(), chan_bytes, chan_bytes + 32);

        // Append to hops_data
        packet.hops_data.insert(packet.hops_data.end(), hop_data.begin(), hop_data.end());
    }

    // Calculate HMAC over the packet
    packet.hmac.resize(32);
    auto hash = SHA3::Hash(packet.hops_data.data(), packet.hops_data.size());
    std::copy(hash.data(), hash.data() + 32, packet.hmac.begin());

    return Result<OnionPacket>::Ok(packet);
}

// Peel one layer of the onion (process by intermediate node)
Result<std::pair<RouteHop, OnionPacket>> OnionPacket::Peel(const SecretKey& node_key) const {
    if (hops_data.empty()) {
        return Result<std::pair<RouteHop, OnionPacket>>::Error("Empty onion packet");
    }

    // Verify HMAC
    // In production: would compute HMAC and verify it matches
    // auto computed_hmac = SHA3::Hash(hops_data.data(), hops_data.size());

    // Decrypt and extract this hop's data (first 44 bytes)
    if (hops_data.size() < 44) {
        return Result<std::pair<RouteHop, OnionPacket>>::Error("Insufficient hop data");
    }

    RouteHop hop;

    // Parse amount (8 bytes)
    hop.amount = 0;
    for (int i = 0; i < 8; i++) {
        hop.amount = (hop.amount << 8) | hops_data[i];
    }

    // Parse CLTV expiry (4 bytes)
    hop.cltv_expiry = 0;
    for (int i = 8; i < 12; i++) {
        hop.cltv_expiry = (hop.cltv_expiry << 8) | hops_data[i];
    }

    // Parse channel ID (32 bytes)
    // Note: Would need proper uint256 construction

    // Create next packet (remove this layer)
    OnionPacket next_packet;
    next_packet.version = version;
    next_packet.public_key = public_key;
    next_packet.hops_data.assign(hops_data.begin() + 44, hops_data.end());

    // Recalculate HMAC for next packet
    next_packet.hmac.resize(32);
    auto next_hash = SHA3::Hash(next_packet.hops_data.data(), next_packet.hops_data.size());
    std::copy(next_hash.data(), next_hash.data() + 32, next_packet.hmac.begin());

    return Result<std::pair<RouteHop, OnionPacket>>::Ok(std::make_pair(hop, next_packet));
}

std::vector<uint8_t> OnionPacket::Serialize() const {
    std::vector<uint8_t> data;
    data.push_back(version);
    data.insert(data.end(), public_key.begin(), public_key.end());

    // Add hops_data length (4 bytes)
    uint32_t len = hops_data.size();
    for (int i = 3; i >= 0; i--) {
        data.push_back((len >> (i * 8)) & 0xFF);
    }
    data.insert(data.end(), hops_data.begin(), hops_data.end());
    data.insert(data.end(), hmac.begin(), hmac.end());

    return data;
}

Result<OnionPacket> OnionPacket::Deserialize(const std::vector<uint8_t>& data) {
    if (data.size() < 1 + 33 + 4 + 32) {  // version + pubkey + len + hmac
        return Result<OnionPacket>::Error("Invalid onion packet size");
    }

    OnionPacket packet;
    size_t offset = 0;

    packet.version = data[offset++];
    packet.public_key.assign(data.begin() + offset, data.begin() + offset + 33);
    offset += 33;

    // Read hops_data length
    uint32_t len = 0;
    for (int i = 0; i < 4; i++) {
        len = (len << 8) | data[offset++];
    }

    if (offset + len + 32 > data.size()) {
        return Result<OnionPacket>::Error("Invalid hops data length");
    }

    packet.hops_data.assign(data.begin() + offset, data.begin() + offset + len);
    offset += len;

    packet.hmac.assign(data.begin() + offset, data.begin() + offset + 32);

    return Result<OnionPacket>::Ok(packet);
}

WatchtowerTask::WatchtowerTask() : watch_until_height(0) {}
Watchtower::Watchtower(Blockchain* blockchain) : blockchain_(blockchain) {}
void Watchtower::WatchChannel(const uint256& channel_id, const WatchtowerTask& task) {
    std::lock_guard<std::mutex> lock(mutex_); tasks_[channel_id].push_back(task);
}
void Watchtower::UnwatchChannel(const uint256& channel_id) {
    std::lock_guard<std::mutex> lock(mutex_); tasks_.erase(channel_id);
}
// Enhanced Watchtower breach detection
void Watchtower::CheckForBreaches() {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!blockchain_) {
        return;
    }

    uint64_t current_height = blockchain_->GetBestHeight();

    // Check each watched channel for breaches
    for (auto& [channel_id, task_list] : tasks_) {
        // Remove expired tasks
        task_list.erase(
            std::remove_if(task_list.begin(), task_list.end(),
                [current_height](const WatchtowerTask& task) {
                    return task.watch_until_height < current_height;
                }),
            task_list.end()
        );

        // Check for revoked commitment transactions in recent blocks
        // In a full implementation, would scan mempool and recent blocks
        for (const auto& task : task_list) {
            // Check if revoked commitment transaction was broadcast
            auto tx_result = blockchain_->GetTransaction(task.revoked_commitment_txid);
            if (tx_result.IsOk()) {
                // Breach detected! Broadcast penalty transaction
                // Note: In production, would verify the transaction is actually
                // a revoked commitment before broadcasting penalty
                BroadcastPenalty(channel_id);
            }
        }
    }
}

Result<void> Watchtower::BroadcastPenalty(const uint256& channel_id) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = tasks_.find(channel_id);
    if (it == tasks_.end() || it->second.empty()) {
        return Result<void>::Error("No penalty task found for channel");
    }

    // Get the penalty transaction
    const auto& task = it->second.front();
    (void)task;  // Suppress unused warning

    // In a full implementation, would:
    // 1. Verify the breach occurred
    // 2. Sign the penalty transaction
    // 3. Broadcast to network via blockchain_->BroadcastTransaction(task.penalty_tx)
    // 4. Monitor for confirmation

    // For now, just return success
    return Result<void>::Ok();
}

LightningNetwork::LightningNetwork(Blockchain* blockchain, P2PNode* p2p)
    : blockchain_(blockchain), p2p_(p2p), running_(false) {}
LightningNetwork::~LightningNetwork() { Stop(); }
Result<void> LightningNetwork::Start(const PublicKey& node_id, const SecretKey& node_key) {
    node_id_ = node_id; node_key_ = node_key; running_ = true;
    watchtower_ = std::make_unique<Watchtower>(blockchain_); stats_ = Stats{};
    return Result<void>::Ok();
}
void LightningNetwork::Stop() {
    running_ = false; std::lock_guard<std::mutex> lock(mutex_); channels_.clear();
}
bool LightningNetwork::IsRunning() const { return running_; }
PublicKey LightningNetwork::GetNodeId() const { return node_id_; }
std::string LightningNetwork::GetNodeAlias() const { return node_alias_; }
void LightningNetwork::SetNodeAlias(const std::string& alias) { node_alias_ = alias; }
Result<uint256> LightningNetwork::OpenChannel(const PublicKey& remote_node, uint64_t capacity, uint64_t) {
    auto channel = std::make_shared<Channel>(node_id_, remote_node, capacity);
    std::lock_guard<std::mutex> lock(mutex_); channels_[channel->temporary_id] = channel;
    return Result<uint256>::Ok(channel->temporary_id);
}
Result<void> LightningNetwork::CloseChannel(const uint256&, bool) { return Result<void>::Ok(); }
std::vector<Channel> LightningNetwork::ListChannels() const {
    std::lock_guard<std::mutex> lock(mutex_); std::vector<Channel> result;
    for (const auto& [id, channel] : channels_) result.push_back(*channel);
    return result;
}
Result<Channel> LightningNetwork::GetChannel(const uint256& channel_id) const {
    std::lock_guard<std::mutex> lock(mutex_); auto it = channels_.find(channel_id);
    if (it == channels_.end()) return Result<Channel>::Error("Channel not found");
    return Result<Channel>::Ok(*it->second);
}
Result<uint256> LightningNetwork::SendPayment(const std::string&) {
    return Result<uint256>::Ok(RandomGenerator::GetRandomUint256());
}
Result<uint256> LightningNetwork::SendPayment(const PublicKey&, uint64_t, const std::string&) {
    return Result<uint256>::Ok(RandomGenerator::GetRandomUint256());
}
Result<Invoice> LightningNetwork::CreateInvoice(uint64_t amount, const std::string& description) {
    return Result<Invoice>::Ok(Invoice(amount, description, node_id_));
}
Result<PaymentRoute> LightningNetwork::FindRoute(const PublicKey& dest, uint64_t amount) const {
    return network_graph_.FindRoute(node_id_, dest, amount);
}
NetworkGraph& LightningNetwork::GetNetworkGraph() { return network_graph_; }
const NetworkGraph& LightningNetwork::GetNetworkGraph() const { return network_graph_; }
LightningNetwork::Stats LightningNetwork::GetStats() const {
    std::lock_guard<std::mutex> lock(mutex_); return stats_;
}
void LightningNetwork::HandleMessage(const PublicKey&, uint16_t, const std::vector<uint8_t>&) {}
void LightningNetwork::HandleOpenChannel(const PublicKey&, const std::vector<uint8_t>&) {}
void LightningNetwork::HandleAcceptChannel(const PublicKey&, const std::vector<uint8_t>&) {}
void LightningNetwork::HandleFundingCreated(const PublicKey&, const std::vector<uint8_t>&) {}
void LightningNetwork::HandleFundingSigned(const PublicKey&, const std::vector<uint8_t>&) {}
void LightningNetwork::HandleFundingLocked(const PublicKey&, const std::vector<uint8_t>&) {}
void LightningNetwork::HandleUpdateAddHTLC(const PublicKey&, const std::vector<uint8_t>&) {}
void LightningNetwork::HandleUpdateFulfillHTLC(const PublicKey&, const std::vector<uint8_t>&) {}
void LightningNetwork::HandleUpdateFailHTLC(const PublicKey&, const std::vector<uint8_t>&) {}
void LightningNetwork::HandleCommitmentSigned(const PublicKey&, const std::vector<uint8_t>&) {}
void LightningNetwork::HandleRevokeAndAck(const PublicKey&, const std::vector<uint8_t>&) {}
Result<std::shared_ptr<Channel>> LightningNetwork::FindChannelByPeer(const PublicKey&) {
    return Result<std::shared_ptr<Channel>>::Error("Not found");
}
Result<void> LightningNetwork::SendMessage(const PublicKey&, uint16_t, const std::vector<uint8_t>&) {
    (void)p2p_;  // TODO: Send message via P2P network
    return Result<void>::Ok();
}
void LightningNetwork::UpdateStats() {
    stats_ = Stats{}; stats_.num_channels = channels_.size();
}

} // namespace intcoin
