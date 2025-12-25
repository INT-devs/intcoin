# INTcoin Build and Installation Scripts

This directory contains automated build and installation scripts for all supported platforms.

## Available Scripts

### Windows

**Script**: `build-windows.ps1`

PowerShell script for building INTcoin on Windows 10/11 with Visual Studio 2022.

**Quick Start**:
```powershell
# Run in PowerShell (as Administrator recommended)
.\scripts\build-windows.ps1

# With custom options:
.\scripts\build-windows.ps1 -BuildType Release -CleanBuild
```

**Output**: Creates `dist-windows/` directory with all executables and DLLs ready for distribution.

**Documentation**: See [BUILDING.md](../docs/BUILDING.md#windows-build-script)

---

### Linux

**Script**: `install-linux.sh`

Bash script for installing INTcoin on Linux distributions (Ubuntu, Debian, Fedora, CentOS, Arch).

**Quick Start**:
```bash
# System-wide installation (requires sudo)
sudo ./scripts/install-linux.sh

# User installation (installs to ~/.local)
./scripts/install-linux.sh
```

**What it does**:
- Detects your Linux distribution
- Installs all dependencies
- Builds liboqs, RandomX, and INTcoin
- Creates systemd service (if run as root)
- Installs binaries to /usr/local/bin or ~/.local/bin

**Documentation**: See [BUILDING.md](../docs/BUILDING.md#linux-installation-script)

---

### FreeBSD

**Script**: `install-freebsd.sh`

Shell script for installing INTcoin on FreeBSD 12.x, 13.x, and 14.x.

**Quick Start**:
```bash
# System-wide installation (requires sudo)
sudo ./scripts/install-freebsd.sh

# User installation (installs to ~/.local)
./scripts/install-freebsd.sh
```

**What it does**:
- Installs dependencies via pkg
- Builds liboqs, RandomX, and INTcoin
- Creates rc.d service (if run as root)
- Installs binaries to /usr/local/bin or ~/.local/bin

**Documentation**: See [BUILDING.md](../docs/BUILDING.md#freebsd-installation-script)

---

## Environment Variables

All scripts support environment variables for customization:

```bash
# Build type (Release, Debug, RelWithDebInfo)
export BUILD_TYPE=Release

# Enable/disable Qt wallet
export BUILD_QT=ON

# Enable/disable tests
export BUILD_TESTS=ON

# Custom install prefix (Linux/FreeBSD only)
export INSTALL_PREFIX=/opt/intcoin

# Then run the script
./scripts/install-linux.sh
```

For Windows PowerShell:
```powershell
# Use command-line parameters
.\scripts\build-windows.ps1 -BuildType Debug -VcpkgRoot "C:\vcpkg"
```

---

## Prerequisites

### Windows
- Windows 10/11 (64-bit)
- Visual Studio 2022 with C++ workload
- Git for Windows
- CMake 3.28+

### Linux
- Ubuntu 20.04+, Debian 11+, Fedora 35+, CentOS 8+, or Arch Linux
- GCC 11+ or Clang 14+
- CMake 3.20+
- Git

### FreeBSD
- FreeBSD 12.x, 13.x, or 14.x
- Clang 14+ (included in base system)
- CMake 3.20+
- Git

---

## Build Times

Approximate build times on a 4-core system with 8GB RAM:

| Platform | First Build | Subsequent Builds |
|----------|-------------|-------------------|
| **Windows** | 60-90 min | 5-10 min |
| **Linux** | 30-45 min | 5-10 min |
| **FreeBSD** | 35-50 min | 5-10 min |

First builds take longer due to dependency compilation (vcpkg on Windows, source builds on Linux/FreeBSD).

---

## Troubleshooting

### Windows

**Error**: "Visual Studio 2022 not found"
- Install Visual Studio 2022 with "Desktop development with C++" workload

**Error**: "CMake not found"
- Install CMake from https://cmake.org/download/ and add to PATH

**Error**: "vcpkg installation failed"
- Delete `%USERPROFILE%\vcpkg` and run script again
- Check internet connection (vcpkg downloads packages)

### Linux/FreeBSD

**Error**: "Permission denied"
- Make script executable: `chmod +x scripts/install-linux.sh`

**Error**: "Unsupported distribution"
- Install dependencies manually (see [BUILDING.md](../docs/BUILDING.md))
- Continue with `-y` flag when prompted

**Error**: "Insufficient disk space"
- Ensure at least 5GB free space for build
- Clean up with: `rm -rf build-linux ~/.intcoin-build`

---

## Support

For detailed build instructions, see:
- [BUILDING.md](../docs/BUILDING.md) - Complete build guide
- [TESTING.md](../docs/TESTING.md) - Testing guide
- [GitHub Issues](https://github.com/intcoin/crypto/issues) - Report problems

---

**Version**: 1.0.0-alpha
**Last Updated**: December 25, 2025
