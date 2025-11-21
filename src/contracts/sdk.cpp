// INTcoin Smart Contract SDK Implementation
// Copyright (c) 2024-2025 INTcoin Developers
// Distributed under the MIT software license

#include "sdk.h"
#include <chrono>
#include <sstream>
#include <iomanip>
#include <iostream>

namespace intcoin {
namespace contracts {

// ============================================================================
// MockState Implementation
// ============================================================================

Word MockState::get_storage(const Address& addr, const Hash256& key) const {
    auto it = storage.find(addr);
    if (it != storage.end()) {
        auto kit = it->second.find(key);
        if (kit != it->second.end()) {
            return kit->second;
        }
    }
    return Word{};
}

void MockState::set_storage(const Address& addr, const Hash256& key, const Word& value) {
    storage[addr][key] = value;
}

std::vector<uint8_t> MockState::get_code(const Address& addr) const {
    auto it = code.find(addr);
    return (it != code.end()) ? it->second : std::vector<uint8_t>{};
}

uint64_t MockState::get_balance(const Address& addr) const {
    auto it = balances.find(addr);
    return (it != balances.end()) ? it->second : 0;
}

bool MockState::transfer(const Address& from, const Address& to, uint64_t amount) {
    if (balances[from] < amount) return false;
    balances[from] -= amount;
    balances[to] += amount;
    return true;
}

void MockState::set_balance(const Address& addr, uint64_t amount) {
    balances[addr] = amount;
}

void MockState::set_code(const Address& addr, const std::vector<uint8_t>& bytecode) {
    code[addr] = bytecode;
}

void MockState::reset() {
    storage.clear();
    code.clear();
    balances.clear();
    nonces.clear();
}

// ============================================================================
// ContractTest Implementation
// ============================================================================

struct ContractTest::Impl {
    std::string name;
    std::vector<uint8_t> bytecode;
    Address sender;
    uint64_t value = 0;
    uint64_t gas_limit = 1000000;

    std::vector<std::pair<Hash256, Word>> expected_storage;
    std::vector<std::pair<Address, uint64_t>> expected_balances;
    std::optional<std::vector<uint8_t>> expected_return;
    bool expect_revert_flag = false;
    std::optional<std::pair<uint64_t, uint64_t>> expected_gas_range;
};

ContractTest::ContractTest(const std::string& name) : impl_(std::make_unique<Impl>()) {
    impl_->name = name;
}

void ContractTest::set_bytecode(const std::vector<uint8_t>& code) {
    impl_->bytecode = code;
}

void ContractTest::set_sender(const Address& sender) {
    impl_->sender = sender;
}

void ContractTest::set_value(uint64_t value) {
    impl_->value = value;
}

void ContractTest::set_gas_limit(uint64_t gas) {
    impl_->gas_limit = gas;
}

void ContractTest::expect_storage(const Hash256& key, const Word& expected) {
    impl_->expected_storage.push_back({key, expected});
}

void ContractTest::expect_balance(const Address& addr, uint64_t expected) {
    impl_->expected_balances.push_back({addr, expected});
}

void ContractTest::expect_return(const std::vector<uint8_t>& expected) {
    impl_->expected_return = expected;
}

void ContractTest::expect_revert() {
    impl_->expect_revert_flag = true;
}

void ContractTest::expect_gas_used(uint64_t min, uint64_t max) {
    impl_->expected_gas_range = {min, max};
}

TestResult ContractTest::run() {
    auto start = std::chrono::high_resolution_clock::now();

    MockState state;
    VM vm;
    Address contract_addr;

    Message msg;
    msg.sender = impl_->sender;
    msg.value = impl_->value;
    msg.gas = impl_->gas_limit;
    msg.data = impl_->bytecode;

    ExecutionResult result = vm.execute(state, contract_addr, msg);

    auto end = std::chrono::high_resolution_clock::now();
    double duration = std::chrono::duration<double, std::milli>(end - start).count();

    TestResult test_result;
    test_result.name = impl_->name;
    test_result.gas_used = impl_->gas_limit - result.gas_remaining;
    test_result.duration_ms = duration;

    // Check revert expectation
    if (impl_->expect_revert_flag) {
        test_result.passed = (result.status == ExecStatus::Revert);
        if (!test_result.passed) {
            test_result.message = "Expected revert but execution succeeded";
        }
        return test_result;
    }

    // Check success
    if (result.status != ExecStatus::Success) {
        test_result.passed = false;
        test_result.message = "Execution failed: " + std::to_string(static_cast<int>(result.status));
        return test_result;
    }

    // Check return data
    if (impl_->expected_return && result.return_data != *impl_->expected_return) {
        test_result.passed = false;
        test_result.message = "Return data mismatch";
        return test_result;
    }

    // Check gas range
    if (impl_->expected_gas_range) {
        uint64_t gas_used = impl_->gas_limit - result.gas_remaining;
        if (gas_used < impl_->expected_gas_range->first ||
            gas_used > impl_->expected_gas_range->second) {
            test_result.passed = false;
            test_result.message = "Gas usage out of expected range";
            return test_result;
        }
    }

    test_result.passed = true;
    test_result.message = "OK";
    return test_result;
}

// ============================================================================
// TestRunner Implementation
// ============================================================================

struct TestRunner::Impl {
    std::map<std::string, std::vector<std::pair<std::string, std::function<bool()>>>> suites;
};

TestRunner::TestRunner() : impl_(std::make_unique<Impl>()) {}
TestRunner::~TestRunner() = default;

void TestRunner::add_test(const std::string& suite, const std::string& name,
                          std::function<bool()> test_func) {
    impl_->suites[suite].push_back({name, test_func});
}

TestSuite TestRunner::run_suite(const std::string& suite) {
    TestSuite result;
    result.name = suite;
    result.passed_count = 0;
    result.failed_count = 0;
    result.total_gas_used = 0;

    auto it = impl_->suites.find(suite);
    if (it == impl_->suites.end()) return result;

    for (const auto& [name, func] : it->second) {
        auto start = std::chrono::high_resolution_clock::now();
        bool passed = false;
        try {
            passed = func();
        } catch (...) {
            passed = false;
        }
        auto end = std::chrono::high_resolution_clock::now();

        TestResult tr;
        tr.name = name;
        tr.passed = passed;
        tr.message = passed ? "OK" : "FAILED";
        tr.duration_ms = std::chrono::duration<double, std::milli>(end - start).count();

        result.results.push_back(tr);
        if (passed) result.passed_count++;
        else result.failed_count++;
    }

    return result;
}

std::vector<TestSuite> TestRunner::run_all() {
    std::vector<TestSuite> results;
    for (const auto& [name, _] : impl_->suites) {
        results.push_back(run_suite(name));
    }
    return results;
}

void TestRunner::print_results(const TestSuite& suite) {
    std::cout << "\n=== " << suite.name << " ===\n";
    for (const auto& r : suite.results) {
        std::cout << (r.passed ? "[PASS]" : "[FAIL]") << " " << r.name;
        std::cout << " (" << std::fixed << std::setprecision(2) << r.duration_ms << "ms)\n";
    }
    std::cout << "Passed: " << suite.passed_count << "/"
              << (suite.passed_count + suite.failed_count) << "\n";
}

void TestRunner::print_summary(const std::vector<TestSuite>& suites) {
    int total_passed = 0, total_failed = 0;
    for (const auto& s : suites) {
        total_passed += s.passed_count;
        total_failed += s.failed_count;
    }
    std::cout << "\n=== SUMMARY ===\n";
    std::cout << "Total: " << total_passed << " passed, " << total_failed << " failed\n";
}

// ============================================================================
// Contract Templates
// ============================================================================

namespace templates {

std::vector<uint8_t> erc20_token(const std::string& name, const std::string& symbol,
                                  uint8_t decimals, uint64_t initial_supply) {
    // Simplified ERC20 bytecode template
    std::vector<uint8_t> bytecode;
    // PUSH initial_supply, SSTORE to slot 0 (total supply)
    bytecode.push_back(0x7F); // PUSH32
    for (int i = 0; i < 24; i++) bytecode.push_back(0);
    bytecode.push_back((initial_supply >> 56) & 0xFF);
    bytecode.push_back((initial_supply >> 48) & 0xFF);
    bytecode.push_back((initial_supply >> 40) & 0xFF);
    bytecode.push_back((initial_supply >> 32) & 0xFF);
    bytecode.push_back((initial_supply >> 24) & 0xFF);
    bytecode.push_back((initial_supply >> 16) & 0xFF);
    bytecode.push_back((initial_supply >> 8) & 0xFF);
    bytecode.push_back(initial_supply & 0xFF);
    bytecode.push_back(0x60); // PUSH1 0 (slot)
    bytecode.push_back(0x00);
    bytecode.push_back(0x55); // SSTORE
    bytecode.push_back(0x00); // STOP
    return bytecode;
}

std::vector<uint8_t> simple_storage() {
    // Simple storage: SSTORE at slot 0, SLOAD from slot 0
    return {
        0x60, 0x00,       // PUSH1 0
        0x54,             // SLOAD
        0x60, 0x00,       // PUSH1 0
        0x52,             // MSTORE
        0x60, 0x20,       // PUSH1 32
        0x60, 0x00,       // PUSH1 0
        0xF3              // RETURN
    };
}

std::vector<uint8_t> multisig_wallet(const std::vector<Address>& owners,
                                      uint32_t required_signatures) {
    std::vector<uint8_t> bytecode;
    // Store owner count
    bytecode.push_back(0x60); // PUSH1
    bytecode.push_back(static_cast<uint8_t>(owners.size()));
    bytecode.push_back(0x60); // PUSH1 0
    bytecode.push_back(0x00);
    bytecode.push_back(0x55); // SSTORE
    // Store required signatures
    bytecode.push_back(0x60); // PUSH1
    bytecode.push_back(static_cast<uint8_t>(required_signatures));
    bytecode.push_back(0x60); // PUSH1 1
    bytecode.push_back(0x01);
    bytecode.push_back(0x55); // SSTORE
    bytecode.push_back(0x00); // STOP
    return bytecode;
}

std::vector<uint8_t> timelock_vault(uint64_t unlock_time) {
    std::vector<uint8_t> bytecode;
    // Store unlock time
    bytecode.push_back(0x7F); // PUSH32
    for (int i = 0; i < 24; i++) bytecode.push_back(0);
    bytecode.push_back((unlock_time >> 56) & 0xFF);
    bytecode.push_back((unlock_time >> 48) & 0xFF);
    bytecode.push_back((unlock_time >> 40) & 0xFF);
    bytecode.push_back((unlock_time >> 32) & 0xFF);
    bytecode.push_back((unlock_time >> 24) & 0xFF);
    bytecode.push_back((unlock_time >> 16) & 0xFF);
    bytecode.push_back((unlock_time >> 8) & 0xFF);
    bytecode.push_back(unlock_time & 0xFF);
    bytecode.push_back(0x60); // PUSH1 0
    bytecode.push_back(0x00);
    bytecode.push_back(0x55); // SSTORE
    bytecode.push_back(0x00); // STOP
    return bytecode;
}

std::vector<uint8_t> quantum_escrow(const Address& buyer, const Address& seller,
                                     const Address& arbiter) {
    // Escrow with quantum-safe verification
    std::vector<uint8_t> bytecode;
    bytecode.push_back(0x00); // STOP (placeholder)
    return bytecode;
}

std::vector<uint8_t> nft_contract(const std::string& name, const std::string& symbol) {
    std::vector<uint8_t> bytecode;
    bytecode.push_back(0x00); // STOP (placeholder)
    return bytecode;
}

std::vector<uint8_t> staking_contract(uint64_t reward_rate, uint64_t min_stake,
                                       uint64_t lock_period) {
    std::vector<uint8_t> bytecode;
    bytecode.push_back(0x00); // STOP (placeholder)
    return bytecode;
}

} // namespace templates

// ============================================================================
// SDK Implementation
// ============================================================================

struct SDK::Impl {
    Address sender;
    std::shared_ptr<StateInterface> state;
    uint64_t block_number = 0;
    uint64_t timestamp = 0;
    VM vm;
};

SDK::SDK() : impl_(std::make_unique<Impl>()) {}
SDK::~SDK() = default;

SDK::CompileResult SDK::compile_solidity(const std::string& source) {
    CompileResult result;
    // Placeholder - would integrate with solc
    result.success = false;
    result.errors.push_back("Solidity compilation requires solc integration");
    return result;
}

SDK::DeployResult SDK::deploy(const std::vector<uint8_t>& bytecode,
                               const std::vector<uint8_t>& constructor_args,
                               uint64_t gas_limit, uint64_t value) {
    DeployResult result;
    if (!impl_->state) {
        result.success = false;
        result.error = "No state configured";
        return result;
    }

    std::vector<uint8_t> init_code = bytecode;
    init_code.insert(init_code.end(), constructor_args.begin(), constructor_args.end());

    ContractDeployer deployer;
    auto [addr, exec_result] = deployer.deploy(*impl_->state, impl_->sender,
                                                init_code, value, gas_limit);

    result.success = (exec_result.status == ExecStatus::Success);
    result.contract_address = addr;
    result.gas_used = gas_limit - exec_result.gas_remaining;
    return result;
}

SDK::CallResult SDK::call(const Address& contract, const std::vector<uint8_t>& calldata,
                           uint64_t gas_limit, uint64_t value) {
    CallResult result;
    if (!impl_->state) {
        result.success = false;
        result.error = "No state configured";
        return result;
    }

    Message msg;
    msg.sender = impl_->sender;
    msg.value = value;
    msg.gas = gas_limit;
    msg.data = calldata;

    ExecutionResult exec_result = impl_->vm.execute(*impl_->state, contract, msg);

    result.success = (exec_result.status == ExecStatus::Success);
    result.return_data = exec_result.return_data;
    result.gas_used = gas_limit - exec_result.gas_remaining;
    result.logs = exec_result.logs;
    return result;
}

uint64_t SDK::estimate_gas(const Address& contract, const std::vector<uint8_t>& calldata) {
    auto result = call(contract, calldata, 10000000, 0);
    return result.gas_used * 12 / 10; // 20% buffer
}

void SDK::set_sender(const Address& sender) {
    impl_->sender = sender;
}

void SDK::set_state(std::shared_ptr<StateInterface> state) {
    impl_->state = state;
}

void SDK::set_block_number(uint64_t block) {
    impl_->block_number = block;
}

void SDK::set_timestamp(uint64_t timestamp) {
    impl_->timestamp = timestamp;
}

// ============================================================================
// CLI Implementation
// ============================================================================

int ContractCLI::run(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "INTcoin Smart Contract CLI\n";
        std::cout << "Usage: intcoin-contract <command> [args]\n\n";
        std::cout << "Commands:\n";
        std::cout << "  compile <source> <output>  Compile Solidity to bytecode\n";
        std::cout << "  deploy <bytecode> [args]   Deploy contract\n";
        std::cout << "  call <addr> <func> [args]  Call contract function\n";
        std::cout << "  test <dir>                 Run contract tests\n";
        std::cout << "  generate <template> <out>  Generate from template\n";
        return 0;
    }

    std::string cmd = argv[1];
    if (cmd == "compile" && argc >= 4) return compile(argv[2], argv[3]);
    if (cmd == "test" && argc >= 3) return test(argv[2]);
    if (cmd == "generate" && argc >= 4) return generate(argv[2], argv[3]);

    std::cerr << "Unknown command or invalid arguments\n";
    return 1;
}

int ContractCLI::compile(const std::string& input, const std::string& output) {
    std::cout << "Compiling " << input << " -> " << output << "\n";
    SDK sdk;
    auto result = sdk.compile_file(input);
    if (!result.success) {
        for (const auto& err : result.errors) {
            std::cerr << "Error: " << err << "\n";
        }
        return 1;
    }
    std::cout << "Compilation successful\n";
    return 0;
}

int ContractCLI::test(const std::string& test_dir) {
    std::cout << "Running tests in " << test_dir << "\n";
    TestRunner runner;
    auto results = runner.run_all();
    runner.print_summary(results);
    return 0;
}

int ContractCLI::generate(const std::string& template_name, const std::string& output) {
    std::cout << "Generating " << template_name << " -> " << output << "\n";
    std::vector<uint8_t> bytecode;
    if (template_name == "erc20") {
        bytecode = templates::erc20_token("Token", "TKN", 18, 1000000);
    } else if (template_name == "storage") {
        bytecode = templates::simple_storage();
    } else if (template_name == "multisig") {
        bytecode = templates::multisig_wallet({}, 2);
    } else {
        std::cerr << "Unknown template: " << template_name << "\n";
        return 1;
    }
    std::cout << "Generated " << bytecode.size() << " bytes\n";
    return 0;
}

} // namespace contracts
} // namespace intcoin
