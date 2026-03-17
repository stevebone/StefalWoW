@echo off
REM ============================================================================
REM Week 4 Performance Test Runner for TrinityCore Playerbot
REM ============================================================================
REM
REM This script runs the Week 4 performance test suite to validate
REM packet-based spell casting at scale (100, 500, 1000, 5000 bots).
REM
REM Usage:
REM   RunWeek4Tests.bat [scenario]
REM
REM Scenarios:
REM   0 - Baseline (100 bots, 30 minutes)
REM   1 - Combat Load (500 bots, 60 minutes)
REM   2 - Stress Test (1000 bots, 120 minutes)
REM   3 - Scaling Test (5000 bots, 240 minutes)
REM   4 - Stability Test (100 bots, 24 hours)
REM   all - Run all scenarios (default)
REM
REM Requirements:
REM   - TrinityCore worldserver must be running
REM   - Database must be configured
REM   - Sufficient system resources for bot count
REM
REM Output:
REM   - WEEK_4_PERFORMANCE_TEST_REPORT.md (comprehensive report)
REM   - week4_performance_metrics.csv (CSV data for graphing)
REM   - worldserver logs with test execution details
REM
REM ============================================================================

setlocal enabledelayedexpansion

REM Configuration
set "BUILD_DIR=C:\TrinityBots\TrinityCore\build"
set "SERVER_DIR=M:\Wplayerbot"
set "WORLDSERVER=%SERVER_DIR%\bin\RelWithDebInfo\worldserver.exe"
set "OUTPUT_DIR=%CD%"

REM Colors for output
set "COLOR_RESET=[0m"
set "COLOR_GREEN=[92m"
set "COLOR_YELLOW=[93m"
set "COLOR_RED=[91m"
set "COLOR_BLUE=[94m"

echo.
echo ============================================================================
echo Week 4 Performance Test Runner
echo ============================================================================
echo.

REM Parse arguments
set "SCENARIO=%~1"
if "%SCENARIO%"=="" set "SCENARIO=all"

echo [INFO] Scenario: %SCENARIO%
echo [INFO] Output Directory: %OUTPUT_DIR%
echo.

REM Check if worldserver is running
tasklist /FI "IMAGENAME eq worldserver.exe" 2>NUL | find /I /N "worldserver.exe">NUL
if "%ERRORLEVEL%"=="1" (
    echo %COLOR_RED%[ERROR] worldserver.exe is not running!%COLOR_RESET%
    echo.
    echo Please start worldserver before running tests:
    echo   cd %SERVER_DIR%\bin\RelWithDebInfo
    echo   worldserver.exe
    echo.
    exit /b 1
)

echo %COLOR_GREEN%[OK] worldserver.exe is running%COLOR_RESET%
echo.

REM Check for GM account
echo [INFO] Checking for GM account...
echo.
echo IMPORTANT: You must have a GM account to spawn bots.
echo.
echo If you don't have a GM account, create one:
echo   1. Connect with worldserver console
echo   2. Run: account create gmtest gmtest
echo   3. Run: account set gmlevel gmtest 3 -1
echo.

pause

REM Display test plan
echo.
echo ============================================================================
echo Test Plan
echo ============================================================================
echo.

if "%SCENARIO%"=="0" (
    echo Scenario 0: Baseline Performance
    echo   - Bot Count: 100
    echo   - Duration: 30 minutes
    echo   - Focus: Baseline metrics for packet-based spell casting
    echo.
) else if "%SCENARIO%"=="1" (
    echo Scenario 1: Combat Load
    echo   - Bot Count: 500
    echo   - Duration: 60 minutes
    echo   - Focus: Sustained combat stress testing
    echo.
) else if "%SCENARIO%"=="2" (
    echo Scenario 2: Stress Test
    echo   - Bot Count: 1000
    echo   - Duration: 120 minutes
    echo   - Focus: Bottleneck identification at high bot count
    echo.
) else if "%SCENARIO%"=="3" (
    echo Scenario 3: Scaling Test
    echo   - Bot Count: 5000
    echo   - Duration: 240 minutes (4 hours)
    echo   - Focus: Phase 0 goal validation
    echo.
) else if "%SCENARIO%"=="4" (
    echo Scenario 4: Stability Test
    echo   - Bot Count: 100
    echo   - Duration: 1440 minutes (24 hours)
    echo   - Focus: Long-running stability validation
    echo.
) else (
    echo ALL Scenarios:
    echo   0. Baseline (100 bots, 30 min)
    echo   1. Combat Load (500 bots, 60 min)
    echo   2. Stress Test (1000 bots, 120 min)
    echo   3. Scaling Test (5000 bots, 4 hours)
    echo   4. Stability Test (100 bots, 24 hours)
    echo.
    echo Total estimated time: 30+ hours
    echo.
)

echo Performance Targets:
echo   - CPU per bot: ^<0.1%%
echo   - Memory per bot: ^<10MB
echo   - Spell cast latency: ^<10ms
echo   - Success rate: ^>99%%
echo   - Main thread blocking: ^<5ms
echo   - Server TPS: ^>20
echo.

echo ============================================================================
echo.

set /p CONFIRM="Start test execution? (Y/N): "
if /i not "%CONFIRM%"=="Y" (
    echo [INFO] Test execution cancelled by user
    exit /b 0
)

echo.
echo [INFO] Starting Week 4 performance tests...
echo [INFO] Monitor worldserver console for real-time progress
echo [INFO] This may take several hours depending on scenario
echo.

REM Execute test via worldserver console command
REM NOTE: This requires a custom console command to be implemented in worldserver
REM For now, this is a placeholder showing the intended workflow

echo [INFO] Sending test command to worldserver...
echo.

REM TODO: Implement actual test execution via GM command or console
REM Example: .playerbot test week4 %SCENARIO%

echo %COLOR_YELLOW%[NOTICE] Automated test execution not yet implemented%COLOR_RESET%
echo.
echo Manual Test Execution Steps:
echo.
echo 1. Connect to worldserver as GM
echo 2. Run command: .playerbot test week4 %SCENARIO%
echo 3. Wait for test completion
echo 4. Check output files:
echo      - %OUTPUT_DIR%\WEEK_4_PERFORMANCE_TEST_REPORT.md
echo      - %OUTPUT_DIR%\week4_performance_metrics.csv
echo.
echo OR compile and run Week4PerformanceTest directly via C++ test harness
echo.

pause

echo.
echo ============================================================================
echo Test Execution Complete
echo ============================================================================
echo.

REM Check for output files
if exist "%OUTPUT_DIR%\WEEK_4_PERFORMANCE_TEST_REPORT.md" (
    echo %COLOR_GREEN%[OK] Report generated: WEEK_4_PERFORMANCE_TEST_REPORT.md%COLOR_RESET%
    echo.
    type "%OUTPUT_DIR%\WEEK_4_PERFORMANCE_TEST_REPORT.md"
    echo.
) else (
    echo %COLOR_YELLOW%[WARN] Report not found - test may not have completed%COLOR_RESET%
)

if exist "%OUTPUT_DIR%\week4_performance_metrics.csv" (
    echo %COLOR_GREEN%[OK] Metrics CSV generated: week4_performance_metrics.csv%COLOR_RESET%
) else (
    echo %COLOR_YELLOW%[WARN] Metrics CSV not found%COLOR_RESET%
)

echo.
echo Next Steps:
echo   1. Review WEEK_4_PERFORMANCE_TEST_REPORT.md for detailed results
echo   2. Analyze week4_performance_metrics.csv for performance trends
echo   3. If all tests PASS: Proceed with Priority 1 tasks
echo   4. If any tests FAIL: Implement recommended optimizations
echo.

pause
endlocal
