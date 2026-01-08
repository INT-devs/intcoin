# INTcoin v1.3.0-beta - Windows Build Instructions

**Version**: 1.3.0-beta
**Platform**: Windows 10/11 (64-bit)
**Last Updated**: January 8, 2026

---

## Table of Contents

1. [System Requirements](#system-requirements)
2. [Prerequisites](#prerequisites)
3. [Building with Visual Studio](#building-with-visual-studio)
4. [Building with MinGW](#building-with-mingw)
5. [Building the Installer](#building-the-installer)
6. [Running INTcoin](#running-intcoin)
7. [Troubleshooting](#troubleshooting)

---

## System Requirements

### Minimum Requirements
- **OS**: Windows 10 (64-bit) or Windows 11
- **CPU**: 64-bit processor (x86-64)
- **RAM**: 4GB minimum (8GB recommended)
- **Disk**: 20GB free space (for build + blockchain)
- **Visual Studio**: 2022 or later

### Recommended Requirements
- **OS**: Windows 11
- **CPU**: 4+ cores
- **RAM**: 16GB+
- **Disk**: 100GB+ SSD
- **Visual Studio**: 2022 Community Edition (latest)

---

## Prerequisites

### 1. Install Visual Studio 2022

Download and install **Visual Studio 2022 Community Edition** (free):
https://visualstudio.microsoft.com/downloads/

**Required Workloads**:
- ✅ Desktop development with C++
- ✅ C++ CMake tools for Windows
- ✅ C++ Clang tools for Windows (optional, for Clang build)

**Individual Components** (select in installer):
- ✅ MSVC v143 - VS 2022 C++ x64/x86 build tools (latest)
- ✅ C++ CMake tools for Windows
- ✅ Windows 10 SDK or Windows 11 SDK
- ✅ C++/CLI support for v143 build tools
- ✅ Git for Windows

### 2. Install vcpkg (Package Manager)

```powershell
# Open PowerShell as Administrator

# Clone vcpkg
cd C:\
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg

# Bootstrap vcpkg
.\bootstrap-vcpkg.bat

# Integrate with Visual Studio
.\vcpkg integrate install

# Add to PATH
[Environment]::SetEnvironmentVariable(
    "Path",
    "$env:Path;C:\vcpkg",
    "Machine"
)
```

### 3. Install Dependencies via vcpkg

```powershell
# Install dependencies (this may take 30-60 minutes)
cd C:\vcpkg

# Install required packages
.\vcpkg install boost:x64-windows
.\vcpkg install openssl:x64-windows
.\vcpkg install rocksdb:x64-windows
.\vcpkg install libevent:x64-windows
.\vcpkg install zeromq:x64-windows
.\vcpkg install protobuf:x64-windows
.\vcpkg install qt6-base:x64-windows
.\vcpkg install qt6-tools:x64-windows
.\vcpkg install qt6-svg:x64-windows
.\vcpkg install qrencode:x64-windows

# Optional: miniupnpc for UPnP
.\vcpkg install miniupnpc:x64-windows
```

### 4. Install Git (if not included with Visual Studio)

Download from: https://git-scm.com/download/win

Install with default options.

### 5. Install CMake (if not included with Visual Studio)

Download from: https://cmake.org/download/

During installation, select **"Add CMake to system PATH"**.

---

## Building with Visual Studio

### Method 1: Using Visual Studio IDE

#### Step 1: Clone Repository

```powershell
# Open PowerShell or Git Bash
cd C:\
git clone https://github.com/intcoin/intcoin.git
cd intcoin
git checkout v1.3.0-beta
```

#### Step 2: Open in Visual Studio

1. Launch **Visual Studio 2022**
2. Select **"Open a local folder"**
3. Navigate to `C:\intcoin` and click **Select Folder**
4. Visual Studio will automatically detect `CMakeLists.txt` and configure CMake

#### Step 3: Configure CMake

Visual Studio will show CMake output in the **Output** window. Wait for configuration to complete.

If you need to change options:
1. Go to **Project > CMake Settings for intcoin**
2. Modify options as needed
3. Save (Visual Studio will reconfigure automatically)

**Common Options**:
```json
{
  "configurations": [
    {
      "name": "x64-Release",
      "generator": "Ninja",
      "configurationType": "Release",
      "buildRoot": "${projectDir}\\out\\build\\${name}",
      "installRoot": "${projectDir}\\out\\install\\${name}",
      "cmakeCommandArgs": "-DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake",
      "buildCommandArgs": "",
      "ctestCommandArgs": "",
      "variables": [
        {
          "name": "BUILD_QT_WALLET",
          "value": "ON",
          "type": "BOOL"
        },
        {
          "name": "BUILD_DAEMON",
          "value": "ON",
          "type": "BOOL"
        },
        {
          "name": "BUILD_CLI",
          "value": "ON",
          "type": "BOOL"
        },
        {
          "name": "BUILD_TESTS",
          "value": "ON",
          "type": "BOOL"
        }
      ]
    }
  ]
}
```

#### Step 4: Build

1. Select **x64-Release** configuration (top toolbar)
2. Go to **Build > Build All** (or press `Ctrl+Shift+B`)
3. Wait for build to complete (10-30 minutes first time)

Build output will be in: `C:\intcoin\out\build\x64-Release\`

#### Step 5: Run Tests (Optional)

1. Go to **Test > Run All Tests**
2. View results in **Test Explorer**

### Method 2: Using Command Line (MSBuild)

```powershell
# Open "Developer PowerShell for VS 2022"

# Navigate to repository
cd C:\intcoin

# Create build directory
mkdir build
cd build

# Configure with CMake
cmake .. `
    -G "Visual Studio 17 2022" `
    -A x64 `
    -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake `
    -DCMAKE_BUILD_TYPE=Release `
    -DBUILD_QT_WALLET=ON `
    -DBUILD_DAEMON=ON `
    -DBUILD_CLI=ON `
    -DBUILD_TESTS=ON

# Build
cmake --build . --config Release -j 8

# Run tests
ctest -C Release --output-on-failure

# View binaries
dir Release\*.exe
```

---

## Building with MinGW

### 1. Install MSYS2

Download from: https://www.msys2.org/

Install to `C:\msys64` (default).

### 2. Install Dependencies

```bash
# Open MSYS2 MinGW 64-bit terminal

# Update package database
pacman -Syu

# Install build tools
pacman -S --needed \
    mingw-w64-x86_64-gcc \
    mingw-w64-x86_64-cmake \
    mingw-w64-x86_64-make \
    mingw-w64-x86_64-ninja \
    git

# Install dependencies
pacman -S --needed \
    mingw-w64-x86_64-qt6-base \
    mingw-w64-x86_64-qt6-tools \
    mingw-w64-x86_64-qt6-svg \
    mingw-w64-x86_64-boost \
    mingw-w64-x86_64-openssl \
    mingw-w64-x86_64-rocksdb \
    mingw-w64-x86_64-libevent \
    mingw-w64-x86_64-zeromq \
    mingw-w64-x86_64-protobuf \
    mingw-w64-x86_64-qrencode \
    mingw-w64-x86_64-miniupnpc
```

### 3. Build

```bash
# Clone repository
cd /c/
git clone https://github.com/intcoin/intcoin.git
cd intcoin
git checkout v1.3.0-beta

# Create build directory
mkdir build
cd build

# Configure
cmake .. \
    -G "Ninja" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/mingw64 \
    -DBUILD_QT_WALLET=ON \
    -DBUILD_DAEMON=ON \
    -DBUILD_CLI=ON \
    -DBUILD_TESTS=ON

# Build
ninja

# Run tests
ctest --output-on-failure

# View binaries
ls -lh *.exe
```

---

## Building the Installer

### Prerequisites

Install **NSIS** (Nullsoft Scriptable Install System):
https://nsis.sourceforge.io/Download

### Create Installer Script

Create `installer.nsi` in the project root:

```nsis
; INTcoin v1.3.0-beta Installer Script
!include "MUI2.nsh"

Name "INTcoin 1.3.0-beta"
OutFile "INTcoin-1.3.0-beta-Setup.exe"
InstallDir "$PROGRAMFILES64\INTcoin"
RequestExecutionLevel admin

!define MUI_ICON "src\qt\res\icons\intcoin.ico"
!define MUI_UNICON "src\qt\res\icons\intcoin.ico"

!insertmacro MUI_PAGE_LICENSE "LICENSE"
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

!insertmacro MUI_LANGUAGE "English"

Section "Install"
    SetOutPath "$INSTDIR"

    ; Copy binaries
    File "build\Release\intcoin-qt.exe"
    File "build\Release\intcoind.exe"
    File "build\Release\intcoin-cli.exe"

    ; Copy Qt6 DLLs
    File "C:\vcpkg\installed\x64-windows\bin\Qt6Core.dll"
    File "C:\vcpkg\installed\x64-windows\bin\Qt6Gui.dll"
    File "C:\vcpkg\installed\x64-windows\bin\Qt6Widgets.dll"
    File "C:\vcpkg\installed\x64-windows\bin\Qt6Network.dll"

    ; Copy other DLLs
    File "C:\vcpkg\installed\x64-windows\bin\*.dll"

    ; Create shortcuts
    CreateDirectory "$SMPROGRAMS\INTcoin"
    CreateShortcut "$SMPROGRAMS\INTcoin\INTcoin.lnk" "$INSTDIR\intcoin-qt.exe"
    CreateShortcut "$DESKTOP\INTcoin.lnk" "$INSTDIR\intcoin-qt.exe"

    ; Write uninstaller
    WriteUninstaller "$INSTDIR\Uninstall.exe"
    CreateShortcut "$SMPROGRAMS\INTcoin\Uninstall.lnk" "$INSTDIR\Uninstall.exe"

    ; Registry keys
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\INTcoin" \
        "DisplayName" "INTcoin 1.3.0-beta"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\INTcoin" \
        "UninstallString" "$INSTDIR\Uninstall.exe"
SectionEnd

Section "Uninstall"
    Delete "$INSTDIR\*.exe"
    Delete "$INSTDIR\*.dll"
    Delete "$SMPROGRAMS\INTcoin\*.lnk"
    Delete "$DESKTOP\INTcoin.lnk"
    RMDir "$SMPROGRAMS\INTcoin"
    RMDir "$INSTDIR"

    DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\INTcoin"
SectionEnd
```

### Build Installer

```powershell
# Compile installer
makensis installer.nsi

# Output: INTcoin-1.3.0-beta-Setup.exe
```

---

## Running INTcoin

### GUI Wallet

```powershell
# From build directory
cd C:\intcoin\build\Release
.\intcoin-qt.exe

# Or if installed
C:\Program Files\INTcoin\intcoin-qt.exe
```

### Daemon

```powershell
# Start daemon
.\intcoind.exe -daemon

# Check status
.\intcoin-cli.exe getblockchaininfo

# Stop daemon
.\intcoin-cli.exe stop
```

### Data Directory

Default data directory:
```
C:\Users\<YourUsername>\AppData\Roaming\INTcoin\
```

Custom data directory:
```powershell
.\intcoin-qt.exe -datadir=D:\INTcoin\data
```

---

## Troubleshooting

### Issue: "VCRUNTIME140.dll not found"

**Solution**:
Install **Visual C++ Redistributable 2022**:
https://aka.ms/vs/17/release/vc_redist.x64.exe

### Issue: "Qt6Core.dll not found"

**Solution**:
Copy Qt6 DLLs from vcpkg:
```powershell
cd C:\intcoin\build\Release
copy C:\vcpkg\installed\x64-windows\bin\Qt6*.dll .
copy C:\vcpkg\installed\x64-windows\bin\*.dll .
```

### Issue: CMake can't find dependencies

**Solution**:
```powershell
# Ensure vcpkg toolchain is specified
cmake .. -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake

# Or set environment variable
$env:VCPKG_ROOT = "C:\vcpkg"
```

### Issue: Build fails with C++23 errors

**Solution**:
```powershell
# Update Visual Studio to latest version
# Tools > Get Tools and Features > Update

# Or specify C++ standard explicitly
cmake .. -DCMAKE_CXX_STANDARD=23
```

### Issue: Out of memory during build

**Solution**:
```powershell
# Reduce parallel jobs
cmake --build . --config Release -j 2

# Or increase virtual memory in Windows settings
```

### Issue: RocksDB build fails

**Solution**:
```powershell
# Reinstall RocksDB via vcpkg
cd C:\vcpkg
.\vcpkg remove rocksdb:x64-windows
.\vcpkg install rocksdb:x64-windows --recurse
```

### Issue: Tests fail to run

**Solution**:
```powershell
# Ensure DLLs are in same directory as test executables
cd C:\intcoin\build\Release
copy C:\vcpkg\installed\x64-windows\bin\*.dll tests\

# Run tests
ctest -C Release -V
```

---

## Performance Optimization

### Build Performance

```powershell
# Use Ninja generator (faster than MSBuild)
cmake .. -G "Ninja" -DCMAKE_BUILD_TYPE=Release

# Increase parallel jobs (if you have enough RAM)
cmake --build . -j 16

# Use ccache (if installed)
cmake .. -DCMAKE_CXX_COMPILER_LAUNCHER=ccache
```

### Runtime Performance

Create `intcoin.conf` in data directory:
```ini
# Performance
dbcache=4096
maxmempool=300
par=8

# AssumeUTXO fast sync
assumevalid=1
```

---

## Packaging for Distribution

### Create Portable ZIP

```powershell
# Copy binaries and DLLs
mkdir INTcoin-Portable
copy build\Release\*.exe INTcoin-Portable\
copy C:\vcpkg\installed\x64-windows\bin\*.dll INTcoin-Portable\

# Create README
echo "INTcoin v1.3.0-beta Portable" > INTcoin-Portable\README.txt

# Create ZIP
Compress-Archive -Path INTcoin-Portable -DestinationPath INTcoin-1.3.0-beta-Win64-Portable.zip
```

### Code Signing (Optional)

```powershell
# Requires code signing certificate
signtool sign /f cert.pfx /p password /tr http://timestamp.digicert.com INTcoin-1.3.0-beta-Setup.exe
```

---

## Cross-Compiling for Windows on Linux

If you prefer to build Windows binaries on Linux:

```bash
# Install MinGW cross-compiler
sudo apt install mingw-w64

# Configure for cross-compile
cmake .. \
    -DCMAKE_TOOLCHAIN_FILE=../cmake/toolchains/mingw-w64.cmake \
    -DCMAKE_BUILD_TYPE=Release

# Build
make -j$(nproc)
```

---

## Additional Resources

- **Visual Studio Documentation**: https://docs.microsoft.com/en-us/visualstudio/
- **vcpkg Documentation**: https://github.com/microsoft/vcpkg
- **CMake Documentation**: https://cmake.org/documentation/
- **INTcoin GitHub**: https://github.com/intcoin/intcoin

---

## Security Notes

1. **Download Visual Studio from official Microsoft site only**
2. **Verify git repository authenticity** before building
3. **Use Windows Defender** or reputable antivirus
4. **Keep Windows and Visual Studio updated**
5. **Build in isolated environment** for production releases

---

**Build Guide Version**: 1.0
**For INTcoin**: v1.3.0-beta
**Platform**: Windows 10/11
**Last Updated**: January 3, 2026
