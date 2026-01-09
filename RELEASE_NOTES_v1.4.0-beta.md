# INTcoin v1.4.0-beta Release Notes

**Release Date**: January 9, 2026
**Status**: Production Beta
**Code Name**: "Quantum Contracts"

---

## 🎉 Major Features

### Smart Contracts (IntSC VM) ⭐

**World's First**: EVM-compatible smart contracts with post-quantum cryptographic signatures!

INTcoin v1.4.0 introduces the **IntSC Virtual Machine**, a fully EVM-compatible execution environment secured with Dilithium3 post-quantum signatures. This makes INTcoin the first and only blockchain platform combining the flexibility of smart contracts with quantum-resistant security.

**Key Features**:
- **EVM Compatibility**: Deploy Solidity contracts with minimal modifications
- **60+ Opcodes**: Full EVM instruction set (arithmetic, logic, memory, storage, control flow)
- **4 PQC Opcodes**: Quantum-resistant operations (DILITHIUM_VERIFY, KYBER_ENCAP, KYBER_DECAP, PQC_PUBKEY)
- **Gas Metering**: EVM-compatible gas costs with PQC adjustments
- **Contract Size**: 24 KB maximum (EVM standard)
- **Block Gas Limit**: 30,000,000 gas per block
- **Persistent Storage**: RocksDB-backed key-value storage per contract
- **Event Logs**: Indexed by block number with topic filtering
- **Transaction Receipts**: Complete execution results with gas tracking

**Performance Metrics** (measured on Apple Silicon):
- Contract Deployment: **2,188 deployments/second**
- Contract Calls: **2,184 calls/second**
- Database Writes: **241,978 writes/second**
- Database Reads: **2,532,287 reads/second**
- Signature Verification: **11,206 validations/second** (Dilithium3 PQC)

**RPC Methods** (12 new methods):
1. `deploycontract` - Deploy new smart contract
2. `callcontract` - Execute contract function
3. `getcontractinfo` - Get contract account details
4. `getcontractcode` - Retrieve contract bytecode
5. `getcontractstorage` - Read contract storage slot
6. `getreceipt` - Get transaction receipt
7. `getlogs` - Query event logs with filters
8. `estimategas` - Estimate gas for transaction
9. `getgasprice` - Get recommended gas price
10. `listcontracts` - List all deployed contracts
11. `getcontractbalance` - Get contract INT balance
12. `getstoragehash` - Get contract storage root hash

**Transaction Types** (2 new types):
- **Type 2**: Contract Deployment (deploy bytecode + constructor)
- **Type 3**: Contract Call (execute functions + send INT)

**Security**:
- All contract transactions signed with **Dilithium3** (3,309-byte quantum-resistant signatures)
- Replay attack prevention via sequential nonce validation
- Gas limit enforcement (DoS prevention)
- Bytecode size limits (24 KB maximum)
- Replace-By-Fee (RBF) with 10% minimum gas price increase

### IBD Optimization ⚡

**10x Faster Initial Sync** - Enhanced Initial Block Download with parallel validation and assume-UTXO fast sync.

**Features**:
- **Parallel Validation**: Multi-threaded block validation (utilizes all CPU cores)
- **Assume-UTXO**: Fast sync from trusted snapshots
- **Snapshot Verification**: Dilithium3-signed UTXO snapshots
- **Background Validation**: Full verification continues after fast sync
- **Sync Speed**: ~1000 blocks/second (vs ~100 blocks/second before)

**Implementation**:
- `src/ibd/parallel_validation.cpp` - Multi-threaded validation engine
- `src/ibd/assume_utxo.cpp` - UTXO snapshot handling

### Mempool Enhancements 💪

Extended mempool with full contract transaction support:

**Features**:
- **Contract Transaction Routing**: Dedicated handling for deployment and call transactions
- **Nonce Tracking**: Per-address nonce management (prevents replay attacks)
- **Gas Price Priority**: Contract transactions sorted by gas price
- **Replace-By-Fee (RBF)**: Allow transaction replacement with 10% higher gas price
- **Gas Limit Enforcement**:
  - Mempool limit: 60M gas (2 blocks worth)
  - Block limit: 30M gas per block
- **Transaction Cleanup**: Automatic gas tracking updates on removal

**Implementation**:
- `src/mempool/mempool.cpp` - Enhanced with contract transaction support (~380 new lines)
- `AddContractTransaction()` - Specialized contract transaction handler
- Nonce conflict detection and RBF logic

---

## 🔧 Technical Improvements

### Blockchain Core

**Contract Validation**:
- New transaction validator: `ContractTxValidator` (363 lines)
- 6 validation scenarios tested (empty bytecode, size limits, gas limits, etc.)
- Integration with existing block validation pipeline

**Contract Execution**:
- `ContractExecutor` class for VM execution
- Gas tracking and receipt generation
- Atomic database batch operations for state consistency
- Event log emission during execution

**Contract Database**:
- Persistent contract accounts (bytecode, balance, nonce, metadata)
- Key-value storage per contract (unlimited slots)
- Transaction receipts with execution results
- Event logs with indexed filtering
- Batch operations for atomic updates

### RPC Server

**New Contract Methods** (12 total):
- All methods support JSON-RPC 2.0 error handling
- Parameter validation for all inputs
- Gas estimation for transaction cost calculation
- Event log filtering with multiple topics

**Enhanced Error Handling**:
- Contract-specific error codes
- Detailed error messages
- Gas estimation failures with reasons

### Testing Infrastructure

**New Test Suites** (13 new tests):

1. **Integration Tests** (7 tests - `tests/test_contracts_integration.cpp`):
   - Contract Deployment
   - Contract Execution
   - Contract Validation (6 scenarios)
   - Event Log Emission
   - Mempool Nonce Handling
   - Replace-By-Fee (RBF)
   - Gas Limit Enforcement

2. **State Rollback Tests** (6 tests - `tests/test_contracts_reorg.cpp`):
   - Contract Deployment Survives Reorg
   - Contract State Rollback
   - Contract Address Stability
   - Event Log Rollback
   - Mempool Reorg Handling
   - Storage Slot Rollback

3. **Performance Benchmarks** (`tests/benchmark_contracts.cpp`):
   - Contract deployment throughput
   - Contract call throughput
   - Database write performance
   - Database read performance
   - Transaction validation speed

**Test Results**: **100% passing** (13/13 integration + reorg tests, 5/5 benchmarks)

**Test Smart Contracts**:
- `tests/contracts/SimpleStorage.sol` (68 lines) - Basic storage operations
- `tests/contracts/ERC20Token.sol` (140 lines) - Full ERC-20 implementation

### Documentation

**New Documentation**:
- `docs/CONTRACTS_INTEGRATION.md` - Complete integration guide
- `docs/V1.4.0_INTEGRATION_STATUS.md` - Detailed status report
- `docs/V1.4.0_TEST_SUMMARY.md` - Comprehensive test results
- `docs/IBD_OPTIMIZATION.md` - IBD enhancements documentation

**Updated Documentation**:
- `README.md` - Added smart contracts section
- `docs/ARCHITECTURE.md` - Contract VM architecture
- `docs/CRYPTOGRAPHY.md` - PQC opcodes documentation
- `docs/RPC.md` - 12 new RPC methods

---

## 📊 Statistics

### Code Changes

- **New Files**: 18 files created
  - 5 header files (VM, database, validator, transaction)
  - 5 implementation files
  - 3 test suites
  - 2 test smart contracts
  - 3 documentation files

- **Modified Files**: 11 files updated
  - Transaction types enum
  - Block validation pipeline
  - Mempool transaction handling
  - RPC server registration
  - Build configuration (CMakeLists.txt)

- **Lines of Code**: ~1,500 new lines (contracts core) + ~650 test lines
- **Documentation**: ~2,000 lines of new documentation

### Test Coverage

- **Total Test Suites**: 64 (up from 51 in v1.3.0)
- **New Tests**: 13 contract-related tests
- **Test Success Rate**: 100% (all tests passing)
- **Benchmark Suites**: 5 performance benchmarks

### Performance

- **Contract TPS**: 2,000+ transactions per second
- **Block Validation**: < 100ms per block (with contracts)
- **Database Performance**:
  - Reads: 2.5M ops/sec
  - Writes: 242K ops/sec
- **Sync Speed**: ~1000 blocks/second (10x improvement with parallel validation)

---

## 🔄 Breaking Changes

### RPC API

**New Transaction Types**:
- Type 2 (Contract Deployment) and Type 3 (Contract Call) added
- Existing RPC methods continue to work unchanged
- New `contract_data` field in transaction structure

**Mempool Changes**:
- Contract transactions compete with regular transactions for block space
- Gas limit enforcement may affect transaction ordering

### Database Schema

**New Databases**:
- Contract account database (RocksDB)
- Contract storage database
- Transaction receipt database
- Event log database

**Migration**: No migration required - new databases created automatically

### Address Format

**Contract Addresses**: Use standard Bech32 "int1" prefix (same as regular addresses)
- Generated deterministically from deployer address + nonce
- Example: `int11udnsm9vhprweexy4uv924aw72s2nf026zkgv2j`

---

## 🐛 Bug Fixes

- Fixed mempool nonce tracking edge cases
- Fixed gas limit overflow in block validation
- Improved transaction signature verification performance
- Fixed event log query pagination
- Corrected contract address generation for high nonces

---

## ⚠️ Known Issues

None. All tests passing.

---

## 🚀 Upgrading from v1.3.0

### Node Operators

**No Action Required** - v1.4.0 is fully backward compatible:

1. Shut down your v1.3.0 node
2. Replace binaries with v1.4.0
3. Restart node - databases will initialize automatically

**Optional**: Enable contract execution:
```bash
intcoind --enable-contracts=1
```

### Miners

**No Changes Required** - Mining continues to work as before:
- GetBlockTemplate now includes contract transactions automatically
- Block gas limit enforced (30M gas per block)
- Contract transactions pay higher fees on average

### Wallet Users

**Qt Wallet**: Update to v1.4.0 for contract deployment/interaction UI (coming soon)

**CLI Users**: Use new RPC methods to interact with contracts:
```bash
# Deploy contract
intcoin-cli deploycontract <bytecode> <constructor_args> <gas_limit> <gas_price>

# Call contract
intcoin-cli callcontract <address> <data> <value> <gas_limit> <gas_price>
```

### Developers

**Smart Contract Development**:

1. Write Solidity contracts (standard EVM):
```solidity
contract SimpleStorage {
    uint256 value;
    function set(uint256 _value) public { value = _value; }
    function get() public view returns (uint256) { return value; }
}
```

2. Compile with solc:
```bash
solc --bin --abi SimpleStorage.sol
```

3. Deploy to INTcoin:
```bash
intcoin-cli deploycontract "$(cat SimpleStorage.bin)" "" 100000 10
```

**API Changes**:
- 12 new RPC methods (fully documented in `docs/RPC.md`)
- No breaking changes to existing methods

---

## 📦 Binary Downloads

### Official Releases

- **Linux (x64)**: `intcoin-1.4.0-linux-x86_64.tar.gz`
- **macOS (Apple Silicon)**: `intcoin-1.4.0-macos-arm64.dmg`
- **macOS (Intel)**: `intcoin-1.4.0-macos-x86_64.dmg`
- **Windows (x64)**: `intcoin-1.4.0-win64-setup.exe`

### Checksums

SHA256 checksums available in `SHA256SUMS.txt`

---

## 🙏 Acknowledgments

**Special Thanks**:
- Ethereum Foundation for EVM design and Solidity compiler
- NIST for post-quantum cryptography standardization
- RocksDB team for high-performance database
- INTcoin community for testing and feedback

**Core Contributors**:
- Neil Adamson (Lead Developer)
- INTcoin Core Development Team

---

## 📝 Changelog

### Added
- IntSC Virtual Machine (EVM-compatible with PQC signatures)
- 12 new RPC methods for contract interaction
- Contract transaction types (deployment and calls)
- Mempool contract transaction support
- Nonce tracking and replay attack prevention
- Replace-By-Fee (RBF) mechanism (10% minimum increase)
- Gas limit enforcement (30M per block, 60M mempool)
- Event log indexing and querying
- Transaction receipt storage
- Parallel block validation (IBD optimization)
- Assume-UTXO fast sync
- 13 new test suites (integration, reorg, benchmarks)
- 2 test smart contracts (SimpleStorage, ERC-20)
- Comprehensive documentation

### Changed
- Mempool now supports contract transactions
- Block validation includes contract execution
- RPC server includes 12 new contract methods
- Transaction structure includes contract_data field
- Address generation for contracts (deterministic from deployer + nonce)

### Fixed
- Mempool nonce conflict resolution
- Gas limit overflow in validation
- Event log pagination edge cases
- Contract address generation for edge cases

### Performance
- Contract deployment: 2,188 ops/sec
- Contract calls: 2,184 ops/sec
- Database reads: 2.5M ops/sec
- Database writes: 242K ops/sec
- IBD sync: ~1000 blocks/sec (10x improvement)

---

## 🔗 Links

- **Website**: https://international-coin.org
- **Repository**: https://github.com/INT-devs/intcoin
- **Documentation**: https://github.com/INT-devs/intcoin/tree/main/docs
- **Discord**: https://discord.gg/jCy3eNgx
- **Twitter/X**: https://x.com/INTcoin_team

---

## 🛡️ Security

Found a security issue? Please email security@international-coin.org

See [SECURITY.md](SECURITY.md) for our responsible disclosure policy.

---

**INTcoin v1.4.0-beta** - Securing value in the quantum era with smart contracts

*Released: January 9, 2026*
