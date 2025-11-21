// Copyright (c) 2025 INTcoin Core (Maddison Lane)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef INTCOIN_ERROR_HANDLING_H
#define INTCOIN_ERROR_HANDLING_H

#include <cstdint>
#include <string>
#include <vector>
#include <optional>
#include <functional>
#include <mutex>
#include <chrono>
#include <sstream>
#include <unordered_map>

namespace intcoin {
namespace error {

/**
 * Error Severity Levels
 */
enum class Severity {
    DEBUG,      // Detailed debugging info (not shown to users)
    INFO,       // Informational messages
    WARNING,    // Non-critical issues
    ERROR,      // Recoverable errors
    CRITICAL,   // Severe errors requiring attention
    FATAL       // Unrecoverable errors
};

/**
 * Error Categories (for filtering and routing)
 */
enum class Category {
    GENERAL,        // General errors
    NETWORK,        // Network-related errors
    CONSENSUS,      // Consensus/validation errors
    WALLET,         // Wallet operations
    CRYPTO,         // Cryptographic operations
    STORAGE,        // Database/file errors
    MEMORY,         // Memory allocation errors
    CONCURRENCY,    // Threading errors
    RPC,            // RPC interface errors
    SECURITY        // Security-related errors
};

/**
 * Error Code - Structured error identification
 */
struct ErrorCode {
    Category category;
    uint32_t code;
    std::string mnemonic;  // e.g., "NET_CONN_REFUSED"

    std::string to_string() const {
        return mnemonic + " (" + std::to_string(code) + ")";
    }
};

/**
 * Error - Full error information
 */
class Error {
    ErrorCode code_;
    Severity severity_;
    std::string internal_message_;   // Detailed (for logs)
    std::string user_message_;       // Safe for users
    std::string context_;            // Additional context
    uint64_t timestamp_;
    std::string source_file_;
    int source_line_;

public:
    Error(ErrorCode code, Severity severity,
          const std::string& internal_msg,
          const std::string& user_msg)
        : code_(code), severity_(severity),
          internal_message_(internal_msg), user_message_(user_msg),
          timestamp_(std::chrono::system_clock::now().time_since_epoch().count()),
          source_line_(0) {}

    // Getters
    const ErrorCode& code() const { return code_; }
    Severity severity() const { return severity_; }
    const std::string& internal_message() const { return internal_message_; }
    const std::string& user_message() const { return user_message_; }
    const std::string& context() const { return context_; }
    uint64_t timestamp() const { return timestamp_; }

    // Builder pattern for additional info
    Error& with_context(const std::string& ctx) {
        context_ = ctx;
        return *this;
    }

    Error& with_source(const std::string& file, int line) {
        source_file_ = file;
        source_line_ = line;
        return *this;
    }

    // Format for logging (detailed)
    std::string format_for_log() const {
        std::ostringstream ss;
        ss << "[" << severity_string() << "] "
           << code_.mnemonic << ": " << internal_message_;
        if (!context_.empty()) {
            ss << " (Context: " << context_ << ")";
        }
        if (!source_file_.empty()) {
            ss << " [" << source_file_ << ":" << source_line_ << "]";
        }
        return ss.str();
    }

    // Format for user (safe, no internals)
    std::string format_for_user() const {
        return user_message_;
    }

private:
    std::string severity_string() const {
        switch (severity_) {
            case Severity::DEBUG:    return "DEBUG";
            case Severity::INFO:     return "INFO";
            case Severity::WARNING:  return "WARN";
            case Severity::ERROR:    return "ERROR";
            case Severity::CRITICAL: return "CRITICAL";
            case Severity::FATAL:    return "FATAL";
        }
        return "UNKNOWN";
    }
};

/**
 * Result<T> - Either a value or an error (no silent failures)
 */
template<typename T>
class Result {
    std::optional<T> value_;
    std::optional<Error> error_;

public:
    // Success constructor
    Result(T value) : value_(std::move(value)) {}

    // Error constructor
    Result(Error error) : error_(std::move(error)) {}

    // Check state
    bool is_ok() const { return value_.has_value(); }
    bool is_error() const { return error_.has_value(); }
    explicit operator bool() const { return is_ok(); }

    // Access value (throws if error)
    T& value() {
        if (!value_) {
            throw std::runtime_error("Attempted to access value of error result");
        }
        return *value_;
    }

    const T& value() const {
        if (!value_) {
            throw std::runtime_error("Attempted to access value of error result");
        }
        return *value_;
    }

    // Access error
    const Error& error() const {
        if (!error_) {
            throw std::runtime_error("Attempted to access error of success result");
        }
        return *error_;
    }

    // Safe access with default
    T value_or(T default_val) const {
        return value_.value_or(std::move(default_val));
    }

    // Map value (transform if success)
    template<typename F>
    auto map(F&& func) -> Result<decltype(func(std::declval<T>()))> {
        if (is_ok()) {
            return Result<decltype(func(std::declval<T>()))>(func(*value_));
        }
        return *error_;
    }

    // Handle error (execute if error)
    Result& on_error(std::function<void(const Error&)> handler) {
        if (is_error()) {
            handler(*error_);
        }
        return *this;
    }
};

// Void specialization
template<>
class Result<void> {
    std::optional<Error> error_;

public:
    Result() = default;  // Success
    Result(Error error) : error_(std::move(error)) {}

    bool is_ok() const { return !error_.has_value(); }
    bool is_error() const { return error_.has_value(); }
    explicit operator bool() const { return is_ok(); }

    const Error& error() const {
        if (!error_) {
            throw std::runtime_error("No error present");
        }
        return *error_;
    }
};

/**
 * Error Logger - Proper error logging without silent failures
 */
class ErrorLogger {
public:
    using LogHandler = std::function<void(const Error&)>;

private:
    std::vector<Error> log_;
    mutable std::mutex mtx_;
    LogHandler external_handler_;
    Severity min_log_level_ = Severity::INFO;
    size_t max_log_size_ = 10000;

    struct Statistics {
        uint64_t total_logged = 0;
        uint64_t debug_count = 0;
        uint64_t info_count = 0;
        uint64_t warning_count = 0;
        uint64_t error_count = 0;
        uint64_t critical_count = 0;
        uint64_t fatal_count = 0;
    } stats_;

public:
    // Log an error (never silent)
    void log(const Error& error) {
        std::lock_guard<std::mutex> lock(mtx_);

        // Always count
        stats_.total_logged++;
        switch (error.severity()) {
            case Severity::DEBUG:    stats_.debug_count++; break;
            case Severity::INFO:     stats_.info_count++; break;
            case Severity::WARNING:  stats_.warning_count++; break;
            case Severity::ERROR:    stats_.error_count++; break;
            case Severity::CRITICAL: stats_.critical_count++; break;
            case Severity::FATAL:    stats_.fatal_count++; break;
        }

        // Store if above minimum level
        if (error.severity() >= min_log_level_) {
            log_.push_back(error);

            // Trim if too large
            if (log_.size() > max_log_size_) {
                log_.erase(log_.begin(), log_.begin() + log_.size() / 2);
            }
        }

        // Forward to external handler if set
        if (external_handler_) {
            external_handler_(error);
        }
    }

    // Set external log handler
    void set_handler(LogHandler handler) {
        std::lock_guard<std::mutex> lock(mtx_);
        external_handler_ = std::move(handler);
    }

    // Set minimum log level
    void set_min_level(Severity level) {
        std::lock_guard<std::mutex> lock(mtx_);
        min_log_level_ = level;
    }

    // Get recent errors
    std::vector<Error> get_recent(size_t count = 100) const {
        std::lock_guard<std::mutex> lock(mtx_);
        size_t start = (log_.size() > count) ? (log_.size() - count) : 0;
        return std::vector<Error>(log_.begin() + start, log_.end());
    }

    // Get errors by severity
    std::vector<Error> get_by_severity(Severity severity) const {
        std::lock_guard<std::mutex> lock(mtx_);
        std::vector<Error> result;
        for (const auto& error : log_) {
            if (error.severity() == severity) {
                result.push_back(error);
            }
        }
        return result;
    }

    // Get statistics
    const Statistics& get_statistics() const {
        return stats_;
    }

    // Check if any errors logged
    bool has_errors() const {
        std::lock_guard<std::mutex> lock(mtx_);
        return stats_.error_count > 0 || stats_.critical_count > 0 || stats_.fatal_count > 0;
    }

    // Clear log
    void clear() {
        std::lock_guard<std::mutex> lock(mtx_);
        log_.clear();
    }
};

/**
 * User Message Sanitizer - Prevents information leakage
 */
class MessageSanitizer {
public:
    // Sanitize message for user display
    static std::string sanitize(const std::string& message) {
        std::string result = message;

        // Remove file paths
        result = remove_paths(result);

        // Remove memory addresses
        result = remove_addresses(result);

        // Remove stack traces
        result = remove_stack_traces(result);

        // Remove internal error codes
        result = remove_internal_codes(result);

        // Truncate if too long
        if (result.length() > 200) {
            result = result.substr(0, 200) + "...";
        }

        return result;
    }

    // Create user-friendly message from internal error
    static std::string create_user_message(Category category, const std::string& generic_msg) {
        // Map category to user-friendly prefix
        std::string prefix;
        switch (category) {
            case Category::NETWORK:
                prefix = "Network error: ";
                break;
            case Category::WALLET:
                prefix = "Wallet error: ";
                break;
            case Category::CRYPTO:
                prefix = "Cryptographic error: ";
                break;
            case Category::STORAGE:
                prefix = "Storage error: ";
                break;
            case Category::RPC:
                prefix = "RPC error: ";
                break;
            case Category::SECURITY:
                prefix = "Security error: ";
                break;
            default:
                prefix = "Error: ";
        }

        return prefix + generic_msg;
    }

private:
    static std::string remove_paths(const std::string& msg) {
        // Remove Unix-style paths
        std::string result = msg;
        size_t pos;
        while ((pos = result.find("/home/")) != std::string::npos ||
               (pos = result.find("/Users/")) != std::string::npos ||
               (pos = result.find("/var/")) != std::string::npos ||
               (pos = result.find("/tmp/")) != std::string::npos) {
            size_t end = result.find_first_of(" \n\t", pos);
            if (end == std::string::npos) end = result.length();
            result.replace(pos, end - pos, "[path]");
        }
        return result;
    }

    static std::string remove_addresses(const std::string& msg) {
        std::string result = msg;
        size_t pos = 0;
        while ((pos = result.find("0x", pos)) != std::string::npos) {
            size_t end = pos + 2;
            while (end < result.length() &&
                   ((result[end] >= '0' && result[end] <= '9') ||
                    (result[end] >= 'a' && result[end] <= 'f') ||
                    (result[end] >= 'A' && result[end] <= 'F'))) {
                end++;
            }
            if (end - pos > 4) {  // Likely an address
                result.replace(pos, end - pos, "[addr]");
            }
            pos++;
        }
        return result;
    }

    static std::string remove_stack_traces(const std::string& msg) {
        std::string result = msg;
        // Remove lines that look like stack traces
        size_t pos;
        while ((pos = result.find("at ")) != std::string::npos &&
               pos > 0 && result[pos-1] == '\n') {
            size_t end = result.find('\n', pos);
            if (end == std::string::npos) end = result.length();
            result.erase(pos, end - pos + 1);
        }
        return result;
    }

    static std::string remove_internal_codes(const std::string& msg) {
        std::string result = msg;
        // Remove internal error codes like "errno=123"
        size_t pos;
        while ((pos = result.find("errno=")) != std::string::npos) {
            size_t end = result.find_first_of(" \n\t", pos);
            if (end == std::string::npos) end = result.length();
            result.replace(pos, end - pos, "[code]");
        }
        return result;
    }
};

/**
 * Common Error Codes
 */
namespace codes {
    // Network errors (1000-1999)
    inline const ErrorCode NET_CONNECTION_REFUSED{Category::NETWORK, 1001, "NET_CONN_REFUSED"};
    inline const ErrorCode NET_TIMEOUT{Category::NETWORK, 1002, "NET_TIMEOUT"};
    inline const ErrorCode NET_PEER_DISCONNECTED{Category::NETWORK, 1003, "NET_PEER_DISCONN"};

    // Wallet errors (2000-2999)
    inline const ErrorCode WALLET_INSUFFICIENT_FUNDS{Category::WALLET, 2001, "WALLET_NO_FUNDS"};
    inline const ErrorCode WALLET_INVALID_ADDRESS{Category::WALLET, 2002, "WALLET_BAD_ADDR"};
    inline const ErrorCode WALLET_LOCKED{Category::WALLET, 2003, "WALLET_LOCKED"};

    // Crypto errors (3000-3999)
    inline const ErrorCode CRYPTO_INVALID_SIGNATURE{Category::CRYPTO, 3001, "CRYPTO_BAD_SIG"};
    inline const ErrorCode CRYPTO_KEY_ERROR{Category::CRYPTO, 3002, "CRYPTO_KEY_ERR"};

    // Consensus errors (4000-4999)
    inline const ErrorCode CONSENSUS_INVALID_BLOCK{Category::CONSENSUS, 4001, "CONS_BAD_BLOCK"};
    inline const ErrorCode CONSENSUS_INVALID_TX{Category::CONSENSUS, 4002, "CONS_BAD_TX"};

    // General errors (5000-5999)
    inline const ErrorCode GENERAL_UNKNOWN{Category::GENERAL, 5000, "GEN_UNKNOWN"};
    inline const ErrorCode GENERAL_INVALID_PARAM{Category::GENERAL, 5001, "GEN_BAD_PARAM"};
}

/**
 * Error Handler Singleton
 */
class ErrorHandler {
    ErrorLogger logger_;

    ErrorHandler() = default;

public:
    static ErrorHandler& instance() {
        static ErrorHandler instance;
        return instance;
    }

    ErrorLogger& logger() { return logger_; }

    // Log and return error (no silent failures)
    Error report(const ErrorCode& code, Severity severity,
                 const std::string& internal_msg,
                 const std::string& user_msg) {
        Error error(code, severity, internal_msg, user_msg);
        logger_.log(error);
        return error;
    }

    // Quick error creation helpers
    Error network_error(const std::string& internal_msg) {
        return report(codes::NET_CONNECTION_REFUSED, Severity::ERROR,
                     internal_msg,
                     MessageSanitizer::create_user_message(Category::NETWORK,
                         "Unable to connect to the network. Please check your connection."));
    }

    Error wallet_error(const std::string& internal_msg) {
        return report(codes::WALLET_INSUFFICIENT_FUNDS, Severity::ERROR,
                     internal_msg,
                     MessageSanitizer::create_user_message(Category::WALLET,
                         "Wallet operation failed. Please try again."));
    }

    Error crypto_error(const std::string& internal_msg) {
        return report(codes::CRYPTO_INVALID_SIGNATURE, Severity::ERROR,
                     internal_msg,
                     MessageSanitizer::create_user_message(Category::CRYPTO,
                         "Cryptographic verification failed."));
    }
};

// Macro for automatic source location
#define REPORT_ERROR(handler, code, severity, internal_msg, user_msg) \
    (handler).report((code), (severity), (internal_msg), (user_msg)) \
        .with_source(__FILE__, __LINE__)

} // namespace error
} // namespace intcoin

#endif // INTCOIN_ERROR_HANDLING_H
