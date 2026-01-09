// Copyright (c) 2026 The INTcoin Core developers
// Distributed under the MIT software license

#ifndef INTCOIN_CONTRACTS_VALIDATOR_H
#define INTCOIN_CONTRACTS_VALIDATOR_H

#include "intcoin/types.h"
#include "intcoin/transaction.h"
#include "intcoin/contracts/transaction.h"
#include "intcoin/contracts/database.h"
#include "intcoin/contracts/vm.h"

namespace intcoin {

// Forward declarations
class Blockchain;

namespace contracts {

/**
 * Contract Transaction Validator
 *
 * Validates contract deployment and call transactions
 */
class ContractTxValidator {
public:
    /// Constructor
    explicit ContractTxValidator(const Blockchain& chain);

    /// Validate contract transaction
    Result<void> Validate(const Transaction& tx) const;

    /// Validate contract deployment transaction
    Result<void> ValidateDeployment(const ContractDeploymentTx& tx) const;

    /// Validate contract call transaction
    Result<void> ValidateCall(const ContractCallTx& tx) const;

private:
    const Blockchain& chain_;
};

/**
 * Contract Executor
 *
 * Executes contract transactions during block processing
 */
class ContractExecutor {
public:
    /// Constructor
    explicit ContractExecutor(ContractDatabase& db);

    /// Execute contract deployment
    /// @param tx Transaction containing deployment data
    /// @param block_height Current block height
    /// @param block_timestamp Current block timestamp
    /// @return Transaction receipt
    Result<TransactionReceipt> ExecuteDeployment(
        const ContractDeploymentTx& deploy_tx,
        const uint256& tx_hash,
        uint64_t block_height,
        uint64_t block_timestamp,
        uint32_t tx_index
    );

    /// Execute contract call
    /// @param tx Transaction containing call data
    /// @param block_height Current block height
    /// @param block_timestamp Current block timestamp
    /// @return Transaction receipt
    Result<TransactionReceipt> ExecuteCall(
        const ContractCallTx& call_tx,
        const uint256& tx_hash,
        uint64_t block_height,
        uint64_t block_timestamp,
        uint32_t tx_index
    );

    /// Get contract database
    ContractDatabase& GetDatabase() { return db_; }

private:
    ContractDatabase& db_;
};

} // namespace contracts
} // namespace intcoin

#endif // INTCOIN_CONTRACTS_VALIDATOR_H
