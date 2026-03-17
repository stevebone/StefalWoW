@echo off
REM Quick Agent Status Dashboard
REM Shows which agents are integrated where

setlocal EnableDelayedExpansion

cls
echo ================================================================================
echo                          PLAYERBOT AGENT STATUS
echo ================================================================================
echo.

REM Check Python availability
python --version >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] Python is not installed or not in PATH
    echo.
    goto fallback_display
)

REM Run Python dashboard
cd /d "%~dp0"
python agent_dashboard.py

goto end

:fallback_display
REM Fallback display if Python is not available
echo AGENT INTEGRATION SUMMARY
echo -------------------------
echo.
echo CODE REVIEW WORKFLOW (10 agents):
echo   1. playerbot-project-coordinator
echo   2. trinity-integration-tester
echo   3. code-quality-reviewer
echo   4. security-auditor
echo   5. performance-analyzer
echo   6. cpp-architecture-optimizer
echo   7. enterprise-compliance-checker
echo   8. automated-fix-agent
echo   9. test-automation-engineer
echo   10. daily-report-generator
echo.
echo DAILY CHECKS (8 agents at different times):
echo   06:00 - Morning Health Check
echo   07:00 - Build Verification
echo   08:00 - Security Scan
echo   12:00 - Performance Check
echo   14:00 - Database Integrity
echo   16:00 - Resource Monitoring
echo   18:00 - Daily Report
echo.
echo GAMEPLAY TESTING (6 agents):
echo   - wow-bot-behavior-designer
echo   - wow-mechanics-expert
echo   - pvp-arena-tactician
echo   - wow-dungeon-raid-coordinator
echo   - wow-economy-manager
echo   - bot-learning-system
echo.
echo SUPPORT AGENTS (2 agents):
echo   - concurrency-threading-specialist
echo   - cpp-server-debugger
echo.

:end
echo.
echo Press any key to exit...
pause >nul
