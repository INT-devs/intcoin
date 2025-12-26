# INTcoin Windows Build Script
# Automates the complete Windows build process for creating .exe executables
# Requirements: Visual Studio 2022, Git, CMake, vcpkg

param(
    [string]$BuildType = "Release",
    [string]$VcpkgRoot = "$env:USERPROFILE\vcpkg",
    [switch]$SkipDependencies,
    [switch]$CleanBuild
)

$ErrorActionPreference = "Stop"

# Colors for output
function Write-Success { Write-Host $args -ForegroundColor Green }
function Write-Info { Write-Host $args -ForegroundColor Cyan }
function Write-Warning { Write-Host $args -ForegroundColor Yellow }
function Write-Error { Write-Host $args -ForegroundColor Red }

Write-Info "========================================="
Write-Info "INTcoin Windows Build Script"
Write-Info "========================================="
Write-Info ""

# Check Windows version (enforce Windows 10 22H2+ or Windows 11+)
$osVersion = [System.Environment]::OSVersion.Version
$osBuild = (Get-ItemProperty "HKLM:\SOFTWARE\Microsoft\Windows NT\CurrentVersion").CurrentBuild

Write-Info "Detected Windows version: $($osVersion.Major).$($osVersion.Minor) (Build $osBuild)"

if ($osVersion.Major -lt 10) {
    Write-Error "ERROR: Windows $($osVersion.Major) is not supported (end-of-life)"
    Write-Error "Minimum required: Windows 10 22H2 (Build 19045) or Windows 11"
    exit 1
}

if ($osVersion.Major -eq 10 -and $osBuild -lt 19045) {
    Write-Error "ERROR: Windows 10 Build $osBuild is not supported (end-of-life)"
    Write-Error "Minimum required: Windows 10 22H2 (Build 19045) or Windows 11"
    Write-Error "Your Windows 10 version is outdated. Please update to 22H2 or upgrade to Windows 11."
    exit 1
}

Write-Success "✓ Windows version check passed"
Write-Info ""

# Get script directory and project root
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$ProjectRoot = $ScriptDir
$BuildDir = Join-Path $ProjectRoot "build-windows"
$DistDir = Join-Path $ProjectRoot "dist-windows"

Write-Info "Project Root: $ProjectRoot"
Write-Info "Build Directory: $BuildDir"
Write-Info "Distribution Directory: $DistDir"
Write-Info ""

# Check prerequisites
Write-Info "Checking prerequisites..."

# Check Visual Studio
$vsPath = & "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe" `
    -latest -property installationPath -version "[17.0,18.0)" 2>$null

if (-not $vsPath) {
    Write-Error "ERROR: Visual Studio 2022 not found!"
    Write-Error "Please install Visual Studio 2022 with C++ workload"
    exit 1
}
Write-Success "✓ Visual Studio 2022: $vsPath"

# Check CMake
$cmake = Get-Command cmake -ErrorAction SilentlyContinue
if (-not $cmake) {
    Write-Error "ERROR: CMake not found!"
    Write-Error "Please install CMake and add it to PATH"
    exit 1
}
Write-Success "✓ CMake: $($cmake.Version)"

# Check Git
$git = Get-Command git -ErrorAction SilentlyContinue
if (-not $git) {
    Write-Error "ERROR: Git not found!"
    Write-Error "Please install Git and add it to PATH"
    exit 1
}
Write-Success "✓ Git: Found"

Write-Info ""

# Setup vcpkg
if (-not $SkipDependencies) {
    Write-Info "Setting up vcpkg..."

    if (-not (Test-Path $VcpkgRoot)) {
        Write-Info "Cloning vcpkg to $VcpkgRoot..."
        git clone https://github.com/Microsoft/vcpkg.git $VcpkgRoot
        if ($LASTEXITCODE -ne 0) {
            Write-Error "ERROR: Failed to clone vcpkg"
            exit 1
        }
    } else {
        Write-Info "vcpkg already exists, updating..."
        Push-Location $VcpkgRoot
        git pull
        Pop-Location
    }

    # Bootstrap vcpkg
    $vcpkgExe = Join-Path $VcpkgRoot "vcpkg.exe"
    if (-not (Test-Path $vcpkgExe)) {
        Write-Info "Bootstrapping vcpkg..."
        Push-Location $VcpkgRoot
        & .\bootstrap-vcpkg.bat
        if ($LASTEXITCODE -ne 0) {
            Write-Error "ERROR: Failed to bootstrap vcpkg"
            exit 1
        }
        Pop-Location
    }

    Write-Success "✓ vcpkg ready"

    # Install dependencies
    Write-Info "Installing dependencies via vcpkg..."
    Write-Info "This may take 30-60 minutes on first run..."

    $packages = @(
        "rocksdb:x64-windows",
        "openssl:x64-windows",
        "zlib:x64-windows",
        "curl:x64-windows",
        "nlohmann-json:x64-windows",
        "sqlite3:x64-windows"
    )

    foreach ($package in $packages) {
        Write-Info "Installing $package..."
        & $vcpkgExe install $package
        if ($LASTEXITCODE -ne 0) {
            Write-Error "ERROR: Failed to install $package"
            exit 1
        }
    }

    Write-Success "✓ All vcpkg dependencies installed"
    Write-Info ""
}

# Build liboqs
if (-not $SkipDependencies) {
    Write-Info "Building liboqs 0.15.0..."

    $liboqsDir = Join-Path $ProjectRoot "build-windows-deps\liboqs"
    $liboqsBuildDir = Join-Path $liboqsDir "build"
    $liboqsInstallDir = Join-Path $ProjectRoot "build-windows-deps\liboqs-install"

    if (-not (Test-Path $liboqsDir)) {
        Write-Info "Cloning liboqs..."
        New-Item -ItemType Directory -Force -Path (Join-Path $ProjectRoot "build-windows-deps") | Out-Null
        git clone --branch 0.15.0 https://github.com/open-quantum-safe/liboqs.git $liboqsDir
        if ($LASTEXITCODE -ne 0) {
            Write-Error "ERROR: Failed to clone liboqs"
            exit 1
        }
    }

    if (-not (Test-Path $liboqsInstallDir) -or $CleanBuild) {
        if (Test-Path $liboqsBuildDir) {
            Remove-Item -Recurse -Force $liboqsBuildDir
        }
        New-Item -ItemType Directory -Force -Path $liboqsBuildDir | Out-Null

        Push-Location $liboqsBuildDir

        cmake -G "Visual Studio 17 2022" -A x64 `
            -DCMAKE_BUILD_TYPE=$BuildType `
            -DCMAKE_INSTALL_PREFIX="$liboqsInstallDir" `
            -DBUILD_SHARED_LIBS=OFF `
            -DOQS_BUILD_ONLY_LIB=ON `
            ..

        if ($LASTEXITCODE -ne 0) {
            Pop-Location
            Write-Error "ERROR: liboqs CMake configuration failed"
            exit 1
        }

        cmake --build . --config $BuildType
        if ($LASTEXITCODE -ne 0) {
            Pop-Location
            Write-Error "ERROR: liboqs build failed"
            exit 1
        }

        cmake --install . --config $BuildType
        if ($LASTEXITCODE -ne 0) {
            Pop-Location
            Write-Error "ERROR: liboqs install failed"
            exit 1
        }

        Pop-Location
    }

    Write-Success "✓ liboqs built and installed"
    Write-Info ""
}

# Build RandomX
if (-not $SkipDependencies) {
    Write-Info "Building RandomX 1.2.1..."

    $randomxDir = Join-Path $ProjectRoot "build-windows-deps\randomx"
    $randomxBuildDir = Join-Path $randomxDir "build"
    $randomxInstallDir = Join-Path $ProjectRoot "build-windows-deps\randomx-install"

    if (-not (Test-Path $randomxDir)) {
        Write-Info "Cloning RandomX..."
        git clone --branch v1.2.1 https://github.com/tevador/RandomX.git $randomxDir
        if ($LASTEXITCODE -ne 0) {
            Write-Error "ERROR: Failed to clone RandomX"
            exit 1
        }
    }

    if (-not (Test-Path $randomxInstallDir) -or $CleanBuild) {
        if (Test-Path $randomxBuildDir) {
            Remove-Item -Recurse -Force $randomxBuildDir
        }
        New-Item -ItemType Directory -Force -Path $randomxBuildDir | Out-Null

        Push-Location $randomxBuildDir

        cmake -G "Visual Studio 17 2022" -A x64 `
            -DCMAKE_BUILD_TYPE=$BuildType `
            -DCMAKE_INSTALL_PREFIX="$randomxInstallDir" `
            -DBUILD_SHARED_LIBS=OFF `
            ..

        if ($LASTEXITCODE -ne 0) {
            Pop-Location
            Write-Error "ERROR: RandomX CMake configuration failed"
            exit 1
        }

        cmake --build . --config $BuildType
        if ($LASTEXITCODE -ne 0) {
            Pop-Location
            Write-Error "ERROR: RandomX build failed"
            exit 1
        }

        cmake --install . --config $BuildType
        if ($LASTEXITCODE -ne 0) {
            Pop-Location
            Write-Error "ERROR: RandomX install failed"
            exit 1
        }

        Pop-Location
    }

    Write-Success "✓ RandomX built and installed"
    Write-Info ""
}

# Build INTcoin
Write-Info "Building INTcoin..."

if ($CleanBuild -and (Test-Path $BuildDir)) {
    Write-Info "Cleaning previous build..."
    Remove-Item -Recurse -Force $BuildDir
}

New-Item -ItemType Directory -Force -Path $BuildDir | Out-Null

Push-Location $BuildDir

$cmakeToolchain = Join-Path $VcpkgRoot "scripts\buildsystems\vcpkg.cmake"
$liboqsInstallDir = Join-Path $ProjectRoot "build-windows-deps\liboqs-install"
$randomxInstallDir = Join-Path $ProjectRoot "build-windows-deps\randomx-install"

Write-Info "Configuring INTcoin build..."

cmake -G "Visual Studio 17 2022" -A x64 `
    -DCMAKE_BUILD_TYPE=$BuildType `
    -DCMAKE_TOOLCHAIN_FILE="$cmakeToolchain" `
    -DLIBOQS_ROOT="$liboqsInstallDir" `
    -DRANDOMX_ROOT="$randomxInstallDir" `
    -DBUILD_TESTS=ON `
    -DBUILD_QT=ON `
    ..

if ($LASTEXITCODE -ne 0) {
    Pop-Location
    Write-Error "ERROR: INTcoin CMake configuration failed"
    exit 1
}

Write-Info "Building INTcoin executables..."

cmake --build . --config $BuildType --parallel

if ($LASTEXITCODE -ne 0) {
    Pop-Location
    Write-Error "ERROR: INTcoin build failed"
    exit 1
}

Pop-Location

Write-Success "✓ INTcoin built successfully"
Write-Info ""

# Create distribution package
Write-Info "Creating distribution package..."

if (Test-Path $DistDir) {
    Remove-Item -Recurse -Force $DistDir
}
New-Item -ItemType Directory -Force -Path $DistDir | Out-Null

# Copy executables
$exeFiles = @(
    "intcoind.exe",
    "intcoin-cli.exe",
    "intcoin-wallet.exe",
    "intcoin-miner.exe",
    "intcoin-pool-server.exe",
    "intcoin-qt.exe"
)

$buildBinDir = Join-Path $BuildDir "$BuildType"

foreach ($exe in $exeFiles) {
    $sourcePath = Join-Path $buildBinDir $exe
    if (Test-Path $sourcePath) {
        Write-Info "Copying $exe..."
        Copy-Item $sourcePath $DistDir
    } else {
        Write-Warning "Warning: $exe not found, skipping..."
    }
}

# Copy required DLLs
Write-Info "Copying required DLLs..."

$vcpkgBinDir = Join-Path $VcpkgRoot "installed\x64-windows\bin"
$requiredDlls = @(
    "rocksdb.dll",
    "zlib1.dll",
    "libcrypto-3-x64.dll",
    "libssl-3-x64.dll"
)

foreach ($dll in $requiredDlls) {
    $sourcePath = Join-Path $vcpkgBinDir $dll
    if (Test-Path $sourcePath) {
        Copy-Item $sourcePath $DistDir
    } else {
        Write-Warning "Warning: $dll not found, application may not run"
    }
}

# Copy Visual C++ runtime (if needed)
$vcRedistDir = Join-Path $vsPath "VC\Redist\MSVC"
if (Test-Path $vcRedistDir) {
    $latestRedist = Get-ChildItem $vcRedistDir | Sort-Object Name -Descending | Select-Object -First 1
    $vcRuntimeDll = Join-Path $latestRedist.FullName "x64\Microsoft.VC143.CRT\msvcp140.dll"
    if (Test-Path $vcRuntimeDll) {
        Copy-Item $vcRuntimeDll $DistDir
        $vcRuntimeDll2 = Join-Path $latestRedist.FullName "x64\Microsoft.VC143.CRT\vcruntime140.dll"
        Copy-Item $vcRuntimeDll2 $DistDir
    }
}

# Create README for distribution
$readmeContent = @"
INTcoin v1.0.0-alpha - Windows Distribution
============================================

This package contains the Windows executables for INTcoin.

Executables:
- intcoind.exe          : Full node daemon
- intcoin-cli.exe       : Command-line interface
- intcoin-wallet.exe    : Wallet management tool
- intcoin-miner.exe     : Standalone miner
- intcoin-pool-server.exe : Mining pool server
- intcoin-qt.exe        : Qt graphical wallet (if built)

Quick Start:
1. Extract all files to a directory
2. Run intcoind.exe to start the node
3. Use intcoin-cli.exe to interact with the node
4. See docs/ for full documentation

Requirements:
- Windows 10/11 (64-bit)
- 4GB RAM minimum, 8GB recommended
- 50GB disk space for blockchain

For more information, visit: https://github.com/intcoin/crypto

Build Date: $(Get-Date -Format "yyyy-MM-dd HH:mm:ss")
Build Type: $BuildType
"@

Set-Content -Path (Join-Path $DistDir "README.txt") -Value $readmeContent

Write-Success "✓ Distribution package created"
Write-Info ""

# Summary
Write-Info "========================================="
Write-Success "Build Complete!"
Write-Info "========================================="
Write-Info ""
Write-Info "Executables location: $DistDir"
Write-Info ""
Write-Info "Created files:"
Get-ChildItem $DistDir | ForEach-Object {
    $size = if ($_.Length -lt 1MB) {
        "{0:N0} KB" -f ($_.Length / 1KB)
    } else {
        "{0:N2} MB" -f ($_.Length / 1MB)
    }
    Write-Info "  - $($_.Name) ($size)"
}
Write-Info ""
Write-Info "To create a distributable archive, run:"
Write-Info "  Compress-Archive -Path '$DistDir\*' -DestinationPath 'intcoin-windows-$BuildType.zip'"
Write-Info ""
Write-Success "Done!"
