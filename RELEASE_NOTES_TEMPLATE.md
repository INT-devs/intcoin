# INTcoin Release Notes v[VERSION]

**Release Date**: [DATE]
**Release Type**: [Alpha/Beta/Stable]
**Git Tag**: v[VERSION]

---

## 🎉 Highlights

[Brief overview of major features, improvements, or changes in this release]

---

## ✨ New Features

- [Feature 1]
- [Feature 2]
- [Feature 3]

---

## 🔧 Improvements

- [Improvement 1]
- [Improvement 2]
- [Improvement 3]

---

## 🐛 Bug Fixes

- [Bug fix 1]
- [Bug fix 2]
- [Bug fix 3]

---

## 📚 Documentation

- [Documentation update 1]
- [Documentation update 2]

---

## ⚠️ Breaking Changes

[List any breaking changes that require user action]

---

## 🙏 Contributors

We would like to thank all contributors who participated in this release!

### Top Contributors

| Contributor | Contributions |
|-------------|---------------|
| [Contributor 1] | [N] commits |
| [Contributor 2] | [N] commits |
| [Contributor 3] | [N] commits |
| [Contributor 4] | [N] commits |
| [Contributor 5] | [N] commits |
| [Contributor 6] | [N] commits |
| [Contributor 7] | [N] commits |
| [Contributor 8] | [N] commits |
| [Contributor 9] | [N] commits |
| [Contributor 10] | [N] commits |
| [Contributor 11] | [N] commits |
| [Contributor 12] | [N] commits |
| [Contributor 13] | [N] commits |
| [Contributor 14] | [N] commits |
| [Contributor 15] | [N] commits |
| [Contributor 16] | [N] commits |
| [Contributor 17] | [N] commits |
| [Contributor 18] | [N] commits |
| [Contributor 19] | [N] commits |
| [Contributor 20] | [N] commits |

**[X] additional contributors** also participated in this release. Thank you!

*Note: When there are more than 20 contributors, only the top 20 by commit count are shown in the table above.*

---

## 📦 Download

- **Source Code**: https://gitlab.com/intcoin/crypto/-/releases/v[VERSION]
- **Binaries**:
  - Linux (x86_64): `intcoin-v[VERSION]-linux-x86_64.tar.gz`
  - macOS (ARM64): `intcoin-v[VERSION]-macos-arm64.tar.gz`
  - macOS (x86_64): `intcoin-v[VERSION]-macos-x86_64.tar.gz`
  - FreeBSD (x86_64): `intcoin-v[VERSION]-freebsd-x86_64.tar.gz`
  - Windows (x86_64): `intcoin-v[VERSION]-windows-x86_64.zip`

---

## 🔐 Verification

### SHA256 Checksums

```
[checksum]  intcoin-v[VERSION]-linux-x86_64.tar.gz
[checksum]  intcoin-v[VERSION]-macos-arm64.tar.gz
[checksum]  intcoin-v[VERSION]-macos-x86_64.tar.gz
[checksum]  intcoin-v[VERSION]-freebsd-x86_64.tar.gz
[checksum]  intcoin-v[VERSION]-windows-x86_64.zip
```

### GPG Signature

All release binaries are signed with the INTcoin release key.

**Fingerprint**: `[GPG_FINGERPRINT]`

To verify:
```bash
gpg --verify intcoin-v[VERSION]-linux-x86_64.tar.gz.asc
```

---

## 📋 Requirements

### Minimum System Requirements

- **CPU**: 64-bit processor (x86_64 or ARM64)
- **RAM**: 4 GB minimum, 8 GB recommended
- **Disk**: 50 GB available space (for full node)
- **OS**:
  - Linux: Ubuntu 22.04+, Debian 12+, Fedora 38+
  - macOS: 13 (Ventura) or later
  - FreeBSD: 13.0 or later
  - Windows: 11 (21H2 or later)

### Dependencies

- CMake 4.2.0+
- C++23 compiler (GCC 13+, Clang 16+, MSVC 2022+)
- OpenSSL 3.5.4+
- Boost 1.89.0+
- RocksDB 10.7+
- liboqs 0.15.0+
- RandomX 1.2.1+

---

## 🔄 Upgrade Instructions

### From v[PREVIOUS_VERSION]

1. **Backup your wallet**:
   ```bash
   intcoin-cli backupwallet ~/intcoin-backup.dat
   ```

2. **Stop the daemon**:
   ```bash
   intcoin-cli stop
   ```

3. **Replace binaries** with new version

4. **Restart the daemon**:
   ```bash
   intcoind
   ```

5. **Verify upgrade**:
   ```bash
   intcoin-cli getnetworkinfo
   ```

---

## 🐛 Known Issues

[List any known issues or limitations in this release]

---

## 📞 Support

- **Website**: https://international-coin.org
- **Documentation**: https://international-coin.org/docs
- **GitLab**: https://gitlab.com/intcoin/crypto
- **Email**: team@international-coin.org
- **Discord**: https://discord.gg/intcoin
- **Telegram**: https://t.me/intcoin

---

## 📜 License

INTcoin is released under the MIT License.

Copyright (c) 2025 INTcoin Team (Neil Adamson)

---

**Full Changelog**: https://gitlab.com/intcoin/crypto/-/compare/v[PREVIOUS_VERSION]...v[VERSION]
