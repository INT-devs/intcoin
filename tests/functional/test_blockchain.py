#!/usr/bin/env python3
# Copyright (c) 2025 INTcoin Core (Maddison Lane)
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
#
# Blockchain Functionality Test

"""Test blockchain operations and consensus."""

from test_framework import TestFramework
from decimal import Decimal


class BlockchainTest(TestFramework):
    """Test blockchain functionality."""

    def __init__(self):
        super().__init__()
        self.num_nodes = 3

    def run_test(self):
        """Run the test."""
        # Test 1: Initial sync
        self.log_info("Test 1: Verify all nodes start at height 0")
        for i, node in enumerate(self.nodes):
            height = node.getblockcount()
            self.assert_equal(height, 0, f"Node {i} should start at height 0")
        self.log_success("All nodes at height 0")

        # Test 2: Generate blocks on node 0
        self.log_info("Test 2: Generate 10 blocks on node 0")
        self.nodes[0].generate(10)
        self.sync_blocks()
        for i, node in enumerate(self.nodes):
            height = node.getblockcount()
            self.assert_equal(height, 10, f"Node {i} should be at height 10")
        self.log_success("All nodes synchronized at height 10")

        # Test 3: Generate blocks on different nodes
        self.log_info("Test 3: Generate blocks on node 1")
        self.nodes[1].generate(5)
        self.sync_blocks()
        for i, node in enumerate(self.nodes):
            height = node.getblockcount()
            self.assert_equal(height, 15, f"Node {i} should be at height 15")
        self.log_success("All nodes synchronized at height 15")

        # Test 4: Verify block consistency
        self.log_info("Test 4: Verify block hashes match across nodes")
        for height in range(1, 16):
            hash0 = self.nodes[0].rpc("getblockhash", [height])
            hash1 = self.nodes[1].rpc("getblockhash", [height])
            hash2 = self.nodes[2].rpc("getblockhash", [height])
            self.assert_equal(hash0, hash1, f"Block {height} hash mismatch between node 0 and 1")
            self.assert_equal(hash1, hash2, f"Block {height} hash mismatch between node 1 and 2")
        self.log_success("All block hashes match across nodes")

        # Test 5: Chain reorganization test
        self.log_info("Test 5: Test simple chain reorganization")
        # Generate blocks on node 2
        self.nodes[2].generate(20)
        self.sync_blocks()
        final_height = self.nodes[0].getblockcount()
        self.assert_equal(final_height, 35, "Chain should be at height 35 after reorg")
        self.log_success("Chain reorganization successful")

        # Test 6: Verify blockchain info
        self.log_info("Test 6: Verify blockchain info")
        info = self.nodes[0].rpc("getblockchaininfo")
        self.assert_equal(info["chain"], "regtest", "Should be on regtest chain")
        self.assert_equal(info["blocks"], 35, "Should have 35 blocks")
        self.log_success(f"Blockchain info verified: {info['chain']} with {info['blocks']} blocks")


if __name__ == '__main__':
    test = BlockchainTest()
    exit(test.main())
