#!/usr/bin/env python3
# Copyright (c) 2025 INTcoin Core (Maddison Lane)
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
#
# Basic Functionality Test

"""Test basic blockchain operations."""

from test_framework import TestFramework
from decimal import Decimal


class BasicTest(TestFramework):
    """Test basic blockchain functionality."""

    def __init__(self):
        super().__init__()
        self.num_nodes = 1

    def run_test(self):
        """Run the test."""
        node = self.nodes[0]

        # Test 1: Initial block count should be 0
        self.log_info("Test 1: Check initial block count")
        block_count = node.getblockcount()
        self.assert_equal(block_count, 0, "Initial block count should be 0")
        self.log_success("Initial block count is correct")

        # Test 2: Generate blocks
        self.log_info("Test 2: Generate 10 blocks")
        blocks = node.generate(10)
        self.assert_equal(len(blocks), 10, "Should generate 10 blocks")
        block_count = node.getblockcount()
        self.assert_equal(block_count, 10, "Block count should be 10")
        self.log_success("Generated 10 blocks successfully")

        # Test 3: Check balance after mining
        self.log_info("Test 3: Check balance after mining")
        balance = node.getbalance()
        self.assert_greater_than(balance, Decimal(0), "Balance should be positive after mining")
        self.log_success(f"Balance after mining: {balance}")

        # Test 4: Generate new address
        self.log_info("Test 4: Generate new address")
        address = node.getnewaddress()
        self.assert_equal(len(address) > 0, True, "Address should not be empty")
        self.log_success(f"Generated address: {address}")

        # Test 5: Send coins
        self.log_info("Test 5: Send coins to address")
        send_amount = Decimal("10.0")
        initial_balance = node.getbalance()
        txid = node.sendtoaddress(address, send_amount)
        self.assert_equal(len(txid), 64, "Transaction ID should be 64 characters")
        self.log_success(f"Transaction sent: {txid}")

        # Test 6: Mine block to confirm transaction
        self.log_info("Test 6: Mine block to confirm transaction")
        node.generate(1)
        block_count = node.getblockcount()
        self.assert_equal(block_count, 11, "Block count should be 11")
        self.log_success("Transaction confirmed in block")


if __name__ == '__main__':
    test = BasicTest()
    exit(test.main())
