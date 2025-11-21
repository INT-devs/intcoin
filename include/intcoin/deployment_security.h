// Copyright (c) 2025 INTcoin Core (Maddison Lane)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef INTCOIN_DEPLOYMENT_SECURITY_H
#define INTCOIN_DEPLOYMENT_SECURITY_H

#include <string>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <optional>
#include <cstdint>
#include <regex>

namespace intcoin {
namespace deployment {

/**
 * Installation Script Validator - Ensures scripts don't run as root
 */
class InstallScriptValidator {
public:
    struct ValidationResult {
        bool safe = false;
        std::vector<std::string> issues;
    };

    static ValidationResult validate(const std::string& script) {
        ValidationResult result;
        result.safe = true;

        // Check for root requirement
        if (script.find("sudo ") != std::string::npos ||
            script.find("as root") != std::string::npos ||
            script.find("EUID == 0") != std::string::npos ||
            script.find("id -u") != std::string::npos) {
            // Allow if it's checking to NOT run as root
            if (script.find("if [ \"$EUID\" -eq 0 ]") != std::string::npos &&
                script.find("exit") != std::string::npos) {
                // OK - exits if root
            } else {
                result.issues.push_back("Script may require root privileges");
                result.safe = false;
            }
        }

        // Check for dangerous operations
        if (script.find("rm -rf /") != std::string::npos) {
            result.issues.push_back("Dangerous recursive deletion detected");
            result.safe = false;
        }

        if (script.find("chmod 777") != std::string::npos) {
            result.issues.push_back("Insecure permissions (777) detected");
            result.safe = false;
        }

        // Check for curl | bash pattern
        std::regex pipe_pattern(R"(curl.*\|\s*(ba)?sh)");
        if (std::regex_search(script, pipe_pattern)) {
            result.issues.push_back("Unsafe curl pipe to shell detected");
            result.safe = false;
        }

        return result;
    }

    static std::string get_safe_install_template() {
        return R"(#!/bin/bash
set -euo pipefail

# Refuse to run as root
if [ "$EUID" -eq 0 ]; then
    echo "Error: Do not run this script as root"
    exit 1
fi

# Create user-local directories
INSTALL_DIR="$HOME/.intcoin"
mkdir -p "$INSTALL_DIR"/{bin,data,config}

# Set restrictive permissions
chmod 700 "$INSTALL_DIR"
chmod 700 "$INSTALL_DIR/data"
chmod 700 "$INSTALL_DIR/config"
)";
    }
};

/**
 * File Permission Manager - Ensures proper permissions
 */
class FilePermissionManager {
public:
    struct PermissionPolicy {
        uint32_t executable_mode = 0750;    // rwxr-x---
        uint32_t config_mode = 0600;        // rw-------
        uint32_t data_mode = 0600;          // rw-------
        uint32_t wallet_mode = 0600;        // rw-------
        uint32_t log_mode = 0640;           // rw-r-----
        uint32_t directory_mode = 0700;     // rwx------
    };

    struct PermissionCheck {
        bool valid = false;
        std::string path;
        uint32_t current_mode;
        uint32_t required_mode;
        std::string issue;
    };

    static PermissionCheck validate_permission(const std::string& path,
                                               uint32_t current_mode,
                                               uint32_t max_allowed) {
        PermissionCheck result;
        result.path = path;
        result.current_mode = current_mode;
        result.required_mode = max_allowed;

        // Check world-readable
        if (current_mode & 0004) {
            result.issue = "World-readable";
            return result;
        }

        // Check world-writable
        if (current_mode & 0002) {
            result.issue = "World-writable";
            return result;
        }

        // Check world-executable
        if (current_mode & 0001) {
            result.issue = "World-executable";
            return result;
        }

        // Check group-writable for sensitive files
        if ((max_allowed & 0020) == 0 && (current_mode & 0020)) {
            result.issue = "Group-writable";
            return result;
        }

        result.valid = true;
        return result;
    }

    static PermissionPolicy get_default_policy() {
        return PermissionPolicy{};
    }

    // Get required mode for file type
    static uint32_t get_required_mode(const std::string& filename,
                                      const PermissionPolicy& policy = PermissionPolicy{}) {
        if (filename.find("wallet") != std::string::npos ||
            filename.find(".key") != std::string::npos) {
            return policy.wallet_mode;
        }
        if (filename.find(".conf") != std::string::npos ||
            filename.find("config") != std::string::npos) {
            return policy.config_mode;
        }
        if (filename.find(".log") != std::string::npos) {
            return policy.log_mode;
        }
        if (filename.find("intcoind") != std::string::npos ||
            filename.find("intcoin-cli") != std::string::npos) {
            return policy.executable_mode;
        }
        return policy.data_mode;
    }
};

/**
 * Secret Detector - Finds secrets in configuration files
 */
class SecretDetector {
public:
    struct DetectionResult {
        bool has_secrets = false;
        std::vector<std::string> findings;
    };

    static DetectionResult scan(const std::string& content, const std::string& filename = "") {
        DetectionResult result;

        // Patterns that indicate secrets
        static const std::vector<std::pair<std::regex, std::string>> patterns = {
            {std::regex(R"(password\s*[=:]\s*[^\s]+)", std::regex::icase), "Hardcoded password"},
            {std::regex(R"(api[_-]?key\s*[=:]\s*[a-zA-Z0-9]{20,})", std::regex::icase), "API key"},
            {std::regex(R"(secret[_-]?key\s*[=:]\s*[^\s]+)", std::regex::icase), "Secret key"},
            {std::regex(R"(private[_-]?key\s*[=:]\s*[^\s]+)", std::regex::icase), "Private key"},
            {std::regex(R"(-----BEGIN\s+(RSA\s+)?PRIVATE\s+KEY-----)"), "PEM private key"},
            {std::regex(R"(-----BEGIN\s+DILITHIUM\s+PRIVATE\s+KEY-----)"), "Dilithium private key"},
            {std::regex(R"([a-f0-9]{64})", std::regex::icase), "Potential 256-bit key/hash"},
            {std::regex(R"(rpcpassword\s*=\s*[^\s]+)"), "RPC password in config"},
        };

        for (const auto& [pattern, description] : patterns) {
            if (std::regex_search(content, pattern)) {
                result.has_secrets = true;
                result.findings.push_back(description);
            }
        }

        // Check filename for sensitive indicators
        if (!filename.empty()) {
            if (filename.find(".env") != std::string::npos ||
                filename.find("credentials") != std::string::npos ||
                filename.find("secret") != std::string::npos) {
                result.findings.push_back("Sensitive filename pattern");
            }
        }

        return result;
    }

    static std::string get_safe_config_template() {
        return R"(# INTcoin Configuration
# Do NOT store passwords or private keys in this file

# Network
testnet=0
regtest=0

# RPC - Use cookie authentication (auto-generated)
# rpcuser and rpcpassword should NOT be set here
# Cookie file: ~/.intcoin/.cookie

# Server
server=1
rpcbind=127.0.0.1
rpcallowip=127.0.0.1

# Wallet
# Private keys stored in encrypted wallet.dat
# Use walletpassphrase RPC to unlock temporarily
)";
    }
};

/**
 * Secure Defaults Manager - Ensures secure default settings
 */
class SecureDefaultsManager {
public:
    struct DefaultSetting {
        std::string name;
        std::string secure_value;
        std::string insecure_value;
        std::string reason;
    };

    static std::vector<DefaultSetting> get_required_defaults() {
        return {
            {"rpcbind", "127.0.0.1", "0.0.0.0", "RPC should only bind to localhost"},
            {"rpcallowip", "127.0.0.1", "*", "RPC should only allow localhost"},
            {"server", "1", "1", "Server mode for RPC"},
            {"listen", "1", "1", "Accept incoming connections"},
            {"discover", "1", "1", "Discover own IP"},
            {"upnp", "0", "1", "UPnP can expose internal services"},
            {"listenonion", "1", "0", "Tor support recommended"},
            {"debug", "0", "1", "Debug logging disabled by default"},
            {"printtoconsole", "0", "1", "Don't print sensitive info to console"},
            {"shrinkdebugfile", "1", "0", "Limit log file size"},
            {"disablewallet", "0", "0", "Wallet enabled by default"},
            {"walletnotify", "", "", "No external scripts by default"},
            {"blocknotify", "", "", "No external scripts by default"},
        };
    }

    struct ValidationResult {
        bool secure = true;
        std::vector<std::string> issues;
    };

    static ValidationResult validate_config(
            const std::unordered_map<std::string, std::string>& config) {
        ValidationResult result;

        for (const auto& def : get_required_defaults()) {
            auto it = config.find(def.name);
            if (it != config.end()) {
                if (it->second == def.insecure_value &&
                    def.secure_value != def.insecure_value) {
                    result.secure = false;
                    result.issues.push_back(def.name + ": " + def.reason);
                }
            }
        }

        // Check for explicitly dangerous settings
        if (config.count("rpcpassword") && config.at("rpcpassword").length() < 12) {
            result.secure = false;
            result.issues.push_back("rpcpassword too short (min 12 chars)");
        }

        return result;
    }
};

/**
 * Update Mechanism Security
 */
class UpdateSecurityManager {
public:
    struct UpdateManifest {
        std::string version;
        std::string download_url;
        std::string sha256_hash;
        std::vector<std::array<uint8_t, 64>> signatures;  // Dilithium5
        std::string release_notes_url;
        uint64_t timestamp;
    };

    struct VerificationResult {
        bool valid = false;
        std::string error;
    };

    static VerificationResult verify_update(const UpdateManifest& manifest,
                                           const std::string& downloaded_hash,
                                           size_t required_sigs = 2) {
        VerificationResult result;

        // Verify hash matches
        if (manifest.sha256_hash != downloaded_hash) {
            result.error = "Hash mismatch";
            return result;
        }

        // Verify HTTPS
        if (manifest.download_url.find("https://") != 0) {
            result.error = "Download URL must use HTTPS";
            return result;
        }

        // Verify signatures (placeholder - integrate with actual crypto)
        if (manifest.signatures.size() < required_sigs) {
            result.error = "Insufficient signatures";
            return result;
        }

        // Verify timestamp is reasonable (not in future, not too old)
        uint64_t now = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();

        if (manifest.timestamp > now + 3600) {  // 1 hour tolerance
            result.error = "Manifest timestamp in future";
            return result;
        }

        if (manifest.timestamp < now - 86400 * 30) {  // 30 days
            result.error = "Manifest too old";
            return result;
        }

        result.valid = true;
        return result;
    }

    static std::vector<std::string> get_trusted_update_sources() {
        return {
            "https://intcoin.org/releases/",
            "https://github.com/intcoin/releases/",
        };
    }
};

/**
 * Deployment Security Manager - Central coordinator
 */
class DeploymentSecurityManager {
public:
    static DeploymentSecurityManager& instance() {
        static DeploymentSecurityManager inst;
        return inst;
    }

    struct DeploymentCheck {
        bool passed = true;
        std::vector<std::string> failures;
        std::vector<std::string> warnings;
    };

    DeploymentCheck run_all_checks(
            const std::string& install_script,
            const std::string& config_content,
            const std::unordered_map<std::string, std::string>& config_map) {
        DeploymentCheck result;

        // Check install script
        auto script_result = InstallScriptValidator::validate(install_script);
        if (!script_result.safe) {
            result.passed = false;
            for (const auto& issue : script_result.issues) {
                result.failures.push_back("Install script: " + issue);
            }
        }

        // Check for secrets in config
        auto secret_result = SecretDetector::scan(config_content);
        if (secret_result.has_secrets) {
            for (const auto& finding : secret_result.findings) {
                result.warnings.push_back("Config secret: " + finding);
            }
        }

        // Check secure defaults
        auto defaults_result = SecureDefaultsManager::validate_config(config_map);
        if (!defaults_result.secure) {
            for (const auto& issue : defaults_result.issues) {
                result.warnings.push_back("Insecure default: " + issue);
            }
        }

        return result;
    }

private:
    DeploymentSecurityManager() = default;
};

} // namespace deployment
} // namespace intcoin

#endif // INTCOIN_DEPLOYMENT_SECURITY_H
