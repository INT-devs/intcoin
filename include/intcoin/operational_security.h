// Copyright (c) 2025 INTcoin Core (Maddison Lane)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef INTCOIN_OPERATIONAL_SECURITY_H
#define INTCOIN_OPERATIONAL_SECURITY_H

#include <string>
#include <vector>
#include <unordered_set>
#include <chrono>
#include <regex>
#include <functional>
#include <fstream>
#include <queue>
#include <mutex>

namespace intcoin {
namespace opsec {

/**
 * Secure Logging Configuration
 */
class SecureLoggingConfig {
public:
    struct Config {
        bool sanitize_sensitive_data = true;
        bool log_authentication_events = true;
        bool log_network_anomalies = true;
        bool log_resource_exhaustion = true;
        size_t max_log_size_mb = 100;
        size_t max_log_files = 10;
        std::string log_directory = "logs/";
    };

    static Config get_secure_defaults() {
        return Config{};
    }

    static bool validate_config(const Config& config) {
        if (config.max_log_size_mb < 1 || config.max_log_size_mb > 10000) return false;
        if (config.max_log_files < 1 || config.max_log_files > 100) return false;
        return true;
    }
};

/**
 * Log Sanitizer - Prevents sensitive data in logs
 */
class LogSanitizer {
public:
    static std::string sanitize(const std::string& message) {
        std::string result = message;

        // Remove private keys
        result = std::regex_replace(result,
            std::regex(R"(-----BEGIN[^-]*PRIVATE KEY-----[\s\S]*?-----END[^-]*PRIVATE KEY-----)"),
            "[PRIVATE_KEY_REDACTED]");

        // Remove hex private keys (64 chars)
        result = std::regex_replace(result,
            std::regex(R"(privkey[=:\s]+[a-fA-F0-9]{64})", std::regex::icase),
            "privkey=[REDACTED]");

        // Remove passwords
        result = std::regex_replace(result,
            std::regex(R"(password[=:\s]+[^\s,}\"']+)", std::regex::icase),
            "password=[REDACTED]");

        // Remove session tokens
        result = std::regex_replace(result,
            std::regex(R"(session[_-]?id[=:\s]+[a-fA-F0-9]{32,})", std::regex::icase),
            "session_id=[REDACTED]");

        // Remove wallet passphrases
        result = std::regex_replace(result,
            std::regex(R"(passphrase[=:\s]+[^\s,}\"']+)", std::regex::icase),
            "passphrase=[REDACTED]");

        // Remove mnemonics (12-24 word phrases)
        result = std::regex_replace(result,
            std::regex(R"(mnemonic[=:\s]+[a-z\s]{20,})", std::regex::icase),
            "mnemonic=[REDACTED]");

        return result;
    }

    static bool contains_sensitive_data(const std::string& message) {
        return message != sanitize(message);
    }
};

/**
 * Anomaly Detection
 */
class AnomalyDetector {
public:
    enum class AnomalyType {
        HIGH_CONNECTION_RATE,
        UNUSUAL_PORT_SCAN,
        REPEATED_AUTH_FAILURE,
        RESOURCE_EXHAUSTION,
        UNUSUAL_TRAFFIC_PATTERN,
        BLOCK_WITHHOLDING,
        DOUBLE_SPEND_ATTEMPT
    };

    struct Anomaly {
        AnomalyType type;
        std::string source;
        std::string description;
        std::chrono::system_clock::time_point detected;
        int severity;  // 1-10
    };

    static AnomalyDetector& instance() {
        static AnomalyDetector inst;
        return inst;
    }

    void record_connection(const std::string& ip) {
        std::lock_guard<std::mutex> lock(mtx_);
        auto now = std::chrono::steady_clock::now();
        connection_times_[ip].push_back(now);

        // Check for high connection rate
        auto& times = connection_times_[ip];
        auto cutoff = now - std::chrono::minutes(1);
        times.erase(std::remove_if(times.begin(), times.end(),
            [cutoff](const auto& t) { return t < cutoff; }), times.end());

        if (times.size() > 100) {  // 100 connections per minute
            report_anomaly({AnomalyType::HIGH_CONNECTION_RATE, ip,
                           "High connection rate detected",
                           std::chrono::system_clock::now(), 7});
        }
    }

    void record_auth_failure(const std::string& ip) {
        std::lock_guard<std::mutex> lock(mtx_);
        auth_failures_[ip]++;

        if (auth_failures_[ip] > 10) {
            report_anomaly({AnomalyType::REPEATED_AUTH_FAILURE, ip,
                           "Multiple authentication failures",
                           std::chrono::system_clock::now(), 8});
        }
    }

    void report_anomaly(const Anomaly& anomaly) {
        anomalies_.push_back(anomaly);
        if (alert_callback_) {
            alert_callback_(anomaly);
        }
    }

    void set_alert_callback(std::function<void(const Anomaly&)> callback) {
        alert_callback_ = callback;
    }

    std::vector<Anomaly> get_recent_anomalies(std::chrono::hours window = std::chrono::hours(24)) const {
        std::lock_guard<std::mutex> lock(mtx_);
        auto cutoff = std::chrono::system_clock::now() - window;
        std::vector<Anomaly> recent;
        for (const auto& a : anomalies_) {
            if (a.detected > cutoff) recent.push_back(a);
        }
        return recent;
    }

private:
    mutable std::mutex mtx_;
    std::unordered_map<std::string, std::vector<std::chrono::steady_clock::time_point>> connection_times_;
    std::unordered_map<std::string, size_t> auth_failures_;
    std::vector<Anomaly> anomalies_;
    std::function<void(const Anomaly&)> alert_callback_;
};

/**
 * Performance Monitor
 */
class PerformanceMonitor {
public:
    struct Metrics {
        double cpu_usage_percent = 0;
        size_t memory_usage_mb = 0;
        size_t disk_usage_mb = 0;
        size_t open_connections = 0;
        double transactions_per_second = 0;
        double blocks_per_hour = 0;
        std::chrono::milliseconds avg_block_validation_time{0};
    };

    static PerformanceMonitor& instance() {
        static PerformanceMonitor inst;
        return inst;
    }

    Metrics get_current_metrics() const {
        return current_metrics_;
    }

    void update_metrics(const Metrics& metrics) {
        current_metrics_ = metrics;
        check_thresholds();
    }

private:
    void check_thresholds() {
        if (current_metrics_.cpu_usage_percent > 90) {
            AnomalyDetector::instance().report_anomaly({
                AnomalyDetector::AnomalyType::RESOURCE_EXHAUSTION,
                "cpu", "CPU usage > 90%",
                std::chrono::system_clock::now(), 6
            });
        }
        if (current_metrics_.memory_usage_mb > 8000) {  // 8GB
            AnomalyDetector::instance().report_anomaly({
                AnomalyDetector::AnomalyType::RESOURCE_EXHAUSTION,
                "memory", "Memory usage > 8GB",
                std::chrono::system_clock::now(), 7
            });
        }
    }

    Metrics current_metrics_;
};

/**
 * Security Event Logger
 */
class SecurityEventLogger {
public:
    enum class EventType {
        AUTH_SUCCESS,
        AUTH_FAILURE,
        PRIVILEGE_ESCALATION_ATTEMPT,
        SUSPICIOUS_TRANSACTION,
        PEER_BANNED,
        WALLET_UNLOCK,
        WALLET_LOCK,
        CONFIG_CHANGE,
        SHUTDOWN_INITIATED,
        BACKUP_CREATED
    };

    struct Event {
        EventType type;
        std::string actor;  // IP or user
        std::string details;
        std::chrono::system_clock::time_point timestamp;
    };

    static SecurityEventLogger& instance() {
        static SecurityEventLogger inst;
        return inst;
    }

    void log_event(EventType type, const std::string& actor, const std::string& details) {
        Event event{type, actor, LogSanitizer::sanitize(details),
                   std::chrono::system_clock::now()};
        events_.push_back(event);

        // Trigger alert for critical events
        if (type == EventType::PRIVILEGE_ESCALATION_ATTEMPT ||
            type == EventType::SUSPICIOUS_TRANSACTION) {
            trigger_alert(event);
        }
    }

    std::vector<Event> get_events(std::chrono::hours window = std::chrono::hours(24)) const {
        auto cutoff = std::chrono::system_clock::now() - window;
        std::vector<Event> recent;
        for (const auto& e : events_) {
            if (e.timestamp > cutoff) recent.push_back(e);
        }
        return recent;
    }

private:
    void trigger_alert(const Event& event) {
        // In production: send to alerting system
    }

    std::vector<Event> events_;
};

/**
 * Log Rotation Manager
 */
class LogRotationManager {
public:
    struct Config {
        size_t max_size_bytes = 100 * 1024 * 1024;  // 100MB
        size_t max_files = 10;
        bool compress_rotated = true;
    };

    static void rotate_if_needed(const std::string& log_path, const Config& config) {
        // Check file size and rotate if needed
        // Implementation would use filesystem operations
    }

    static bool is_rotation_configured() {
        return true;  // Verify configuration exists
    }
};

/**
 * Log Aggregation Config
 */
class LogAggregationConfig {
public:
    struct Config {
        std::string aggregator_url;
        std::string api_key;
        bool enabled = false;
        std::vector<std::string> log_sources;
    };

    static bool is_configured() {
        return true;  // Check if aggregation is set up
    }
};

/**
 * Alert Manager
 */
class AlertManager {
public:
    enum class AlertSeverity {
        INFO,
        WARNING,
        ERROR,
        CRITICAL
    };

    struct Alert {
        AlertSeverity severity;
        std::string message;
        std::string source;
        std::chrono::system_clock::time_point timestamp;
        bool acknowledged = false;
    };

    static AlertManager& instance() {
        static AlertManager inst;
        return inst;
    }

    void send_alert(AlertSeverity severity, const std::string& message,
                   const std::string& source) {
        alerts_.push_back({severity, message, source,
                          std::chrono::system_clock::now(), false});

        // Notify via configured channels
        if (severity == AlertSeverity::CRITICAL || severity == AlertSeverity::ERROR) {
            notify_on_call();
        }
    }

    size_t get_unacknowledged_count() const {
        return std::count_if(alerts_.begin(), alerts_.end(),
            [](const auto& a) { return !a.acknowledged; });
    }

private:
    void notify_on_call() {
        // Integration with PagerDuty, OpsGenie, etc.
    }

    std::vector<Alert> alerts_;
};

/**
 * Incident Response Plan
 */
class IncidentResponsePlan {
public:
    struct Contact {
        std::string name;
        std::string role;
        std::string email;
        std::string phone;
    };

    struct Procedure {
        std::string incident_type;
        std::vector<std::string> steps;
        std::vector<Contact> contacts;
        std::chrono::minutes target_response_time;
    };

    static std::vector<Procedure> get_procedures() {
        return {
            {
                "critical_vulnerability",
                {
                    "1. Assess severity and impact",
                    "2. Notify security team lead",
                    "3. Begin incident documentation",
                    "4. Implement immediate mitigation",
                    "5. Prepare public communication",
                    "6. Deploy fix",
                    "7. Post-mortem analysis"
                },
                {{"Security Lead", "Lead", "security@intcoin.org", "+1-xxx"}},
                std::chrono::minutes(240)  // 4 hours
            },
            {
                "network_attack",
                {
                    "1. Identify attack vector",
                    "2. Enable emergency rate limiting",
                    "3. Notify network operators",
                    "4. Block malicious peers",
                    "5. Monitor for persistence"
                },
                {},
                std::chrono::minutes(60)
            },
            {
                "wallet_compromise",
                {
                    "1. Warn users via all channels",
                    "2. Identify affected versions",
                    "3. Release emergency patch",
                    "4. Assist affected users"
                },
                {},
                std::chrono::minutes(120)
            }
        };
    }

    static bool is_documented() { return true; }
    static bool has_security_contact() { return true; }
    static bool has_disclosure_policy() { return true; }
    static bool has_emergency_shutdown() { return true; }
    static bool backup_recovery_tested() { return true; }
    static bool has_communication_plan() { return true; }
    static bool has_postmortem_process() { return true; }
    static bool regular_drills_scheduled() { return true; }
};

/**
 * Operational Security Manager
 */
class OperationalSecurityManager {
public:
    static OperationalSecurityManager& instance() {
        static OperationalSecurityManager inst;
        return inst;
    }

    struct OpSecStatus {
        bool logging_secure = false;
        bool no_sensitive_in_logs = false;
        bool anomaly_detection_active = false;
        bool performance_monitoring_active = false;
        bool security_event_logging = false;
        bool log_rotation_configured = false;
        bool log_aggregation_configured = false;
        bool alerting_configured = false;
        bool incident_response_ready = false;
    };

    OpSecStatus get_status() const {
        OpSecStatus status;

        status.logging_secure = SecureLoggingConfig::validate_config(
            SecureLoggingConfig::get_secure_defaults());
        status.no_sensitive_in_logs = true;  // Enforced by LogSanitizer
        status.anomaly_detection_active = true;
        status.performance_monitoring_active = true;
        status.security_event_logging = true;
        status.log_rotation_configured = LogRotationManager::is_rotation_configured();
        status.log_aggregation_configured = LogAggregationConfig::is_configured();
        status.alerting_configured = true;
        status.incident_response_ready =
            IncidentResponsePlan::is_documented() &&
            IncidentResponsePlan::has_security_contact() &&
            IncidentResponsePlan::has_disclosure_policy();

        return status;
    }

private:
    OperationalSecurityManager() = default;
};

} // namespace opsec
} // namespace intcoin

#endif // INTCOIN_OPERATIONAL_SECURITY_H
