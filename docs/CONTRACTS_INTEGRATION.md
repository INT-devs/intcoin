# Smart Contract Integration Plan

**Version**: 1.4.0-dev
**Status**: Integration Phase
**Last Updated**: January 9, 2026

---

## Completed Components ✅

### 1. Core VM Implementation
- **IntSC Virtual Machine**: EVM-compatible stack-based VM
- **60+ Opcodes**: Full EVM opcode set (arithmetic, logic, memory, storage, control flow)
- **4 PQC Opcodes**: DILITHIUM_VERIFY, KYBER_ENCAP, KYBER_DECAP, PQC_PUBKEY
- **Gas Metering**: Per-opcode gas costs with PQC adjustments
- **Execution Modes**: NORMAL, STATIC (view functions), DELEGATE (delegatecall)

### 2. Transaction Types
- **ContractDeploymentTx** (Type 2):
  - Bytecode deployment
  - Constructor argument encoding
  - Deterministic address generation (SHA3-256(from || nonce))
  - Dilithium3 signature support
- **ContractCallTx** (Type 3):
  - Function call encoding (ABI)
  - Value transfers
  - Gas limit specification

### 3. RPC API
- **12 RPC Methods**: deploycontract, callcontract, getcontractinfo, etc.
- **JSON-RPC 2.0 Compatible**: Standard error handling
- **Integrated with Main Server**: ContractsRPC class registered

### 4. Database Layer
- **ContractDatabase**: RocksDB-backed persistent storage
- **Contract Accounts**: Bytecode, balance, nonce, metadata
- **Contract Storage**: Key-value pairs per contract
- **Transaction Receipts**: Execution results with gas tracking
- **Event Logs**: Indexed by block number with filtering support

---

## Remaining Tasks

### Task 5: Block Validation for Contract Transactions

**Goal**: Integrate contract transaction validation into block processing

#### 5.1 Transaction Validation
Location: `src/blockchain/validation.cpp`

**Requirements**:
```cpp
// Add to ValidateTransaction()
bool ValidateTransaction(const Transaction& tx, const ChainState& state) {
    // Existing validation...

    // Contract transaction validation
    if (tx.type == TX_CONTRACT_DEPLOYMENT || tx.type == TX_CONTRACT_CALL) {
        return ValidateContractTransaction(tx, state);
    }
}

bool ValidateContractTransaction(const Transaction& tx, const ChainState& state) {
    // 1. Verify transaction structure
    if (tx.type == TX_CONTRACT_DEPLOYMENT) {
        ContractDeploymentTx deploy_tx = DeserializeDeploymentTx(tx.data);

        // Check bytecode validity
        if (deploy_tx.bytecode.empty()) return false;

        // Verify signature
        if (!deploy_tx.Verify()) return false;

        // Check nonce
        uint64_t expected_nonce = state.GetAccountNonce(deploy_tx.from);
        if (deploy_tx.nonce != expected_nonce) return false;

        // Check gas limit is sufficient
        uint64_t min_gas = ContractDeployer::EstimateGas(deploy_tx.bytecode, {});
        if (deploy_tx.gas_limit < min_gas) return false;

        // Check sender has sufficient balance for value + gas
        uint64_t required = deploy_tx.value + (deploy_tx.gas_limit * deploy_tx.gas_price);
        if (state.GetBalance(deploy_tx.from) < required) return false;
    }

    if (tx.type == TX_CONTRACT_CALL) {
        ContractCallTx call_tx = DeserializeCallTx(tx.data);

        // Verify signature
        if (!call_tx.Verify()) return false;

        // Check contract exists
        if (!state.ContractExists(call_tx.to)) return false;

        // Check nonce
        uint64_t expected_nonce = state.GetAccountNonce(call_tx.from);
        if (call_tx.nonce != expected_nonce) return false;

        // Check sender has sufficient balance
        uint64_t required = call_tx.value + (call_tx.gas_limit * call_tx.gas_price);
        if (state.GetBalance(call_tx.from) < required) return false;
    }

    return true;
}
```

#### 5.2 Contract Execution in Block Processing
Location: `src/blockchain/block.cpp`

**Requirements**:
```cpp
bool ProcessBlock(const Block& block, ChainState& state) {
    // Begin database batch for atomic updates
    state.contract_db.BeginBatch();

    for (const Transaction& tx : block.transactions) {
        if (tx.type == TX_CONTRACT_DEPLOYMENT) {
            ExecuteContractDeployment(tx, block, state);
        } else if (tx.type == TX_CONTRACT_CALL) {
            ExecuteContractCall(tx, block, state);
        } else {
            // Handle regular transactions
        }
    }

    // Commit all contract state changes
    state.contract_db.CommitBatch();

    return true;
}

void ExecuteContractDeployment(const Transaction& tx, const Block& block, ChainState& state) {
    ContractDeploymentTx deploy_tx = DeserializeDeploymentTx(tx.data);

    // Create execution context
    ExecutionContext ctx;
    ctx.caller = deploy_tx.from;
    ctx.origin = deploy_tx.from;
    ctx.value = deploy_tx.value;
    ctx.gas_limit = deploy_tx.gas_limit;
    ctx.gas_price = deploy_tx.gas_price;
    ctx.block_number = block.height;
    ctx.block_timestamp = block.timestamp;
    ctx.storage = new DatabaseStorage(state.contract_db, deploy_tx.GetContractAddress());

    // Execute constructor
    IntSCVM vm;
    ExecutionResult result = vm.Execute(deploy_tx.bytecode, ctx);

    // Create contract account
    ContractAccount account;
    account.address = deploy_tx.GetContractAddress();
    account.balance = deploy_tx.value;
    account.nonce = 0;
    account.bytecode = deploy_tx.bytecode;
    account.code_hash = SHA3::Hash(deploy_tx.bytecode);
    account.creator = AddressFromPublicKey(deploy_tx.from);
    account.creation_tx = tx.GetTxID();
    account.block_created = block.height;
    account.block_updated = block.height;

    state.contract_db.PutContractAccount(account);

    // Create receipt
    TransactionReceipt receipt;
    receipt.txid = tx.GetTxID();
    receipt.contract_address = account.address;
    receipt.from = AddressFromPublicKey(deploy_tx.from);
    receipt.gas_used = ctx.gas_limit - vm.GetGasRemaining();
    receipt.gas_price = deploy_tx.gas_price;
    receipt.total_fee = receipt.gas_used * receipt.gas_price;
    receipt.status = result;
    receipt.block_number = block.height;
    receipt.block_timestamp = block.timestamp;
    receipt.tx_index = GetTxIndex(tx, block);

    state.contract_db.PutReceipt(receipt);

    // Store event logs
    for (const auto& log : vm.GetLogs()) {
        EventLogEntry entry;
        entry.contract_address = account.address;
        entry.topics = log.topics;
        entry.data = log.data;
        entry.block_number = block.height;
        entry.transaction_hash = tx.GetTxID();
        entry.log_index = log.index;

        state.contract_db.PutEventLog(entry);
    }

    // Deduct gas fees
    state.DeductFee(deploy_tx.from, receipt.total_fee);
}

void ExecuteContractCall(const Transaction& tx, const Block& block, ChainState& state) {
    ContractCallTx call_tx = DeserializeCallTx(tx.data);

    // Get contract account
    auto account_result = state.contract_db.GetContractAccount(call_tx.to);
    if (!account_result.IsOk()) return; // Contract doesn't exist

    ContractAccount account = account_result.GetValue();

    // Create execution context
    ExecutionContext ctx;
    ctx.caller = call_tx.from;
    ctx.origin = call_tx.from;
    ctx.contract_address = call_tx.to;
    ctx.value = call_tx.value;
    ctx.gas_limit = call_tx.gas_limit;
    ctx.gas_price = call_tx.gas_price;
    ctx.block_number = block.height;
    ctx.block_timestamp = block.timestamp;
    ctx.call_data = call_tx.data;
    ctx.storage = new DatabaseStorage(state.contract_db, call_tx.to);

    // Execute contract
    IntSCVM vm;
    ExecutionResult result = vm.Execute(account.bytecode, ctx);

    // Update account
    account.balance += call_tx.value;
    account.block_updated = block.height;
    state.contract_db.PutContractAccount(account);

    // Create receipt
    TransactionReceipt receipt;
    receipt.txid = tx.GetTxID();
    receipt.from = AddressFromPublicKey(call_tx.from);
    receipt.to = call_tx.to;
    receipt.gas_used = ctx.gas_limit - vm.GetGasRemaining();
    receipt.gas_price = call_tx.gas_price;
    receipt.total_fee = receipt.gas_used * receipt.gas_price;
    receipt.status = result;
    receipt.return_data = vm.GetReturnData();
    receipt.block_number = block.height;
    receipt.block_timestamp = block.timestamp;
    receipt.tx_index = GetTxIndex(tx, block);

    state.contract_db.PutReceipt(receipt);

    // Store event logs
    for (const auto& log : vm.GetLogs()) {
        EventLogEntry entry;
        entry.contract_address = call_tx.to;
        entry.topics = log.topics;
        entry.data = log.data;
        entry.block_number = block.height;
        entry.transaction_hash = tx.GetTxID();
        entry.log_index = log.index;

        state.contract_db.PutEventLog(entry);
    }

    // Deduct gas fees
    state.DeductFee(call_tx.from, receipt.total_fee);
}
```

#### 5.3 State Persistence
Location: `src/blockchain/chainstate.cpp`

**Requirements**:
- Add `ContractDatabase contract_db` to ChainState
- Open contract database on node startup
- Close on shutdown
- Integrate with existing UTXO database

---

### Task 6: Mempool Integration for Contract Transactions

**Goal**: Add contract transaction handling to the mempool

#### 6.1 Transaction Admission
Location: `src/blockchain/mempool.cpp`

**Requirements**:
```cpp
bool Mempool::AcceptTransaction(const Transaction& tx) {
    // Existing validation...

    if (tx.type == TX_CONTRACT_DEPLOYMENT || tx.type == TX_CONTRACT_CALL) {
        return AcceptContractTransaction(tx);
    }
}

bool Mempool::AcceptContractTransaction(const Transaction& tx) {
    // 1. Validate transaction structure
    if (!ValidateContractTransaction(tx, chain_state_)) {
        return false;
    }

    // 2. Check for nonce conflicts
    std::string from_address;
    uint64_t nonce;

    if (tx.type == TX_CONTRACT_DEPLOYMENT) {
        ContractDeploymentTx deploy_tx = DeserializeDeploymentTx(tx.data);
        from_address = AddressFromPublicKey(deploy_tx.from);
        nonce = deploy_tx.nonce;
    } else {
        ContractCallTx call_tx = DeserializeCallTx(tx.data);
        from_address = AddressFromPublicKey(call_tx.from);
        nonce = call_tx.nonce;
    }

    // Check if nonce is already in mempool
    for (const auto& pending_tx : transactions_) {
        if (GetSenderAddress(pending_tx) == from_address &&
            GetNonce(pending_tx) == nonce) {
            // Nonce conflict - check gas price
            uint64_t pending_gas_price = GetGasPrice(pending_tx);
            uint64_t new_gas_price = GetGasPrice(tx);

            if (new_gas_price > pending_gas_price * 1.1) {
                // Replace with higher gas price
                RemoveTransaction(pending_tx);
                break;
            } else {
                return false; // Reject lower gas price
            }
        }
    }

    // 3. Add to priority queue (by gas price)
    AddToQueue(tx);

    return true;
}
```

#### 6.2 Priority Ordering
**Requirements**:
- Contract transactions compete with regular transactions for block space
- Sort by gas price (higher = higher priority)
- Respect nonce ordering for same sender
- Enforce gas limit per block (sum of all tx gas_limit <= block_gas_limit)

#### 6.3 Transaction Replacement (RBF)
**Requirements**:
- Allow replacing pending tx with same nonce if gas price is 10% higher
- Broadcast replacement to network
- Update mempool statistics

---

## Implementation Checklist

### Block Validation (Task 5)
- [ ] Add contract transaction types to transaction enum
- [ ] Implement `ValidateContractTransaction()`
- [ ] Add contract execution to `ProcessBlock()`
- [ ] Implement `ExecuteContractDeployment()`
- [ ] Implement `ExecuteContractCall()`
- [ ] Add `ContractDatabase` to `ChainState`
- [ ] Update state persistence for contracts
- [ ] Add unit tests for contract validation
- [ ] Add integration tests for block processing with contracts

### Mempool Integration (Task 6)
- [ ] Implement `AcceptContractTransaction()`
- [ ] Add nonce tracking per address in mempool
- [ ] Implement gas price-based priority queue
- [ ] Add transaction replacement logic (RBF)
- [ ] Enforce block gas limit
- [ ] Update mempool RPC methods to include contract txs
- [ ] Add contract transaction metrics
- [ ] Add unit tests for mempool contract handling
- [ ] Add integration tests for contract tx propagation

### Testing
- [ ] Deploy SimpleStorage contract via RPC
- [ ] Call contract functions and verify receipts
- [ ] Test event log emission and querying
- [ ] Test mempool ordering with mixed tx types
- [ ] Test nonce conflict resolution
- [ ] Test gas limit enforcement
- [ ] Test contract reorgs and state rollback

---

## Integration Points

### Files to Modify

1. **Transaction Types**
   - `include/intcoin/blockchain.h`: Add TX_CONTRACT_DEPLOYMENT, TX_CONTRACT_CALL
   - `include/intcoin/types.h`: Add contract transaction enums

2. **Block Validation**
   - `src/blockchain/validation.cpp`: Add contract validation
   - `src/blockchain/block.cpp`: Add contract execution
   - `include/intcoin/blockchain.h`: Update Block processing interface

3. **Chain State**
   - `src/blockchain/chainstate.cpp`: Add ContractDatabase
   - `include/intcoin/blockchain.h`: Add contract state methods

4. **Mempool**
   - `src/blockchain/mempool.cpp`: Add contract tx handling
   - `include/intcoin/blockchain.h`: Update Mempool interface

5. **Network**
   - `src/network/p2p.cpp`: Add contract tx relay
   - `include/intcoin/network.h`: Update message types

---

## Performance Considerations

1. **Gas Limit Per Block**: 30,000,000 gas (target)
2. **Block Gas Target**: 15,000,000 gas (50% of limit)
3. **Min Gas Price**: 1 satINT/gas
4. **Contract Size Limit**: 24 KB (EVM compatible)
5. **Storage Slot Gas**: 20,000 gas (new), 5,000 gas (update)

---

## Security Considerations

1. **Reentrancy Protection**: Check-effects-interactions pattern enforced
2. **Gas Exhaustion**: Metered execution prevents infinite loops
3. **Integer Overflow**: Solidity 0.8+ has built-in protection
4. **Signature Verification**: Dilithium3 quantum-resistant signatures
5. **DoS Prevention**: Gas limits prevent resource exhaustion

---

## Testing Strategy

### Unit Tests
- Transaction validation for each contract type
- Gas calculation accuracy
- Nonce increment logic
- Storage persistence and retrieval

### Integration Tests
- Full contract deployment and execution flow
- Mempool ordering and replacement
- Block validation with mixed transaction types
- State rollback on chain reorg

### Performance Tests
- 1000 contract deployments per block
- 10,000 contract calls per block
- Event log query performance
- Database size growth under load

---

## Deployment Timeline

### Phase 1: Block Validation (Week 1)
- Implement transaction validation
- Add execution to block processing
- Integrate ContractDatabase with ChainState

### Phase 2: Mempool Integration (Week 2)
- Add contract tx acceptance logic
- Implement priority queue
- Add transaction replacement

### Phase 3: Testing (Week 3)
- Write comprehensive unit tests
- Run integration tests
- Performance testing and optimization

### Phase 4: Release (Week 4)
- Final QA testing
- Documentation updates
- v1.4.0 release

---

**Last Updated**: January 9, 2026
**Author**: INTcoin Development Team
**Status**: In Progress - Phase 1 Complete, Phase 2-4 Pending
