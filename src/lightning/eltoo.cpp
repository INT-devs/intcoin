// Copyright (c) 2025 INTcoin Core (Maddison Lane)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "intcoin/eltoo.h"
#include <random>
#include <chrono>
#include <algorithm>

namespace intcoin {
namespace eltoo {

//=============================================================================
// EltooUpdate Serialization
//=============================================================================

std::vector<uint8_t> EltooUpdate::serialize() const {
    std::vector<uint8_t> result;

    // Update number
    result.insert(result.end(),
                 reinterpret_cast<const uint8_t*>(&update_number),
                 reinterpret_cast<const uint8_t*>(&update_number) + sizeof(update_number));

    // Balances
    result.insert(result.end(),
                 reinterpret_cast<const uint8_t*>(&party_a_balance_sat),
                 reinterpret_cast<const uint8_t*>(&party_a_balance_sat) + sizeof(party_a_balance_sat));
    result.insert(result.end(),
                 reinterpret_cast<const uint8_t*>(&party_b_balance_sat),
                 reinterpret_cast<const uint8_t*>(&party_b_balance_sat) + sizeof(party_b_balance_sat));

    // Public keys
    auto pka_bytes = party_a_pubkey.serialize();
    result.insert(result.end(), pka_bytes.begin(), pka_bytes.end());
    auto pkb_bytes = party_b_pubkey.serialize();
    result.insert(result.end(), pkb_bytes.begin(), pkb_bytes.end());

    // Transactions
    auto update_bytes = update_tx.serialize();
    uint32_t update_size = update_bytes.size();
    result.insert(result.end(),
                 reinterpret_cast<const uint8_t*>(&update_size),
                 reinterpret_cast<const uint8_t*>(&update_size) + sizeof(update_size));
    result.insert(result.end(), update_bytes.begin(), update_bytes.end());

    auto settlement_bytes = settlement_tx.serialize();
    uint32_t settlement_size = settlement_bytes.size();
    result.insert(result.end(),
                 reinterpret_cast<const uint8_t*>(&settlement_size),
                 reinterpret_cast<const uint8_t*>(&settlement_size) + sizeof(settlement_size));
    result.insert(result.end(), settlement_bytes.begin(), settlement_bytes.end());

    // Settlement delay
    result.insert(result.end(),
                 reinterpret_cast<const uint8_t*>(&settlement_delay),
                 reinterpret_cast<const uint8_t*>(&settlement_delay) + sizeof(settlement_delay));

    // Signatures
    auto sig_a_bytes = party_a_sig.serialize();
    result.insert(result.end(), sig_a_bytes.begin(), sig_a_bytes.end());
    auto sig_b_bytes = party_b_sig.serialize();
    result.insert(result.end(), sig_b_bytes.begin(), sig_b_bytes.end());

    // Timestamp
    result.insert(result.end(),
                 reinterpret_cast<const uint8_t*>(&timestamp),
                 reinterpret_cast<const uint8_t*>(&timestamp) + sizeof(timestamp));

    return result;
}

EltooUpdate EltooUpdate::deserialize(const std::vector<uint8_t>& data) {
    EltooUpdate update;
    size_t offset = 0;

    // Update number
    std::memcpy(&update.update_number, &data[offset], sizeof(update.update_number));
    offset += sizeof(update.update_number);

    // Balances
    std::memcpy(&update.party_a_balance_sat, &data[offset], sizeof(update.party_a_balance_sat));
    offset += sizeof(update.party_a_balance_sat);
    std::memcpy(&update.party_b_balance_sat, &data[offset], sizeof(update.party_b_balance_sat));
    offset += sizeof(update.party_b_balance_sat);

    // Public keys (skip for now)
    offset += 1952 * 2;  // Dilithium5 pubkey size

    // Transactions
    uint32_t update_size;
    std::memcpy(&update_size, &data[offset], sizeof(update_size));
    offset += sizeof(update_size);
    std::vector<uint8_t> update_bytes(data.begin() + offset, data.begin() + offset + update_size);
    update.update_tx = Transaction::deserialize(update_bytes);
    offset += update_size;

    uint32_t settlement_size;
    std::memcpy(&settlement_size, &data[offset], sizeof(settlement_size));
    offset += sizeof(settlement_size);
    std::vector<uint8_t> settlement_bytes(data.begin() + offset, data.begin() + offset + settlement_size);
    update.settlement_tx = Transaction::deserialize(settlement_bytes);
    offset += settlement_size;

    // Settlement delay
    std::memcpy(&update.settlement_delay, &data[offset], sizeof(update.settlement_delay));
    offset += sizeof(update.settlement_delay);

    // Signatures (skip for now)
    offset += 4595 * 2;  // Dilithium5 signature size

    // Timestamp
    std::memcpy(&update.timestamp, &data[offset], sizeof(update.timestamp));
    offset += sizeof(update.timestamp);

    return update;
}

//=============================================================================
// EltooChannel Serialization
//=============================================================================

std::vector<uint8_t> EltooChannel::serialize() const {
    std::vector<uint8_t> result;

    // Channel ID
    result.insert(result.end(), channel_id.data.begin(), channel_id.data.end());

    // State
    result.push_back(static_cast<uint8_t>(state));

    // Public keys
    auto local_pk_bytes = local_pubkey.serialize();
    result.insert(result.end(), local_pk_bytes.begin(), local_pk_bytes.end());
    auto remote_pk_bytes = remote_pubkey.serialize();
    result.insert(result.end(), remote_pk_bytes.begin(), remote_pk_bytes.end());

    // Funding
    auto funding_bytes = funding_tx.serialize();
    uint32_t funding_size = funding_bytes.size();
    result.insert(result.end(),
                 reinterpret_cast<const uint8_t*>(&funding_size),
                 reinterpret_cast<const uint8_t*>(&funding_size) + sizeof(funding_size));
    result.insert(result.end(), funding_bytes.begin(), funding_bytes.end());

    result.insert(result.end(),
                 reinterpret_cast<const uint8_t*>(&funding_amount_sat),
                 reinterpret_cast<const uint8_t*>(&funding_amount_sat) + sizeof(funding_amount_sat));
    result.insert(result.end(),
                 reinterpret_cast<const uint8_t*>(&funding_confirmation_height),
                 reinterpret_cast<const uint8_t*>(&funding_confirmation_height) + sizeof(funding_confirmation_height));

    // Current state
    result.insert(result.end(),
                 reinterpret_cast<const uint8_t*>(&current_update_number),
                 reinterpret_cast<const uint8_t*>(&current_update_number) + sizeof(current_update_number));
    result.insert(result.end(),
                 reinterpret_cast<const uint8_t*>(&local_balance_sat),
                 reinterpret_cast<const uint8_t*>(&local_balance_sat) + sizeof(local_balance_sat));
    result.insert(result.end(),
                 reinterpret_cast<const uint8_t*>(&remote_balance_sat),
                 reinterpret_cast<const uint8_t*>(&remote_balance_sat) + sizeof(remote_balance_sat));

    // Updates
    uint32_t update_count = recent_updates.size();
    result.insert(result.end(),
                 reinterpret_cast<const uint8_t*>(&update_count),
                 reinterpret_cast<const uint8_t*>(&update_count) + sizeof(update_count));

    for (const auto& update : recent_updates) {
        auto update_bytes = update.serialize();
        uint32_t size = update_bytes.size();
        result.insert(result.end(),
                     reinterpret_cast<const uint8_t*>(&size),
                     reinterpret_cast<const uint8_t*>(&size) + sizeof(size));
        result.insert(result.end(), update_bytes.begin(), update_bytes.end());
    }

    // Parameters
    result.insert(result.end(),
                 reinterpret_cast<const uint8_t*>(&settlement_delay_blocks),
                 reinterpret_cast<const uint8_t*>(&settlement_delay_blocks) + sizeof(settlement_delay_blocks));
    result.insert(result.end(),
                 reinterpret_cast<const uint8_t*>(&dust_limit_sat),
                 reinterpret_cast<const uint8_t*>(&dust_limit_sat) + sizeof(dust_limit_sat));

    // Timestamps
    result.insert(result.end(),
                 reinterpret_cast<const uint8_t*>(&created_at),
                 reinterpret_cast<const uint8_t*>(&created_at) + sizeof(created_at));
    result.insert(result.end(),
                 reinterpret_cast<const uint8_t*>(&closed_at),
                 reinterpret_cast<const uint8_t*>(&closed_at) + sizeof(closed_at));

    return result;
}

EltooChannel EltooChannel::deserialize(const std::vector<uint8_t>& data) {
    EltooChannel channel;
    size_t offset = 0;

    // Channel ID
    std::copy(data.begin() + offset, data.begin() + offset + 32, channel.channel_id.data.begin());
    offset += 32;

    // State
    channel.state = static_cast<EltooChannelState>(data[offset++]);

    // Skip public keys
    offset += 1952 * 2;

    // Funding
    uint32_t funding_size;
    std::memcpy(&funding_size, &data[offset], sizeof(funding_size));
    offset += sizeof(funding_size);
    std::vector<uint8_t> funding_bytes(data.begin() + offset, data.begin() + offset + funding_size);
    channel.funding_tx = Transaction::deserialize(funding_bytes);
    offset += funding_size;

    std::memcpy(&channel.funding_amount_sat, &data[offset], sizeof(channel.funding_amount_sat));
    offset += sizeof(channel.funding_amount_sat);
    std::memcpy(&channel.funding_confirmation_height, &data[offset], sizeof(channel.funding_confirmation_height));
    offset += sizeof(channel.funding_confirmation_height);

    // Current state
    std::memcpy(&channel.current_update_number, &data[offset], sizeof(channel.current_update_number));
    offset += sizeof(channel.current_update_number);
    std::memcpy(&channel.local_balance_sat, &data[offset], sizeof(channel.local_balance_sat));
    offset += sizeof(channel.local_balance_sat);
    std::memcpy(&channel.remote_balance_sat, &data[offset], sizeof(channel.remote_balance_sat));
    offset += sizeof(channel.remote_balance_sat);

    // Updates
    uint32_t update_count;
    std::memcpy(&update_count, &data[offset], sizeof(update_count));
    offset += sizeof(update_count);

    for (uint32_t i = 0; i < update_count; i++) {
        uint32_t size;
        std::memcpy(&size, &data[offset], sizeof(size));
        offset += sizeof(size);
        std::vector<uint8_t> update_bytes(data.begin() + offset, data.begin() + offset + size);
        channel.recent_updates.push_back(EltooUpdate::deserialize(update_bytes));
        offset += size;
    }

    // Parameters
    std::memcpy(&channel.settlement_delay_blocks, &data[offset], sizeof(channel.settlement_delay_blocks));
    offset += sizeof(channel.settlement_delay_blocks);
    std::memcpy(&channel.dust_limit_sat, &data[offset], sizeof(channel.dust_limit_sat));
    offset += sizeof(channel.dust_limit_sat);

    // Timestamps
    std::memcpy(&channel.created_at, &data[offset], sizeof(channel.created_at));
    offset += sizeof(channel.created_at);
    std::memcpy(&channel.closed_at, &data[offset], sizeof(channel.closed_at));
    offset += sizeof(channel.closed_at);

    return channel;
}

//=============================================================================
// EltooChannelManager Implementation
//=============================================================================

EltooChannelManager::EltooChannelManager()
    : default_settlement_delay_(144),
      max_stored_updates_(10),
      current_height_(0) {
}

std::optional<Hash256> EltooChannelManager::open_channel(
    const DilithiumPubKey& peer_pubkey,
    uint64_t local_funding,
    uint64_t remote_funding,
    uint32_t settlement_delay) {

    std::lock_guard<std::mutex> lock(mutex_);

    EltooChannel channel;
    channel.channel_id = generate_channel_id();
    channel.state = EltooChannelState::INITIALIZING;
    channel.remote_pubkey = peer_pubkey;
    channel.funding_amount_sat = local_funding + remote_funding;
    channel.local_balance_sat = local_funding;
    channel.remote_balance_sat = remote_funding;
    channel.settlement_delay_blocks = settlement_delay;
    channel.current_update_number = 0;
    channel.created_at = current_height_;

    // Create funding transaction
    channel.funding_tx = create_funding_transaction(channel);

    channels_[channel.channel_id] = channel;

    return channel.channel_id;
}

bool EltooChannelManager::accept_channel(
    const Hash256& channel_id,
    uint64_t remote_funding) {

    std::lock_guard<std::mutex> lock(mutex_);

    auto it = channels_.find(channel_id);
    if (it == channels_.end()) {
        return false;
    }

    EltooChannel& channel = it->second;
    channel.remote_balance_sat = remote_funding;
    channel.funding_amount_sat += remote_funding;

    return true;
}

bool EltooChannelManager::confirm_funding(
    const Hash256& channel_id,
    uint32_t confirmation_height) {

    std::lock_guard<std::mutex> lock(mutex_);

    auto it = channels_.find(channel_id);
    if (it == channels_.end()) {
        return false;
    }

    EltooChannel& channel = it->second;
    channel.funding_confirmation_height = confirmation_height;
    channel.state = EltooChannelState::OPEN;

    return true;
}

std::optional<EltooUpdate> EltooChannelManager::create_update(
    const Hash256& channel_id,
    uint64_t new_local_balance,
    uint64_t new_remote_balance) {

    std::lock_guard<std::mutex> lock(mutex_);

    auto it = channels_.find(channel_id);
    if (it == channels_.end()) {
        return std::nullopt;
    }

    EltooChannel& channel = it->second;

    if (channel.state != EltooChannelState::OPEN) {
        return std::nullopt;
    }

    // Validate balances
    if (new_local_balance + new_remote_balance != channel.funding_amount_sat) {
        return std::nullopt;
    }

    // Create new update
    EltooUpdate update;
    update.update_number = channel.current_update_number + 1;
    update.party_a_balance_sat = new_local_balance;
    update.party_b_balance_sat = new_remote_balance;
    update.party_a_pubkey = channel.local_pubkey;
    update.party_b_pubkey = channel.remote_pubkey;
    update.settlement_delay = channel.settlement_delay_blocks;

    // Create update transaction (uses SIGHASH_NOINPUT)
    update.update_tx = create_update_transaction(
        channel,
        update.update_number,
        new_local_balance,
        new_remote_balance
    );

    // Create settlement transaction
    update.settlement_tx = create_settlement_transaction(channel, update);

    auto now = std::chrono::system_clock::now();
    update.timestamp = std::chrono::system_clock::to_time_t(now);

    return update;
}

bool EltooChannelManager::sign_update(
    const Hash256& channel_id,
    uint32_t update_number,
    const DilithiumSignature& signature) {

    std::lock_guard<std::mutex> lock(mutex_);

    auto it = channels_.find(channel_id);
    if (it == channels_.end()) {
        return false;
    }

    EltooChannel& channel = it->second;

    // Find update in recent updates
    auto update_it = std::find_if(
        channel.recent_updates.begin(),
        channel.recent_updates.end(),
        [update_number](const EltooUpdate& u) {
            return u.update_number == update_number;
        }
    );

    if (update_it != channel.recent_updates.end()) {
        update_it->party_b_sig = signature;
        return true;
    }

    return false;
}

bool EltooChannelManager::apply_update(
    const Hash256& channel_id,
    const EltooUpdate& update) {

    std::lock_guard<std::mutex> lock(mutex_);

    auto it = channels_.find(channel_id);
    if (it == channels_.end()) {
        return false;
    }

    EltooChannel& channel = it->second;

    if (!validate_update(channel, update)) {
        return false;
    }

    // Apply update
    channel.current_update_number = update.update_number;
    channel.local_balance_sat = update.party_a_balance_sat;
    channel.remote_balance_sat = update.party_b_balance_sat;

    // Store update
    channel.add_update(update);

    return true;
}

Hash256 EltooChannelManager::get_update_sighash(
    const EltooUpdate& update,
    SigHashType sighash_type) const {

    // Compute signing hash using SIGHASH_NOINPUT
    // This allows the signature to be valid for any previous update
    std::vector<uint8_t> script_code;  // Empty for SIGHASH_NOINPUT

    return compute_sighash_noinput(
        update.update_tx,
        0,  // input index
        script_code,
        update.get_capacity(),
        sighash_type
    );
}

bool EltooChannelManager::send_payment(
    const Hash256& channel_id,
    uint64_t amount_sat) {

    std::lock_guard<std::mutex> lock(mutex_);

    auto it = channels_.find(channel_id);
    if (it == channels_.end()) {
        return false;
    }

    EltooChannel& channel = it->second;

    if (channel.local_balance_sat < amount_sat) {
        return false;
    }

    // Create new update with transferred amount
    uint64_t new_local = channel.local_balance_sat - amount_sat;
    uint64_t new_remote = channel.remote_balance_sat + amount_sat;

    auto update = create_update(channel_id, new_local, new_remote);
    if (!update.has_value()) {
        return false;
    }

    return apply_update(channel_id, *update);
}

bool EltooChannelManager::receive_payment(
    const Hash256& channel_id,
    uint64_t amount_sat) {

    std::lock_guard<std::mutex> lock(mutex_);

    auto it = channels_.find(channel_id);
    if (it == channels_.end()) {
        return false;
    }

    EltooChannel& channel = it->second;

    if (channel.remote_balance_sat < amount_sat) {
        return false;
    }

    // Create new update with received amount
    uint64_t new_local = channel.local_balance_sat + amount_sat;
    uint64_t new_remote = channel.remote_balance_sat - amount_sat;

    auto update = create_update(channel_id, new_local, new_remote);
    if (!update.has_value()) {
        return false;
    }

    return apply_update(channel_id, *update);
}

bool EltooChannelManager::close_channel_cooperative(const Hash256& channel_id) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = channels_.find(channel_id);
    if (it == channels_.end()) {
        return false;
    }

    EltooChannel& channel = it->second;
    channel.state = EltooChannelState::CLOSING;

    // Create and broadcast cooperative close transaction
    auto close_tx = create_cooperative_close_transaction(channel);
    if (!broadcast_transaction(close_tx)) {
        return false;
    }

    channel.state = EltooChannelState::CLOSED;
    channel.closed_at = current_height_;

    return true;
}

bool EltooChannelManager::close_channel_force(const Hash256& channel_id) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = channels_.find(channel_id);
    if (it == channels_.end()) {
        return false;
    }

    EltooChannel& channel = it->second;

    auto latest = channel.get_latest_update();
    if (!latest.has_value()) {
        return false;
    }

    // Broadcast latest update transaction to network
    auto update_tx = create_update_transaction(channel, latest->update_number,
                                               latest->party_a_balance_sat,
                                               latest->party_b_balance_sat);
    if (!broadcast_transaction(update_tx)) {
        return false;
    }

    channel.state = EltooChannelState::FORCE_CLOSING;

    return true;
}

bool EltooChannelManager::broadcast_settlement(const Hash256& channel_id) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = channels_.find(channel_id);
    if (it == channels_.end()) {
        return false;
    }

    EltooChannel& channel = it->second;

    if (channel.state != EltooChannelState::FORCE_CLOSING) {
        return false;
    }

    auto latest = channel.get_latest_update();
    if (!latest.has_value()) {
        return false;
    }

    // Check if settlement delay (CSV) has passed
    uint32_t update_height = channel.updates.back().created_at_height;
    if (current_height_ < update_height + SETTLEMENT_DELAY_BLOCKS) {
        return false;  // Timelock not yet expired
    }

    // Broadcast settlement transaction
    auto settlement_tx = create_settlement_transaction(channel, *latest);
    if (!broadcast_transaction(settlement_tx)) {
        return false;
    }

    channel.state = EltooChannelState::CLOSED;
    channel.closed_at = current_height_;

    return true;
}

std::optional<EltooChannel> EltooChannelManager::get_channel(const Hash256& channel_id) const {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = channels_.find(channel_id);
    if (it == channels_.end()) {
        return std::nullopt;
    }

    return it->second;
}

std::vector<EltooChannel> EltooChannelManager::list_channels() const {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<EltooChannel> result;
    result.reserve(channels_.size());

    for (const auto& [id, channel] : channels_) {
        result.push_back(channel);
    }

    return result;
}

std::vector<EltooChannel> EltooChannelManager::list_channels_by_state(EltooChannelState state) const {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<EltooChannel> result;

    for (const auto& [id, channel] : channels_) {
        if (channel.state == state) {
            result.push_back(channel);
        }
    }

    return result;
}

std::pair<uint64_t, uint64_t> EltooChannelManager::get_channel_balance(const Hash256& channel_id) const {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = channels_.find(channel_id);
    if (it == channels_.end()) {
        return {0, 0};
    }

    return {it->second.local_balance_sat, it->second.remote_balance_sat};
}

EltooChannelManager::EltooStats EltooChannelManager::get_stats() const {
    std::lock_guard<std::mutex> lock(mutex_);

    EltooStats stats{};
    stats.total_channels = channels_.size();

    uint64_t total_updates = 0;

    for (const auto& [id, channel] : channels_) {
        if (channel.state == EltooChannelState::OPEN) {
            stats.open_channels++;
        }

        stats.total_capacity_sat += channel.funding_amount_sat;
        stats.total_local_balance_sat += channel.local_balance_sat;
        stats.total_remote_balance_sat += channel.remote_balance_sat;

        total_updates += channel.current_update_number;
    }

    if (stats.total_channels > 0) {
        stats.avg_updates_per_channel = static_cast<double>(total_updates) / stats.total_channels;
    }

    stats.total_updates_created = total_updates;

    return stats;
}

void EltooChannelManager::set_default_settlement_delay(uint32_t blocks) {
    std::lock_guard<std::mutex> lock(mutex_);
    default_settlement_delay_ = blocks;
}

void EltooChannelManager::set_max_stored_updates(size_t max_updates) {
    std::lock_guard<std::mutex> lock(mutex_);
    max_stored_updates_ = max_updates;
}

//=============================================================================
// Private Helper Methods
//=============================================================================

Hash256 EltooChannelManager::generate_channel_id() const {
    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_int_distribution<uint64_t> dis;

    Hash256 id;
    for (size_t i = 0; i < 4; i++) {
        uint64_t val = dis(gen);
        std::memcpy(&id.data[i * 8], &val, 8);
    }

    return id;
}

Transaction EltooChannelManager::create_funding_transaction(const EltooChannel& channel) const {
    Transaction tx;
    tx.version = 1;
    tx.locktime = 0;

    // Create 2-of-2 multisig funding output
    TxOutput funding_output;
    funding_output.amount = channel.funding_amount_sat;
    funding_output.script = create_2of2_multisig_script(
        channel.party_a_pubkey, channel.party_b_pubkey);
    tx.outputs.push_back(funding_output);

    return tx;
}

Transaction EltooChannelManager::create_update_transaction(
    const EltooChannel& channel,
    uint32_t update_number,
    uint64_t party_a_balance,
    uint64_t party_b_balance) const {

    Transaction tx;
    tx.version = 1;
    tx.locktime = 0;

    // Update transactions use SIGHASH_ANYPREVOUT (BIP-118)
    // They can spend any previous update or the funding output
    TxInput input;
    input.prev_txid = Hash256{};  // ANYPREVOUT - can be any previous output
    input.prev_index = 0;
    input.sequence = 0xFFFFFFFE;  // Enable RBF
    tx.inputs.push_back(input);

    // Single output to update script with CSV delay
    TxOutput update_output;
    update_output.amount = party_a_balance + party_b_balance;
    update_output.script = create_eltoo_update_script(
        channel.party_a_pubkey, channel.party_b_pubkey, update_number);
    tx.outputs.push_back(update_output);

    return tx;
}

Transaction EltooChannelManager::create_settlement_transaction(
    const EltooChannel& channel,
    const EltooUpdate& update) const {

    Transaction tx;
    tx.version = 1;
    tx.locktime = 0;

    // Settlement transaction spends update output after CSV delay
    // Pays out final balances to both parties
    TxInput input;
    input.prev_txid = Hash256{};  // Will be filled with update txid
    input.prev_index = 0;
    input.sequence = SETTLEMENT_DELAY_BLOCKS;  // CSV relative timelock
    tx.inputs.push_back(input);

    // Output to party A
    TxOutput output_a;
    output_a.amount = update.party_a_balance_sat;
    output_a.script = create_p2pkh_script(channel.party_a_pubkey);
    tx.outputs.push_back(output_a);

    // Output to party B
    TxOutput output_b;
    output_b.amount = update.party_b_balance_sat;
    output_b.script = create_p2pkh_script(channel.party_b_pubkey);
    tx.outputs.push_back(output_b);

    return tx;
}

bool EltooChannelManager::validate_update(
    const EltooChannel& channel,
    const EltooUpdate& update) const {

    // Update number must be greater than current
    if (update.update_number <= channel.current_update_number) {
        return false;
    }

    // Balances must sum to capacity
    if (update.party_a_balance_sat + update.party_b_balance_sat != channel.funding_amount_sat) {
        return false;
    }

    // Verify both party signatures on the update
    auto update_hash = compute_update_hash(update);
    if (!verify_dilithium_signature(update.party_a_sig, update_hash, channel.party_a_pubkey)) {
        return false;
    }
    if (!verify_dilithium_signature(update.party_b_sig, update_hash, channel.party_b_pubkey)) {
        return false;
    }

    return true;
}

Hash256 EltooChannelManager::compute_sighash_noinput(
    const Transaction& tx,
    size_t input_index,
    const std::vector<uint8_t>& script_code,
    uint64_t amount,
    SigHashType sighash_type) const {

    std::vector<uint8_t> data;

    // SIGHASH_NOINPUT doesn't commit to:
    // - Previous output being spent (txid + vout)
    // - Sequence number
    // - scriptPubKey being spent

    // It DOES commit to:
    // - Transaction version
    data.insert(data.end(),
               reinterpret_cast<const uint8_t*>(&tx.version),
               reinterpret_cast<const uint8_t*>(&tx.version) + sizeof(tx.version));

    // - Outputs
    for (const auto& output : tx.outputs) {
        auto out_bytes = output.serialize();
        data.insert(data.end(), out_bytes.begin(), out_bytes.end());
    }

    // - Locktime
    data.insert(data.end(),
               reinterpret_cast<const uint8_t*>(&tx.locktime),
               reinterpret_cast<const uint8_t*>(&tx.locktime) + sizeof(tx.locktime));

    // - Sighash type
    uint8_t sighash_byte = static_cast<uint8_t>(sighash_type);
    data.push_back(sighash_byte);

    // Return SHA3-256 hash
    return sha3_256(data);
}

} // namespace eltoo
} // namespace intcoin
