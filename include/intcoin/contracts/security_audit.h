// Copyright (c) 2025 INTcoin Core (Maddison Lane)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef INTCOIN_CONTRACTS_SECURITY_AUDIT_H
#define INTCOIN_CONTRACTS_SECURITY_AUDIT_H

#include <string>
#include <vector>
#include <map>
#include <functional>
#include <cstdint>

namespace intcoin {
namespace contracts {

/**
 * Security audit severity levels
 */
enum class AuditSeverity {
    CRITICAL,  // Immediate security risk
    HIGH,      // Significant vulnerability
    MEDIUM,    // Potential issue
    LOW,       // Minor issue
    INFO       // Informational
};

/**
 * Security audit finding
 */
struct AuditFinding {
    AuditSeverity severity;
    std::string rule_id;
    std::string description;
    std::string location;
    std::string recommendation;
    bool is_security_critical;

    std::string severity_string() const {
        switch (severity) {
            case AuditSeverity::CRITICAL: return "CRITICAL";
            case AuditSeverity::HIGH: return "HIGH";
            case AuditSeverity::MEDIUM: return "MEDIUM";
            case AuditSeverity::LOW: return "LOW";
            case AuditSeverity::INFO: return "INFO";
            default: return "UNKNOWN";
        }
    }
};

/**
 * Smart Contract Security Analyzer
 * Performs comprehensive security audits on contract bytecode and behavior
 */
class ContractSecurityAudit {
private:
    std::vector<uint8_t> bytecode_;
    std::vector<AuditFinding> findings_;
    std::map<std::string, std::function<void()>> audit_checks_;

public:
    ContractSecurityAudit(const std::vector<uint8_t>& bytecode);

    /**
     * Run full security audit
     */
    std::vector<AuditFinding> audit();

    /**
     * Check for reentrancy vulnerabilities
     */
    void check_reentrancy();

    /**
     * Check for integer overflow/underflow
     */
    void check_integer_overflow();

    /**
     * Check for unbounded loops
     */
    void check_unbounded_loops();

    /**
     * Check for unsafe delegatecall usage
     */
    void check_delegatecall_safety();

    /**
     * Check for access control issues
     */
    void check_access_control();

    /**
     * Check for timing attack vulnerabilities
     */
    void check_timing_attacks();

    /**
     * Check for memory safety issues
     */
    void check_memory_safety();

    /**
     * Check for cryptographic misuse
     */
    void check_crypto_misuse();

    /**
     * Check for denial of service vectors
     */
    void check_dos_vectors();

    /**
     * Check for quantum safety
     */
    void check_quantum_safety();

    /**
     * Get audit results
     */
    const std::vector<AuditFinding>& get_findings() const {
        return findings_;
    }

    /**
     * Get critical findings count
     */
    size_t critical_findings_count() const;

    /**
     * Get audit report
     */
    std::string generate_report() const;

private:
    void add_finding(const AuditFinding& finding);
};

/**
 * Bytecode Analysis Engine
 */
class BytecodeAnalyzer {
public:
    /**
     * Analyze opcode patterns for security issues
     */
    static bool has_reentrancy_pattern(const std::vector<uint8_t>& bytecode);

    /**
     * Check for unbounded loops
     */
    static bool has_unbounded_loop(const std::vector<uint8_t>& bytecode);

    /**
     * Analyze control flow for vulnerabilities
     */
    static std::vector<std::pair<size_t, std::string>> analyze_control_flow(
        const std::vector<uint8_t>& bytecode);

    /**
     * Check for sensitive data exposure
     */
    static bool has_sensitive_data_exposure(const std::vector<uint8_t>& bytecode);

    /**
     * Verify cryptographic operations
     */
    static bool verify_crypto_operations(const std::vector<uint8_t>& bytecode);
};

/**
 * Runtime Behavior Auditor
 */
class RuntimeAuditor {
public:
    /**
     * Monitor contract execution for violations
     */
    struct ExecutionMonitor {
        size_t gas_used;
        size_t memory_accessed;
        std::vector<std::string> operations;
        bool completed_successfully;
    };

    /**
     * Audit contract execution
     */
    static ExecutionMonitor audit_execution(
        const std::vector<uint8_t>& bytecode,
        const std::vector<uint8_t>& input_data);

    /**
     * Check for gas limit issues
     */
    static bool check_gas_efficiency(uint64_t gas_used, uint64_t gas_limit);

    /**
     * Monitor state changes
     */
    static std::vector<std::pair<std::string, std::string>> audit_state_changes(
        const std::vector<uint8_t>& bytecode);
};

/**
 * Access Control Validator
 */
class AccessControlValidator {
public:
    /**
     * Verify access control patterns
     */
    struct AccessControlCheck {
        bool has_owner_validation;
        bool has_role_based_access;
        bool has_time_based_restrictions;
        bool properly_enforced;
    };

    /**
     * Validate access control in contract
     */
    static AccessControlCheck validate(const std::vector<uint8_t>& bytecode);

    /**
     * Check for privilege escalation
     */
    static bool can_escalate_privileges(const std::vector<uint8_t>& bytecode);
};

} // namespace contracts
} // namespace intcoin

#endif // INTCOIN_CONTRACTS_SECURITY_AUDIT_H
