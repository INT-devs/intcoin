// Copyright (c) 2025 INTcoin Core (Maddison Lane)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "intcoin/amp.h"
#include <random>
#include <chrono>
#include <algorithm>
#include <thread>

namespace intcoin {
namespace amp {

//=============================================================================
// AMPPath Serialization
//=============================================================================

std::vector<uint8_t> AMPPath::serialize() const {
    std::vector<uint8_t> result;

    // Path ID
    result.insert(result.end(), path_id.data.begin(), path_id.data.end());

    // Payment hash
    result.insert(result.end(), payment_hash.data.begin(), payment_hash.data.end());

    // Preimage
    result.insert(result.end(), preimage.data.begin(), preimage.data.end());

    // Amount and timeout
    result.insert(result.end(),
                 reinterpret_cast<const uint8_t*>(&amount_sat),
                 reinterpret_cast<const uint8_t*>(&amount_sat) + sizeof(amount_sat));
    result.insert(result.end(),
                 reinterpret_cast<const uint8_t*>(&timeout_height),
                 reinterpret_cast<const uint8_t*>(&timeout_height) + sizeof(timeout_height));

    // Status flags
    result.push_back(sent ? 1 : 0);
    result.push_back(completed ? 1 : 0);

    // Route (simplified - just store hop count)
    uint32_t hop_count = route.size();
    result.insert(result.end(),
                 reinterpret_cast<const uint8_t*>(&hop_count),
                 reinterpret_cast<const uint8_t*>(&hop_count) + sizeof(hop_count));

    // Optional HTLC ID
    if (htlc_id.has_value()) {
        result.push_back(1);
        result.insert(result.end(),
                     htlc_id->data.begin(),
                     htlc_id->data.end());
    } else {
        result.push_back(0);
    }

    // Optional error
    if (error.has_value()) {
        result.push_back(1);
        uint32_t error_len = error->length();
        result.insert(result.end(),
                     reinterpret_cast<const uint8_t*>(&error_len),
                     reinterpret_cast<const uint8_t*>(&error_len) + sizeof(error_len));
        result.insert(result.end(), error->begin(), error->end());
    } else {
        result.push_back(0);
    }

    return result;
}

AMPPath AMPPath::deserialize(const std::vector<uint8_t>& data) {
    AMPPath path;
    size_t offset = 0;

    // Path ID
    std::copy(data.begin() + offset, data.begin() + offset + 32, path.path_id.data.begin());
    offset += 32;

    // Payment hash
    std::copy(data.begin() + offset, data.begin() + offset + 32, path.payment_hash.data.begin());
    offset += 32;

    // Preimage
    std::copy(data.begin() + offset, data.begin() + offset + 32, path.preimage.data.begin());
    offset += 32;

    // Amount and timeout
    std::memcpy(&path.amount_sat, &data[offset], sizeof(path.amount_sat));
    offset += sizeof(path.amount_sat);
    std::memcpy(&path.timeout_height, &data[offset], sizeof(path.timeout_height));
    offset += sizeof(path.timeout_height);

    // Status flags
    path.sent = (data[offset++] == 1);
    path.completed = (data[offset++] == 1);

    // Route
    uint32_t hop_count;
    std::memcpy(&hop_count, &data[offset], sizeof(hop_count));
    offset += sizeof(hop_count);
    // TODO: Deserialize actual route hops

    // Optional HTLC ID
    if (data[offset++] == 1) {
        Hash256 hid;
        std::copy(data.begin() + offset, data.begin() + offset + 32, hid.data.begin());
        path.htlc_id = hid;
        offset += 32;
    }

    // Optional error
    if (data[offset++] == 1) {
        uint32_t error_len;
        std::memcpy(&error_len, &data[offset], sizeof(error_len));
        offset += sizeof(error_len);
        path.error = std::string(data.begin() + offset, data.begin() + offset + error_len);
        offset += error_len;
    }

    return path;
}

//=============================================================================
// AMPPayment Methods
//=============================================================================

std::vector<uint8_t> AMPPayment::serialize() const {
    std::vector<uint8_t> result;

    // Payment ID
    result.insert(result.end(), payment_id.data.begin(), payment_id.data.end());

    // Root secret
    result.insert(result.end(), root_secret.data.begin(), root_secret.data.end());

    // Amounts
    result.insert(result.end(),
                 reinterpret_cast<const uint8_t*>(&total_amount_sat),
                 reinterpret_cast<const uint8_t*>(&total_amount_sat) + sizeof(total_amount_sat));
    result.insert(result.end(),
                 reinterpret_cast<const uint8_t*>(&total_fee_sat),
                 reinterpret_cast<const uint8_t*>(&total_fee_sat) + sizeof(total_fee_sat));

    // State
    result.push_back(static_cast<uint8_t>(state));

    // Timestamps
    result.insert(result.end(),
                 reinterpret_cast<const uint8_t*>(&created_at),
                 reinterpret_cast<const uint8_t*>(&created_at) + sizeof(created_at));
    result.insert(result.end(),
                 reinterpret_cast<const uint8_t*>(&completed_at),
                 reinterpret_cast<const uint8_t*>(&completed_at) + sizeof(completed_at));

    // Paths
    uint32_t path_count = paths.size();
    result.insert(result.end(),
                 reinterpret_cast<const uint8_t*>(&path_count),
                 reinterpret_cast<const uint8_t*>(&path_count) + sizeof(path_count));

    for (const auto& path : paths) {
        auto path_bytes = path.serialize();
        uint32_t path_size = path_bytes.size();
        result.insert(result.end(),
                     reinterpret_cast<const uint8_t*>(&path_size),
                     reinterpret_cast<const uint8_t*>(&path_size) + sizeof(path_size));
        result.insert(result.end(), path_bytes.begin(), path_bytes.end());
    }

    return result;
}

AMPPayment AMPPayment::deserialize(const std::vector<uint8_t>& data) {
    AMPPayment payment;
    size_t offset = 0;

    // Payment ID
    std::copy(data.begin() + offset, data.begin() + offset + 32, payment.payment_id.data.begin());
    offset += 32;

    // Root secret
    std::copy(data.begin() + offset, data.begin() + offset + 32, payment.root_secret.data.begin());
    offset += 32;

    // Amounts
    std::memcpy(&payment.total_amount_sat, &data[offset], sizeof(payment.total_amount_sat));
    offset += sizeof(payment.total_amount_sat);
    std::memcpy(&payment.total_fee_sat, &data[offset], sizeof(payment.total_fee_sat));
    offset += sizeof(payment.total_fee_sat);

    // State
    payment.state = static_cast<AMPPaymentState>(data[offset++]);

    // Timestamps
    std::memcpy(&payment.created_at, &data[offset], sizeof(payment.created_at));
    offset += sizeof(payment.created_at);
    std::memcpy(&payment.completed_at, &data[offset], sizeof(payment.completed_at));
    offset += sizeof(payment.completed_at);

    // Paths
    uint32_t path_count;
    std::memcpy(&path_count, &data[offset], sizeof(path_count));
    offset += sizeof(path_count);

    for (uint32_t i = 0; i < path_count; i++) {
        uint32_t path_size;
        std::memcpy(&path_size, &data[offset], sizeof(path_size));
        offset += sizeof(path_size);

        std::vector<uint8_t> path_bytes(data.begin() + offset, data.begin() + offset + path_size);
        payment.paths.push_back(AMPPath::deserialize(path_bytes));
        offset += path_size;
    }

    return payment;
}

bool AMPPayment::all_paths_succeeded() const {
    if (paths.empty()) {
        return false;
    }

    return std::all_of(paths.begin(), paths.end(),
                      [](const AMPPath& path) { return path.completed; });
}

bool AMPPayment::any_path_failed() const {
    return std::any_of(paths.begin(), paths.end(),
                      [](const AMPPath& path) { return path.error.has_value(); });
}

size_t AMPPayment::num_completed_paths() const {
    return std::count_if(paths.begin(), paths.end(),
                        [](const AMPPath& path) { return path.completed; });
}

//=============================================================================
// AMPPaymentManager Implementation
//=============================================================================

AMPPaymentManager::AMPPaymentManager()
    : default_strategy_(SplitStrategy::WEIGHTED),
      max_paths_(MAX_AMP_PATHS),
      min_path_amount_(MIN_PATH_AMOUNT),
      current_height_(0) {
}

std::optional<Hash256> AMPPaymentManager::create_amp_payment(
    const DilithiumPubKey& destination,
    const AMPPaymentParams& params) {

    std::lock_guard<std::mutex> lock(mutex_);

    // Validate parameters
    if (params.total_amount_sat == 0) {
        return std::nullopt;
    }

    // Determine number of paths
    size_t num_paths = params.num_paths;
    if (num_paths == 0) {
        // Auto-determine based on amount
        // Use more paths for larger amounts
        if (params.total_amount_sat < 100000) {
            num_paths = 2;
        } else if (params.total_amount_sat < 1000000) {
            num_paths = 4;
        } else {
            num_paths = 8;
        }
    }

    // Clamp to max
    if (num_paths > max_paths_) {
        num_paths = max_paths_;
    }

    // Find multiple routes
    auto routes = find_multiple_routes(destination, num_paths, params.total_amount_sat);

    if (routes.empty()) {
        return std::nullopt;
    }

    // Split payment amount across routes
    auto amounts = split_payment_amount(params.total_amount_sat, routes, params.strategy);

    if (amounts.size() != routes.size()) {
        return std::nullopt;
    }

    // Create payment
    AMPPayment payment;
    payment.payment_id = generate_payment_id();
    payment.root_secret = generate_root_secret();
    payment.total_amount_sat = params.total_amount_sat;
    payment.total_fee_sat = 0;  // Will be calculated when sending
    payment.state = AMPPaymentState::PENDING;
    payment.created_at = current_height_;
    payment.completed_at = 0;

    // Create paths
    for (size_t i = 0; i < routes.size(); i++) {
        AMPPath path;
        path.path_id = generate_payment_id();  // Unique ID for each path

        // Derive path secret and preimage
        Hash256 path_secret = derive_path_secret(payment.root_secret, i);
        path.preimage = derive_path_preimage(path_secret);
        path.payment_hash = compute_payment_hash(path.preimage);

        path.amount_sat = amounts[i];
        path.timeout_height = current_height_ + params.timeout_blocks;
        path.route = routes[i];
        path.sent = false;
        path.completed = false;

        payment.paths.push_back(path);
    }

    // Store payment
    payments_[payment.payment_id] = payment;

    return payment.payment_id;
}

bool AMPPaymentManager::send_amp_payment(const Hash256& payment_id) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = payments_.find(payment_id);
    if (it == payments_.end()) {
        return false;
    }

    AMPPayment& payment = it->second;

    if (payment.state != AMPPaymentState::PENDING) {
        return false;
    }

    // Send HTLCs on all paths
    bool all_sent = true;
    for (auto& path : payment.paths) {
        // Send HTLC on this path
        // In a production implementation, this would:
        // 1. Create update_add_htlc message with path payment hash
        // 2. Sign commitment transaction with new HTLC
        // 3. Send to first hop in route
        // 4. Wait for acknowledgment

        // For now, simulate HTLC sending
        bool htlc_sent = send_htlc_on_path(path);

        if (htlc_sent) {
            path.sent = true;
            // Generate HTLC ID (in production, this comes from channel state)
            path.htlc_id = generate_payment_id();
        } else {
            all_sent = false;
            break;  // If any path fails, stop sending
        }
    }

    if (all_sent) {
        payment.state = AMPPaymentState::IN_FLIGHT;
    }

    return all_sent;
}

bool AMPPaymentManager::cancel_amp_payment(const Hash256& payment_id) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = payments_.find(payment_id);
    if (it == payments_.end()) {
        return false;
    }

    AMPPayment& payment = it->second;

    if (payment.state != AMPPaymentState::PENDING &&
        payment.state != AMPPaymentState::IN_FLIGHT) {
        return false;
    }

    // TODO: Cancel/reclaim HTLCs on all paths

    payment.state = AMPPaymentState::CANCELLED;
    payment.completed_at = current_height_;

    completion_cv_.notify_all();

    return true;
}

bool AMPPaymentManager::handle_path_success(
    const Hash256& payment_id,
    const Hash256& path_id,
    const Hash256& preimage) {

    std::unique_lock<std::mutex> lock(mutex_);

    auto it = payments_.find(payment_id);
    if (it == payments_.end()) {
        return false;
    }

    AMPPayment& payment = it->second;

    // Find the path
    auto path_it = std::find_if(payment.paths.begin(), payment.paths.end(),
                                [&path_id](const AMPPath& p) { return p.path_id == path_id; });

    if (path_it == payment.paths.end()) {
        return false;
    }

    // Verify preimage
    Hash256 computed_hash = compute_payment_hash(preimage);
    if (computed_hash != path_it->payment_hash) {
        return false;
    }

    // Mark path as completed
    path_it->completed = true;

    // Check if all paths completed
    if (payment.all_paths_succeeded()) {
        payment.state = AMPPaymentState::SUCCEEDED;
        payment.completed_at = current_height_;
        completion_cv_.notify_all();
    }

    return true;
}

bool AMPPaymentManager::handle_path_failure(
    const Hash256& payment_id,
    const Hash256& path_id,
    const std::string& error) {

    std::unique_lock<std::mutex> lock(mutex_);

    auto it = payments_.find(payment_id);
    if (it == payments_.end()) {
        return false;
    }

    AMPPayment& payment = it->second;

    // Find the path
    auto path_it = std::find_if(payment.paths.begin(), payment.paths.end(),
                                [&path_id](const AMPPath& p) { return p.path_id == path_id; });

    if (path_it == payment.paths.end()) {
        return false;
    }

    // Mark path as failed
    path_it->error = error;

    // Payment fails if ANY path fails (all-or-nothing)
    payment.state = AMPPaymentState::FAILED;
    payment.completed_at = current_height_;

    // Cleanup other paths
    cleanup_failed_paths(payment_id);

    completion_cv_.notify_all();

    return true;
}

bool AMPPaymentManager::is_payment_complete(const Hash256& payment_id) const {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = payments_.find(payment_id);
    if (it == payments_.end()) {
        return false;
    }

    const AMPPayment& payment = it->second;

    return (payment.state == AMPPaymentState::SUCCEEDED ||
            payment.state == AMPPaymentState::FAILED ||
            payment.state == AMPPaymentState::CANCELLED);
}

bool AMPPaymentManager::wait_for_completion(
    const Hash256& payment_id,
    uint32_t timeout_seconds) {

    std::unique_lock<std::mutex> lock(mutex_);

    auto timeout_time = std::chrono::steady_clock::now() +
                       std::chrono::seconds(timeout_seconds);

    while (!is_payment_complete(payment_id)) {
        if (completion_cv_.wait_until(lock, timeout_time) == std::cv_status::timeout) {
            return false;  // Timed out
        }
    }

    auto it = payments_.find(payment_id);
    if (it == payments_.end()) {
        return false;
    }

    return (it->second.state == AMPPaymentState::SUCCEEDED);
}

std::optional<AMPPayment> AMPPaymentManager::get_payment(const Hash256& payment_id) const {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = payments_.find(payment_id);
    if (it == payments_.end()) {
        return std::nullopt;
    }

    return it->second;
}

std::vector<AMPPayment> AMPPaymentManager::list_payments() const {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<AMPPayment> result;
    result.reserve(payments_.size());

    for (const auto& [id, payment] : payments_) {
        result.push_back(payment);
    }

    return result;
}

std::vector<AMPPayment> AMPPaymentManager::list_payments_by_state(AMPPaymentState state) const {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<AMPPayment> result;

    for (const auto& [id, payment] : payments_) {
        if (payment.state == state) {
            result.push_back(payment);
        }
    }

    return result;
}

bool AMPPaymentManager::remove_payment(const Hash256& payment_id) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = payments_.find(payment_id);
    if (it == payments_.end()) {
        return false;
    }

    // Only remove completed payments
    if (!is_payment_complete(payment_id)) {
        return false;
    }

    payments_.erase(it);
    return true;
}

AMPPaymentManager::AMPStats AMPPaymentManager::get_stats() const {
    std::lock_guard<std::mutex> lock(mutex_);

    AMPStats stats{};
    stats.total_payments = payments_.size();

    uint64_t total_paths = 0;

    for (const auto& [id, payment] : payments_) {
        total_paths += payment.paths.size();

        switch (payment.state) {
            case AMPPaymentState::SUCCEEDED:
                stats.successful_payments++;
                stats.total_volume_sat += payment.total_amount_sat;
                stats.total_fees_sat += payment.total_fee_sat;
                break;
            case AMPPaymentState::FAILED:
            case AMPPaymentState::CANCELLED:
                stats.failed_payments++;
                break;
            case AMPPaymentState::PENDING:
            case AMPPaymentState::IN_FLIGHT:
                stats.pending_payments++;
                break;
        }
    }

    if (stats.total_payments > 0) {
        stats.average_paths_per_payment = static_cast<double>(total_paths) / stats.total_payments;
    }

    if (stats.total_payments > 0) {
        stats.success_rate = static_cast<double>(stats.successful_payments) / stats.total_payments;
    }

    return stats;
}

void AMPPaymentManager::set_default_strategy(SplitStrategy strategy) {
    std::lock_guard<std::mutex> lock(mutex_);
    default_strategy_ = strategy;
}

void AMPPaymentManager::set_max_paths(size_t max_paths) {
    std::lock_guard<std::mutex> lock(mutex_);
    max_paths_ = std::min(max_paths, MAX_AMP_PATHS);
}

void AMPPaymentManager::set_min_path_amount(uint64_t min_amount_sat) {
    std::lock_guard<std::mutex> lock(mutex_);
    min_path_amount_ = min_amount_sat;
}

//=============================================================================
// Private Helper Methods
//=============================================================================

Hash256 AMPPaymentManager::generate_payment_id() const {
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

Hash256 AMPPaymentManager::generate_root_secret() const {
    return generate_payment_id();  // Same random generation
}

Hash256 AMPPaymentManager::derive_path_secret(const Hash256& root_secret, size_t path_index) const {
    // SHA3-256(root_secret || path_index)
    std::vector<uint8_t> input(root_secret.data.begin(), root_secret.data.end());
    input.insert(input.end(),
                reinterpret_cast<const uint8_t*>(&path_index),
                reinterpret_cast<const uint8_t*>(&path_index) + sizeof(path_index));

    return sha3_256(input);
}

Hash256 AMPPaymentManager::derive_path_preimage(const Hash256& path_secret) const {
    // Preimage is just the path secret itself
    return path_secret;
}

Hash256 AMPPaymentManager::compute_payment_hash(const Hash256& preimage) const {
    return sha3_256(preimage.data);
}

std::vector<std::vector<lightning::RouteHop>> AMPPaymentManager::find_multiple_routes(
    const DilithiumPubKey& destination,
    size_t max_paths,
    uint64_t total_amount_sat) const {

    std::vector<std::vector<lightning::RouteHop>> routes;

    // Multi-path route finding algorithm
    // Strategy: Find node-disjoint paths to destination
    // This ensures maximum reliability and privacy

    // In a production implementation, this would:
    // 1. Query the network graph from the routing table
    // 2. Use Dijkstra's or Yen's K-shortest paths algorithm
    // 3. Filter for node-disjoint paths (no shared intermediate nodes)
    // 4. Check capacity constraints for each path
    // 5. Calculate fees for each route

    // For now, create realistic mock routes with varying hop counts
    // and simulate different paths through the network
    for (size_t i = 0; i < max_paths && i < 6; i++) {
        std::vector<lightning::RouteHop> route;

        // Vary hop count (2-5 hops) for different routes
        size_t hops = 2 + (i % 4);

        for (size_t j = 0; j < hops; j++) {
            lightning::RouteHop hop;

            // Simulate hop data
            // In production, these would be actual nodes from the network graph
            hop.node_id = generate_payment_id();  // Mock node ID
            hop.channel_id = generate_payment_id();  // Mock channel ID

            // Estimate fees (0.1% base + 1 sat)
            uint64_t hop_amount = total_amount_sat / max_paths;
            hop.fee_sat = std::max(uint64_t(1), hop_amount / 1000);

            // HTLC parameters
            hop.cltv_expiry_delta = 40;  // 40 blocks per hop
            hop.amount_to_forward_sat = hop_amount;

            route.push_back(hop);
        }

        // Only add route if we successfully generated hops
        if (!route.empty()) {
            routes.push_back(route);
        }
    }

    return routes;
}

std::vector<uint64_t> AMPPaymentManager::split_payment_amount(
    uint64_t total_amount_sat,
    const std::vector<std::vector<lightning::RouteHop>>& routes,
    SplitStrategy strategy) const {

    std::vector<uint64_t> amounts;

    switch (strategy) {
        case SplitStrategy::EQUAL: {
            // Split equally
            uint64_t amount_per_path = total_amount_sat / routes.size();
            uint64_t remainder = total_amount_sat % routes.size();

            for (size_t i = 0; i < routes.size(); i++) {
                amounts.push_back(amount_per_path + (i == 0 ? remainder : 0));
            }
            break;
        }

        case SplitStrategy::WEIGHTED: {
            // TODO: Weight by path capacity and reliability
            // For now, use equal split
            uint64_t amount_per_path = total_amount_sat / routes.size();
            uint64_t remainder = total_amount_sat % routes.size();

            for (size_t i = 0; i < routes.size(); i++) {
                amounts.push_back(amount_per_path + (i == 0 ? remainder : 0));
            }
            break;
        }

        case SplitStrategy::RANDOM: {
            // Random split for privacy
            std::random_device rd;
            std::mt19937_64 gen(rd());

            uint64_t remaining = total_amount_sat;
            for (size_t i = 0; i < routes.size() - 1; i++) {
                uint64_t max_amount = remaining - (routes.size() - i - 1) * min_path_amount_;
                std::uniform_int_distribution<uint64_t> dis(min_path_amount_, max_amount);
                uint64_t amount = dis(gen);
                amounts.push_back(amount);
                remaining -= amount;
            }
            amounts.push_back(remaining);  // Last path gets remainder
            break;
        }
    }

    return amounts;
}

bool AMPPaymentManager::verify_all_preimages(const AMPPayment& payment) const {
    for (const auto& path : payment.paths) {
        Hash256 computed_hash = compute_payment_hash(path.preimage);
        if (computed_hash != path.payment_hash) {
            return false;
        }
    }
    return true;
}

void AMPPaymentManager::finalize_payment(const Hash256& payment_id) {
    // Already done in handle_path_success/failure
}

void AMPPaymentManager::cleanup_failed_paths(const Hash256& payment_id) {
    // Reclaim HTLCs from paths that haven't failed yet
    // When a payment fails, we need to fail back all HTLCs that were sent

    auto it = payments_.find(payment_id);
    if (it == payments_.end()) {
        return;
    }

    AMPPayment& payment = it->second;

    for (auto& path : payment.paths) {
        if (path.sent && !path.completed && !path.error.has_value()) {
            // Send fail_htlc message back along the route
            // In production, this would send update_fail_htlc messages
            path.error = "Payment failed - reclaiming HTLC";
        }
    }
}

bool AMPPaymentManager::send_htlc_on_path(const AMPPath& path) const {
    // Send HTLC on the specified path
    // In a production implementation, this would:
    //
    // 1. Get the first channel in the route
    // 2. Create update_add_htlc message:
    //    - htlc_id: next available ID in channel
    //    - amount_msat: path.amount_sat * 1000
    //    - payment_hash: path.payment_hash
    //    - cltv_expiry: current_height + timeout + route delays
    //    - onion_routing_packet: encrypted route info
    //
    // 3. Add HTLC to local commitment transaction
    // 4. Sign new commitment
    // 5. Send commitment_signed message
    // 6. Wait for revoke_and_ack
    // 7. Send update_add_htlc to first hop
    //
    // For now, simulate successful send with 95% success rate
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(0.0, 1.0);

    // Simulate network conditions (95% success rate)
    return dis(gen) < 0.95;
}

//=============================================================================
// AMPInvoice Implementation
//=============================================================================

std::vector<uint8_t> AMPInvoice::serialize() const {
    std::vector<uint8_t> result;

    // Payment ID
    result.insert(result.end(), payment_id.data.begin(), payment_id.data.end());

    // Destination pubkey
    auto pubkey_bytes = destination.serialize();
    result.insert(result.end(), pubkey_bytes.begin(), pubkey_bytes.end());

    // Amount
    result.insert(result.end(),
                 reinterpret_cast<const uint8_t*>(&amount_sat),
                 reinterpret_cast<const uint8_t*>(&amount_sat) + sizeof(amount_sat));

    // Description
    uint32_t desc_len = description.length();
    result.insert(result.end(),
                 reinterpret_cast<const uint8_t*>(&desc_len),
                 reinterpret_cast<const uint8_t*>(&desc_len) + sizeof(desc_len));
    result.insert(result.end(), description.begin(), description.end());

    // Expiry
    result.insert(result.end(),
                 reinterpret_cast<const uint8_t*>(&expiry_timestamp),
                 reinterpret_cast<const uint8_t*>(&expiry_timestamp) + sizeof(expiry_timestamp));

    // AMP fields
    result.push_back(amp_required ? 1 : 0);
    result.insert(result.end(),
                 reinterpret_cast<const uint8_t*>(&min_paths),
                 reinterpret_cast<const uint8_t*>(&min_paths) + sizeof(min_paths));
    result.insert(result.end(),
                 reinterpret_cast<const uint8_t*>(&max_paths),
                 reinterpret_cast<const uint8_t*>(&max_paths) + sizeof(max_paths));

    return result;
}

AMPInvoice AMPInvoice::deserialize(const std::vector<uint8_t>& data) {
    AMPInvoice invoice;
    size_t offset = 0;

    // Payment ID
    std::copy(data.begin() + offset, data.begin() + offset + 32, invoice.payment_id.data.begin());
    offset += 32;

    // Destination pubkey
    // TODO: Deserialize pubkey
    offset += 1952;  // Dilithium5 public key size

    // Amount
    std::memcpy(&invoice.amount_sat, &data[offset], sizeof(invoice.amount_sat));
    offset += sizeof(invoice.amount_sat);

    // Description
    uint32_t desc_len;
    std::memcpy(&desc_len, &data[offset], sizeof(desc_len));
    offset += sizeof(desc_len);
    invoice.description = std::string(data.begin() + offset, data.begin() + offset + desc_len);
    offset += desc_len;

    // Expiry
    std::memcpy(&invoice.expiry_timestamp, &data[offset], sizeof(invoice.expiry_timestamp));
    offset += sizeof(invoice.expiry_timestamp);

    // AMP fields
    invoice.amp_required = (data[offset++] == 1);
    std::memcpy(&invoice.min_paths, &data[offset], sizeof(invoice.min_paths));
    offset += sizeof(invoice.min_paths);
    std::memcpy(&invoice.max_paths, &data[offset], sizeof(invoice.max_paths));
    offset += sizeof(invoice.max_paths);

    return invoice;
}

std::string AMPInvoice::encode() const {
    // TODO: Implement Bech32 encoding
    return "intc1...";
}

std::optional<AMPInvoice> AMPInvoice::decode(const std::string& encoded) {
    // TODO: Implement Bech32 decoding
    return std::nullopt;
}

} // namespace amp
} // namespace intcoin
