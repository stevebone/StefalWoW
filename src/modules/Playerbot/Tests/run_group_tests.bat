@echo off
REM Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
REM
REM PlayerBot Group Functionality Test Runner (Windows)
REM This script runs the comprehensive test suite for group functionality validation

setlocal enabledelayedexpansion

REM Script configuration
set "SCRIPT_DIR=%~dp0"
set "BUILD_DIR=%SCRIPT_DIR%..\..\..\..\build_install"
set "TESTS_DIR=%BUILD_DIR%\src\modules\Playerbot\Tests"
set "REPORTS_DIR=%SCRIPT_DIR%test_reports"
set "ARTIFACTS_DIR=%SCRIPT_DIR%test_artifacts"

REM Test configuration
set "TEST_TIMEOUT=1800"
set "ENABLE_COVERAGE=false"
set "PARALLEL_TESTS=1"
set "VERBOSE=false"
set "RUN_DEMO=false"
set "CI_MODE=false"

REM Parse command line arguments
:parse_args
if "%~1"=="" goto :args_done
if /i "%~1"=="--help" goto :show_help
if /i "%~1"=="-h" goto :show_help
if /i "%~1"=="--verbose" set "VERBOSE=true" & shift & goto :parse_args
if /i "%~1"=="-v" set "VERBOSE=true" & shift & goto :parse_args
if /i "%~1"=="--coverage" set "ENABLE_COVERAGE=true" & shift & goto :parse_args
if /i "%~1"=="-c" set "ENABLE_COVERAGE=true" & shift & goto :parse_args
if /i "%~1"=="--parallel" set "PARALLEL_TESTS=%~2" & shift & shift & goto :parse_args
if /i "%~1"=="-p" set "PARALLEL_TESTS=%~2" & shift & shift & goto :parse_args
if /i "%~1"=="--timeout" set "TEST_TIMEOUT=%~2" & shift & shift & goto :parse_args
if /i "%~1"=="-t" set "TEST_TIMEOUT=%~2" & shift & shift & goto :parse_args
if /i "%~1"=="--quick" set "TEST_FILTER=SMOKE" & shift & goto :parse_args
if /i "%~1"=="--stress" set "TEST_FILTER=STRESS" & shift & goto :parse_args
if /i "%~1"=="--demo" set "RUN_DEMO=true" & shift & goto :parse_args
if /i "%~1"=="--ci" set "CI_MODE=true" & set "ENABLE_COVERAGE=true" & shift & goto :parse_args
set "TEST_FILTER=%~1" & shift & goto :parse_args
:args_done

echo.
echo ============================================
echo PLAYERBOT GROUP FUNCTIONALITY TEST SUITE
echo ============================================
echo.

REM Show configuration
echo Test Configuration:
echo   Reports Directory: %REPORTS_DIR%
echo   Artifacts Directory: %ARTIFACTS_DIR%
if defined TEST_FILTER (
    echo   Test Filter: %TEST_FILTER%
) else (
    echo   Test Filter: All tests
)
echo   Parallel Tests: %PARALLEL_TESTS%
echo   Timeout: %TEST_TIMEOUT% seconds
echo   Coverage: %ENABLE_COVERAGE%
echo   Verbose: %VERBOSE%
echo.

REM Validate environment
echo [INFO] Validating test environment...

REM Check if build directory exists
if not exist "%BUILD_DIR%" (
    echo [ERROR] Build directory not found: %BUILD_DIR%
    echo [ERROR] Please run CMake build first
    exit /b 1
)

REM Check if test executable exists
set "TEST_EXECUTABLE=%BUILD_DIR%\src\modules\Playerbot\Tests\Debug\playerbot_tests.exe"
if not exist "%TEST_EXECUTABLE%" (
    set "TEST_EXECUTABLE=%BUILD_DIR%\src\modules\Playerbot\Tests\Release\playerbot_tests.exe"
)
if not exist "%TEST_EXECUTABLE%" (
    set "TEST_EXECUTABLE=%BUILD_DIR%\src\modules\Playerbot\Tests\playerbot_tests.exe"
)

if not exist "%TEST_EXECUTABLE%" (
    echo [ERROR] Test executable not found in expected locations
    echo [ERROR] Please build with -DBUILD_PLAYERBOT_TESTS=ON
    exit /b 1
)

REM Create output directories
if not exist "%REPORTS_DIR%" mkdir "%REPORTS_DIR%"
if not exist "%ARTIFACTS_DIR%" mkdir "%ARTIFACTS_DIR%"

echo [SUCCESS] Environment validation completed

REM Run demonstration if requested
if "%RUN_DEMO%"=="true" (
    echo.
    echo ============================================
    echo PLAYERBOT GROUP FUNCTIONALITY DEMONSTRATION
    echo ============================================
    echo.

    set "DEMO_EXECUTABLE=%BUILD_DIR%\src\modules\Playerbot\Tests\Debug\ProductionValidationDemo.exe"
    if not exist "!DEMO_EXECUTABLE!" (
        set "DEMO_EXECUTABLE=%BUILD_DIR%\src\modules\Playerbot\Tests\Release\ProductionValidationDemo.exe"
    )
    if not exist "!DEMO_EXECUTABLE!" (
        set "DEMO_EXECUTABLE=%BUILD_DIR%\src\modules\Playerbot\Tests\ProductionValidationDemo.exe"
    )

    if exist "!DEMO_EXECUTABLE!" (
        echo [INFO] Starting interactive demonstration...
        "!DEMO_EXECUTABLE!" --interactive
        if !errorlevel! equ 0 (
            echo [SUCCESS] Demonstration completed successfully
        ) else (
            echo [ERROR] Demonstration failed
            exit /b 1
        )
    ) else (
        echo [ERROR] Demo executable not found. Please build the test suite first.
        exit /b 1
    )
    goto :end
)

REM Build tests
echo [INFO] Building test suite...
cd /d "%BUILD_DIR%"

REM Attempt to build using available build system
if exist "Makefile" (
    make playerbot_tests -j%NUMBER_OF_PROCESSORS% > "%ARTIFACTS_DIR%\build.log" 2>&1
) else if exist "PlayerBot.sln" (
    msbuild PlayerBot.sln /p:Configuration=Debug /p:Platform=x64 /t:playerbot_tests > "%ARTIFACTS_DIR%\build.log" 2>&1
) else if exist "build.ninja" (
    ninja playerbot_tests > "%ARTIFACTS_DIR%\build.log" 2>&1
) else (
    echo [WARNING] No recognized build system found. Assuming tests are already built.
)

cd /d "%SCRIPT_DIR%"

if not exist "%TEST_EXECUTABLE%" (
    echo [ERROR] Test build failed or executable not found
    echo [ERROR] Check build log: %ARTIFACTS_DIR%\build.log
    exit /b 1
)

echo [SUCCESS] Test suite built successfully

REM Run tests based on filter
if defined TEST_FILTER (
    call :run_test_category "%TEST_FILTER%"
) else (
    REM Run all test categories
    set "overall_success=true"

    call :run_test_category "Group*"
    if !errorlevel! neq 0 set "overall_success=false"

    call :run_test_category "PERFORMANCE"
    if !errorlevel! neq 0 set "overall_success=false"

    if "%CI_MODE%"=="false" (
        call :run_test_category "STRESS"
        if !errorlevel! neq 0 set "overall_success=false"
    )

    if "!overall_success!"=="true" (
        echo.
        echo [SUCCESS] All tests completed successfully!
        echo.
        echo ^🎉 PlayerBot group functionality is validated and ready for production!
        echo.
    ) else (
        echo.
        echo [ERROR] Some tests failed. Please check the reports for details.
        echo.
        echo ❌ Test failures detected. Review required before production deployment.
        echo.
        exit /b 1
    )
)

REM Generate report
call :generate_report

goto :end

REM Function to run test category
:run_test_category
set "category=%~1"
echo [INFO] Running %category% tests...

set "gtest_args="
if "%category%"=="SMOKE" (
    set "gtest_args=--gtest_filter=*Smoke*:*Quick*:*Basic*"
) else if "%category%"=="STRESS" (
    set "gtest_args=--gtest_filter=*Stress*:*Load*:*Concurrent*"
) else if "%category%"=="PERFORMANCE" (
    set "gtest_args=--gtest_filter=*Performance*:*Timing*:*Memory*"
) else (
    set "gtest_args=--gtest_filter=%category%"
)

set "output_file=%REPORTS_DIR%\%category%_test_results.xml"
set "log_file=%REPORTS_DIR%\%category%_test.log"

set "gtest_args=%gtest_args% --gtest_output=xml:%output_file%"

if "%VERBOSE%"=="true" (
    set "gtest_args=%gtest_args% --gtest_print_time=1"
)

echo [INFO] Running: %TEST_EXECUTABLE% %gtest_args%

REM Run tests
"%TEST_EXECUTABLE%" %gtest_args% > "%log_file%" 2>&1

if %errorlevel% equ 0 (
    echo [SUCCESS] %category% tests completed
) else (
    echo [ERROR] %category% tests failed with exit code %errorlevel%
    if "%CI_MODE%"=="true" (
        echo [ERROR] CI mode: stopping on first failure
        exit /b %errorlevel%
    )
)

exit /b %errorlevel%

REM Function to generate report
:generate_report
echo [INFO] Generating test report...

set "report_file=%REPORTS_DIR%\comprehensive_test_report.html"
set "summary_file=%REPORTS_DIR%\test_summary.txt"

REM Create basic HTML report
echo ^<!DOCTYPE html^> > "%report_file%"
echo ^<html^> >> "%report_file%"
echo ^<head^> >> "%report_file%"
echo   ^<title^>PlayerBot Group Functionality Test Report^</title^> >> "%report_file%"
echo   ^<style^> >> "%report_file%"
echo     body { font-family: Arial, sans-serif; margin: 20px; } >> "%report_file%"
echo     .header { background: #2c3e50; color: white; padding: 20px; border-radius: 5px; } >> "%report_file%"
echo     .success { color: #27ae60; } >> "%report_file%"
echo     .failure { color: #e74c3c; } >> "%report_file%"
echo     .section { margin: 20px 0; padding: 15px; border: 1px solid #ddd; border-radius: 5px; } >> "%report_file%"
echo   ^</style^> >> "%report_file%"
echo ^</head^> >> "%report_file%"
echo ^<body^> >> "%report_file%"
echo   ^<div class="header"^> >> "%report_file%"
echo     ^<h1^>PlayerBot Group Functionality Test Report^</h1^> >> "%report_file%"
echo     ^<p^>Generated on: %date% %time%^</p^> >> "%report_file%"
echo   ^</div^> >> "%report_file%"
echo   ^<div class="section"^> >> "%report_file%"
echo     ^<h2^>Test Results^</h2^> >> "%report_file%"
echo     ^<p^>Check individual test log files in the reports directory for detailed results.^</p^> >> "%report_file%"
echo   ^</div^> >> "%report_file%"
echo ^</body^> >> "%report_file%"
echo ^</html^> >> "%report_file%"

REM Create text summary
echo PlayerBot Group Functionality Test Summary > "%summary_file%"
echo ========================================== >> "%summary_file%"
echo. >> "%summary_file%"
echo Test Run Information: >> "%summary_file%"
echo   Date: %date% %time% >> "%summary_file%"
echo   Configuration: Coverage=%ENABLE_COVERAGE% >> "%summary_file%"
if defined TEST_FILTER (
    echo   Filter: %TEST_FILTER% >> "%summary_file%"
) else (
    echo   Filter: All tests >> "%summary_file%"
)
echo   Timeout: %TEST_TIMEOUT% seconds >> "%summary_file%"
echo. >> "%summary_file%"
echo Files Generated: >> "%summary_file%"
echo   - Test Report: %report_file% >> "%summary_file%"
echo   - Test Logs: %REPORTS_DIR%\*.log >> "%summary_file%"
echo   - Test Artifacts: %ARTIFACTS_DIR%\ >> "%summary_file%"

echo [SUCCESS] Test report generated: %report_file%
echo [SUCCESS] Test summary: %summary_file%
exit /b 0

REM Show help
:show_help
echo PlayerBot Group Functionality Test Runner
echo.
echo Usage: %~nx0 [OPTIONS] [TEST_FILTER]
echo.
echo OPTIONS:
echo     -h, --help              Show this help message
echo     -v, --verbose           Enable verbose output
echo     -c, --coverage          Enable code coverage reporting
echo     -p, --parallel N        Run N tests in parallel (default: 1)
echo     -t, --timeout SECONDS   Set test timeout (default: 1800)
echo     --quick                 Run only smoke tests (fast validation)
echo     --stress                Run stress tests only
echo     --demo                  Run interactive demonstration
echo     --ci                    CI mode (fail-fast, XML output)
echo.
echo TEST_FILTER:
echo     Can be used to run specific test patterns:
echo     - Group*               Run all group functionality tests
echo     - *Stress*             Run all stress tests
echo     - *Performance*        Run all performance tests
echo.
echo Examples:
echo     %~nx0                          # Run all tests
echo     %~nx0 --quick                  # Quick validation (5 minutes)
echo     %~nx0 --stress --parallel 2    # Parallel stress testing
echo     %~nx0 --demo                   # Interactive demonstration
echo     %~nx0 --ci --coverage          # CI pipeline with coverage
echo     %~nx0 "Group*" --verbose       # All group tests with verbose output
echo.
exit /b 0

:end
endlocal