#!/bin/bash
# Copyright (c) 2025 INTcoin Core (Maddison Lane)
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
#
# Run all functional tests

set -e

# Colors
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${YELLOW}======================================${NC}"
echo -e "${YELLOW}INTcoin Functional Tests${NC}"
echo -e "${YELLOW}======================================${NC}"
echo ""

# Check if binary exists
BINARY="../build/intcoind"
if [ ! -f "../../build/intcoind" ]; then
    echo -e "${RED}Error: intcoind binary not found${NC}"
    echo -e "${YELLOW}Please build the project first:${NC}"
    echo -e "  ./build.sh"
    exit 1
fi

echo -e "${GREEN}Found intcoind binary${NC}"
echo ""

# Track results
PASSED=0
FAILED=0
FAILED_TESTS=()

# Run each test
for test in test_*.py; do
    echo -e "${YELLOW}Running $test...${NC}"
    if python3 "$test"; then
        echo -e "${GREEN}✓ PASSED: $test${NC}"
        ((PASSED++))
    else
        echo -e "${RED}✗ FAILED: $test${NC}"
        ((FAILED++))
        FAILED_TESTS+=("$test")
    fi
    echo ""
done

# Print summary
echo -e "${YELLOW}======================================${NC}"
echo -e "${YELLOW}Test Summary${NC}"
echo -e "${YELLOW}======================================${NC}"
echo -e "${GREEN}Passed: $PASSED${NC}"
echo -e "${RED}Failed: $FAILED${NC}"

if [ $FAILED -gt 0 ]; then
    echo ""
    echo -e "${RED}Failed tests:${NC}"
    for test in "${FAILED_TESTS[@]}"; do
        echo -e "  ${RED}- $test${NC}"
    done
    exit 1
else
    echo ""
    echo -e "${GREEN}All tests passed!${NC}"
    exit 0
fi
