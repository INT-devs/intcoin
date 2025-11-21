#!/usr/bin/env python3
# Copyright (c) 2025 INTcoin Core (Maddison Lane)
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
#
# INTcoin Functional Test Framework

"""
INTcoin Functional Test Framework

Provides base classes and utilities for writing functional tests
for the INTcoin daemon, wallet, and RPC interface.
"""

import os
import sys
import time
import json
import subprocess
import http.client
import tempfile
import shutil
import signal
from pathlib import Path
from typing import Optional, List, Any, Dict
from decimal import Decimal


class RPCError(Exception):
    """Exception raised when RPC call fails."""
    def __init__(self, code: int, message: str):
        self.code = code
        self.message = message
        super().__init__(f"RPC Error {code}: {message}")


class TestNode:
    """Represents a single INTcoin node for testing."""

    def __init__(self, index: int, datadir: Path, binary: Path, rpcport: int = 9332):
        self.index = index
        self.datadir = datadir
        self.binary = binary
        self.rpcport = rpcport
        self.process = None
        self.rpcuser = f"testuser{index}"
        self.rpcpass = f"testpass{index}"

    def start(self, extra_args: List[str] = None) -> bool:
        """Start the node."""
        if extra_args is None:
            extra_args = []

        args = [
            str(self.binary),
            f"-datadir={self.datadir}",
            f"-rpcport={self.rpcport}",
            f"-rpcuser={self.rpcuser}",
            f"-rpcpassword={self.rpcpass}",
            "-regtest",
            "-server",
            "-daemon=0",
            "-printtoconsole=0",
        ] + extra_args

        try:
            self.process = subprocess.Popen(
                args,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True
            )
            # Wait for RPC to be ready
            time.sleep(2)
            return self.is_running()
        except Exception as e:
            print(f"Failed to start node {self.index}: {e}")
            return False

    def stop(self) -> bool:
        """Stop the node."""
        if self.process:
            try:
                self.rpc("stop")
                self.process.wait(timeout=10)
            except:
                self.process.terminate()
                try:
                    self.process.wait(timeout=5)
                except:
                    self.process.kill()
            self.process = None
        return True

    def is_running(self) -> bool:
        """Check if node is running."""
        if not self.process:
            return False
        return self.process.poll() is None

    def rpc(self, method: str, params: List[Any] = None) -> Any:
        """Make RPC call to the node."""
        if params is None:
            params = []

        payload = {
            "jsonrpc": "2.0",
            "id": "test",
            "method": method,
            "params": params
        }

        try:
            conn = http.client.HTTPConnection(f"localhost:{self.rpcport}")
            auth = f"{self.rpcuser}:{self.rpcpass}".encode()
            import base64
            auth_header = "Basic " + base64.b64encode(auth).decode()

            conn.request(
                "POST",
                "/",
                json.dumps(payload),
                {"Content-Type": "application/json", "Authorization": auth_header}
            )

            response = conn.getresponse()
            data = json.loads(response.read().decode())

            if "error" in data and data["error"]:
                raise RPCError(data["error"]["code"], data["error"]["message"])

            return data.get("result")
        except http.client.HTTPException as e:
            raise RPCError(-1, f"HTTP error: {e}")
        finally:
            conn.close()

    def generate(self, nblocks: int) -> List[str]:
        """Generate blocks."""
        return self.rpc("generate", [nblocks])

    def getblockcount(self) -> int:
        """Get current block height."""
        return self.rpc("getblockcount")

    def getbalance(self) -> Decimal:
        """Get wallet balance."""
        return Decimal(str(self.rpc("getbalance")))

    def sendtoaddress(self, address: str, amount: Decimal) -> str:
        """Send coins to address."""
        return self.rpc("sendtoaddress", [address, float(amount)])

    def getnewaddress(self) -> str:
        """Get new address."""
        return self.rpc("getnewaddress")


class TestFramework:
    """Base class for functional tests."""

    def __init__(self):
        self.nodes: List[TestNode] = []
        self.num_nodes = 1
        self.datadir = None
        self.binary = None
        self.setup_clean_chain = True

    def setup_nodes(self):
        """Setup test nodes."""
        if not self.binary:
            # Try to find binary
            build_dir = Path(__file__).parent.parent.parent / "build"
            binary_path = build_dir / "intcoind"
            if not binary_path.exists():
                raise FileNotFoundError(f"intcoind binary not found at {binary_path}")
            self.binary = binary_path

        # Create temporary data directory
        self.datadir = Path(tempfile.mkdtemp(prefix="intcoin_test_"))

        # Create nodes
        for i in range(self.num_nodes):
            node_datadir = self.datadir / f"node{i}"
            node_datadir.mkdir(parents=True, exist_ok=True)
            node = TestNode(i, node_datadir, self.binary, 9332 + i)
            self.nodes.append(node)

        # Start all nodes
        for node in self.nodes:
            if not node.start():
                raise RuntimeError(f"Failed to start node {node.index}")

        self.log_info(f"Started {len(self.nodes)} node(s)")

    def stop_nodes(self):
        """Stop all test nodes."""
        for node in self.nodes:
            node.stop()
        self.nodes = []

    def cleanup(self):
        """Cleanup test environment."""
        self.stop_nodes()
        if self.datadir and self.datadir.exists():
            shutil.rmtree(self.datadir)

    def run_test(self):
        """Override with test logic."""
        raise NotImplementedError

    def log_info(self, message: str):
        """Log informational message."""
        print(f"[INFO] {message}")

    def log_success(self, message: str):
        """Log success message."""
        print(f"[✓] {message}")

    def log_error(self, message: str):
        """Log error message."""
        print(f"[✗] {message}")

    def assert_equal(self, actual, expected, message=""):
        """Assert two values are equal."""
        if actual != expected:
            raise AssertionError(f"Expected {expected}, got {actual}. {message}")

    def assert_not_equal(self, actual, expected, message=""):
        """Assert two values are not equal."""
        if actual == expected:
            raise AssertionError(f"Expected not {expected}, got {actual}. {message}")

    def assert_greater_than(self, actual, expected, message=""):
        """Assert actual > expected."""
        if not actual > expected:
            raise AssertionError(f"Expected {actual} > {expected}. {message}")

    def assert_greater_or_equal(self, actual, expected, message=""):
        """Assert actual >= expected."""
        if not actual >= expected:
            raise AssertionError(f"Expected {actual} >= {expected}. {message}")

    def assert_raises(self, exception_type, callable_obj, *args, **kwargs):
        """Assert that callable raises exception."""
        try:
            callable_obj(*args, **kwargs)
            raise AssertionError(f"Expected {exception_type.__name__} but no exception was raised")
        except exception_type:
            pass  # Expected

    def assert_raises_rpc_error(self, code: int, message_pattern: str, callable_obj, *args, **kwargs):
        """Assert that RPC call raises specific error."""
        try:
            callable_obj(*args, **kwargs)
            raise AssertionError(f"Expected RPC error {code} but no error was raised")
        except RPCError as e:
            if e.code != code:
                raise AssertionError(f"Expected error code {code}, got {e.code}")
            if message_pattern and message_pattern not in e.message:
                raise AssertionError(f"Expected message containing '{message_pattern}', got '{e.message}'")

    def sync_blocks(self, timeout: int = 60):
        """Wait for all nodes to sync to same block height."""
        start_time = time.time()
        while time.time() - start_time < timeout:
            heights = [node.getblockcount() for node in self.nodes]
            if len(set(heights)) == 1:
                return True
            time.sleep(0.5)
        raise TimeoutError("Nodes failed to sync blocks")

    def wait_for_balance(self, node: TestNode, expected: Decimal, timeout: int = 30) -> bool:
        """Wait for node to reach expected balance."""
        start_time = time.time()
        while time.time() - start_time < timeout:
            balance = node.getbalance()
            if balance >= expected:
                return True
            time.sleep(0.5)
        return False

    def main(self):
        """Main test runner."""
        success = False
        try:
            self.log_info("Setting up test environment")
            self.setup_nodes()

            self.log_info("Running test")
            self.run_test()

            self.log_success("Test passed")
            success = True
        except AssertionError as e:
            self.log_error(f"Test failed: {e}")
        except Exception as e:
            self.log_error(f"Test error: {e}")
            import traceback
            traceback.print_exc()
        finally:
            self.log_info("Cleaning up")
            self.cleanup()

        return 0 if success else 1
