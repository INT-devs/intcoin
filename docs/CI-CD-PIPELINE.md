# INTcoin CI/CD Pipeline Documentation

## Overview

INTcoin uses GitLab CI/CD with comprehensive security scanning (SAST and DAST) to ensure code quality, security, and reliability throughout the development lifecycle.

## Pipeline Stages

The pipeline consists of 4 main stages:

1. **Build** - Compile binaries across multiple platforms
2. **Test** - Run comprehensive test suites
3. **Security** - Perform SAST, DAST, and other security scans
4. **Deploy** - Package and publish release artifacts

---

## Build Stage

### Platforms

- **Ubuntu 22.04** (primary, stable)
- **Ubuntu 24.04** (future compatibility)
- **Debian 12** (enterprise)

### Build Outputs

All binaries are compiled and stored as artifacts:
- `intcoind` - Full node daemon
- `intcoin-cli` - Command-line interface
- `intcoin-miner` - CPU miner
- `intcoin-qt` - Desktop wallet (GUI)
- `intcoin-faucet` - Testnet faucet server
- `tests/*` - Test executables

**Artifacts expire in:** 1 week

---

## Test Stage

### Test Suites (12 total)

| Test Suite | Description | Coverage |
|------------|-------------|----------|
| `test_crypto` | Post-quantum cryptography (Dilithium3, Kyber768, SHA3-256) | Signatures, key generation, encryption |
| `test_randomx` | RandomX PoW algorithm | Mining, hashing, difficulty |
| `test_bech32` | Address format validation | Bech32 encoding/decoding, checksums |
| `test_serialization` | Data serialization | Block/transaction serialization |
| `test_storage` | RocksDB persistence | Database operations, UTXO set |
| `test_validation` | Consensus rules | Block/transaction validation |
| `test_genesis` | Genesis block | Initial blockchain state |
| `test_network` | P2P networking | Message handling, peer discovery |
| `test_ml` | Machine learning components | Model training, inference |
| `test_wallet` | HD wallet functionality | Key derivation (BIP32/BIP39), transactions |
| `test_fuzz` | Fuzzing tests | Input validation, crash detection |
| `test_integration` | End-to-end tests | Full system integration |

### Test Reports

- **Format:** JUnit XML
- **Location:** `build/test-results/*.xml`
- **Retention:** 1 week
- **Visibility:** GitLab Test Reports UI

---

## Security Stage

### SAST (Static Application Security Testing)

#### 1. GitLab SAST (Multi-Analyzer)
- **Description:** GitLab's built-in SAST templates
- **Analyzers:** Multiple language-specific scanners
- **Report:** `gl-sast-report.json` (SARIF format)
- **Coverage:** Full codebase scan

#### 2. Semgrep
- **Description:** Modern semantic code scanner
- **Language:** C/C++ focused
- **Config:** Auto-configuration
- **Output:** SARIF format
- **Website:** https://semgrep.dev/

#### 3. Flawfinder
- **Description:** C/C++ security vulnerability scanner
- **Focus:** Buffer overflows, format strings, race conditions
- **Output:** HTML report + critical findings
- **Minimum Level:** 4 (critical issues only)
- **Website:** https://dwheeler.com/flawfinder/

#### 4. Cppcheck
- **Description:** Static analysis for C/C++
- **Checks:** All enabled (warning, style, performance, portability)
- **Output:** XML + text report
- **Website:** https://cppcheck.sourceforge.io/

#### 5. Clang-Tidy
- **Description:** C++ linter and static analyzer
- **Features:** Code smells, modernization, performance
- **Integration:** Uses CMake compile_commands.json
- **Website:** https://clang.llvm.org/extra/clang-tidy/

### Secret Detection

#### 1. GitLab Secret Detection
- **Description:** Built-in secret scanner
- **Coverage:** Credentials, API keys, tokens
- **Report:** `gl-secret-detection-report.json`

#### 2. Gitleaks
- **Description:** Advanced secret detection
- **Mode:** Full repository scan (no git required)
- **Output:** JSON report
- **Website:** https://github.com/gitleaks/gitleaks

### DAST (Dynamic Application Security Testing)

#### 1. GitLab DAST
- **Target:** Testnet RPC endpoint (http://localhost:12212)
- **Authentication:** HTTP Basic Auth
- **Mode:** Full scan enabled
- **Report:** `gl-dast-report.json`

#### 2. OWASP ZAP
- **Description:** OWASP Zed Attack Proxy
- **Scan Type:** Baseline scan
- **Target:** RPC API endpoints
- **Output:** HTML + JSON reports
- **Website:** https://www.zaproxy.org/

### Additional Security Scans

#### Dependency Scanning
- **Tool:** GitLab Dependency Scanning
- **Purpose:** Detect vulnerable dependencies
- **Report:** `gl-dependency-scanning-report.json`
- **Coverage:** C++ libraries, system packages

#### License Scanning
- **Tool:** GitLab License Scanning
- **Purpose:** License compliance verification
- **Report:** `gl-license-scanning-report.json`
- **Focus:** MIT license compatibility

#### Container Scanning
- **Tool:** Trivy
- **Mode:** Filesystem scanning
- **Severity:** HIGH, CRITICAL
- **Output:** JSON report
- **Website:** https://aquasecurity.github.io/trivy/

#### Code Quality
- **Tool:** GitLab Code Quality
- **Analyzers:** Multiple quality checkers
- **Report:** `gl-code-quality-report.json`
- **Branches:** main, develop only

---

## Security Reports

### Unified Security Summary

A consolidated markdown report is generated after all security scans complete:

**File:** `security-summary.md`

**Contents:**
- Pipeline ID and build date
- Branch and commit information
- Summary of all security scan results
- Links to detailed reports

**Retention:** 1 month

### Report Locations

All security reports are available as GitLab artifacts:

| Report Type | File Name | Format |
|-------------|-----------|--------|
| SAST (GitLab) | `gl-sast-report.json` | SARIF |
| SAST (Semgrep) | `gl-sast-report.json` | SARIF |
| Flawfinder | `flawfinder-report.html` | HTML |
| Cppcheck | `cppcheck-report.xml` | XML |
| Clang-Tidy | `clang-tidy-report.txt` | Text |
| DAST (GitLab) | `gl-dast-report.json` | JSON |
| OWASP ZAP | `zap-baseline-report.html` | HTML |
| Secrets (GitLab) | `gl-secret-detection-report.json` | JSON |
| Gitleaks | `gitleaks-report.json` | JSON |
| Dependency | `gl-dependency-scanning-report.json` | JSON |
| License | `gl-license-scanning-report.json` | JSON |
| Trivy | `trivy-report.json` | JSON |
| Code Quality | `gl-code-quality-report.json` | JSON |
| Summary | `security-summary.md` | Markdown |

---

## Deploy Stage

### Package Release

**Trigger:** Tags and main branch only

**Process:**
1. Collect all compiled binaries
2. Create tar.gz archive
3. Upload to GitLab Package Registry

**Artifact:**
- **Name:** `intcoin-$VERSION-linux-x86_64.tar.gz`
- **Contents:** intcoind, intcoin-cli, intcoin-miner, intcoin-qt, intcoin-faucet
- **Retention:** 1 month

### GitLab Package Registry

**URL:** `${CI_API_V4_URL}/projects/${CI_PROJECT_ID}/packages/generic/intcoin/${VERSION}/`

**Authentication:** JOB-TOKEN (automatic)

---

## Pipeline Configuration

### GitLab Runners

**Runner Type:** GitLab.com Free Shared Runners

The pipeline uses GitLab's free shared runners which are automatically available on GitLab.com:
- **No custom runner setup required**
- **Docker support enabled by default**
- **Automatic scaling and availability**
- **Free tier allocation:** 400 CI/CD minutes per month
- **Architecture:** Linux x86_64 with Docker executor

All jobs run on gitlab.com's shared runner fleet without requiring any `tags` configuration.

### Variables

```yaml
BUILD_TYPE: "Release"                  # CMake build type
CMAKE_BUILD_PARALLEL_LEVEL: "4"        # Parallel compilation jobs
SAST_EXCLUDED_PATHS: "build/,deps/,external/,third_party/"
SECURE_LOG_LEVEL: "info"
DOCKER_DRIVER: "overlay2"              # Docker storage driver
DOCKER_TLS_CERTDIR: "/certs"           # Docker TLS certificate directory
```

### Triggers

| Event | Stages Executed |
|-------|-----------------|
| Push to any branch | Build, Test, Security (SAST only) |
| Push to main/develop | Build, Test, Security (full), Deploy |
| Tag creation | Build, Test, Security (full), Deploy, Package |
| Merge Request | Build, Test, Security (SAST only) |

---

## Security Best Practices

### SAST Coverage

✅ **Buffer Overflows** - Flawfinder, Cppcheck
✅ **Format String Bugs** - Flawfinder
✅ **Race Conditions** - Flawfinder, Clang-Tidy
✅ **Memory Leaks** - Cppcheck, Clang-Tidy
✅ **Null Pointer Dereference** - Cppcheck
✅ **Use After Free** - Cppcheck
✅ **Integer Overflow** - Semgrep, Cppcheck
✅ **SQL Injection** - Semgrep (if applicable)
✅ **Command Injection** - Semgrep, Flawfinder
✅ **Path Traversal** - Semgrep
✅ **Insecure Crypto** - Flawfinder, Semgrep

### DAST Coverage

✅ **Authentication Bypass** - OWASP ZAP
✅ **Broken Access Control** - OWASP ZAP
✅ **Injection Attacks** - OWASP ZAP
✅ **XSS** - OWASP ZAP
✅ **Security Misconfiguration** - OWASP ZAP
✅ **Sensitive Data Exposure** - OWASP ZAP

### Secret Detection Coverage

✅ **API Keys** - Gitleaks, GitLab
✅ **Private Keys** - Gitleaks, GitLab
✅ **Passwords** - Gitleaks, GitLab
✅ **Tokens** - Gitleaks, GitLab
✅ **Certificates** - Gitleaks
✅ **AWS Credentials** - Gitleaks

---

## Viewing Results

### GitLab Security Dashboard

1. Navigate to **Security & Compliance > Vulnerability Report**
2. View all detected vulnerabilities across SAST, DAST, Dependency, Secret scans
3. Triage vulnerabilities: Dismiss, Create Issue, or Resolve
4. Track vulnerability trends over time

### Pipeline Artifacts

1. Navigate to **CI/CD > Pipelines**
2. Click on pipeline number
3. View job details
4. Download artifacts from **Browse** button

### Merge Request Security Widget

Security scan results appear automatically in Merge Requests:
- New vulnerabilities detected
- Existing vulnerabilities resolved
- Security approval rules (if configured)

---

## Failure Handling

### Critical Failures (Pipeline Fails)

- Build failures on primary platform (Ubuntu 22.04)
- Test failures in any test suite

### Non-Critical Failures (Pipeline Continues)

- Build failures on secondary platforms (Ubuntu 24.04)
- Security scan failures (tools may timeout or have false positives)
- Fuzz test failures (random nature of fuzzing)
- DAST setup failures (service dependencies)

**Rationale:** Security tools should enhance development, not block it. All security findings are still reported even if jobs are marked as `allow_failure: true`.

---

## Local Testing

### Run SAST Locally

```bash
# Semgrep
docker run --rm -v "$PWD:/src" returntocorp/semgrep semgrep scan --config=auto /src

# Flawfinder
pip install flawfinder
flawfinder src/

# Cppcheck
sudo apt install cppcheck
cppcheck --enable=all src/

# Clang-Tidy
mkdir build && cd build
cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON ..
clang-tidy ../src/*.cpp -p .
```

### Run Secret Detection Locally

```bash
# Gitleaks
docker run -v "$PWD:/path" zricethezav/gitleaks:latest detect --source /path --no-git
```

### Run Tests Locally

```bash
mkdir build && cd build
cmake ..
cmake --build . --parallel 4
ctest --output-on-failure
```

---

## Performance

### Pipeline Duration

| Stage | Approximate Duration |
|-------|---------------------|
| Build | 5-10 minutes |
| Test | 3-5 minutes |
| Security (SAST) | 5-10 minutes |
| Security (DAST) | 10-15 minutes |
| Deploy | 1-2 minutes |

**Total:** ~25-40 minutes (stages run in parallel where possible)

### Optimization Tips

1. **Parallel Execution:** Jobs within each stage run in parallel
2. **Artifact Caching:** Build artifacts reused across stages
3. **Targeted Scanning:** SAST excludes build/deps directories
4. **Allow Failure:** Non-critical jobs don't block pipeline

---

## Troubleshooting

### Build Failures

**Symptom:** Build fails on Ubuntu 22.04
**Solution:**
1. Check CMake configuration
2. Verify all dependencies installed
3. Check for compilation errors in job logs

### Test Failures

**Symptom:** Test suite fails
**Solution:**
1. Review test output in artifacts
2. Run test locally: `./build/tests/test_<name>`
3. Check for environment-specific issues

### Security Scan False Positives

**Symptom:** Security tool reports non-issue
**Solution:**
1. Review finding context
2. Add suppression comment if appropriate
3. File issue with security tool if bug

### DAST Setup Failures

**Symptom:** DAST jobs fail to start
**Solution:**
1. Check testnet node starts successfully
2. Verify RPC port accessibility
3. Review dast-setup job logs

---

## Security Contact

For security vulnerability reports related to the CI/CD pipeline or findings from security scans:

**Security Team:** security@international-coin.org
**PGP Key:** `4E5A A9F8 A4C2 F245 E3FC  9381 BCFB 6274 A3AF EEE9`

See [SECURITY.md](../SECURITY.md) for full responsible disclosure policy.

---

## References

- [GitLab CI/CD Documentation](https://docs.gitlab.com/ee/ci/)
- [GitLab Security Scanning](https://docs.gitlab.com/ee/user/application_security/)
- [OWASP Top 10](https://owasp.org/www-project-top-ten/)
- [NIST Secure Software Development Framework](https://csrc.nist.gov/Projects/ssdf)

---

**Last Updated:** December 17, 2025
**Maintainer:** Neil Adamson
**License:** MIT
