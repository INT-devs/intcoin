#!/bin/bash
# Copyright (c) 2025 INTcoin Core (Maddison Lane)
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

# Fuzz Testing Runner

set -e

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
NC='\033[0m'

# Configuration
FUZZ_TIME=3600  # 1 hour per target
BUILD_DIR="build"
CORPUS_DIR="fuzz_corpus"
CRASHES_DIR="fuzz_crashes"

# Usage
usage() {
    cat << EOF
INTcoin Fuzz Testing Runner

Usage: $0 [OPTIONS]

Options:
    -t, --time N          Fuzz time per target in seconds (default: 3600)
    -e, --engine ENGINE   Fuzzing engine: libfuzzer or afl (default: libfuzzer)
    -j, --jobs N          Number of parallel jobs (default: 1)
    -c, --continue        Continue from existing corpus
    -h, --help            Show this help message

Examples:
    $0                    # Run all fuzz tests for 1 hour each
    $0 --time 300         # Run for 5 minutes each
    $0 --jobs 4           # Run 4 fuzzers in parallel

EOF
    exit 0
}

# Parse arguments
ENGINE="libfuzzer"
JOBS=1
CONTINUE=0

while [[ $# -gt 0 ]]; do
    case $1 in
        -t|--time)
            FUZZ_TIME="$2"
            shift 2
            ;;
        -e|--engine)
            ENGINE="$2"
            shift 2
            ;;
        -j|--jobs)
            JOBS="$2"
            shift 2
            ;;
        -c|--continue)
            CONTINUE=1
            shift
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

echo -e "${CYAN}================================${NC}"
echo -e "${CYAN}INTcoin Fuzz Testing${NC}"
echo -e "${CYAN}================================${NC}"
echo ""
echo -e "Engine: ${CYAN}$ENGINE${NC}"
echo -e "Time per target: ${CYAN}$FUZZ_TIME seconds${NC}"
echo -e "Parallel jobs: ${CYAN}$JOBS${NC}"
echo ""

# Check if build directory exists
if [ ! -d "$BUILD_DIR" ]; then
    echo -e "${RED}Error: Build directory not found${NC}"
    echo -e "${YELLOW}Run: mkdir build && cd build && cmake .. -DENABLE_FUZZ_TESTING=ON && make${NC}"
    exit 1
fi

cd "$BUILD_DIR"

# Create corpus and crash directories
mkdir -p "../$CORPUS_DIR"
mkdir -p "../$CRASHES_DIR"

# Fuzz targets
TARGETS=(
    "fuzz_transaction"
    "fuzz_block"
    "fuzz_p2p_message"
    "fuzz_script"
    "fuzz_rpc"
)

# Function to run single fuzzer
run_fuzzer() {
    local target=$1
    local corpus_dir="../$CORPUS_DIR/$target"
    local crash_dir="../$CRASHES_DIR/$target"

    mkdir -p "$corpus_dir"
    mkdir -p "$crash_dir"

    echo -e "\n${CYAN}Fuzzing $target...${NC}"

    if [ "$ENGINE" = "libfuzzer" ]; then
        ./"$target" \
            -max_total_time="$FUZZ_TIME" \
            -print_final_stats=1 \
            -artifact_prefix="$crash_dir/" \
            "$corpus_dir" \
            2>&1 | tee "../fuzz_${target}.log"
    elif [ "$ENGINE" = "afl" ]; then
        afl-fuzz \
            -i "$corpus_dir" \
            -o "$crash_dir" \
            -t 1000 \
            -- ./"$target" @@
    fi

    echo -e "${GREEN}✓ Completed $target${NC}"
}

# Run fuzzers
if [ "$JOBS" -eq 1 ]; then
    # Sequential execution
    for target in "${TARGETS[@]}"; do
        if [ -f "$target" ]; then
            run_fuzzer "$target"
        else
            echo -e "${YELLOW}Warning: $target not found, skipping${NC}"
        fi
    done
else
    # Parallel execution
    echo -e "${CYAN}Running $JOBS fuzzers in parallel${NC}"

    parallel_count=0
    pids=()

    for target in "${TARGETS[@]}"; do
        if [ -f "$target" ]; then
            run_fuzzer "$target" &
            pids+=($!)
            ((parallel_count++))

            if [ "$parallel_count" -ge "$JOBS" ]; then
                # Wait for one to finish
                wait -n
                ((parallel_count--))
            fi
        fi
    done

    # Wait for remaining
    for pid in "${pids[@]}"; do
        wait "$pid"
    done
fi

# Summary
echo ""
echo -e "${CYAN}================================${NC}"
echo -e "${CYAN}Fuzzing Complete${NC}"
echo -e "${CYAN}================================${NC}"
echo ""

# Check for crashes
CRASHES_FOUND=0
for target in "${TARGETS[@]}"; do
    crash_dir="../$CRASHES_DIR/$target"
    if [ -d "$crash_dir" ]; then
        crash_count=$(find "$crash_dir" -name "crash-*" -o -name "leak-*" | wc -l)
        if [ "$crash_count" -gt 0 ]; then
            echo -e "${RED}✗ $target: $crash_count crashes found${NC}"
            ((CRASHES_FOUND+=crash_count))
        else
            echo -e "${GREEN}✓ $target: No crashes${NC}"
        fi
    fi
done

echo ""
if [ "$CRASHES_FOUND" -gt 0 ]; then
    echo -e "${RED}Total crashes found: $CRASHES_FOUND${NC}"
    echo -e "${YELLOW}Review crashes in: $CRASHES_DIR/${NC}"
    exit 1
else
    echo -e "${GREEN}No crashes found!${NC}"
    exit 0
fi
