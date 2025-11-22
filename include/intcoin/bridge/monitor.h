// Copyright (c) 2025 INTcoin Core (Maddison Lane)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef INTCOIN_BRIDGE_MONITOR_H
#define INTCOIN_BRIDGE_MONITOR_H

#include "../types.h"
#include "bridge.h"
#include <memory>
#include <vector>
#include <map>
#include <optional>
#include <mutex>
#include <functional>
#include <thread>
#include <atomic>

namespace intcoin {
namespace bridge {

//==============================================================================
// Monitoring Data Structures
//==============================================================================

/**
 * Bridge health status
 */
enum class HealthStatus {
    HEALTHY,      // All systems operational
    DEGRADED,     // Some issues but functional
    UNHEALTHY,    // Significant problems
    OFFLINE       // Bridge not responding
};

/**
 * Alert severity levels
 */
enum class AlertSeverity {
    INFO,         // Informational
    WARNING,      // Potential issue
    ERROR,        // Definite problem
    CRITICAL      // Immediate action required
};

/**
 * Alert types
 */
enum class AlertType {
    BRIDGE_DOWN,
    CHAIN_SYNC_FAILED,
    SWAP_TIMEOUT,
    HIGH_FAILURE_RATE,
    LOW_LIQUIDITY,
    PROOF_VERIFICATION_FAILED,
    UNUSUAL_VOLUME,
    STUCK_TRANSACTION
};

/**
 * Monitoring alert
 */
struct MonitorAlert {
    Hash256 alert_id;
    AlertType type;
    AlertSeverity severity;
    ChainType chain;
    std::string message;
    uint64_t timestamp;
    bool acknowledged;
    std::string details;
};

/**
 * Bridge health check result
 */
struct HealthCheck {
    ChainType chain;
    HealthStatus status;
    uint64_t timestamp;
    uint32_t response_time_ms;
    uint32_t chain_height;
    uint32_t sync_height;
    uint32_t pending_swaps;
    std::string status_message;
};

/**
 * Performance metrics
 */
struct PerformanceMetrics {
    ChainType chain;
    uint64_t timestamp;
    
    // Timing metrics
    uint32_t avg_swap_time_sec;
    uint32_t avg_confirmation_time_sec;
    uint32_t avg_sync_time_sec;
    
    // Success metrics
    double success_rate_24h;
    double success_rate_7d;
    uint32_t total_swaps_24h;
    uint32_t failed_swaps_24h;
    
    // Volume metrics
    uint64_t volume_24h;
    uint64_t volume_7d;
    uint64_t volume_30d;
    
    // Chain metrics
    uint32_t blocks_behind;
    bool is_syncing;
};

/**
 * Anomaly detection result
 */
struct Anomaly {
    Hash256 anomaly_id;
    ChainType chain;
    std::string type;
    double severity_score;  // 0-100
    std::string description;
    uint64_t timestamp;
    std::map<std::string, std::string> metadata;
};

//==============================================================================
// BridgeMonitor
//==============================================================================

/**
 * Real-time bridge monitoring and alerting system
 */
class BridgeMonitor {
public:
    BridgeMonitor(BridgeManager* bridge_manager);
    ~BridgeMonitor();

    // Lifecycle
    bool start();
    void stop();
    bool is_running() const { return running_; }
    
    // Health checks
    HealthCheck check_bridge_health(ChainType chain);
    std::vector<HealthCheck> check_all_bridges();
    HealthStatus get_overall_health() const;
    
    // Performance monitoring
    PerformanceMetrics get_metrics(ChainType chain) const;
    std::map<ChainType, PerformanceMetrics> get_all_metrics() const;
    
    // Alert management
    std::vector<MonitorAlert> get_active_alerts() const;
    std::vector<MonitorAlert> get_alerts_by_severity(AlertSeverity severity) const;
    bool acknowledge_alert(const Hash256& alert_id);
    void clear_acknowledged_alerts();
    
    // Anomaly detection
    std::vector<Anomaly> detect_anomalies(ChainType chain);
    std::vector<Anomaly> get_recent_anomalies(uint32_t max_age_seconds = 3600) const;
    
    // Alert callbacks
    using AlertCallback = std::function<void(const MonitorAlert&)>;
    void register_alert_callback(AlertCallback callback);
    
    // Statistics
    struct MonitorStats {
        uint32_t total_alerts_24h;
        uint32_t critical_alerts_24h;
        uint32_t total_health_checks;
        uint32_t failed_health_checks;
        double avg_response_time_ms;
        uint32_t total_anomalies_detected;
    };
    
    MonitorStats get_stats() const;

private:
    BridgeManager* bridge_manager_;
    std::atomic<bool> running_;
    
    std::vector<MonitorAlert> alerts_;
    std::vector<Anomaly> anomalies_;
    std::map<ChainType, PerformanceMetrics> metrics_;
    std::vector<AlertCallback> callbacks_;
    
    mutable std::mutex monitor_mutex_;
    std::unique_ptr<std::thread> monitor_thread_;
    
    // Monitoring loop
    void monitor_loop();
    void check_all_bridges_health();
    void check_swap_timeouts();
    void check_sync_status();
    void update_performance_metrics();
    void detect_volume_anomalies();
    
    // Alert generation
    void create_alert(AlertType type, AlertSeverity severity, 
                     ChainType chain, const std::string& message);
    void notify_callbacks(const MonitorAlert& alert);
    
    // Anomaly detection algorithms
    bool is_volume_anomalous(ChainType chain, uint64_t current_volume);
    bool is_failure_rate_high(ChainType chain, double failure_rate);
    
    // Statistics tracking
    uint32_t total_health_checks_;
    uint32_t failed_health_checks_;
    std::vector<uint32_t> response_times_;
};

//==============================================================================
// BridgeAnalytics
//==============================================================================

/**
 * Historical analytics and reporting
 */
class BridgeAnalytics {
public:
    BridgeAnalytics();
    ~BridgeAnalytics() = default;

    // Data recording
    void record_swap(ChainType chain, bool success, uint64_t amount, uint32_t duration_sec);
    void record_sync(ChainType chain, uint32_t blocks_synced, uint32_t duration_sec);
    void record_health_check(const HealthCheck& check);
    
    // Reporting
    struct SwapReport {
        ChainType chain;
        uint64_t period_start;
        uint64_t period_end;
        uint32_t total_swaps;
        uint32_t successful_swaps;
        uint32_t failed_swaps;
        double success_rate;
        uint64_t total_volume;
        uint32_t avg_duration_sec;
    };
    
    SwapReport generate_swap_report(ChainType chain, uint64_t period_seconds) const;
    std::map<ChainType, SwapReport> generate_all_swap_reports(uint64_t period_seconds) const;
    
    struct SyncReport {
        ChainType chain;
        uint32_t total_syncs;
        uint32_t total_blocks_synced;
        uint32_t avg_blocks_per_sync;
        uint32_t avg_sync_duration_sec;
        double uptime_percentage;
    };
    
    SyncReport generate_sync_report(ChainType chain, uint64_t period_seconds) const;
    
    // Export
    std::string export_to_json(uint64_t period_seconds) const;
    std::string export_to_csv(uint64_t period_seconds) const;

private:
    struct SwapRecord {
        ChainType chain;
        bool success;
        uint64_t amount;
        uint32_t duration_sec;
        uint64_t timestamp;
    };
    
    struct SyncRecord {
        ChainType chain;
        uint32_t blocks_synced;
        uint32_t duration_sec;
        uint64_t timestamp;
    };
    
    std::vector<SwapRecord> swap_records_;
    std::vector<SyncRecord> sync_records_;
    std::vector<HealthCheck> health_records_;
    
    mutable std::mutex analytics_mutex_;
    
    void cleanup_old_records(uint64_t max_age_seconds);
};

//==============================================================================
// BridgeDashboard
//==============================================================================

/**
 * Real-time dashboard data provider
 */
class BridgeDashboard {
public:
    BridgeDashboard(BridgeMonitor* monitor, BridgeAnalytics* analytics);
    ~BridgeDashboard() = default;

    struct DashboardData {
        // Current status
        HealthStatus overall_health;
        uint32_t bridges_online;
        uint32_t bridges_total;
        
        // Recent activity
        uint32_t swaps_1h;
        uint32_t swaps_24h;
        uint64_t volume_24h_usd;
        
        // Alerts
        uint32_t active_alerts;
        uint32_t critical_alerts;
        
        // Performance
        double avg_success_rate;
        uint32_t avg_swap_time_sec;
        
        // Per-chain summary
        std::map<ChainType, PerformanceMetrics> chain_metrics;
    };
    
    DashboardData get_dashboard_data() const;
    std::string get_dashboard_json() const;

private:
    BridgeMonitor* monitor_;
    BridgeAnalytics* analytics_;
};

//==============================================================================
// Utility Functions
//==============================================================================

namespace monitoring {

/**
 * Convert health status to string
 */
std::string health_status_to_string(HealthStatus status);

/**
 * Convert alert severity to string
 */
std::string alert_severity_to_string(AlertSeverity severity);

/**
 * Convert alert type to string
 */
std::string alert_type_to_string(AlertType type);

/**
 * Calculate uptime percentage
 */
double calculate_uptime(uint32_t total_checks, uint32_t failed_checks);

/**
 * Format duration for display
 */
std::string format_duration(uint32_t seconds);

} // namespace monitoring

} // namespace bridge
} // namespace intcoin

#endif // INTCOIN_BRIDGE_MONITOR_H
