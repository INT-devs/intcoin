// Copyright (c) 2025 INTcoin Core (Maddison Lane)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef INTCOIN_RPC_AUTHORIZATION_H
#define INTCOIN_RPC_AUTHORIZATION_H

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <mutex>
#include <optional>
#include <regex>
#include <functional>

namespace intcoin {
namespace rpc {

/**
 * Privilege Level - Defines access tiers
 */
enum class PrivilegeLevel {
    NONE = 0,
    READ_ONLY = 1,      // getinfo, getbalance, etc.
    STANDARD = 2,       // sendtoaddress, createrawtransaction
    WALLET = 3,         // wallet operations
    NETWORK = 4,        // addnode, disconnectnode
    ADMIN = 5           // stop, debug commands
};

/**
 * RPC Method Registry - Maps methods to required privileges
 */
class RPCMethodRegistry {
public:
    struct MethodInfo {
        std::string name;
        PrivilegeLevel required_level;
        bool requires_auth;
        bool is_sensitive;
    };

    static RPCMethodRegistry& instance() {
        static RPCMethodRegistry inst;
        return inst;
    }

    void register_method(const std::string& name, PrivilegeLevel level,
                        bool requires_auth = true, bool is_sensitive = false) {
        std::lock_guard<std::mutex> lock(mtx_);
        methods_[name] = {name, level, requires_auth, is_sensitive};
    }

    std::optional<MethodInfo> get_method(const std::string& name) const {
        std::lock_guard<std::mutex> lock(mtx_);
        auto it = methods_.find(name);
        if (it != methods_.end()) return it->second;
        return std::nullopt;
    }

    bool method_exists(const std::string& name) const {
        std::lock_guard<std::mutex> lock(mtx_);
        return methods_.count(name) > 0;
    }

private:
    RPCMethodRegistry() {
        // Register default methods with privileges
        // Read-only
        register_method("getinfo", PrivilegeLevel::READ_ONLY, false);
        register_method("getblockcount", PrivilegeLevel::READ_ONLY, false);
        register_method("getbestblockhash", PrivilegeLevel::READ_ONLY, false);
        register_method("getblock", PrivilegeLevel::READ_ONLY, false);
        register_method("getblockhash", PrivilegeLevel::READ_ONLY, false);
        register_method("gettransaction", PrivilegeLevel::READ_ONLY);
        register_method("getbalance", PrivilegeLevel::READ_ONLY);

        // Standard
        register_method("sendtoaddress", PrivilegeLevel::STANDARD, true, true);
        register_method("createrawtransaction", PrivilegeLevel::STANDARD);
        register_method("signrawtransaction", PrivilegeLevel::STANDARD, true, true);

        // Wallet
        register_method("dumpprivkey", PrivilegeLevel::WALLET, true, true);
        register_method("importprivkey", PrivilegeLevel::WALLET, true, true);
        register_method("encryptwallet", PrivilegeLevel::WALLET, true, true);
        register_method("walletpassphrase", PrivilegeLevel::WALLET, true, true);

        // Network
        register_method("addnode", PrivilegeLevel::NETWORK);
        register_method("disconnectnode", PrivilegeLevel::NETWORK);

        // Admin
        register_method("stop", PrivilegeLevel::ADMIN, true, true);
        register_method("debug", PrivilegeLevel::ADMIN);
    }

    mutable std::mutex mtx_;
    std::unordered_map<std::string, MethodInfo> methods_;
};

/**
 * User Privilege Manager - Assigns privileges to users
 */
class UserPrivilegeManager {
public:
    static UserPrivilegeManager& instance() {
        static UserPrivilegeManager inst;
        return inst;
    }

    void set_user_privilege(const std::string& username, PrivilegeLevel level) {
        std::lock_guard<std::mutex> lock(mtx_);
        user_privileges_[username] = level;
    }

    PrivilegeLevel get_user_privilege(const std::string& username) const {
        std::lock_guard<std::mutex> lock(mtx_);
        auto it = user_privileges_.find(username);
        if (it != user_privileges_.end()) return it->second;
        return PrivilegeLevel::NONE;
    }

    bool check_access(const std::string& username, const std::string& method) const {
        auto method_info = RPCMethodRegistry::instance().get_method(method);
        if (!method_info) return false;

        PrivilegeLevel user_level = get_user_privilege(username);
        return static_cast<int>(user_level) >= static_cast<int>(method_info->required_level);
    }

private:
    mutable std::mutex mtx_;
    std::unordered_map<std::string, PrivilegeLevel> user_privileges_;
};

/**
 * Authentication Enforcer - Ensures sensitive ops require auth
 */
class AuthenticationEnforcer {
public:
    struct EnforcementResult {
        bool allowed = false;
        std::string reason;
    };

    static EnforcementResult check(const std::string& method,
                                   bool is_authenticated,
                                   const std::string& username = "") {
        EnforcementResult result;

        auto method_info = RPCMethodRegistry::instance().get_method(method);
        if (!method_info) {
            result.reason = "Unknown RPC method";
            return result;
        }

        if (method_info->requires_auth && !is_authenticated) {
            result.reason = "Authentication required for this method";
            return result;
        }

        if (is_authenticated && !username.empty()) {
            if (!UserPrivilegeManager::instance().check_access(username, method)) {
                result.reason = "Insufficient privileges";
                return result;
            }
        }

        result.allowed = true;
        return result;
    }
};

/**
 * Command Injection Preventer - Detects and blocks injection attacks
 */
class CommandInjectionPreventer {
public:
    struct ValidationResult {
        bool safe = false;
        std::string threat;
    };

    static ValidationResult validate(const std::string& input) {
        ValidationResult result;

        // Shell metacharacters
        static const std::regex shell_pattern(R"([;&|`$(){}[\]<>\\'\"\n\r])");
        if (std::regex_search(input, shell_pattern)) {
            result.threat = "Shell metacharacters detected";
            return result;
        }

        // Command chaining
        static const std::regex chain_pattern(R"(&&|\|\||;|`.*`)");
        if (std::regex_search(input, chain_pattern)) {
            result.threat = "Command chaining detected";
            return result;
        }

        // Path traversal
        static const std::regex traversal_pattern(R"(\.\./|\.\.\\)");
        if (std::regex_search(input, traversal_pattern)) {
            result.threat = "Path traversal detected";
            return result;
        }

        // Null bytes
        if (input.find('\0') != std::string::npos) {
            result.threat = "Null byte injection detected";
            return result;
        }

        result.safe = true;
        return result;
    }

    static std::string escape_for_shell(const std::string& input) {
        std::string result;
        result.reserve(input.size() * 2);
        for (char c : input) {
            if (c == '\'' || c == '"' || c == '\\' || c == '$' ||
                c == '`' || c == '!' || c == ';' || c == '&' ||
                c == '|' || c == '<' || c == '>' || c == '(' ||
                c == ')' || c == '{' || c == '}' || c == '[' ||
                c == ']' || c == '\n' || c == '\r') {
                result += '\\';
            }
            result += c;
        }
        return result;
    }
};

/**
 * Input Sanitizer - Sanitizes all RPC inputs
 */
class InputSanitizer {
public:
    struct SanitizeResult {
        bool valid = false;
        std::string sanitized;
        std::string error;
    };

    // Sanitize string parameter
    static SanitizeResult sanitize_string(const std::string& input, size_t max_len = 10000) {
        SanitizeResult result;

        if (input.length() > max_len) {
            result.error = "Input too long";
            return result;
        }

        auto injection_check = CommandInjectionPreventer::validate(input);
        if (!injection_check.safe) {
            result.error = injection_check.threat;
            return result;
        }

        // Remove control characters except whitespace
        result.sanitized.reserve(input.size());
        for (char c : input) {
            if (c >= 32 || c == '\t' || c == '\n' || c == '\r') {
                result.sanitized += c;
            }
        }

        result.valid = true;
        return result;
    }

    // Sanitize address parameter
    static SanitizeResult sanitize_address(const std::string& input) {
        SanitizeResult result;

        // INTcoin addresses: alphanumeric, 26-62 chars
        static const std::regex addr_pattern(R"(^[a-zA-Z0-9]{26,62}$)");
        if (!std::regex_match(input, addr_pattern)) {
            result.error = "Invalid address format";
            return result;
        }

        result.sanitized = input;
        result.valid = true;
        return result;
    }

    // Sanitize numeric parameter
    static SanitizeResult sanitize_amount(const std::string& input) {
        SanitizeResult result;

        static const std::regex amount_pattern(R"(^-?[0-9]+(\.[0-9]{1,8})?$)");
        if (!std::regex_match(input, amount_pattern)) {
            result.error = "Invalid amount format";
            return result;
        }

        result.sanitized = input;
        result.valid = true;
        return result;
    }

    // Sanitize hex parameter
    static SanitizeResult sanitize_hex(const std::string& input, size_t expected_len = 0) {
        SanitizeResult result;

        static const std::regex hex_pattern(R"(^[a-fA-F0-9]+$)");
        if (!std::regex_match(input, hex_pattern)) {
            result.error = "Invalid hex format";
            return result;
        }

        if (expected_len > 0 && input.length() != expected_len) {
            result.error = "Invalid hex length";
            return result;
        }

        result.sanitized = input;
        result.valid = true;
        return result;
    }
};

/**
 * RPC Authorization Manager - Central coordinator
 */
class RPCAuthorizationManager {
public:
    static RPCAuthorizationManager& instance() {
        static RPCAuthorizationManager inst;
        return inst;
    }

    struct AuthzResult {
        bool allowed = false;
        std::string reason;
    };

    AuthzResult authorize(const std::string& method,
                         const std::string& username,
                         bool is_authenticated,
                         const std::vector<std::string>& params) {
        AuthzResult result;

        // Check method exists
        if (!RPCMethodRegistry::instance().method_exists(method)) {
            result.reason = "Unknown method";
            return result;
        }

        // Check authentication requirement
        auto auth_result = AuthenticationEnforcer::check(method, is_authenticated, username);
        if (!auth_result.allowed) {
            result.reason = auth_result.reason;
            return result;
        }

        // Validate all parameters for injection
        for (const auto& param : params) {
            auto injection_check = CommandInjectionPreventer::validate(param);
            if (!injection_check.safe) {
                result.reason = "Invalid parameter: " + injection_check.threat;
                return result;
            }
        }

        result.allowed = true;
        return result;
    }

private:
    RPCAuthorizationManager() = default;
};

} // namespace rpc
} // namespace intcoin

#endif // INTCOIN_RPC_AUTHORIZATION_H
