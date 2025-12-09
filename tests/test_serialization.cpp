/*
 * Copyright (c) 2025 INTcoin Team (Neil Adamson)
 * Serialization Test Suite
 */

#include "intcoin/transaction.h"
#include "intcoin/block.h"
#include "intcoin/crypto.h"
#include "intcoin/util.h"
#include <iostream>
#include <cassert>

using namespace intcoin;

void TestTxInSerialization() {
    std::cout << "\n=== Test 1: TxIn Serialization/Deserialization ===\n";

    // Create a TxIn
    TxIn original;
    original.prev_tx_hash = uint256{1, 2, 3, 4, 5};
    original.prev_tx_index = 42;
    original.script_sig = Script(std::vector<uint8_t>{0x76, 0xa9, 0x14});  // Simple script
    original.sequence = 0xFFFFFFFE;

    // Serialize
    auto serialized = original.Serialize();
    std::cout << "✓ TxIn serialized: " << serialized.size() << " bytes\n";

    // Deserialize
    auto result = TxIn::Deserialize(serialized);
    assert(result.IsOk());
    TxIn deserialized = *result.value;

    // Verify
    assert(deserialized.prev_tx_hash == original.prev_tx_hash);
    assert(deserialized.prev_tx_index == original.prev_tx_index);
    assert(deserialized.script_sig.bytes == original.script_sig.bytes);
    assert(deserialized.sequence == original.sequence);
    std::cout << "✓ TxIn round-trip successful\n";
}

void TestTxOutSerialization() {
    std::cout << "\n=== Test 2: TxOut Serialization/Deserialization ===\n";

    // Create a TxOut
    uint256 pubkey_hash{9, 8, 7, 6, 5};
    Script script_pubkey = Script::CreateP2PKH(pubkey_hash);
    TxOut original(100000000, script_pubkey);

    // Serialize
    auto serialized = original.Serialize();
    std::cout << "✓ TxOut serialized: " << serialized.size() << " bytes\n";

    // Deserialize
    auto result = TxOut::Deserialize(serialized);
    assert(result.IsOk());
    TxOut deserialized = *result.value;

    // Verify
    assert(deserialized.value == original.value);
    assert(deserialized.script_pubkey.bytes == original.script_pubkey.bytes);
    std::cout << "✓ TxOut round-trip successful\n";
}

void TestOutPointSerialization() {
    std::cout << "\n=== Test 3: OutPoint Serialization/Deserialization ===\n";

    // Create an OutPoint
    OutPoint original;
    original.tx_hash = uint256{10, 20, 30, 40, 50};
    original.index = 123;

    // Serialize
    auto serialized = original.Serialize();
    std::cout << "✓ OutPoint serialized: " << serialized.size() << " bytes\n";

    // Deserialize
    auto result = OutPoint::Deserialize(serialized);
    assert(result.IsOk());
    OutPoint deserialized = *result.value;

    // Verify
    assert(deserialized.tx_hash == original.tx_hash);
    assert(deserialized.index == original.index);
    std::cout << "✓ OutPoint round-trip successful\n";
}

void TestTransactionSerialization() {
    std::cout << "\n=== Test 4: Transaction Serialization/Deserialization ===\n";

    // Create a transaction
    Transaction original;
    original.version = 1;

    // Add input
    TxIn input;
    input.prev_tx_hash = uint256{1, 2, 3, 4, 5};
    input.prev_tx_index = 0;
    input.script_sig = Script(std::vector<uint8_t>{0x48, 0x30, 0x45});  // Signature script
    input.sequence = 0xFFFFFFFF;
    original.inputs.push_back(input);

    // Add output
    uint256 pubkey_hash{9, 8, 7, 6, 5};
    Script script_pubkey = Script::CreateP2PKH(pubkey_hash);
    TxOut output(50000000, script_pubkey);
    original.outputs.push_back(output);

    original.locktime = 0;
    // signature is default-initialized (zeros)

    // Serialize
    auto serialized = original.Serialize();
    std::cout << "✓ Transaction serialized: " << serialized.size() << " bytes\n";
    std::cout << "  Calculated size: " << original.GetSerializedSize() << " bytes\n";
    // Note: Size calculation may differ slightly due to variable-length encoding

    // Deserialize
    auto result = Transaction::Deserialize(serialized);
    assert(result.IsOk());
    Transaction deserialized = *result.value;

    // Verify
    assert(deserialized.version == original.version);
    assert(deserialized.inputs.size() == original.inputs.size());
    assert(deserialized.outputs.size() == original.outputs.size());
    assert(deserialized.locktime == original.locktime);

    // Verify input
    assert(deserialized.inputs[0].prev_tx_hash == original.inputs[0].prev_tx_hash);
    assert(deserialized.inputs[0].prev_tx_index == original.inputs[0].prev_tx_index);
    assert(deserialized.inputs[0].sequence == original.inputs[0].sequence);

    // Verify output
    assert(deserialized.outputs[0].value == original.outputs[0].value);

    std::cout << "✓ Transaction round-trip successful\n";
}

void TestMultiInputOutputTransaction() {
    std::cout << "\n=== Test 5: Multi-Input/Output Transaction ===\n";

    // Create a complex transaction with multiple inputs and outputs
    Transaction original;
    original.version = 1;

    // Add 3 inputs
    for (int i = 0; i < 3; i++) {
        TxIn input;
        input.prev_tx_hash = uint256{static_cast<uint8_t>(i), static_cast<uint8_t>(i+1), static_cast<uint8_t>(i+2)};
        input.prev_tx_index = i;
        input.script_sig = Script(std::vector<uint8_t>{0x48, static_cast<uint8_t>(i)});
        input.sequence = 0xFFFFFFFF - i;
        original.inputs.push_back(input);
    }

    // Add 2 outputs
    for (int i = 0; i < 2; i++) {
        uint256 pubkey_hash{static_cast<uint8_t>(i*10), static_cast<uint8_t>(i*10+1)};
        Script script_pubkey = Script::CreateP2PKH(pubkey_hash);
        TxOut output(25000000 * (i + 1), script_pubkey);
        original.outputs.push_back(output);
    }

    original.locktime = 500000;

    // Serialize
    auto serialized = original.Serialize();
    std::cout << "✓ Complex transaction serialized: " << serialized.size() << " bytes\n";

    // Deserialize
    auto result = Transaction::Deserialize(serialized);
    assert(result.IsOk());
    Transaction deserialized = *result.value;

    // Verify counts
    assert(deserialized.inputs.size() == 3);
    assert(deserialized.outputs.size() == 2);

    // Verify all inputs
    for (size_t i = 0; i < 3; i++) {
        assert(deserialized.inputs[i].prev_tx_hash == original.inputs[i].prev_tx_hash);
        assert(deserialized.inputs[i].prev_tx_index == original.inputs[i].prev_tx_index);
    }

    // Verify all outputs
    for (size_t i = 0; i < 2; i++) {
        assert(deserialized.outputs[i].value == original.outputs[i].value);
    }

    std::cout << "✓ Complex transaction round-trip successful\n";
}

void TestBlockHeaderSerialization() {
    std::cout << "\n=== Test 6: BlockHeader Serialization/Deserialization ===\n";

    // Create a block header
    BlockHeader original;
    original.version = 1;
    original.prev_block_hash = uint256{1, 2, 3, 4, 5};
    original.merkle_root = uint256{9, 8, 7, 6, 5};
    original.timestamp = 1735171200;
    original.bits = 0x1e0ffff0;
    original.nonce = 123456;
    original.randomx_hash = uint256{11, 22, 33, 44, 55};
    original.randomx_key = uint256{99, 88, 77, 66, 55};

    // Serialize
    auto serialized = original.Serialize();
    std::cout << "✓ BlockHeader serialized: " << serialized.size() << " bytes\n";
    assert(serialized.size() == 152);  // Fixed size

    // Deserialize
    auto result = BlockHeader::Deserialize(serialized);
    assert(result.IsOk());
    BlockHeader deserialized = *result.value;

    // Verify
    assert(deserialized.version == original.version);
    assert(deserialized.prev_block_hash == original.prev_block_hash);
    assert(deserialized.merkle_root == original.merkle_root);
    assert(deserialized.timestamp == original.timestamp);
    assert(deserialized.bits == original.bits);
    assert(deserialized.nonce == original.nonce);
    assert(deserialized.randomx_hash == original.randomx_hash);
    assert(deserialized.randomx_key == original.randomx_key);

    std::cout << "✓ BlockHeader round-trip successful\n";
}

void TestBlockSerialization() {
    std::cout << "\n=== Test 7: Block Serialization/Deserialization ===\n";

    // Create a block header
    BlockHeader header;
    header.version = 1;
    header.prev_block_hash = uint256{};  // Genesis
    header.timestamp = 1735171200;
    header.bits = 0x1e0ffff0;
    header.nonce = 0;

    // Create a coinbase transaction
    Transaction coinbase;
    coinbase.version = 1;

    TxIn coinbase_input;
    coinbase_input.prev_tx_hash = uint256{};  // Null hash
    coinbase_input.prev_tx_index = 0xFFFFFFFF;
    coinbase_input.script_sig = Script(std::vector<uint8_t>{0x03, 0x00, 0x00, 0x00});  // Block height
    coinbase_input.sequence = 0xFFFFFFFF;
    coinbase.inputs.push_back(coinbase_input);

    uint256 miner_pubkey_hash{1, 2, 3, 4, 5};
    TxOut coinbase_output(105113636, Script::CreateP2PKH(miner_pubkey_hash));
    coinbase.outputs.push_back(coinbase_output);

    coinbase.locktime = 0;

    // Create block
    std::vector<Transaction> transactions;
    transactions.push_back(coinbase);

    Block original(header, transactions);

    // Serialize
    auto serialized = original.Serialize();
    std::cout << "✓ Block serialized: " << serialized.size() << " bytes\n";
    std::cout << "  Expected size: " << original.GetSerializedSize() << " bytes\n";

    // Deserialize
    auto result = Block::Deserialize(serialized);
    assert(result.IsOk());
    Block deserialized = *result.value;

    // Verify header
    assert(deserialized.header.version == original.header.version);
    assert(deserialized.header.prev_block_hash == original.header.prev_block_hash);
    assert(deserialized.header.timestamp == original.header.timestamp);
    assert(deserialized.header.bits == original.header.bits);

    // Verify transactions
    assert(deserialized.transactions.size() == 1);
    assert(deserialized.transactions[0].version == original.transactions[0].version);
    assert(deserialized.transactions[0].inputs.size() == 1);
    assert(deserialized.transactions[0].outputs.size() == 1);
    assert(deserialized.transactions[0].outputs[0].value == 105113636);

    std::cout << "✓ Block round-trip successful\n";
}

void TestSerializationErrorHandling() {
    std::cout << "\n=== Test 8: Serialization Error Handling ===\n";

    // Test empty data
    std::vector<uint8_t> empty;
    auto result1 = TxIn::Deserialize(empty);
    assert(result1.IsError());
    std::cout << "✓ Empty data correctly rejected for TxIn\n";

    // Test truncated data
    std::vector<uint8_t> truncated(10, 0);  // Not enough bytes
    auto result2 = BlockHeader::Deserialize(truncated);
    assert(result2.IsError());
    std::cout << "✓ Truncated data correctly rejected for BlockHeader\n";

    // Test invalid transaction count (says 1 transaction but doesn't provide it)
    std::vector<uint8_t> invalid_block(152, 0);  // Valid header
    // Add transaction count = 1
    SerializeUint64(invalid_block, 1);
    // But don't add the transaction data - this should fail
    auto result3 = Block::Deserialize(invalid_block);
    assert(result3.IsError());
    std::cout << "✓ Invalid block data correctly rejected\n";

    std::cout << "✓ Error handling working correctly\n";
}

void TestSerializationDeterminism() {
    std::cout << "\n=== Test 9: Serialization Determinism ===\n";

    // Create a transaction
    Transaction tx;
    tx.version = 1;

    TxIn input;
    input.prev_tx_hash = uint256{1, 2, 3, 4, 5};
    input.prev_tx_index = 0;
    input.script_sig = Script(std::vector<uint8_t>{0x48, 0x30, 0x45});
    input.sequence = 0xFFFFFFFF;
    tx.inputs.push_back(input);

    uint256 pubkey_hash{9, 8, 7, 6, 5};
    TxOut output(50000000, Script::CreateP2PKH(pubkey_hash));
    tx.outputs.push_back(output);

    tx.locktime = 0;

    // Serialize multiple times
    auto serialized1 = tx.Serialize();
    auto serialized2 = tx.Serialize();
    auto serialized3 = tx.Serialize();

    // Verify all serializations are identical
    assert(serialized1 == serialized2);
    assert(serialized2 == serialized3);
    std::cout << "✓ Serialization is deterministic\n";

    // Verify hash is the same
    auto hash1 = tx.GetHash();
    auto hash2 = tx.GetHash();
    assert(hash1 == hash2);
    std::cout << "✓ Transaction hashes are deterministic\n";
}

int main() {
    std::cout << "========================================\n";
    std::cout << "Serialization Test Suite\n";
    std::cout << "========================================\n";

    try {
        TestTxInSerialization();
        TestTxOutSerialization();
        TestOutPointSerialization();
        TestTransactionSerialization();
        TestMultiInputOutputTransaction();
        TestBlockHeaderSerialization();
        TestBlockSerialization();
        TestSerializationErrorHandling();
        TestSerializationDeterminism();

        std::cout << "\n========================================\n";
        std::cout << "✓ All serialization tests passed!\n";
        std::cout << "========================================\n";

        return 0;
    } catch (const std::exception& e) {
        std::cerr << "\n❌ Test failed with exception: " << e.what() << "\n";
        return 1;
    }
}
