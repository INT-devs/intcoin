# INTcoin CI/CD Pipeline Documentation

**Copyright (c) 2025 INTcoin Core (Maddison Lane)**

This document describes the GitLab CI/CD pipeline for INTcoin, including caching strategies, build optimizations, and workflow rules.

## Table of Contents

- [Overview](#overview)
- [Pipeline Stages](#pipeline-stages)
- [Caching Strategy](#caching-strategy)
- [Build Jobs](#build-jobs)
- [Test Jobs](#test-jobs)
- [Package Jobs](#package-jobs)
- [Security Scanning](#security-scanning)
- [Workflow Rules](#workflow-rules)

---

## Overview

The INTcoin CI/CD pipeline provides:

✅ **Multi-platform builds** (Ubuntu, Debian, Fedora, Arch Linux)
✅ **Efficient caching** with ccache (reduces build time by 50-80%)
✅ **Automated testing** (unit tests, integration tests)
✅ **Package generation** (.deb, .rpm, .tar.gz)
✅ **Security scanning** (SAST, secret detection, dependency scanning)
✅ **Code quality analysis**
✅ **Release automation** (for git tags)
✅ **Documentation deployment** (GitLab Pages)

---

## Pipeline Stages

The pipeline consists of 5 stages:

1. **prepare** - Dependency preparation (reserved for future use)
2. **build** - Compile binaries for multiple platforms
3. **test** - Run unit and integration tests
4. **package** - Create distribution packages
5. **security** - Security scanning and code quality checks

---

## Caching Strategy

### ccache (Compiler Cache)

**Purpose**: Dramatically reduce compilation time on subsequent builds

**Configuration**:
```yaml
variables:
  CCACHE_DIR: ${CI_PROJECT_DIR}/.ccache
  CCACHE_MAXSIZE: 2G
```

**Cache Locations**:
- `.ccache/` - Compiled object cache (ccache)
- `build/` - CMake build artifacts
- `.vcpkg/` - vcpkg packages (Windows builds)

**Cache Keys**: `${CI_COMMIT_REF_SLUG}-${CI_JOB_NAME}`
- Separate cache per branch and job
- Prevents cache conflicts between different configurations

### Cache Policies

**Pull-Push** (build jobs):
```yaml
cache:
  policy: pull-push
```
- Downloads cache at start
- Uploads cache at end
- Used for compilation jobs

**Pull-Only** (test/package jobs):
```yaml
cache:
  policy: pull
```
- Only downloads cache
- Used for jobs that don't modify compiled artifacts
- Faster execution, no upload overhead

### Expected Performance

| Build | First Run | Cached Run | Improvement |
|-------|-----------|------------|-------------|
| Ubuntu 22.04 | ~15 min | ~3 min | 80% faster |
| Fedora | ~12 min | ~2.5 min | 79% faster |
| Debian | ~14 min | ~3 min | 78% faster |
| Arch Linux | ~10 min | ~2 min | 80% faster |

---

## Build Jobs

### Ubuntu 22.04 LTS (Primary)

**Job**: `build:ubuntu-22.04`
**Image**: `ubuntu:22.04`
**Features**: Full build with all components

**Artifacts**:
- `intcoind` - Daemon
- `intcoin-cli` - CLI tool
- `intcoin-qt` - GUI wallet
- `intcoin-miner` - CPU miner

**Retention**: 1 week

### Ubuntu 24.04 LTS

**Job**: `build:ubuntu-24.04`
**Image**: `ubuntu:24.04`
**Purpose**: Test compatibility with newer Ubuntu

### Debian 12 (Bookworm)

**Job**: `build:debian-12`
**Image**: `debian:12`
**Purpose**: Debian stable support

### Fedora Latest

**Job**: `build:fedora`
**Image**: `fedora:latest`
**Purpose**: RPM-based distribution support

### Arch Linux

**Job**: `build:arch`
**Image**: `archlinux:latest`
**Purpose**: Rolling release testing

### Build Configuration

All build jobs use:
```yaml
cmake .. \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_CXX_COMPILER_LAUNCHER=ccache \
  -DCMAKE_C_COMPILER_LAUNCHER=ccache \
  -DBUILD_QT_WALLET=ON \
  -DBUILD_DAEMON=ON \
  -DBUILD_CLI=ON \
  -DBUILD_MINER=ON \
  -DBUILD_TESTS=ON
```

**Parallel Compilation**: `make -j$(nproc)` (uses all CPU cores)

---

## Test Jobs

### Unit Tests

**Job**: `test:unit`
**Dependencies**: `build:ubuntu-22.04`
**Command**: `ctest --output-on-failure --verbose`

**Test Categories**:
- Cryptography tests (Dilithium, Kyber, SHA3)
- Blockchain tests (blocks, transactions, UTXO)
- Network tests (P2P protocol)
- Consensus tests (validation, difficulty)

**Artifacts**: JUnit XML reports for GitLab integration

### Integration Tests

**Job**: `test:integration`
**Dependencies**: `build:ubuntu-22.04`
**Framework**: pytest

**Test Coverage**:
- RPC API tests
- Wallet functionality
- Mining operations
- Network synchronization

**Status**: `allow_failure: true` (tests still in development)

### Performance Benchmarks

**Job**: `benchmark:mining`
**Purpose**: Measure mining performance
**Duration**: 10 seconds
**Only runs**: main branch and tags

---

## Package Jobs

### Debian Package (.deb)

**Job**: `package:deb`
**Platform**: Ubuntu 22.04
**Generator**: `cpack -G DEB`
**Output**: `intcoin-${CI_COMMIT_SHORT_SHA}-amd64.deb`
**Retention**: 1 month
**Trigger**: main branch and tags only

**Installation**:
```bash
sudo dpkg -i intcoin-*.deb
```

### RPM Package (.rpm)

**Job**: `package:rpm`
**Platform**: Fedora
**Generator**: `cpack -G RPM`
**Output**: `intcoin-${CI_COMMIT_SHORT_SHA}.x86_64.rpm`
**Retention**: 1 month
**Trigger**: main branch and tags only

**Installation**:
```bash
sudo rpm -i intcoin-*.rpm
```

### Tarball (.tar.gz)

**Job**: `package:tarball`
**Platform**: Ubuntu 22.04
**Generator**: `cpack -G TGZ`
**Output**: `intcoin-${CI_COMMIT_SHORT_SHA}-linux-x64.tar.gz`
**Retention**: 1 month
**Trigger**: main branch and tags only

**Installation**:
```bash
tar xzf intcoin-*.tar.gz
sudo cp -r * /usr/local/
```

---

## Release Builds

### Tagged Releases

**Job**: `release:ubuntu`
**Trigger**: Git tags only (e.g., `v0.1.0`)
**Optimizations**:
- Link-time optimization (LTO) enabled
- `-DCMAKE_INTERPROCEDURAL_OPTIMIZATION=ON`

**Artifacts**:
- `.deb` package
- `.tar.gz` tarball
**Retention**: Forever

**Creating a Release**:
```bash
git tag -a v0.1.0 -m "Release v0.1.0"
git push origin v0.1.0
```

---

## Security Scanning

### SAST (Static Application Security Testing)

**Template**: `Security/SAST.gitlab-ci.yml`
**Purpose**: Detect security vulnerabilities in code
**Languages**: C, C++, Python, Shell
**Status**: `allow_failure: true`

**Checks**:
- Buffer overflows
- SQL injection (if applicable)
- Command injection
- Hardcoded secrets
- Unsafe functions

### Secret Detection

**Template**: `Security/Secret-Detection.gitlab-ci.yml`
**Purpose**: Find accidentally committed secrets
**Status**: `allow_failure: true`

**Detects**:
- API keys
- Private keys
- Passwords
- Tokens
- Credentials

### Dependency Scanning

**Template**: `Security/Dependency-Scanning.gitlab-ci.yml`
**Purpose**: Check for vulnerable dependencies
**Status**: `allow_failure: true`

**Scans**:
- Boost libraries
- OpenSSL
- Qt5
- External dependencies

### Code Quality

**Job**: `code_quality`
**Tool**: GitLab Code Quality
**Trigger**: main branch and merge requests

**Metrics**:
- Code smells
- Duplication
- Complexity
- Maintainability issues

**Report**: `gl-code-quality-report.json`

---

## Documentation

### GitLab Pages

**Job**: `pages`
**URL**: `https://intcoin.gitlab.io/crypto/`
**Content**:
- Generated API documentation (Doxygen)
- Markdown documentation from `docs/`
- README as index page

**Deployment**: Automatic on main branch

---

## Workflow Rules

### Automatic Triggers

Pipeline runs automatically for:
- **Merge requests** - Full pipeline
- **Tags** - Release pipeline
- **Main branch** - Full pipeline + packages

### Manual Triggers

Pipeline requires manual start for:
- **Feature branches** - Must click "Run Pipeline"
- **Draft MRs** - Skipped until marked ready

### Rule Configuration

```yaml
workflow:
  rules:
    - if: $CI_PIPELINE_SOURCE == "merge_request_event"
    - if: $CI_COMMIT_TAG
    - if: $CI_COMMIT_BRANCH == "main"
    - if: $CI_COMMIT_BRANCH && $CI_COMMIT_REF_PROTECTED == "false"
      when: manual
```

---

## Variables

### Global Variables

```yaml
GIT_SUBMODULE_STRATEGY: recursive      # Clone submodules
CMAKE_BUILD_TYPE: Release              # Default build type
CCACHE_DIR: ${CI_PROJECT_DIR}/.ccache  # Cache directory
CCACHE_MAXSIZE: 2G                     # Max cache size
DEBIAN_FRONTEND: noninteractive        # No apt prompts
```

### Overriding Variables

**Via GitLab UI**:
1. Go to Settings → CI/CD → Variables
2. Add variable (e.g., `CMAKE_BUILD_TYPE=Debug`)
3. Choose scope (all branches, protected, etc.)

**Via .gitlab-ci.yml**:
```yaml
build:debug:
  variables:
    CMAKE_BUILD_TYPE: Debug
```

---

## Artifacts

### Build Artifacts

**Expiration**: 1 week
**Size**: ~50-100 MB per build
**Contents**: Compiled binaries

### Package Artifacts

**Expiration**: 1 month (30 days)
**Size**: ~20-30 MB per package
**Contents**: Installable packages

### Release Artifacts

**Expiration**: Never
**Size**: ~20-30 MB
**Contents**: Official release packages

### Downloading Artifacts

**Via GitLab UI**:
1. Go to CI/CD → Pipelines
2. Click pipeline number
3. Click job name
4. Click "Download" on right side

**Via API**:
```bash
curl --header "PRIVATE-TOKEN: <token>" \
  "https://gitlab.com/api/v4/projects/intcoin%2Fcrypto/jobs/artifacts/main/download?job=build:ubuntu-22.04"
```

---

## Troubleshooting

### Cache Not Working

**Symptom**: Every build starts from scratch

**Solutions**:
1. Check cache key uniqueness
2. Verify ccache is installed in `before_script`
3. Check cache path permissions
4. Clear cache in GitLab UI: Settings → CI/CD → Clear runner caches

### Out of Disk Space

**Symptom**: Build fails with "No space left on device"

**Solutions**:
1. Reduce `CCACHE_MAXSIZE`
2. Add cache cleanup:
   ```yaml
   after_script:
     - ccache -C  # Clear cache
   ```

### Build Timeout

**Symptom**: Job fails after 1 hour

**Solutions**:
1. Increase timeout in GitLab settings
2. Reduce parallel jobs: `make -j2` instead of `make -j$(nproc)`
3. Disable some components:
   ```yaml
   -DBUILD_QT_WALLET=OFF
   ```

### Runner Issues

**Symptom**: No available runners

**Solutions**:
1. Check runner tags match job tags (`docker`)
2. Verify runners are online: Settings → CI/CD → Runners
3. Use shared runners: Settings → CI/CD → Enable shared runners

---

## Best Practices

### 1. Efficient Caching

✅ Use ccache for C++ compilation
✅ Cache per branch and job
✅ Use pull-only policy for test jobs
✅ Set reasonable cache size limits

### 2. Parallel Execution

✅ Run independent jobs in parallel
✅ Use `dependencies` to specify job order
✅ Use `needs` for explicit dependencies (faster)

### 3. Artifact Management

✅ Set appropriate expiration times
✅ Only save necessary files
✅ Compress large artifacts
✅ Use `artifacts:when: on_failure` for debug info

### 4. Resource Optimization

✅ Use minimal Docker images
✅ Install only required packages
✅ Use `--no-install-recommends` for apt
✅ Share base images across jobs

### 5. Security

✅ Never commit secrets to `.gitlab-ci.yml`
✅ Use GitLab CI/CD variables for sensitive data
✅ Enable security scanning jobs
✅ Review code quality reports

---

## Metrics

### Pipeline Performance

**Target Times**:
- **Build** (cached): 2-3 minutes
- **Test**: 1-2 minutes
- **Package**: 30 seconds
- **Total pipeline**: 5-10 minutes (cached)

**First Run** (no cache):
- **Build**: 12-15 minutes
- **Test**: 1-2 minutes
- **Package**: 30 seconds
- **Total pipeline**: 20-25 minutes

### Cache Hit Rates

**Expected**:
- **First build**: 0% (no cache)
- **Subsequent builds**: 80-95% (with cache)
- **After small changes**: 95-99% (minimal recompilation)

### Cost Optimization

**CI Minutes Usage**:
- **Per commit** (cached): ~10-15 minutes
- **Per merge request**: ~30-50 minutes
- **Monthly estimate**: ~500-1000 minutes (active development)

---

## Future Improvements

### Planned Enhancements

🔄 **Windows builds** - Add Windows Server runner
🔄 **macOS builds** - Add macOS runner for cross-platform testing
🔄 **Docker images** - Build and publish Docker images
🔄 **Performance tracking** - Track benchmark results over time
🔄 **Fuzzing** - Add fuzzing jobs for security
🔄 **Coverage reports** - Add code coverage analysis

---

## Support

**Pipeline Issues**: team@international-coin.org
**GitLab CI Help**: https://docs.gitlab.com/ee/ci/
**Runner Setup**: https://docs.gitlab.com/runner/

---

**Last Updated**: January 2025
