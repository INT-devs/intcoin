// Copyright (c) 2025 INTcoin Core (Maddison Lane)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef INTCOIN_EXCHANGE_API_TEST_H
#define INTCOIN_EXCHANGE_API_TEST_H

#include <string>
#include <vector>
#include <map>
#include <cstdint>

namespace intcoin {
namespace exchange {

/**
 * Exchange API Test Result
 */
struct TestResult {
    std::string test_name;
    bool passed;
    std::string error_message;
    double execution_time_ms;
};

/**
 * Exchange Integration API Tester
 * Validates exchange integration features and API compliance
 */
class ExchangeAPITester {
public:
    /**
     * Test hot wallet operations
     */
    static TestResult test_hot_wallet_operations();

    /**
     * Test cold wallet segregation
     */
    static TestResult test_cold_wallet_segregation();

    /**
     * Test multi-signature withdrawal
     */
    static TestResult test_multisig_withdrawal();

    /**
     * Test deposit address generation
     */
    static TestResult test_deposit_address_generation();

    /**
     * Test withdrawal processing
     */
    static TestResult test_withdrawal_processing();

    /**
     * Test transaction signing
     */
    static TestResult test_transaction_signing();

    /**
     * Test balance queries
     */
    static TestResult test_balance_queries();

    /**
     * Test rate limiting
     */
    static TestResult test_rate_limiting();

    /**
     * Test audit logging
     */
    static TestResult test_audit_logging();

    /**
     * Test error handling
     */
    static TestResult test_error_handling();

    /**
     * Test quantum-resistant signatures
     */
    static TestResult test_quantum_signatures();

    /**
     * Test batch operations
     */
    static TestResult test_batch_operations();

    /**
     * Run all exchange API tests
     */
    static std::vector<TestResult> run_all_tests();

    /**
     * Generate test report
     */
    static std::string generate_report(const std::vector<TestResult>& results);
};

/**
 * Exchange API Security Validator
 */
class ExchangeSecurityValidator {
public:
    /**
     * Validate API key security
     */
    struct APIKeyValidation {
        bool key_strength_sufficient;
        bool key_rotation_enforced;
        bool api_rate_limiting_enabled;
        bool request_signing_enforced;
    };

    /**
     * Validate wallet security
     */
    struct WalletSecurity {
        bool multisig_enforced;
        bool cold_storage_separation;
        bool automatic_sweeping;
        bool backup_encryption;
    };

    /**
     * Validate transaction security
     */
    struct TransactionSecurity {
        bool signing_verification;
        bool nonce_protection;
        bool replay_protection;
        bool timelocks_enforced;
    };

    /**
     * Validate API key
     */
    static APIKeyValidation validate_api_key();

    /**
     * Validate wallet configuration
     */
    static WalletSecurity validate_wallet_security();

    /**
     * Validate transaction security
     */
    static TransactionSecurity validate_transaction_security();
};

/**
 * Exchange Performance Tester
 */
class ExchangePerformanceTester {
public:
    /**
     * Measure deposit processing time
     */
    static double measure_deposit_latency(size_t num_deposits = 100);

    /**
     * Measure withdrawal processing time
     */
    static double measure_withdrawal_latency(size_t num_withdrawals = 100);

    /**
     * Measure batch operation throughput
     */
    static double measure_batch_throughput(size_t batch_size = 1000);

    /**
     * Test concurrent operations
     */
    static bool test_concurrent_operations(size_t num_concurrent = 50);

    /**
     * Test under load
     */
    static bool stress_test(size_t num_operations = 10000);
};

/**
 * Exchange Compliance Checker
 */
class ExchangeComplianceChecker {
public:
    /**
     * Check AML compliance
     */
    static bool check_aml_compliance();

    /**
     * Check KYC requirements
     */
    static bool check_kyc_requirements();

    /**
     * Check transaction limits
     */
    static bool check_transaction_limits();

    /**
     * Check audit trail completeness
     */
    static bool check_audit_trail();

    /**
     * Check data retention
     */
    static bool check_data_retention();
};

} // namespace exchange
} // namespace intcoin

#endif // INTCOIN_EXCHANGE_API_TEST_H
