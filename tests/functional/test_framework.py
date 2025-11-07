#!/usr/bin/env python3
# Copyright (c) 2025 INTcoin Core (Maddison Lane)
# Distributed under the MIT software license

"""INTcoin Functional Test Framework"""

import os
import sys
import time
import json
import subprocess
import http.client
from typing import Optional, List, Any


class TestFramework:
    """Base class for functional tests."""

    def __init__(self):
        self.nodes = []
        self.num_nodes = 1

    def run_test(self):
        """Override with test logic."""
        raise NotImplementedError

    def log_info(self, message: str):
        """Log informational message."""
        print(f"[INFO] {message}")

    def assert_equal(self, actual, expected, message=""):
        """Assert two values are equal."""
        if actual != expected:
            raise AssertionError(f"Expected {expected}, got {actual}. {message}")

    def main(self):
        """Main test runner."""
        success = False
        try:
            self.log_info("Running test")
            self.run_test()
            self.log_info("Test passed")
            success = True
        except AssertionError as e:
            self.log_info(f"Test failed: {e}")
        except Exception as e:
            self.log_info(f"Test error: {e}")
            import traceback
            traceback.print_exc()
        
        return 0 if success else 1
