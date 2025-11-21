#!/usr/bin/env python3
# Copyright (c) 2025 INTcoin Core (Maddison Lane)
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
#
# RPC Interface Test

"""Test RPC interface and commands."""

from test_framework import TestFramework, RPCError
from decimal import Decimal


class RPCTest(TestFramework):
    """Test RPC interface."""

    def __init__(self):
        super().__init__()
        self.num_nodes = 1

    def run_test(self):
        """Run the test."""
        node = self.nodes[0]

        # Test 1: getblockchaininfo
        self.log_info("Test 1: Test getblockchaininfo RPC")
        info = node.rpc("getblockchaininfo")
        self.assert_equal(info["chain"], "regtest", "Should be regtest chain")
        self.assert_equal(info["blocks"], 0, "Should start with 0 blocks")
        self.log_success("getblockchaininfo works correctly")

        # Test 2: getnetworkinfo
        self.log_info("Test 2: Test getnetworkinfo RPC")
        netinfo = node.rpc("getnetworkinfo")
        self.assert_equal("version" in netinfo, True, "Should have version field")
        self.assert_equal("protocolversion" in netinfo, True, "Should have protocol version")
        self.log_success("getnetworkinfo works correctly")

        # Test 3: getwalletinfo
        self.log_info("Test 3: Test getwalletinfo RPC")
        walletinfo = node.rpc("getwalletinfo")
        self.assert_equal("balance" in walletinfo, True, "Should have balance field")
        self.log_success("getwalletinfo works correctly")

        # Test 4: Generate blocks
        self.log_info("Test 4: Test generate RPC")
        node.generate(10)
        height = node.getblockcount()
        self.assert_equal(height, 10, "Should have 10 blocks")
        self.log_success("generate RPC works correctly")

        # Test 5: getblock
        self.log_info("Test 5: Test getblock RPC")
        blockhash = node.rpc("getblockhash", [1])
        block = node.rpc("getblock", [blockhash])
        self.assert_equal(block["height"], 1, "Block height should be 1")
        self.assert_equal(block["hash"], blockhash, "Block hash should match")
        self.log_success("getblock RPC works correctly")

        # Test 6: gettransaction (after generating address and sending)
        self.log_info("Test 6: Test transaction RPCs")
        addr = node.getnewaddress()
        node.generate(100)  # Mine to maturity
        txid = node.sendtoaddress(addr, Decimal("10.0"))
        tx = node.rpc("gettransaction", [txid])
        self.assert_equal(tx["txid"], txid, "Transaction ID should match")
        self.log_success("gettransaction RPC works correctly")

        # Test 7: getmininginfo
        self.log_info("Test 7: Test getmininginfo RPC")
        mininginfo = node.rpc("getmininginfo")
        self.assert_equal("difficulty" in mininginfo, True, "Should have difficulty")
        self.assert_equal("networkhashps" in mininginfo, True, "Should have network hashrate")
        self.log_success("getmininginfo RPC works correctly")

        # Test 8: Invalid RPC call
        self.log_info("Test 8: Test invalid RPC call")
        try:
            node.rpc("nonexistentmethod")
            raise AssertionError("Should have raised RPC error")
        except RPCError as e:
            self.log_success(f"Correctly rejected invalid method: {e.message}")

        # Test 9: help command
        self.log_info("Test 9: Test help RPC")
        help_text = node.rpc("help")
        self.assert_greater_than(len(help_text), 0, "Help should return text")
        self.log_success("help RPC works correctly")

        # Test 10: getpeerinfo
        self.log_info("Test 10: Test getpeerinfo RPC")
        peers = node.rpc("getpeerinfo")
        self.assert_equal(isinstance(peers, list), True, "Should return list of peers")
        self.log_success(f"getpeerinfo works correctly ({len(peers)} peers)")


if __name__ == '__main__':
    test = RPCTest()
    exit(test.main())
