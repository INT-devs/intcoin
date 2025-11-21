// Copyright (c) 2025 INTcoin Core (Maddison Lane)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef INTCOIN_RPC_AUTHENTICATION_H
#define INTCOIN_RPC_AUTHENTICATION_H

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <mutex>
#include <chrono>
#include <optional>
#include <random>
#include <regex>
#include <functional>
#include <array>

namespace intcoin {
namespace rpc {

/**
 * Password Policy - Strong password enforcement
 */
class PasswordPolicy {
public:
    struct Requirements {
        size_t min_length = 12;
        size_t min_uppercase = 1;
        size_t min_lowercase = 1;
        size_t min_digits = 1;
        size_t min_special = 1;
        bool no_common_passwords = true;
        bool no_username_in_password = true;
    };

    struct ValidationResult {
        bool valid = false;
        std::vector<std::string> violations;
    };

    static ValidationResult validate(const std::string& password,
                                     const std::string& username = "",
                                     const Requirements& req = Requirements{}) {
        ValidationResult result;
        result.valid = true;

        if (password.length() < req.min_length) {
            result.violations.push_back("Password too short");
            result.valid = false;
        }

        size_t upper = 0, lower = 0, digit = 0, special = 0;
        for (char c : password) {
            if (std::isupper(c)) upper++;
            else if (std::islower(c)) lower++;
            else if (std::isdigit(c)) digit++;
            else special++;
        }

        if (upper < req.min_uppercase) {
            result.violations.push_back("Insufficient uppercase characters");
            result.valid = false;
        }
        if (lower < req.min_lowercase) {
            result.violations.push_back("Insufficient lowercase characters");
            result.valid = false;
        }
        if (digit < req.min_digits) {
            result.violations.push_back("Insufficient digits");
            result.valid = false;
        }
        if (special < req.min_special) {
            result.violations.push_back("Insufficient special characters");
            result.valid = false;
        }

        if (req.no_common_passwords && is_common_password(password)) {
            result.violations.push_back("Password is too common");
            result.valid = false;
        }

        if (req.no_username_in_password && !username.empty()) {
            std::string lower_pass = password;
            std::string lower_user = username;
            std::transform(lower_pass.begin(), lower_pass.end(), lower_pass.begin(), ::tolower);
            std::transform(lower_user.begin(), lower_user.end(), lower_user.begin(), ::tolower);
            if (lower_pass.find(lower_user) != std::string::npos) {
                result.violations.push_back("Password contains username");
                result.valid = false;
            }
        }

        return result;
    }

private:
    static bool is_common_password(const std::string& password) {
        static const std::unordered_set<std::string> common = {
            "password", "123456", "password123", "admin", "letmein",
            "welcome", "monkey", "dragon", "master", "qwerty",
            "login", "passw0rd", "abc123", "111111", "iloveyou"
        };
        std::string lower = password;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        return common.count(lower) > 0;
    }
};

/**
 * Credential Protector - Prevents credentials from appearing in logs
 */
class CredentialProtector {
public:
    static std::string sanitize_for_log(const std::string& message) {
        std::string result = message;

        // Mask password fields
        result = std::regex_replace(result,
            std::regex(R"(password[\"':\s]*[=:][\"'\s]*[^\s\"',}]+)", std::regex::icase),
            "password=***REDACTED***");

        // Mask auth tokens
        result = std::regex_replace(result,
            std::regex(R"(auth[_-]?token[\"':\s]*[=:][\"'\s]*[^\s\"',}]+)", std::regex::icase),
            "auth_token=***REDACTED***");

        // Mask session IDs
        result = std::regex_replace(result,
            std::regex(R"(session[_-]?id[\"':\s]*[=:][\"'\s]*[a-f0-9-]{32,})", std::regex::icase),
            "session_id=***REDACTED***");

        // Mask rpcpassword in config
        result = std::regex_replace(result,
            std::regex(R"(rpcpassword\s*=\s*\S+)", std::regex::icase),
            "rpcpassword=***REDACTED***");

        return result;
    }

    // Wrapper for logging that auto-sanitizes
    template<typename LogFunc>
    static void safe_log(LogFunc&& log_fn, const std::string& message) {
        log_fn(sanitize_for_log(message));
    }
};

/**
 * Default Credential Checker - Detects and rejects default/weak credentials
 */
class DefaultCredentialChecker {
public:
    struct CheckResult {
        bool is_default = false;
        std::string reason;
    };

    static CheckResult check(const std::string& username, const std::string& password) {
        CheckResult result;

        // Check default usernames
        static const std::unordered_set<std::string> default_users = {
            "admin", "root", "user", "rpc", "intcoin", "bitcoin",
            "test", "guest", "default", "administrator"
        };

        std::string lower_user = username;
        std::transform(lower_user.begin(), lower_user.end(), lower_user.begin(), ::tolower);

        if (default_users.count(lower_user)) {
            result.is_default = true;
            result.reason = "Default username detected";
            return result;
        }

        // Check default passwords
        static const std::unordered_set<std::string> default_passwords = {
            "password", "admin", "root", "123456", "intcoin",
            "bitcoin", "rpc", "changeme", "default", "test"
        };

        std::string lower_pass = password;
        std::transform(lower_pass.begin(), lower_pass.end(), lower_pass.begin(), ::tolower);

        if (default_passwords.count(lower_pass)) {
            result.is_default = true;
            result.reason = "Default password detected";
            return result;
        }

        // Check if username equals password
        if (lower_user == lower_pass) {
            result.is_default = true;
            result.reason = "Username and password are identical";
            return result;
        }

        return result;
    }

    static bool reject_if_default(const std::string& username, const std::string& password) {
        return check(username, password).is_default;
    }
};

/**
 * Rate Limiter - Limits authentication attempts
 */
class RateLimiter {
public:
    struct Config {
        size_t max_attempts = 5;
        std::chrono::seconds window = std::chrono::seconds(300);  // 5 minutes
        std::chrono::seconds lockout_duration = std::chrono::seconds(900);  // 15 minutes
        bool enable_progressive_delay = true;
    };

    enum class AttemptResult {
        ALLOWED,
        RATE_LIMITED,
        LOCKED_OUT
    };

    AttemptResult check_attempt(const std::string& identifier) {
        std::lock_guard<std::mutex> lock(mtx_);
        auto now = std::chrono::steady_clock::now();

        // Check lockout
        auto lockout_it = lockouts_.find(identifier);
        if (lockout_it != lockouts_.end()) {
            if (now < lockout_it->second) {
                return AttemptResult::LOCKED_OUT;
            }
            lockouts_.erase(lockout_it);
        }

        // Clean old attempts
        auto& attempts = attempts_[identifier];
        auto cutoff = now - config_.window;
        attempts.erase(
            std::remove_if(attempts.begin(), attempts.end(),
                [cutoff](const auto& t) { return t < cutoff; }),
            attempts.end()
        );

        if (attempts.size() >= config_.max_attempts) {
            return AttemptResult::RATE_LIMITED;
        }

        return AttemptResult::ALLOWED;
    }

    void record_attempt(const std::string& identifier, bool success) {
        std::lock_guard<std::mutex> lock(mtx_);
        auto now = std::chrono::steady_clock::now();

        if (success) {
            attempts_.erase(identifier);
            lockouts_.erase(identifier);
        } else {
            attempts_[identifier].push_back(now);

            if (attempts_[identifier].size() >= config_.max_attempts) {
                lockouts_[identifier] = now + config_.lockout_duration;
            }
        }
    }

    std::chrono::milliseconds get_delay(const std::string& identifier) const {
        if (!config_.enable_progressive_delay) {
            return std::chrono::milliseconds(0);
        }

        std::lock_guard<std::mutex> lock(mtx_);
        auto it = attempts_.find(identifier);
        if (it == attempts_.end()) {
            return std::chrono::milliseconds(0);
        }

        // Progressive delay: 0, 1s, 2s, 4s, 8s...
        size_t failed = it->second.size();
        if (failed == 0) return std::chrono::milliseconds(0);
        return std::chrono::milliseconds(1000 * (1 << std::min(failed - 1, size_t(4))));
    }

    void set_config(const Config& config) { config_ = config; }

private:
    mutable std::mutex mtx_;
    Config config_;
    std::unordered_map<std::string, std::vector<std::chrono::steady_clock::time_point>> attempts_;
    std::unordered_map<std::string, std::chrono::steady_clock::time_point> lockouts_;
};

/**
 * Session Manager - Secure session handling
 */
class SessionManager {
public:
    struct Session {
        std::string session_id;
        std::string username;
        std::string ip_address;
        std::chrono::steady_clock::time_point created;
        std::chrono::steady_clock::time_point last_activity;
        std::chrono::steady_clock::time_point expires;
        bool is_valid = true;
    };

    struct Config {
        std::chrono::seconds session_timeout = std::chrono::seconds(3600);  // 1 hour
        std::chrono::seconds idle_timeout = std::chrono::seconds(900);      // 15 minutes
        size_t max_sessions_per_user = 5;
        bool single_session_mode = false;
    };

    std::optional<Session> create_session(const std::string& username,
                                          const std::string& ip_address) {
        std::lock_guard<std::mutex> lock(mtx_);

        // Enforce session limits
        auto& user_sessions = user_session_map_[username];
        if (config_.single_session_mode) {
            // Invalidate all existing sessions
            for (const auto& sid : user_sessions) {
                if (auto it = sessions_.find(sid); it != sessions_.end()) {
                    it->second.is_valid = false;
                }
            }
            user_sessions.clear();
        } else if (user_sessions.size() >= config_.max_sessions_per_user) {
            return std::nullopt;
        }

        Session session;
        session.session_id = generate_session_id();
        session.username = username;
        session.ip_address = ip_address;
        session.created = std::chrono::steady_clock::now();
        session.last_activity = session.created;
        session.expires = session.created + config_.session_timeout;
        session.is_valid = true;

        sessions_[session.session_id] = session;
        user_sessions.push_back(session.session_id);

        return session;
    }

    enum class ValidationResult {
        VALID,
        INVALID,
        EXPIRED,
        IDLE_TIMEOUT,
        IP_MISMATCH
    };

    ValidationResult validate_session(const std::string& session_id,
                                      const std::string& ip_address) {
        std::lock_guard<std::mutex> lock(mtx_);

        auto it = sessions_.find(session_id);
        if (it == sessions_.end() || !it->second.is_valid) {
            return ValidationResult::INVALID;
        }

        auto& session = it->second;
        auto now = std::chrono::steady_clock::now();

        if (now > session.expires) {
            session.is_valid = false;
            return ValidationResult::EXPIRED;
        }

        if (now - session.last_activity > config_.idle_timeout) {
            session.is_valid = false;
            return ValidationResult::IDLE_TIMEOUT;
        }

        if (session.ip_address != ip_address) {
            return ValidationResult::IP_MISMATCH;
        }

        session.last_activity = now;
        return ValidationResult::VALID;
    }

    void invalidate_session(const std::string& session_id) {
        std::lock_guard<std::mutex> lock(mtx_);
        if (auto it = sessions_.find(session_id); it != sessions_.end()) {
            it->second.is_valid = false;
        }
    }

    void invalidate_user_sessions(const std::string& username) {
        std::lock_guard<std::mutex> lock(mtx_);
        if (auto it = user_session_map_.find(username); it != user_session_map_.end()) {
            for (const auto& sid : it->second) {
                if (auto sit = sessions_.find(sid); sit != sessions_.end()) {
                    sit->second.is_valid = false;
                }
            }
        }
    }

    void cleanup_expired() {
        std::lock_guard<std::mutex> lock(mtx_);
        auto now = std::chrono::steady_clock::now();

        for (auto it = sessions_.begin(); it != sessions_.end();) {
            if (!it->second.is_valid || now > it->second.expires) {
                it = sessions_.erase(it);
            } else {
                ++it;
            }
        }
    }

    void set_config(const Config& config) { config_ = config; }

private:
    std::string generate_session_id() {
        static std::random_device rd;
        static std::mt19937_64 gen(rd());
        static std::uniform_int_distribution<uint64_t> dist;

        std::array<uint8_t, 32> bytes;
        for (size_t i = 0; i < 4; ++i) {
            uint64_t val = dist(gen);
            std::memcpy(&bytes[i * 8], &val, 8);
        }

        static const char hex[] = "0123456789abcdef";
        std::string result;
        result.reserve(64);
        for (uint8_t b : bytes) {
            result += hex[b >> 4];
            result += hex[b & 0xf];
        }
        return result;
    }

    mutable std::mutex mtx_;
    Config config_;
    std::unordered_map<std::string, Session> sessions_;
    std::unordered_map<std::string, std::vector<std::string>> user_session_map_;
};

/**
 * RPC Authentication Manager - Central coordinator
 */
class RPCAuthenticationManager {
public:
    static RPCAuthenticationManager& instance() {
        static RPCAuthenticationManager inst;
        return inst;
    }

    struct AuthResult {
        bool success = false;
        std::string session_id;
        std::string error;
    };

    AuthResult authenticate(const std::string& username,
                           const std::string& password,
                           const std::string& ip_address) {
        AuthResult result;

        // Check rate limiting
        auto rate_result = rate_limiter_.check_attempt(ip_address);
        if (rate_result == RateLimiter::AttemptResult::LOCKED_OUT) {
            result.error = "Account locked due to too many failed attempts";
            return result;
        }
        if (rate_result == RateLimiter::AttemptResult::RATE_LIMITED) {
            result.error = "Too many authentication attempts";
            return result;
        }

        // Apply progressive delay
        auto delay = rate_limiter_.get_delay(ip_address);
        if (delay.count() > 0) {
            std::this_thread::sleep_for(delay);
        }

        // Check for default credentials
        if (DefaultCredentialChecker::reject_if_default(username, password)) {
            rate_limiter_.record_attempt(ip_address, false);
            result.error = "Default credentials not allowed";
            return result;
        }

        // Validate credentials (placeholder - integrate with actual auth system)
        if (!validate_credentials(username, password)) {
            rate_limiter_.record_attempt(ip_address, false);
            result.error = "Invalid credentials";
            return result;
        }

        // Create session
        auto session = session_manager_.create_session(username, ip_address);
        if (!session) {
            result.error = "Maximum sessions reached";
            return result;
        }

        rate_limiter_.record_attempt(ip_address, true);
        result.success = true;
        result.session_id = session->session_id;
        return result;
    }

    SessionManager::ValidationResult validate_session(const std::string& session_id,
                                                      const std::string& ip_address) {
        return session_manager_.validate_session(session_id, ip_address);
    }

    void logout(const std::string& session_id) {
        session_manager_.invalidate_session(session_id);
    }

    // Configuration
    void set_password_policy(const PasswordPolicy::Requirements& req) {
        password_requirements_ = req;
    }

    void set_rate_limit_config(const RateLimiter::Config& config) {
        rate_limiter_.set_config(config);
    }

    void set_session_config(const SessionManager::Config& config) {
        session_manager_.set_config(config);
    }

    PasswordPolicy::ValidationResult validate_password(const std::string& password,
                                                       const std::string& username = "") {
        return PasswordPolicy::validate(password, username, password_requirements_);
    }

private:
    RPCAuthenticationManager() = default;

    bool validate_credentials(const std::string& username, const std::string& password) {
        // Placeholder - integrate with actual credential store
        // In production, this would check against hashed credentials
        std::lock_guard<std::mutex> lock(cred_mtx_);
        auto it = credentials_.find(username);
        if (it == credentials_.end()) return false;
        return it->second == password;  // In production: use constant-time comparison with hash
    }

    PasswordPolicy::Requirements password_requirements_;
    RateLimiter rate_limiter_;
    SessionManager session_manager_;

    std::mutex cred_mtx_;
    std::unordered_map<std::string, std::string> credentials_;
};

} // namespace rpc
} // namespace intcoin

#endif // INTCOIN_RPC_AUTHENTICATION_H
