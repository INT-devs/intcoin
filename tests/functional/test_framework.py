#!/usr/bin/env python3
# Copyright (c) 2025 INTcoin Core (Maddison Lane)
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

"""
INTcoin Functional Test Framework

Provides utilities for running functional tests with multiple nodes,
RPC communication, and blockchain validation.
"""

import subprocess
import time
import os
import json
import requests
from typing import List, Dict, Optional, Any

class TestNode:
    """Represents a single INTcoin node for testing"""

    def __init__(self, index: int, datadir: str, port: int, rpc_port: int):
        self.index = index
        self.datadir = datadir
        self.port = port
        self.rpc_port = rpc_port
        self.process = None
        self.rpc_user = "test"
        self.rpc_password = "test123"

    def start(self, extra_args: List[str] = None):
        """Start the node"""
        if extra_args is None:
            extra_args = []

        args = [
            "./intcoind",
            f"-datadir={self.datadir}",
            f"-port={self.port}",
            f"-rpcport={self.rpc_port}",
            f"-rpcuser={self.rpc_user}",
            f"-rpcpassword={self.rpc_password}",
            "-server=1",
            "-regtest=1",
            "-debug=1"
        ] + extra_args

        self.process = subprocess.Popen(args)

        # Wait for RPC to be ready
        self.wait_for_rpc()

    def stop(self):
        """Stop the node"""
        if self.process:
            try:
                self.rpc("stop")
                self.process.wait(timeout=10)
            except:
                self.process.kill()
                self.process.wait()
            self.process = None

    def wait_for_rpc(self, timeout: int = 60):
        """Wait for RPC interface to be ready"""
        start_time = time.time()
        while time.time() - start_time < timeout:
            try:
                self.rpc("getblockchaininfo")
                return
            except:
                time.sleep(0.1)
        raise Exception(f"Node {self.index} RPC not ready after {timeout}s")

    def rpc(self, method: str, params: List[Any] = None) -> Any:
        """Make an RPC call to the node"""
        if params is None:
            params = []

        url = f"http://127.0.0.1:{self.rpc_port}"
        headers = {"content-type": "application/json"}
        payload = {
            "method": method,
            "params": params,
            "jsonrpc": "2.0",
            "id": 0
        }

        response = requests.post(
            url,
            data=json.dumps(payload),
            headers=headers,
            auth=(self.rpc_user, self.rpc_password)
        )

        result = response.json()
        if "error" in result and result["error"]:
            raise Exception(f"RPC error: {result['error']}")

        return result.get("result")

    def generate(self, num_blocks: int) -> List[str]:
        """Generate blocks"""
        return self.rpc("generate", [num_blocks])

    def get_blockchain_info(self) -> Dict:
        """Get blockchain info"""
        return self.rpc("getblockchaininfo")

    def get_balance(self) -> float:
        """Get wallet balance"""
        return self.rpc("getbalance")

    def send_to_address(self, address: str, amount: float) -> str:
        """Send coins to address"""
        return self.rpc("sendtoaddress", [address, amount])

    def get_new_address(self, label: str = "") -> str:
        """Get new address"""
        return self.rpc("getnewaddress", [label])

    def get_peer_info(self) -> List[Dict]:
        """Get connected peer information"""
        return self.rpc("getpeerinfo")

    def add_node(self, node_addr: str, command: str = "add"):
        """Add a node connection"""
        self.rpc("addnode", [node_addr, command])


class TestFramework:
    """Base class for functional tests"""

    def __init__(self):
        self.nodes: List[TestNode] = []
        self.test_dir = "/tmp/intcoin_test"
        self.num_nodes = 4
        self.base_port = 19000
        self.base_rpc_port = 19100

    def setup_chain(self):
        """Setup the blockchain (called once before all tests)"""
        # Create test directory
        os.makedirs(self.test_dir, exist_ok=True)

    def setup_nodes(self):
        """Setup test nodes"""
        for i in range(self.num_nodes):
            datadir = os.path.join(self.test_dir, f"node{i}")
            os.makedirs(datadir, exist_ok=True)

            node = TestNode(
                index=i,
                datadir=datadir,
                port=self.base_port + i,
                rpc_port=self.base_rpc_port + i
            )

            self.nodes.append(node)
            node.start()

        # Connect nodes in a chain
        self.connect_nodes()

    def connect_nodes(self):
        """Connect all nodes together"""
        for i in range(len(self.nodes) - 1):
            self.nodes[i].add_node(
                f"127.0.0.1:{self.base_port + i + 1}",
                "add"
            )

    def disconnect_nodes(self):
        """Disconnect all nodes"""
        for node in self.nodes:
            for peer in node.get_peer_info():
                node.rpc("disconnectnode", [peer["addr"]])

    def sync_all(self, timeout: int = 60):
        """Wait for all nodes to sync"""
        start_time = time.time()

        while time.time() - start_time < timeout:
            heights = [node.get_blockchain_info()["blocks"] for node in self.nodes]
            if len(set(heights)) == 1:
                return
            time.sleep(0.5)

        raise Exception("Nodes failed to sync")

    def stop_nodes(self):
        """Stop all nodes"""
        for node in self.nodes:
            node.stop()

    def cleanup(self):
        """Clean up test environment"""
        import shutil
        if os.path.exists(self.test_dir):
            shutil.rmtree(self.test_dir)

    def run_test(self):
        """Override this with test logic"""
        raise NotImplementedError("Test must implement run_test()")

    def main(self):
        """Main test execution"""
        print(f"\n{'='*60}")
        print(f"Running: {self.__class__.__name__}")
        print(f"{'='*60}\n")

        try:
            self.setup_chain()
            self.setup_nodes()
            self.run_test()
            print(f"\n✓ {self.__class__.__name__} passed\n")
            return True
        except Exception as e:
            print(f"\n✗ {self.__class__.__name__} failed: {e}\n")
            import traceback
            traceback.print_exc()
            return False
        finally:
            self.stop_nodes()
            self.cleanup()


# Utility functions

def assert_equal(a, b, message=""):
    """Assert two values are equal"""
    if a != b:
        raise AssertionError(f"{message}: {a} != {b}")

def assert_greater_than(a, b, message=""):
    """Assert a > b"""
    if not a > b:
        raise AssertionError(f"{message}: {a} <= {b}")

def assert_raises(exception, func, *args, **kwargs):
    """Assert function raises specific exception"""
    try:
        func(*args, **kwargs)
        raise AssertionError(f"Expected {exception.__name__} but no exception was raised")
    except exception:
        pass
    except Exception as e:
        raise AssertionError(f"Expected {exception.__name__} but got {type(e).__name__}: {e}")

def wait_until(predicate, timeout: int = 60, poll_interval: float = 0.5):
    """Wait until predicate returns True"""
    start_time = time.time()
    while time.time() - start_time < timeout:
        if predicate():
            return
        time.sleep(poll_interval)
    raise Exception("Timeout waiting for condition")
