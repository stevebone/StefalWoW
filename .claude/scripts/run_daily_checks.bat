@echo off
REM Daily Automation Batch Script for PlayerBot Project
REM This script runs the daily automation checks

setlocal EnableDelayedExpansion

REM Configuration
set PROJECT_ROOT=C:\TrinityBots\TrinityCore
set CLAUDE_DIR=%PROJECT_ROOT%\.claude
set SCRIPTS_DIR=%CLAUDE_DIR%\scripts
set LOGS_DIR=%CLAUDE_DIR%\logs
set PYTHON_PATH=python

REM Create logs directory if not exists
if not exist "%LOGS_DIR%" mkdir "%LOGS_DIR%"

REM Get current date and time for logging
for /f "tokens=2 delims==" %%I in ('wmic os get localdatetime /value') do set datetime=%%I
set CURRENT_DATE=%datetime:~0,4%-%datetime:~4,2%-%datetime:~6,2%
set CURRENT_TIME=%datetime:~8,2%-%datetime:~10,2%-%datetime:~12,2%
set LOG_FILE=%LOGS_DIR%\automation_%CURRENT_DATE%.log

echo ======================================== >> "%LOG_FILE%"
echo PlayerBot Daily Automation Started >> "%LOG_FILE%"
echo Date: %CURRENT_DATE% Time: %CURRENT_TIME% >> "%LOG_FILE%"
echo ======================================== >> "%LOG_FILE%"

REM Parse command line arguments
set CHECK_TYPE=full
set AUTO_FIX=false
set EMAIL_REPORT=false

:parse_args
if "%~1"=="" goto end_parse
if /i "%~1"=="--type" (
    set CHECK_TYPE=%~2
    shift
    shift
    goto parse_args
)
if /i "%~1"=="--autofix" (
    set AUTO_FIX=true
    shift
    goto parse_args
)
if /i "%~1"=="--email" (
    set EMAIL_REPORT=true
    shift
    goto parse_args
)
shift
goto parse_args
:end_parse

echo Running check type: %CHECK_TYPE% >> "%LOG_FILE%"

REM Determine which checks to run based on time of day
for /f "tokens=1 delims=:" %%h in ("%TIME%") do set HOUR=%%h

if %HOUR% LSS 10 (
    set DEFAULT_CHECK=morning
) else if %HOUR% LSS 15 (
    set DEFAULT_CHECK=midday
) else (
    set DEFAULT_CHECK=evening
)

if "%CHECK_TYPE%"=="auto" set CHECK_TYPE=%DEFAULT_CHECK%

echo Executing %CHECK_TYPE% checks... >> "%LOG_FILE%"

REM Run PowerShell automation script
echo Starting Python automation... >> "%LOG_FILE%"
    %PYTHON_PATH% "%SCRIPTS_DIR%\daily_automation.py" -CheckType %CHECK_TYPE% -ProjectRoot "%PROJECT_ROOT%" -AutoFix:%AUTO_FIX% -EmailReport:%EMAIL_REPORT% >> "%LOG_FILE%" 2>&1

set PS_EXIT_CODE=%ERRORLEVEL%
if %PS_EXIT_CODE% NEQ 0 (
    echo PowerShell script failed with exit code %PS_EXIT_CODE% >> "%LOG_FILE%"
    goto error_handler
)

REM Run Python master review if full check
if "%CHECK_TYPE%"=="full" (
    echo Starting Python master review... >> "%LOG_FILE%"
    %PYTHON_PATH% "%SCRIPTS_DIR%\master_review.py" --mode standard --project-root "%PROJECT_ROOT%" >> "%LOG_FILE%" 2>&1
    
    if !ERRORLEVEL! NEQ 0 (
        echo Python script failed with exit code !ERRORLEVEL! >> "%LOG_FILE%"
        goto error_handler
    )
)

REM Check for critical issues
findstr /C:"CRITICAL" /C:"ERROR" "%LOG_FILE%" >nul
if %ERRORLEVEL% EQU 0 (
    echo Critical issues found! Sending notifications... >> "%LOG_FILE%"
    REM Add notification logic here (email, Slack, etc.)
)

echo ======================================== >> "%LOG_FILE%"
echo Automation completed successfully >> "%LOG_FILE%"
echo ======================================== >> "%LOG_FILE%"

REM Open report in browser if running interactively
if "%CHECK_TYPE%"=="full" (
    start "" "%CLAUDE_DIR%\reports\report_%CURRENT_DATE%.html"
)

exit /b 0

:error_handler
echo ======================================== >> "%LOG_FILE%"
echo Automation failed with errors >> "%LOG_FILE%"
echo Please check the log file for details >> "%LOG_FILE%"
echo ======================================== >> "%LOG_FILE%"

REM Send error notification
REM Add error notification logic here

exit /b 1
