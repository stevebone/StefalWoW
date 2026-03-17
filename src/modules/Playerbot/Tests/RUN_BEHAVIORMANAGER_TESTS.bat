@echo off
REM ============================================================================
REM BehaviorManager Test Runner - Windows
REM ============================================================================
REM
REM This batch file provides convenient shortcuts for running BehaviorManager
REM unit tests on Windows platforms.
REM
REM Usage:
REM   RUN_BEHAVIORMANAGER_TESTS.bat [command]
REM
REM Commands:
REM   all         - Run all BehaviorManager tests (default)
REM   throttling  - Run throttling mechanism tests
REM   performance - Run performance tests
REM   errors      - Run error handling tests
REM   init        - Run initialization tests
REM   edge        - Run edge case tests
REM   scenarios   - Run integration scenario tests
REM   verbose     - Run all tests with verbose output
REM   repeat      - Run all tests 10 times (stability check)
REM
REM Requirements:
REM   - TrinityCore built with -DBUILD_PLAYERBOT_TESTS=ON
REM   - playerbot_tests.exe in build output directory
REM
REM ============================================================================

setlocal enabledelayedexpansion

REM Detect build directory
set BUILD_DIR=..\..\..\..\..\build_install
set TEST_EXE=%BUILD_DIR%\bin\Debug\playerbot_tests.exe

if not exist "%TEST_EXE%" (
    set TEST_EXE=%BUILD_DIR%\bin\Release\playerbot_tests.exe
)

if not exist "%TEST_EXE%" (
    echo ERROR: Could not find playerbot_tests.exe
    echo.
    echo Please build TrinityCore with -DBUILD_PLAYERBOT_TESTS=ON
    echo Expected location: %BUILD_DIR%\bin\Debug\playerbot_tests.exe
    echo.
    pause
    exit /b 1
)

REM Parse command line argument
set COMMAND=%1
if "%COMMAND%"=="" set COMMAND=all

echo ============================================================================
echo BehaviorManager Test Runner
echo ============================================================================
echo Test Executable: %TEST_EXE%
echo Command: %COMMAND%
echo ============================================================================
echo.

REM Execute appropriate test command
if "%COMMAND%"=="all" (
    echo Running all BehaviorManager tests...
    "%TEST_EXE%" --gtest_filter="BehaviorManagerTest.*"
) else if "%COMMAND%"=="throttling" (
    echo Running throttling mechanism tests...
    "%TEST_EXE%" --gtest_filter="BehaviorManagerTest.*Throttling*"
) else if "%COMMAND%"=="performance" (
    echo Running performance tests...
    "%TEST_EXE%" --gtest_filter="BehaviorManagerTest.*Performance*"
) else if "%COMMAND%"=="errors" (
    echo Running error handling tests...
    "%TEST_EXE%" --gtest_filter="BehaviorManagerTest.*ErrorHandling*"
) else if "%COMMAND%"=="init" (
    echo Running initialization tests...
    "%TEST_EXE%" --gtest_filter="BehaviorManagerTest.*Initialization*"
) else if "%COMMAND%"=="edge" (
    echo Running edge case tests...
    "%TEST_EXE%" --gtest_filter="BehaviorManagerTest.*EdgeCase*"
) else if "%COMMAND%"=="scenarios" (
    echo Running integration scenario tests...
    "%TEST_EXE%" --gtest_filter="BehaviorManagerTest.*Scenario*"
) else if "%COMMAND%"=="verbose" (
    echo Running all tests with verbose output...
    "%TEST_EXE%" --gtest_filter="BehaviorManagerTest.*" --gtest_color=yes
) else if "%COMMAND%"=="repeat" (
    echo Running all tests 10 times for stability check...
    "%TEST_EXE%" --gtest_filter="BehaviorManagerTest.*" --gtest_repeat=10
) else (
    echo ERROR: Unknown command "%COMMAND%"
    echo.
    echo Available commands:
    echo   all         - Run all BehaviorManager tests
    echo   throttling  - Run throttling mechanism tests
    echo   performance - Run performance tests
    echo   errors      - Run error handling tests
    echo   init        - Run initialization tests
    echo   edge        - Run edge case tests
    echo   scenarios   - Run integration scenario tests
    echo   verbose     - Run all tests with verbose output
    echo   repeat      - Run all tests 10 times
    echo.
    pause
    exit /b 1
)

REM Capture exit code
set EXIT_CODE=%ERRORLEVEL%

echo.
echo ============================================================================
if %EXIT_CODE%==0 (
    echo TESTS PASSED
) else (
    echo TESTS FAILED - Exit code: %EXIT_CODE%
)
echo ============================================================================
echo.

pause
exit /b %EXIT_CODE%
