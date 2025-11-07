#!/bin/bash
# Copyright (c) 2025 INTcoin Core (Maddison Lane)
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

# INTcoin Test Runner
# Run all test suites and generate reports

set -e

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# Configuration
BUILD_DIR="build"
VERBOSE=0
INTEGRATION=0
COVERAGE=0

# Usage
usage() {
    cat << EOF
INTcoin Test Runner

Usage: $0 [OPTIONS]

Options:
    -v, --verbose         Verbose output
    -i, --integration     Run integration tests (requires services)
    -c, --coverage        Generate coverage report (requires gcov/lcov)
    -b, --build-dir DIR   Build directory (default: build)
    -h, --help            Show this help message

Examples:
    $0                    # Run all unit tests
    $0 --verbose          # Run with verbose output
    $0 --integration      # Include integration tests
    $0 --coverage         # Generate coverage report

EOF
    exit 0
}

# Parse arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        -v|--verbose)
            VERBOSE=1
            shift
            ;;
        -i|--integration)
            INTEGRATION=1
            shift
            ;;
        -c|--coverage)
            COVERAGE=1
            shift
            ;;
        -b|--build-dir)
            BUILD_DIR="$2"
            shift 2
            ;;
        -h|--help)
            usage
            ;;
        *)
            echo "Unknown option: $1"
            usage
            ;;
    esac
done

# Print header
echo -e "${CYAN}================================${NC}"
echo -e "${CYAN}INTcoin Test Suite${NC}"
echo -e "${CYAN}================================${NC}"
echo ""

# Check if build directory exists
if [ ! -d "$BUILD_DIR" ]; then
    echo -e "${RED}Error: Build directory '$BUILD_DIR' not found${NC}"
    echo -e "${YELLOW}Run: mkdir build && cd build && cmake .. && make${NC}"
    exit 1
fi

cd "$BUILD_DIR"

# Test suites
TESTS=(
    "crypto_tests:Cryptography"
    "blockchain_tests:Blockchain"
    "wallet_tests:Wallet"
    "network_tests:Network/P2P"
    "lightning_tests:Lightning Network"
    "tor_tests:TOR Support"
    "security_tests:Security"
)

# Counters
TOTAL_PASSED=0
TOTAL_FAILED=0
SUITES_PASSED=0
SUITES_FAILED=0

# Run each test suite
for test_info in "${TESTS[@]}"; do
    IFS=':' read -r test_name test_desc <<< "$test_info"

    echo -e "\n${CYAN}Running $test_desc Tests...${NC}"
    echo -e "${CYAN}================================${NC}"

    # Check if test executable exists
    if [ ! -f "$test_name" ]; then
        echo -e "${YELLOW}Warning: $test_name not found, skipping${NC}"
        continue
    fi

    # Run test
    if [ $VERBOSE -eq 1 ]; then
        if ./"$test_name"; then
            echo -e "${GREEN}✓ $test_desc tests passed${NC}"
            ((SUITES_PASSED++))
        else
            echo -e "${RED}✗ $test_desc tests failed${NC}"
            ((SUITES_FAILED++))
        fi
    else
        if ./"$test_name" > /tmp/"$test_name".log 2>&1; then
            echo -e "${GREEN}✓ $test_desc tests passed${NC}"
            ((SUITES_PASSED++))
        else
            echo -e "${RED}✗ $test_desc tests failed${NC}"
            echo -e "${YELLOW}See log: /tmp/$test_name.log${NC}"
            ((SUITES_FAILED++))
        fi
    fi
done

# Run integration tests if requested
if [ $INTEGRATION -eq 1 ]; then
    echo -e "\n${CYAN}Running Integration Tests...${NC}"
    echo -e "${CYAN}================================${NC}"

    # Check if TOR is running for TOR integration tests
    if command -v tor &> /dev/null; then
        if pgrep -x "tor" > /dev/null; then
            echo -e "${GREEN}✓ TOR is running${NC}"
            if [ -f "test_tor" ]; then
                ./test_tor --integration
            fi
        else
            echo -e "${YELLOW}⚠ TOR is not running, skipping TOR integration tests${NC}"
        fi
    fi
fi

# Generate coverage report if requested
if [ $COVERAGE -eq 1 ]; then
    echo -e "\n${CYAN}Generating Coverage Report...${NC}"
    echo -e "${CYAN}================================${NC}"

    if command -v lcov &> /dev/null; then
        make coverage || echo -e "${YELLOW}Coverage generation failed${NC}"
        echo -e "${GREEN}Coverage report: build/coverage/index.html${NC}"
    else
        echo -e "${YELLOW}lcov not found. Install with: apt-get install lcov${NC}"
    fi
fi

# Summary
echo ""
echo -e "${CYAN}================================${NC}"
echo -e "${CYAN}Test Summary${NC}"
echo -e "${CYAN}================================${NC}"
echo -e "Test suites passed: ${GREEN}$SUITES_PASSED${NC}"
echo -e "Test suites failed: ${RED}$SUITES_FAILED${NC}"
echo -e "${CYAN}================================${NC}"
echo ""

# Exit with error if any tests failed
if [ $SUITES_FAILED -gt 0 ]; then
    echo -e "${RED}Some tests failed!${NC}"
    exit 1
else
    echo -e "${GREEN}All tests passed!${NC}"
    exit 0
fi
