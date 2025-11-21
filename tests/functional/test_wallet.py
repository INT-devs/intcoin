#!/usr/bin/env python3
# Copyright (c) 2025 INTcoin Core (Maddison Lane)
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
#
# Wallet Functionality Test

"""Test wallet operations and transactions."""

from test_framework import TestFramework, RPCError
from decimal import Decimal


class WalletTest(TestFramework):
    """Test wallet functionality."""

    def __init__(self):
        super().__init__()
        self.num_nodes = 2

    def run_test(self):
        """Run the test."""
        node0 = self.nodes[0]
        node1 = self.nodes[1]

        # Test 1: Generate coins on node0
        self.log_info("Test 1: Generate initial coins on node0")
        node0.generate(101)  # Need 101 blocks for coinbase maturity
        balance0 = node0.getbalance()
        self.assert_greater_than(balance0, Decimal(0), "Node0 should have positive balance")
        self.log_success(f"Node0 balance: {balance0}")

        # Test 2: Create addresses
        self.log_info("Test 2: Create addresses on both nodes")
        addr0 = node0.getnewaddress()
        addr1 = node1.getnewaddress()
        self.log_success(f"Node0 address: {addr0}")
        self.log_success(f"Node1 address: {addr1}")

        # Test 3: Send from node0 to node1
        self.log_info("Test 3: Send 50 INT from node0 to node1")
        send_amount = Decimal("50.0")
        txid = node0.sendtoaddress(addr1, send_amount)
        self.log_success(f"Transaction ID: {txid}")

        # Test 4: Mine transaction
        self.log_info("Test 4: Mine block to confirm transaction")
        node0.generate(1)
        self.sync_blocks()
        self.log_success("Transaction confirmed")

        # Test 5: Check balances
        self.log_info("Test 5: Verify balances")
        balance1 = node1.getbalance()
        self.assert_greater_or_equal(balance1, send_amount, "Node1 should have received coins")
        self.log_success(f"Node1 balance: {balance1}")

        # Test 6: Send back
        self.log_info("Test 6: Send 25 INT back from node1 to node0")
        return_amount = Decimal("25.0")
        txid = node1.sendtoaddress(addr0, return_amount)
        node0.generate(1)
        self.sync_blocks()
        self.log_success("Return transaction confirmed")

        # Test 7: Final balance check
        self.log_info("Test 7: Final balance verification")
        final_balance0 = node0.getbalance()
        final_balance1 = node1.getbalance()
        self.log_success(f"Final node0 balance: {final_balance0}")
        self.log_success(f"Final node1 balance: {final_balance1}")

        # Test 8: Invalid transaction (insufficient funds)
        self.log_info("Test 8: Test insufficient funds error")
        try:
            huge_amount = Decimal("999999.0")
            node1.sendtoaddress(addr0, huge_amount)
            raise AssertionError("Should have raised insufficient funds error")
        except RPCError as e:
            self.log_success(f"Correctly rejected: {e.message}")


if __name__ == '__main__':
    test = WalletTest()
    exit(test.main())
