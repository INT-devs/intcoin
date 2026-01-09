/*
 * Copyright (c) 2025 INTcoin Team (Neil Adamson)
 * Validation Test Suite
 */

#include "intcoin/blockchain.h"
#include "intcoin/consensus.h"
#include "intcoin/crypto.h"
#include "intcoin/util.h"
#include <iostream>
#include <cassert>
#include <ctime>

using namespace intcoin;

// ============================================================================
// Test Helper Functions
// ============================================================================

// Global test keypair for signing test transactions
static DilithiumCrypto::KeyPair g_test_keypair;
static bool g_keypair_initialized = false;

void InitializeTestKeyPair() {
    if (!g_keypair_initialized) {
        auto result = DilithiumCrypto::GenerateKeyPair();
        assert(result.IsOk());
        g_test_keypair = *result.value;
        g_keypair_initialized = true;
    }
}

void CleanupTestDB() {
    [[maybe_unused]] int result = system("rm -rf /tmp/intcoin_test_validation_db");
}

Block CreateValidTestBlock(const uint256& prev_hash, uint64_t height) {
    InitializeTestKeyPair();

    Block block;

    // Create header
    block.header.version = 1;
    block.header.prev_block_hash = prev_hash;
    block.header.timestamp = std::time(nullptr);
    block.header.bits = consensus::MIN_DIFFICULTY_BITS;
    block.header.nonce = 0;

    // Create coinbase transaction
    Transaction coinbase;
    coinbase.version = 1;
    coinbase.locktime = 0;

    // Coinbase input (no previous output)
    TxIn coinbase_input;
    coinbase_input.prev_tx_hash = uint256{}; // All zeros
    coinbase_input.prev_tx_index = 0xFFFFFFFF;

    // Create coinbase script_sig with block height (BIP34 compliance)
    std::vector<uint8_t> script_data;
    // Push block height as a simple data push
    if (height < 0x7F) {
        script_data.push_back(1); // Push 1 byte
        script_data.push_back(static_cast<uint8_t>(height));
    } else {
        script_data.push_back(2); // Push 2 bytes
        script_data.push_back(static_cast<uint8_t>(height & 0xFF));
        script_data.push_back(static_cast<uint8_t>((height >> 8) & 0xFF));
    }
    coinbase_input.script_sig = Script(script_data);
    coinbase_input.sequence = 0xFFFFFFFF;
    coinbase.inputs.push_back(coinbase_input);

    // Coinbase output - use test keypair's public key hash
    std::vector<uint8_t> pubkey_vec(g_test_keypair.public_key.begin(), g_test_keypair.public_key.end());
    uint256 pubkey_hash = SHA3_256(pubkey_vec);
    TxOut coinbase_output(GetBlockReward(height), Script::CreateP2PKH(pubkey_hash));
    coinbase.outputs.push_back(coinbase_output);

    block.transactions.push_back(coinbase);

    // Calculate merkle root
    block.header.merkle_root = block.CalculateMerkleRoot();

    return block;
}

Transaction CreateValidTransaction(const uint256& prev_tx, uint32_t prev_index, uint64_t value) {
    InitializeTestKeyPair();

    Transaction tx;
    tx.version = 1;
    tx.locktime = 0;

    // Input - initially with empty script_sig for signing
    TxIn input;
    input.prev_tx_hash = prev_tx;
    input.prev_tx_index = prev_index;
    input.script_sig = Script(); // Will be filled after signing
    input.sequence = 0xFFFFFFFF;
    tx.inputs.push_back(input);

    // Output - send to same test keypair
    std::vector<uint8_t> pubkey_vec(g_test_keypair.public_key.begin(), g_test_keypair.public_key.end());
    uint256 pubkey_hash = SHA3_256(pubkey_vec);
    TxOut output(value - 1000, Script::CreateP2PKH(pubkey_hash)); // Deduct fee
    tx.outputs.push_back(output);

    // Sign the transaction
    // The previous output has a P2PKH script with the same pubkey hash
    Script prev_scriptpubkey = Script::CreateP2PKH(pubkey_hash);
    uint256 sig_hash = tx.GetHashForSigning(SIGHASH_ALL, 0, prev_scriptpubkey);
    auto sig_result = DilithiumCrypto::SignHash(sig_hash, g_test_keypair.secret_key);
    assert(sig_result.IsOk());

    // Create P2PKH script_sig: <signature> <public key>
    std::vector<uint8_t> script_bytes;

    // Push signature (3309 bytes) using OP_PUSHDATA with 2-byte length
    const Signature& sig = *sig_result.value;
    script_bytes.push_back(0x01);  // OP_PUSHDATA
    uint16_t sig_len = static_cast<uint16_t>(sig.size());
    script_bytes.push_back(sig_len & 0xFF);          // Low byte
    script_bytes.push_back((sig_len >> 8) & 0xFF);  // High byte
    script_bytes.insert(script_bytes.end(), sig.begin(), sig.end());

    // Push public key (1952 bytes) using OP_PUSHDATA with 2-byte length
    script_bytes.push_back(0x01);  // OP_PUSHDATA
    uint16_t pubkey_len = static_cast<uint16_t>(g_test_keypair.public_key.size());
    script_bytes.push_back(pubkey_len & 0xFF);          // Low byte
    script_bytes.push_back((pubkey_len >> 8) & 0xFF);  // High byte
    script_bytes.insert(script_bytes.end(), g_test_keypair.public_key.begin(), g_test_keypair.public_key.end());

    // Update input with signed script_sig
    tx.inputs[0].script_sig = Script(script_bytes);

    return tx;
}

// ============================================================================
// BlockValidator Tests
// ============================================================================

void TestBlockHeaderValidation() {
    std::cout << "\n=== Test 1: Block Header Validation ===\n";

    CleanupTestDB();
    auto db = std::make_shared<BlockchainDB>("/tmp/intcoin_test_validation_db");
    db->Open();

    Blockchain chain(db);
    BlockValidator validator(chain);

    // Test 1: Valid header
    BlockHeader valid_header;
    valid_header.version = 1;
    valid_header.prev_block_hash = uint256{};
    valid_header.merkle_root = uint256{};
    valid_header.timestamp = std::time(nullptr);
    valid_header.bits = consensus::MIN_DIFFICULTY_BITS;
    valid_header.nonce = 0;

    auto result1 = validator.ValidateHeader(valid_header);
    assert(result1.IsOk());
    std::cout << "✓ Valid header accepted\n";

    // Test 2: Invalid version (0)
    BlockHeader invalid_version = valid_header;
    invalid_version.version = 0;

    auto result2 = validator.ValidateHeader(invalid_version);
    assert(result2.IsError());
    std::cout << "✓ Invalid version rejected: " << result2.error << "\n";

    // Test 3: Future timestamp (too far)
    BlockHeader future_header = valid_header;
    future_header.timestamp = std::time(nullptr) + consensus::MAX_FUTURE_BLOCK_TIME + 1000;

    auto result3 = validator.ValidateHeader(future_header);
    assert(result3.IsError());
    std::cout << "✓ Future timestamp rejected: " << result3.error << "\n";

    // Test 4: Invalid bits (0)
    BlockHeader invalid_bits = valid_header;
    invalid_bits.bits = 0;

    auto result4 = validator.ValidateHeader(invalid_bits);
    assert(result4.IsError());
    std::cout << "✓ Invalid bits rejected: " << result4.error << "\n";

    db->Close();
    CleanupTestDB();
}

void TestTransactionStructureValidation() {
    std::cout << "\n=== Test 2: Transaction Structure Validation ===\n";

    CleanupTestDB();
    auto db = std::make_shared<BlockchainDB>("/tmp/intcoin_test_validation_db");
    db->Open();

    Blockchain chain(db);
    TxValidator validator(chain);

    uint256 pubkey_hash{1, 2, 3, 4, 5}; // Mock public key hash

    // Test 1: Valid transaction structure
    Transaction valid_tx;
    valid_tx.version = 1;
    valid_tx.locktime = 0;

    TxIn input;
    input.prev_tx_hash = uint256{1, 2, 3, 4};
    input.prev_tx_index = 0;
    input.script_sig = Script();
    input.sequence = 0xFFFFFFFF;
    valid_tx.inputs.push_back(input);

    TxOut output(50000000, Script::CreateP2PKH(pubkey_hash));
    valid_tx.outputs.push_back(output);

    auto result1 = validator.ValidateStructure(valid_tx);
    assert(result1.IsOk());
    std::cout << "✓ Valid transaction structure accepted\n";

    // Test 2: Invalid version (0)
    Transaction invalid_version = valid_tx;
    invalid_version.version = 0;

    auto result2 = validator.ValidateStructure(invalid_version);
    assert(result2.IsError());
    std::cout << "✓ Invalid version rejected: " << result2.error << "\n";

    // Test 3: Non-coinbase with no inputs
    Transaction no_inputs;
    no_inputs.version = 1;
    no_inputs.locktime = 0;
    no_inputs.outputs.push_back(output);

    auto result3 = validator.ValidateStructure(no_inputs);
    assert(result3.IsError());
    std::cout << "✓ Non-coinbase with no inputs rejected: " << result3.error << "\n";

    // Test 4: Transaction with no outputs
    Transaction no_outputs;
    no_outputs.version = 1;
    no_outputs.locktime = 0;
    no_outputs.inputs.push_back(input);

    auto result4 = validator.ValidateStructure(no_outputs);
    assert(result4.IsError());
    std::cout << "✓ Transaction with no outputs rejected: " << result4.error << "\n";

    // Test 5: Zero value output
    Transaction zero_output = valid_tx;
    zero_output.outputs[0].value = 0;

    auto result5 = validator.ValidateStructure(zero_output);
    assert(result5.IsError());
    std::cout << "✓ Zero value output rejected: " << result5.error << "\n";

    // Test 6: Duplicate inputs
    Transaction duplicate_inputs = valid_tx;
    duplicate_inputs.inputs.push_back(input); // Add same input twice

    auto result6 = validator.ValidateStructure(duplicate_inputs);
    assert(result6.IsError());
    std::cout << "✓ Duplicate inputs rejected: " << result6.error << "\n";

    db->Close();
    CleanupTestDB();
}

void TestCoinbaseValidation() {
    std::cout << "\n=== Test 3: Coinbase Validation ===\n";

    CleanupTestDB();
    auto db = std::make_shared<BlockchainDB>("/tmp/intcoin_test_validation_db");
    db->Open();

    Blockchain chain(db);

    uint256 pubkey_hash{1, 2, 3, 4, 5}; // Mock public key hash

    // Create a valid coinbase transaction
    Transaction coinbase;
    coinbase.version = 1;
    coinbase.locktime = 0;

    TxIn coinbase_input;
    coinbase_input.prev_tx_hash = uint256{}; // All zeros
    coinbase_input.prev_tx_index = 0xFFFFFFFF;
    coinbase_input.script_sig = Script();
    coinbase_input.sequence = 0xFFFFFFFF;
    coinbase.inputs.push_back(coinbase_input);

    TxOut coinbase_output(GetBlockReward(0), Script::CreateP2PKH(pubkey_hash));
    coinbase.outputs.push_back(coinbase_output);

    // Test 1: Verify coinbase is recognized
    assert(coinbase.IsCoinbase());
    std::cout << "✓ Coinbase transaction recognized\n";

    // Test 2: Normal transaction is not coinbase
    Transaction normal_tx;
    normal_tx.version = 1;
    normal_tx.locktime = 0;

    TxIn normal_input;
    normal_input.prev_tx_hash = uint256{1, 2, 3, 4};
    normal_input.prev_tx_index = 0;
    normal_input.script_sig = Script();
    normal_input.sequence = 0xFFFFFFFF;
    normal_tx.inputs.push_back(normal_input);

    normal_tx.outputs.push_back(coinbase_output);

    assert(!normal_tx.IsCoinbase());
    std::cout << "✓ Normal transaction not recognized as coinbase\n";

    db->Close();
    CleanupTestDB();
}

void TestBlockValidation() {
    std::cout << "\n=== Test 4: Complete Block Validation ===\n";

    CleanupTestDB();
    auto db = std::make_shared<BlockchainDB>("/tmp/intcoin_test_validation_db");
    db->Open();

    Blockchain chain(db);
    BlockValidator validator(chain);

    // Test 1: Valid block
    Block valid_block = CreateValidTestBlock(uint256{}, 0);

    auto result1 = validator.ValidateHeader(valid_block.header);
    assert(result1.IsOk());
    std::cout << "✓ Valid block header accepted\n";

    auto result2 = validator.ValidateMerkleRoot(valid_block);
    assert(result2.IsOk());
    std::cout << "✓ Valid merkle root accepted\n";

    auto result3 = validator.ValidateTransactions(valid_block);
    assert(result3.IsOk());
    std::cout << "✓ Valid block transactions accepted\n";

    // Test 2: Block with no transactions
    Block no_tx_block = valid_block;
    no_tx_block.transactions.clear();

    auto result4 = validator.ValidateTransactions(no_tx_block);
    assert(result4.IsError());
    std::cout << "✓ Block with no transactions rejected: " << result4.error << "\n";

    // Test 3: Block where first tx is not coinbase
    Block non_coinbase_first = valid_block;
    Transaction normal_tx;
    normal_tx.version = 1;
    normal_tx.locktime = 0;

    TxIn input;
    input.prev_tx_hash = uint256{1, 2, 3, 4};
    input.prev_tx_index = 0;
    input.script_sig = Script();
    input.sequence = 0xFFFFFFFF;
    normal_tx.inputs.push_back(input);

    uint256 pubkey_hash{1, 2, 3, 4, 5}; // Mock public key hash
    TxOut output(50000000, Script::CreateP2PKH(pubkey_hash));
    normal_tx.outputs.push_back(output);

    non_coinbase_first.transactions[0] = normal_tx;

    auto result5 = validator.ValidateTransactions(non_coinbase_first);
    assert(result5.IsError());
    std::cout << "✓ Block with non-coinbase first tx rejected: " << result5.error << "\n";

    // Test 4: Invalid merkle root
    Block invalid_merkle = valid_block;
    invalid_merkle.header.merkle_root = uint256{9, 9, 9, 9};

    auto result6 = validator.ValidateMerkleRoot(invalid_merkle);
    assert(result6.IsError());
    std::cout << "✓ Invalid merkle root rejected: " << result6.error << "\n";

    db->Close();
    CleanupTestDB();
}

void TestUTXOValidation() {
    std::cout << "\n=== Test 5: UTXO Validation ===\n";

    CleanupTestDB();
    auto db = std::make_shared<BlockchainDB>("/tmp/intcoin_test_validation_db");
    db->Open();

    Blockchain chain(db);
    TxValidator validator(chain);

    // Add genesis block to create some UTXOs
    Block genesis = CreateValidTestBlock(uint256{}, 0);
    auto add_result = chain.AddBlock(genesis);
    if (add_result.IsError()) {
        std::cout << "✗ Failed to add genesis block: " << add_result.error << "\n";
    }
    assert(add_result.IsOk());
    std::cout << "✓ Genesis block added\n";

    // Get the coinbase transaction hash
    uint256 coinbase_hash = genesis.transactions[0].GetHash();

    // Test 1: Transaction spending existing UTXO
    Transaction valid_spend = CreateValidTransaction(coinbase_hash, 0,
        genesis.transactions[0].outputs[0].value);

    auto result1 = validator.ValidateInputs(valid_spend);
    if (!result1.IsOk()) {
        std::cout << "✗ ValidateInputs failed: " << result1.error << "\n";
    }
    assert(result1.IsOk());
    std::cout << "✓ Transaction spending existing UTXO accepted\n";

    // Test 2: Transaction spending non-existent UTXO
    Transaction invalid_spend = CreateValidTransaction(uint256{9, 9, 9, 9}, 0,
        50000000);

    auto result2 = validator.ValidateInputs(invalid_spend);
    assert(result2.IsError());
    std::cout << "✓ Transaction spending non-existent UTXO rejected: " << result2.error << "\n";

    // Test 3: Double spend detection
    auto result3 = validator.CheckDoubleSpend(valid_spend);
    assert(result3.IsOk());
    std::cout << "✓ First spend of UTXO accepted\n";

    // Add the transaction to a block to spend the UTXO
    Block block1 = CreateValidTestBlock(genesis.GetHash(), 1);
    block1.transactions.push_back(valid_spend);
    block1.header.merkle_root = block1.CalculateMerkleRoot();

    auto add_block1 = chain.AddBlock(block1);
    assert(add_block1.IsOk());
    std::cout << "✓ Block with spending transaction added\n";

    // Try to spend the same UTXO again
    auto result4 = validator.CheckDoubleSpend(valid_spend);
    assert(result4.IsError());
    std::cout << "✓ Double spend detected: " << result4.error << "\n";

    db->Close();
    CleanupTestDB();
}

void TestFeeValidation() {
    std::cout << "\n=== Test 6: Transaction Fee Validation ===\n";

    CleanupTestDB();
    auto db = std::make_shared<BlockchainDB>("/tmp/intcoin_test_validation_db");
    db->Open();

    Blockchain chain(db);
    TxValidator validator(chain);

    // Add genesis block to create some UTXOs
    Block genesis = CreateValidTestBlock(uint256{}, 0);
    auto add_result = chain.AddBlock(genesis);
    assert(add_result.IsOk());

    uint256 coinbase_hash = genesis.transactions[0].GetHash();
    uint64_t coinbase_value = genesis.transactions[0].outputs[0].value;

    // Test 1: Transaction with reasonable fee
    uint256 pubkey_hash{1, 2, 3, 4, 5}; // Mock public key hash

    Transaction valid_fee_tx;
    valid_fee_tx.version = 1;
    valid_fee_tx.locktime = 0;

    TxIn input;
    input.prev_tx_hash = coinbase_hash;
    input.prev_tx_index = 0;
    input.script_sig = Script();
    input.sequence = 0xFFFFFFFF;
    valid_fee_tx.inputs.push_back(input);

    // Output is slightly less than input (reasonable fee)
    TxOut output(coinbase_value - 10000, Script::CreateP2PKH(pubkey_hash));
    valid_fee_tx.outputs.push_back(output);

    auto result1 = validator.ValidateFees(valid_fee_tx);
    assert(result1.IsOk());
    std::cout << "✓ Transaction with reasonable fee accepted\n";

    // Test 2: Transaction with output > input
    Transaction invalid_fee_tx = valid_fee_tx;
    invalid_fee_tx.outputs[0].value = coinbase_value + 1000000; // More than input

    auto result2 = validator.ValidateFees(invalid_fee_tx);
    assert(result2.IsError());
    std::cout << "✓ Transaction with output > input rejected: " << result2.error << "\n";

    // Test 3: Transaction with excessive fee (> 50% of input)
    Transaction excessive_fee_tx = valid_fee_tx;
    excessive_fee_tx.outputs[0].value = coinbase_value / 3; // Fee > 50%

    auto result3 = validator.ValidateFees(excessive_fee_tx);
    assert(result3.IsError());
    std::cout << "✓ Transaction with excessive fee rejected: " << result3.error << "\n";

    db->Close();
    CleanupTestDB();
}

void TestCompleteBlockValidation() {
    std::cout << "\n=== Test 7: Complete Block Validation Pipeline ===\n";

    CleanupTestDB();
    auto db = std::make_shared<BlockchainDB>("/tmp/intcoin_test_validation_db");
    db->Open();

    Blockchain chain(db);
    BlockValidator validator(chain);

    // Create a test block
    Block block = CreateValidTestBlock(uint256{}, 0);

    // Test individual validation components (skip PoW for now as it requires actual mining)
    auto header_result = validator.ValidateHeader(block.header);
    assert(header_result.IsOk());
    std::cout << "✓ Block header validation passed\n";

    auto merkle_result = validator.ValidateMerkleRoot(block);
    assert(merkle_result.IsOk());
    std::cout << "✓ Merkle root validation passed\n";

    auto tx_result = validator.ValidateTransactions(block);
    assert(tx_result.IsOk());
    std::cout << "✓ Transaction validation passed\n";

    auto timestamp_result = validator.ValidateTimestamp(block.header);
    assert(timestamp_result.IsOk());
    std::cout << "✓ Timestamp validation passed\n";

    // Note: PoW validation skipped in tests as it requires actual mining
    std::cout << "✓ Individual validations passed (PoW skipped)\n";

    // Add the block to the chain (this also does validation)
    auto add_result = chain.AddBlock(block);
    assert(add_result.IsOk());
    std::cout << "✓ Valid block added to chain\n";

    // Verify chain state
    assert(chain.GetBestHeight() == 0);
    std::cout << "✓ Chain height updated correctly\n";

    // Add another block
    Block block2 = CreateValidTestBlock(block.GetHash(), 1);

    auto header_result2 = validator.ValidateHeader(block2.header);
    assert(header_result2.IsOk());

    auto add_result2 = chain.AddBlock(block2);
    assert(add_result2.IsOk());
    std::cout << "✓ Second block added to chain\n";

    assert(chain.GetBestHeight() == 1);
    std::cout << "✓ Chain height updated to 1\n";

    db->Close();
    CleanupTestDB();
}

// ============================================================================
// Main Test Runner
// ============================================================================

int main() {
    std::cout << "========================================\n";
    std::cout << "INTcoin Validation Test Suite\n";
    std::cout << "========================================\n";

    try {
        TestBlockHeaderValidation();
        TestTransactionStructureValidation();
        TestCoinbaseValidation();
        TestBlockValidation();
        TestUTXOValidation();
        TestFeeValidation();
        TestCompleteBlockValidation();

        std::cout << "\n========================================\n";
        std::cout << "✓ All validation tests passed!\n";
        std::cout << "========================================\n";

        CleanupTestDB();
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "\n✗ Test failed with exception: " << e.what() << "\n";
        CleanupTestDB();
        return 1;
    }
}
