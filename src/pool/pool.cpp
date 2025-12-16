/*
 * Copyright (c) 2025 INTcoin Team (Neil Adamson)
 * MIT License
 * Mining Pool Server Implementation (Stubs - API Complete)
 */

#include "intcoin/pool.h"
#include "intcoin/rpc.h"
#include "intcoin/util.h"
#include <algorithm>
#include <cmath>

namespace intcoin {

// ============================================================================
// Variable Difficulty Manager
// ============================================================================

VarDiffManager::VarDiffManager(double target_share_time, double retarget_time, double variance)
    : target_share_time_(target_share_time)
    , retarget_time_(retarget_time)
    , variance_(variance) {}

uint64_t VarDiffManager::CalculateDifficulty(const Worker& worker) const {
    if (worker.recent_shares.size() < 3) {
        return worker.current_difficulty;
    }

    // Calculate average time between shares from recent_shares
    auto total_duration = std::chrono::duration_cast<std::chrono::seconds>(
        worker.recent_shares.back() - worker.recent_shares.front());
    double avg_time = static_cast<double>(total_duration.count()) /
                     static_cast<double>(worker.recent_shares.size() - 1);
    double ratio = avg_time / target_share_time_;

    uint64_t new_diff = worker.current_difficulty;
    if (ratio < (1.0 - variance_)) {
        new_diff = static_cast<uint64_t>(worker.current_difficulty * 1.5);
    } else if (ratio > (1.0 + variance_)) {
        new_diff = static_cast<uint64_t>(worker.current_difficulty * 0.75);
    }

    return std::max(uint64_t(1000), new_diff);
}

bool VarDiffManager::ShouldAdjust(const Worker& worker) const {
    auto now = std::chrono::system_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(
        now - worker.last_share_time);
    return duration.count() >= retarget_time_ && worker.recent_shares.size() >= 3;
}

double VarDiffManager::GetShareRate(const Worker& worker) const {
    if (worker.recent_shares.size() < 2) return 0.0;
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(
        worker.recent_shares.back() - worker.recent_shares.front());
    if (duration.count() == 0) return 0.0;
    return static_cast<double>(worker.recent_shares.size()) / duration.count();
}

// ============================================================================
// Share Validator
// ============================================================================

bool ShareValidator::ValidateDifficulty(const uint256& hash, uint64_t difficulty) {
    // Validate that the hash meets the required difficulty
    // A share is valid if: hash_difficulty >= required_difficulty
    //
    // Calculate the actual difficulty achieved by this hash
    uint64_t actual_difficulty = CalculateShareDifficulty(hash);

    // Share is valid if it meets or exceeds the required difficulty
    return actual_difficulty >= difficulty;
}

bool ShareValidator::ValidateWork(const Share& share, const Work& work) {
    return share.job_id == work.job_id;
}

bool ShareValidator::IsValidBlock(const uint256& hash, uint64_t network_difficulty) {
    // Check if this share also meets network difficulty (valid block)
    // A block is valid if: hash_difficulty >= network_difficulty
    //
    // Calculate the actual difficulty achieved by this hash
    uint64_t actual_difficulty = CalculateShareDifficulty(hash);

    // This is a valid block if it meets or exceeds network difficulty
    return actual_difficulty >= network_difficulty;
}

bool ShareValidator::ValidateTimestamp(const Share& share, const Work& work) {
    auto share_time = share.timestamp;
    auto work_time = work.created_at;
    auto diff = std::chrono::duration_cast<std::chrono::seconds>(share_time - work_time);
    return diff.count() >= 0 && diff.count() < 300;  // Within 5 minutes
}

bool ShareValidator::IsDuplicateShare(const Share& share, const std::vector<Share>& recent_shares) {
    for (const auto& s : recent_shares) {
        if (s.nonce == share.nonce && s.job_id == share.job_id) {
            return true;
        }
    }
    return false;
}

// ============================================================================
// Payout Calculator
// ============================================================================

std::map<uint64_t, uint64_t> PayoutCalculator::CalculatePPLNS(
    const std::vector<Share>& shares,
    size_t n_shares,
    uint64_t block_reward,
    double pool_fee)
{
    std::map<uint64_t, uint64_t> payouts;
    uint64_t fee = CalculateFee(block_reward, pool_fee);
    uint64_t reward = block_reward - fee;

    // Count shares per miner in last N shares
    std::map<uint64_t, uint64_t> miner_shares;
    size_t start = shares.size() > n_shares ? shares.size() - n_shares : 0;
    uint64_t total = 0;

    for (size_t i = start; i < shares.size(); ++i) {
        if (shares[i].valid) {
            miner_shares[shares[i].miner_id]++;
            total++;
        }
    }

    if (total == 0) return payouts;

    for (const auto& [miner_id, count] : miner_shares) {
        uint64_t payout = (reward * count) / total;
        payouts[miner_id] = payout;
    }

    return payouts;
}

std::map<uint64_t, uint64_t> PayoutCalculator::CalculatePPS(
    const std::vector<Share>& shares,
    uint64_t expected_shares_per_block,
    uint64_t block_reward,
    double pool_fee)
{
    std::map<uint64_t, uint64_t> payouts;
    uint64_t fee = CalculateFee(block_reward, pool_fee);
    uint64_t reward_per_share = (block_reward - fee) / expected_shares_per_block;

    for (const auto& share : shares) {
        if (share.valid) {
            payouts[share.miner_id] += reward_per_share;
        }
    }

    return payouts;
}

std::map<uint64_t, uint64_t> PayoutCalculator::CalculateProportional(
    const std::vector<Share>& round_shares,
    uint64_t block_reward,
    double pool_fee)
{
    std::map<uint64_t, uint64_t> payouts;
    uint64_t fee = CalculateFee(block_reward, pool_fee);
    uint64_t reward = block_reward - fee;

    std::map<uint64_t, uint64_t> miner_shares;
    uint64_t total = 0;

    for (const auto& share : round_shares) {
        if (share.valid) {
            miner_shares[share.miner_id]++;
            total++;
        }
    }

    if (total == 0) return payouts;

    for (const auto& [miner_id, count] : miner_shares) {
        payouts[miner_id] = (reward * count) / total;
    }

    return payouts;
}

uint64_t PayoutCalculator::CalculateFee(uint64_t amount, double fee_percent) {
    return static_cast<uint64_t>(amount * fee_percent / 100.0);
}

// ============================================================================
// Hashrate Calculator
// ============================================================================

double HashrateCalculator::CalculateHashrate(const std::vector<Share>& shares,
                                             std::chrono::seconds window)
{
    auto now = std::chrono::system_clock::now();
    auto cutoff = now - window;

    uint64_t total_difficulty = 0;
    size_t count = 0;

    for (const auto& share : shares) {
        if (share.timestamp >= cutoff && share.valid) {
            total_difficulty += share.difficulty;
            count++;
        }
    }

    if (count == 0 || window.count() == 0) return 0.0;

    // Hashrate = (shares * difficulty * 2^32) / time
    return (total_difficulty * 4294967296.0) / window.count();
}

double HashrateCalculator::CalculateHashrateFromDifficulty(uint64_t difficulty,
                                                           std::chrono::seconds time)
{
    if (time.count() == 0) return 0.0;
    return (difficulty * 4294967296.0) / time.count();
}

std::chrono::seconds HashrateCalculator::EstimateBlockTime(double pool_hashrate,
                                                           uint64_t network_difficulty)
{
    if (pool_hashrate == 0.0) {
        return std::chrono::seconds(std::numeric_limits<int64_t>::max());
    }

    double expected_hashes = network_difficulty * 4294967296.0;
    int64_t seconds = static_cast<int64_t>(expected_hashes / pool_hashrate);

    return std::chrono::seconds(seconds);
}

uint64_t HashrateCalculator::CalculateExpectedShares(uint64_t network_difficulty,
                                                     uint64_t share_difficulty)
{
    if (share_difficulty == 0) return 0;
    return network_difficulty / share_difficulty;
}

// ============================================================================
// Utility Functions
// ============================================================================

std::string ToString(PoolConfig::PayoutMethod method) {
    switch (method) {
        case PoolConfig::PayoutMethod::PPLNS: return "PPLNS";
        case PoolConfig::PayoutMethod::PPS: return "PPS";
        case PoolConfig::PayoutMethod::PROP: return "Proportional";
        case PoolConfig::PayoutMethod::SOLO: return "Solo";
        default: return "Unknown";
    }
}

Result<stratum::Message> ParseStratumMessage(const std::string& json) {
    // Parse JSON string using RPC JSON parser
    auto json_result = rpc::JSONValue::Parse(json);
    if (json_result.IsError()) {
        return Result<stratum::Message>::Error("Invalid JSON: " +
                                               std::string(json_result.error));
    }

    const auto& json_obj = *json_result.value;
    if (!json_obj.IsObject()) {
        return Result<stratum::Message>::Error("JSON must be an object");
    }

    stratum::Message msg;
    msg.type = stratum::MessageType::UNKNOWN;

    // Parse ID (can be null for notifications)
    if (json_obj.HasKey("id")) {
        const auto& id_val = json_obj["id"];
        if (id_val.IsNumber()) {
            msg.id = static_cast<uint64_t>(id_val.GetInt());
        }
    }

    // Parse method (for requests and notifications)
    if (json_obj.HasKey("method")) {
        msg.method = json_obj["method"].GetString();

        // Determine message type from method
        if (msg.method == "mining.subscribe") {
            msg.type = stratum::MessageType::SUBSCRIBE;
        } else if (msg.method == "mining.authorize") {
            msg.type = stratum::MessageType::AUTHORIZE;
        } else if (msg.method == "mining.submit") {
            msg.type = stratum::MessageType::SUBMIT;
        } else if (msg.method == "mining.notify") {
            msg.type = stratum::MessageType::NOTIFY;
        } else if (msg.method == "mining.set_difficulty") {
            msg.type = stratum::MessageType::SET_DIFFICULTY;
        } else if (msg.method == "mining.set_extranonce") {
            msg.type = stratum::MessageType::SET_EXTRANONCE;
        } else if (msg.method == "client.get_version") {
            msg.type = stratum::MessageType::GET_VERSION;
        } else if (msg.method == "client.show_message") {
            msg.type = stratum::MessageType::SHOW_MESSAGE;
        } else if (msg.method == "client.reconnect") {
            msg.type = stratum::MessageType::RECONNECT;
        }
    }

    // Parse params (array of strings)
    if (json_obj.HasKey("params")) {
        const auto& params_val = json_obj["params"];
        if (params_val.IsArray()) {
            const auto& params_arr = params_val.GetArray();
            for (const auto& param : params_arr) {
                if (param.IsString()) {
                    msg.params.push_back(param.GetString());
                } else if (param.IsNumber()) {
                    msg.params.push_back(std::to_string(param.GetInt()));
                } else if (param.IsBool()) {
                    msg.params.push_back(param.GetBool() ? "true" : "false");
                }
            }
        }
    }

    // Parse result (for responses)
    if (json_obj.HasKey("result")) {
        const auto& result_val = json_obj["result"];
        if (result_val.IsString()) {
            msg.result = result_val.GetString();
        } else if (result_val.IsBool()) {
            msg.result = result_val.GetBool() ? "true" : "false";
        } else if (result_val.IsNull()) {
            msg.result = "null";
        } else {
            msg.result = result_val.ToJSONString();
        }
    }

    // Parse error (for error responses)
    if (json_obj.HasKey("error")) {
        const auto& error_val = json_obj["error"];
        if (!error_val.IsNull()) {
            if (error_val.IsString()) {
                msg.error = error_val.GetString();
            } else if (error_val.IsArray()) {
                // Stratum error format: [error_code, "error_message", null]
                const auto& err_arr = error_val.GetArray();
                if (err_arr.size() >= 2) {
                    msg.error = err_arr[1].GetString();
                }
            } else {
                msg.error = error_val.ToJSONString();
            }
        }
    }

    return Result<stratum::Message>::Ok(msg);
}

std::string FormatStratumResponse(const stratum::Message& msg) {
    // Build JSON response using RPC JSON builder
    rpc::JSONValue response;
    response.type = rpc::JSONType::Object;

    // Add ID (required for responses, null for notifications)
    if (msg.id != 0 || !msg.method.empty()) {
        response["id"] = rpc::JSONValue(static_cast<int64_t>(msg.id));
    } else {
        response["id"] = rpc::JSONValue();  // null
    }

    // If this is a notification (has method but response format)
    if (!msg.method.empty()) {
        response["method"] = rpc::JSONValue(msg.method);

        // Add params for notifications
        if (!msg.params.empty()) {
            std::vector<rpc::JSONValue> params_arr;
            for (const auto& param : msg.params) {
                params_arr.push_back(rpc::JSONValue(param));
            }
            response["params"] = rpc::JSONValue(params_arr);
        }
    }

    // Add result (for successful responses)
    if (msg.result.has_value()) {
        const std::string& result_str = *msg.result;

        // Try to parse result as JSON if it looks like JSON
        if (!result_str.empty() && (result_str[0] == '{' || result_str[0] == '[')) {
            auto result_json = rpc::JSONValue::Parse(result_str);
            if (result_json.IsOk()) {
                response["result"] = *result_json.value;
            } else {
                response["result"] = rpc::JSONValue(result_str);
            }
        } else if (result_str == "true") {
            response["result"] = rpc::JSONValue(true);
        } else if (result_str == "false") {
            response["result"] = rpc::JSONValue(false);
        } else if (result_str == "null") {
            response["result"] = rpc::JSONValue();  // null
        } else {
            response["result"] = rpc::JSONValue(result_str);
        }
    } else {
        response["result"] = rpc::JSONValue();  // null
    }

    // Add error (Stratum format: [error_code, "message", null])
    if (msg.error.has_value()) {
        std::vector<rpc::JSONValue> error_arr;
        error_arr.push_back(rpc::JSONValue(static_cast<int64_t>(20)));  // Generic error code
        error_arr.push_back(rpc::JSONValue(*msg.error));
        error_arr.push_back(rpc::JSONValue());  // null traceback
        response["error"] = rpc::JSONValue(error_arr);
    } else {
        response["error"] = rpc::JSONValue();  // null
    }

    return response.ToJSONString();
}

uint64_t CalculateShareDifficulty(const uint256& hash) {
    // Calculate difficulty from hash using Bitcoin's pool difficulty formula
    // difficulty = difficulty_1_target / hash
    //
    // Pool difficulty 1 target (pdiff):
    // 0x00000000FFFF0000000000000000000000000000000000000000000000000000
    //
    // This is the target for 1 difficulty share (roughly 2^32 hashes on average)

    // Count leading zero bits in hash
    size_t leading_zeros = 0;
    for (size_t i = 0; i < 32; i++) {
        uint8_t byte = hash.data()[31 - i];  // Start from most significant byte
        if (byte == 0) {
            leading_zeros += 8;
        } else {
            // Count leading zeros in this byte
            for (int bit = 7; bit >= 0; bit--) {
                if ((byte & (1 << bit)) == 0) {
                    leading_zeros++;
                } else {
                    break;
                }
            }
            break;
        }
    }

    // Calculate difficulty based on leading zeros
    // Each leading zero bit roughly doubles the difficulty
    // Base difficulty (pool difficulty 1) has ~32 leading zeros (0x00000000FFFF...)
    //
    // For a more accurate calculation:
    // difficulty = 2^(leading_zeros - 32) * adjustment_factor
    //
    // Simplified formula: difficulty ≈ 2^(leading_zeros - 32) * 65536

    if (leading_zeros < 32) {
        // Hash doesn't meet even difficulty 1 (very unlikely in practice)
        return 1;
    }

    // Calculate difficulty: 2^(leading_zeros - 32) * 65536
    uint64_t difficulty = 65536ULL;  // Base multiplier (0xFFFF + 1)
    size_t extra_zeros = leading_zeros - 32;

    // Multiply by 2^extra_zeros (with overflow protection)
    if (extra_zeros > 0) {
        if (extra_zeros >= 48) {
            // Cap at maximum uint64_t to prevent overflow
            return UINT64_MAX;
        }
        difficulty <<= extra_zeros;
    }

    // Minimum difficulty of 1
    return std::max(difficulty, static_cast<uint64_t>(1));
}

uint256 GenerateJobID() {
    return GetRandomUint256();
}

} // namespace intcoin
