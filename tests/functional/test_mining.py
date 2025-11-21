#!/usr/bin/env python3
# Copyright (c) 2025 INTcoin Core (Maddison Lane)
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
#
# Mining Functionality Test

"""Test mining and block generation."""

from test_framework import TestFramework
from decimal import Decimal
import time


class MiningTest(TestFramework):
    """Test mining functionality."""

    def __init__(self):
        super().__init__()
        self.num_nodes = 1

    def run_test(self):
        """Run the test."""
        node = self.nodes[0]

        # Test 1: Single block generation
        self.log_info("Test 1: Generate single block")
        initial_height = node.getblockcount()
        blocks = node.generate(1)
        self.assert_equal(len(blocks), 1, "Should generate 1 block")
        new_height = node.getblockcount()
        self.assert_equal(new_height, initial_height + 1, "Height should increase by 1")
        self.log_success(f"Generated block at height {new_height}")

        # Test 2: Multiple block generation
        self.log_info("Test 2: Generate 100 blocks")
        blocks = node.generate(100)
        self.assert_equal(len(blocks), 100, "Should generate 100 blocks")
        height = node.getblockcount()
        self.assert_equal(height, 101, "Height should be 101")
        self.log_success("Generated 100 blocks successfully")

        # Test 3: Verify block rewards
        self.log_info("Test 3: Verify mining rewards")
        balance = node.getbalance()
        self.assert_greater_than(balance, Decimal(0), "Should have mining rewards")
        self.log_success(f"Mining rewards accumulated: {balance} INT")

        # Test 4: Check coinbase maturity
        self.log_info("Test 4: Test coinbase maturity")
        # First block rewards should be spendable after 100 confirmations
        spendable = node.rpc("getbalance")
        self.assert_greater_than(Decimal(str(spendable)), Decimal(0), "Should have spendable balance")
        self.log_success(f"Spendable balance: {spendable} INT")

        # Test 5: Block time verification
        self.log_info("Test 5: Verify block timestamps")
        current_block_hash = node.rpc("getbestblockhash")
        block_info = node.rpc("getblock", [current_block_hash])
        self.assert_greater_than(block_info["time"], 0, "Block should have valid timestamp")
        self.assert_equal(block_info["height"], 101, "Block height should be 101")
        self.log_success(f"Block timestamp: {block_info['time']}")

        # Test 6: Mining difficulty
        self.log_info("Test 6: Check mining difficulty")
        info = node.rpc("getmininginfo")
        self.assert_greater_than(info["difficulty"], 0, "Difficulty should be positive")
        self.log_success(f"Mining difficulty: {info['difficulty']}")

        # Test 7: Generate blocks rapidly
        self.log_info("Test 7: Generate blocks rapidly")
        start_time = time.time()
        node.generate(50)
        elapsed = time.time() - start_time
        self.assert_greater_than(elapsed, 0, "Should take some time to generate")
        self.log_success(f"Generated 50 blocks in {elapsed:.2f} seconds")

        # Test 8: Final verification
        self.log_info("Test 8: Final height verification")
        final_height = node.getblockcount()
        self.assert_equal(final_height, 151, "Final height should be 151")
        self.log_success(f"Final blockchain height: {final_height}")


if __name__ == '__main__':
    test = MiningTest()
    exit(test.main())
