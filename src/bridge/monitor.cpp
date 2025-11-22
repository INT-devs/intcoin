// Copyright (c) 2025 INTcoin Core (Maddison Lane)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "../../include/intcoin/bridge/monitor.h"
#include "../../include/intcoin/crypto/hash.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <ctime>
#include <chrono>

namespace intcoin {
namespace bridge {

//==============================================================================
// BridgeMonitor Implementation
//==============================================================================

BridgeMonitor::BridgeMonitor(BridgeManager* bridge_manager)
    : bridge_manager_(bridge_manager),
      running_(false),
      total_health_checks_(0),
      failed_health_checks_(0) {
    
    if (!bridge_manager_) {
        throw std::runtime_error("BridgeMonitor: bridge_manager cannot be null");
    }
}

BridgeMonitor::~BridgeMonitor() {
    stop();
}

bool BridgeMonitor::start() {
    if (running_) {
        return true;
    }
    
    std::cout << "Starting bridge monitor..." << std::endl;
    
    running_ = true;
    monitor_thread_ = std::make_unique<std::thread>([this]() { monitor_loop(); });
    
    return true;
}

void BridgeMonitor::stop() {
    if (!running_) {
        return;
    }
    
    std::cout << "Stopping bridge monitor..." << std::endl;
    
    running_ = false;
    
    if (monitor_thread_ && monitor_thread_->joinable()) {
        monitor_thread_->join();
    }
}

HealthCheck BridgeMonitor::check_bridge_health(ChainType chain) {
    auto start_time = std::chrono::steady_clock::now();
    
    HealthCheck check;
    check.chain = chain;
    check.timestamp = std::time(nullptr);
    check.status = HealthStatus::OFFLINE;
    check.response_time_ms = 0;
    check.chain_height = 0;
    check.sync_height = 0;
    check.pending_swaps = 0;
    check.status_message = "Bridge not found";
    
    auto bridge = bridge_manager_->get_bridge(chain);
    if (!bridge) {
        return check;
    }
    
    // Check if bridge is running
    if (!bridge->is_running()) {
        check.status = HealthStatus::OFFLINE;
        check.status_message = "Bridge offline";
        return check;
    }
    
    // Get bridge status
    BridgeStatus bridge_status = bridge->get_status();
    
    switch (bridge_status) {
        case BridgeStatus::ONLINE:
            check.status = HealthStatus::HEALTHY;
            check.status_message = "Bridge online and synced";
            break;
        case BridgeStatus::SYNCING:
            check.status = HealthStatus::DEGRADED;
            check.status_message = "Bridge syncing";
            break;
        case BridgeStatus::ERROR:
            check.status = HealthStatus::UNHEALTHY;
            check.status_message = "Bridge in error state";
            break;
        case BridgeStatus::OFFLINE:
            check.status = HealthStatus::OFFLINE;
            check.status_message = "Bridge offline";
            break;
    }
    
    // Get chain heights
    check.chain_height = bridge->get_chain_height();
    check.sync_height = bridge->get_sync_height();
    
    // Check sync lag
    if (check.chain_height > 0 && check.sync_height > 0) {
        uint32_t blocks_behind = check.chain_height - check.sync_height;
        
        if (blocks_behind > 100) {
            check.status = HealthStatus::UNHEALTHY;
            check.status_message = "Bridge significantly behind chain";
        } else if (blocks_behind > 10) {
            check.status = HealthStatus::DEGRADED;
            check.status_message = "Bridge slightly behind chain";
        }
    }
    
    // Calculate response time
    auto end_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    check.response_time_ms = static_cast<uint32_t>(duration.count());
    
    // Track health check
    std::lock_guard<std::mutex> lock(monitor_mutex_);
    total_health_checks_++;
    if (check.status == HealthStatus::UNHEALTHY || check.status == HealthStatus::OFFLINE) {
        failed_health_checks_++;
    }
    response_times_.push_back(check.response_time_ms);
    
    // Keep only last 1000 response times
    if (response_times_.size() > 1000) {
        response_times_.erase(response_times_.begin());
    }
    
    return check;
}

std::vector<HealthCheck> BridgeMonitor::check_all_bridges() {
    auto chains = bridge_manager_->get_available_chains();
    std::vector<HealthCheck> results;
    
    for (const auto& chain : chains) {
        results.push_back(check_bridge_health(chain));
    }
    
    return results;
}

HealthStatus BridgeMonitor::get_overall_health() const {
    auto chains = bridge_manager_->get_available_chains();
    
    if (chains.empty()) {
        return HealthStatus::OFFLINE;
    }
    
    uint32_t healthy = 0, degraded = 0, unhealthy = 0, offline = 0;
    
    for (const auto& chain : chains) {
        auto bridge = bridge_manager_->get_bridge(chain);
        if (!bridge || !bridge->is_running()) {
            offline++;
            continue;
        }
        
        BridgeStatus status = bridge->get_status();
        
        switch (status) {
            case BridgeStatus::ONLINE:
                healthy++;
                break;
            case BridgeStatus::SYNCING:
                degraded++;
                break;
            case BridgeStatus::ERROR:
                unhealthy++;
                break;
            case BridgeStatus::OFFLINE:
                offline++;
                break;
        }
    }
    
    // Overall health is worst of all bridges
    if (unhealthy > 0 || offline == chains.size()) {
        return HealthStatus::UNHEALTHY;
    } else if (degraded > 0 || offline > 0) {
        return HealthStatus::DEGRADED;
    } else {
        return HealthStatus::HEALTHY;
    }
}

PerformanceMetrics BridgeMonitor::get_metrics(ChainType chain) const {
    std::lock_guard<std::mutex> lock(monitor_mutex_);
    
    auto it = metrics_.find(chain);
    if (it != metrics_.end()) {
        return it->second;
    }
    
    // Return empty metrics if not found
    PerformanceMetrics metrics;
    metrics.chain = chain;
    metrics.timestamp = std::time(nullptr);
    metrics.avg_swap_time_sec = 0;
    metrics.avg_confirmation_time_sec = 0;
    metrics.avg_sync_time_sec = 0;
    metrics.success_rate_24h = 0.0;
    metrics.success_rate_7d = 0.0;
    metrics.total_swaps_24h = 0;
    metrics.failed_swaps_24h = 0;
    metrics.volume_24h = 0;
    metrics.volume_7d = 0;
    metrics.volume_30d = 0;
    metrics.blocks_behind = 0;
    metrics.is_syncing = false;
    
    return metrics;
}

std::map<ChainType, PerformanceMetrics> BridgeMonitor::get_all_metrics() const {
    std::lock_guard<std::mutex> lock(monitor_mutex_);
    return metrics_;
}

std::vector<MonitorAlert> BridgeMonitor::get_active_alerts() const {
    std::lock_guard<std::mutex> lock(monitor_mutex_);
    
    std::vector<MonitorAlert> active;
    for (const auto& alert : alerts_) {
        if (!alert.acknowledged) {
            active.push_back(alert);
        }
    }
    
    return active;
}

std::vector<MonitorAlert> BridgeMonitor::get_alerts_by_severity(AlertSeverity severity) const {
    std::lock_guard<std::mutex> lock(monitor_mutex_);
    
    std::vector<MonitorAlert> filtered;
    for (const auto& alert : alerts_) {
        if (!alert.acknowledged && alert.severity == severity) {
            filtered.push_back(alert);
        }
    }
    
    return filtered;
}

bool BridgeMonitor::acknowledge_alert(const Hash256& alert_id) {
    std::lock_guard<std::mutex> lock(monitor_mutex_);
    
    for (auto& alert : alerts_) {
        if (alert.alert_id == alert_id) {
            alert.acknowledged = true;
            return true;
        }
    }
    
    return false;
}

void BridgeMonitor::clear_acknowledged_alerts() {
    std::lock_guard<std::mutex> lock(monitor_mutex_);
    
    alerts_.erase(
        std::remove_if(alerts_.begin(), alerts_.end(),
                      [](const MonitorAlert& a) { return a.acknowledged; }),
        alerts_.end()
    );
}

std::vector<Anomaly> BridgeMonitor::detect_anomalies(ChainType chain) {
    std::vector<Anomaly> detected;
    
    auto bridge = bridge_manager_->get_bridge(chain);
    if (!bridge) {
        return detected;
    }
    
    auto stats = bridge->get_stats();
    
    // Check for high failure rate
    if (is_failure_rate_high(chain, 1.0 - stats.success_rate)) {
        Anomaly anomaly;
        anomaly.chain = chain;
        anomaly.type = "HIGH_FAILURE_RATE";
        anomaly.severity_score = (1.0 - stats.success_rate) * 100.0;
        anomaly.description = "Unusually high swap failure rate detected";
        anomaly.timestamp = std::time(nullptr);
        detected.push_back(anomaly);
    }
    
    // Check for volume anomalies
    if (is_volume_anomalous(chain, stats.total_volume_sent + stats.total_volume_received)) {
        Anomaly anomaly;
        anomaly.chain = chain;
        anomaly.type = "UNUSUAL_VOLUME";
        anomaly.severity_score = 50.0;
        anomaly.description = "Unusual transaction volume detected";
        anomaly.timestamp = std::time(nullptr);
        detected.push_back(anomaly);
    }
    
    return detected;
}

std::vector<Anomaly> BridgeMonitor::get_recent_anomalies(uint32_t max_age_seconds) const {
    std::lock_guard<std::mutex> lock(monitor_mutex_);
    
    uint64_t cutoff = std::time(nullptr) - max_age_seconds;
    std::vector<Anomaly> recent;
    
    for (const auto& anomaly : anomalies_) {
        if (anomaly.timestamp >= cutoff) {
            recent.push_back(anomaly);
        }
    }
    
    return recent;
}

void BridgeMonitor::register_alert_callback(AlertCallback callback) {
    std::lock_guard<std::mutex> lock(monitor_mutex_);
    callbacks_.push_back(callback);
}

BridgeMonitor::MonitorStats BridgeMonitor::get_stats() const {
    std::lock_guard<std::mutex> lock(monitor_mutex_);
    
    MonitorStats stats;
    stats.total_health_checks = total_health_checks_;
    stats.failed_health_checks = failed_health_checks_;
    stats.total_anomalies_detected = anomalies_.size();
    
    // Count recent alerts
    uint64_t cutoff = std::time(nullptr) - 86400;  // 24 hours
    stats.total_alerts_24h = 0;
    stats.critical_alerts_24h = 0;
    
    for (const auto& alert : alerts_) {
        if (alert.timestamp >= cutoff) {
            stats.total_alerts_24h++;
            if (alert.severity == AlertSeverity::CRITICAL) {
                stats.critical_alerts_24h++;
            }
        }
    }
    
    // Calculate average response time
    if (!response_times_.empty()) {
        uint64_t sum = 0;
        for (uint32_t rt : response_times_) {
            sum += rt;
        }
        stats.avg_response_time_ms = static_cast<double>(sum) / response_times_.size();
    } else {
        stats.avg_response_time_ms = 0.0;
    }
    
    return stats;
}

void BridgeMonitor::monitor_loop() {
    std::cout << "Bridge monitor loop started" << std::endl;
    
    while (running_) {
        try {
            check_all_bridges_health();
            check_swap_timeouts();
            check_sync_status();
            update_performance_metrics();
            detect_volume_anomalies();
        } catch (const std::exception& e) {
            std::cerr << "Monitor loop error: " << e.what() << std::endl;
        }
        
        // Monitor every 30 seconds
        std::this_thread::sleep_for(std::chrono::seconds(30));
    }
    
    std::cout << "Bridge monitor loop stopped" << std::endl;
}

void BridgeMonitor::check_all_bridges_health() {
    auto checks = check_all_bridges();
    
    for (const auto& check : checks) {
        if (check.status == HealthStatus::OFFLINE) {
            create_alert(AlertType::BRIDGE_DOWN, AlertSeverity::CRITICAL,
                        check.chain, "Bridge is offline");
        } else if (check.status == HealthStatus::UNHEALTHY) {
            create_alert(AlertType::CHAIN_SYNC_FAILED, AlertSeverity::ERROR,
                        check.chain, check.status_message);
        }
    }
}

void BridgeMonitor::check_swap_timeouts() {
    // In production: query swap manager for pending swaps near timeout
    // For now, placeholder
}

void BridgeMonitor::check_sync_status() {
    auto chains = bridge_manager_->get_available_chains();
    
    for (const auto& chain : chains) {
        auto bridge = bridge_manager_->get_bridge(chain);
        if (!bridge || !bridge->is_running()) {
            continue;
        }
        
        uint32_t chain_height = bridge->get_chain_height();
        uint32_t sync_height = bridge->get_sync_height();
        
        if (chain_height > 0 && sync_height > 0) {
            uint32_t blocks_behind = chain_height - sync_height;
            
            if (blocks_behind > 100) {
                create_alert(AlertType::CHAIN_SYNC_FAILED, AlertSeverity::WARNING,
                            chain, "Bridge is " + std::to_string(blocks_behind) + " blocks behind");
            }
        }
    }
}

void BridgeMonitor::update_performance_metrics() {
    std::lock_guard<std::mutex> lock(monitor_mutex_);
    
    auto chains = bridge_manager_->get_available_chains();
    
    for (const auto& chain : chains) {
        auto bridge = bridge_manager_->get_bridge(chain);
        if (!bridge) {
            continue;
        }
        
        auto stats = bridge->get_stats();
        
        PerformanceMetrics metrics;
        metrics.chain = chain;
        metrics.timestamp = std::time(nullptr);
        metrics.avg_swap_time_sec = 300;  // Placeholder
        metrics.avg_confirmation_time_sec = 600;  // Placeholder
        metrics.avg_sync_time_sec = 60;  // Placeholder
        metrics.success_rate_24h = stats.success_rate;
        metrics.success_rate_7d = stats.success_rate;  // Would need historical data
        metrics.total_swaps_24h = stats.total_swaps;
        metrics.failed_swaps_24h = stats.failed_swaps;
        metrics.volume_24h = stats.total_volume_sent + stats.total_volume_received;
        metrics.volume_7d = metrics.volume_24h;  // Placeholder
        metrics.volume_30d = metrics.volume_24h;  // Placeholder
        
        uint32_t chain_height = bridge->get_chain_height();
        uint32_t sync_height = bridge->get_sync_height();
        metrics.blocks_behind = (chain_height > sync_height) ? (chain_height - sync_height) : 0;
        metrics.is_syncing = (bridge->get_status() == BridgeStatus::SYNCING);
        
        metrics_[chain] = metrics;
    }
}

void BridgeMonitor::detect_volume_anomalies() {
    auto chains = bridge_manager_->get_available_chains();
    
    for (const auto& chain : chains) {
        auto anomalies = detect_anomalies(chain);
        
        if (!anomalies.empty()) {
            std::lock_guard<std::mutex> lock(monitor_mutex_);
            for (auto& anomaly : anomalies) {
                // Generate anomaly ID
                crypto::SHA256 hasher;
                hasher.update(reinterpret_cast<const uint8_t*>(&anomaly.timestamp), sizeof(anomaly.timestamp));
                hasher.finalize(anomaly.anomaly_id.data());
                
                anomalies_.push_back(anomaly);
                
                // Create alert for significant anomalies
                if (anomaly.severity_score > 70.0) {
                    create_alert(AlertType::UNUSUAL_VOLUME, AlertSeverity::WARNING,
                                chain, anomaly.description);
                }
            }
        }
    }
}

void BridgeMonitor::create_alert(AlertType type, AlertSeverity severity, 
                                 ChainType chain, const std::string& message) {
    std::lock_guard<std::mutex> lock(monitor_mutex_);
    
    MonitorAlert alert;
    
    // Generate alert ID
    crypto::SHA256 hasher;
    uint64_t timestamp = std::time(nullptr);
    hasher.update(reinterpret_cast<const uint8_t*>(&timestamp), sizeof(timestamp));
    hasher.update(reinterpret_cast<const uint8_t*>(&type), sizeof(type));
    hasher.finalize(alert.alert_id.data());
    
    alert.type = type;
    alert.severity = severity;
    alert.chain = chain;
    alert.message = message;
    alert.timestamp = timestamp;
    alert.acknowledged = false;
    
    alerts_.push_back(alert);
    
    // Notify callbacks
    notify_callbacks(alert);
    
    // Log critical alerts
    if (severity == AlertSeverity::CRITICAL) {
        std::cerr << "CRITICAL ALERT: " << message << " on " 
                  << BridgeUtils::chain_type_to_string(chain) << std::endl;
    }
}

void BridgeMonitor::notify_callbacks(const MonitorAlert& alert) {
    for (const auto& callback : callbacks_) {
        try {
            callback(alert);
        } catch (const std::exception& e) {
            std::cerr << "Alert callback error: " << e.what() << std::endl;
        }
    }
}

bool BridgeMonitor::is_volume_anomalous(ChainType chain, uint64_t current_volume) {
    // Simple threshold-based detection
    // In production: use statistical models (z-score, moving average, etc.)
    
    std::lock_guard<std::mutex> lock(monitor_mutex_);
    
    auto it = metrics_.find(chain);
    if (it == metrics_.end()) {
        return false;
    }
    
    uint64_t avg_volume = it->second.volume_24h;
    
    // Alert if volume is 10x normal or near zero when it should be active
    if (current_volume > avg_volume * 10) {
        return true;
    }
    
    return false;
}

bool BridgeMonitor::is_failure_rate_high(ChainType chain, double failure_rate) {
    // Alert if failure rate exceeds 10%
    return failure_rate > 0.10;
}

//==============================================================================
// BridgeAnalytics Implementation
//==============================================================================

BridgeAnalytics::BridgeAnalytics() {
}

void BridgeAnalytics::record_swap(ChainType chain, bool success, uint64_t amount, uint32_t duration_sec) {
    std::lock_guard<std::mutex> lock(analytics_mutex_);
    
    SwapRecord record;
    record.chain = chain;
    record.success = success;
    record.amount = amount;
    record.duration_sec = duration_sec;
    record.timestamp = std::time(nullptr);
    
    swap_records_.push_back(record);
    
    // Cleanup old records (keep last 30 days)
    cleanup_old_records(30 * 86400);
}

void BridgeAnalytics::record_sync(ChainType chain, uint32_t blocks_synced, uint32_t duration_sec) {
    std::lock_guard<std::mutex> lock(analytics_mutex_);
    
    SyncRecord record;
    record.chain = chain;
    record.blocks_synced = blocks_synced;
    record.duration_sec = duration_sec;
    record.timestamp = std::time(nullptr);
    
    sync_records_.push_back(record);
    
    cleanup_old_records(30 * 86400);
}

void BridgeAnalytics::record_health_check(const HealthCheck& check) {
    std::lock_guard<std::mutex> lock(analytics_mutex_);
    health_records_.push_back(check);
    cleanup_old_records(7 * 86400);  // Keep 7 days of health checks
}

BridgeAnalytics::SwapReport BridgeAnalytics::generate_swap_report(ChainType chain, uint64_t period_seconds) const {
    std::lock_guard<std::mutex> lock(analytics_mutex_);
    
    uint64_t cutoff = std::time(nullptr) - period_seconds;
    
    SwapReport report;
    report.chain = chain;
    report.period_start = cutoff;
    report.period_end = std::time(nullptr);
    report.total_swaps = 0;
    report.successful_swaps = 0;
    report.failed_swaps = 0;
    report.success_rate = 0.0;
    report.total_volume = 0;
    report.avg_duration_sec = 0;
    
    uint64_t total_duration = 0;
    
    for (const auto& record : swap_records_) {
        if (record.chain == chain && record.timestamp >= cutoff) {
            report.total_swaps++;
            if (record.success) {
                report.successful_swaps++;
            } else {
                report.failed_swaps++;
            }
            report.total_volume += record.amount;
            total_duration += record.duration_sec;
        }
    }
    
    if (report.total_swaps > 0) {
        report.success_rate = static_cast<double>(report.successful_swaps) / 
                             static_cast<double>(report.total_swaps);
        report.avg_duration_sec = static_cast<uint32_t>(total_duration / report.total_swaps);
    }
    
    return report;
}

std::map<ChainType, BridgeAnalytics::SwapReport> BridgeAnalytics::generate_all_swap_reports(uint64_t period_seconds) const {
    std::map<ChainType, SwapReport> reports;
    
    // Get unique chains from records
    std::set<ChainType> chains;
    {
        std::lock_guard<std::mutex> lock(analytics_mutex_);
        for (const auto& record : swap_records_) {
            chains.insert(record.chain);
        }
    }
    
    for (const auto& chain : chains) {
        reports[chain] = generate_swap_report(chain, period_seconds);
    }
    
    return reports;
}

BridgeAnalytics::SyncReport BridgeAnalytics::generate_sync_report(ChainType chain, uint64_t period_seconds) const {
    std::lock_guard<std::mutex> lock(analytics_mutex_);
    
    uint64_t cutoff = std::time(nullptr) - period_seconds;
    
    SyncReport report;
    report.total_syncs = 0;
    report.total_blocks_synced = 0;
    report.avg_blocks_per_sync = 0;
    report.avg_sync_duration_sec = 0;
    report.uptime_percentage = 0.0;
    report.chain = chain;
    
    uint64_t total_duration = 0;
    
    for (const auto& record : sync_records_) {
        if (record.chain == chain && record.timestamp >= cutoff) {
            report.total_syncs++;
            report.total_blocks_synced += record.blocks_synced;
            total_duration += record.duration_sec;
        }
    }
    
    if (report.total_syncs > 0) {
        report.avg_blocks_per_sync = report.total_blocks_synced / report.total_syncs;
        report.avg_sync_duration_sec = static_cast<uint32_t>(total_duration / report.total_syncs);
    }
    
    // Calculate uptime from health checks
    uint32_t total_checks = 0;
    uint32_t healthy_checks = 0;
    
    for (const auto& check : health_records_) {
        if (check.chain == chain && check.timestamp >= cutoff) {
            total_checks++;
            if (check.status == HealthStatus::HEALTHY || check.status == HealthStatus::DEGRADED) {
                healthy_checks++;
            }
        }
    }
    
    if (total_checks > 0) {
        report.uptime_percentage = (static_cast<double>(healthy_checks) / total_checks) * 100.0;
    }
    
    return report;
}

std::string BridgeAnalytics::export_to_json(uint64_t period_seconds) const {
    auto reports = generate_all_swap_reports(period_seconds);
    
    std::ostringstream json;
    json << "{\"reports\":[";
    
    bool first = true;
    for (const auto& [chain, report] : reports) {
        if (!first) json << ",";
        first = false;
        
        json << "{";
        json << "\"chain\":\"" << BridgeUtils::chain_type_to_string(chain) << "\",";
        json << "\"total_swaps\":" << report.total_swaps << ",";
        json << "\"successful_swaps\":" << report.successful_swaps << ",";
        json << "\"failed_swaps\":" << report.failed_swaps << ",";
        json << "\"success_rate\":" << std::fixed << std::setprecision(4) << report.success_rate << ",";
        json << "\"total_volume\":" << report.total_volume << ",";
        json << "\"avg_duration_sec\":" << report.avg_duration_sec;
        json << "}";
    }
    
    json << "]}";
    return json.str();
}

std::string BridgeAnalytics::export_to_csv(uint64_t period_seconds) const {
    auto reports = generate_all_swap_reports(period_seconds);
    
    std::ostringstream csv;
    csv << "Chain,Total Swaps,Successful,Failed,Success Rate,Volume,Avg Duration\n";
    
    for (const auto& [chain, report] : reports) {
        csv << BridgeUtils::chain_type_to_string(chain) << ",";
        csv << report.total_swaps << ",";
        csv << report.successful_swaps << ",";
        csv << report.failed_swaps << ",";
        csv << std::fixed << std::setprecision(2) << (report.success_rate * 100) << "%,";
        csv << report.total_volume << ",";
        csv << report.avg_duration_sec << "\n";
    }
    
    return csv.str();
}

void BridgeAnalytics::cleanup_old_records(uint64_t max_age_seconds) {
    uint64_t cutoff = std::time(nullptr) - max_age_seconds;
    
    swap_records_.erase(
        std::remove_if(swap_records_.begin(), swap_records_.end(),
                      [cutoff](const SwapRecord& r) { return r.timestamp < cutoff; }),
        swap_records_.end()
    );
    
    sync_records_.erase(
        std::remove_if(sync_records_.begin(), sync_records_.end(),
                      [cutoff](const SyncRecord& r) { return r.timestamp < cutoff; }),
        sync_records_.end()
    );
    
    health_records_.erase(
        std::remove_if(health_records_.begin(), health_records_.end(),
                      [cutoff](const HealthCheck& r) { return r.timestamp < cutoff; }),
        health_records_.end()
    );
}

//==============================================================================
// BridgeDashboard Implementation
//==============================================================================

BridgeDashboard::BridgeDashboard(BridgeMonitor* monitor, BridgeAnalytics* analytics)
    : monitor_(monitor), analytics_(analytics) {
}

BridgeDashboard::DashboardData BridgeDashboard::get_dashboard_data() const {
    DashboardData data;
    
    data.overall_health = monitor_->get_overall_health();
    data.bridges_total = 0;
    data.bridges_online = 0;
    
    auto checks = monitor_->check_all_bridges();
    for (const auto& check : checks) {
        data.bridges_total++;
        if (check.status == HealthStatus::HEALTHY || check.status == HealthStatus::DEGRADED) {
            data.bridges_online++;
        }
    }
    
    // Get metrics
    data.chain_metrics = monitor_->get_all_metrics();
    
    // Aggregate activity
    data.swaps_1h = 0;
    data.swaps_24h = 0;
    data.volume_24h_usd = 0;
    double total_success_rate = 0.0;
    uint32_t total_swap_time = 0;
    uint32_t num_chains = 0;
    
    for (const auto& [chain, metrics] : data.chain_metrics) {
        data.swaps_24h += metrics.total_swaps_24h;
        data.volume_24h_usd += metrics.volume_24h;  // Would convert to USD
        total_success_rate += metrics.success_rate_24h;
        total_swap_time += metrics.avg_swap_time_sec;
        num_chains++;
    }
    
    if (num_chains > 0) {
        data.avg_success_rate = total_success_rate / num_chains;
        data.avg_swap_time_sec = total_swap_time / num_chains;
    } else {
        data.avg_success_rate = 0.0;
        data.avg_swap_time_sec = 0;
    }
    
    // Get alerts
    auto alerts = monitor_->get_active_alerts();
    data.active_alerts = alerts.size();
    data.critical_alerts = 0;
    
    for (const auto& alert : alerts) {
        if (alert.severity == AlertSeverity::CRITICAL) {
            data.critical_alerts++;
        }
    }
    
    return data;
}

std::string BridgeDashboard::get_dashboard_json() const {
    auto data = get_dashboard_data();
    
    std::ostringstream json;
    json << "{";
    json << "\"overall_health\":\"" << monitoring::health_status_to_string(data.overall_health) << "\",";
    json << "\"bridges_online\":" << data.bridges_online << ",";
    json << "\"bridges_total\":" << data.bridges_total << ",";
    json << "\"swaps_24h\":" << data.swaps_24h << ",";
    json << "\"volume_24h_usd\":" << data.volume_24h_usd << ",";
    json << "\"active_alerts\":" << data.active_alerts << ",";
    json << "\"critical_alerts\":" << data.critical_alerts << ",";
    json << "\"avg_success_rate\":" << std::fixed << std::setprecision(2) << (data.avg_success_rate * 100) << ",";
    json << "\"avg_swap_time_sec\":" << data.avg_swap_time_sec;
    json << "}";
    
    return json.str();
}

//==============================================================================
// Utility Functions
//==============================================================================

namespace monitoring {

std::string health_status_to_string(HealthStatus status) {
    switch (status) {
        case HealthStatus::HEALTHY: return "HEALTHY";
        case HealthStatus::DEGRADED: return "DEGRADED";
        case HealthStatus::UNHEALTHY: return "UNHEALTHY";
        case HealthStatus::OFFLINE: return "OFFLINE";
        default: return "UNKNOWN";
    }
}

std::string alert_severity_to_string(AlertSeverity severity) {
    switch (severity) {
        case AlertSeverity::INFO: return "INFO";
        case AlertSeverity::WARNING: return "WARNING";
        case AlertSeverity::ERROR: return "ERROR";
        case AlertSeverity::CRITICAL: return "CRITICAL";
        default: return "UNKNOWN";
    }
}

std::string alert_type_to_string(AlertType type) {
    switch (type) {
        case AlertType::BRIDGE_DOWN: return "BRIDGE_DOWN";
        case AlertType::CHAIN_SYNC_FAILED: return "CHAIN_SYNC_FAILED";
        case AlertType::SWAP_TIMEOUT: return "SWAP_TIMEOUT";
        case AlertType::HIGH_FAILURE_RATE: return "HIGH_FAILURE_RATE";
        case AlertType::LOW_LIQUIDITY: return "LOW_LIQUIDITY";
        case AlertType::PROOF_VERIFICATION_FAILED: return "PROOF_VERIFICATION_FAILED";
        case AlertType::UNUSUAL_VOLUME: return "UNUSUAL_VOLUME";
        case AlertType::STUCK_TRANSACTION: return "STUCK_TRANSACTION";
        default: return "UNKNOWN";
    }
}

double calculate_uptime(uint32_t total_checks, uint32_t failed_checks) {
    if (total_checks == 0) return 0.0;
    uint32_t successful = total_checks - failed_checks;
    return (static_cast<double>(successful) / total_checks) * 100.0;
}

std::string format_duration(uint32_t seconds) {
    if (seconds < 60) {
        return std::to_string(seconds) + "s";
    } else if (seconds < 3600) {
        return std::to_string(seconds / 60) + "m " + std::to_string(seconds % 60) + "s";
    } else {
        uint32_t hours = seconds / 3600;
        uint32_t mins = (seconds % 3600) / 60;
        return std::to_string(hours) + "h " + std::to_string(mins) + "m";
    }
}

} // namespace monitoring

} // namespace bridge
} // namespace intcoin
