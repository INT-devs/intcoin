# Operating System Support Policy

**Last Updated**: December 26, 2025
**Policy Version**: 1.0

## Policy Statement

**INTcoin does not support end-of-life (EOL) operating systems that are no longer maintained by their original authors.**

This policy ensures:
- **Security**: EOL operating systems no longer receive security patches
- **Compatibility**: Modern C++23 features and dependencies require current OS versions
- **Maintainability**: Supporting EOL systems creates technical debt
- **Performance**: Newer OS versions provide better performance and features

## Supported Operating Systems

### macOS

| Version | Status | Support Until |
|---------|--------|---------------|
| **macOS 15 (Sequoia)** | ✅ Supported | Current |
| **macOS 14 (Sonoma)** | ✅ Supported | ~2027 |
| **macOS 13 (Ventura)** | ✅ Supported | ~2026 |
| **macOS 12 (Monterey)** | ✅ Minimum Required | ~2025 |
| macOS 11 (Big Sur) | ❌ Not Supported | EOL September 2023 |
| macOS 10.15 (Catalina) | ❌ Not Supported | EOL September 2022 |

**Minimum Required**: macOS 12.0 (Monterey) or later

### Windows

| Version | Build | Status | Support Until |
|---------|-------|--------|---------------|
| **Windows 11** | Any | ✅ Supported | Current |
| **Windows 10 22H2** | 19045+ | ✅ Minimum Required | October 2025 |
| Windows 10 21H2 | <19045 | ❌ Not Supported | EOL June 2023 |
| Windows 8.1 | N/A | ❌ Not Supported | EOL January 2023 |
| Windows 7 | N/A | ❌ Not Supported | EOL January 2020 |

**Minimum Required**: Windows 10 Build 19045 (22H2) or Windows 11

### Linux

#### Ubuntu

| Version | Status | Support Until |
|---------|--------|---------------|
| **Ubuntu 24.04 LTS** | ✅ Minimum Required | April 2029 |
| Ubuntu 22.04 LTS | ❌ Not Supported | Outdated toolchain |
| Ubuntu 20.04 LTS | ❌ Not Supported | EOL April 2025 |
| Ubuntu 18.04 LTS | ❌ Not Supported | EOL April 2023 |

**Minimum Required**: Ubuntu 24.04 LTS

#### Debian

| Version | Codename | Status | Support Until |
|---------|----------|--------|---------------|
| **Debian 12** | Bookworm | ✅ Supported | ~2028 |
| **Debian 11** | Bullseye | ✅ Minimum Required | ~2026 |
| Debian 10 | Buster | ❌ Not Supported | EOL June 2024 |
| Debian 9 | Stretch | ❌ Not Supported | EOL June 2022 |

**Minimum Required**: Debian 11 (Bullseye)

#### Fedora

| Version | Status | Support Until |
|---------|--------|---------------|
| **Fedora 41** | ✅ Supported | Current |
| **Fedora 40** | ✅ Supported | Current |
| **Fedora 39** | ✅ Supported | ~November 2024 |
| **Fedora 38** | ✅ Minimum Required | ~May 2024 |
| Fedora 37 | ❌ Not Supported | EOL November 2023 |
| Fedora 36 | ❌ Not Supported | EOL May 2023 |

**Minimum Required**: Fedora 38
**Note**: Fedora has short support cycles (~13 months). Update regularly.

#### CentOS / RHEL / Rocky Linux / AlmaLinux

| Version | Status | Support Until |
|---------|--------|---------------|
| **RHEL/Rocky/Alma 9** | ✅ Supported | ~2032 |
| **RHEL/Rocky/Alma 8** | ✅ Minimum Required | ~2029 |
| CentOS 7 | ❌ Not Supported | EOL June 2024 |
| CentOS 6 | ❌ Not Supported | EOL November 2020 |

**Minimum Required**: Version 8

#### Arch Linux / Manjaro

| Distribution | Status | Notes |
|--------------|--------|-------|
| **Arch Linux** | ✅ Supported | Rolling release, always current |
| **Manjaro** | ✅ Supported | Rolling release, always current |

**Note**: Rolling release distributions are always supported when kept up-to-date.

### FreeBSD

| Version | Status | Support Until |
|---------|--------|---------------|
| **FreeBSD 14.x** | ✅ Supported | Current |
| **FreeBSD 13.x** | ✅ Minimum Required | January 2026 |
| FreeBSD 12.x | ❌ Not Supported | EOL June 2024 |
| FreeBSD 11.x | ❌ Not Supported | EOL September 2021 |

**Minimum Required**: FreeBSD 13.0

## Enforcement

### Build-Time Checks

INTcoin enforces OS version requirements at multiple stages:

1. **CMake Configuration** (`CMakeLists.txt:27-53`)
   - macOS: Checks version via `sw_vers`
   - Windows: Displays version warning
   - Linux/BSD: Reminder to check compatibility

2. **Installation Scripts**
   - `scripts/build-windows.ps1`: Checks Windows build number
   - `scripts/install-linux.sh`: Validates Ubuntu/Debian/Fedora/CentOS versions
   - `scripts/install-freebsd.sh`: Validates FreeBSD major version

3. **Error Messages**
   - Clear indication when OS is EOL
   - Minimum version requirements stated
   - Upgrade path recommendations

### Example Error Messages

**macOS**:
```
CMake Error: macOS 11.6 is end-of-life and not supported.
Minimum required: macOS 12 (Monterey)
```

**Windows**:
```
ERROR: Windows 10 Build 19044 is not supported (end-of-life)
Minimum required: Windows 10 22H2 (Build 19045) or Windows 11
Your Windows 10 version is outdated. Please update to 22H2 or upgrade to Windows 11.
```

**Linux (Ubuntu)**:
```
ERROR: Ubuntu 18.04 is not supported. Minimum required: Ubuntu 20.04 LTS
```

## Exceptions

No exceptions to this policy. Security and maintainability take precedence.

If you require support for an EOL operating system for special circumstances:
1. **Fork the repository** and maintain your own branch
2. **Accept all security risks** of running on EOL systems
3. **Do not expect community support** for EOL platforms

## Policy Updates

This policy is reviewed:
- Every 6 months (January, July)
- When major OS versions reach EOL
- When new OS versions are released

### Update History

| Date | Change |
|------|--------|
| December 26, 2025 | Initial policy v1.0 |

## References

- [Apple Security Updates](https://support.apple.com/en-us/HT201222)
- [Microsoft Windows Lifecycle](https://docs.microsoft.com/en-us/lifecycle/products/windows)
- [Ubuntu Release Cycle](https://ubuntu.com/about/release-cycle)
- [Debian Release History](https://www.debian.org/releases/)
- [Fedora Release Schedule](https://fedoraproject.org/wiki/Releases)
- [Red Hat Enterprise Linux Life Cycle](https://access.redhat.com/support/policy/updates/errata)
- [FreeBSD Release Cycle](https://www.freebsd.org/security/)

---

**For Questions**: https://github.com/INT-devs/intcoin/issues
