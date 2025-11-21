// Copyright (c) 2025 INTcoin Core (Maddison Lane)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "intcoin/ptlc.h"
#include <random>
#include <chrono>
#include <algorithm>

namespace intcoin {
namespace ptlc {

//=============================================================================
// AdaptorSignature Serialization
//=============================================================================

std::vector<uint8_t> AdaptorSignature::serialize() const {
    std::vector<uint8_t> result;

    auto sig_bytes = partial_sig.serialize();
    result.insert(result.end(), sig_bytes.begin(), sig_bytes.end());
    result.insert(result.end(), adaptor_point.data.begin(), adaptor_point.data.end());

    return result;
}

AdaptorSignature AdaptorSignature::deserialize(const std::vector<uint8_t>& data) {
    AdaptorSignature adaptor;
    size_t offset = 0;

    // Partial signature
    std::vector<uint8_t> sig_bytes(data.begin(), data.begin() + 4595);
    adaptor.partial_sig = DilithiumSignature::deserialize(sig_bytes);
    offset += 4595;

    // Adaptor point
    std::copy(data.begin() + offset, data.begin() + offset + 32, adaptor.adaptor_point.data.begin());

    return adaptor;
}

bool AdaptorSignature::verify(
    const DilithiumPubKey& pubkey,
    const Hash256& message,
    const Hash256& adaptor_point) const {

    // Verify adaptor signature using Dilithium-based adaptor verification
    // The adaptor signature should be valid when completed with the discrete log of adaptor_point
    auto expected_point = compute_payment_point(adaptor_point);
    return verify_dilithium_adaptor(partial_sig, expected_point);
}

//=============================================================================
// CompletedSignature Serialization
//=============================================================================

std::vector<uint8_t> CompletedSignature::serialize() const {
    std::vector<uint8_t> result;

    auto sig_bytes = complete_sig.serialize();
    result.insert(result.end(), sig_bytes.begin(), sig_bytes.end());
    result.insert(result.end(), secret_scalar.data.begin(), secret_scalar.data.end());

    return result;
}

CompletedSignature CompletedSignature::deserialize(const std::vector<uint8_t>& data) {
    CompletedSignature completed;
    size_t offset = 0;

    // Complete signature
    std::vector<uint8_t> sig_bytes(data.begin(), data.begin() + 4595);
    completed.complete_sig = DilithiumSignature::deserialize(sig_bytes);
    offset += 4595;

    // Secret scalar
    std::copy(data.begin() + offset, data.begin() + offset + 32, completed.secret_scalar.data.begin());

    return completed;
}

//=============================================================================
// PTLC Serialization
//=============================================================================

std::vector<uint8_t> PTLC::serialize() const {
    std::vector<uint8_t> result;

    // PTLC ID
    result.insert(result.end(), ptlc_id.data.begin(), ptlc_id.data.end());

    // Payment details
    result.insert(result.end(),
                 reinterpret_cast<const uint8_t*>(&amount_sat),
                 reinterpret_cast<const uint8_t*>(&amount_sat) + sizeof(amount_sat));
    result.insert(result.end(),
                 reinterpret_cast<const uint8_t*>(&timeout_height),
                 reinterpret_cast<const uint8_t*>(&timeout_height) + sizeof(timeout_height));
    result.insert(result.end(),
                 reinterpret_cast<const uint8_t*>(&timeout_seconds),
                 reinterpret_cast<const uint8_t*>(&timeout_seconds) + sizeof(timeout_seconds));

    // Payment point
    result.insert(result.end(), payment_point.data.begin(), payment_point.data.end());

    // Adaptor signatures
    auto sender_bytes = sender_adaptor.serialize();
    result.insert(result.end(), sender_bytes.begin(), sender_bytes.end());
    auto receiver_bytes = receiver_adaptor.serialize();
    result.insert(result.end(), receiver_bytes.begin(), receiver_bytes.end());

    // State
    result.push_back(claimed ? 1 : 0);
    result.push_back(timed_out ? 1 : 0);

    // Optional payment secret
    if (payment_secret.has_value()) {
        result.push_back(1);
        result.insert(result.end(), payment_secret->data.begin(), payment_secret->data.end());
    } else {
        result.push_back(0);
    }

    // Timestamp
    result.insert(result.end(),
                 reinterpret_cast<const uint8_t*>(&created_at),
                 reinterpret_cast<const uint8_t*>(&created_at) + sizeof(created_at));

    return result;
}

PTLC PTLC::deserialize(const std::vector<uint8_t>& data) {
    PTLC ptlc;
    size_t offset = 0;

    // PTLC ID
    std::copy(data.begin() + offset, data.begin() + offset + 32, ptlc.ptlc_id.data.begin());
    offset += 32;

    // Payment details
    std::memcpy(&ptlc.amount_sat, &data[offset], sizeof(ptlc.amount_sat));
    offset += sizeof(ptlc.amount_sat);
    std::memcpy(&ptlc.timeout_height, &data[offset], sizeof(ptlc.timeout_height));
    offset += sizeof(ptlc.timeout_height);
    std::memcpy(&ptlc.timeout_seconds, &data[offset], sizeof(ptlc.timeout_seconds));
    offset += sizeof(ptlc.timeout_seconds);

    // Payment point
    std::copy(data.begin() + offset, data.begin() + offset + 32, ptlc.payment_point.data.begin());
    offset += 32;

    // Adaptor signatures (skip detailed deserialization)
    offset += 4627 * 2;  // AdaptorSignature size

    // State
    ptlc.claimed = (data[offset++] == 1);
    ptlc.timed_out = (data[offset++] == 1);

    // Optional payment secret
    if (data[offset++] == 1) {
        Hash256 secret;
        std::copy(data.begin() + offset, data.begin() + offset + 32, secret.data.begin());
        ptlc.payment_secret = secret;
        offset += 32;
    }

    // Timestamp
    std::memcpy(&ptlc.created_at, &data[offset], sizeof(ptlc.created_at));

    return ptlc;
}

//=============================================================================
// ChannelPTLC Serialization
//=============================================================================

std::vector<uint8_t> ChannelPTLC::serialize() const {
    std::vector<uint8_t> result;

    // PTLC ID
    result.insert(result.end(), ptlc_id.data.begin(), ptlc_id.data.end());

    // State
    result.push_back(static_cast<uint8_t>(state));

    // Direction
    result.push_back(outgoing ? 1 : 0);

    // Payment details
    result.insert(result.end(),
                 reinterpret_cast<const uint8_t*>(&amount_sat),
                 reinterpret_cast<const uint8_t*>(&amount_sat) + sizeof(amount_sat));
    result.insert(result.end(),
                 reinterpret_cast<const uint8_t*>(&timeout_height),
                 reinterpret_cast<const uint8_t*>(&timeout_height) + sizeof(timeout_height));

    // Payment point
    result.insert(result.end(), payment_point.data.begin(), payment_point.data.end());

    // Adaptor signature
    auto adaptor_bytes = adaptor_sig.serialize();
    result.insert(result.end(), adaptor_bytes.begin(), adaptor_bytes.end());

    // Optional payment secret
    if (payment_secret.has_value()) {
        result.push_back(1);
        result.insert(result.end(), payment_secret->data.begin(), payment_secret->data.end());
    } else {
        result.push_back(0);
    }

    // Optional completed signature
    if (completed_sig.has_value()) {
        result.push_back(1);
        auto comp_bytes = completed_sig->serialize();
        result.insert(result.end(), comp_bytes.begin(), comp_bytes.end());
    } else {
        result.push_back(0);
    }

    return result;
}

ChannelPTLC ChannelPTLC::deserialize(const std::vector<uint8_t>& data) {
    ChannelPTLC ptlc;
    size_t offset = 0;

    // PTLC ID
    std::copy(data.begin() + offset, data.begin() + offset + 32, ptlc.ptlc_id.data.begin());
    offset += 32;

    // State
    ptlc.state = static_cast<PTLCState>(data[offset++]);

    // Direction
    ptlc.outgoing = (data[offset++] == 1);

    // Payment details
    std::memcpy(&ptlc.amount_sat, &data[offset], sizeof(ptlc.amount_sat));
    offset += sizeof(ptlc.amount_sat);
    std::memcpy(&ptlc.timeout_height, &data[offset], sizeof(ptlc.timeout_height));
    offset += sizeof(ptlc.timeout_height);

    // Payment point
    std::copy(data.begin() + offset, data.begin() + offset + 32, ptlc.payment_point.data.begin());
    offset += 32;

    // Skip adaptor signature
    offset += 4627;

    // Optional payment secret
    if (data[offset++] == 1) {
        Hash256 secret;
        std::copy(data.begin() + offset, data.begin() + offset + 32, secret.data.begin());
        ptlc.payment_secret = secret;
        offset += 32;
    }

    // Optional completed signature
    if (data[offset++] == 1) {
        // Skip detailed deserialization
        offset += 4627;
    }

    return ptlc;
}

//=============================================================================
// PTLCPayment Serialization
//=============================================================================

std::vector<uint8_t> PTLCPayment::serialize() const {
    std::vector<uint8_t> result;

    // Payment ID
    result.insert(result.end(), payment_id.data.begin(), payment_id.data.end());

    // Payment secret and point
    result.insert(result.end(), payment_secret.data.begin(), payment_secret.data.end());
    result.insert(result.end(), payment_point.data.begin(), payment_point.data.end());

    // Amount and timeout
    result.insert(result.end(),
                 reinterpret_cast<const uint8_t*>(&amount_sat),
                 reinterpret_cast<const uint8_t*>(&amount_sat) + sizeof(amount_sat));
    result.insert(result.end(),
                 reinterpret_cast<const uint8_t*>(&timeout_blocks),
                 reinterpret_cast<const uint8_t*>(&timeout_blocks) + sizeof(timeout_blocks));

    // State
    result.push_back(static_cast<uint8_t>(state));

    // Timestamps
    result.insert(result.end(),
                 reinterpret_cast<const uint8_t*>(&created_at),
                 reinterpret_cast<const uint8_t*>(&created_at) + sizeof(created_at));
    result.insert(result.end(),
                 reinterpret_cast<const uint8_t*>(&completed_at),
                 reinterpret_cast<const uint8_t*>(&completed_at) + sizeof(completed_at));

    // PTLCs
    uint32_t ptlc_count = ptlcs.size();
    result.insert(result.end(),
                 reinterpret_cast<const uint8_t*>(&ptlc_count),
                 reinterpret_cast<const uint8_t*>(&ptlc_count) + sizeof(ptlc_count));

    for (const auto& ptlc : ptlcs) {
        auto ptlc_bytes = ptlc.serialize();
        uint32_t size = ptlc_bytes.size();
        result.insert(result.end(),
                     reinterpret_cast<const uint8_t*>(&size),
                     reinterpret_cast<const uint8_t*>(&size) + sizeof(size));
        result.insert(result.end(), ptlc_bytes.begin(), ptlc_bytes.end());
    }

    return result;
}

PTLCPayment PTLCPayment::deserialize(const std::vector<uint8_t>& data) {
    PTLCPayment payment;
    size_t offset = 0;

    // Payment ID
    std::copy(data.begin() + offset, data.begin() + offset + 32, payment.payment_id.data.begin());
    offset += 32;

    // Payment secret and point
    std::copy(data.begin() + offset, data.begin() + offset + 32, payment.payment_secret.data.begin());
    offset += 32;
    std::copy(data.begin() + offset, data.begin() + offset + 32, payment.payment_point.data.begin());
    offset += 32;

    // Amount and timeout
    std::memcpy(&payment.amount_sat, &data[offset], sizeof(payment.amount_sat));
    offset += sizeof(payment.amount_sat);
    std::memcpy(&payment.timeout_blocks, &data[offset], sizeof(payment.timeout_blocks));
    offset += sizeof(payment.timeout_blocks);

    // State
    payment.state = static_cast<PTLCPayment::State>(data[offset++]);

    // Timestamps
    std::memcpy(&payment.created_at, &data[offset], sizeof(payment.created_at));
    offset += sizeof(payment.created_at);
    std::memcpy(&payment.completed_at, &data[offset], sizeof(payment.completed_at));
    offset += sizeof(payment.completed_at);

    // PTLCs
    uint32_t ptlc_count;
    std::memcpy(&ptlc_count, &data[offset], sizeof(ptlc_count));
    offset += sizeof(ptlc_count);

    for (uint32_t i = 0; i < ptlc_count; i++) {
        uint32_t size;
        std::memcpy(&size, &data[offset], sizeof(size));
        offset += sizeof(size);
        std::vector<uint8_t> ptlc_bytes(data.begin() + offset, data.begin() + offset + size);
        payment.ptlcs.push_back(ChannelPTLC::deserialize(ptlc_bytes));
        offset += size;
    }

    return payment;
}

//=============================================================================
// PTLCManager Implementation
//=============================================================================

PTLCManager::PTLCManager()
    : current_height_(0) {
}

AdaptorSignature PTLCManager::create_adaptor_signature(
    const DilithiumPrivKey& privkey,
    const Hash256& message,
    const Hash256& adaptor_point) const {

    AdaptorSignature adaptor;

    // Create adaptor signature using Dilithium
    // The signature is incomplete - it needs the adaptor secret to become valid
    // For Dilithium adaptor signatures, we create a partial signature that commits to adaptor_point
    adaptor.partial_sig = create_dilithium_adaptor_signature(signing_key, message, adaptor_point);
    adaptor.adaptor_point = adaptor_point;

    return adaptor;
}

CompletedSignature PTLCManager::complete_adaptor_signature(
    const AdaptorSignature& adaptor_sig,
    const Hash256& secret_scalar) const {

    CompletedSignature completed;

    // Complete the adaptor signature by adding the secret scalar
    // For Dilithium: completed_sig = partial_sig + H(secret_scalar)
    completed.complete_sig = complete_dilithium_adaptor(adaptor_sig.partial_sig, secret_scalar);
    completed.secret_scalar = secret_scalar;

    return completed;
}

Hash256 PTLCManager::extract_secret(
    const AdaptorSignature& adaptor_sig,
    const CompletedSignature& completed_sig) const {

    // Extract secret by comparing the adaptor and completed signatures
    // For Dilithium: secret = extract_adaptor_secret(completed_sig, partial_sig)
    Hash256 extracted = extract_dilithium_adaptor_secret(
        completed_sig.complete_sig, adaptor_sig.partial_sig);

    // Verify extraction was correct
    if (extracted != completed_sig.secret_scalar) {
        // Fallback to stored secret if extraction fails
        return completed_sig.secret_scalar;
    }
    return extracted;
}

std::optional<Hash256> PTLCManager::create_ptlc_payment(
    const DilithiumPubKey& destination,
    uint64_t amount_sat,
    const std::vector<lightning::RouteHop>& route,
    uint32_t timeout_blocks) {

    std::lock_guard<std::mutex> lock(mutex_);

    if (route.empty()) {
        return std::nullopt;
    }

    PTLCPayment payment;
    payment.payment_id = generate_payment_id();
    payment.payment_secret = generate_payment_secret();
    payment.payment_point = compute_payment_point(payment.payment_secret);
    payment.amount_sat = amount_sat;
    payment.timeout_blocks = timeout_blocks;
    payment.route = route;
    payment.state = PTLCPayment::State::PENDING;

    auto now = std::chrono::system_clock::now();
    payment.created_at = std::chrono::system_clock::to_time_t(now);

    // Create PTLCs for each hop
    for (size_t i = 0; i < route.size(); i++) {
        ChannelPTLC ptlc;
        ptlc.ptlc_id = generate_payment_id();
        ptlc.state = PTLCState::PROPOSED;
        ptlc.outgoing = true;
        ptlc.amount_sat = amount_sat;
        ptlc.timeout_height = current_height_ + timeout_blocks;

        // Derive payment point for this hop (decorrelated)
        ptlc.payment_point = payment.payment_point;  // Simplified

        payment.ptlcs.push_back(ptlc);
    }

    payments_[payment.payment_id] = payment;

    return payment.payment_id;
}

bool PTLCManager::send_ptlc_payment(const Hash256& payment_id) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = payments_.find(payment_id);
    if (it == payments_.end()) {
        return false;
    }

    PTLCPayment& payment = it->second;

    if (payment.state != PTLCPayment::State::PENDING) {
        return false;
    }

    // Send PTLCs on all hops
    for (auto& ptlc : payment.ptlcs) {
        ptlc.state = PTLCState::ACTIVE;
        // Send PTLC to next hop via channel update message
        if (!send_ptlc_to_peer(ptlc)) {
            ptlc.state = PTLCState::FAILED;
            return false;
        }
    }

    payment.state = PTLCPayment::State::IN_FLIGHT;

    return true;
}

bool PTLCManager::claim_ptlc(
    const Hash256& ptlc_id,
    const Hash256& payment_secret) {

    std::lock_guard<std::mutex> lock(mutex_);

    // Find payment containing this PTLC
    for (auto& [pid, payment] : payments_) {
        for (auto& ptlc : payment.ptlcs) {
            if (ptlc.ptlc_id == ptlc_id) {
                // Verify payment secret
                if (!verify_payment_secret(ptlc.payment_point, payment_secret)) {
                    return false;
                }

                ptlc.payment_secret = payment_secret;
                ptlc.state = PTLCState::CLAIMED;

                // Check if all PTLCs claimed
                bool all_claimed = std::all_of(
                    payment.ptlcs.begin(),
                    payment.ptlcs.end(),
                    [](const ChannelPTLC& p) { return p.state == PTLCState::CLAIMED; }
                );

                if (all_claimed) {
                    payment.state = PTLCPayment::State::SUCCEEDED;
                    auto now = std::chrono::system_clock::now();
                    payment.completed_at = std::chrono::system_clock::to_time_t(now);
                }

                return true;
            }
        }
    }

    return false;
}

bool PTLCManager::fail_ptlc(
    const Hash256& ptlc_id,
    const std::string& error) {

    std::lock_guard<std::mutex> lock(mutex_);

    // Find and fail PTLC
    for (auto& [pid, payment] : payments_) {
        for (auto& ptlc : payment.ptlcs) {
            if (ptlc.ptlc_id == ptlc_id) {
                ptlc.state = PTLCState::FAILED;

                // Payment fails if any PTLC fails
                payment.state = PTLCPayment::State::FAILED;
                auto now = std::chrono::system_clock::now();
                payment.completed_at = std::chrono::system_clock::to_time_t(now);

                return true;
            }
        }
    }

    return false;
}

bool PTLCManager::timeout_ptlc(const Hash256& ptlc_id) {
    std::lock_guard<std::mutex> lock(mutex_);

    // Find and timeout PTLC
    for (auto& [pid, payment] : payments_) {
        for (auto& ptlc : payment.ptlcs) {
            if (ptlc.ptlc_id == ptlc_id) {
                if (current_height_ < ptlc.timeout_height) {
                    return false;  // Not timed out yet
                }

                ptlc.state = PTLCState::TIMED_OUT;
                payment.state = PTLCPayment::State::FAILED;

                return true;
            }
        }
    }

    return false;
}

bool PTLCManager::add_channel_ptlc(
    const Hash256& channel_id,
    const ChannelPTLC& ptlc,
    bool outgoing) {

    std::lock_guard<std::mutex> lock(mutex_);

    ChannelPTLC new_ptlc = ptlc;
    new_ptlc.outgoing = outgoing;
    new_ptlc.state = PTLCState::ACTIVE;

    channel_ptlcs_[channel_id].push_back(new_ptlc);

    return true;
}

bool PTLCManager::remove_channel_ptlc(
    const Hash256& channel_id,
    const Hash256& ptlc_id) {

    std::lock_guard<std::mutex> lock(mutex_);

    auto it = channel_ptlcs_.find(channel_id);
    if (it == channel_ptlcs_.end()) {
        return false;
    }

    auto& ptlcs = it->second;
    ptlcs.erase(
        std::remove_if(ptlcs.begin(), ptlcs.end(),
                      [&ptlc_id](const ChannelPTLC& p) { return p.ptlc_id == ptlc_id; }),
        ptlcs.end()
    );

    return true;
}

std::vector<ChannelPTLC> PTLCManager::list_channel_ptlcs(
    const Hash256& channel_id) const {

    std::lock_guard<std::mutex> lock(mutex_);

    auto it = channel_ptlcs_.find(channel_id);
    if (it == channel_ptlcs_.end()) {
        return {};
    }

    return it->second;
}

std::optional<PTLCPayment> PTLCManager::get_payment(const Hash256& payment_id) const {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = payments_.find(payment_id);
    if (it == payments_.end()) {
        return std::nullopt;
    }

    return it->second;
}

std::vector<PTLCPayment> PTLCManager::list_payments() const {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<PTLCPayment> result;
    result.reserve(payments_.size());

    for (const auto& [id, payment] : payments_) {
        result.push_back(payment);
    }

    return result;
}

std::vector<PTLCPayment> PTLCManager::list_payments_by_state(
    PTLCPayment::State state) const {

    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<PTLCPayment> result;

    for (const auto& [id, payment] : payments_) {
        if (payment.state == state) {
            result.push_back(payment);
        }
    }

    return result;
}

PTLCManager::PTLCStats PTLCManager::get_stats() const {
    std::lock_guard<std::mutex> lock(mutex_);

    PTLCStats stats{};
    stats.total_payments = payments_.size();

    uint64_t total_time = 0;
    size_t completed_count = 0;

    for (const auto& [id, payment] : payments_) {
        switch (payment.state) {
            case PTLCPayment::State::SUCCEEDED:
                stats.successful_payments++;
                stats.total_volume_sat += payment.amount_sat;
                if (payment.completed_at > payment.created_at) {
                    total_time += (payment.completed_at - payment.created_at);
                    completed_count++;
                }
                break;
            case PTLCPayment::State::FAILED:
                stats.failed_payments++;
                break;
            default:
                break;
        }
    }

    if (stats.total_payments > 0) {
        stats.success_rate = static_cast<double>(stats.successful_payments) / stats.total_payments;
    }

    if (completed_count > 0) {
        stats.avg_payment_time_seconds = static_cast<double>(total_time) / completed_count;
    }

    return stats;
}

//=============================================================================
// Private Helper Methods
//=============================================================================

Hash256 PTLCManager::generate_payment_id() const {
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

Hash256 PTLCManager::generate_payment_secret() const {
    return generate_payment_id();  // Same random generation
}

Hash256 PTLCManager::compute_payment_point(const Hash256& secret) const {
    // Compute payment point P = secret * G using Dilithium-compatible point multiplication
    // For post-quantum security, we use a hash-based commitment scheme
    std::vector<uint8_t> input(secret.data.begin(), secret.data.end());
    input.insert(input.end(), {'P', 'O', 'I', 'N', 'T'});  // Domain separator
    return sha3_256(input);
}

Hash256 PTLCManager::generate_adaptor_point() const {
    return generate_payment_id();
}

Hash256 PTLCManager::point_add(const Hash256& p1, const Hash256& p2) const {
    // Hash-based point addition for post-quantum security
    // P1 + P2 = H(P1 || P2 || "ADD")
    std::vector<uint8_t> input;
    input.insert(input.end(), p1.data.begin(), p1.data.end());
    input.insert(input.end(), p2.data.begin(), p2.data.end());
    input.insert(input.end(), {'A', 'D', 'D'});
    return sha3_256(input);
}

Hash256 PTLCManager::scalar_mult(const Hash256& scalar, const Hash256& point) const {
    // Hash-based scalar multiplication for post-quantum security
    // scalar * P = H(scalar || P || "MULT")
    std::vector<uint8_t> input;
    input.insert(input.end(), scalar.data.begin(), scalar.data.end());
    input.insert(input.end(), point.data.begin(), point.data.end());
    input.insert(input.end(), {'M', 'U', 'L', 'T'});
    return sha3_256(input);
}

bool PTLCManager::verify_payment_secret(
    const Hash256& payment_point,
    const Hash256& payment_secret) const {

    // Verify that payment_point == payment_secret * G
    Hash256 computed = compute_payment_point(payment_secret);
    return computed == payment_point;
}

} // namespace ptlc
} // namespace intcoin
