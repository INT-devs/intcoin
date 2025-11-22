# CI/CD Pipeline Documentation

**INTcoin GitLab CI/CD Pipeline**
**Last Updated**: November 22, 2025
**Pipeline Version**: 2.0 (Enhanced)

---

## Table of Contents

- [Overview](#overview)
- [Pipeline Stages](#pipeline-stages)
- [Build Matrix](#build-matrix)
- [Security Scanning](#security-scanning)
- [Code Quality](#code-quality)
- [Artifacts](#artifacts)
- [Caching Strategy](#caching-strategy)
- [Dependencies](#dependencies)
- [Configuration](#configuration)
- [Troubleshooting](#troubleshooting)

---

## Overview

The INTcoin CI/CD pipeline provides comprehensive automated testing, security scanning, and deployment capabilities across multiple platforms.

### Pipeline Features

✅ **Multi-Platform Builds**: Ubuntu, Debian, Fedora, Arch Linux
✅ **Comprehensive Testing**: Unit, integration, and benchmark tests
✅ **Security Scanning**: SAST, DAST, dependency scanning, secret detection
✅ **Code Quality**: Static analysis (cppcheck, clang-tidy), code coverage
✅ **Automated Packaging**: DEB, RPM, and tarball generation
✅ **Container Security**: Trivy filesystem scanning
✅ **License Compliance**: Automated license scanning

---

## Pipeline Stages

The pipeline consists of 7 main stages:

### 1. Prepare
- Environment setup
- Dependency caching
- Submodule initialization

### 2. Build
- **Ubuntu 22.04 LTS** (Qt5) - Primary build target
- **Ubuntu 24.04 LTS** (Qt6) - Latest LTS
- **Debian 12** (Bookworm) - Stable release
- **Fedora Latest** - Rolling release testing
- **Arch Linux** - Bleeding edge testing

**Build Configuration**:
```cmake
-DCMAKE_BUILD_TYPE=Release
-DCMAKE_CXX_COMPILER_LAUNCHER=ccache
-DBUILD_QT_WALLET=ON
-DBUILD_DAEMON=ON
-DBUILD_CLI=ON
-DBUILD_MINER=ON
-DBUILD_TESTS=ON
```

### 3. Test
- **Unit Tests**: C++ test suites via CTest
- **Integration Tests**: Python-based integration tests
- **Performance Benchmarks**: Mining performance tests

### 4. Security
- **SAST**: Static Application Security Testing
- **Dependency Scanning**: CVE vulnerability detection
- **Secret Detection**: Exposed credentials/keys detection
- **SOOS SCA**: Software Composition Analysis
- **Container Scanning**: Trivy filesystem analysis
- **Cppcheck**: Static C++ code analysis
- **Clang-Tidy**: Modern C++ linter

### 5. Coverage
- **Code Coverage**: gcovr + lcov
- **Coverage Reports**: HTML, XML, Cobertura format
- **Coverage Metrics**: Integrated with GitLab UI

### 6. Package
- **DEB packages**: Ubuntu/Debian compatible
- **RPM packages**: Fedora/RHEL compatible
- **Tarballs**: Generic Linux distributions
- **Release builds**: Optimized for tagged releases

### 7. Deploy
- **GitLab Pages**: API documentation (Doxygen)
- **DAST Analysis**: Dynamic security testing
- **Release Publishing**: Automated release creation

---

## Build Matrix

| Platform | Image | Qt Version | Compiler | Architecture |
|----------|-------|------------|----------|--------------|
| Ubuntu 22.04 | ubuntu:22.04 | Qt 5 | GCC 11 | x86_64 |
| Ubuntu 24.04 | ubuntu:24.04 | Qt 6 | GCC 13 | x86_64 |
| Debian 12 | debian:12 | Qt 6 | GCC 12 | x86_64 |
| Fedora Latest | fedora:latest | Qt 6 | GCC 14 | x86_64 |
| Arch Linux | archlinux:latest | Qt 6 | GCC 14 | x86_64 |

---

## Security Scanning

### SAST (Static Application Security Testing)

**Purpose**: Detect security vulnerabilities in source code
**Tools**: GitLab SAST, SOOS SCA
**Triggers**: Main branch, merge requests

**Features**:
- SQL injection detection
- XSS vulnerability scanning
- Buffer overflow detection
- Cryptographic misuse detection

### Dependency Scanning

**Purpose**: Identify vulnerabilities in dependencies
**Tools**: GitLab Dependency Scanning
**Database**: CVE, NVD

**Scanned Dependencies**:
- Boost libraries
- OpenSSL/liboqs
- RocksDB
- Qt framework
- Hwloc

### Secret Detection

**Purpose**: Prevent credential leaks
**Patterns**: API keys, private keys, passwords, tokens

**Example Detections**:
- AWS access keys
- Private SSH keys
- Database credentials
- API tokens

### Container Scanning (Trivy)

**Purpose**: Filesystem vulnerability scanning
**Severity Levels**: UNKNOWN, LOW, MEDIUM, HIGH, CRITICAL
**Exit Policy**: Fails on CRITICAL vulnerabilities

```bash
trivy filesystem --severity CRITICAL --exit-code 1 .
```

### Static Analysis

#### Cppcheck
**Configuration**:
- Enable: all checks
- Suppress: unused functions, system includes
- Output: XML format for GitLab integration

#### Clang-Tidy
**Checks**:
- Modern C++ best practices
- Performance optimizations
- Readability improvements
- Bugprone patterns

---

## Code Quality

### Coverage Analysis

**Tools**: gcovr + lcov
**Target**: >80% code coverage
**Reports**:
- HTML (detailed, interactive)
- XML (Cobertura for GitLab)
- LCOV info files

**Metrics Tracked**:
- Line coverage
- Branch coverage
- Function coverage

### Code Quality Metrics

**Tool**: CodeClimate
**Categories**:
- Maintainability
- Complexity
- Duplication
- Test Coverage

---

## Artifacts

### Build Artifacts

| Artifact | Retention | Size (approx) | Platform |
|----------|-----------|---------------|----------|
| intcoind | 1 week | ~50 MB | All |
| intcoin-cli | 1 week | ~10 MB | All |
| intcoin-qt | 1 week | ~60 MB | All |
| intcoin-miner | 1 week | ~15 MB | All |

### Package Artifacts

| Package Type | Retention | Trigger | Compression |
|--------------|-----------|---------|-------------|
| .deb | 1 month | main, tags | dpkg |
| .rpm | 1 month | main, tags | rpm |
| .tar.gz | 1 month | main, tags | gzip |

### Release Artifacts

**Retention**: Forever (never expires)
**Trigger**: Git tags only
**Optimizations**: LTO enabled, Release build

**Example**:
```
intcoin-v1.3.0-ubuntu-22.04-amd64.deb
intcoin-v1.3.0-linux-x64.tar.gz
```

---

## Caching Strategy

### Build Cache

**Key**: `${CI_COMMIT_REF_SLUG}-${CI_JOB_NAME}`
**Policy**: Pull-push
**Max Size**: 2 GB

**Cached Paths**:
- `.ccache/` - Compiler cache
- `build/` - CMake build directory
- `.vcpkg/` - Package manager cache

**Effectiveness**:
- **First build**: ~15 minutes
- **Cached build**: ~3-5 minutes
- **Cache hit rate**: ~85%

### Test Cache

**Policy**: Pull only (read-only)
**Purpose**: Reuse build artifacts for testing

---

## Dependencies

### System Dependencies

**Ubuntu/Debian**:
```bash
build-essential cmake git ccache
libboost-all-dev libssl-dev librocksdb-dev libhwloc-dev
qt6-base-dev qt6-tools-dev qt6-svg-dev
python3 python3-pip ninja-build
```

**Fedora**:
```bash
gcc-c++ cmake git ccache
boost-devel openssl-devel rocksdb-devel hwloc-devel
qt6-qtbase-devel qt6-qttools-devel qt6-qtsvg-devel
python3 python3-pip ninja-build
```

**Arch Linux**:
```bash
base-devel cmake git ccache
boost openssl rocksdb hwloc
qt6-base qt6-tools qt6-svg
python python-pip ninja
```

### Quantum-Resistant Cryptography

**liboqs** (Open Quantum Safe)
**Version**: 0.12.0 (Latest stable)
**Build**: From source (not in repos)

**Installation**:
```bash
wget https://github.com/open-quantum-safe/liboqs/archive/refs/tags/0.12.0.tar.gz
tar -xzf 0.12.0.tar.gz
cd liboqs-0.12.0
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=ON -GNinja
ninja && ninja install
ldconfig
```

**Algorithms Provided**:
- CRYSTALS-Dilithium (signatures)
- CRYSTALS-Kyber (KEMs)
- SPHINCS+ (signatures)
- Falcon (signatures)

---

## Configuration

### Environment Variables

| Variable | Default | Purpose |
|----------|---------|---------|
| `CMAKE_BUILD_TYPE` | Release | Build configuration |
| `CCACHE_DIR` | `.ccache` | Compiler cache location |
| `CCACHE_MAXSIZE` | 2G | Maximum cache size |
| `LIBOQS_VERSION` | 0.12.0 | liboqs version |
| `QT_VERSION` | 6 | Qt major version |
| `BOOST_MIN_VERSION` | 1.74 | Minimum Boost version |

### Pipeline Triggers

**Automatic Triggers**:
- Commits to `main` branch
- Merge requests
- Git tags

**Manual Triggers**:
- GitLab UI "Run Pipeline" button
- API: `POST /projects/:id/pipeline`

### Job Policies

**Main Branch**:
- All build jobs
- All test jobs
- All security jobs
- Package jobs
- Documentation deployment

**Merge Requests**:
- All build jobs
- All test jobs
- Security scanning
- No packaging

**Tags Only**:
- Release builds
- Forever artifact retention
- LTO optimization enabled

---

## Troubleshooting

### Build Failures

#### Missing Dependencies

**Symptom**: CMake configuration fails
**Solution**: Check `before_script` section for missing packages

```yaml
before_script:
  - apt-get install -y missing-package
```

#### Cache Corruption

**Symptom**: Inconsistent build failures
**Solution**: Clear cache via GitLab UI → CI/CD → Pipelines → Clear Runner Caches

#### ccache Issues

**Symptom**: No cache hits despite previous builds
**Solution**: Check cache key matches previous jobs

```yaml
cache:
  key: ${CI_COMMIT_REF_SLUG}-${CI_JOB_NAME}
```

### Test Failures

#### Unit Test Failures

**Log Location**: Build artifacts → `test-results/*.xml`
**Command**: `ctest --output-on-failure --verbose`

**Common Issues**:
- Timing-sensitive tests
- Platform-specific behavior
- Missing test fixtures

#### Integration Test Failures

**Log Location**: Job output
**Dependencies**: pytest, pytest-asyncio

**Common Issues**:
- Network connectivity
- Port conflicts
- Daemon startup timeout

### Security Scan Failures

#### False Positives

**Solution**: Add suppressions to `.gitlab-ci.yml`

```yaml
variables:
  SAST_EXCLUDED_PATHS: "build/, tests/, .ccache/"
```

#### Container Scanning Fails

**Solution**: Update base images to patch CVEs

```bash
# Check for updates
docker pull ubuntu:24.04
```

### Artifact Upload Failures

**Symptom**: "artifact too large" error
**Solution**: Compress or split artifacts

```bash
- tar czf intcoind.tar.gz intcoind
```

**Size Limits**:
- Free tier: 1 GB per artifact
- Premium tier: 5 GB per artifact

---

## Performance Metrics

### Typical Pipeline Duration

| Stage | Duration | Parallelization |
|-------|----------|-----------------|
| Build (Ubuntu 22.04) | 8-12 min | Cached: 3-5 min |
| Build (All platforms) | 10-15 min | 5 parallel jobs |
| Tests | 2-5 min | After build complete |
| Security | 5-10 min | Parallel |
| Packaging | 3-5 min | After tests |
| **Total** | **20-35 min** | **Peak: 10 jobs** |

### Resource Usage

**Compute**:
- **CPU**: 4-8 cores per job
- **RAM**: 4-8 GB per job
- **Disk**: 10-20 GB per job

**Network**:
- **Download**: ~500 MB (dependencies)
- **Upload**: ~200 MB (artifacts)

---

## Best Practices

### For Contributors

✅ **Run local builds** before pushing
✅ **Enable pre-commit hooks** for code quality
✅ **Write tests** for new features
✅ **Update documentation** with code changes
✅ **Keep commits atomic** for easier CI debugging

### For Maintainers

✅ **Monitor pipeline success rate**
✅ **Review security scan results** regularly
✅ **Update dependencies** quarterly
✅ **Optimize cache usage** for faster builds
✅ **Archive old artifacts** to save storage

---

## CI/CD Metrics Dashboard

### Key Metrics

- **Pipeline Success Rate**: Target >95%
- **Average Duration**: Target <30 minutes
- **Cache Hit Rate**: Target >80%
- **Security Vulnerabilities**: Target 0 critical
- **Code Coverage**: Target >80%

### Monitoring

**GitLab Built-in**:
- CI/CD → Pipelines → Charts
- CI/CD → Jobs → Statistics
- Security → Vulnerability Report

**External**:
- SOOS Dashboard: https://app.soos.io
- CodeClimate: Code quality trends

---

## Migration Guide

### From v1.0 to v2.0

**Changes**:
1. Added `security` stage
2. Added `coverage` stage
3. Updated Node.js: 20 → 22
4. Updated liboqs: 0.10.1 → 0.12.0
5. Enhanced caching strategy
6. Added Trivy container scanning
7. Added clang-tidy analysis

**Action Required**:
- None (backward compatible)
- Old pipelines continue working
- New features enabled automatically

---

## References

- [GitLab CI/CD Documentation](https://docs.gitlab.com/ee/ci/)
- [liboqs Documentation](https://github.com/open-quantum-safe/liboqs)
- [SOOS Documentation](https://kb.soos.io/)
- [Trivy Documentation](https://aquasecurity.github.io/trivy/)
- [CodeClimate Documentation](https://docs.codeclimate.com/)

---

**Questions?** Open an issue or contact the maintainers.

**Copyright © 2025 INTcoin Core (Maddison Lane)**
**License**: MIT
