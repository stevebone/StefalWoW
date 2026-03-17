@echo off
REM ============================================
REM Playerbot Overnight Automation Script
REM ============================================
REM Run this script before leaving for the night
REM It will perform comprehensive analysis and fixes

setlocal enabledelayedexpansion

set "PROJECT_ROOT=C:\TrinityBots\TrinityCore"
set "CLAUDE_DIR=%PROJECT_ROOT%\.claude"
set "REPORTS_DIR=%CLAUDE_DIR%\reports"
set "LOGS_DIR=%CLAUDE_DIR%\logs"
set "DATE=%date:~-4,4%-%date:~-10,2%-%date:~-7,2%"
set "TIME=%time:~0,2%-%time:~3,2%"
set "TIMESTAMP=%DATE%_%TIME: =0%"

echo.
echo ============================================
echo PLAYERBOT OVERNIGHT AUTOMATION
echo Started: %DATE% %TIME%
echo ============================================
echo.

REM Create directories if needed
if not exist "%REPORTS_DIR%" mkdir "%REPORTS_DIR%"
if not exist "%LOGS_DIR%" mkdir "%LOGS_DIR%"

REM Create lock file to indicate overnight mode
echo %TIMESTAMP% > "%CLAUDE_DIR%\overnight_mode.lock"

echo [1/8] Creating checkpoint...
cd /d "%PROJECT_ROOT%"
git stash push -m "overnight_checkpoint_%TIMESTAMP%"
git log -1 --format="%%H" > "%CLAUDE_DIR%\rollback_points\checkpoint_%TIMESTAMP%.txt"

echo [2/8] Building all configurations...
echo Building Debug...
cmake --build build --config Debug -- /m > "%LOGS_DIR%\build_debug_%TIMESTAMP%.log" 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo   WARNING: Debug build failed
) else (
    echo   Debug build OK
)

echo Building RelWithDebInfo...
cmake --build build --config RelWithDebInfo -- /m > "%LOGS_DIR%\build_relwithdebinfo_%TIMESTAMP%.log" 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo   WARNING: RelWithDebInfo build failed
) else (
    echo   RelWithDebInfo build OK
)

echo Building Release...
cmake --build build --config Release -- /m > "%LOGS_DIR%\build_release_%TIMESTAMP%.log" 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo   WARNING: Release build failed
) else (
    echo   Release build OK
)

echo [3/8] Scanning for memory leak patterns...
echo Searching for raw pointer allocations...
findstr /s /n /c:"= new " "%PROJECT_ROOT%\src\modules\Playerbot\*.cpp" > "%REPORTS_DIR%\memory_patterns_%TIMESTAMP%.txt" 2>&1
findstr /s /n /c:"new \[" "%PROJECT_ROOT%\src\modules\Playerbot\*.cpp" >> "%REPORTS_DIR%\memory_patterns_%TIMESTAMP%.txt" 2>&1

echo [4/8] Scanning for threading issues...
echo Searching for mutex patterns...
findstr /s /n /c:"std::mutex" "%PROJECT_ROOT%\src\modules\Playerbot\*.cpp" > "%REPORTS_DIR%\threading_patterns_%TIMESTAMP%.txt" 2>&1
findstr /s /n /c:"std::lock_guard" "%PROJECT_ROOT%\src\modules\Playerbot\*.cpp" >> "%REPORTS_DIR%\threading_patterns_%TIMESTAMP%.txt" 2>&1
findstr /s /n /c:"std::unique_lock" "%PROJECT_ROOT%\src\modules\Playerbot\*.cpp" >> "%REPORTS_DIR%\threading_patterns_%TIMESTAMP%.txt" 2>&1

echo [5/8] Scanning for legacy packet usage...
findstr /s /n /c:"WorldPacket" "%PROJECT_ROOT%\src\modules\Playerbot\*.cpp" > "%REPORTS_DIR%\legacy_packets_%TIMESTAMP%.txt" 2>&1

echo [6/8] Analyzing crash dumps...
if exist "%PROJECT_ROOT%\build\bin\crashes\*.dmp" (
    echo Found crash dumps, listing...
    dir /b "%PROJECT_ROOT%\build\bin\crashes\*.dmp" > "%REPORTS_DIR%\crash_dumps_%TIMESTAMP%.txt"
) else (
    echo No crash dumps found
    echo No crash dumps found > "%REPORTS_DIR%\crash_dumps_%TIMESTAMP%.txt"
)

echo [7/8] Generating code statistics...
echo Counting lines of code...
powershell -Command "(Get-ChildItem -Path '%PROJECT_ROOT%\src\modules\Playerbot' -Recurse -Include '*.cpp','*.h' | Get-Content | Measure-Object -Line).Lines" > "%REPORTS_DIR%\loc_count_%TIMESTAMP%.txt"

echo [8/8] Generating overnight report...
(
echo # Overnight Analysis Report
echo.
echo **Date**: %DATE%
echo **Started**: %TIME%
echo.
echo ## Build Status
echo.
echo ^| Config ^| Status ^|
echo ^|--------|--------|
if exist "%LOGS_DIR%\build_debug_%TIMESTAMP%.log" (
    findstr /c:"error" "%LOGS_DIR%\build_debug_%TIMESTAMP%.log" >nul 2>&1
    if !ERRORLEVEL! EQU 0 (
        echo ^| Debug ^| ❌ Failed ^|
    ) else (
        echo ^| Debug ^| ✅ OK ^|
    )
)
if exist "%LOGS_DIR%\build_relwithdebinfo_%TIMESTAMP%.log" (
    findstr /c:"error" "%LOGS_DIR%\build_relwithdebinfo_%TIMESTAMP%.log" >nul 2>&1
    if !ERRORLEVEL! EQU 0 (
        echo ^| RelWithDebInfo ^| ❌ Failed ^|
    ) else (
        echo ^| RelWithDebInfo ^| ✅ OK ^|
    )
)
if exist "%LOGS_DIR%\build_release_%TIMESTAMP%.log" (
    findstr /c:"error" "%LOGS_DIR%\build_release_%TIMESTAMP%.log" >nul 2>&1
    if !ERRORLEVEL! EQU 0 (
        echo ^| Release ^| ❌ Failed ^|
    ) else (
        echo ^| Release ^| ✅ OK ^|
    )
)
echo.
echo ## Analysis Files Generated
echo.
echo - Memory patterns: `reports/memory_patterns_%TIMESTAMP%.txt`
echo - Threading patterns: `reports/threading_patterns_%TIMESTAMP%.txt`
echo - Legacy packets: `reports/legacy_packets_%TIMESTAMP%.txt`
echo - Crash dumps: `reports/crash_dumps_%TIMESTAMP%.txt`
echo - LOC count: `reports/loc_count_%TIMESTAMP%.txt`
echo.
echo ## Next Steps
echo.
echo 1. Review build logs for warnings
echo 2. Check memory patterns for potential leaks
echo 3. Analyze threading patterns for deadlock risks
echo 4. Plan packet migration for remaining legacy packets
echo.
echo ---
echo *Generated by Overnight Automation Script*
) > "%REPORTS_DIR%\OVERNIGHT_REPORT_%TIMESTAMP%.md"

REM Remove lock file
del "%CLAUDE_DIR%\overnight_mode.lock" 2>nul

echo.
echo ============================================
echo OVERNIGHT AUTOMATION COMPLETE
echo Report: %REPORTS_DIR%\OVERNIGHT_REPORT_%TIMESTAMP%.md
echo ============================================
echo.

REM Open report
start "" "%REPORTS_DIR%\OVERNIGHT_REPORT_%TIMESTAMP%.md"

pause
