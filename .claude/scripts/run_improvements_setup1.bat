@echo off
REM Critical Improvements Setup Launcher
REM This batch file bypasses PowerShell execution policy restrictions

echo ===============================================================
echo PlayerBot Critical Improvements Setup
echo ===============================================================
echo.

REM Get the directory where this batch file is located
set "SCRIPT_DIR=%~dp0"
set "PS_SCRIPT=%SCRIPT_DIR%implement_improvements_repaired.ps1"

REM Check if PowerShell script exists
if not exist "%PS_SCRIPT%" (
    echo ERROR: PowerShell script not found at %PS_SCRIPT%
    echo Please ensure implement_improvements_repaired.ps1 is in the same directory as this batch file.
    echo.
    pause
    exit /b 1
)

REM Show menu for selective installation
echo Select which improvements to install:
echo.
echo 1. Install all improvements [Recommended]
echo 2. Dependency Scanner only
echo 3. Rollback System only  
echo 4. Real-time Monitoring only
echo 5. Alert System only
echo 6. Quick Wins only
echo 7. Custom selection
echo 8. Skip - run with current settings
echo.
set /p "CHOICE=Enter choice (1-8): "

REM Set parameters based on choice
set "PARAMS="
if "%CHOICE%"=="1" set "PARAMS="
if "%CHOICE%"=="2" set "PARAMS=-InstallDependencyScanner -SetupRollback:$false -EnableMonitoring:$false -ConfigureAlerts:$false -QuickWins:$false"
if "%CHOICE%"=="3" set "PARAMS=-InstallDependencyScanner:$false -SetupRollback -EnableMonitoring:$false -ConfigureAlerts:$false -QuickWins:$false"
if "%CHOICE%"=="4" set "PARAMS=-InstallDependencyScanner:$false -SetupRollback:$false -EnableMonitoring -ConfigureAlerts:$false -QuickWins:$false"
if "%CHOICE%"=="5" set "PARAMS=-InstallDependencyScanner:$false -SetupRollback:$false -EnableMonitoring:$false -ConfigureAlerts -QuickWins:$false"
if "%CHOICE%"=="6" set "PARAMS=-InstallDependencyScanner:$false -SetupRollback:$false -EnableMonitoring:$false -ConfigureAlerts:$false -QuickWins"

if "%CHOICE%"=="7" (
    echo.
    echo Available parameters ^(use $true/$false^):
    echo   -InstallDependencyScanner
    echo   -SetupRollback
    echo   -EnableMonitoring
    echo   -ConfigureAlerts
    echo   -QuickWins
    echo.
    echo Example: -InstallDependencyScanner -SetupRollback:$false
    echo.
    set /p "PARAMS=Enter custom parameters: "
)

if "%CHOICE%"=="8" (
    echo Skipping installation.
    pause
    exit /b 0
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
    echo [SUCCESS] Critical improvements setup completed successfully!
    echo.
    echo Your PlayerBot system now has:
    echo   📦 Enhanced dependency scanning
    echo   🔄 Automated rollback capabilities
    echo   📊 Real-time monitoring dashboard
    echo   🔔 Intelligent alert system
    echo   ⚡ Quick development wins
) else (
    echo [ERROR] Setup completed with some errors ^(exit code %EXIT_CODE%^).
    echo Please check the output above for details.
)
echo ===============================================================

REM Show next steps
echo.
echo NEXT STEPS:
echo 1. Check desktop for "PlayerBot Monitor" shortcut
echo 2. Navigate to C:\TrinityBots\TrinityCore\.claude\ to see new files
echo 3. Run dependency scanner: python .claude\scripts\dependency_scanner.py
echo 4. Create rollback point: powershell .claude\scripts\rollback.ps1 -Mode last

echo.
echo Press any key to continue...
pause >nul

exit /b %EXIT_CODE%
