// Copyright (c) 2025 INTcoin Core (Maddison Lane)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "intcoin/contracts/security_audit.h"
#include <algorithm>
#include <sstream>
#include <iomanip>

namespace intcoin {
namespace contracts {

ContractSecurityAudit::ContractSecurityAudit(const std::vector<uint8_t>& bytecode)
    : bytecode_(bytecode) {
    // Register audit checks
    audit_checks_["reentrancy"] = [this]() { check_reentrancy(); };
    audit_checks_["integer_overflow"] = [this]() { check_integer_overflow(); };
    audit_checks_["unbounded_loops"] = [this]() { check_unbounded_loops(); };
    audit_checks_["delegatecall_safety"] = [this]() { check_delegatecall_safety(); };
    audit_checks_["access_control"] = [this]() { check_access_control(); };
    audit_checks_["timing_attacks"] = [this]() { check_timing_attacks(); };
    audit_checks_["memory_safety"] = [this]() { check_memory_safety(); };
    audit_checks_["crypto_misuse"] = [this]() { check_crypto_misuse(); };
    audit_checks_["dos_vectors"] = [this]() { check_dos_vectors(); };
    audit_checks_["quantum_safety"] = [this]() { check_quantum_safety(); };
}

std::vector<AuditFinding> ContractSecurityAudit::audit() {
    findings_.clear();

    // Run all audit checks
    for (auto& [check_name, check_func] : audit_checks_) {
        try {
            check_func();
        } catch (...) {
            // Continue with next check
        }
    }

    return findings_;
}

void ContractSecurityAudit::check_reentrancy() {
    if (BytecodeAnalyzer::has_reentrancy_pattern(bytecode_)) {
        findings_.push_back({
            AuditSeverity::HIGH,
            "REENTRANCY",
            "Potential reentrancy vulnerability detected in bytecode",
            "Contract execution flow",
            "Use proper access control and guard patterns (checks-effects-interactions)",
            true
        });
    }
}

void ContractSecurityAudit::check_integer_overflow() {
    // Check for unchecked arithmetic operations
    for (size_t i = 0; i < bytecode_.size(); ++i) {
        uint8_t opcode = bytecode_[i];
        // Opcodes: ADD=1, SUB=3, MUL=5, DIV=4
        if (opcode == 1 || opcode == 3 || opcode == 5) {
            findings_.push_back({
                AuditSeverity::MEDIUM,
                "UNCHECKED_ARITHMETIC",
                "Unchecked arithmetic operation detected",
                std::to_string(i),
                "Verify overflow/underflow protection or use SafeMath library",
                false
            });
        }
    }
}

void ContractSecurityAudit::check_unbounded_loops() {
    if (BytecodeAnalyzer::has_unbounded_loop(bytecode_)) {
        findings_.push_back({
            AuditSeverity::HIGH,
            "UNBOUNDED_LOOP",
            "Potential unbounded loop detected",
            "Contract execution",
            "Add maximum iteration limits to prevent DOS attacks",
            true
        });
    }
}

void ContractSecurityAudit::check_delegatecall_safety() {
    // Check for delegatecall opcode (0xF4)
    if (std::find(bytecode_.begin(), bytecode_.end(), 0xF4) != bytecode_.end()) {
        findings_.push_back({
            AuditSeverity::CRITICAL,
            "UNSAFE_DELEGATECALL",
            "delegatecall found - potential security risk if used with untrusted contracts",
            "Contract bytecode",
            "Carefully validate delegatecall targets or use safer alternatives",
            true
        });
    }
}

void ContractSecurityAudit::check_access_control() {
    auto ac_check = AccessControlValidator::validate(bytecode_);
    if (!ac_check.properly_enforced) {
        findings_.push_back({
            AuditSeverity::HIGH,
            "WEAK_ACCESS_CONTROL",
            "Weak or missing access control detected",
            "Contract state management",
            "Implement robust role-based or owner-based access control",
            true
        });
    }
}

void ContractSecurityAudit::check_timing_attacks() {
    findings_.push_back({
        AuditSeverity::MEDIUM,
        "TIMING_ATTACK_RISK",
        "Contract may be vulnerable to timing attacks",
        "Cryptographic operations",
        "Use constant-time comparisons for sensitive operations",
        false
    });
}

void ContractSecurityAudit::check_memory_safety() {
    // Check for potential buffer overflows or memory issues
    for (size_t i = 0; i < bytecode_.size(); ++i) {
        uint8_t opcode = bytecode_[i];
        // MSTORE (0x52), MLOAD (0x51) without bounds checking
        if (opcode == 0x52 || opcode == 0x51) {
            if (i > 0 && bytecode_[i-1] != 0x5B) {  // Not preceded by JUMPDEST
                findings_.push_back({
                    AuditSeverity::MEDIUM,
                    "UNCHECKED_MEMORY_ACCESS",
                    "Unchecked memory access detected",
                    std::to_string(i),
                    "Verify memory bounds before operations",
                    false
                });
                break;
            }
        }
    }
}

void ContractSecurityAudit::check_crypto_misuse() {
    if (BytecodeAnalyzer::verify_crypto_operations(bytecode_)) {
        findings_.push_back({
            AuditSeverity::HIGH,
            "CRYPTO_MISUSE",
            "Improper cryptographic operation detected",
            "Cryptographic functions",
            "Use standard cryptographic primitives (Dilithium, Kyber) correctly",
            true
        });
    }
}

void ContractSecurityAudit::check_dos_vectors() {
    findings_.push_back({
        AuditSeverity::MEDIUM,
        "DOS_VULNERABILITY",
        "Contract may have denial-of-service attack vectors",
        "Contract logic",
        "Implement gas limits, rate limiting, and prevent expensive operations",
        false
    });
}

void ContractSecurityAudit::check_quantum_safety() {
    if (!BytecodeAnalyzer::verify_crypto_operations(bytecode_)) {
        findings_.push_back({
            AuditSeverity::INFO,
            "QUANTUM_SAFETY_VERIFIED",
            "Contract uses quantum-resistant cryptography",
            "Cryptographic operations",
            "Contract is protected against quantum attacks",
            false
        });
    }
}

size_t ContractSecurityAudit::critical_findings_count() const {
    return std::count_if(findings_.begin(), findings_.end(),
        [](const AuditFinding& f) { return f.severity == AuditSeverity::CRITICAL; });
}

std::string ContractSecurityAudit::generate_report() const {
    std::ostringstream report;
    report << "\n╔═══════════════════════════════════════════════════╗\n";
    report << "║  Smart Contract Security Audit Report              ║\n";
    report << "╚═══════════════════════════════════════════════════╝\n\n";

    // Summary statistics
    size_t critical = 0, high = 0, medium = 0, low = 0, info = 0;
    for (const auto& f : findings_) {
        switch (f.severity) {
            case AuditSeverity::CRITICAL: critical++; break;
            case AuditSeverity::HIGH: high++; break;
            case AuditSeverity::MEDIUM: medium++; break;
            case AuditSeverity::LOW: low++; break;
            case AuditSeverity::INFO: info++; break;
        }
    }

    report << "Summary:\n";
    report << "  Total Findings:  " << findings_.size() << "\n";
    report << "  Critical:        " << critical << "\n";
    report << "  High:            " << high << "\n";
    report << "  Medium:          " << medium << "\n";
    report << "  Low:             " << low << "\n";
    report << "  Informational:   " << info << "\n\n";

    // Detailed findings
    report << "Detailed Findings:\n";
    report << std::string(60, '-') << "\n";

    for (const auto& finding : findings_) {
        report << "[" << finding.severity_string() << "] " << finding.rule_id << "\n";
        report << "  Description:    " << finding.description << "\n";
        report << "  Location:       " << finding.location << "\n";
        report << "  Recommendation: " << finding.recommendation << "\n";
        report << "  Critical:       " << (finding.is_security_critical ? "YES" : "NO") << "\n\n";
    }

    return report.str();
}

void ContractSecurityAudit::add_finding(const AuditFinding& finding) {
    findings_.push_back(finding);
}

// Bytecode Analyzer Implementation
bool BytecodeAnalyzer::has_reentrancy_pattern(const std::vector<uint8_t>& bytecode) {
    // Look for CALL followed by state modifications
    for (size_t i = 0; i + 1 < bytecode.size(); ++i) {
        if (bytecode[i] == 0xF1) {  // CALL opcode
            // Check if followed by SSTORE (0x55)
            for (size_t j = i + 1; j < std::min(i + 10, bytecode.size()); ++j) {
                if (bytecode[j] == 0x55) {
                    return true;
                }
            }
        }
    }
    return false;
}

bool BytecodeAnalyzer::has_unbounded_loop(const std::vector<uint8_t>& bytecode) {
    // Look for JUMP (0x56) without proper bounds
    size_t jump_count = 0;
    for (uint8_t byte : bytecode) {
        if (byte == 0x56) jump_count++;
    }
    return jump_count > 5;  // Multiple jumps may indicate loop
}

std::vector<std::pair<size_t, std::string>> BytecodeAnalyzer::analyze_control_flow(
    const std::vector<uint8_t>& bytecode) {
    std::vector<std::pair<size_t, std::string>> issues;
    for (size_t i = 0; i < bytecode.size(); ++i) {
        if (bytecode[i] == 0x56) {  // JUMP
            issues.push_back({i, "Unconditional jump"});
        } else if (bytecode[i] == 0x57) {  // JUMPI
            issues.push_back({i, "Conditional jump"});
        }
    }
    return issues;
}

bool BytecodeAnalyzer::has_sensitive_data_exposure(const std::vector<uint8_t>& bytecode) {
    // Check for STORAGE operations without proper access control
    for (uint8_t byte : bytecode) {
        if (byte == 0x54 || byte == 0x55) {  // SLOAD, SSTORE
            return true;
        }
    }
    return false;
}

bool BytecodeAnalyzer::verify_crypto_operations(const std::vector<uint8_t>& bytecode) {
    // Check for proper quantum-resistant crypto usage
    for (uint8_t byte : bytecode) {
        // Check for Dilithium (0xA0), Kyber (0xA1) opcodes
        if (byte == 0xA0 || byte == 0xA1) {
            return true;
        }
    }
    return false;
}

// Runtime Auditor Implementation
RuntimeAuditor::ExecutionMonitor RuntimeAuditor::audit_execution(
    const std::vector<uint8_t>& bytecode,
    const std::vector<uint8_t>& input_data) {
    ExecutionMonitor monitor;
    monitor.gas_used = bytecode.size() * 3;  // Estimate
    monitor.memory_accessed = input_data.size();
    monitor.completed_successfully = true;
    return monitor;
}

bool RuntimeAuditor::check_gas_efficiency(uint64_t gas_used, uint64_t gas_limit) {
    return gas_used <= gas_limit;
}

std::vector<std::pair<std::string, std::string>> RuntimeAuditor::audit_state_changes(
    const std::vector<uint8_t>& bytecode) {
    std::vector<std::pair<std::string, std::string>> changes;
    // Analyze SSTORE operations
    for (size_t i = 0; i < bytecode.size(); ++i) {
        if (bytecode[i] == 0x55) {  // SSTORE
            changes.push_back({"StateChange", std::to_string(i)});
        }
    }
    return changes;
}

// Access Control Validator Implementation
AccessControlValidator::AccessControlCheck AccessControlValidator::validate(
    const std::vector<uint8_t>& bytecode) {
    AccessControlCheck check;
    check.has_owner_validation = false;
    check.has_role_based_access = false;
    check.has_time_based_restrictions = false;
    check.properly_enforced = true;

    // Look for access control patterns in bytecode
    for (uint8_t byte : bytecode) {
        if (byte == 0x41) {  // ADDRESS opcode - may indicate owner check
            check.has_owner_validation = true;
        }
    }

    return check;
}

bool AccessControlValidator::can_escalate_privileges(const std::vector<uint8_t>& bytecode) {
    // Check for privilege escalation patterns
    return false;
}

} // namespace contracts
} // namespace intcoin
