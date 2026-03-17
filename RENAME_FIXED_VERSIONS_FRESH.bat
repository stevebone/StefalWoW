@echo off
REM ============================================================================
REM PLAYERBOT - RENAME "FIXED" VERSIONS TO STANDARD NAMES - FRESH AUDIT
REM Generated from: PLAYERBOT_FRESH_AUDIT_2025-10-17.md
REM Git Commit: 923206d4ae
REM ============================================================================

echo.
echo ============================================================================
echo PLAYERBOT - RENAME "FIXED" VERSIONS TO STANDARD NAMES
echo ============================================================================
echo.
echo This script renames "Fixed" versions to their standard names:
echo.
echo   1. InterruptCoordinatorFixed.h ^-^> InterruptCoordinator.h
echo   2. InterruptCoordinatorFixed.cpp ^-^> InterruptCoordinator.cpp
echo   3. BaselineRotationManagerFixed.cpp ^-^> BaselineRotationManager.cpp
echo.
echo WARNING: Make sure you have deleted the old versions first!
echo          Run DELETE_DEAD_CODE_FRESH.bat before running this script.
echo.
set /p CONFIRM="Are you sure you want to proceed? (yes/no): "
if /i not "%CONFIRM%"=="yes" (
    echo.
    echo Rename cancelled.
    pause
    exit /b 0
)

echo.
echo ============================================================================
echo Verifying Old Versions Are Deleted
echo ============================================================================
echo.

if exist "src\modules\Playerbot\AI\Combat\InterruptCoordinator.h" (
    echo [ERROR] Old InterruptCoordinator.h still exists!
    echo         Please run DELETE_DEAD_CODE_FRESH.bat first.
    pause
    exit /b 1
)

if exist "src\modules\Playerbot\AI\Combat\InterruptCoordinator.cpp" (
    echo [ERROR] Old InterruptCoordinator.cpp still exists!
    echo         Please run DELETE_DEAD_CODE_FRESH.bat first.
    pause
    exit /b 1
)

if exist "src\modules\Playerbot\AI\ClassAI\BaselineRotationManager.cpp" (
    echo [ERROR] Old BaselineRotationManager.cpp still exists!
    echo         Please run DELETE_DEAD_CODE_FRESH.bat first.
    pause
    exit /b 1
)

echo [OK] All old versions verified deleted.

echo.
echo ============================================================================
echo Verifying Fixed Versions Exist
echo ============================================================================
echo.

if not exist "src\modules\Playerbot\AI\Combat\InterruptCoordinatorFixed.h" (
    echo [ERROR] InterruptCoordinatorFixed.h not found!
    echo         Make sure the Fixed version exists before renaming.
    pause
    exit /b 1
)

if not exist "src\modules\Playerbot\AI\Combat\InterruptCoordinatorFixed.cpp" (
    echo [ERROR] InterruptCoordinatorFixed.cpp not found!
    echo         Make sure the Fixed version exists before renaming.
    pause
    exit /b 1
)

if not exist "src\modules\Playerbot\AI\ClassAI\BaselineRotationManagerFixed.cpp" (
    echo [ERROR] BaselineRotationManagerFixed.cpp not found!
    echo         Make sure the Fixed version exists before renaming.
    pause
    exit /b 1
)

echo [OK] All Fixed versions verified present.

echo.
echo ============================================================================
echo Renaming Files
echo ============================================================================
echo.

REM Rename files
ren "src\modules\Playerbot\AI\Combat\InterruptCoordinatorFixed.h" "InterruptCoordinator.h" && echo [OK] Renamed InterruptCoordinatorFixed.h ^-^> InterruptCoordinator.h || echo [ERROR] Failed to rename InterruptCoordinatorFixed.h
ren "src\modules\Playerbot\AI\Combat\InterruptCoordinatorFixed.cpp" "InterruptCoordinator.cpp" && echo [OK] Renamed InterruptCoordinatorFixed.cpp ^-^> InterruptCoordinator.cpp || echo [ERROR] Failed to rename InterruptCoordinatorFixed.cpp
ren "src\modules\Playerbot\AI\ClassAI\BaselineRotationManagerFixed.cpp" "BaselineRotationManager.cpp" && echo [OK] Renamed BaselineRotationManagerFixed.cpp ^-^> BaselineRotationManager.cpp || echo [ERROR] Failed to rename BaselineRotationManagerFixed.cpp

echo.
echo ============================================================================
echo RENAME COMPLETE
echo ============================================================================
echo.
echo CRITICAL NEXT STEP: Update all #include statements
echo.
echo Search and replace across the entire Playerbot module:
echo.
echo   Find:    #include "InterruptCoordinatorFixed.h"
echo   Replace: #include "InterruptCoordinator.h"
echo.
echo   Find:    #include "BaselineRotationManagerFixed.cpp"
echo   Replace: #include "BaselineRotationManager.cpp"
echo.
echo Use Visual Studio "Replace in Files" or similar tool.
echo See UPDATE_INCLUDES_FRESH.txt for complete list of replacements.
echo.
echo After updating includes:
echo   1. Run: configure_release.bat
echo   2. Run: build_playerbot_and_worldserver_release.bat
echo   3. Verify compilation succeeds
echo   4. Test worldserver startup
echo.
pause
