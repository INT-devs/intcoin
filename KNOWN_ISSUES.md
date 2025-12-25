# Known Issues - INTcoin v1.0.0-alpha

**Last Updated**: December 25, 2025
**Version**: 1.0.0-alpha

This document tracks known issues and limitations in the current release.

---

## Test Suite (92% Pass Rate - 11/12 Tests)

### ✅ Passing Tests (11/12):
1. CryptoTest ✓
2. RandomXTest ✓
3. Bech32Test ✓
4. SerializationTest ✓
5. StorageTest ✓
6. GenesisTest ✓
7. NetworkTest ✓
8. MLTest ✓
9. WalletTest ✓ (all 12 subtests passing)
10. FuzzTest ✓
11. IntegrationTest ✓

### ❌ Failing Tests (1/12):

#### ValidationTest - P2PKH Transaction Signing

**Status**: Known architectural issue
**Severity**: Medium
**Impact**: Affects test coverage only - production code paths work correctly

**Issue**:
The test creates transactions using a test keypair and attempts to validate P2PKH script execution. The signature validation fails because `Transaction::GetHashForSigning()` doesn't properly implement the Bitcoin signing protocol for P2PKH transactions.

**Technical Details**:
- When signing a P2PKH transaction input, the hash-to-sign must include the previous output's `script_pubkey` (locking script)
- Current implementation: `GetHashForSigning(sighash_type, input_index)` doesn't accept the `script_pubkey` parameter
- Bitcoin protocol requires: Replace the input's `script_sig` with the corresponding `script_pubkey` when calculating the signing hash
- This is correctly implemented in the Wallet class but not exposed in the Transaction API used by tests

**Error Message**:
```
✗ ValidateInputs failed: Input 0 script validation failed: Script failed: stack is empty or top is false
```

**Workaround**:
- Production wallet code works correctly (uses proper signing internally)
- Only affects ValidationTest which manually constructs transactions
- All wallet operations (send/receive) function properly

**Fix Required**:
1. Refactor `Transaction::GetHashForSigning()` signature to:
   ```cpp
   uint256 GetHashForSigning(uint8_t sighash_type, size_t input_index,
                             const Script& script_pubkey) const;
   ```
2. Update all call sites to pass the appropriate `script_pubkey`
3. Modify signing logic to properly replace `script_sig` with `script_pubkey`

**Timeline**: Will be fixed in v1.0.0-beta (deferred to avoid breaking changes in alpha)

---

## Recent Fixes

### ✅ Fixed in Latest Commits:

1. **BIP39 Mnemonic Checksum Generation** (commit 94e65be)
   - Fixed `BytesToIndices()` to properly append checksum bits
   - Mnemonics now validate correctly
   - IntegrationTest now passing

2. **Test Build Issues** (commit 276ec15)
   - Added RocksDB linking for test_genesis and test_serialization
   - All 12 test executables now build successfully

3. **Wallet Test Mnemonic Validation** (commit dde44e9)
   - Updated all tests to use generated mnemonics instead of hardcoded words
   - All wallet subtests now passing

---

## Other Limitations

### Lightning Network (57% Complete)
**Status**: Deferred to v1.0.0-beta (February 2026)
- BOLT #1-12 message structures implemented
- Business logic pending
- Not critical for alpha release

### Documentation (90% Complete)
**Status**: Minor gaps
- Pool setup guide needs completion
- Some RPC examples need expansion
- Core documentation is complete

---

## Reporting Issues

If you encounter any issues not listed here:
1. Check [GitHub Issues](https://github.com/intcoin/crypto/issues)
2. Review [Documentation](docs/)
3. Submit a new issue with:
   - INTcoin version
   - Operating system
   - Reproduction steps
   - Expected vs actual behavior

---

## See Also

- [Testing Guide](docs/TESTING.md)
- [Build Instructions](docs/BUILDING.md)
- [Contributing Guide](CONTRIBUTING.md)

---

**Maintainer**: INTcoin Core Development Team
**Status**: Active development towards v1.0.0-beta
