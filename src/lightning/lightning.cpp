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
    uint32_t htlc_count = static_cast<uint32_t>(data[offset]) |
                          (static_cast<uint32_t>(data[offset + 1]) << 8) |
                          (static_cast<uint32_t>(data[offset + 2]) << 16) |
                          (static_cast<uint32_t>(data[offset + 3]) << 24);
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

bool Channel::open(const crypto::DilithiumPubKey& remote_key, uint64_t capacity) {
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

bool Channel::verify_remote_signature(const crypto::DilithiumSignature& sig) {
    if (!latest_commitment) {
        return false;
    }

    Hash256 commitment_hash = latest_commitment->get_hash();
    crypto::DilithiumPubKey remote_key = remote_pubkey;

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
    const crypto::DilithiumPubKey& remote_pubkey,
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

} // namespace messages

} // namespace lightning
} // namespace intcoin
