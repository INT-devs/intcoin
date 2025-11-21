// INTcoin Smart Contract SDK
// Copyright (c) 2024-2025 INTcoin Developers
// Distributed under the MIT software license

#ifndef INTCOIN_CONTRACTS_SDK_H
#define INTCOIN_CONTRACTS_SDK_H

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <optional>
#include <map>
#include "vm.h"

namespace intcoin {
namespace contracts {

// ============================================================================
// Contract Testing Framework
// ============================================================================

struct TestResult {
    std::string name;
    bool passed;
    std::string message;
    uint64_t gas_used;
    double duration_ms;
};

struct TestSuite {
    std::string name;
    std::vector<TestResult> results;
    int passed_count;
    int failed_count;
    uint64_t total_gas_used;
};

class MockState : public StateInterface {
public:
    // Storage
    std::map<Address, std::map<Hash256, Word>> storage;
    std::map<Address, std::vector<uint8_t>> code;
    std::map<Address, uint64_t> balances;
    std::map<Address, uint64_t> nonces;

    // StateInterface implementation
    Word get_storage(const Address& addr, const Hash256& key) const override;
    void set_storage(const Address& addr, const Hash256& key, const Word& value) override;
    std::vector<uint8_t> get_code(const Address& addr) const override;
    uint64_t get_balance(const Address& addr) const override;
    bool transfer(const Address& from, const Address& to, uint64_t amount) override;

    // Test helpers
    void set_balance(const Address& addr, uint64_t amount);
    void set_code(const Address& addr, const std::vector<uint8_t>& bytecode);
    void reset();
};

class ContractTest {
public:
    using TestFunc = std::function<bool(MockState&, VM&)>;

    ContractTest(const std::string& name);

    // Test setup
    void set_bytecode(const std::vector<uint8_t>& code);
    void set_sender(const Address& sender);
    void set_value(uint64_t value);
    void set_gas_limit(uint64_t gas);

    // Assertions
    void expect_storage(const Hash256& key, const Word& expected);
    void expect_balance(const Address& addr, uint64_t expected);
    void expect_return(const std::vector<uint8_t>& expected);
    void expect_revert();
    void expect_gas_used(uint64_t min, uint64_t max);

    // Run test
    TestResult run();

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

class TestRunner {
public:
    TestRunner();
    ~TestRunner();

    void add_test(const std::string& suite, ContractTest test);
    void add_test(const std::string& suite, const std::string& name,
                  std::function<bool()> test_func);

    TestSuite run_suite(const std::string& suite);
    std::vector<TestSuite> run_all();

    // Output
    void print_results(const TestSuite& suite);
    void print_summary(const std::vector<TestSuite>& suites);
    std::string generate_report(const std::vector<TestSuite>& suites);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

// ============================================================================
// Contract Templates
// ============================================================================

namespace templates {

// ERC20-compatible token template
std::vector<uint8_t> erc20_token(
    const std::string& name,
    const std::string& symbol,
    uint8_t decimals,
    uint64_t initial_supply
);

// Simple storage contract
std::vector<uint8_t> simple_storage();

// Multi-signature wallet
std::vector<uint8_t> multisig_wallet(
    const std::vector<Address>& owners,
    uint32_t required_signatures
);

// Time-locked vault
std::vector<uint8_t> timelock_vault(uint64_t unlock_time);

// Quantum-safe escrow
std::vector<uint8_t> quantum_escrow(
    const Address& buyer,
    const Address& seller,
    const Address& arbiter
);

// NFT contract (ERC721-compatible)
std::vector<uint8_t> nft_contract(
    const std::string& name,
    const std::string& symbol
);

// Staking contract
std::vector<uint8_t> staking_contract(
    uint64_t reward_rate,
    uint64_t min_stake,
    uint64_t lock_period
);

} // namespace templates

// ============================================================================
// SDK Utilities
// ============================================================================

class SDK {
public:
    SDK();
    ~SDK();

    // Compilation (Solidity to bytecode)
    struct CompileResult {
        bool success;
        std::vector<uint8_t> bytecode;
        std::string abi_json;
        std::vector<std::string> errors;
        std::vector<std::string> warnings;
    };

    CompileResult compile_solidity(const std::string& source);
    CompileResult compile_file(const std::string& path);

    // Deployment
    struct DeployResult {
        bool success;
        Address contract_address;
        Hash256 tx_hash;
        uint64_t gas_used;
        std::string error;
    };

    DeployResult deploy(
        const std::vector<uint8_t>& bytecode,
        const std::vector<uint8_t>& constructor_args,
        uint64_t gas_limit,
        uint64_t value = 0
    );

    // Contract interaction
    struct CallResult {
        bool success;
        std::vector<uint8_t> return_data;
        uint64_t gas_used;
        std::vector<LogEntry> logs;
        std::string error;
    };

    CallResult call(
        const Address& contract,
        const std::vector<uint8_t>& calldata,
        uint64_t gas_limit,
        uint64_t value = 0
    );

    // ABI helpers
    std::vector<uint8_t> encode_function_call(
        const std::string& signature,
        const std::vector<std::string>& args
    );

    std::vector<std::string> decode_return_data(
        const std::string& types,
        const std::vector<uint8_t>& data
    );

    // Gas estimation
    uint64_t estimate_gas(
        const Address& contract,
        const std::vector<uint8_t>& calldata
    );

    // Event parsing
    struct ParsedEvent {
        std::string name;
        std::map<std::string, std::string> indexed_args;
        std::map<std::string, std::string> data_args;
    };

    ParsedEvent parse_log(const LogEntry& log, const std::string& abi_json);

    // Configuration
    void set_sender(const Address& sender);
    void set_state(std::shared_ptr<StateInterface> state);
    void set_block_number(uint64_t block);
    void set_timestamp(uint64_t timestamp);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

// ============================================================================
// CLI Tool Support
// ============================================================================

class ContractCLI {
public:
    static int run(int argc, char* argv[]);

    // Commands
    static int compile(const std::string& input, const std::string& output);
    static int deploy(const std::string& bytecode_file, const std::string& args);
    static int call(const std::string& address, const std::string& function,
                    const std::vector<std::string>& args);
    static int test(const std::string& test_dir);
    static int verify(const std::string& address, const std::string& source);
    static int generate(const std::string& template_name, const std::string& output);
};

} // namespace contracts
} // namespace intcoin

#endif // INTCOIN_CONTRACTS_SDK_H
