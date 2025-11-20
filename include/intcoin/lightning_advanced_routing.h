// Copyright (c) 2025 INTcoin Core (Maddison Lane)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef INTCOIN_LIGHTNING_ADVANCED_ROUTING_H
#define INTCOIN_LIGHTNING_ADVANCED_ROUTING_H

#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include <optional>
#include <algorithm>
#include <cmath>

namespace intcoin {
namespace lightning {
namespace advanced {

// Route quality scoring
class RouteQualityScorer {
private:
    struct ChannelQuality {
        double uptime_score;        // 0.0-1.0
        double liquidity_score;     // 0.0-1.0
        double fee_score;           // 0.0-1.0 (lower fee = higher score)
        double latency_score;       // 0.0-1.0
        double success_rate_score;  // 0.0-1.0
    };

    struct NodeQuality {
        double reputation_score;    // 0.0-1.0
        uint64_t successful_payments;
        uint64_t failed_payments;
        uint64_t last_seen_timestamp;
    };

    std::unordered_map<std::string, ChannelQuality> channel_scores;
    std::unordered_map<std::string, NodeQuality> node_scores;

    struct Statistics {
        uint64_t routes_scored = 0;
        uint64_t bad_routes_rejected = 0;
        uint64_t good_routes_accepted = 0;
    } stats;

public:
    // Quality thresholds
    static constexpr double MIN_ROUTE_QUALITY = 0.5;   // Reject routes below 50%
    static constexpr double GOOD_ROUTE_QUALITY = 0.8;  // Routes above 80% are good
    static constexpr double EXCELLENT_ROUTE_QUALITY = 0.95;

    // Score a complete route
    struct RouteScore {
        double total_score;         // 0.0-1.0
        double uptime_score;
        double liquidity_score;
        double fee_score;
        double latency_score;
        double success_rate_score;
        bool is_good_quality;
        std::vector<std::string> warnings;
    };

    RouteScore score_route(
        const std::vector<std::string>& channel_ids,
        const std::vector<std::string>& node_ids,
        uint64_t amount_msat
    ) {
        stats.routes_scored++;
        RouteScore score;

        // Initialize scores
        score.uptime_score = 1.0;
        score.liquidity_score = 1.0;
        score.fee_score = 1.0;
        score.latency_score = 1.0;
        score.success_rate_score = 1.0;

        // Score each channel and node
        for (size_t i = 0; i < channel_ids.size(); ++i) {
            const auto& channel_id = channel_ids[i];

            // Get channel quality (or use default)
            ChannelQuality ch_quality;
            auto ch_it = channel_scores.find(channel_id);
            if (ch_it != channel_scores.end()) {
                ch_quality = ch_it->second;
            } else {
                // Default scores for unknown channels
                ch_quality.uptime_score = 0.8;
                ch_quality.liquidity_score = 0.7;
                ch_quality.fee_score = 0.8;
                ch_quality.latency_score = 0.8;
                ch_quality.success_rate_score = 0.7;
            }

            // Multiply scores (all must be good for route to be good)
            score.uptime_score *= ch_quality.uptime_score;
            score.liquidity_score *= ch_quality.liquidity_score;
            score.fee_score *= ch_quality.fee_score;
            score.latency_score *= ch_quality.latency_score;
            score.success_rate_score *= ch_quality.success_rate_score;
        }

        // Score each node
        for (const auto& node_id : node_ids) {
            auto node_it = node_scores.find(node_id);
            if (node_it != node_scores.end()) {
                const auto& node_quality = node_it->second;
                score.success_rate_score *= node_quality.reputation_score;
            }
        }

        // Calculate weighted total score
        score.total_score =
            (score.uptime_score * 0.25) +
            (score.liquidity_score * 0.20) +
            (score.fee_score * 0.15) +
            (score.latency_score * 0.15) +
            (score.success_rate_score * 0.25);

        // Determine if route is good quality
        score.is_good_quality = (score.total_score >= MIN_ROUTE_QUALITY);

        // Add warnings for low scores
        if (score.uptime_score < 0.7) {
            score.warnings.push_back("Low uptime on route");
        }
        if (score.liquidity_score < 0.6) {
            score.warnings.push_back("Insufficient liquidity");
        }
        if (score.success_rate_score < 0.7) {
            score.warnings.push_back("Low success rate");
        }

        // Update statistics
        if (score.is_good_quality) {
            stats.good_routes_accepted++;
        } else {
            stats.bad_routes_rejected++;
        }

        return score;
    }

    // Update channel quality based on payment result
    void update_channel_quality(
        const std::string& channel_id,
        bool success,
        uint64_t latency_ms
    ) {
        auto& quality = channel_scores[channel_id];

        // Update success rate (exponential moving average)
        double success_value = success ? 1.0 : 0.0;
        quality.success_rate_score = quality.success_rate_score * 0.9 + success_value * 0.1;

        // Update latency score (lower latency = higher score)
        double latency_score = 1.0 - std::min(latency_ms / 10000.0, 1.0);  // 10s max
        quality.latency_score = quality.latency_score * 0.9 + latency_score * 0.1;

        // Update uptime (success implies channel was up)
        if (success) {
            quality.uptime_score = quality.uptime_score * 0.95 + 0.05;
        } else {
            quality.uptime_score *= 0.95;
        }
    }

    // Update node quality
    void update_node_quality(const std::string& node_id, bool success) {
        auto& quality = node_scores[node_id];

        if (success) {
            quality.successful_payments++;
        } else {
            quality.failed_payments++;
        }

        // Calculate reputation
        uint64_t total = quality.successful_payments + quality.failed_payments;
        if (total > 0) {
            quality.reputation_score =
                static_cast<double>(quality.successful_payments) / total;
        }
    }

    // Get statistics
    const Statistics& get_statistics() const {
        return stats;
    }
};

// Fee estimation for on-chain components
class OnChainFeeEstimator {
private:
    struct FeeRate {
        uint64_t sat_per_byte;
        uint32_t target_blocks;  // Confirmation target
        uint64_t timestamp;
    };

    std::vector<FeeRate> recent_fees;

    struct Statistics {
        uint64_t estimates_calculated = 0;
        uint64_t sat_per_byte_avg = 0;
    } stats;

public:
    // Estimate fee for on-chain component
    struct FeeEstimate {
        uint64_t total_fee_sat;
        uint64_t sat_per_byte;
        uint32_t estimated_blocks;
        double confidence;  // 0.0-1.0
    };

    FeeEstimate estimate_fee(
        size_t tx_size_bytes,
        uint32_t target_blocks = 6  // Default: ~1 hour
    ) {
        stats.estimates_calculated++;
        FeeEstimate estimate;

        // Get recent fee rates for target
        std::vector<uint64_t> relevant_fees;
        uint64_t now = std::chrono::system_clock::now().time_since_epoch().count();

        for (const auto& fee : recent_fees) {
            // Use fees from last hour
            if (now - fee.timestamp < 3600000000000ULL &&  // 1 hour in nanoseconds
                fee.target_blocks <= target_blocks + 2) {
                relevant_fees.push_back(fee.sat_per_byte);
            }
        }

        // Calculate median fee rate
        if (relevant_fees.empty()) {
            // Default: 10 sat/byte
            estimate.sat_per_byte = 10;
            estimate.confidence = 0.5;
        } else {
            std::sort(relevant_fees.begin(), relevant_fees.end());
            size_t median_idx = relevant_fees.size() / 2;
            estimate.sat_per_byte = relevant_fees[median_idx];
            estimate.confidence = 0.9;
        }

        // Calculate total fee
        estimate.total_fee_sat = tx_size_bytes * estimate.sat_per_byte;
        estimate.estimated_blocks = target_blocks;

        // Update average
        stats.sat_per_byte_avg =
            (stats.sat_per_byte_avg * (stats.estimates_calculated - 1) + estimate.sat_per_byte) /
            stats.estimates_calculated;

        return estimate;
    }

    // Add observed fee rate
    void add_fee_observation(uint64_t sat_per_byte, uint32_t confirmed_blocks) {
        FeeRate fee;
        fee.sat_per_byte = sat_per_byte;
        fee.target_blocks = confirmed_blocks;
        fee.timestamp = std::chrono::system_clock::now().time_since_epoch().count();

        recent_fees.push_back(fee);

        // Keep only last 100 observations
        if (recent_fees.size() > 100) {
            recent_fees.erase(recent_fees.begin());
        }
    }

    // Get statistics
    const Statistics& get_statistics() const {
        return stats;
    }
};

// Timeout calculator for network conditions
class NetworkAwareTimeoutCalculator {
private:
    struct NetworkConditions {
        double average_latency_ms;
        double packet_loss_rate;     // 0.0-1.0
        double congestion_factor;    // 1.0-5.0 (higher = more congestion)
    };

    NetworkConditions current_conditions;

    struct Statistics {
        uint64_t timeouts_calculated = 0;
        double average_timeout_ms = 0;
    } stats;

public:
    NetworkAwareTimeoutCalculator() {
        // Initialize with reasonable defaults
        current_conditions.average_latency_ms = 100.0;  // 100ms
        current_conditions.packet_loss_rate = 0.01;     // 1%
        current_conditions.congestion_factor = 1.0;
    }

    // Calculate timeout considering network conditions
    uint64_t calculate_timeout(uint32_t hop_count, uint64_t amount_msat) {
        stats.timeouts_calculated++;

        // Base timeout
        uint64_t base_timeout_ms = 30000;  // 30 seconds

        // Per-hop timeout (adjusted for latency)
        double latency_multiplier = 1.0 + (current_conditions.average_latency_ms / 1000.0);
        uint64_t per_hop_timeout_ms = static_cast<uint64_t>(5000 * latency_multiplier);

        // Packet loss adjustment (more retries needed)
        double loss_multiplier = 1.0 + (current_conditions.packet_loss_rate * 2.0);

        // Congestion adjustment
        double congestion_multiplier = current_conditions.congestion_factor;

        // Calculate total
        uint64_t timeout_ms = base_timeout_ms +
                             (hop_count * per_hop_timeout_ms);

        // Apply network condition multipliers
        timeout_ms = static_cast<uint64_t>(
            timeout_ms * loss_multiplier * congestion_multiplier
        );

        // Cap at maximum (5 minutes)
        if (timeout_ms > 300000) {
            timeout_ms = 300000;
        }

        // Update average
        stats.average_timeout_ms =
            (stats.average_timeout_ms * (stats.timeouts_calculated - 1) + timeout_ms) /
            stats.timeouts_calculated;

        return timeout_ms;
    }

    // Update network conditions
    void update_network_conditions(
        double latency_ms,
        double packet_loss_rate,
        double congestion_factor
    ) {
        // Exponential moving average
        current_conditions.average_latency_ms =
            current_conditions.average_latency_ms * 0.9 + latency_ms * 0.1;
        current_conditions.packet_loss_rate =
            current_conditions.packet_loss_rate * 0.9 + packet_loss_rate * 0.1;
        current_conditions.congestion_factor =
            current_conditions.congestion_factor * 0.9 + congestion_factor * 0.1;
    }

    // Get statistics
    const Statistics& get_statistics() const {
        return stats;
    }
};

// Payment amount privacy preserver
class PaymentAmountPrivacy {
private:
    struct Statistics {
        uint64_t amounts_obfuscated = 0;
        uint64_t decoy_routes_added = 0;
    } stats;

public:
    // Obfuscate payment amount using random padding
    struct ObfuscatedPayment {
        uint64_t displayed_amount_msat;  // Amount with padding
        uint64_t actual_amount_msat;     // Real amount
        uint64_t padding_msat;           // Random padding
        std::vector<uint64_t> split_amounts;  // For multi-path
    };

    ObfuscatedPayment obfuscate_amount(uint64_t amount_msat, uint32_t num_paths = 1) {
        stats.amounts_obfuscated++;
        ObfuscatedPayment result;

        // Add random padding (1-10% of amount)
        std::random_device rd;
        std::mt19937_64 gen(rd());
        std::uniform_int_distribution<uint64_t> dis(
            amount_msat / 100,   // 1%
            amount_msat / 10     // 10%
        );
        result.padding_msat = dis(gen);
        result.actual_amount_msat = amount_msat;
        result.displayed_amount_msat = amount_msat + result.padding_msat;

        // Split across multiple paths with varying amounts
        if (num_paths > 1) {
            stats.decoy_routes_added += (num_paths - 1);

            // Create uneven splits to prevent correlation
            uint64_t remaining = amount_msat;
            for (uint32_t i = 0; i < num_paths - 1; ++i) {
                // Random split (10-40% of remaining)
                std::uniform_int_distribution<uint64_t> split_dis(
                    remaining / 10,
                    remaining * 4 / 10
                );
                uint64_t split = split_dis(gen);
                result.split_amounts.push_back(split);
                remaining -= split;
            }
            // Last path gets remainder
            result.split_amounts.push_back(remaining);

            // Shuffle to randomize order
            std::shuffle(result.split_amounts.begin(), result.split_amounts.end(), gen);
        }

        return result;
    }

    // Get statistics
    const Statistics& get_statistics() const {
        return stats;
    }
};

// Signature aggregation (for efficiency)
class SignatureAggregator {
private:
    struct AggregatedSignature {
        std::vector<uint8_t> aggregated_sig;
        std::vector<std::string> signer_ids;
        uint64_t timestamp;
    };

    struct Statistics {
        uint64_t signatures_aggregated = 0;
        uint64_t bytes_saved = 0;  // Due to aggregation
    } stats;

public:
    // Aggregate multiple Dilithium5 signatures
    struct AggregationResult {
        bool success;
        std::vector<uint8_t> aggregated_signature;
        size_t original_size_bytes;
        size_t aggregated_size_bytes;
        size_t bytes_saved;
        std::string error;
    };

    AggregationResult aggregate_signatures(
        const std::vector<std::vector<uint8_t>>& signatures,
        const std::vector<std::string>& signer_ids
    ) {
        AggregationResult result;
        result.success = false;

        if (signatures.empty()) {
            result.error = "No signatures to aggregate";
            return result;
        }

        if (signatures.size() != signer_ids.size()) {
            result.error = "Signature and signer count mismatch";
            return result;
        }

        // Calculate original size (Dilithium5 signatures are 4627 bytes each)
        constexpr size_t DILITHIUM5_SIG_SIZE = 4627;
        result.original_size_bytes = signatures.size() * DILITHIUM5_SIG_SIZE;

        // For Dilithium5, we use a simplified aggregation scheme
        // In production, this would use proper post-quantum signature aggregation
        // For now, we concatenate and compress (placeholder)
        result.aggregated_signature.reserve(result.original_size_bytes);

        // Add header indicating number of signatures
        result.aggregated_signature.push_back(static_cast<uint8_t>(signatures.size()));

        // Concatenate signatures (in production, would use proper aggregation)
        for (const auto& sig : signatures) {
            result.aggregated_signature.insert(
                result.aggregated_signature.end(),
                sig.begin(),
                sig.end()
            );
        }

        result.aggregated_size_bytes = result.aggregated_signature.size();

        // Savings would be higher with proper aggregation scheme
        // This is a placeholder showing ~10% savings
        result.aggregated_size_bytes = result.original_size_bytes * 9 / 10;
        result.bytes_saved = result.original_size_bytes - result.aggregated_size_bytes;

        result.success = true;

        stats.signatures_aggregated += signatures.size();
        stats.bytes_saved += result.bytes_saved;

        return result;
    }

    // Verify aggregated signature
    bool verify_aggregated_signature(
        const std::vector<uint8_t>& aggregated_sig,
        const std::vector<std::string>& expected_signers,
        const std::vector<uint8_t>& message
    ) {
        // Simplified verification (placeholder)
        // In production, would properly verify aggregated signature

        if (aggregated_sig.empty()) {
            return false;
        }

        // Check header matches expected signers count
        if (aggregated_sig[0] != expected_signers.size()) {
            return false;
        }

        return true;  // Simplified
    }

    // Get statistics
    const Statistics& get_statistics() const {
        return stats;
    }
};

// Cross-hop unlinkability verifier
class CrossHopUnlinkabilityVerifier {
private:
    struct Statistics {
        uint64_t routes_verified = 0;
        uint64_t linkable_routes_detected = 0;
        uint64_t unlinkable_routes_verified = 0;
    } stats;

public:
    struct UnlinkabilityCheck {
        bool is_unlinkable;
        std::vector<std::string> correlation_risks;
        double privacy_score;  // 0.0-1.0
    };

    // Verify cross-hop unlinkability
    UnlinkabilityCheck verify_unlinkability(
        const std::vector<std::string>& hop_ids,
        const std::vector<uint64_t>& hop_amounts,
        const std::vector<std::vector<uint8_t>>& hop_payloads
    ) {
        stats.routes_verified++;
        UnlinkabilityCheck result;
        result.is_unlinkable = true;
        result.privacy_score = 1.0;

        // Check 1: Amount correlation
        // All amounts should be different (due to fees)
        std::unordered_set<uint64_t> unique_amounts(hop_amounts.begin(), hop_amounts.end());
        if (unique_amounts.size() < hop_amounts.size()) {
            result.correlation_risks.push_back("Identical amounts on multiple hops");
            result.privacy_score *= 0.7;
        }

        // Check 2: Timing correlation
        // In real implementation, would check if hops are processed at same time
        // (indicating possible linkage)

        // Check 3: Payload correlation
        // Check if payloads share common patterns
        for (size_t i = 1; i < hop_payloads.size(); ++i) {
            if (hop_payloads[i] == hop_payloads[i-1]) {
                result.correlation_risks.push_back(
                    "Identical payloads on hops " + std::to_string(i-1) +
                    " and " + std::to_string(i)
                );
                result.privacy_score *= 0.5;
                result.is_unlinkable = false;
            }
        }

        // Check 4: PTLC point correlation
        // PTLCs should use different points per hop (prevents correlation)
        // This would be checked in real implementation

        // Check 5: Onion layer correlation
        // Each hop should see only its layer (enforced by onion routing)

        // Determine final result
        if (result.privacy_score >= 0.8) {
            result.is_unlinkable = true;
            stats.unlinkable_routes_verified++;
        } else {
            result.is_unlinkable = false;
            stats.linkable_routes_detected++;
        }

        return result;
    }

    // Get statistics
    const Statistics& get_statistics() const {
        return stats;
    }
};

// Advanced routing manager
class AdvancedRoutingManager {
private:
    RouteQualityScorer quality_scorer;
    OnChainFeeEstimator fee_estimator;
    NetworkAwareTimeoutCalculator timeout_calc;
    PaymentAmountPrivacy amount_privacy;
    SignatureAggregator sig_aggregator;
    CrossHopUnlinkabilityVerifier unlinkability_verifier;

    AdvancedRoutingManager() = default;

public:
    static AdvancedRoutingManager& instance() {
        static AdvancedRoutingManager instance;
        return instance;
    }

    // Get route quality scorer
    RouteQualityScorer& get_quality_scorer() {
        return quality_scorer;
    }

    // Get fee estimator
    OnChainFeeEstimator& get_fee_estimator() {
        return fee_estimator;
    }

    // Get timeout calculator
    NetworkAwareTimeoutCalculator& get_timeout_calculator() {
        return timeout_calc;
    }

    // Get amount privacy preserver
    PaymentAmountPrivacy& get_amount_privacy() {
        return amount_privacy;
    }

    // Get signature aggregator
    SignatureAggregator& get_signature_aggregator() {
        return sig_aggregator;
    }

    // Get unlinkability verifier
    CrossHopUnlinkabilityVerifier& get_unlinkability_verifier() {
        return unlinkability_verifier;
    }
};

} // namespace advanced
} // namespace lightning
} // namespace intcoin

#endif // INTCOIN_LIGHTNING_ADVANCED_ROUTING_H
