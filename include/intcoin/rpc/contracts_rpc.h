// Copyright (c) 2026 The INTcoin Core developers
// Distributed under the MIT software license

#ifndef INTCOIN_RPC_CONTRACTS_RPC_H
#define INTCOIN_RPC_CONTRACTS_RPC_H

#include <string>
#include <vector>
#include <memory>
#include <optional>

namespace intcoin {
namespace rpc {

/**
 * Smart Contract RPC Endpoints
 *
 * Provides JSON-RPC methods for smart contract deployment, execution,
 * and querying on the INTcoin blockchain.
 */
namespace contracts {

/**
 * Deploy a smart contract
 *
 * RPC Method: deploycontract
 *
 * Parameters:
 * - bytecode (string): Contract bytecode (hex-encoded)
 * - constructor_args (array, optional): Constructor arguments (hex-encoded)
 * - gas_limit (number): Maximum gas to use
 * - value (number, optional): Initial contract balance in satINT
 *
 * Returns:
 * {
 *   "txid": "transaction_id",
 *   "contract_address": "int1q...",
 *   "gas_estimated": 123456
 * }
 *
 * Example:
 * intcoin-cli deploycontract "60806040..." [] 1000000 0
 */
std::string DeployContract(const std::vector<std::string>& params);

/**
 * Call a smart contract function
 *
 * RPC Method: callcontract
 *
 * Parameters:
 * - contract_address (string): Contract address (Bech32)
 * - function_signature (string): Function signature (e.g., "transfer(address,uint256)")
 * - args (array): Function arguments (hex-encoded)
 * - gas_limit (number): Maximum gas to use
 * - value (number, optional): Amount to send in satINT
 *
 * Returns:
 * {
 *   "txid": "transaction_id",
 *   "gas_estimated": 50000
 * }
 *
 * Example:
 * intcoin-cli callcontract "int1q..." "transfer(address,uint256)" ["0x123...","0x03e8"] 100000 0
 */
std::string CallContract(const std::vector<std::string>& params);

/**
 * Perform a static call (read-only, no transaction)
 *
 * RPC Method: callcontractstatic
 *
 * Parameters:
 * - contract_address (string): Contract address
 * - function_signature (string): Function signature
 * - args (array): Function arguments
 *
 * Returns:
 * {
 *   "result": "0x...",
 *   "decoded": [...],
 *   "gas_used": 21000
 * }
 *
 * Example:
 * intcoin-cli callcontractstatic "int1q..." "balanceOf(address)" ["0x123..."]
 */
std::string CallContractStatic(const std::vector<std::string>& params);

/**
 * Get contract information
 *
 * RPC Method: getcontractinfo
 *
 * Parameters:
 * - contract_address (string): Contract address
 *
 * Returns:
 * {
 *   "address": "int1q...",
 *   "balance": 1000000,
 *   "code_hash": "0x...",
 *   "storage_root": "0x...",
 *   "nonce": 0,
 *   "creator": "int1q...",
 *   "creation_tx": "0x...",
 *   "block_created": 12345
 * }
 *
 * Example:
 * intcoin-cli getcontractinfo "int1q..."
 */
std::string GetContractInfo(const std::vector<std::string>& params);

/**
 * Get contract bytecode
 *
 * RPC Method: getcontractcode
 *
 * Parameters:
 * - contract_address (string): Contract address
 *
 * Returns:
 * {
 *   "code": "0x60806040...",
 *   "size": 1234,
 *   "disassembly": "PUSH1 0x80..."
 * }
 *
 * Example:
 * intcoin-cli getcontractcode "int1q..."
 */
std::string GetContractCode(const std::vector<std::string>& params);

/**
 * Get transaction receipt
 *
 * RPC Method: getcontractreceipt
 *
 * Parameters:
 * - txid (string): Transaction ID
 *
 * Returns:
 * {
 *   "txid": "0x...",
 *   "contract_address": "int1q...",
 *   "from": "int1q...",
 *   "to": "int1q...",
 *   "gas_used": 50000,
 *   "gas_price": 1,
 *   "total_fee": 50000,
 *   "status": "SUCCESS",
 *   "return_data": "0x...",
 *   "logs": [...],
 *   "block_number": 12345,
 *   "block_timestamp": 1234567890,
 *   "tx_index": 5
 * }
 *
 * Example:
 * intcoin-cli getcontractreceipt "0x..."
 */
std::string GetContractReceipt(const std::vector<std::string>& params);

/**
 * Estimate gas for contract call
 *
 * RPC Method: estimatecontractgas
 *
 * Parameters:
 * - contract_address (string): Contract address
 * - function_signature (string): Function signature
 * - args (array): Function arguments
 * - value (number, optional): Amount to send
 *
 * Returns:
 * {
 *   "gas_estimate": 50000,
 *   "gas_price": 1,
 *   "estimated_fee": 50000
 * }
 *
 * Example:
 * intcoin-cli estimatecontractgas "int1q..." "transfer(address,uint256)" ["0x123...","0x03e8"] 0
 */
std::string EstimateContractGas(const std::vector<std::string>& params);

/**
 * Get contract event logs
 *
 * RPC Method: getcontractlogs
 *
 * Parameters:
 * - contract_address (string, optional): Filter by contract address
 * - from_block (number, optional): Starting block number
 * - to_block (number, optional): Ending block number
 * - topics (array, optional): Event topics to filter
 *
 * Returns:
 * {
 *   "logs": [
 *     {
 *       "address": "int1q...",
 *       "topics": ["0x...", "0x..."],
 *       "data": "0x...",
 *       "block_number": 12345,
 *       "transaction_hash": "0x...",
 *       "log_index": 0
 *     }
 *   ]
 * }
 *
 * Example:
 * intcoin-cli getcontractlogs "int1q..." 10000 20000 []
 */
std::string GetContractLogs(const std::vector<std::string>& params);

/**
 * List deployed contracts
 *
 * RPC Method: listcontracts
 *
 * Parameters:
 * - limit (number, optional): Maximum number to return (default: 100)
 * - offset (number, optional): Pagination offset (default: 0)
 *
 * Returns:
 * {
 *   "contracts": [
 *     {
 *       "address": "int1q...",
 *       "creator": "int1q...",
 *       "creation_block": 12345,
 *       "creation_tx": "0x...",
 *       "balance": 1000000,
 *       "code_size": 1234
 *     }
 *   ],
 *   "total": 150,
 *   "offset": 0,
 *   "limit": 100
 * }
 *
 * Example:
 * intcoin-cli listcontracts 10 0
 */
std::string ListContracts(const std::vector<std::string>& params);

/**
 * Get contract storage value
 *
 * RPC Method: getcontractstorage
 *
 * Parameters:
 * - contract_address (string): Contract address
 * - storage_key (string): Storage key (hex-encoded 32 bytes)
 *
 * Returns:
 * {
 *   "key": "0x...",
 *   "value": "0x..."
 * }
 *
 * Example:
 * intcoin-cli getcontractstorage "int1q..." "0x0000..."
 */
std::string GetContractStorage(const std::vector<std::string>& params);

/**
 * Compile Solidity source code
 *
 * RPC Method: compilesolidity
 *
 * Parameters:
 * - source_code (string): Solidity source code
 * - compiler_version (string, optional): Compiler version (default: latest)
 *
 * Returns:
 * {
 *   "bytecode": "0x60806040...",
 *   "abi": [...],
 *   "warnings": [],
 *   "errors": []
 * }
 *
 * Example:
 * intcoin-cli compilesolidity "pragma solidity ^0.8.0; contract MyContract { ... }" "0.8.20"
 */
std::string CompileSolidity(const std::vector<std::string>& params);

/**
 * Verify contract source code
 *
 * RPC Method: verifycontract
 *
 * Parameters:
 * - contract_address (string): Contract address
 * - source_code (string): Solidity source code
 * - constructor_args (string): Constructor arguments (hex)
 * - compiler_version (string): Compiler version used
 *
 * Returns:
 * {
 *   "verified": true,
 *   "source_hash": "0x...",
 *   "bytecode_match": true
 * }
 *
 * Example:
 * intcoin-cli verifycontract "int1q..." "pragma solidity..." "0x" "0.8.20"
 */
std::string VerifyContract(const std::vector<std::string>& params);

} // namespace contracts
} // namespace rpc
} // namespace intcoin

#endif // INTCOIN_RPC_CONTRACTS_RPC_H
