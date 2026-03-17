@echo off
REM =========================================================================
REM Autonomous Crash Processing System - Startup Script
REM =========================================================================
REM
REM This script starts both the Python monitor and Claude processor
REM in separate windows for easy monitoring.
REM
REM Usage:
REM     start_autonomous_system.bat
REM
REM =========================================================================

echo.
echo ========================================================================
echo AUTONOMOUS CRASH PROCESSING SYSTEM - STARTUP
echo ========================================================================
echo.
echo Starting autonomous crash processing system...
echo.
echo This will open TWO windows:
echo   1. Python Monitor (detects crashes, creates markers)
echo   2. Claude Processor (processes markers, generates fixes)
echo.
echo Both windows must remain open for autonomous processing to work.
echo.
echo Press Ctrl+C in either window to stop that process.
echo.
pause

REM Get script directory and TrinityCore root
set SCRIPT_DIR=%~dp0
set TRINITY_ROOT=%SCRIPT_DIR%..\..

echo Starting Python Monitor...
start "Python Crash Monitor" cmd /k "cd /d %TRINITY_ROOT% && python %SCRIPT_DIR%autonomous_crash_monitor_enhanced.py"

timeout /t 2 /nobreak >nul

echo Starting Claude Processor...
start "Claude Auto-Processor" cmd /k "cd /d %TRINITY_ROOT% && python %SCRIPT_DIR%claude_auto_processor.py"

echo.
echo ========================================================================
echo AUTONOMOUS SYSTEM STARTED
echo ========================================================================
echo.
echo Two windows have been opened:
echo   - Python Crash Monitor (detects crashes)
echo   - Claude Auto-Processor (analyzes crashes)
echo.
echo Logs are being written to:
echo   %TRINITY_ROOT%\.claude\logs\autonomous_monitor_enhanced.log
echo   %TRINITY_ROOT%\.claude\logs\claude_auto_processor.log
echo.
echo To stop the system:
echo   - Press Ctrl+C in each window
echo   - Or close the windows
echo.
echo ========================================================================
echo.
pause
