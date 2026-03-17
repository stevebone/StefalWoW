@echo off
REM PlayerBot Management Launcher

echo ===============================================
echo PlayerBot Management System
echo ===============================================
echo.

echo Select an option:
echo 1. Run Security Scan
echo 2. Start Monitoring Service  
echo 3. Create Rollback Point
echo 4. Emergency Rollback
echo 5. System Health Check
echo 6. Open Monitoring Dashboard
echo 7. Exit
echo.
set /p choice=Enter your choice (1-7): 

if "%choice%"=="1" (
    echo Running security scan...
    python "C:\TrinityBots\TrinityCore\.claude/scripts/dependency_scanner.py"
) else if "%choice%"=="2" (
    echo Starting monitoring service...
    python "C:\TrinityBots\TrinityCore\.claude/monitoring/monitor_service.py"
) else if "%choice%"=="3" (
    echo Creating rollback point...
    python "C:\TrinityBots\TrinityCore\.claude/scripts/rollback.py" create
) else if "%choice%"=="4" (
    echo Performing emergency rollback...
    python "C:\TrinityBots\TrinityCore\.claude/scripts/rollback.py" emergency
) else if "%choice%"=="5" (
    echo Running health check...
    python "C:\TrinityBots\TrinityCore\.claude/scripts/health_check.py"
) else if "%choice%"=="6" (
    echo Opening monitoring dashboard...
    start "C:\TrinityBots\TrinityCore\.claude/monitoring/dashboard.html"
) else if "%choice%"=="7" (
    echo Goodbye!
    exit /b 0
) else (
    echo Invalid choice!
    pause
    goto start
)

echo.
pause
