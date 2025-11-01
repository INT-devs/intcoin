# Copyright (c) 2025 INTcoin Core (Maddison Lane)
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

# INTcoin Windows Build Script
# Usage: .\build-windows.ps1 [-Clean] [-BuildType Release|Debug] [-WithInstaller]

param(
    [switch]$Clean,
    [ValidateSet('Release', 'Debug')]
    [string]$BuildType = 'Release',
    [switch]$WithInstaller,
    [string]$VcpkgPath = 'C:\vcpkg',
    [int]$Jobs = 8
)

Write-Host "================================" -ForegroundColor Cyan
Write-Host "INTcoin Windows Build Script" -ForegroundColor Cyan
Write-Host "================================" -ForegroundColor Cyan
Write-Host ""

# Check prerequisites
Write-Host "[1/7] Checking prerequisites..." -ForegroundColor Yellow

if (!(Test-Path $VcpkgPath)) {
    Write-Host "ERROR: vcpkg not found at $VcpkgPath" -ForegroundColor Red
    Write-Host "Please install vcpkg or specify path with -VcpkgPath" -ForegroundColor Red
    exit 1
}

# Check for CMake
if (!(Get-Command cmake -ErrorAction SilentlyContinue)) {
    Write-Host "ERROR: CMake not found in PATH" -ForegroundColor Red
    Write-Host "Please install CMake 3.20 or later" -ForegroundColor Red
    exit 1
}

# Check for Git
if (!(Get-Command git -ErrorAction SilentlyContinue)) {
    Write-Host "ERROR: Git not found in PATH" -ForegroundColor Red
    exit 1
}

Write-Host "✓ Prerequisites OK" -ForegroundColor Green
Write-Host ""

# Clean build directory if requested
if ($Clean -and (Test-Path "build")) {
    Write-Host "[2/7] Cleaning build directory..." -ForegroundColor Yellow
    Remove-Item -Recurse -Force build
    Write-Host "✓ Build directory cleaned" -ForegroundColor Green
} else {
    Write-Host "[2/7] Skipping clean (use -Clean to clean)" -ForegroundColor Yellow
}
Write-Host ""

# Create build directory
Write-Host "[3/7] Creating build directory..." -ForegroundColor Yellow
if (!(Test-Path "build")) {
    New-Item -ItemType Directory -Path "build" | Out-Null
}
Write-Host "✓ Build directory ready" -ForegroundColor Green
Write-Host ""

# Configure with CMake
Write-Host "[4/7] Configuring with CMake..." -ForegroundColor Yellow
Write-Host "Build type: $BuildType" -ForegroundColor Cyan

Push-Location build

$cmakeArgs = @(
    "..",
    "-DCMAKE_TOOLCHAIN_FILE=$VcpkgPath\scripts\buildsystems\vcpkg.cmake",
    "-DCMAKE_BUILD_TYPE=$BuildType",
    "-G", "Visual Studio 17 2022",
    "-A", "x64"
)

Write-Host "Running: cmake $($cmakeArgs -join ' ')" -ForegroundColor Gray
& cmake $cmakeArgs

if ($LASTEXITCODE -ne 0) {
    Write-Host "ERROR: CMake configuration failed" -ForegroundColor Red
    Pop-Location
    exit 1
}

Write-Host "✓ Configuration complete" -ForegroundColor Green
Write-Host ""

# Build
Write-Host "[5/7] Building ($BuildType configuration)..." -ForegroundColor Yellow
Write-Host "Using $Jobs parallel jobs" -ForegroundColor Cyan

& cmake --build . --config $BuildType -j $Jobs

if ($LASTEXITCODE -ne 0) {
    Write-Host "ERROR: Build failed" -ForegroundColor Red
    Pop-Location
    exit 1
}

Write-Host "✓ Build complete" -ForegroundColor Green
Write-Host ""

# Run tests
Write-Host "[6/7] Running tests..." -ForegroundColor Yellow
& ctest -C $BuildType

if ($LASTEXITCODE -ne 0) {
    Write-Host "WARNING: Some tests failed" -ForegroundColor Yellow
} else {
    Write-Host "✓ All tests passed" -ForegroundColor Green
}
Write-Host ""

# Create installer if requested
if ($WithInstaller) {
    Write-Host "[7/7] Creating installer..." -ForegroundColor Yellow

    # Check for NSIS
    $nsisPath = "C:\Program Files (x86)\NSIS\makensis.exe"
    if (Test-Path $nsisPath) {
        Write-Host "Using NSIS installer..." -ForegroundColor Cyan
        & $nsisPath "..\installer.nsi"
        if ($LASTEXITCODE -eq 0) {
            Write-Host "✓ NSIS installer created" -ForegroundColor Green
        }
    }

    # Create ZIP package
    Write-Host "Creating CPack packages..." -ForegroundColor Cyan
    & cpack -G "NSIS;ZIP" -C $BuildType

    if ($LASTEXITCODE -eq 0) {
        Write-Host "✓ CPack packages created" -ForegroundColor Green
    }
} else {
    Write-Host "[7/7] Skipping installer (use -WithInstaller to create)" -ForegroundColor Yellow
}

Pop-Location

Write-Host ""
Write-Host "================================" -ForegroundColor Cyan
Write-Host "Build Summary" -ForegroundColor Cyan
Write-Host "================================" -ForegroundColor Cyan
Write-Host "Build type: $BuildType" -ForegroundColor White
Write-Host "Build directory: build\" -ForegroundColor White
Write-Host ""
Write-Host "Executables:" -ForegroundColor Yellow
Write-Host "  - build\$BuildType\intcoind.exe (Daemon)" -ForegroundColor White
Write-Host "  - build\$BuildType\intcoin-cli.exe (CLI)" -ForegroundColor White
Write-Host "  - build\$BuildType\intcoin-qt.exe (GUI Wallet)" -ForegroundColor White
Write-Host "  - build\$BuildType\intcoin-miner.exe (CPU Miner)" -ForegroundColor White
Write-Host ""

if ($WithInstaller) {
    Write-Host "Installers:" -ForegroundColor Yellow
    Write-Host "  - INTcoin-Setup-0.1.0.exe (NSIS)" -ForegroundColor White
    Write-Host "  - INTcoin-0.1.0-win64.exe (CPack NSIS)" -ForegroundColor White
    Write-Host "  - INTcoin-0.1.0-win64.zip (Portable)" -ForegroundColor White
    Write-Host ""
}

Write-Host "To run:" -ForegroundColor Yellow
Write-Host "  .\build\$BuildType\intcoin-qt.exe" -ForegroundColor Cyan
Write-Host ""
Write-Host "✓ Build completed successfully!" -ForegroundColor Green
