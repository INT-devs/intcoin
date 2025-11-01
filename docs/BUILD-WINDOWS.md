# Building INTcoin on Windows

**Copyright (c) 2025 INTcoin Core (Maddison Lane)**

This guide explains how to build INTcoin on Windows and create a distributable `.exe` installer.

## Prerequisites

### Required Software

1. **Visual Studio 2022 or later** (Community Edition is free)
   - Download: https://visualstudio.microsoft.com/downloads/
   - During installation, select:
     - Desktop development with C++
     - C++ CMake tools for Windows
     - C++ Clang tools for Windows (optional but recommended)

2. **CMake 3.20 or later**
   - Download: https://cmake.org/download/
   - Or install via Visual Studio installer

3. **Git for Windows**
   - Download: https://git-scm.com/download/win

4. **Python 3.12 or later**
   - Download: https://www.python.org/downloads/
   - Check "Add Python to PATH" during installation

### Optional but Recommended

5. **vcpkg** (Package Manager)
   - Clone: `git clone https://github.com/Microsoft/vcpkg.git`
   - Bootstrap: `.\vcpkg\bootstrap-vcpkg.bat`

6. **Qt 5.15+**
   - Install via vcpkg: `vcpkg install qt5:x64-windows`
   - Or download installer: https://www.qt.io/download

## Building with vcpkg (Recommended)

### Step 1: Install Dependencies via vcpkg

```powershell
# Navigate to vcpkg directory
cd C:\vcpkg

# Install dependencies
.\vcpkg.exe install boost:x64-windows
.\vcpkg.exe install openssl:x64-windows
.\vcpkg.exe install qt5:x64-windows

# Integrate with Visual Studio
.\vcpkg.exe integrate install
```

### Step 2: Clone INTcoin Repository

```powershell
# Navigate to your projects directory
cd C:\Projects

# Clone the repository
git clone https://gitlab.com/intcoin/crypto.git
cd crypto

# Initialize submodules (for external dependencies)
git submodule update --init --recursive
```

### Step 3: Configure with CMake

```powershell
# Create build directory
mkdir build
cd build

# Configure (adjust vcpkg path if needed)
cmake .. -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake ^
         -DCMAKE_BUILD_TYPE=Release ^
         -G "Visual Studio 17 2022" ^
         -A x64

# Or use CMake GUI for easier configuration
cmake-gui ..
```

### Step 4: Build

```powershell
# Build using CMake
cmake --build . --config Release -j 8

# Or open in Visual Studio
start INTcoin.sln
```

### Step 5: Run Tests

```powershell
# Run C++ tests
ctest -C Release

# Run Python tests
cd ..\tests
python -m pytest
```

## Building with Visual Studio GUI

### Method 1: CMake Project

1. Open Visual Studio 2022
2. File → Open → CMake...
3. Navigate to INTcoin directory and open `CMakeLists.txt`
4. Visual Studio will automatically configure the project
5. Build → Build All (Ctrl+Shift+B)

### Method 2: Generate Solution File

1. Open CMake GUI
2. Set source directory to INTcoin folder
3. Set build directory to `INTcoin/build`
4. Click "Configure" → Select "Visual Studio 17 2022"
5. Click "Generate"
6. Open `INTcoin.sln` in Visual Studio

## Creating Windows Installer (.exe)

### Using CPack (Built-in)

After building, create installer:

```powershell
cd build
cpack -G NSIS -C Release
```

This creates:
- `INTcoin-0.1.0-win64.exe` (NSIS installer)
- `INTcoin-0.1.0-win64.zip` (Portable zip)

### Using NSIS Manually

1. **Install NSIS**
   - Download: https://nsis.sourceforge.io/Download

2. **Create installer script** (`installer.nsi`):

```nsis
!define PRODUCT_NAME "INTcoin"
!define PRODUCT_VERSION "0.1.0"
!define PRODUCT_PUBLISHER "INTcoin Core (Maddison Lane)"
!define PRODUCT_WEB_SITE "https://international-coin.org"

!include "MUI2.nsh"

Name "${PRODUCT_NAME} ${PRODUCT_VERSION}"
OutFile "INTcoin-Setup-${PRODUCT_VERSION}.exe"
InstallDir "$PROGRAMFILES64\INTcoin"

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "..\COPYING"
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_LANGUAGE "English"

Section "INTcoin Core" SEC01
  SetOutPath "$INSTDIR"
  File "Release\intcoind.exe"
  File "Release\intcoin-cli.exe"
  File "Release\intcoin-qt.exe"
  File "Release\intcoin-miner.exe"

  # DLL dependencies
  File "Release\*.dll"

  # Config files
  SetOutPath "$INSTDIR\config"
  File /r "..\config\*.*"

  # Create shortcuts
  CreateDirectory "$SMPROGRAMS\INTcoin"
  CreateShortCut "$SMPROGRAMS\INTcoin\INTcoin Wallet.lnk" "$INSTDIR\intcoin-qt.exe"
  CreateShortCut "$DESKTOP\INTcoin Wallet.lnk" "$INSTDIR\intcoin-qt.exe"
SectionEnd

Section "Uninstall"
  Delete "$INSTDIR\*.*"
  RMDir /r "$INSTDIR"
  Delete "$SMPROGRAMS\INTcoin\*.*"
  RMDir "$SMPROGRAMS\INTcoin"
  Delete "$DESKTOP\INTcoin Wallet.lnk"
SectionEnd
```

3. **Compile installer**:

```powershell
"C:\Program Files (x86)\NSIS\makensis.exe" installer.nsi
```

## Troubleshooting

### Qt Not Found

If CMake can't find Qt:

```powershell
# Set Qt path
set Qt5_DIR=C:\Qt\5.15.2\msvc2019_64\lib\cmake\Qt5
cmake .. -DCMAKE_PREFIX_PATH=C:\Qt\5.15.2\msvc2019_64
```

### OpenSSL Errors

Install OpenSSL via vcpkg or download from:
https://slproweb.com/products/Win32OpenSSL.html

### Boost Link Errors

Ensure you're using matching architectures (x64) and build types (Release/Debug).

### Python Tests Fail

Install test dependencies:

```powershell
pip install pytest pytest-asyncio
```

## Build Options

```powershell
# Disable Qt wallet
cmake .. -DBUILD_QT_WALLET=OFF

# Disable tests
cmake .. -DBUILD_TESTS=OFF

# Debug build
cmake .. -DCMAKE_BUILD_TYPE=Debug

# Enable all features
cmake .. -DBUILD_QT_WALLET=ON ^
         -DBUILD_DAEMON=ON ^
         -DBUILD_CLI=ON ^
         -DBUILD_MINER=ON ^
         -DBUILD_EXPLORER=ON
```

## Performance Optimization

For maximum performance:

```powershell
cmake .. -DCMAKE_BUILD_TYPE=Release ^
         -DCMAKE_CXX_FLAGS="/O2 /GL /arch:AVX2" ^
         -DCMAKE_EXE_LINKER_FLAGS="/LTCG"
```

## Cross-Compiling for ARM64

```powershell
cmake .. -G "Visual Studio 17 2022" -A ARM64
```

## Static Linking

For portable executable without DLL dependencies:

```powershell
cmake .. -DVCPKG_TARGET_TRIPLET=x64-windows-static
```

## Continuous Integration

Example GitHub Actions workflow:

```yaml
name: Windows Build

on: [push, pull_request]

jobs:
  build:
    runs-on: windows-latest
    steps:
      - uses: actions/checkout@v3
      - uses: microsoft/setup-msbuild@v1
      - name: Setup vcpkg
        run: |
          git clone https://github.com/Microsoft/vcpkg.git
          .\vcpkg\bootstrap-vcpkg.bat
          .\vcpkg\vcpkg integrate install
      - name: Install dependencies
        run: |
          .\vcpkg\vcpkg install boost:x64-windows openssl:x64-windows qt5:x64-windows
      - name: Configure
        run: |
          mkdir build
          cd build
          cmake .. -DCMAKE_TOOLCHAIN_FILE=..\vcpkg\scripts\buildsystems\vcpkg.cmake
      - name: Build
        run: cmake --build build --config Release
      - name: Test
        run: cd build && ctest -C Release
      - name: Package
        run: cd build && cpack -G NSIS -C Release
```

## Recommended IDE Setup

### Visual Studio Code

Install extensions:
- C/C++ (Microsoft)
- CMake Tools (Microsoft)
- CMake (twxs)

Configure `.vscode/settings.json`:

```json
{
    "cmake.configureArgs": [
        "-DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake"
    ],
    "cmake.buildDirectory": "${workspaceFolder}/build",
    "C_Cpp.default.configurationProvider": "ms-vscode.cmake-tools"
}
```

## Additional Resources

- Visual Studio: https://visualstudio.microsoft.com/
- vcpkg: https://vcpkg.io/
- CMake documentation: https://cmake.org/documentation/
- NSIS documentation: https://nsis.sourceforge.io/Docs/

## Support

For build issues:
- Email: team@international-coin.org
- GitLab: https://gitlab.com/intcoin/crypto/issues

---

**Last Updated**: January 2025
