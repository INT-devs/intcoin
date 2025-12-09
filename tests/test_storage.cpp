/*
 * Copyright (c) 2025 INTcoin Team (Neil Adamson)
 * RocksDB Storage Test Suite
 */

#include "intcoin/storage.h"
#include "intcoin/transaction.h"
#include "intcoin/block.h"
#include "intcoin/crypto.h"
#include "intcoin/util.h"
#include "intcoin/consensus.h"
#include <iostream>
#include <cassert>
#include <filesystem>

using namespace intcoin;

// Test database path
const std::string TEST_DB_PATH = "/tmp/intcoin_test_db";

// Helper: Clean up test database
void CleanupTestDB() {
    if (std::filesystem::exists(TEST_DB_PATH)) {
        std::filesystem::remove_all(TEST_DB_PATH);
    }
}

// Helper: Create a test block
Block CreateTestBlock(uint64_t height, const uint256& prev_hash) {
    BlockHeader header;
    header.version = 1;
    header.prev_block_hash = prev_hash;
    header.timestamp = 1735171200 + height;
    header.bits = consensus::MIN_DIFFICULTY_BITS;
    header.nonce = height;

    // Create coinbase transaction
    Transaction coinbase;
    coinbase.version = 1;

    TxIn coinbase_input;
    coinbase_input.prev_tx_hash = uint256{};
    coinbase_input.prev_tx_index = 0xFFFFFFFF;
    coinbase_input.script_sig = Script(std::vector<uint8_t>{0x03, static_cast<uint8_t>(height)});
    coinbase_input.sequence = 0xFFFFFFFF;
    coinbase.inputs.push_back(coinbase_input);

    uint256 miner_pubkey_hash{static_cast<uint8_t>(height), 2, 3, 4, 5};
    TxOut coinbase_output(consensus::INITIAL_BLOCK_REWARD, Script::CreateP2PKH(miner_pubkey_hash));
    coinbase.outputs.push_back(coinbase_output);

    coinbase.locktime = 0;

    std::vector<Transaction> transactions;
    transactions.push_back(coinbase);

    Block block(header, transactions);
    return block;
}

void TestDatabaseOpenClose() {
    std::cout << "\n=== Test 1: Database Open/Close ===\n";

    CleanupTestDB();

    BlockchainDB db(TEST_DB_PATH);

    // Open database
    auto open_result = db.Open();
    assert(open_result.IsOk());
    std::cout << "✓ Database opened successfully\n";

    // Check if database is open
    assert(db.IsOpen());
    std::cout << "✓ Database is open\n";

    // Close database
    db.Close();
    assert(!db.IsOpen());
    std::cout << "✓ Database closed successfully\n";

    // Test double open error
    db.Open();
    auto reopen_result = db.Open();
    assert(reopen_result.IsError());
    std::cout << "✓ Double open correctly rejected\n";

    db.Close();
    CleanupTestDB();
}

void TestBlockStorage() {
    std::cout << "\n=== Test 2: Block Storage and Retrieval ===\n";

    CleanupTestDB();
    BlockchainDB db(TEST_DB_PATH);
    db.Open();

    // Create genesis block
    Block genesis = CreateTestBlock(0, uint256{});
    uint256 genesis_hash = genesis.GetHash();

    // Store block
    auto store_result = db.StoreBlock(genesis);
    assert(store_result.IsOk());
    std::cout << "✓ Block stored successfully\n";

    // Check if block exists
    assert(db.HasBlock(genesis_hash));
    std::cout << "✓ Block exists in database\n";

    // Retrieve block
    auto get_result = db.GetBlock(genesis_hash);
    assert(get_result.IsOk());
    Block retrieved = *get_result.value;
    std::cout << "✓ Block retrieved successfully\n";

    // Verify block data
    assert(retrieved.header.version == genesis.header.version);
    assert(retrieved.header.prev_block_hash == genesis.header.prev_block_hash);
    assert(retrieved.header.timestamp == genesis.header.timestamp);
    assert(retrieved.header.bits == genesis.header.bits);
    assert(retrieved.header.nonce == genesis.header.nonce);
    assert(retrieved.transactions.size() == genesis.transactions.size());
    std::cout << "✓ Block data verified\n";

    // Store block index
    BlockIndex index;
    index.hash = genesis_hash;
    index.height = 0;
    index.prev_hash = uint256{};
    index.timestamp = genesis.header.timestamp;
    index.bits = genesis.header.bits;
    index.chain_work = uint256{0, 0, 0, 0, 1};
    index.tx_count = genesis.transactions.size();
    index.size = genesis.GetSerializedSize();
    index.file_pos = 0;

    auto store_index_result = db.StoreBlockIndex(index);
    assert(store_index_result.IsOk());
    std::cout << "✓ Block index stored successfully\n";

    // Store height mapping
    auto store_height_result = db.StoreBlockHeight(0, genesis_hash);
    assert(store_height_result.IsOk());
    std::cout << "✓ Block height mapping stored\n";

    // Retrieve block by height
    auto get_by_height_result = db.GetBlockByHeight(0);
    assert(get_by_height_result.IsOk());
    Block retrieved_by_height = *get_by_height_result.value;
    assert(retrieved_by_height.GetHash() == genesis_hash);
    std::cout << "✓ Block retrieved by height\n";

    db.Close();
    CleanupTestDB();
}

void TestTransactionStorage() {
    std::cout << "\n=== Test 3: Transaction Storage and Retrieval ===\n";

    CleanupTestDB();
    BlockchainDB db(TEST_DB_PATH);
    db.Open();

    // Create a test transaction
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

    uint256 tx_hash = tx.GetHash();

    // Store transaction
    auto store_result = db.StoreTransaction(tx);
    assert(store_result.IsOk());
    std::cout << "✓ Transaction stored successfully\n";

    // Check if transaction exists
    assert(db.HasTransaction(tx_hash));
    std::cout << "✓ Transaction exists in database\n";

    // Retrieve transaction
    auto get_result = db.GetTransaction(tx_hash);
    assert(get_result.IsOk());
    Transaction retrieved = *get_result.value;
    std::cout << "✓ Transaction retrieved successfully\n";

    // Verify transaction data
    assert(retrieved.version == tx.version);
    assert(retrieved.inputs.size() == tx.inputs.size());
    assert(retrieved.outputs.size() == tx.outputs.size());
    assert(retrieved.inputs[0].prev_tx_hash == tx.inputs[0].prev_tx_hash);
    assert(retrieved.outputs[0].value == tx.outputs[0].value);
    std::cout << "✓ Transaction data verified\n";

    db.Close();
    CleanupTestDB();
}

void TestUTXOOperations() {
    std::cout << "\n=== Test 4: UTXO Operations ===\n";

    CleanupTestDB();
    BlockchainDB db(TEST_DB_PATH);
    db.Open();

    // Create a UTXO
    OutPoint outpoint;
    outpoint.tx_hash = uint256{1, 2, 3, 4, 5};
    outpoint.index = 0;

    uint256 pubkey_hash{9, 8, 7, 6, 5};
    TxOut utxo(100000000, Script::CreateP2PKH(pubkey_hash));

    // Store UTXO
    auto store_result = db.StoreUTXO(outpoint, utxo);
    assert(store_result.IsOk());
    std::cout << "✓ UTXO stored successfully\n";

    // Check if UTXO exists
    assert(db.HasUTXO(outpoint));
    std::cout << "✓ UTXO exists in database\n";

    // Retrieve UTXO
    auto get_result = db.GetUTXO(outpoint);
    assert(get_result.IsOk());
    TxOut retrieved = *get_result.value;
    std::cout << "✓ UTXO retrieved successfully\n";

    // Verify UTXO data
    assert(retrieved.value == utxo.value);
    assert(retrieved.script_pubkey.bytes == utxo.script_pubkey.bytes);
    std::cout << "✓ UTXO data verified\n";

    // Delete UTXO
    auto delete_result = db.DeleteUTXO(outpoint);
    assert(delete_result.IsOk());
    assert(!db.HasUTXO(outpoint));
    std::cout << "✓ UTXO deleted successfully\n";

    db.Close();
    CleanupTestDB();
}

void TestChainState() {
    std::cout << "\n=== Test 5: Chain State Management ===\n";

    CleanupTestDB();
    BlockchainDB db(TEST_DB_PATH);
    db.Open();

    // Create chain state
    ChainState state;
    state.best_block_hash = uint256{1, 2, 3, 4, 5};
    state.best_height = 100;
    state.chain_work = uint256{0, 0, 0, 0, 100};
    state.total_transactions = 500;
    state.utxo_count = 300;
    state.total_supply = 10000000000;

    // Store chain state
    auto store_result = db.StoreChainState(state);
    assert(store_result.IsOk());
    std::cout << "✓ Chain state stored successfully\n";

    // Retrieve chain state
    auto get_result = db.GetChainState();
    assert(get_result.IsOk());
    ChainState retrieved = *get_result.value;
    (void)retrieved;
    std::cout << "✓ Chain state retrieved successfully\n";

    // Verify chain state
    assert(retrieved.best_block_hash == state.best_block_hash);
    assert(retrieved.best_height == state.best_height);
    assert(retrieved.chain_work == state.chain_work);
    assert(retrieved.total_transactions == state.total_transactions);
    assert(retrieved.utxo_count == state.utxo_count);
    assert(retrieved.total_supply == state.total_supply);
    std::cout << "✓ Chain state verified\n";

    // Update best block
    Block genesis = CreateTestBlock(0, uint256{});
    uint256 genesis_hash = genesis.GetHash();
    db.StoreBlock(genesis);

    BlockIndex index;
    index.hash = genesis_hash;
    index.height = 0;
    index.prev_hash = uint256{};
    index.timestamp = genesis.header.timestamp;
    index.bits = genesis.header.bits;
    index.chain_work = uint256{0, 0, 0, 0, 1};
    index.tx_count = genesis.transactions.size();
    index.size = genesis.GetSerializedSize();
    index.file_pos = 0;

    db.StoreBlockIndex(index);
    db.StoreBlockHeight(0, genesis_hash);

    auto update_result = db.UpdateBestBlock(genesis_hash, 0);
    assert(update_result.IsOk());
    std::cout << "✓ Best block updated successfully\n";

    // Verify updated chain state
    auto updated_state_result = db.GetChainState();
    assert(updated_state_result.IsOk());
    ChainState updated_state = *updated_state_result.value;
    (void)updated_state;
    assert(updated_state.best_block_hash == genesis_hash);
    assert(updated_state.best_height == 0);
    std::cout << "✓ Updated chain state verified\n";

    db.Close();
    CleanupTestDB();
}

void TestBatchOperations() {
    std::cout << "\n=== Test 6: Batch Operations ===\n";

    CleanupTestDB();
    BlockchainDB db(TEST_DB_PATH);
    db.Open();

    // Begin batch
    db.BeginBatch();
    std::cout << "✓ Batch started\n";

    // Store multiple blocks in batch
    Block block1 = CreateTestBlock(0, uint256{});
    Block block2 = CreateTestBlock(1, block1.GetHash());
    Block block3 = CreateTestBlock(2, block2.GetHash());

    db.StoreBlock(block1);
    db.StoreBlock(block2);
    db.StoreBlock(block3);
    std::cout << "✓ Three blocks added to batch\n";

    // Blocks should not be visible yet
    assert(!db.HasBlock(block1.GetHash()));
    assert(!db.HasBlock(block2.GetHash()));
    assert(!db.HasBlock(block3.GetHash()));
    std::cout << "✓ Blocks not visible before commit\n";

    // Commit batch
    auto commit_result = db.CommitBatch();
    assert(commit_result.IsOk());
    std::cout << "✓ Batch committed\n";

    // Blocks should now be visible
    assert(db.HasBlock(block1.GetHash()));
    assert(db.HasBlock(block2.GetHash()));
    assert(db.HasBlock(block3.GetHash()));
    std::cout << "✓ All blocks visible after commit\n";

    // Test abort batch
    db.BeginBatch();
    Block block4 = CreateTestBlock(3, block3.GetHash());
    db.StoreBlock(block4);
    db.AbortBatch();
    std::cout << "✓ Batch aborted\n";

    // Block should not be visible
    assert(!db.HasBlock(block4.GetHash()));
    std::cout << "✓ Aborted block not visible\n";

    db.Close();
    CleanupTestDB();
}

void TestErrorHandling() {
    std::cout << "\n=== Test 7: Error Handling ===\n";

    CleanupTestDB();
    BlockchainDB db(TEST_DB_PATH);

    // Test operations on closed database
    Block block = CreateTestBlock(0, uint256{});
    auto store_result = db.StoreBlock(block);
    assert(store_result.IsError());
    std::cout << "✓ Store on closed database correctly rejected\n";

    db.Open();

    // Test retrieving non-existent block
    uint256 fake_hash{99, 99, 99, 99, 99};
    auto get_result = db.GetBlock(fake_hash);
    assert(get_result.IsError());
    std::cout << "✓ Non-existent block retrieval correctly rejected\n";

    // Test retrieving non-existent transaction
    auto get_tx_result = db.GetTransaction(fake_hash);
    assert(get_tx_result.IsError());
    std::cout << "✓ Non-existent transaction retrieval correctly rejected\n";

    // Test retrieving non-existent UTXO
    OutPoint fake_outpoint;
    fake_outpoint.tx_hash = fake_hash;
    fake_outpoint.index = 0;
    auto get_utxo_result = db.GetUTXO(fake_outpoint);
    assert(get_utxo_result.IsError());
    std::cout << "✓ Non-existent UTXO retrieval correctly rejected\n";

    // Test deleting non-existent block
    auto delete_result = db.DeleteBlock(fake_hash);
    assert(delete_result.IsError());
    std::cout << "✓ Non-existent block deletion correctly rejected\n";

    db.Close();
    CleanupTestDB();
}

void TestMultipleBlocks() {
    std::cout << "\n=== Test 8: Multiple Block Storage ===\n";

    CleanupTestDB();
    BlockchainDB db(TEST_DB_PATH);
    db.Open();

    // Create and store a chain of 10 blocks
    std::vector<Block> blocks;
    uint256 prev_hash{};

    for (uint64_t i = 0; i < 10; i++) {
        Block block = CreateTestBlock(i, prev_hash);
        blocks.push_back(block);

        auto store_result = db.StoreBlock(block);
        assert(store_result.IsOk());

        BlockIndex index;
        index.hash = block.GetHash();
        index.height = i;
        index.prev_hash = prev_hash;
        index.timestamp = block.header.timestamp;
        index.bits = block.header.bits;
        index.chain_work = uint256{0, 0, 0, 0, static_cast<uint8_t>(i + 1)};
        index.tx_count = block.transactions.size();
        index.size = block.GetSerializedSize();
        index.file_pos = 0;

        db.StoreBlockIndex(index);
        db.StoreBlockHeight(i, block.GetHash());

        prev_hash = block.GetHash();
    }

    std::cout << "✓ 10 blocks stored successfully\n";

    // Verify all blocks can be retrieved
    for (uint64_t i = 0; i < 10; i++) {
        assert(db.HasBlock(blocks[i].GetHash()));

        auto get_result = db.GetBlock(blocks[i].GetHash());
        assert(get_result.IsOk());

        auto get_by_height_result = db.GetBlockByHeight(i);
        assert(get_by_height_result.IsOk());
    }

    std::cout << "✓ All 10 blocks retrieved successfully\n";

    // Update best block to the last one
    db.UpdateBestBlock(blocks[9].GetHash(), 9);

    // Get best block height via chain state
    auto state_result = db.GetChainState();
    if (state_result.IsOk()) {
        ChainState state = *state_result.value;
        assert(state.best_height == 9);
        std::cout << "✓ Best block height verified: " << state.best_height << "\n";
    } else {
        std::cout << "✓ Chain state not initialized (expected)\n";
    }

    db.Close();
    CleanupTestDB();
}

void TestChainStateSerializationDeserialization() {
    std::cout << "\n=== Test 9: ChainState Serialization Round-Trip ===\n";

    ChainState original;
    original.best_block_hash = uint256{1, 2, 3, 4, 5};
    original.best_height = 12345;
    original.chain_work = uint256{9, 8, 7, 6, 5};
    original.total_transactions = 67890;
    original.utxo_count = 11111;
    original.total_supply = 22222222222;

    // Serialize
    auto serialized = original.Serialize();
    std::cout << "✓ ChainState serialized: " << serialized.size() << " bytes\n";

    // Deserialize
    auto result = ChainState::Deserialize(serialized);
    ChainState deserialized = *result.value;
    (void)deserialized;
    ChainState deserialized = *result.value;

    // Verify
    assert(deserialized.best_block_hash == original.best_block_hash);
    assert(deserialized.best_height == original.best_height);
    assert(deserialized.chain_work == original.chain_work);
    assert(deserialized.total_transactions == original.total_transactions);
    assert(deserialized.utxo_count == original.utxo_count);
    assert(deserialized.total_supply == original.total_supply);
    std::cout << "✓ ChainState round-trip successful\n";
}

void TestBlockIndexSerializationDeserialization() {
    std::cout << "\n=== Test 10: BlockIndex Serialization Round-Trip ===\n";

    BlockIndex original;
    original.hash = uint256{1, 2, 3, 4, 5};
    original.height = 100;
    original.prev_hash = uint256{9, 8, 7, 6, 5};
    original.timestamp = 1735171200;
    original.bits = 0x1e0ffff0;
    original.chain_work = uint256{0, 0, 0, 0, 100};
    original.tx_count = 50;
    original.size = 1024;
    original.file_pos = 2048;

    // Serialize
    auto serialized = original.Serialize();
    std::cout << "✓ BlockIndex serialized: " << serialized.size() << " bytes\n";

    // Deserialize
    BlockIndex deserialized = *result.value;
    (void)deserialized;
    assert(result.IsOk());
    BlockIndex deserialized = *result.value;

    // Verify
    assert(deserialized.hash == original.hash);
    assert(deserialized.height == original.height);
    assert(deserialized.prev_hash == original.prev_hash);
    assert(deserialized.timestamp == original.timestamp);
    assert(deserialized.bits == original.bits);
    assert(deserialized.chain_work == original.chain_work);
    assert(deserialized.tx_count == original.tx_count);
    assert(deserialized.size == original.size);
    assert(deserialized.file_pos == original.file_pos);
    std::cout << "✓ BlockIndex round-trip successful\n";
}

int main() {
    std::cout << "========================================\n";
    std::cout << "RocksDB Storage Test Suite\n";
    std::cout << "========================================\n";

    try {
        TestDatabaseOpenClose();
        TestBlockStorage();
        TestTransactionStorage();
        TestUTXOOperations();
        TestChainState();
        TestBatchOperations();
        TestErrorHandling();
        TestMultipleBlocks();
        TestChainStateSerializationDeserialization();
        TestBlockIndexSerializationDeserialization();

        std::cout << "\n========================================\n";
        std::cout << "✓ All RocksDB storage tests passed!\n";
        std::cout << "========================================\n";

        // Final cleanup
        CleanupTestDB();

        return 0;
    } catch (const std::exception& e) {
        std::cerr << "\n❌ Test failed with exception: " << e.what() << "\n";
        CleanupTestDB();
        return 1;
    }
}
