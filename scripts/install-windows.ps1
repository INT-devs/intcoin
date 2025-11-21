# Copyright (c) 2025 INTcoin Core (Maddison Lane)
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

# INTcoin Installation Script for Windows
# Requires: Windows 10/11, PowerShell 5.1+, Administrator privileges

#Requires -RunAsAdministrator

param(
    [switch]$SkipDependencies,
    [switch]$BuildOnly,
    [string]$InstallPath = "$env:ProgramFiles\INTcoin",
    [string]$DataPath = "$env:APPDATA\INTcoin"
)

# Colors
$script:ColorScheme = @{
    Header = 'Cyan'
    Step = 'Yellow'
    Success = 'Green'
    Error = 'Red'
    Info = 'Blue'
}

# Print functions
function Write-Header {
    param([string]$Message)
    Write-Host "================================" -ForegroundColor $ColorScheme.Header
    Write-Host $Message -ForegroundColor $ColorScheme.Header
    Write-Host "================================" -ForegroundColor $ColorScheme.Header
}

function Write-Step {
    param([string]$Step, [string]$Message)
    Write-Host "[$Step] $Message" -ForegroundColor $ColorScheme.Step
}

function Write-Success {
    param([string]$Message)
    Write-Host "✓ $Message" -ForegroundColor $ColorScheme.Success
}

function Write-ErrorMsg {
    param([string]$Message)
    Write-Host "✗ $Message" -ForegroundColor $ColorScheme.Error
}

function Write-InfoMsg {
    param([string]$Message)
    Write-Host "ℹ $Message" -ForegroundColor $ColorScheme.Info
}

# Check prerequisites
function Test-Prerequisites {
    Write-Step "1/8" "Checking prerequisites..."

    # Check Windows version
    $osVersion = [System.Environment]::OSVersion.Version
    if ($osVersion.Major -lt 10) {
        Write-ErrorMsg "Windows 10 or later is required"
        exit 1
    }
    Write-Success "Windows version: $($osVersion.Major).$($osVersion.Minor)"

    # Check PowerShell version
    if ($PSVersionTable.PSVersion.Major -lt 5) {
        Write-ErrorMsg "PowerShell 5.1 or later is required"
        exit 1
    }
    Write-Success "PowerShell version: $($PSVersionTable.PSVersion)"

    # Check for Git
    if (!(Get-Command git -ErrorAction SilentlyContinue)) {
        Write-ErrorMsg "Git is not installed. Please install Git for Windows from https://git-scm.com/"
        exit 1
    }
    Write-Success "Git found"

    # Check for CMake
    if (!(Get-Command cmake -ErrorAction SilentlyContinue)) {
        Write-ErrorMsg "CMake is not installed. Please install from https://cmake.org/download/"
        exit 1
    }
    Write-Success "CMake found"

    Write-Success "Prerequisites OK"
}

# Install dependencies via vcpkg
function Install-Dependencies {
    if ($SkipDependencies) {
        Write-Step "2/8" "Skipping dependencies (--SkipDependencies)"
        return
    }

    Write-Step "2/8" "Installing dependencies..."

    # Check for vcpkg
    $vcpkgPath = "C:\vcpkg"
    if (!(Test-Path $vcpkgPath)) {
        Write-InfoMsg "Installing vcpkg..."

        # Clone vcpkg
        Push-Location C:\
        git clone https://github.com/Microsoft/vcpkg.git
        Pop-Location

        # Bootstrap vcpkg
        & "$vcpkgPath\bootstrap-vcpkg.bat"

        Write-Success "vcpkg installed"
    } else {
        Write-InfoMsg "vcpkg already installed at $vcpkgPath"
        # Update vcpkg
        Push-Location $vcpkgPath
        git pull
        Pop-Location
    }

    # Install required packages
    Write-InfoMsg "Installing vcpkg packages (this may take 30+ minutes)..."

    $packages = @(
        "boost-system:x64-windows"
        "boost-filesystem:x64-windows"
        "boost-thread:x64-windows"
        "boost-program-options:x64-windows"
        "openssl:x64-windows"
        "sqlite3:x64-windows"
        "zeromq:x64-windows"
        "libevent:x64-windows"
        "qt5-base:x64-windows"
        "qt5-tools:x64-windows"
        "qt5-svg:x64-windows"
    )

    foreach ($package in $packages) {
        Write-InfoMsg "Installing $package..."
        & "$vcpkgPath\vcpkg.exe" install $package
    }

    # Integrate vcpkg
    & "$vcpkgPath\vcpkg.exe" integrate install

    Write-Success "Dependencies installed"
}

# Clone repository
function Get-Repository {
    Write-Step "3/8" "Cloning INTcoin repository..."

    $buildDir = "$env:USERPROFILE\intcoin-build"

    if (Test-Path $buildDir) {
        Write-InfoMsg "Repository already exists. Pulling latest changes..."
        Push-Location $buildDir
        git pull
        Pop-Location
    } else {
        git clone https://gitlab.com/intcoin/crypto.git $buildDir
    }

    Write-Success "Repository ready at $buildDir"
    return $buildDir
}

# Build INTcoin
function Build-INTcoin {
    param([string]$RepoPath)

    Write-Step "4/8" "Building INTcoin..."

    Push-Location $RepoPath

    # Clean previous build
    if (Test-Path "build") {
        Remove-Item -Recurse -Force build
    }

    New-Item -ItemType Directory -Path "build" | Out-Null
    Push-Location build

    # Configure
    Write-InfoMsg "Configuring with CMake..."
    $vcpkgPath = "C:\vcpkg"
    & cmake .. `
        -DCMAKE_TOOLCHAIN_FILE="$vcpkgPath\scripts\buildsystems\vcpkg.cmake" `
        -DCMAKE_BUILD_TYPE=Release `
        -DBUILD_QT_WALLET=ON `
        -DENABLE_LIGHTNING=ON `
        -DBUILD_TESTS=ON `
        -G "Visual Studio 17 2022" `
        -A x64

    if ($LASTEXITCODE -ne 0) {
        Pop-Location
        Pop-Location
        Write-ErrorMsg "CMake configuration failed"
        exit 1
    }

    # Build
    Write-InfoMsg "Compiling (this may take 10-20 minutes)..."
    & cmake --build . --config Release -j $env:NUMBER_OF_PROCESSORS

    if ($LASTEXITCODE -ne 0) {
        Pop-Location
        Pop-Location
        Write-ErrorMsg "Build failed"
        exit 1
    }

    Pop-Location
    Pop-Location

    Write-Success "Build complete"
    return "$RepoPath\build\Release"
}

# Run tests
function Test-Build {
    param([string]$BuildPath)

    Write-Step "5/8" "Running tests..."

    Push-Location (Split-Path $BuildPath)

    & ctest -C Release --output-on-failure

    if ($LASTEXITCODE -ne 0) {
        Write-ErrorMsg "Some tests failed (non-critical, continuing installation)"
    } else {
        Write-Success "All tests passed"
    }

    Pop-Location
}

# Install binaries
function Install-Binaries {
    param([string]$BuildPath)

    Write-Step "6/8" "Installing binaries..."

    # Create installation directory
    if (!(Test-Path $InstallPath)) {
        New-Item -ItemType Directory -Path $InstallPath | Out-Null
    }

    # Copy binaries
    $binaries = @(
        "intcoind.exe"
        "intcoin-cli.exe"
        "intcoin-wallet.exe"
        "intcoin-miner.exe"
    )

    foreach ($binary in $binaries) {
        $source = Join-Path $BuildPath $binary
        if (Test-Path $source) {
            Copy-Item $source $InstallPath -Force
            Write-Success "Installed $binary"
        }
    }

    # Copy GUI wallet if built
    $qtWallet = Join-Path $BuildPath "intcoin-qt.exe"
    if (Test-Path $qtWallet) {
        Copy-Item $qtWallet $InstallPath -Force
        Write-Success "Installed intcoin-qt.exe (GUI wallet)"
    }

    # Copy required DLLs
    Write-InfoMsg "Copying required DLLs..."
    $vcpkgPath = "C:\vcpkg\installed\x64-windows\bin"
    if (Test-Path $vcpkgPath) {
        Get-ChildItem "$vcpkgPath\*.dll" | Copy-Item -Destination $InstallPath -Force
    }

    Write-Success "Binaries installed to $InstallPath"
}

# Add to PATH
function Add-ToPath {
    Write-Step "7/8" "Adding to system PATH..."

    $currentPath = [Environment]::GetEnvironmentVariable("Path", "Machine")

    if ($currentPath -notlike "*$InstallPath*") {
        $newPath = "$currentPath;$InstallPath"
        [Environment]::SetEnvironmentVariable("Path", $newPath, "Machine")
        $env:Path = $newPath
        Write-Success "Added to system PATH"
    } else {
        Write-InfoMsg "Already in system PATH"
    }
}

# Create data directory and config
function Initialize-DataDirectory {
    Write-Step "8/8" "Creating data directory..."

    if (!(Test-Path $DataPath)) {
        New-Item -ItemType Directory -Path $DataPath | Out-Null
        Write-Success "Created $DataPath"
    } else {
        Write-InfoMsg "Data directory already exists"
    }

    # Create default config
    $configPath = Join-Path $DataPath "intcoin.conf"
    if (!(Test-Path $configPath)) {
        $rpcPassword = -join ((65..90) + (97..122) + (48..57) | Get-Random -Count 32 | ForEach-Object {[char]$_})

        @"
# INTcoin Configuration File
# Generated by install script

# Server mode
server=1

# RPC settings
rpcuser=intcoinrpc
rpcpassword=$rpcPassword
rpcallowip=127.0.0.1
rpcport=9334

# Network settings
listen=1
maxconnections=125

# Mining settings
# gen=0
# genproclimit=4

# Data directory
datadir=$DataPath

# Logging
debug=0
printtoconsole=0
"@ | Out-File -FilePath $configPath -Encoding utf8

        Write-Success "Created default configuration file"
    } else {
        Write-InfoMsg "Configuration file already exists"
    }

    # Create desktop shortcut for GUI wallet
    $qtWallet = Join-Path $InstallPath "intcoin-qt.exe"
    if (Test-Path $qtWallet) {
        $WshShell = New-Object -ComObject WScript.Shell
        $Shortcut = $WshShell.CreateShortcut("$env:USERPROFILE\Desktop\INTcoin Wallet.lnk")
        $Shortcut.TargetPath = $qtWallet
        $Shortcut.WorkingDirectory = $InstallPath
        $Shortcut.Description = "INTcoin Quantum-Resistant Cryptocurrency Wallet"
        $Shortcut.Save()
        Write-Success "Created desktop shortcut"
    }
}

# Print summary
function Show-Summary {
    Write-Host ""
    Write-Header "Installation Complete!"
    Write-Host ""
    Write-Host "INTcoin has been successfully installed!" -ForegroundColor Green
    Write-Host ""
    Write-Host "Installed binaries:" -ForegroundColor Yellow
    Write-Host "  • intcoind.exe         - Daemon"
    Write-Host "  • intcoin-cli.exe      - Command-line interface"
    Write-Host "  • intcoin-wallet.exe   - Wallet management tool"
    Write-Host "  • intcoin-miner.exe    - CPU miner"

    if (Test-Path "$InstallPath\intcoin-qt.exe") {
        Write-Host "  • intcoin-qt.exe       - GUI wallet"
    }

    Write-Host ""
    Write-Host "Installation paths:" -ForegroundColor Yellow
    Write-Host "  • Program files: $InstallPath"
    Write-Host "  • Data directory: $DataPath"
    Write-Host "  • Config file: $DataPath\intcoin.conf"
    Write-Host ""
    Write-Host "Quick start commands (PowerShell):" -ForegroundColor Yellow
    Write-Host "  # Start GUI wallet (double-click desktop shortcut or run)"
    Write-Host "  PS> intcoin-qt.exe"
    Write-Host ""
    Write-Host "  # Or start daemon"
    Write-Host "  PS> intcoind.exe -daemon"
    Write-Host ""
    Write-Host "  # Check blockchain status"
    Write-Host "  PS> intcoin-cli.exe getblockchaininfo"
    Write-Host ""
    Write-Host "  # Get new address"
    Write-Host "  PS> intcoin-cli.exe getnewaddress"
    Write-Host ""
    Write-Host "  # Start mining"
    Write-Host "  PS> intcoin-cli.exe startmining 4"
    Write-Host ""
    Write-Host "Windows Service (optional):" -ForegroundColor Yellow
    Write-Host "  You can install INTcoin as a Windows service using NSSM:"
    Write-Host "  1. Download NSSM from https://nssm.cc/"
    Write-Host "  2. Run: nssm install intcoind `"$InstallPath\intcoind.exe`""
    Write-Host ""
    Write-Host "Documentation:" -ForegroundColor Yellow
    Write-Host "  • Website: https://international-coin.org"
    Write-Host "  • Repository: https://gitlab.com/intcoin/crypto"
    Write-Host "  • Wiki: https://gitlab.com/intcoin/crypto/-/wikis/home"
    Write-Host ""
    Write-Host "Thank you for installing INTcoin!" -ForegroundColor Green
    Write-Host ""
}

# Main installation flow
function Main {
    Write-Header "INTcoin Installation Script for Windows"
    Write-Host ""
    Write-InfoMsg "This script will install INTcoin on your system"
    Write-InfoMsg "Supported: Windows 10/11"
    Write-Host ""

    Test-Prerequisites

    if (!$BuildOnly) {
        Install-Dependencies
    }

    $repoPath = Get-Repository
    $buildPath = Build-INTcoin -RepoPath $repoPath

    Test-Build -BuildPath $buildPath
    Install-Binaries -BuildPath $buildPath
    Add-ToPath
    Initialize-DataDirectory

    Show-Summary

    Write-Host ""
    Write-InfoMsg "Build directory preserved at: $repoPath"
    Write-InfoMsg "You can safely delete it to free up disk space"
    Write-Host ""
}

# Run main
Main
