// Copyright (c) 2024-2026 The INTcoin Core developers
// Distributed under the MIT software license

#ifndef INTCOIN_BRIDGE_MONITOR_H
#define INTCOIN_BRIDGE_MONITOR_H

#include <intcoin/bridge.h>
#include <intcoin/util.h>

#include <string>
#include <vector>
#include <map>
#include <functional>
#include <cstdint>
#include <ctime>

namespace intcoin {
namespace bridge {

/// Alert severity levels
enum class AlertSeverity : uint8_t {
    INFO = 0,        // Informational message
    WARNING = 1,     // Warning - potential issue
    CRITICAL = 2,    // Critical - requires immediate attention
    EMERGENCY = 3    // Emergency - automatic pause triggered
};

/// Alert types
enum class AlertType : uint8_t {
    SUPPLY_MISMATCH = 0,         // Wrapped token supply doesn't match locked funds
    VALIDATOR_OFFLINE = 1,       // Validator hasn't signed recently
    VALIDATOR_MALICIOUS = 2,     // Validator signed conflicting proofs
    UNUSUAL_VOLUME = 3,          // Abnormally high transaction volume
    RAPID_WITHDRAWALS = 4,       // Many withdrawals in short time
    FAILED_VALIDATION = 5,       // Deposit/withdrawal validation failed
    THRESHOLD_VIOLATION = 6,     // Not enough validator signatures
    DOUBLE_SPEND_ATTEMPT = 7,    // Same deposit proof submitted twice
    INVALID_MERKLE_PROOF = 8,    // Merkle proof verification failed
    EMERGENCY_PAUSE = 9          // Emergency pause activated
};

/// Bridge alert
struct BridgeAlert {
    AlertType type;
    AlertSeverity severity;
    std::string message;
    std::time_t timestamp;
    std::map<std::string, std::string> metadata;  // Additional context
};

/// Validator activity statistics
struct ValidatorStats {
    std::vector<uint8_t> public_key;
    uint64_t total_signatures;
    uint64_t successful_signatures;
    uint64_t failed_signatures;
    uint64_t missed_signatures;
    std::time_t last_active;
    double uptime_percentage;
    double reputation_score;
};

/// Bridge health metrics
struct BridgeHealthMetrics {
    // Supply metrics
    uint64_t total_locked_btc;
    uint64_t total_minted_wbtc;
    uint64_t total_locked_eth;
    uint64_t total_minted_weth;
    uint64_t total_locked_ltc;
    uint64_t total_minted_wltc;
    bool supply_consistent;

    // Validator metrics
    uint32_t active_validators;
    uint32_t offline_validators;
    uint32_t total_validators;
    double avg_validator_uptime;

    // Transaction metrics
    uint64_t total_deposits;
    uint64_t total_withdrawals;
    uint64_t pending_deposits;
    uint64_t pending_withdrawals;
    uint64_t failed_transactions;

    // Volume metrics (24h)
    uint64_t deposit_volume_24h;
    uint64_t withdrawal_volume_24h;

    // Security metrics
    uint32_t active_alerts;
    uint32_t critical_alerts;
    bool emergency_paused;
    std::time_t last_health_check;
};

/// Alert callback function type
using AlertCallback = std::function<void(const BridgeAlert& alert)>;

/// Bridge monitoring system
class BridgeMonitor {
public:
    BridgeMonitor() = default;
    virtual ~BridgeMonitor() = default;

    /// Initialize monitor with bridge instance
    virtual Result<void> Initialize(INTcoinBridge* bridge) = 0;

    /// Shutdown monitor
    virtual Result<void> Shutdown() = 0;

    /// Register alert callback
    virtual void RegisterAlertCallback(AlertCallback callback) = 0;

    /// Check supply consistency
    /// Verifies that wrapped token supply matches locked funds on origin chains
    virtual Result<bool> CheckSupplyConsistency(const WrappedToken& token) = 0;

    /// Monitor validator activity
    /// Returns list of validators and their activity stats
    virtual Result<std::vector<ValidatorStats>> GetValidatorStats() = 0;

    /// Detect anomalies in transaction patterns
    /// Returns true if anomaly detected
    virtual Result<bool> DetectAnomalies() = 0;

    /// Get bridge health metrics
    virtual Result<BridgeHealthMetrics> GetHealthMetrics() = 0;

    /// Get recent alerts
    virtual Result<std::vector<BridgeAlert>> GetRecentAlerts(
        uint32_t count = 100,
        std::optional<AlertSeverity> min_severity = std::nullopt
    ) = 0;

    /// Clear old alerts
    virtual Result<uint32_t> ClearOldAlerts(uint32_t days_old = 30) = 0;

    /// Trigger manual health check
    virtual Result<void> RunHealthCheck() = 0;

    /// Check specific validator status
    virtual Result<ValidatorStats> GetValidatorStatus(
        const std::vector<uint8_t>& validator_pubkey
    ) = 0;
};

/// INTcoin bridge monitor implementation
class INTcoinBridgeMonitor : public BridgeMonitor {
public:
    INTcoinBridgeMonitor();
    ~INTcoinBridgeMonitor() override;

    Result<void> Initialize(INTcoinBridge* bridge) override;
    Result<void> Shutdown() override;
    void RegisterAlertCallback(AlertCallback callback) override;
    Result<bool> CheckSupplyConsistency(const WrappedToken& token) override;
    Result<std::vector<ValidatorStats>> GetValidatorStats() override;
    Result<bool> DetectAnomalies() override;
    Result<BridgeHealthMetrics> GetHealthMetrics() override;
    Result<std::vector<BridgeAlert>> GetRecentAlerts(
        uint32_t count,
        std::optional<AlertSeverity> min_severity
    ) override;
    Result<uint32_t> ClearOldAlerts(uint32_t days_old) override;
    Result<void> RunHealthCheck() override;
    Result<ValidatorStats> GetValidatorStatus(
        const std::vector<uint8_t>& validator_pubkey
    ) override;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;

    // Internal monitoring functions
    void CheckValidatorActivity();
    void CheckTransactionVolume();
    void CheckSupplyBalance();
    void TriggerAlert(AlertType type, AlertSeverity severity, const std::string& message,
                     const std::map<std::string, std::string>& metadata = {});
};

/// Helper functions
std::string AlertTypeToString(AlertType type);
std::string AlertSeverityToString(AlertSeverity severity);

} // namespace bridge
} // namespace intcoin

#endif // INTCOIN_BRIDGE_MONITOR_H
