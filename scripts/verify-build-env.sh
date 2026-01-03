#!/bin/bash
# INTcoin v1.3.0-beta - Build Environment Verification Script
# This script verifies that all required dependencies and tools are available

set -e

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Counters
CHECKS_PASSED=0
CHECKS_FAILED=0
CHECKS_WARNING=0

print_header() {
    echo ""
    echo "=================================================================="
    echo -e "${BLUE}$1${NC}"
    echo "=================================================================="
    echo ""
}

check_pass() {
    echo -e "${GREEN}✓${NC} $1"
    ((CHECKS_PASSED++))
}

check_fail() {
    echo -e "${RED}✗${NC} $1"
    ((CHECKS_FAILED++))
}

check_warn() {
    echo -e "${YELLOW}⚠${NC} $1"
    ((CHECKS_WARNING++))
}

check_command() {
    local cmd=$1
    local name=$2
    local required=$3

    if command -v "$cmd" &> /dev/null; then
        local version=$($cmd --version 2>&1 | head -1)
        check_pass "$name found: $version"
        return 0
    else
        if [ "$required" = "required" ]; then
            check_fail "$name not found (required)"
            return 1
        else
            check_warn "$name not found (optional)"
            return 0
        fi
    fi
}

print_header "INTcoin v1.3.0-beta Build Environment Verification"

echo "Platform: $(uname -s) $(uname -r)"
echo "Architecture: $(uname -m)"
echo "Date: $(date)"
echo ""

# ============================================================================
# System Requirements
# ============================================================================
print_header "System Requirements"

# Check 64-bit system
if [ "$(uname -m)" = "x86_64" ] || [ "$(uname -m)" = "arm64" ] || [ "$(uname -m)" = "aarch64" ]; then
    check_pass "64-bit system detected"
else
    check_fail "32-bit system detected - INTcoin requires 64-bit"
fi

# Check available memory
total_mem=$(free -g 2>/dev/null | awk '/^Mem:/{print $2}' || sysctl -n hw.memsize 2>/dev/null | awk '{print int($1/1024/1024/1024)}' || echo "unknown")
if [ "$total_mem" != "unknown" ]; then
    if [ "$total_mem" -ge 4 ]; then
        check_pass "Sufficient RAM: ${total_mem}GB"
    elif [ "$total_mem" -ge 2 ]; then
        check_warn "Low RAM: ${total_mem}GB (4GB+ recommended)"
    else
        check_fail "Insufficient RAM: ${total_mem}GB (minimum 2GB)"
    fi
else
    check_warn "Could not determine RAM amount"
fi

# Check available disk space
available_disk=$(df -BG . 2>/dev/null | tail -1 | awk '{print $4}' | sed 's/G//' || df -g . 2>/dev/null | tail -1 | awk '{print $4}' || echo "unknown")
if [ "$available_disk" != "unknown" ]; then
    if [ "$available_disk" -ge 20 ]; then
        check_pass "Sufficient disk space: ${available_disk}GB"
    elif [ "$available_disk" -ge 10 ]; then
        check_warn "Low disk space: ${available_disk}GB (20GB+ recommended)"
    else
        check_fail "Insufficient disk space: ${available_disk}GB (minimum 10GB)"
    fi
else
    check_warn "Could not determine available disk space"
fi

# ============================================================================
# Build Tools
# ============================================================================
print_header "Build Tools"

check_command "cmake" "CMake" "required"
if command -v cmake &> /dev/null; then
    cmake_version=$(cmake --version | head -1 | awk '{print $3}')
    cmake_major=$(echo "$cmake_version" | cut -d. -f1)
    cmake_minor=$(echo "$cmake_version" | cut -d. -f2)

    if [ "$cmake_major" -ge 3 ] && [ "$cmake_minor" -ge 28 ]; then
        check_pass "CMake version $cmake_version meets requirement (≥3.28)"
    else
        check_fail "CMake version $cmake_version too old (need ≥3.28)"
    fi
fi

check_command "make" "GNU Make" "required"
check_command "ninja" "Ninja" "optional"
check_command "git" "Git" "required"
check_command "pkg-config" "pkg-config" "required"

# ============================================================================
# C++ Compiler
# ============================================================================
print_header "C++ Compiler"

# Check for GCC
if command -v g++ &> /dev/null; then
    gcc_version=$(g++ --version | head -1 | grep -oE '[0-9]+\.[0-9]+' | head -1)
    gcc_major=$(echo "$gcc_version" | cut -d. -f1)

    if [ "$gcc_major" -ge 13 ]; then
        check_pass "GCC version $gcc_version supports C++23"
    else
        check_warn "GCC version $gcc_version may not fully support C++23 (need ≥13)"
    fi
fi

# Check for Clang
if command -v clang++ &> /dev/null; then
    clang_version=$(clang++ --version | head -1 | grep -oE '[0-9]+\.[0-9]+' | head -1)
    clang_major=$(echo "$clang_version" | cut -d. -f1)

    if [ "$clang_major" -ge 16 ]; then
        check_pass "Clang version $clang_version supports C++23"
    else
        check_warn "Clang version $clang_version may not fully support C++23 (need ≥16)"
    fi
fi

# Ensure at least one compiler
if ! command -v g++ &> /dev/null && ! command -v clang++ &> /dev/null; then
    check_fail "No C++ compiler found (need GCC 13+ or Clang 16+)"
fi

# ============================================================================
# Dependencies
# ============================================================================
print_header "Required Dependencies"

# Qt6
if pkg-config --exists Qt6Core 2>/dev/null || [ -d "/usr/local/lib/qt6" ] || [ -d "/opt/homebrew/opt/qt@6" ]; then
    if pkg-config --exists Qt6Core; then
        qt_version=$(pkg-config --modversion Qt6Core)
        check_pass "Qt6 found: $qt_version"
    else
        check_pass "Qt6 found (version check unavailable)"
    fi
else
    check_fail "Qt6 not found"
fi

# Boost
if pkg-config --exists boost 2>/dev/null; then
    boost_version=$(pkg-config --modversion boost)
    check_pass "Boost found: $boost_version"
else
    check_warn "Boost not found via pkg-config (may still be available)"
fi

# OpenSSL
if pkg-config --exists openssl 2>/dev/null; then
    openssl_version=$(pkg-config --modversion openssl)
    check_pass "OpenSSL found: $openssl_version"
else
    check_warn "OpenSSL not found via pkg-config"
fi

# RocksDB
if pkg-config --exists rocksdb 2>/dev/null; then
    rocksdb_version=$(pkg-config --modversion rocksdb)
    check_pass "RocksDB found: $rocksdb_version"
else
    check_warn "RocksDB not found via pkg-config (may still be available)"
fi

# libevent
if pkg-config --exists libevent 2>/dev/null; then
    libevent_version=$(pkg-config --modversion libevent)
    check_pass "libevent found: $libevent_version"
else
    check_warn "libevent not found via pkg-config"
fi

# ZeroMQ
if pkg-config --exists libzmq 2>/dev/null; then
    zmq_version=$(pkg-config --modversion libzmq)
    check_pass "ZeroMQ found: $zmq_version"
else
    check_warn "ZeroMQ not found via pkg-config"
fi

# ============================================================================
# Optional Dependencies
# ============================================================================
print_header "Optional Dependencies"

check_command "ccache" "ccache (build cache)" "optional"
check_command "clang-tidy" "clang-tidy (static analysis)" "optional"
check_command "cppcheck" "cppcheck (static analysis)" "optional"
check_command "valgrind" "valgrind (memory check)" "optional"
check_command "doxygen" "Doxygen (documentation)" "optional"

# ============================================================================
# Platform-Specific
# ============================================================================
print_header "Platform-Specific Checks"

case "$(uname -s)" in
    Darwin)
        echo "Platform: macOS"

        # Check Xcode Command Line Tools
        if xcode-select -p &> /dev/null; then
            check_pass "Xcode Command Line Tools installed"
        else
            check_fail "Xcode Command Line Tools not installed (run: xcode-select --install)"
        fi

        # Check Homebrew
        if command -v brew &> /dev/null; then
            check_pass "Homebrew found"
        else
            check_warn "Homebrew not found (recommended for dependency management)"
        fi
        ;;

    Linux)
        echo "Platform: Linux"

        # Check distribution
        if [ -f /etc/os-release ]; then
            . /etc/os-release
            echo "Distribution: $NAME $VERSION"
            check_pass "Linux distribution detected: $NAME"
        else
            check_warn "Could not detect Linux distribution"
        fi

        # Check for package managers
        if command -v apt &> /dev/null; then
            check_pass "APT package manager found"
        elif command -v dnf &> /dev/null; then
            check_pass "DNF package manager found"
        elif command -v pacman &> /dev/null; then
            check_pass "Pacman package manager found"
        elif command -v zypper &> /dev/null; then
            check_pass "Zypper package manager found"
        else
            check_warn "No recognized package manager found"
        fi
        ;;

    FreeBSD)
        echo "Platform: FreeBSD"

        # Check FreeBSD version
        freebsd_version=$(uname -r | cut -d- -f1)
        echo "FreeBSD version: $freebsd_version"

        if [ "${freebsd_version%%.*}" -ge 13 ]; then
            check_pass "FreeBSD version $freebsd_version is supported"
        else
            check_fail "FreeBSD version $freebsd_version is too old (need ≥13.0)"
        fi

        # Check for pkg
        if command -v pkg &> /dev/null; then
            check_pass "pkg package manager found"
        else
            check_fail "pkg not found (install with: pkg bootstrap)"
        fi
        ;;

    MINGW*|MSYS*|CYGWIN*)
        echo "Platform: Windows (MSYS2/MinGW)"

        if command -v pacman &> /dev/null; then
            check_pass "MSYS2/MinGW environment detected"
        else
            check_warn "MSYS2 package manager not found"
        fi
        ;;

    *)
        check_warn "Unknown platform: $(uname -s)"
        ;;
esac

# ============================================================================
# Summary
# ============================================================================
print_header "Verification Summary"

echo ""
echo "Checks passed:  $CHECKS_PASSED"
echo "Checks warned:  $CHECKS_WARNING"
echo "Checks failed:  $CHECKS_FAILED"
echo ""

if [ $CHECKS_FAILED -eq 0 ]; then
    echo -e "${GREEN}✓ Build environment is ready!${NC}"
    echo ""
    echo "Next steps:"
    echo "  1. mkdir -p build && cd build"
    echo "  2. cmake .. -DCMAKE_BUILD_TYPE=Release"
    echo "  3. cmake --build . -j\$(nproc || sysctl -n hw.ncpu)"
    echo "  4. ctest --output-on-failure"
    echo ""
    exit 0
elif [ $CHECKS_FAILED -le 2 ]; then
    echo -e "${YELLOW}⚠ Build environment has warnings${NC}"
    echo "You may be able to build, but some dependencies are missing."
    echo "Review the failed checks above and install missing components."
    echo ""
    exit 1
else
    echo -e "${RED}✗ Build environment is not ready${NC}"
    echo "Please install missing dependencies before building."
    echo "See docs/INSTALL_LINUX.md or docs/INSTALL_FREEBSD.md for instructions."
    echo ""
    exit 2
fi
