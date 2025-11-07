// Copyright (c) 2025 INTcoin Core (Maddison Lane)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef INTCOIN_CONTRACTS_CONTRACT_H
#define INTCOIN_CONTRACTS_CONTRACT_H

#include "vm.h"
#include "../primitives.h"
#include "../transaction.h"
#include <string>
#include <memory>

namespace intcoin {
namespace contracts {

/**
 * Contract state
 */
enum class ContractState {
    ACTIVE,
    PAUSED,
    DESTROYED
};

/**
 * Smart contract
 */
class Contract {
public:
    Contract(const Hash256& address, const std::vector<uint8_t>& bytecode);
    ~Contract();

    // Execution
    ExecutionResult execute(const std::vector<uint8_t>& call_data,
                           const Hash256& caller,
                           uint64_t value,
                           uint64_t gas_limit,
                           std::vector<uint8_t>& return_data);

    // State management
    ContractState get_state() const { return state_; }
    void pause();
    void resume();
    void destroy();

    // Contract info
    Hash256 get_address() const { return address_; }
    std::vector<uint8_t> get_bytecode() const { return bytecode_; }
    Hash256 get_creator() const { return creator_; }
    uint64_t get_balance() const { return balance_; }
    uint32_t get_creation_block() const { return creation_block_; }

    // Balance operations with overflow protection
    bool add_balance(uint64_t amount);
    bool sub_balance(uint64_t amount);

    // Storage access
    ContractStorage& get_storage() { return storage_; }
    const ContractStorage& get_storage() const { return storage_; }

    // Serialization
    std::vector<uint8_t> serialize() const;
    static std::optional<Contract> deserialize(const std::vector<uint8_t>& data);

    // Validation
    bool validate() const;

private:
    Hash256 address_;
    Hash256 creator_;
    std::vector<uint8_t> bytecode_;
    ContractStorage storage_;
    ContractState state_;
    uint64_t balance_;
    uint32_t creation_block_;

    std::unique_ptr<VirtualMachine> vm_;
    mutable std::mutex contract_mutex_;

    // Input validation
    bool validate_call_data(const std::vector<uint8_t>& data) const;
    bool validate_caller(const Hash256& caller) const;
};

/**
 * Contract deployment transaction
 */
struct ContractDeployment {
    Hash256 deployer;
    std::vector<uint8_t> bytecode;
    std::vector<uint8_t> constructor_args;
    uint64_t value;
    uint64_t gas_limit;
    uint64_t gas_price;

    ContractDeployment() : value(0), gas_limit(0), gas_price(0) {}

    // Calculate contract address
    Hash256 calculate_address() const;

    // Validate deployment with input validation
    bool validate() const;

    // Check bytecode safety
    bool is_safe_bytecode() const;
};

/**
 * Contract call transaction
 */
struct ContractCall {
    Hash256 from;
    Hash256 to;
    std::vector<uint8_t> data;
    uint64_t value;
    uint64_t gas_limit;
    uint64_t gas_price;

    ContractCall() : value(0), gas_limit(0), gas_price(0) {}

    // Validate call with input validation
    bool validate() const;

    // Check for reentrancy patterns
    bool is_safe_call() const;
};

/**
 * Contract manager
 */
class ContractManager {
public:
    ContractManager();
    ~ContractManager();

    // Contract deployment with validation
    std::optional<Hash256> deploy_contract(const ContractDeployment& deployment,
                                          uint32_t block_number);

    // Contract execution with safety checks
    ExecutionResult call_contract(const ContractCall& call,
                                 std::vector<uint8_t>& return_data,
                                 uint64_t& gas_used);

    // Contract queries
    std::shared_ptr<Contract> get_contract(const Hash256& address);
    bool contract_exists(const Hash256& address) const;
    std::vector<Hash256> get_all_contracts() const;

    // Contract state
    bool pause_contract(const Hash256& address);
    bool resume_contract(const Hash256& address);
    bool destroy_contract(const Hash256& address);

    // Statistics
    size_t get_contract_count() const;
    uint64_t get_total_gas_used() const;

    // Validation
    bool validate_deployment(const ContractDeployment& deployment) const;
    bool validate_call(const ContractCall& call) const;

private:
    std::unordered_map<Hash256, std::shared_ptr<Contract>> contracts_;
    mutable std::mutex contracts_mutex_;

    uint64_t total_gas_used_;

    // Security checks
    bool check_deployment_limits(const ContractDeployment& deployment) const;
    bool check_call_limits(const ContractCall& call) const;
    bool detect_reentrancy(const Hash256& address) const;

    std::unordered_set<Hash256> active_calls_;  // For reentrancy detection
};

/**
 * Standard contract interfaces
 */

// ERC20-like token interface
class IToken {
public:
    virtual ~IToken() = default;

    virtual uint64_t total_supply() const = 0;
    virtual uint64_t balance_of(const Hash256& account) const = 0;
    virtual bool transfer(const Hash256& to, uint64_t amount) = 0;
    virtual bool transfer_from(const Hash256& from, const Hash256& to, uint64_t amount) = 0;
    virtual bool approve(const Hash256& spender, uint64_t amount) = 0;
    virtual uint64_t allowance(const Hash256& owner, const Hash256& spender) const = 0;
};

// NFT interface
class INFT {
public:
    virtual ~INFT() = default;

    virtual Hash256 owner_of(uint64_t token_id) const = 0;
    virtual bool transfer(const Hash256& to, uint64_t token_id) = 0;
    virtual bool approve(const Hash256& spender, uint64_t token_id) = 0;
    virtual std::optional<Hash256> get_approved(uint64_t token_id) const = 0;
};

/**
 * Contract templates with built-in security
 */

// Simple token contract with overflow protection
class TokenContract : public IToken {
public:
    TokenContract(const Hash256& owner, uint64_t initial_supply);

    uint64_t total_supply() const override { return total_supply_; }
    uint64_t balance_of(const Hash256& account) const override;
    bool transfer(const Hash256& to, uint64_t amount) override;
    bool transfer_from(const Hash256& from, const Hash256& to, uint64_t amount) override;
    bool approve(const Hash256& spender, uint64_t amount) override;
    uint64_t allowance(const Hash256& owner, const Hash256& spender) const override;

private:
    uint64_t total_supply_;
    std::unordered_map<Hash256, uint64_t> balances_;
    std::unordered_map<Hash256, std::unordered_map<Hash256, uint64_t>> allowances_;

    // Input validation
    bool validate_address(const Hash256& addr) const;
    bool validate_amount(uint64_t amount) const;
};

/**
 * Contract security analyzer
 */
class SecurityAnalyzer {
public:
    struct SecurityReport {
        bool has_integer_overflow_risk;
        bool has_reentrancy_risk;
        bool has_unbounded_loops;
        bool has_timestamp_dependence;
        bool has_unchecked_calls;
        std::vector<std::string> warnings;

        SecurityReport() : has_integer_overflow_risk(false),
                          has_reentrancy_risk(false),
                          has_unbounded_loops(false),
                          has_timestamp_dependence(false),
                          has_unchecked_calls(false) {}

        bool is_safe() const {
            return !has_integer_overflow_risk &&
                   !has_reentrancy_risk &&
                   !has_unbounded_loops &&
                   !has_timestamp_dependence &&
                   !has_unchecked_calls;
        }
    };

    // Analyze contract bytecode for security issues
    static SecurityReport analyze(const std::vector<uint8_t>& bytecode);

    // Check for specific vulnerabilities
    static bool check_integer_overflow(const std::vector<uint8_t>& bytecode);
    static bool check_reentrancy(const std::vector<uint8_t>& bytecode);
    static bool check_unbounded_loops(const std::vector<uint8_t>& bytecode);
    static bool check_timestamp_dependence(const std::vector<uint8_t>& bytecode);

private:
    // Pattern detection
    static std::vector<size_t> find_call_instructions(const std::vector<uint8_t>& bytecode);
    static std::vector<size_t> find_storage_writes(const std::vector<uint8_t>& bytecode);
    static std::vector<size_t> find_jumps(const std::vector<uint8_t>& bytecode);
};

} // namespace contracts
} // namespace intcoin

#endif // INTCOIN_CONTRACTS_CONTRACT_H
