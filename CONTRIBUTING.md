# Contributing to INTcoin

Thank you for your interest in contributing to INTcoin! This document provides guidelines and instructions for contributing to the project.

**Project Status**: v1.2.0-beta - IN DEVELOPMENT
**Last Updated**: December 27, 2025
**Test Status**: 13/13 test suites passing (100%)

---

## 📜 Code of Conduct

### Our Values

- **Open and Inclusive**: Everyone is welcome to contribute, regardless of experience level
- **Respectful**: Treat all contributors with respect and kindness
- **Collaborative**: Work together to build the best quantum-resistant cryptocurrency
- **Transparent**: All development happens in the open

### Expected Behavior

- Use welcoming and inclusive language
- Be respectful of differing viewpoints and experiences
- Accept constructive criticism gracefully
- Focus on what is best for the community and project
- Show empathy towards other community members

---

## 🚀 How to Contribute

### Reporting Issues

Found a bug or have a feature request? Please open an issue on GitHub:

1. **Search existing issues** first to avoid duplicates
2. **Use the issue templates** if available
3. **Provide clear details**:
   - For bugs: steps to reproduce, expected vs actual behavior, system info
   - For features: clear description, use case, and benefits
4. **Be responsive** to follow-up questions

**Security Issues**: Please report security vulnerabilities privately to team@international-coin.org. See [SECURITY.md](SECURITY.md) for details.

### Suggesting Enhancements

We welcome suggestions for new features and improvements:

1. **Open a GitHub issue** with the `enhancement` label
2. **Explain the problem** you're trying to solve
3. **Describe your proposed solution**
4. **Discuss alternatives** you've considered
5. **Be open to feedback** from maintainers and community

### Writing Code

Ready to contribute code? Follow these steps:

#### 1. Fork and Clone

```bash
# Fork the repository on GitHub
# Clone your fork
git clone https://github.com/YOUR_USERNAME/intcoin.git
cd intcoin

# Add upstream remote
git remote add upstream https://github.com/INT-devs/intcoin.git
```

#### 2. Create a Branch

```bash
# Create a feature branch
git checkout -b feature/your-feature-name

# Or for bug fixes
git checkout -b fix/bug-description
```

#### 3. Make Your Changes

- **Follow the code style** (see below)
- **Write tests** for new functionality
- **Update documentation** as needed
- **Keep commits atomic** (one logical change per commit)
- **Write clear commit messages** (see below)

#### 4. Test Your Changes

```bash
# Build the project
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Debug ..
make -j$(nproc)

# Run all tests (13/13 suites passing - 100%)
ctest --output-on-failure

# Run specific test suites
./tests/test_crypto          # Cryptography (Dilithium3, Kyber768, SHA3-256)
./tests/test_randomx         # RandomX PoW and Digishield V3
./tests/test_bech32          # Address encoding/decoding
./tests/test_serialization   # Block/transaction serialization
./tests/test_storage         # RocksDB integration
./tests/test_validation      # Block/transaction validation (P2PKH signing FIXED)
./tests/test_genesis         # Genesis block verification
./tests/test_network         # P2P protocol and mempool
./tests/test_ml              # Machine learning (anomaly detection)
./tests/test_wallet          # HD wallet, BIP39, UTXO management
./tests/test_fuzz            # Fuzzing tests (~3,500 iterations)
./tests/test_integration     # End-to-end integration tests
./tests/test_lightning       # Lightning Network (10/10 subtests)
```

#### 5. Submit a Pull Request

```bash
# Push your branch
git push origin feature/your-feature-name

# Go to GitHub and create a Pull Request
# Fill out the PR template
# Link related issues
```

---

## 💻 Development Guidelines

### Code Style

#### C++

- **Standard**: C++23 (ISO/IEC 14882:2023)
- **Style Guide**: Based on [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html)
- **Formatter**: Use `clang-format` (config provided in `.clang-format`)

**Key Points**:
- Use descriptive variable and function names
- Prefer `snake_case` for variables and functions
- Use `PascalCase` for class names
- Add comments for complex logic
- Use modern C++23 features appropriately
- Avoid raw pointers; prefer smart pointers

**Example**:
```cpp
// Good
Result<BlockHeader> DeserializeBlockHeader(const std::vector<uint8_t>& data) {
    if (data.size() < BLOCK_HEADER_SIZE) {
        return Result<BlockHeader>::Error("Invalid header size");
    }
    // ... implementation
}

// Bad
blockheader deserialize(std::vector<uint8_t> d) {
    // ... implementation
}
```

#### Python

- **Standard**: Python 3.10+
- **Style Guide**: [PEP 8](https://pep8.org/)
- **Formatter**: Use `black` with default settings

### Commit Messages

Follow the [Conventional Commits](https://www.conventionalcommits.org/) specification:

```
<type>(<scope>): <subject>

<body>

<footer>
```

**Types**:
- `feat`: New feature
- `fix`: Bug fix
- `docs`: Documentation changes
- `style`: Code style changes (formatting, etc.)
- `refactor`: Code refactoring
- `test`: Adding or updating tests
- `chore`: Maintenance tasks

**Examples**:
```
feat(wallet): add HD wallet key derivation

Implements BIP32/BIP44 hierarchical deterministic wallet key
derivation adapted for Dilithium3 post-quantum signatures.

Closes #123
```

```
fix(network): resolve peer connection timeout issue

Fixes a race condition in peer handshake that caused connection
timeouts under high load.

Fixes #456
```

### Testing Requirements

All code contributions must include appropriate tests:

- **Unit Tests**: Test individual functions and classes
- **Integration Tests**: Test component interactions
- **Functional Tests**: Test end-to-end scenarios

**Test Coverage**: Aim for **80%+ code coverage** on new code.

**Test Location**:
- Unit tests: `tests/test_*.cpp`
- Integration tests: `tests/integration/`
- Functional tests: `tests/functional/`

### Documentation

- **Code Comments**: Document complex algorithms and non-obvious behavior
- **API Documentation**: Use Doxygen-style comments for public APIs
- **User Documentation**: Update relevant docs in `docs/` folder
- **Wiki**: Update wiki pages for significant changes

**Doxygen Example**:
```cpp
/// Calculate the block reward for a given height
/// @param height The block height
/// @return The block reward in INTS (1 INT = 1,000,000 INTS)
uint64_t GetBlockReward(uint64_t height);
```

---

## 🔍 Code Review Process

### What to Expect

1. **Automated Checks**: CI/CD pipeline runs automatically
   - Build verification
   - Test execution
   - Static analysis (clang-tidy, cppcheck)
   - Code coverage check

2. **Maintainer Review**: A maintainer will review your MR
   - Code quality and style
   - Test coverage
   - Documentation completeness
   - Adherence to project goals

3. **Feedback**: You may receive requests for changes
   - Be responsive to feedback
   - Make requested changes promptly
   - Ask questions if unclear

4. **Approval and Merge**: Once approved, a maintainer will merge

### Review Criteria

- ✅ Builds successfully on all supported platforms
- ✅ All tests pass (existing + new)
- ✅ Code follows style guidelines
- ✅ Adequate test coverage
- ✅ Documentation updated
- ✅ No unnecessary dependencies added
- ✅ Performance impact considered

---

## 🏗️ Development Environment

### Recommended Tools

**IDE/Editor**:
- Visual Studio Code (with C++ extensions)
- CLion
- Vim/Neovim (with coc.nvim or LSP)

**Debugging**:
- GDB (Linux/macOS)
- LLDB (macOS)
- Visual Studio Debugger (Windows)

**Profiling**:
- Valgrind (memory leaks, profiling)
- Perf (Linux performance analysis)
- Instruments (macOS)

### Building from Source

See [BUILDING.md](BUILDING.md) for complete build instructions.

**Quick Start** (macOS/Linux):
```bash
# Install dependencies via Homebrew
brew install cmake boost openssl@3 rocksdb qt@6 zeromq libevent

# Build liboqs (post-quantum crypto)
git clone https://github.com/open-quantum-safe/liboqs.git
cd liboqs && mkdir build && cd build
cmake -DCMAKE_INSTALL_PREFIX=/usr/local ..
make -j$(nproc) && sudo make install

# Build RandomX (ASIC-resistant PoW)
git clone https://github.com/tevador/RandomX.git
cd RandomX && mkdir build && cd build
cmake -DCMAKE_INSTALL_PREFIX=/usr/local ..
make -j$(nproc) && sudo make install

# Build INTcoin
cd /path/to/intcoin
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Debug ..
make -j$(nproc)

# Run tests to verify build
ctest --output-on-failure
```

**Current Binaries** (after successful build):
- `intcoind` (7.2 MB) - Full node daemon
- `intcoin-cli` (73 KB) - Command-line RPC client
- `intcoin-miner` (7.0 MB) - CPU miner with pool support
- `intcoin-qt` (7.4 MB) - Qt6 desktop wallet
- `intcoin-faucet` (7.1 MB) - Testnet faucet server
- `libintcoin_core.a` (883 KB) - Core library

---

## 📦 Submitting a Merge Request

### Checklist

Before submitting, ensure:

- [ ] Code builds successfully on your machine
- [ ] All tests pass locally
- [ ] New functionality has tests
- [ ] Documentation updated (if applicable)
- [ ] Code follows style guidelines
- [ ] Commit messages are clear and follow conventions
- [ ] Branch is up-to-date with `main`
- [ ] No merge conflicts

### MR Template

When creating a Merge Request, fill out the template:

```markdown
## Description
[Clear description of what this MR does]

## Related Issues
Closes #XXX

## Type of Change
- [ ] Bug fix
- [ ] New feature
- [ ] Breaking change
- [ ] Documentation update

## Testing
[How you tested this change]

## Checklist
- [ ] Code builds successfully
- [ ] All tests pass
- [ ] New tests added
- [ ] Documentation updated
```

---

## 🎯 Areas for Contribution

### Current Focus (97% → 100%)

We're in the final 3% push to v1.0! Priority areas:

**Outstanding TODO Items** (27 remaining):
- Wallet encryption (Kyber768) - HIGH PRIORITY
- GETHEADERS message handling - HIGH PRIORITY
- Mining pool Stratum protocol - HIGH PRIORITY
- Network hashrate calculation - ✅ COMPLETED
- SIGHASH transaction signing - ✅ COMPLETED
- HTTP Basic Auth for RPC - ✅ COMPLETED
- Block template generation - ✅ COMPLETED

See [PROGRESS_SUMMARY.md](PROGRESS_SUMMARY.md) for complete TODO list.

### Good First Issues

New to the project? Look for issues labeled `good-first-issue`:

- Documentation improvements
- Test coverage additions
- Small bug fixes
- Code style cleanup
- Utility function implementations (hex conversion, amount parsing)

### High Priority

Need something challenging? Check `high-priority` labels:

- Performance optimizations
- Community code review
- Test coverage improvements
- Documentation enhancements

### Help Wanted

Community expertise needed on `help-wanted` issues:

- Cross-platform testing (Windows 11, FreeBSD)
- Hardware wallet integration (Ledger, Trezor)
- Mobile wallet development (separate projects)
- Community security reviews and penetration testing

---

## 🤝 Community

### Communication Channels

- **GitHub Issues**: https://github.com/INT-devs/intcoin/issues
- **GitHub Discussions**: https://github.com/INT-devs/intcoin/discussions
- **Discord**: https://discord.gg/jCy3eNgx
- **Reddit**: https://www.reddit.com/r/INTcoin
- **Twitter/X**: https://x.com/INTcoin_team
- **Email**: team@international-coin.org
- **Wiki**: https://github.com/INT-devs/intcoin/wiki

### Getting Help

Need help with your contribution?

1. **Check the docs**: [Documentation](docs/)
2. **Search existing issues**: Someone may have asked before
3. **Ask on Discord/Telegram**: Community is happy to help
4. **Open a discussion issue**: Label it as `question`

---

## 📜 License

By contributing to INTcoin, you agree that your contributions will be licensed under the [MIT License](LICENSE).

---

## 🙏 Recognition

All contributors are recognized in:

- Release notes (see [RELEASE_NOTES_TEMPLATE.md](RELEASE_NOTES_TEMPLATE.md))
- GitLab contributors page
- Project README

---

**Thank you for contributing to INTcoin! Together, we're building the future of quantum-resistant cryptocurrency. 🚀**
