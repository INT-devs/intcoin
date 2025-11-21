// Copyright (c) 2025 INTcoin Core (Maddison Lane)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
//
// Atomic Swap Implementation

#include "intcoin/bridge/atomic_swap.h"
#include "intcoin/crypto/sha256.h"
#include "intcoin/crypto/random.h"
#include <algorithm>
#include <cstring>

namespace intcoin {
namespace bridge {

// ============================================================================
// HTLC Implementation
// ============================================================================

bool HTLC::verify_secret(const Hash256& preimage) const {
    // Hash the preimage and compare with hash_lock
    SHA256 hasher;
    Hash256 computed_hash;
    hasher.update(preimage.data(), preimage.size());
    hasher.finalize(computed_hash.data());

    return computed_hash == hash_lock;
}

bool HTLC::is_timelocked(uint32_t current_height) const {
    return current_height < time_lock;
}

// ============================================================================
// AtomicSwap Implementation
// ============================================================================

AtomicSwap::AtomicSwap()
    : initiator_amount_(0)
    , participant_amount_(0)
    , initiator_chain_(ChainType::INTCOIN)
    , participant_chain_(ChainType::BITCOIN)
    , state_(SwapState::INITIATED)
    , timelock_duration_(24 * 60)  // 24 hours in blocks (assuming 1 min blocks)
    , created_at_(std::chrono::system_clock::now())
{
}

AtomicSwap::~AtomicSwap() = default;

std::shared_ptr<AtomicSwap> AtomicSwap::initiate(
    const PublicKey& initiator,
    const PublicKey& participant,
    uint64_t initiator_amount,
    uint64_t participant_amount,
    ChainType initiator_chain,
    ChainType participant_chain,
    uint32_t timelock_duration)
{
    auto swap = std::make_shared<AtomicSwap>();

    // Generate secret and hash lock
    swap->secret_ = generate_secret();
    swap->hash_lock_ = hash_secret(swap->secret_);

    // Generate swap ID from hash lock + initiator pubkey
    SHA256 hasher;
    hasher.update(swap->hash_lock_.data(), swap->hash_lock_.size());
    hasher.update(initiator.data(), initiator.size());
    hasher.finalize(swap->swap_id_.data());

    swap->initiator_ = initiator;
    swap->participant_ = participant;
    swap->initiator_amount_ = initiator_amount;
    swap->participant_amount_ = participant_amount;
    swap->initiator_chain_ = initiator_chain;
    swap->participant_chain_ = participant_chain;
    swap->timelock_duration_ = timelock_duration;

    // Setup initiator HTLC
    swap->initiator_htlc_.hash_lock = swap->hash_lock_;
    swap->initiator_htlc_.sender = initiator;
    swap->initiator_htlc_.receiver = participant;
    swap->initiator_htlc_.amount = initiator_amount;
    swap->initiator_htlc_.time_lock = timelock_duration;
    swap->initiator_htlc_.chain = initiator_chain;
    swap->initiator_htlc_.secret_revealed = false;

    // Setup participant HTLC (same hash lock!)
    swap->participant_htlc_.hash_lock = swap->hash_lock_;
    swap->participant_htlc_.sender = participant;
    swap->participant_htlc_.receiver = initiator;
    swap->participant_htlc_.amount = participant_amount;
    swap->participant_htlc_.time_lock = timelock_duration / 2;  // Participant timeout is shorter
    swap->participant_htlc_.chain = participant_chain;
    swap->participant_htlc_.secret_revealed = false;

    swap->state_ = SwapState::INITIATED;

    return swap;
}

bool AtomicSwap::lock_funds(const Transaction& tx, ChainType chain) {
    if (state_ != SwapState::INITIATED && state_ != SwapState::LOCKED) {
        return false;
    }

    // Verify transaction and update HTLC state
    if (chain == initiator_chain_) {
        initiator_htlc_.chain_txid = tx.get_hash_string();

        // Check if both sides are locked
        if (!participant_htlc_.chain_txid.empty()) {
            state_ = SwapState::LOCKED;
        }
        return true;
    } else if (chain == participant_chain_) {
        participant_htlc_.chain_txid = tx.get_hash_string();

        // Check if both sides are locked
        if (!initiator_htlc_.chain_txid.empty()) {
            state_ = SwapState::LOCKED;
        }
        return true;
    }

    return false;
}

bool AtomicSwap::claim_funds(const Hash256& secret) {
    if (state_ != SwapState::LOCKED) {
        return false;
    }

    // Verify secret matches hash lock
    if (!initiator_htlc_.verify_secret(secret)) {
        return false;
    }

    // Reveal secret and mark as claimed
    initiator_htlc_.secret = secret;
    initiator_htlc_.secret_revealed = true;
    participant_htlc_.secret = secret;
    participant_htlc_.secret_revealed = true;

    state_ = SwapState::CLAIMED;
    return true;
}

bool AtomicSwap::refund_funds() {
    if (state_ != SwapState::LOCKED && state_ != SwapState::INITIATED) {
        return false;
    }

    state_ = SwapState::REFUNDED;
    return true;
}

std::optional<Hash256> AtomicSwap::get_secret() const {
    if (initiator_htlc_.secret_revealed) {
        return initiator_htlc_.secret;
    }
    return std::nullopt;
}

bool AtomicSwap::verify_initiator_lock() const {
    return !initiator_htlc_.chain_txid.empty();
}

bool AtomicSwap::verify_participant_lock() const {
    return !participant_htlc_.chain_txid.empty();
}

bool AtomicSwap::is_expired(uint32_t current_height) const {
    return initiator_htlc_.is_timelocked(current_height) == false;
}

std::vector<uint8_t> AtomicSwap::serialize() const {
    std::vector<uint8_t> data;

    // Serialize swap data
    data.insert(data.end(), swap_id_.begin(), swap_id_.end());
    data.insert(data.end(), hash_lock_.begin(), hash_lock_.end());
    data.insert(data.end(), secret_.begin(), secret_.end());
    data.insert(data.end(), initiator_.begin(), initiator_.end());
    data.insert(data.end(), participant_.begin(), participant_.end());

    // Amounts
    for (int i = 0; i < 8; ++i) {
        data.push_back((initiator_amount_ >> (i * 8)) & 0xFF);
    }
    for (int i = 0; i < 8; ++i) {
        data.push_back((participant_amount_ >> (i * 8)) & 0xFF);
    }

    // Chains
    data.push_back(static_cast<uint8_t>(initiator_chain_));
    data.push_back(static_cast<uint8_t>(participant_chain_));

    // State
    data.push_back(static_cast<uint8_t>(state_));

    // Timelock
    for (int i = 0; i < 4; ++i) {
        data.push_back((timelock_duration_ >> (i * 8)) & 0xFF);
    }

    return data;
}

std::optional<AtomicSwap> AtomicSwap::deserialize(const std::vector<uint8_t>& data) {
    if (data.size() < 32 + 32 + 32 + 32 + 32 + 8 + 8 + 1 + 1 + 1 + 4) {
        return std::nullopt;  // Not enough data
    }

    AtomicSwap swap;
    size_t offset = 0;

    // Deserialize swap ID
    std::copy(data.begin() + offset, data.begin() + offset + 32, swap.swap_id_.begin());
    offset += 32;

    // Hash lock
    std::copy(data.begin() + offset, data.begin() + offset + 32, swap.hash_lock_.begin());
    offset += 32;

    // Secret
    std::copy(data.begin() + offset, data.begin() + offset + 32, swap.secret_.begin());
    offset += 32;

    // Initiator
    std::copy(data.begin() + offset, data.begin() + offset + 32, swap.initiator_.begin());
    offset += 32;

    // Participant
    std::copy(data.begin() + offset, data.begin() + offset + 32, swap.participant_.begin());
    offset += 32;

    // Amounts
    swap.initiator_amount_ = 0;
    for (int i = 0; i < 8; ++i) {
        swap.initiator_amount_ |= static_cast<uint64_t>(data[offset + i]) << (i * 8);
    }
    offset += 8;

    swap.participant_amount_ = 0;
    for (int i = 0; i < 8; ++i) {
        swap.participant_amount_ |= static_cast<uint64_t>(data[offset + i]) << (i * 8);
    }
    offset += 8;

    // Chains
    swap.initiator_chain_ = static_cast<ChainType>(data[offset++]);
    swap.participant_chain_ = static_cast<ChainType>(data[offset++]);

    // State
    swap.state_ = static_cast<SwapState>(data[offset++]);

    // Timelock
    swap.timelock_duration_ = 0;
    for (int i = 0; i < 4; ++i) {
        swap.timelock_duration_ |= static_cast<uint32_t>(data[offset + i]) << (i * 8);
    }

    return swap;
}

Hash256 AtomicSwap::generate_secret() {
    Hash256 secret;
    crypto::get_random_bytes(secret.data(), secret.size());
    return secret;
}

Hash256 AtomicSwap::hash_secret(const Hash256& secret) {
    SHA256 hasher;
    Hash256 hash;
    hasher.update(secret.data(), secret.size());
    hasher.finalize(hash.data());
    return hash;
}

// ============================================================================
// AtomicSwapManager Implementation
// ============================================================================

AtomicSwapManager::AtomicSwapManager() = default;
AtomicSwapManager::~AtomicSwapManager() = default;

std::shared_ptr<AtomicSwap> AtomicSwapManager::create_swap(
    const PublicKey& participant,
    uint64_t send_amount,
    uint64_t receive_amount,
    ChainType send_chain,
    ChainType receive_chain,
    uint32_t timelock_duration)
{
    // Generate a dummy initiator key (in real implementation, use actual wallet key)
    PublicKey initiator;
    crypto::get_random_bytes(initiator.data(), initiator.size());

    auto swap = AtomicSwap::initiate(
        initiator,
        participant,
        send_amount,
        receive_amount,
        send_chain,
        receive_chain,
        timelock_duration
    );

    add_swap(swap);
    return swap;
}

bool AtomicSwapManager::add_swap(std::shared_ptr<AtomicSwap> swap) {
    std::lock_guard<std::mutex> lock(swaps_mutex_);

    Hash256 swap_id = swap->get_swap_id();
    if (swaps_.find(swap_id) != swaps_.end()) {
        return false;  // Swap already exists
    }

    swaps_[swap_id] = swap;
    return true;
}

std::shared_ptr<AtomicSwap> AtomicSwapManager::get_swap(const Hash256& swap_id) {
    std::lock_guard<std::mutex> lock(swaps_mutex_);

    auto it = swaps_.find(swap_id);
    if (it != swaps_.end()) {
        return it->second;
    }
    return nullptr;
}

bool AtomicSwapManager::remove_swap(const Hash256& swap_id) {
    std::lock_guard<std::mutex> lock(swaps_mutex_);
    return swaps_.erase(swap_id) > 0;
}

bool AtomicSwapManager::lock_funds(const Hash256& swap_id, const Transaction& tx, ChainType chain) {
    auto swap = get_swap(swap_id);
    if (!swap) {
        return false;
    }
    return swap->lock_funds(tx, chain);
}

bool AtomicSwapManager::claim_funds(const Hash256& swap_id, const Hash256& secret) {
    auto swap = get_swap(swap_id);
    if (!swap) {
        return false;
    }

    bool success = swap->claim_funds(secret);
    if (success && swap_completed_callback_) {
        swap_completed_callback_(swap_id);
    }
    return success;
}

bool AtomicSwapManager::refund_funds(const Hash256& swap_id) {
    auto swap = get_swap(swap_id);
    if (!swap) {
        return false;
    }
    return swap->refund_funds();
}

std::vector<std::shared_ptr<AtomicSwap>> AtomicSwapManager::get_all_swaps() const {
    std::lock_guard<std::mutex> lock(swaps_mutex_);

    std::vector<std::shared_ptr<AtomicSwap>> result;
    result.reserve(swaps_.size());

    for (const auto& [id, swap] : swaps_) {
        result.push_back(swap);
    }

    return result;
}

std::vector<std::shared_ptr<AtomicSwap>> AtomicSwapManager::get_swaps_by_state(SwapState state) const {
    std::lock_guard<std::mutex> lock(swaps_mutex_);

    std::vector<std::shared_ptr<AtomicSwap>> result;

    for (const auto& [id, swap] : swaps_) {
        if (swap->get_state() == state) {
            result.push_back(swap);
        }
    }

    return result;
}

std::vector<std::shared_ptr<AtomicSwap>> AtomicSwapManager::get_initiated_swaps() const {
    return get_swaps_by_state(SwapState::INITIATED);
}

std::vector<std::shared_ptr<AtomicSwap>> AtomicSwapManager::get_pending_swaps() const {
    auto initiated = get_swaps_by_state(SwapState::INITIATED);
    auto locked = get_swaps_by_state(SwapState::LOCKED);

    initiated.insert(initiated.end(), locked.begin(), locked.end());
    return initiated;
}

void AtomicSwapManager::monitor_swaps(uint32_t current_height) {
    std::lock_guard<std::mutex> lock(swaps_mutex_);

    for (auto& [id, swap] : swaps_) {
        if (swap->is_expired(current_height)) {
            if (swap->get_state() == SwapState::LOCKED ||
                swap->get_state() == SwapState::INITIATED) {

                // Auto-refund expired swaps
                swap->refund_funds();

                if (swap_expired_callback_) {
                    swap_expired_callback_(id);
                }
            }
        }
    }
}

std::vector<Hash256> AtomicSwapManager::get_expired_swaps(uint32_t current_height) const {
    std::lock_guard<std::mutex> lock(swaps_mutex_);

    std::vector<Hash256> expired;

    for (const auto& [id, swap] : swaps_) {
        if (swap->is_expired(current_height)) {
            expired.push_back(id);
        }
    }

    return expired;
}

void AtomicSwapManager::cleanup_completed_swaps(uint32_t max_age_seconds) {
    std::lock_guard<std::mutex> lock(swaps_mutex_);

    auto now = std::chrono::system_clock::now();
    std::vector<Hash256> to_remove;

    for (const auto& [id, swap] : swaps_) {
        SwapState state = swap->get_state();
        if (state == SwapState::CLAIMED ||
            state == SwapState::REFUNDED ||
            state == SwapState::CANCELLED) {

            // Check age (would need to store creation time in swap)
            to_remove.push_back(id);
        }
    }

    for (const auto& id : to_remove) {
        swaps_.erase(id);
    }
}

size_t AtomicSwapManager::get_swap_count() const {
    std::lock_guard<std::mutex> lock(swaps_mutex_);
    return swaps_.size();
}

size_t AtomicSwapManager::get_active_swap_count() const {
    std::lock_guard<std::mutex> lock(swaps_mutex_);

    size_t count = 0;
    for (const auto& [id, swap] : swaps_) {
        SwapState state = swap->get_state();
        if (state == SwapState::INITIATED || state == SwapState::LOCKED) {
            count++;
        }
    }

    return count;
}

uint64_t AtomicSwapManager::get_total_volume() const {
    std::lock_guard<std::mutex> lock(swaps_mutex_);

    uint64_t total = 0;
    for (const auto& [id, swap] : swaps_) {
        if (swap->get_state() == SwapState::CLAIMED) {
            total += swap->get_initiator_amount();
        }
    }

    return total;
}

void AtomicSwapManager::set_swap_completed_callback(
    std::function<void(const Hash256&)> callback)
{
    swap_completed_callback_ = callback;
}

void AtomicSwapManager::set_swap_expired_callback(
    std::function<void(const Hash256&)> callback)
{
    swap_expired_callback_ = callback;
}

// ============================================================================
// SwapBuilder Implementation
// ============================================================================

SwapBuilder::SwapBuilder()
    : send_amount_(0)
    , receive_amount_(0)
    , send_chain_(ChainType::INTCOIN)
    , receive_chain_(ChainType::BITCOIN)
    , timelock_(24 * 60)
{
}

SwapBuilder& SwapBuilder::initiator(const PublicKey& key) {
    initiator_ = key;
    return *this;
}

SwapBuilder& SwapBuilder::participant(const PublicKey& key) {
    participant_ = key;
    return *this;
}

SwapBuilder& SwapBuilder::send_amount(uint64_t amount) {
    send_amount_ = amount;
    return *this;
}

SwapBuilder& SwapBuilder::receive_amount(uint64_t amount) {
    receive_amount_ = amount;
    return *this;
}

SwapBuilder& SwapBuilder::send_chain(ChainType chain) {
    send_chain_ = chain;
    return *this;
}

SwapBuilder& SwapBuilder::receive_chain(ChainType chain) {
    receive_chain_ = chain;
    return *this;
}

SwapBuilder& SwapBuilder::timelock(uint32_t blocks) {
    timelock_ = blocks;
    return *this;
}

std::shared_ptr<AtomicSwap> SwapBuilder::build() {
    return AtomicSwap::initiate(
        initiator_,
        participant_,
        send_amount_,
        receive_amount_,
        send_chain_,
        receive_chain_,
        timelock_
    );
}

} // namespace bridge
} // namespace intcoin
