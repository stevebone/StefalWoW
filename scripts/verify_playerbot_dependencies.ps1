# TrinityCore Playerbot Dependency Verification Script (Windows)
# Verifies all required enterprise-grade dependencies via vcpkg

param(
    [string]$VcpkgRoot = $env:VCPKG_ROOT,
    [string]$Triplet = "x64-windows"
)

Write-Host "=== TrinityCore Playerbot Dependency Verification ===" -ForegroundColor Cyan

$Failed = $false

function Test-VcpkgPackage {
    param(
        [string]$PackageName,
        [string]$MinVersion = $null
    )

    if (-not $VcpkgRoot) {
        Write-Host "❌ VCPKG_ROOT environment variable not set" -ForegroundColor Red
        Write-Host "   Set it to your vcpkg installation directory" -ForegroundColor Yellow
        return $false
    }

    $VcpkgPath = Join-Path $VcpkgRoot "vcpkg.exe"
    if (-not (Test-Path $VcpkgPath)) {
        Write-Host "❌ vcpkg.exe not found at $VcpkgPath" -ForegroundColor Red
        return $false
    }

    # Check if package is installed
    $ListOutput = & $VcpkgPath list $PackageName --triplet=$Triplet 2>$null
    if ($LASTEXITCODE -eq 0 -and $ListOutput) {
        # Parse version if needed
        if ($MinVersion) {
            $InstalledVersion = ($ListOutput | Select-String "$PackageName" | Select-Object -First 1).ToString().Split()[1]
            Write-Host "Found version: $InstalledVersion" -ForegroundColor Gray
        }
        return $true
    }

    return $false
}

function Test-VisualStudio {
    # Check for Visual Studio 2022
    $VSPaths = @(
        "${env:ProgramFiles}\Microsoft Visual Studio\2022\Community\Common7\IDE\devenv.exe",
        "${env:ProgramFiles}\Microsoft Visual Studio\2022\Professional\Common7\IDE\devenv.exe",
        "${env:ProgramFiles}\Microsoft Visual Studio\2022\Enterprise\Common7\IDE\devenv.exe",
        "${env:ProgramFiles(x86)}\Microsoft Visual Studio\2022\Community\Common7\IDE\devenv.exe",
        "${env:ProgramFiles(x86)}\Microsoft Visual Studio\2022\Professional\Common7\IDE\devenv.exe",
        "${env:ProgramFiles(x86)}\Microsoft Visual Studio\2022\Enterprise\Common7\IDE\devenv.exe"
    )

    foreach ($Path in $VSPaths) {
        if (Test-Path $Path) {
            return $true
        }
    }

    # Check for Visual Studio Build Tools
    $BuildToolsPaths = @(
        "${env:ProgramFiles}\Microsoft Visual Studio\2022\BuildTools\Common7\Tools\VsDevCmd.bat",
        "${env:ProgramFiles(x86)}\Microsoft Visual Studio\2022\BuildTools\Common7\Tools\VsDevCmd.bat"
    )

    foreach ($Path in $BuildToolsPaths) {
        if (Test-Path $Path) {
            return $true
        }
    }

    return $false
}

function Get-SystemInfo {
    Write-Host "=== System Information ===" -ForegroundColor Blue
    Write-Host "OS: $((Get-CimInstance Win32_OperatingSystem).Caption)"
    Write-Host "Architecture: $env:PROCESSOR_ARCHITECTURE"
    Write-Host "PowerShell: $($PSVersionTable.PSVersion)"
    if ($VcpkgRoot) {
        Write-Host "vcpkg Root: $VcpkgRoot"
    }
    Write-Host ""
}

# Main verification
Get-SystemInfo

Write-Host "=== Dependency Verification ===" -ForegroundColor Blue

# Verify Visual Studio
Write-Host "Checking Visual Studio 2022... " -NoNewline
if (Test-VisualStudio) {
    Write-Host "✅ Found" -ForegroundColor Green
} else {
    Write-Host "❌ Not found" -ForegroundColor Red
    Write-Host "  Install: Visual Studio 2022 with C++ workload" -ForegroundColor Yellow
    Write-Host "  Download: https://visualstudio.microsoft.com/downloads/" -ForegroundColor Yellow
    $Failed = $true
}

# Verify CMake
Write-Host "Checking CMake... " -NoNewline
if (Get-Command cmake -ErrorAction SilentlyContinue) {
    $CMakeVersion = (cmake --version | Select-String "cmake version" | ForEach-Object { $_.ToString().Split()[2] })
    $VersionParts = $CMakeVersion.Split('.')
    $Major = [int]$VersionParts[0]
    $Minor = [int]$VersionParts[1]

    if ($Major -gt 3 -or ($Major -eq 3 -and $Minor -ge 24)) {
        Write-Host "✅ Found CMake $CMakeVersion" -ForegroundColor Green
    } else {
        Write-Host "⚠️ Found CMake $CMakeVersion (requires 3.24+)" -ForegroundColor Yellow
        $Failed = $true
    }
} else {
    Write-Host "❌ Not found" -ForegroundColor Red
    Write-Host "  Install: choco install cmake (via Chocolatey)" -ForegroundColor Yellow
    Write-Host "  Or:     Download from https://cmake.org/download/" -ForegroundColor Yellow
    $Failed = $true
}

# Verify vcpkg
Write-Host "Checking vcpkg... " -NoNewline
if ($VcpkgRoot -and (Test-Path (Join-Path $VcpkgRoot "vcpkg.exe"))) {
    Write-Host "✅ Found" -ForegroundColor Green
} else {
    Write-Host "❌ Not found or VCPKG_ROOT not set" -ForegroundColor Red
    Write-Host "  Install: git clone https://github.com/Microsoft/vcpkg.git" -ForegroundColor Yellow
    Write-Host "           cd vcpkg && .\bootstrap-vcpkg.bat" -ForegroundColor Yellow
    Write-Host "           .\vcpkg integrate install" -ForegroundColor Yellow
    Write-Host "           Set VCPKG_ROOT environment variable" -ForegroundColor Yellow
    $Failed = $true
}

if (-not $Failed) {
    # Verify Intel TBB
    Write-Host "Checking Intel TBB... " -NoNewline
    if (Test-VcpkgPackage "tbb") {
        Write-Host "✅ Found" -ForegroundColor Green
    } else {
        Write-Host "❌ Not found" -ForegroundColor Red
        Write-Host "  Install: vcpkg install tbb:$Triplet" -ForegroundColor Yellow
        $Failed = $true
    }

    # Verify Parallel Hashmap
    Write-Host "Checking Parallel Hashmap... " -NoNewline
    if (Test-VcpkgPackage "parallel-hashmap") {
        Write-Host "✅ Found" -ForegroundColor Green
    } else {
        Write-Host "❌ Not found" -ForegroundColor Red
        Write-Host "  Install: vcpkg install parallel-hashmap:$Triplet" -ForegroundColor Yellow
        $Failed = $true
    }

    # Verify Boost
    Write-Host "Checking Boost... " -NoNewline
    if (Test-VcpkgPackage "boost") {
        Write-Host "✅ Found" -ForegroundColor Green
    } else {
        Write-Host "❌ Not found" -ForegroundColor Red
        Write-Host "  Install: vcpkg install boost:$Triplet" -ForegroundColor Yellow
        $Failed = $true
    }

    # Verify MySQL
    Write-Host "Checking MySQL... " -NoNewline
    if (Test-VcpkgPackage "mysql") {
        Write-Host "✅ Found" -ForegroundColor Green
    } else {
        Write-Host "❌ Not found" -ForegroundColor Red
        Write-Host "  Install: vcpkg install mysql:$Triplet" -ForegroundColor Yellow
        $Failed = $true
    }

    # Verify OpenSSL
    Write-Host "Checking OpenSSL... " -NoNewline
    if (Test-VcpkgPackage "openssl") {
        Write-Host "✅ Found" -ForegroundColor Green
    } else {
        Write-Host "❌ Not found" -ForegroundColor Red
        Write-Host "  Install: vcpkg install openssl:$Triplet" -ForegroundColor Yellow
        $Failed = $true
    }
}

Write-Host ""

if (-not $Failed) {
    Write-Host "🎉 All dependencies verified successfully!" -ForegroundColor Green
    Write-Host "You can now build TrinityCore with BUILD_PLAYERBOT=ON" -ForegroundColor Green
    Write-Host ""
    Write-Host "Build commands:" -ForegroundColor Blue
    Write-Host "  mkdir build && cd build"
    Write-Host "  cmake .. -G `"Visual Studio 17 2022`" -DBUILD_PLAYERBOT=ON"
    Write-Host "  cmake --build . --config Release"
    exit 0
} else {
    Write-Host "❌ Some dependencies are missing." -ForegroundColor Red
    Write-Host "Please install the missing dependencies before building Playerbot." -ForegroundColor Red
    Write-Host ""
    Write-Host "Quick install (if vcpkg is set up):" -ForegroundColor Blue
    Write-Host "  vcpkg install tbb parallel-hashmap boost mysql openssl --triplet=$Triplet"
    Write-Host ""
    Write-Host "For complete installation instructions, see:" -ForegroundColor Blue
    Write-Host "  .claude\PHASE1\DEPENDENCY_VERIFICATION.md"
    exit 1
}