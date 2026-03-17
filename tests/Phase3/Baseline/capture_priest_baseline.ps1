# =========================================================================
# PHASE 3A - PRIEST BASELINE METRICS CAPTURE
# =========================================================================
# Purpose: Capture comprehensive baseline metrics for PriestAI before refactoring
# Date: 2025-10-17
# Author: Claude Code (Phase 3A Week 1)
# =========================================================================

$ErrorActionPreference = "Stop"
$ProgressPreference = "SilentlyContinue"

# Configuration
$PROJECT_ROOT = "C:\TrinityBots\TrinityCore"
$BUILD_DIR = "$PROJECT_ROOT\build"
$OUTPUT_DIR = "$PROJECT_ROOT\Tests\Phase3\Baseline"
$TIMESTAMP = Get-Date -Format "yyyy-MM-dd_HH-mm-ss"
$BASELINE_FILE = "$OUTPUT_DIR\priest_baseline_$TIMESTAMP.json"

Write-Host "`n========================================" -ForegroundColor Cyan
Write-Host "PRIEST BASELINE METRICS CAPTURE" -ForegroundColor Cyan
Write-Host "========================================`n" -ForegroundColor Cyan

# Create output directory
New-Item -ItemType Directory -Force -Path $OUTPUT_DIR | Out-Null

# =========================================================================
# 1. CODE METRICS - Static Analysis
# =========================================================================

Write-Host "[1/5] Capturing Code Metrics..." -ForegroundColor Yellow

$priestFiles = @(
    "src\modules\Playerbot\AI\ClassAI\Priests\PriestAI.h",
    "src\modules\Playerbot\AI\ClassAI\Priests\PriestAI.cpp",
    "src\modules\Playerbot\AI\ClassAI\Priests\PriestSpecialization.h",
    "src\modules\Playerbot\AI\ClassAI\Priests\PriestSpecialization.cpp",
    "src\modules\Playerbot\AI\ClassAI\Priests\PriestAI_Specialization.cpp",
    "src\modules\Playerbot\AI\ClassAI\Priests\DisciplineSpecialization.h",
    "src\modules\Playerbot\AI\ClassAI\Priests\DisciplineSpecialization.cpp",
    "src\modules\Playerbot\AI\ClassAI\Priests\HolySpecialization.h",
    "src\modules\Playerbot\AI\ClassAI\Priests\HolySpecialization.cpp",
    "src\modules\Playerbot\AI\ClassAI\Priests\ShadowSpecialization.h",
    "src\modules\Playerbot\AI\ClassAI\Priests\ShadowSpecialization.cpp"
)

$codeMetrics = @{
    timestamp = $TIMESTAMP
    files = @()
    totals = @{
        lineCount = 0
        fileCount = $priestFiles.Count
        headerLines = 0
        sourceLines = 0
    }
}

foreach ($file in $priestFiles) {
    $fullPath = Join-Path $PROJECT_ROOT $file
    if (Test-Path $fullPath) {
        $content = Get-Content $fullPath -Raw
        $lineCount = ($content -split "`n").Count

        $fileMetrics = @{
            path = $file
            lineCount = $lineCount
            sizeBytes = (Get-Item $fullPath).Length
            type = if ($file -match "\.h$") { "header" } else { "source" }
        }

        $codeMetrics.files += $fileMetrics
        $codeMetrics.totals.lineCount += $lineCount

        if ($file -match "\.h$") {
            $codeMetrics.totals.headerLines += $lineCount
        } else {
            $codeMetrics.totals.sourceLines += $lineCount
        }
    }
}

Write-Host "  Total Lines: $($codeMetrics.totals.lineCount)" -ForegroundColor Green
Write-Host "  Total Files: $($codeMetrics.totals.fileCount)" -ForegroundColor Green

# =========================================================================
# 2. COMPILATION METRICS - Build Performance
# =========================================================================

Write-Host "`n[2/5] Capturing Compilation Metrics..." -ForegroundColor Yellow

# Clean build of PriestAI only
$compilationMetrics = @{
    timestamp = $TIMESTAMP
    buildConfig = "Release"
    platform = "x64"
    measurements = @()
}

# Measure PriestAI.cpp compilation time (3 runs for average)
$compileTimes = @()

for ($i = 1; $i -le 3; $i++) {
    Write-Host "  Compilation run $i/3..." -ForegroundColor Gray

    # Force recompilation by touching the file
    (Get-Item "$PROJECT_ROOT\src\modules\Playerbot\AI\ClassAI\Priests\PriestAI.cpp").LastWriteTime = Get-Date

    $startTime = Get-Date

    $buildOutput = & "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\MSBuild\Current\Bin\MSBuild.exe" `
        "$BUILD_DIR\src\server\modules\Playerbot\playerbot.vcxproj" `
        -p:Configuration=Release `
        -p:Platform=x64 `
        -verbosity:quiet `
        -nologo `
        2>&1

    $endTime = Get-Date
    $duration = ($endTime - $startTime).TotalSeconds
    $compileTimes += $duration

    Write-Host "    Duration: $([math]::Round($duration, 2))s" -ForegroundColor Gray
}

$compilationMetrics.measurements = @{
    compileTimeAvg = [math]::Round(($compileTimes | Measure-Object -Average).Average, 2)
    compileTimeMin = [math]::Round(($compileTimes | Measure-Object -Minimum).Minimum, 2)
    compileTimeMax = [math]::Round(($compileTimes | Measure-Object -Maximum).Maximum, 2)
    runs = 3
}

Write-Host "  Average Compile Time: $($compilationMetrics.measurements.compileTimeAvg)s" -ForegroundColor Green

# =========================================================================
# 3. BINARY SIZE METRICS
# =========================================================================

Write-Host "`n[3/5] Capturing Binary Size Metrics..." -ForegroundColor Yellow

$binaryMetrics = @{
    timestamp = $TIMESTAMP
}

# Measure playerbot.lib size
$playerbotLib = "$BUILD_DIR\src\server\modules\Playerbot\Release\playerbot.lib"
if (Test-Path $playerbotLib) {
    $libSize = (Get-Item $playerbotLib).Length
    $binaryMetrics.playerbotLibSize = $libSize
    $binaryMetrics.playerbotLibSizeMB = [math]::Round($libSize / 1MB, 2)
    Write-Host "  playerbot.lib: $($binaryMetrics.playerbotLibSizeMB) MB" -ForegroundColor Green
}

# Measure worldserver.exe size
$worldserverExe = "$BUILD_DIR\src\server\worldserver\Release\worldserver.exe"
if (Test-Path $worldserverExe) {
    $exeSize = (Get-Item $worldserverExe).Length
    $binaryMetrics.worldserverExeSize = $exeSize
    $binaryMetrics.worldserverExeSizeMB = [math]::Round($exeSize / 1MB, 2)
    Write-Host "  worldserver.exe: $($binaryMetrics.worldserverExeSizeMB) MB" -ForegroundColor Green
}

# =========================================================================
# 4. DEPENDENCY METRICS - Include Analysis
# =========================================================================

Write-Host "`n[4/5] Capturing Dependency Metrics..." -ForegroundColor Yellow

$dependencyMetrics = @{
    timestamp = $TIMESTAMP
    priestAI = @{
        directIncludes = @()
        includeCount = 0
    }
}

$priestAICpp = "$PROJECT_ROOT\src\modules\Playerbot\AI\ClassAI\Priests\PriestAI.cpp"
if (Test-Path $priestAICpp) {
    $includes = Select-String -Path $priestAICpp -Pattern '^\s*#include\s+"([^"]+)"' -AllMatches |
        ForEach-Object { $_.Matches.Groups[1].Value }

    $dependencyMetrics.priestAI.directIncludes = $includes
    $dependencyMetrics.priestAI.includeCount = $includes.Count

    Write-Host "  Direct Includes: $($includes.Count)" -ForegroundColor Green
}

# =========================================================================
# 5. COMPLEXITY METRICS - Cyclomatic Complexity Estimation
# =========================================================================

Write-Host "`n[5/5] Capturing Complexity Metrics..." -ForegroundColor Yellow

$complexityMetrics = @{
    timestamp = $TIMESTAMP
    priestAICpp = @{
        functionCount = 0
        ifStatements = 0
        switchStatements = 0
        loopStatements = 0
        estimatedComplexity = 0
    }
}

if (Test-Path $priestAICpp) {
    $content = Get-Content $priestAICpp -Raw

    # Count control flow structures (rough complexity estimation)
    $complexityMetrics.priestAICpp.functionCount = ([regex]::Matches($content, '\w+::\w+\s*\(')).Count
    $complexityMetrics.priestAICpp.ifStatements = ([regex]::Matches($content, '\bif\s*\(')).Count
    $complexityMetrics.priestAICpp.switchStatements = ([regex]::Matches($content, '\bswitch\s*\(')).Count
    $complexityMetrics.priestAICpp.loopStatements = ([regex]::Matches($content, '\b(for|while)\s*\(')).Count

    # Estimated cyclomatic complexity: functions + if + switch + loops
    $complexityMetrics.priestAICpp.estimatedComplexity =
        $complexityMetrics.priestAICpp.functionCount +
        $complexityMetrics.priestAICpp.ifStatements +
        $complexityMetrics.priestAICpp.switchStatements +
        $complexityMetrics.priestAICpp.loopStatements

    Write-Host "  Functions: $($complexityMetrics.priestAICpp.functionCount)" -ForegroundColor Green
    Write-Host "  If Statements: $($complexityMetrics.priestAICpp.ifStatements)" -ForegroundColor Green
    Write-Host "  Switch Statements: $($complexityMetrics.priestAICpp.switchStatements)" -ForegroundColor Green
    Write-Host "  Loop Statements: $($complexityMetrics.priestAICpp.loopStatements)" -ForegroundColor Green
    Write-Host "  Estimated Complexity: $($complexityMetrics.priestAICpp.estimatedComplexity)" -ForegroundColor Green
}

# =========================================================================
# 6. COMBINE AND SAVE BASELINE
# =========================================================================

Write-Host "`n[6/6] Saving Baseline Data..." -ForegroundColor Yellow

$baseline = @{
    metadata = @{
        timestamp = $TIMESTAMP
        projectRoot = $PROJECT_ROOT
        gitCommit = (git rev-parse HEAD 2>$null)
        gitBranch = (git rev-parse --abbrev-ref HEAD 2>$null)
        phase = "3A"
        week = 1
        target = "PriestAI"
        status = "BASELINE_BEFORE_REFACTORING"
    }
    codeMetrics = $codeMetrics
    compilationMetrics = $compilationMetrics
    binaryMetrics = $binaryMetrics
    dependencyMetrics = $dependencyMetrics
    complexityMetrics = $complexityMetrics
}

# Save as JSON
$baseline | ConvertTo-Json -Depth 10 | Out-File -FilePath $BASELINE_FILE -Encoding UTF8

Write-Host "  Baseline saved to: $BASELINE_FILE" -ForegroundColor Green

# =========================================================================
# 7. GENERATE HUMAN-READABLE REPORT
# =========================================================================

$reportFile = "$OUTPUT_DIR\priest_baseline_$TIMESTAMP.md"

$report = @"
# PRIEST BASELINE METRICS REPORT
**Date**: $TIMESTAMP
**Phase**: 3A Week 1
**Target**: PriestAI God Class Refactoring
**Status**: BASELINE_BEFORE_REFACTORING

---

## Executive Summary

This baseline captures the current state of the PriestAI implementation before Phase 3A refactoring begins.

### Code Metrics
- **Total Lines**: $($codeMetrics.totals.lineCount)
- **Total Files**: $($codeMetrics.totals.fileCount)
- **Header Lines**: $($codeMetrics.totals.headerLines)
- **Source Lines**: $($codeMetrics.totals.sourceLines)

### God Class Metrics (PriestAI.cpp)
- **Line Count**: 3,154 lines
- **Functions**: $($complexityMetrics.priestAICpp.functionCount)
- **If Statements**: $($complexityMetrics.priestAICpp.ifStatements)
- **Switch Statements**: $($complexityMetrics.priestAICpp.switchStatements)
- **Loop Statements**: $($complexityMetrics.priestAICpp.loopStatements)
- **Estimated Complexity**: $($complexityMetrics.priestAICpp.estimatedComplexity)

### Compilation Metrics
- **Average Compile Time**: $($compilationMetrics.measurements.compileTimeAvg)s
- **Min Compile Time**: $($compilationMetrics.measurements.compileTimeMin)s
- **Max Compile Time**: $($compilationMetrics.measurements.compileTimeMax)s

### Binary Size
- **playerbot.lib**: $($binaryMetrics.playerbotLibSizeMB) MB
- **worldserver.exe**: $($binaryMetrics.worldserverExeSizeMB) MB

### Dependencies
- **Direct Includes**: $($dependencyMetrics.priestAI.includeCount)

---

## File Breakdown

| File | Lines | Size (KB) | Type |
|------|-------|-----------|------|
"@

foreach ($file in $codeMetrics.files) {
    $sizeKB = [math]::Round($file.sizeBytes / 1KB, 1)
    $report += "| $($file.path) | $($file.lineCount) | $sizeKB | $($file.type) |`n"
}

$report += @"

---

## Refactoring Targets

### Phase 3A Goals
1. **PriestAI.cpp**: Reduce from 3,154 lines to ~500 lines (-84%)
2. **Pattern**: Migrate to header-based template specialization
3. **Performance**: Reduce compile time by >30%
4. **Maintainability**: Reduce complexity by >50%

### Expected Outcomes
- Faster compilation (incremental builds)
- Better cache efficiency (smaller translation units)
- Improved testability (isolated specializations)
- Zero runtime overhead (compile-time polymorphism)

---

**Git Commit**: $($baseline.metadata.gitCommit)
**Git Branch**: $($baseline.metadata.gitBranch)
**Baseline File**: $BASELINE_FILE

---

*Generated by Phase 3A Week 1 Baseline Capture Script*
"@

$report | Out-File -FilePath $reportFile -Encoding UTF8

Write-Host "`n========================================" -ForegroundColor Cyan
Write-Host "BASELINE CAPTURE COMPLETE" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "`nBaseline Data: $BASELINE_FILE" -ForegroundColor Green
Write-Host "Report: $reportFile" -ForegroundColor Green
Write-Host "`nNext Steps:" -ForegroundColor Yellow
Write-Host "  1. Review baseline metrics" -ForegroundColor White
Write-Host "  2. Create unit tests for Holy/Shadow specializations" -ForegroundColor White
Write-Host "  3. Begin PriestAI.cpp coordinator transformation" -ForegroundColor White
Write-Host ""
