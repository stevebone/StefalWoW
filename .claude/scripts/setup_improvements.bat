@echo off
REM PlayerBot Critical Improvements Setup - Python Version

echo ===============================================================
echo PlayerBot Critical Improvements Setup (Python)
echo ===============================================================
echo.

REM Get the directory where this batch file is located
set "SCRIPT_DIR=%~dp0"
set "PYTHON_SCRIPT=%SCRIPT_DIR%implement_improvements.py"

REM Check if Python script exists
if not exist "%PYTHON_SCRIPT%" (
    echo ERROR: Python script not found at %PYTHON_SCRIPT%
    echo Please ensure implement_improvements.py is in the same directory as this batch file.
    echo.
    pause
    exit /b 1
)

REM Check if Python is available
python --version >nul 2>&1
if %ERRORLEVEL% neq 0 (
    echo ERROR: Python is not installed or not in PATH
    echo Please install Python 3.6+ and try again.
    echo Download from: https://python.org/downloads/
    echo.
    pause
    exit /b 1
)

REM Show menu for selective installation
echo This setup will install PlayerBot critical improvements:
echo.
echo   📦 Dependency Scanner - Security vulnerability scanner
echo   🔄 Rollback System - Git-based recovery system
echo   📊 Monitoring System - Real-time dashboard
echo   🔔 Alert System - Configurable notifications  
echo   ⚡ Quick Wins - Pre-commit hooks and health checks
echo   🚀 Management Launchers - Easy-to-use interfaces
echo.
echo Options:
echo   1. Install all improvements [Recommended]
echo   2. Custom selection (advanced users)
echo   3. Exit
echo.
set /p "CHOICE=Enter choice (1-3): "

REM Set parameters based on choice
set "PARAMS="
if "%CHOICE%"=="1" set "PARAMS="
if "%CHOICE%"=="3" (
    echo Exiting setup.
    exit /b 0
)

if "%CHOICE%"=="2" (
    echo.
    echo Custom Options:
    echo Add --skip-X flags to skip components:
    echo   --skip-dependency-scanner
    echo   --skip-rollback  
    echo   --skip-monitoring
    echo   --skip-alerts
    echo   --skip-quick-wins
    echo.
    echo Example: --skip-monitoring --skip-alerts
    echo.
    set /p "PARAMS=Enter parameters (or press Enter for all): "
)

REM Execute Python script
echo.
echo Running setup...
echo ===============================================================
echo.

python "%PYTHON_SCRIPT%" %PARAMS%

REM Capture exit code
set "EXIT_CODE=%ERRORLEVEL%"

echo.
echo ===============================================================
if %EXIT_CODE%==0 (
    echo [SUCCESS] PlayerBot improvements setup completed successfully!
    echo.
    echo Your system now has enhanced:
    echo   🔍 Security scanning capabilities
    echo   🔄 Automated rollback and recovery
    echo   📊 Real-time monitoring dashboard  
    echo   🔔 Intelligent alert system
    echo   ⚡ Development workflow improvements
    echo.
    echo Use the management launcher for easy access:
    echo   C:\TrinityBots\TrinityCore\.claude\playerbot_manager.bat
) else (
    echo [WARNING] Setup completed with some issues ^(exit code %EXIT_CODE%^).
    echo Please check the output above for details.
    echo Some features may still be functional.
)
echo ===============================================================

echo.
echo Press any key to continue...
pause >nul

exit /b %EXIT_CODE%
