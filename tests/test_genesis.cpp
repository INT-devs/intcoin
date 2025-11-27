/*
 * Copyright (c) 2025 INTcoin Team (Neil Adamson)
 * SPDX-License-Identifier: MIT License
 * Genesis Block Verification
 */

#include "intcoin/block.h"
#include "intcoin/util.h"
#include <iostream>
#include <iomanip>
#include <cassert>

using namespace intcoin;

int main() {
    std::cout << "========================================\n";
    std::cout << "INTcoin Genesis Block Information\n";
    std::cout << "========================================\n\n";

    // Create genesis block
    Block genesis = CreateGenesisBlock();

    // Display header information
    std::cout << "Block Header:\n";
    std::cout << "  Version:     " << genesis.header.version << "\n";
    std::cout << "  Timestamp:   " << genesis.header.timestamp << " (26 Nov 2025 13:18:00 UTC)\n";
    std::cout << "  Bits:        0x" << std::hex << genesis.header.bits << std::dec << "\n";
    std::cout << "  Nonce:       " << genesis.header.nonce << "\n";
    std::cout << "  Prev Hash:   " << ToHex(genesis.header.prev_block_hash) << "\n";
    std::cout << "  Merkle Root: " << ToHex(genesis.header.merkle_root) << "\n";
    std::cout << "  Block Hash:  " << ToHex(genesis.GetHash()) << "\n\n";

    // Display coinbase transaction
    assert(!genesis.transactions.empty());
    const Transaction& coinbase = genesis.transactions[0];

    std::cout << "Coinbase Transaction:\n";
    std::cout << "  Version:     " << coinbase.version << "\n";
    std::cout << "  Locktime:    " << coinbase.locktime << "\n";
    std::cout << "  Inputs:      " << coinbase.inputs.size() << "\n";
    std::cout << "  Outputs:     " << coinbase.outputs.size() << "\n";
    std::cout << "  TX Hash:     " << ToHex(coinbase.GetHash()) << "\n\n";

    // Display genesis message from coinbase script_sig
    assert(!coinbase.inputs.empty());
    const TxIn& input = coinbase.inputs[0];

    std::cout << "Genesis Message (from coinbase script_sig):\n";
    std::cout << "  Length: " << input.script_sig.GetSize() << " bytes\n";

    if (!input.script_sig.IsEmpty()) {
        std::string message(input.script_sig.bytes.begin(), input.script_sig.bytes.end());
        std::cout << "  Message: \"" << message << "\"\n\n";

        // Verify the message content
        std::string expected = "13:18, 26 November 2025 This Is Money, Financial markets in turmoil as Budget leak fiasco sends pound and gilts on rollercoaster ride";
        assert(message == expected);
        std::cout << "✓ Genesis message verified!\n";
    } else {
        std::cout << "  (Empty script)\n";
    }

    // Display coinbase output
    assert(!coinbase.outputs.empty());
    const TxOut& output = coinbase.outputs[0];

    std::cout << "\nCoinbase Output:\n";
    std::cout << "  Value:  " << output.value << " INTS\n";
    std::cout << "         (" << (output.value / 1000000.0) << " INT)\n";
    std::cout << "  Script: " << output.script_pubkey.GetSize() << " bytes\n";

    std::cout << "\n========================================\n";
    std::cout << "✓ Genesis block created successfully!\n";
    std::cout << "========================================\n";

    return 0;
}
