#!/bin/bash
#
# INTcoin Security Audit Script
# Performs static analysis and security checks
# OUTPUT IS CONFIDENTIAL - DO NOT COMMIT
#

AUDIT_DIR="$(cd "$(dirname "$0")" && pwd)"
REPORT_FILE="$AUDIT_DIR/security_audit_$(date +%Y%m%d_%H%M%S).md"
PROJECT_ROOT="$(dirname "$AUDIT_DIR")"

echo "# INTcoin Security Audit Report" > "$REPORT_FILE"
echo "**Generated**: $(date)" >> "$REPORT_FILE"
echo "**Auditor**: Automated Security Scanner" >> "$REPORT_FILE"
echo "**Confidentiality**: INTERNAL USE ONLY - DO NOT DISTRIBUTE" >> "$REPORT_FILE"
echo "" >> "$REPORT_FILE"

echo "Running security audit..."
echo "Report will be saved to: $REPORT_FILE"
echo ""

# Function to add section to report
add_section() {
    echo "" >> "$REPORT_FILE"
    echo "## $1" >> "$REPORT_FILE"
    echo "" >> "$REPORT_FILE"
}

# Function to check for dangerous patterns
check_pattern() {
    local pattern="$1"
    local description="$2"
    local severity="$3"

    results=$(grep -rn "$pattern" "$PROJECT_ROOT/src" "$PROJECT_ROOT/include" \
              --include="*.cpp" --include="*.h" 2>/dev/null | head -20)

    if [ -n "$results" ]; then
        echo "### [$severity] $description" >> "$REPORT_FILE"
        echo '```' >> "$REPORT_FILE"
        echo "$results" >> "$REPORT_FILE"
        echo '```' >> "$REPORT_FILE"
        echo "" >> "$REPORT_FILE"
    fi
}

# 1. DANGEROUS FUNCTIONS
add_section "1. Dangerous C/C++ Functions"
echo "Checking for use of dangerous functions..." >> "$REPORT_FILE"

check_pattern "strcpy\|strcat\|sprintf\|vsprintf\|gets" \
    "Unsafe string functions - buffer overflow risk" "HIGH"

check_pattern "memcpy\|memmove" \
    "Memory copy functions - check buffer sizes" "MEDIUM"

check_pattern "malloc\|free\|realloc\|calloc" \
    "Manual memory management - use smart pointers" "MEDIUM"

# 2. CRYPTOGRAPHIC ISSUES
add_section "2. Cryptographic Security"

check_pattern "rand(\|random(\|srand(" \
    "Weak random number generation - use crypto RNG" "CRITICAL"

check_pattern "MD5\|SHA1\|DES\|RC4" \
    "Deprecated cryptographic algorithms" "HIGH"

check_pattern "== 0.*verify\|verify.*== 0" \
    "Potential timing attack in crypto verification" "HIGH"

# 3. INPUT VALIDATION
add_section "3. Input Validation"

check_pattern "atoi\|atol\|atof" \
    "Unsafe string-to-number conversion" "MEDIUM"

check_pattern "scanf\|sscanf" \
    "Unsafe input parsing" "HIGH"

# 4. INTEGER OVERFLOW
add_section "4. Integer Overflow Risks"

check_pattern "uint.*+.*uint\|size_t.*+.*size_t" \
    "Potential integer overflow in arithmetic" "MEDIUM"

# 5. HARDCODED SECRETS
add_section "5. Hardcoded Credentials/Secrets"

check_pattern 'password.*=.*".*"\|api_key.*=.*".*"\|secret.*=.*".*"' \
    "Potential hardcoded credentials" "CRITICAL"

check_pattern "TODO.*security\|FIXME.*security\|XXX.*security" \
    "Security-related TODO comments" "MEDIUM"

# 6. RACE CONDITIONS
add_section "6. Concurrency Issues"

check_pattern "static.*=\|global.*=" \
    "Mutable global or static variables - race condition risk" "MEDIUM"

# 7. COMMAND INJECTION
add_section "7. Command Injection Risks"

check_pattern "system(\|popen(\|exec" \
    "System command execution - injection risk" "HIGH"

# 8. SQL INJECTION (if applicable)
add_section "8. Injection Vulnerabilities"

check_pattern "query.*+.*user\|sql.*+.*input" \
    "Potential SQL injection" "HIGH"

# 9. FILE OPERATIONS
add_section "9. File Operation Security"

check_pattern "fopen\|open(" \
    "File operations - check path validation" "LOW"

check_pattern "\.\." \
    "Path traversal patterns" "HIGH"

# 10. NETWORK SECURITY
add_section "10. Network Security"

check_pattern "recv.*sizeof\|send.*sizeof" \
    "Network buffer operations - check bounds" "MEDIUM"

check_pattern "SSL_CTX_set_verify.*SSL_VERIFY_NONE" \
    "Disabled SSL certificate verification" "CRITICAL"

# 11. MEMORY SAFETY
add_section "11. Memory Safety"

check_pattern "delete\[\]\|delete " \
    "Manual delete operations - use RAII" "MEDIUM"

check_pattern "new .*\[" \
    "Manual array allocation - use vectors" "MEDIUM"

# 12. CODE STATISTICS
add_section "12. Code Statistics"

echo "### Lines of Code" >> "$REPORT_FILE"
echo '```' >> "$REPORT_FILE"
find "$PROJECT_ROOT/src" "$PROJECT_ROOT/include" -name "*.cpp" -o -name "*.h" 2>/dev/null | \
    xargs wc -l 2>/dev/null | tail -1 >> "$REPORT_FILE"
echo '```' >> "$REPORT_FILE"

echo "### File Count" >> "$REPORT_FILE"
echo '```' >> "$REPORT_FILE"
echo "Header files (.h): $(find "$PROJECT_ROOT/include" -name "*.h" 2>/dev/null | wc -l)" >> "$REPORT_FILE"
echo "Source files (.cpp): $(find "$PROJECT_ROOT/src" -name "*.cpp" 2>/dev/null | wc -l)" >> "$REPORT_FILE"
echo '```' >> "$REPORT_FILE"

# 13. DEPENDENCIES
add_section "13. External Dependencies"

echo "### Third-Party Libraries" >> "$REPORT_FILE"
echo '```' >> "$REPORT_FILE"
grep -h "find_package\|find_library" "$PROJECT_ROOT/CMakeLists.txt" 2>/dev/null >> "$REPORT_FILE"
echo '```' >> "$REPORT_FILE"

# 14. COMPILER WARNINGS CHECK
add_section "14. Build Configuration"

echo "### Compiler Security Flags" >> "$REPORT_FILE"
echo '```' >> "$REPORT_FILE"
grep -h "CMAKE_CXX_FLAGS\|add_compile_options" "$PROJECT_ROOT/CMakeLists.txt" 2>/dev/null >> "$REPORT_FILE"
echo '```' >> "$REPORT_FILE"

# 15. RECOMMENDATIONS
add_section "15. Security Recommendations"

cat >> "$REPORT_FILE" << 'EOF'
### General Recommendations

1. **Use Modern C++ Features**
   - Replace raw pointers with smart pointers (`unique_ptr`, `shared_ptr`)
   - Use RAII for all resource management
   - Prefer `std::array` and `std::vector` over C-style arrays

2. **Input Validation**
   - Validate all external inputs (network, files, user input)
   - Use safe parsing functions (`std::stoi`, `std::stoul` with exception handling)
   - Implement bounds checking for all buffer operations

3. **Cryptographic Best Practices**
   - Use cryptographically secure RNG (CSPRNG)
   - Implement constant-time comparison for sensitive data
   - Regular security audits of crypto code
   - Use established libraries (liboqs, OpenSSL)

4. **Memory Safety**
   - Enable compiler sanitizers (AddressSanitizer, UBSanitizer)
   - Run valgrind for memory leak detection
   - Use static analysis tools (clang-tidy, cppcheck)

5. **Concurrency Safety**
   - Use mutexes for all shared data
   - Prefer `std::atomic` for simple counters
   - Consider thread sanitizer for race condition detection

6. **Build Security**
   - Enable all compiler warnings (`-Wall -Wextra -Werror`)
   - Use stack protection (`-fstack-protector-strong`)
   - Enable ASLR and DEP
   - Build with `-D_FORTIFY_SOURCE=2`

7. **Dependency Management**
   - Keep all dependencies up to date
   - Monitor for CVEs in third-party libraries
   - Verify checksums of downloaded dependencies

8. **Code Review**
   - All security-critical code should be peer-reviewed
   - Use automated code analysis in CI/CD
   - Regular external security audits

9. **Fuzzing**
   - Implement continuous fuzzing for parsers
   - Use AFL or libFuzzer for coverage-guided fuzzing
   - Test all network protocol handlers

10. **Secure Defaults**
    - Fail securely (deny by default)
    - Validate whitelists, not blacklists
    - Minimize attack surface
EOF

# Summary
add_section "16. Audit Summary"

echo "**Audit completed**: $(date)" >> "$REPORT_FILE"
echo "" >> "$REPORT_FILE"
echo "**Note**: This is an automated scan. Manual code review is recommended for:" >> "$REPORT_FILE"
echo "- Cryptographic implementations" >> "$REPORT_FILE"
echo "- Network protocol handlers" >> "$REPORT_FILE"
echo "- Transaction validation logic" >> "$REPORT_FILE"
echo "- Consensus algorithms" >> "$REPORT_FILE"
echo "" >> "$REPORT_FILE"
echo "**Confidentiality**: This report contains sensitive security information." >> "$REPORT_FILE"
echo "Do not commit to version control or share publicly." >> "$REPORT_FILE"

echo ""
echo "✓ Security audit complete!"
echo "Report saved to: $REPORT_FILE"
echo ""
echo "⚠️  IMPORTANT: This report is confidential and should NOT be committed to git!"
echo "✓ Already added to .gitignore: security_audit/"
