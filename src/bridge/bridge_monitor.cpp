// Copyright (c) 2024-2026 The INTcoin Core developers
// Distributed under the MIT software license

#include <intcoin/bridge_monitor.h>
#include <intcoin/bridge.h>
#include <intcoin/util.h>

#include <unordered_map>
#include <deque>
#include <mutex>
#include <algorithm>
#include <ctime>
#include <sstream>
#include <iomanip>

namespace intcoin {
namespace bridge {

// Helper functions
std::string AlertTypeToString(AlertType type) {
    switch (type) {
        case AlertType::SUPPLY_MISMATCH:      return "Supply Mismatch";
        case AlertType::VALIDATOR_OFFLINE:    return "Validator Offline";
        case AlertType::VALIDATOR_MALICIOUS:  return "Validator Malicious";
        case AlertType::UNUSUAL_VOLUME:       return "Unusual Volume";
        case AlertType::RAPID_WITHDRAWALS:    return "Rapid Withdrawals";
        case AlertType::FAILED_VALIDATION:    return "Failed Validation";
        case AlertType::THRESHOLD_VIOLATION:  return "Threshold Violation";
        case AlertType::DOUBLE_SPEND_ATTEMPT: return "Double Spend Attempt";
        case AlertType::INVALID_MERKLE_PROOF: return "Invalid Merkle Proof";
        case AlertType::EMERGENCY_PAUSE:      return "Emergency Pause";
        default: return "Unknown";
    }
}

std::string AlertSeverityToString(AlertSeverity severity) {
    switch (severity) {
        case AlertSeverity::INFO:      return "INFO";
        case AlertSeverity::WARNING:   return "WARNING";
        case AlertSeverity::CRITICAL:  return "CRITICAL";
        case AlertSeverity::EMERGENCY: return "EMERGENCY";
        default: return "UNKNOWN";
    }
}

// Implementation details
struct INTcoinBridgeMonitor::Impl {
    INTcoinBridge* bridge;
    bool is_initialized;
    std::mutex mutex;

    // Alert storage
    std::deque<BridgeAlert> alerts;
    const size_t max_alerts = 10000;  // Keep last 10k alerts
    AlertCallback alert_callback;

    // Validator tracking
    std::unordered_map<std::string, ValidatorStats> validator_stats;

    // Transaction volume tracking (last 24h)
    struct VolumeSnapshot {
        uint64_t deposits;
        uint64_t withdrawals;
        std::time_t timestamp;
    };
    std::deque<VolumeSnapshot> volume_history;
    const size_t max_volume_snapshots = 288;  // 24h at 5min intervals

    // Anomaly detection thresholds
    const uint64_t max_24h_volume = 1000000000000;  // 10,000 BTC equivalent
    const uint32_t max_withdrawals_per_hour = 100;
    const double min_validator_uptime = 0.95;  // 95%

    Impl() : bridge(nullptr), is_initialized(false) {}

    // Helper: Convert bytes to hex string
    std::string BytesToHex(const std::vector<uint8_t>& bytes) {
        std::ostringstream oss;
        oss << std::hex << std::setfill('0');
        for (uint8_t byte : bytes) {
            oss << std::setw(2) << static_cast<int>(byte);
        }
        return oss.str();
    }
};

INTcoinBridgeMonitor::INTcoinBridgeMonitor()
    : impl_(std::make_unique<Impl>()) {
}

INTcoinBridgeMonitor::~INTcoinBridgeMonitor() {
    if (impl_->is_initialized) {
        Shutdown();
    }
}

Result<void> INTcoinBridgeMonitor::Initialize(INTcoinBridge* bridge) {
    std::lock_guard<std::mutex> lock(impl_->mutex);

    if (impl_->is_initialized) {
        return Result<void>::Error("Monitor already initialized");
    }

    if (!bridge) {
        return Result<void>::Error("Bridge instance is null");
    }

    impl_->bridge = bridge;
    impl_->is_initialized = true;

    LogF(LogLevel::INFO, "Bridge Monitor: Initialized successfully");
    return Result<void>::Ok();
}

Result<void> INTcoinBridgeMonitor::Shutdown() {
    std::lock_guard<std::mutex> lock(impl_->mutex);

    if (!impl_->is_initialized) {
        return Result<void>::Error("Monitor not initialized");
    }

    impl_->is_initialized = false;
    impl_->bridge = nullptr;
    impl_->alerts.clear();
    impl_->validator_stats.clear();
    impl_->volume_history.clear();

    LogF(LogLevel::INFO, "Bridge Monitor: Shutdown complete");
    return Result<void>::Ok();
}

void INTcoinBridgeMonitor::RegisterAlertCallback(AlertCallback callback) {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    impl_->alert_callback = callback;
    LogF(LogLevel::INFO, "Bridge Monitor: Alert callback registered");
}

Result<bool> INTcoinBridgeMonitor::CheckSupplyConsistency(const WrappedToken& token) {
    std::lock_guard<std::mutex> lock(impl_->mutex);

    if (!impl_->is_initialized || !impl_->bridge) {
        return Result<bool>::Error("Monitor not initialized");
    }

    // In a real implementation, this would:
    // 1. Query the locked amount on the origin chain (via blockchain monitor)
    // 2. Compare with the minted wrapped token supply
    // 3. Flag any discrepancies

    // For now, we simulate the check
    uint64_t locked_amount = 0;  // Would query from blockchain
    uint64_t minted_amount = token.total_supply;

    bool consistent = (locked_amount == minted_amount);

    if (!consistent) {
        std::map<std::string, std::string> metadata;
        metadata["token"] = token.symbol;
        metadata["locked"] = std::to_string(locked_amount);
        metadata["minted"] = std::to_string(minted_amount);
        metadata["difference"] = std::to_string(
            static_cast<int64_t>(minted_amount) - static_cast<int64_t>(locked_amount)
        );

        TriggerAlert(
            AlertType::SUPPLY_MISMATCH,
            AlertSeverity::CRITICAL,
            "Supply mismatch detected for " + token.symbol,
            metadata
        );
    }

    return Result<bool>::Ok(consistent);
}

Result<std::vector<ValidatorStats>> INTcoinBridgeMonitor::GetValidatorStats() {
    std::lock_guard<std::mutex> lock(impl_->mutex);

    if (!impl_->is_initialized) {
        return Result<std::vector<ValidatorStats>>::Error("Monitor not initialized");
    }

    std::vector<ValidatorStats> stats;
    for (const auto& pair : impl_->validator_stats) {
        stats.push_back(pair.second);
    }

    return Result<std::vector<ValidatorStats>>::Ok(stats);
}

Result<bool> INTcoinBridgeMonitor::DetectAnomalies() {
    std::lock_guard<std::mutex> lock(impl_->mutex);

    if (!impl_->is_initialized) {
        return Result<bool>::Error("Monitor not initialized");
    }

    bool anomaly_detected = false;

    // Check 1: Unusual 24h volume
    uint64_t volume_24h = 0;
    std::time_t now = std::time(nullptr);
    std::time_t yesterday = now - (24 * 3600);

    for (const auto& snapshot : impl_->volume_history) {
        if (snapshot.timestamp >= yesterday) {
            volume_24h += snapshot.deposits + snapshot.withdrawals;
        }
    }

    if (volume_24h > impl_->max_24h_volume) {
        std::map<std::string, std::string> metadata;
        metadata["volume_24h"] = std::to_string(volume_24h);
        metadata["threshold"] = std::to_string(impl_->max_24h_volume);

        TriggerAlert(
            AlertType::UNUSUAL_VOLUME,
            AlertSeverity::WARNING,
            "Unusually high 24h volume detected",
            metadata
        );
        anomaly_detected = true;
    }

    // Check 2: Rapid withdrawals (last hour)
    uint32_t withdrawals_1h = 0;
    std::time_t one_hour_ago = now - 3600;

    for (const auto& snapshot : impl_->volume_history) {
        if (snapshot.timestamp >= one_hour_ago) {
            withdrawals_1h += snapshot.withdrawals;
        }
    }

    if (withdrawals_1h > impl_->max_withdrawals_per_hour) {
        std::map<std::string, std::string> metadata;
        metadata["withdrawals_1h"] = std::to_string(withdrawals_1h);
        metadata["threshold"] = std::to_string(impl_->max_withdrawals_per_hour);

        TriggerAlert(
            AlertType::RAPID_WITHDRAWALS,
            AlertSeverity::CRITICAL,
            "Rapid withdrawal activity detected",
            metadata
        );
        anomaly_detected = true;
    }

    // Check 3: Validator uptime
    for (const auto& pair : impl_->validator_stats) {
        const ValidatorStats& stats = pair.second;
        if (stats.uptime_percentage < impl_->min_validator_uptime) {
            std::map<std::string, std::string> metadata;
            metadata["validator"] = impl_->BytesToHex(stats.public_key).substr(0, 16);
            metadata["uptime"] = std::to_string(stats.uptime_percentage * 100.0) + "%";
            metadata["threshold"] = std::to_string(impl_->min_validator_uptime * 100.0) + "%";

            TriggerAlert(
                AlertType::VALIDATOR_OFFLINE,
                AlertSeverity::WARNING,
                "Validator has low uptime",
                metadata
            );
            anomaly_detected = true;
        }
    }

    return Result<bool>::Ok(anomaly_detected);
}

Result<BridgeHealthMetrics> INTcoinBridgeMonitor::GetHealthMetrics() {
    std::lock_guard<std::mutex> lock(impl_->mutex);

    if (!impl_->is_initialized || !impl_->bridge) {
        return Result<BridgeHealthMetrics>::Error("Monitor not initialized");
    }

    BridgeHealthMetrics metrics = {};

    // Get bridge config
    auto config_result = impl_->bridge->GetConfig();
    if (!config_result.IsOk()) {
        return Result<BridgeHealthMetrics>::Error("Failed to get bridge config");
    }
    const BridgeConfig& config = config_result.GetValue();

    // Populate metrics
    metrics.total_validators = config.total_validators;
    metrics.active_validators = config.total_validators;  // Simplified
    metrics.offline_validators = 0;

    // Calculate validator uptime
    double total_uptime = 0.0;
    if (!impl_->validator_stats.empty()) {
        for (const auto& pair : impl_->validator_stats) {
            total_uptime += pair.second.uptime_percentage;
        }
        metrics.avg_validator_uptime = total_uptime / impl_->validator_stats.size();
    } else {
        metrics.avg_validator_uptime = 1.0;  // 100% if no data
    }

    // Calculate 24h volume
    metrics.deposit_volume_24h = 0;
    metrics.withdrawal_volume_24h = 0;
    std::time_t now = std::time(nullptr);
    std::time_t yesterday = now - (24 * 3600);

    for (const auto& snapshot : impl_->volume_history) {
        if (snapshot.timestamp >= yesterday) {
            metrics.deposit_volume_24h += snapshot.deposits;
            metrics.withdrawal_volume_24h += snapshot.withdrawals;
        }
    }

    // Count alerts
    metrics.active_alerts = 0;
    metrics.critical_alerts = 0;
    std::time_t one_hour_ago = now - 3600;

    for (const auto& alert : impl_->alerts) {
        if (alert.timestamp >= one_hour_ago) {
            metrics.active_alerts++;
            if (alert.severity >= AlertSeverity::CRITICAL) {
                metrics.critical_alerts++;
            }
        }
    }

    metrics.emergency_paused = config.emergency_paused;
    metrics.supply_consistent = true;  // Would check all tokens
    metrics.last_health_check = now;

    // Placeholder values for supply (would query from blockchain)
    metrics.total_locked_btc = 0;
    metrics.total_minted_wbtc = 0;
    metrics.total_locked_eth = 0;
    metrics.total_minted_weth = 0;
    metrics.total_locked_ltc = 0;
    metrics.total_minted_wltc = 0;

    // Placeholder transaction counts
    metrics.total_deposits = 0;
    metrics.total_withdrawals = 0;
    metrics.pending_deposits = 0;
    metrics.pending_withdrawals = 0;
    metrics.failed_transactions = 0;

    return Result<BridgeHealthMetrics>::Ok(metrics);
}

Result<std::vector<BridgeAlert>> INTcoinBridgeMonitor::GetRecentAlerts(
    uint32_t count,
    std::optional<AlertSeverity> min_severity
) {
    std::lock_guard<std::mutex> lock(impl_->mutex);

    if (!impl_->is_initialized) {
        return Result<std::vector<BridgeAlert>>::Error("Monitor not initialized");
    }

    std::vector<BridgeAlert> result;

    // Iterate from most recent to oldest
    for (auto it = impl_->alerts.rbegin(); it != impl_->alerts.rend() && result.size() < count; ++it) {
        if (!min_severity || it->severity >= *min_severity) {
            result.push_back(*it);
        }
    }

    return Result<std::vector<BridgeAlert>>::Ok(result);
}

Result<uint32_t> INTcoinBridgeMonitor::ClearOldAlerts(uint32_t days_old) {
    std::lock_guard<std::mutex> lock(impl_->mutex);

    if (!impl_->is_initialized) {
        return Result<uint32_t>::Error("Monitor not initialized");
    }

    std::time_t cutoff = std::time(nullptr) - (days_old * 24 * 3600);
    uint32_t removed = 0;

    auto it = impl_->alerts.begin();
    while (it != impl_->alerts.end()) {
        if (it->timestamp < cutoff) {
            it = impl_->alerts.erase(it);
            removed++;
        } else {
            ++it;
        }
    }

    LogF(LogLevel::INFO, "Bridge Monitor: Cleared %u old alerts (>%u days)", removed, days_old);
    return Result<uint32_t>::Ok(removed);
}

Result<void> INTcoinBridgeMonitor::RunHealthCheck() {
    std::lock_guard<std::mutex> lock(impl_->mutex);

    if (!impl_->is_initialized) {
        return Result<void>::Error("Monitor not initialized");
    }

    LogF(LogLevel::INFO, "Bridge Monitor: Running health check...");

    // Check supply consistency (would check all registered tokens)
    // CheckSupplyConsistency() for each token

    // Check validator activity
    CheckValidatorActivity();

    // Check transaction volume
    CheckTransactionVolume();

    // Detect anomalies
    DetectAnomalies();

    LogF(LogLevel::INFO, "Bridge Monitor: Health check complete");
    return Result<void>::Ok();
}

Result<ValidatorStats> INTcoinBridgeMonitor::GetValidatorStatus(
    const std::vector<uint8_t>& validator_pubkey
) {
    std::lock_guard<std::mutex> lock(impl_->mutex);

    if (!impl_->is_initialized) {
        return Result<ValidatorStats>::Error("Monitor not initialized");
    }

    std::string key = impl_->BytesToHex(validator_pubkey);
    auto it = impl_->validator_stats.find(key);

    if (it == impl_->validator_stats.end()) {
        return Result<ValidatorStats>::Error("Validator not found");
    }

    return Result<ValidatorStats>::Ok(it->second);
}

// Private methods

void INTcoinBridgeMonitor::CheckValidatorActivity() {
    // Check each validator's recent activity
    std::time_t now = std::time(nullptr);
    std::time_t inactive_threshold = now - (3600 * 24);  // 24 hours

    for (const auto& pair : impl_->validator_stats) {
        const ValidatorStats& stats = pair.second;
        if (stats.last_active < inactive_threshold) {
            std::map<std::string, std::string> metadata;
            metadata["validator"] = impl_->BytesToHex(stats.public_key).substr(0, 16);
            metadata["last_active"] = std::to_string(stats.last_active);
            metadata["hours_inactive"] = std::to_string((now - stats.last_active) / 3600);

            TriggerAlert(
                AlertType::VALIDATOR_OFFLINE,
                AlertSeverity::WARNING,
                "Validator has been inactive for >24h",
                metadata
            );
        }
    }
}

void INTcoinBridgeMonitor::CheckTransactionVolume() {
    // Record current volume snapshot
    Impl::VolumeSnapshot snapshot;
    snapshot.timestamp = std::time(nullptr);
    snapshot.deposits = 0;      // Would query from bridge
    snapshot.withdrawals = 0;   // Would query from bridge

    impl_->volume_history.push_back(snapshot);

    // Keep only last 24h of snapshots
    while (impl_->volume_history.size() > impl_->max_volume_snapshots) {
        impl_->volume_history.pop_front();
    }
}

void INTcoinBridgeMonitor::CheckSupplyBalance() {
    // Would check all registered wrapped tokens
    // For each token, verify locked == minted
}

void INTcoinBridgeMonitor::TriggerAlert(
    AlertType type,
    AlertSeverity severity,
    const std::string& message,
    const std::map<std::string, std::string>& metadata
) {
    BridgeAlert alert;
    alert.type = type;
    alert.severity = severity;
    alert.message = message;
    alert.timestamp = std::time(nullptr);
    alert.metadata = metadata;

    // Add to alerts queue
    impl_->alerts.push_back(alert);

    // Limit alert queue size
    while (impl_->alerts.size() > impl_->max_alerts) {
        impl_->alerts.pop_front();
    }

    // Log alert
    LogF(
        severity >= AlertSeverity::CRITICAL ? LogLevel::ERROR : LogLevel::WARNING,
        "Bridge Alert [%s] %s: %s",
        AlertSeverityToString(severity).c_str(),
        AlertTypeToString(type).c_str(),
        message.c_str()
    );

    // Trigger callback if registered
    if (impl_->alert_callback) {
        impl_->alert_callback(alert);
    }

    // Auto-pause on emergency alerts
    if (severity == AlertSeverity::EMERGENCY && impl_->bridge) {
        impl_->bridge->EmergencyPause();
        LogF(LogLevel::ERROR, "Bridge Monitor: Emergency pause triggered by alert");
    }
}

} // namespace bridge
} // namespace intcoin
