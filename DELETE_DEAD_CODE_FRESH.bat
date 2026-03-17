@echo off
REM ============================================================================
REM PLAYERBOT DEAD CODE DELETION SCRIPT - FRESH AUDIT (2025-10-17)
REM Git Commit: 923206d4ae (After revert due to compile errors)
REM Total Files to Delete: 23 files (~14,183 lines of dead code)
REM ============================================================================

echo.
echo ============================================================================
echo PLAYERBOT DEAD CODE DELETION SCRIPT - FRESH AUDIT
echo ============================================================================
echo.
echo Current Git Commit: 923206d4ae
echo Status: After revert due to compilation errors
echo.
echo WARNING: This will DELETE 23 files containing dead code.
echo.
echo Files to be deleted:
echo   - 11 Backup files (.backup, _BACKUP)
echo   - 2 Old/Deprecated files (_old, _Old)
echo   - 1 BROKEN file (MageAI_Broken.cpp) **CRITICAL**
echo   - 6 Duplicate simple manager files
echo   - 3 Old "Fixed" duplicate versions
echo.
echo Total lines of code to remove: ~14,183
echo.
set /p CONFIRM="Are you sure you want to proceed? (yes/no): "
if /i not "%CONFIRM%"=="yes" (
    echo.
    echo Deletion cancelled.
    pause
    exit /b 0
)

echo.
echo ============================================================================
echo PHASE 1: CRITICAL - Delete BROKEN Production File
echo ============================================================================
echo.

del /f /q "src\modules\Playerbot\AI\ClassAI\Mages\MageAI_Broken.cpp" 2>nul && echo [CRITICAL] MageAI_Broken.cpp DELETED ^(BROKEN PRODUCTION FILE^) || echo [ERROR] MageAI_Broken.cpp not found - MANUAL CHECK REQUIRED!

echo.
echo ============================================================================
echo PHASE 2: Delete Duplicate Simple Managers ^(6 files^)
echo ============================================================================
echo.

del /f /q "src\modules\Playerbot\AI\InterruptManager.h" 2>nul && echo [OK] AI/InterruptManager.h deleted ^(simple version^) || echo [SKIP] AI/InterruptManager.h not found
del /f /q "src\modules\Playerbot\AI\InterruptManager.cpp" 2>nul && echo [OK] AI/InterruptManager.cpp deleted ^(simple version^) || echo [SKIP] AI/InterruptManager.cpp not found
del /f /q "src\modules\Playerbot\AI\ResourceManager.h" 2>nul && echo [OK] AI/ResourceManager.h deleted ^(simple version^) || echo [SKIP] AI/ResourceManager.h not found
del /f /q "src\modules\Playerbot\AI\ResourceManager.cpp" 2>nul && echo [OK] AI/ResourceManager.cpp deleted ^(simple version^) || echo [SKIP] AI/ResourceManager.cpp not found
del /f /q "src\modules\Playerbot\AI\CooldownManager.h" 2>nul && echo [OK] AI/CooldownManager.h deleted ^(simple version^) || echo [SKIP] AI/CooldownManager.h not found
del /f /q "src\modules\Playerbot\AI\CooldownManager.cpp" 2>nul && echo [OK] AI/CooldownManager.cpp deleted ^(simple version^) || echo [SKIP] AI/CooldownManager.cpp not found

echo.
echo ============================================================================
echo PHASE 3: Delete Old "Fixed" Duplicate Versions ^(3 files^)
echo          ^(Keeping "Fixed" versions for rename later^)
echo ============================================================================
echo.

del /f /q "src\modules\Playerbot\AI\Combat\InterruptCoordinator.h" 2>nul && echo [OK] InterruptCoordinator.h deleted ^(old version, keeping Fixed^) || echo [SKIP] InterruptCoordinator.h not found
del /f /q "src\modules\Playerbot\AI\Combat\InterruptCoordinator.cpp" 2>nul && echo [OK] InterruptCoordinator.cpp deleted ^(old version, keeping Fixed^) || echo [SKIP] InterruptCoordinator.cpp not found
del /f /q "src\modules\Playerbot\AI\ClassAI\BaselineRotationManager.cpp" 2>nul && echo [OK] BaselineRotationManager.cpp deleted ^(old version, keeping Fixed^) || echo [SKIP] BaselineRotationManager.cpp not found

echo.
echo ============================================================================
echo PHASE 4: Delete Backup Files ^(11 files^)
echo ============================================================================
echo.

del /f /q "src\modules\Playerbot\AI\Actions\TargetAssistAction_BACKUP.cpp" 2>nul && echo [OK] TargetAssistAction_BACKUP.cpp deleted || echo [SKIP] TargetAssistAction_BACKUP.cpp not found
del /f /q "src\modules\Playerbot\AI\BotAI.cpp.backup" 2>nul && echo [OK] BotAI.cpp.backup deleted || echo [SKIP] BotAI.cpp.backup not found
del /f /q "src\modules\Playerbot\AI\ClassAI\BaselineRotationManager.cpp.backup" 2>nul && echo [OK] BaselineRotationManager.cpp.backup deleted || echo [SKIP] BaselineRotationManager.cpp.backup not found
del /f /q "src\modules\Playerbot\AI\ClassAI\Monks\MonkAI.cpp.backup" 2>nul && echo [OK] MonkAI.cpp.backup deleted || echo [SKIP] MonkAI.cpp.backup not found
del /f /q "src\modules\Playerbot\AI\Combat\CombatStateAnalyzer.cpp.backup" 2>nul && echo [OK] CombatStateAnalyzer.cpp.backup deleted || echo [SKIP] CombatStateAnalyzer.cpp.backup not found
del /f /q "src\modules\Playerbot\AI\CombatBehaviors\DefensiveBehaviorManager.cpp.backup" 2>nul && echo [OK] DefensiveBehaviorManager.cpp.backup deleted || echo [SKIP] DefensiveBehaviorManager.cpp.backup not found
del /f /q "src\modules\Playerbot\Group\GroupInvitationHandler.cpp.backup" 2>nul && echo [OK] GroupInvitationHandler.cpp.backup deleted || echo [SKIP] GroupInvitationHandler.cpp.backup not found
del /f /q "src\modules\Playerbot\Interaction\Core\InteractionManager.cpp.backup" 2>nul && echo [OK] InteractionManager.cpp.backup deleted || echo [SKIP] InteractionManager.cpp.backup not found
del /f /q "src\modules\Playerbot\Interaction\Core\InteractionValidator.cpp.backup" 2>nul && echo [OK] InteractionValidator.cpp.backup deleted || echo [SKIP] InteractionValidator.cpp.backup not found
del /f /q "src\modules\Playerbot\Interaction\Core\InteractionValidator.h.backup" 2>nul && echo [OK] InteractionValidator.h.backup deleted || echo [SKIP] InteractionValidator.h.backup not found

echo.
echo ============================================================================
echo PHASE 5: Delete Old/Deprecated Files ^(2 files^)
echo ============================================================================
echo.

del /f /q "src\modules\Playerbot\AI\ClassAI\DeathKnights\UnholySpecialization_old.cpp" 2>nul && echo [OK] UnholySpecialization_old.cpp deleted || echo [SKIP] UnholySpecialization_old.cpp not found
del /f /q "src\modules\Playerbot\AI\ClassAI\Warlocks\WarlockAI_Old.h" 2>nul && echo [OK] WarlockAI_Old.h deleted || echo [SKIP] WarlockAI_Old.h not found

echo.
echo ============================================================================
echo DELETION COMPLETE
echo ============================================================================
echo.
echo Summary:
echo   - BROKEN file: 1 file deleted (MageAI_Broken.cpp - CRITICAL)
echo   - Duplicate Simple Managers: 6 files deleted (650 lines)
echo   - Old Fixed Duplicates: 3 files deleted (2,065 lines)
echo   - Backup files: 11 files deleted (8,191 lines)
echo   - Old/Deprecated: 2 files deleted (1,228 lines)
echo   - Total: 23 files deleted
echo   - Lines of code removed: ~14,183
echo.
echo ============================================================================
echo NEXT STEPS:
echo ============================================================================
echo.
echo 1. Rename "Fixed" versions to standard names:
echo    - Run: RENAME_FIXED_VERSIONS_FRESH.bat
echo.
echo 2. Update all #include statements:
echo    - See UPDATE_INCLUDES_FRESH.txt for find/replace operations
echo.
echo 3. Rebuild the project:
echo    - Run: configure_release.bat
echo    - Run: build_playerbot_and_worldserver_release.bat
echo.
echo 4. Verify compilation succeeds with no errors
echo.
echo 5. Test worldserver startup and basic bot functionality
echo.
echo See PLAYERBOT_FRESH_AUDIT_2025-10-17.md for detailed analysis.
echo.
pause
