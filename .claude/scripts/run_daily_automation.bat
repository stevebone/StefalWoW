@echo off
REM Daily Automation Launcher - Debug Version
REM This batch file bypasses PowerShell execution policy restrictions and enables debug mode

echo Starting PlayerBot Daily Automation (Debug Version)...
echo.

REM Get the directory where this batch file is located
set "SCRIPT_DIR=%~dp0"
set "PS_SCRIPT=%SCRIPT_DIR%daily_automation_debug.ps1"

REM Check if PowerShell script exists
if not exist "%PS_SCRIPT%" (
    echo ERROR: PowerShell script not found at %PS_SCRIPT%
    echo Please ensure daily_automation_debug.ps1 is in the same directory as this batch file.
    echo.
    echo Available files in directory:
    dir "%SCRIPT_DIR%*.ps1" /b
    pause
    exit /b 1
)

REM Show menu for execution options
echo Select execution mode:
echo 1. Full review (default)
echo 2. Morning checks only
echo 3. Midday checks (security + performance)
echo 4. Evening summary
echo 5. Critical checks only
echo 6. Custom parameters
echo.
set /p "CHOICE=Enter choice (1-6, or press Enter for default): "

REM Set parameters based on choice
set "PARAMS=-DebugMode"
if "%CHOICE%"=="1" set "PARAMS=%PARAMS% -CheckType full"
if "%CHOICE%"=="2" set "PARAMS=%PARAMS% -CheckType morning"
if "%CHOICE%"=="3" set "PARAMS=%PARAMS% -CheckType midday"
if "%CHOICE%"=="4" set "PARAMS=%PARAMS% -CheckType evening"
if "%CHOICE%"=="5" set "PARAMS=%PARAMS% -CheckType critical"

if "%CHOICE%"=="6" (
    echo.
    echo Available parameters:
    echo   -CheckType: morning, midday, evening, full, critical
    echo   -AutoFix: Enable automatic fixes
    echo   -EmailReport: Send email report
    echo   -DebugMode: Enable debug output ^(already enabled^)
    echo.
    set /p "CUSTOM_PARAMS=Enter custom parameters: "
    set "PARAMS=-DebugMode %CUSTOM_PARAMS%"
)

REM Add any additional command line arguments
if not "%*"=="" (
    set "PARAMS=%PARAMS% %*"
)

REM Execute PowerShell script with bypass policy
echo.
echo Executing: %PS_SCRIPT% %PARAMS%
echo ===============================================================
echo.

PowerShell.exe -ExecutionPolicy Bypass -NoProfile -File "%PS_SCRIPT%" %PARAMS%

REM Capture exit code
set "EXIT_CODE=%ERRORLEVEL%"

echo.
echo ===============================================================
if %EXIT_CODE%==0 (
    echo [SUCCESS] Daily automation completed successfully.
    echo No critical issues found.
) else if %EXIT_CODE%==1 (
    echo [WARNING] Daily automation completed with critical issues found.
    echo Please check the log file and generated report.
) else if %EXIT_CODE%==2 (
    echo [ERROR] Daily automation failed with fatal error.
    echo Please check the log file for details.
) else (
    echo [UNKNOWN] Daily automation completed with unexpected exit code %EXIT_CODE%.
)
echo ===============================================================

REM Show log file location and recent entries
set "LOG_DIR=C:\TrinityBots\TrinityCore\.claude"
if exist "%LOG_DIR%\automation.log" (
    echo.
    echo Log file: %LOG_DIR%\automation.log
    echo Reports directory: %LOG_DIR%\reports
    echo.
    echo Last 5 log entries:
    echo -------------------
    PowerShell -Command "Get-Content '%LOG_DIR%\automation.log' | Select-Object -Last 5"
)

REM Show generated report if exists
set "REPORT_DATE=%date:~6,4%-%date:~3,2%-%date:~0,2%"
set "REPORT_FILE=%LOG_DIR%\reports\daily_report_%REPORT_DATE%.html"
if exist "%REPORT_FILE%" (
    echo.
    echo Report generated: %REPORT_FILE%
    set /p "OPEN_REPORT=Open report in browser? (Y/N): "
    if /i "!OPEN_REPORT!"=="Y" start "" "%REPORT_FILE%"
)

echo.
echo Press any key to continue...
pause >nul

exit /b %EXIT_CODE%
