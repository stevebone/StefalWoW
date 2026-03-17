@echo off
REM =========================================================================
REM Overnight Autonomous Mode - Startup Script
REM =========================================================================
REM
REM This script starts overnight autonomous crash fixing mode.
REM
REM Safety Features:
REM   - Creates overnight-YYYYMMDD branch (doesn't touch playerbot-dev)
REM   - Compiles every fix before committing
REM   - Only commits fixes that compile successfully
REM   - Deploys to M:/Wplayerbot automatically
REM   - Runs until you press Ctrl+C
REM
REM Morning Review:
REM   - Check overnight branch commits
REM   - Test the fixes
REM   - Merge good fixes to playerbot-dev
REM
REM =========================================================================

echo.
echo ========================================================================
echo OVERNIGHT AUTONOMOUS MODE - STARTUP
echo ========================================================================
echo.
echo This will start FULLY AUTONOMOUS crash fixing with NO human approval.
echo.
echo Safety mechanisms:
echo   1. All fixes go to overnight-YYYYMMDD branch (not playerbot-dev)
echo   2. Every fix is compiled before committing
echo   3. Failed compilations are reverted and skipped
echo   4. worldserver.exe deployed to M:/Wplayerbot after each fix
echo.
echo The system will run continuously until you press Ctrl+C.
echo.
echo In the morning, review the overnight branch and merge good fixes.
echo.
echo Press Ctrl+C anytime to stop.
echo.
pause

REM Get script directory
set SCRIPT_DIR=%~dp0
set TRINITY_ROOT=%SCRIPT_DIR%..\..

echo.
echo Starting Overnight Autonomous Mode...
echo.
echo Log file: %TRINITY_ROOT%\.claude\logs\overnight_autonomous.log
echo.
echo To monitor in another window:
echo   tail -f .claude\logs\overnight_autonomous.log
echo.

cd /d %TRINITY_ROOT%
python %SCRIPT_DIR%overnight_autonomous_mode.py

echo.
echo ========================================================================
echo OVERNIGHT MODE STOPPED
echo ========================================================================
echo.
echo Next steps:
echo   1. Check overnight branch: git log overnight-YYYYMMDD
echo   2. Review and test fixes
echo   3. Merge good fixes to playerbot-dev:
echo      git checkout playerbot-dev
echo      git merge overnight-YYYYMMDD
echo      git push
echo.
pause
