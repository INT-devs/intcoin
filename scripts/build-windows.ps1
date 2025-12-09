# INTcoin Windows Build Script
# Copyright (c) 2025 INTcoin Team (Neil Adamson)

#Requires -RunAsAdministrator

param(
    [string]$InstallPrefix = "C:\Program Files\INTcoin",
    [string]$BuildType = "Release",
    [string]$VcpkgRoot = "C:\vcpkg",
    [switch]$SkipDependencies,
    [switch]$SkipTests
)

$ErrorActionPreference = "Stop"

# Configuration
$LIBOQS_VERSION = "0.15.0"
$RANDOMX_VERSION = "v1.2.1"
$TEMP_DIR = "$env:TEMP\intcoin-build-$PID"

# Colors for output
function Write-Info {
    param([string]$Message)
    Write-Host "[INFO] $Message" -ForegroundColor Cyan
}

function Write-Success {
    param([string]$Message)
    Write-Host "[SUCCESS] $Message" -ForegroundColor Green
}

function Write-Warning {
    param([string]$Message)
    Write-Host "[WARNING] $Message" -ForegroundColor Yellow
}

function Write-Error {
    param([string]$Message)
    Write-Host "[ERROR] $Message" -ForegroundColor Red
}

# Print banner
function Print-Banner {
    Write-Host ""
    Write-Host "╔═══════════════════════════════════════════════════════╗" -ForegroundColor Blue
    Write-Host "║        INTcoin Windows Build Script                  ║" -ForegroundColor Blue
    Write-Host "║                                                       ║" -ForegroundColor Blue
    Write-Host "║  This script will build INTcoin for Windows           ║" -ForegroundColor Blue
    Write-Host "╚═══════════════════════════════════════════════════════╝" -ForegroundColor Blue
    Write-Host ""
}

# Check prerequisites
function Test-Prerequisites {
    Write-Info "Checking prerequisites..."

    # Check Visual Studio
    $vsWhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    if (-not (Test-Path $vsWhere)) {
        throw "Visual Studio 2022 not found. Please install Visual Studio 2022 with C++ workload."
    }

    $vsPath = & $vsWhere -latest -property installationPath
    if (-not $vsPath) {
        throw "Visual Studio installation path not found."
    }

    Write-Success "Found Visual Studio at: $vsPath"

    # Check CMake
    if (-not (Get-Command cmake -ErrorAction SilentlyContinue)) {
        throw "CMake not found. Please install CMake and add it to PATH."
    }

    $cmakeVersion = (cmake --version | Select-String -Pattern "(\d+\.\d+\.\d+)").Matches.Groups[1].Value
    Write-Success "Found CMake version: $cmakeVersion"

    # Check Git
    if (-not (Get-Command git -ErrorAction SilentlyContinue)) {
        throw "Git not found. Please install Git and add it to PATH."
    }

    Write-Success "Prerequisites check passed"
}

# Install vcpkg dependencies
function Install-VcpkgDependencies {
    Write-Info "Installing dependencies via vcpkg..."

    if (-not (Test-Path $VcpkgRoot)) {
        Write-Info "vcpkg not found. Installing vcpkg..."
        git clone https://github.com/Microsoft/vcpkg.git $VcpkgRoot
        & "$VcpkgRoot\bootstrap-vcpkg.bat"
    }

    # Install dependencies
    $packages = @(
        "boost:x64-windows",
        "openssl:x64-windows",
        "rocksdb:x64-windows",
        "zeromq:x64-windows",
        "libevent:x64-windows"
    )

    foreach ($package in $packages) {
        Write-Info "Installing $package..."
        & "$VcpkgRoot\vcpkg.exe" install $package
    }

    # Integrate with Visual Studio
    & "$VcpkgRoot\vcpkg.exe" integrate install

    Write-Success "Dependencies installed"
}

# Build liboqs
function Build-Liboqs {
    Write-Info "Building liboqs $LIBOQS_VERSION..."

    Push-Location $TEMP_DIR

    if (-not (Test-Path "liboqs")) {
        git clone --depth 1 --branch $LIBOQS_VERSION https://github.com/open-quantum-safe/liboqs.git
    }

    Push-Location liboqs
    New-Item -ItemType Directory -Force -Path build | Out-Null
    Push-Location build

    cmake -G "Visual Studio 17 2022" -A x64 `
        -DCMAKE_BUILD_TYPE=$BuildType `
        -DBUILD_SHARED_LIBS=OFF `
        -DOQS_BUILD_ONLY_LIB=ON `
        ..

    cmake --build . --config $BuildType --parallel

    # Install
    cmake --install . --prefix "$env:ProgramFiles\liboqs" --config $BuildType

    Pop-Location
    Pop-Location
    Pop-Location

    Write-Success "liboqs built and installed"
}

# Build RandomX
function Build-RandomX {
    Write-Info "Building RandomX $RANDOMX_VERSION..."

    Push-Location $TEMP_DIR

    if (-not (Test-Path "RandomX")) {
        git clone --depth 1 --branch $RANDOMX_VERSION https://github.com/tevador/RandomX.git
    }

    Push-Location RandomX
    New-Item -ItemType Directory -Force -Path build | Out-Null
    Push-Location build

    cmake -G "Visual Studio 17 2022" -A x64 `
        -DCMAKE_BUILD_TYPE=$BuildType `
        ..

    cmake --build . --config $BuildType --parallel

    # Install
    cmake --install . --prefix "$env:ProgramFiles\RandomX" --config $BuildType

    Pop-Location
    Pop-Location
    Pop-Location

    Write-Success "RandomX built and installed"
}

# Build INTcoin
function Build-Intcoin {
    Write-Info "Building INTcoin..."

    # Get source directory (where this script is located)
    $SourceDir = Split-Path -Parent $MyInvocation.MyCommand.Path

    # Create build directory
    $BuildDir = Join-Path $SourceDir "build-windows"
    New-Item -ItemType Directory -Force -Path $BuildDir | Out-Null
    Push-Location $BuildDir

    # Configure
    cmake -G "Visual Studio 17 2022" -A x64 `
        -DCMAKE_BUILD_TYPE=$BuildType `
        -DCMAKE_TOOLCHAIN_FILE="$VcpkgRoot\scripts\buildsystems\vcpkg.cmake" `
        -DCMAKE_PREFIX_PATH="$env:ProgramFiles\liboqs;$env:ProgramFiles\RandomX" `
        ..

    # Build
    cmake --build . --config $BuildType --parallel

    # Run tests
    if (-not $SkipTests) {
        Write-Info "Running tests..."
        ctest -C $BuildType --output-on-failure
    }

    Pop-Location

    Write-Success "INTcoin built successfully"

    # Copy binaries
    $BinDir = Join-Path $BuildDir "$BuildType"
    Write-Info "Binaries located at: $BinDir"
    Write-Info "  - intcoind.exe"
    Write-Info "  - intcoin-cli.exe"
    Write-Info "  - intcoin-miner.exe"
}

# Create installer package
function Create-Installer {
    Write-Info "Creating installer package..."

    $BuildDir = Join-Path $PWD "build-windows"
    $BinDir = Join-Path $BuildDir "$BuildType"
    $PackageDir = Join-Path $BuildDir "package"

    # Create package directory structure
    New-Item -ItemType Directory -Force -Path "$PackageDir\bin" | Out-Null
    New-Item -ItemType Directory -Force -Path "$PackageDir\conf" | Out-Null
    New-Item -ItemType Directory -Force -Path "$PackageDir\doc" | Out-Null

    # Copy binaries
    Copy-Item "$BinDir\intcoind.exe" "$PackageDir\bin\"
    Copy-Item "$BinDir\intcoin-cli.exe" "$PackageDir\bin\"
    Copy-Item "$BinDir\intcoin-miner.exe" "$PackageDir\bin\"

    # Copy configuration template
    $ConfigContent = @"
# INTcoin Configuration File

# Network settings
#testnet=0
#port=9333
#rpcport=9332

# Connection settings
#rpcuser=intcoinrpc
#rpcpassword=changeme
#rpcallowip=127.0.0.1

# Data directory
#datadir=C:\Users\$env:USERNAME\AppData\Roaming\INTcoin

# Daemon settings
#daemon=1
#server=1

# Mining
#gen=0
#genproclimit=-1

# Logging
#debug=0
#printtoconsole=0
"@
    Set-Content -Path "$PackageDir\conf\intcoin.conf" -Value $ConfigContent

    # Copy documentation
    if (Test-Path "README.md") {
        Copy-Item "README.md" "$PackageDir\doc\"
    }

    # Create installer script
    $InstallerContent = @"
@echo off
echo INTcoin Windows Installer
echo.

set INSTALL_DIR=%ProgramFiles%\INTcoin
set DATA_DIR=%APPDATA%\INTcoin

echo Installing to: %INSTALL_DIR%
echo.

mkdir "%INSTALL_DIR%\bin"
mkdir "%DATA_DIR%"

copy /Y bin\*.exe "%INSTALL_DIR%\bin\"
copy /Y conf\intcoin.conf "%DATA_DIR%\"

echo.
echo Installation complete!
echo.
echo Binaries installed to: %INSTALL_DIR%\bin
echo Configuration file: %DATA_DIR%\intcoin.conf
echo.
echo Add to PATH: setx PATH "%PATH%;%INSTALL_DIR%\bin"
echo.
pause
"@
    Set-Content -Path "$PackageDir\install.bat" -Value $InstallerContent

    # Create ZIP package
    $ZipFile = Join-Path $BuildDir "intcoin-windows-x64.zip"
    if (Test-Path $ZipFile) {
        Remove-Item $ZipFile
    }

    Compress-Archive -Path "$PackageDir\*" -DestinationPath $ZipFile

    Write-Success "Installer package created: $ZipFile"
}

# Print summary
function Print-Summary {
    Write-Host ""
    Write-Host "╔═══════════════════════════════════════════════════════╗" -ForegroundColor Green
    Write-Host "║          Build Complete!                              ║" -ForegroundColor Green
    Write-Host "╚═══════════════════════════════════════════════════════╝" -ForegroundColor Green
    Write-Host ""
    Write-Host "Build artifacts:" -ForegroundColor Cyan
    Write-Host "  - Build directory: build-windows\$BuildType"
    Write-Host "  - Package: build-windows\intcoin-windows-x64.zip"
    Write-Host ""
    Write-Host "Binaries:" -ForegroundColor Cyan
    Write-Host "  - intcoind.exe      : Full node daemon"
    Write-Host "  - intcoin-cli.exe   : Command-line interface"
    Write-Host "  - intcoin-miner.exe : Mining software"
    Write-Host ""
    Write-Host "Next steps:" -ForegroundColor Cyan
    Write-Host "  1. Extract the ZIP package to your desired location"
    Write-Host "  2. Run install.bat to install system-wide"
    Write-Host "  3. Configure: Edit %APPDATA%\INTcoin\intcoin.conf"
    Write-Host "  4. Start daemon: intcoind.exe"
    Write-Host ""
    Write-Host "Documentation: https://code.intcoin.org/intcoin/core/wiki" -ForegroundColor Yellow
    Write-Host ""
}

# Main build function
function Main {
    Print-Banner

    try {
        # Check prerequisites
        Test-Prerequisites

        # Create temp directory
        New-Item -ItemType Directory -Force -Path $TEMP_DIR | Out-Null

        # Install dependencies
        if (-not $SkipDependencies) {
            Install-VcpkgDependencies
            Build-Liboqs
            Build-RandomX
        }

        # Build INTcoin
        Build-Intcoin

        # Create installer
        Create-Installer

        # Cleanup
        Write-Info "Cleaning up temporary files..."
        Remove-Item -Recurse -Force $TEMP_DIR -ErrorAction SilentlyContinue

        # Print summary
        Print-Summary

        exit 0
    }
    catch {
        Write-Error "Build failed: $_"
        Write-Host $_.ScriptStackTrace -ForegroundColor Red
        exit 1
    }
}

# Run main function
Main
