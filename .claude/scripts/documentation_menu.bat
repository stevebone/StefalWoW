@echo off
REM Quick launcher for PlayerBot Documentation System

set PYTHON_PATH=python

echo ================================================================================
echo                    PlayerBot Documentation System
echo ================================================================================
echo.
echo Select an option:
echo.
echo [1] Full Documentation (Analyze + Generate + Insert Comments)
echo [2] Generate Documentation Only (No Comment Insertion)
echo [3] Analyze Code Only
echo [4] Insert Comments (Requires existing documentation.json)
echo [5] Open Existing Documentation
echo [Q] Quit
echo.

set /p choice="Enter your choice: "

set PROJECT_ROOT=C:\TrinityBots\TrinityCore
set SCRIPT_DIR=%PROJECT_ROOT%\.claude\scripts

if "%choice%"=="1" (
    echo.
    echo Starting FULL documentation workflow...
    %PYTHON_PATH% "%SCRIPT_DIR%\document_project.py" --mode full -autofix
    pause
) else if "%choice%"=="2" (
    echo.
    echo Generating documentation without modifying code...
    %PYTHON_PATH% "%SCRIPT_DIR%\document_project.py" --mode document
    pause
) else if "%choice%"=="3" (
    echo.
    echo Analyzing code structure...
    %PYTHON_PATH% "%SCRIPT_DIR%\document_project.py" -Mode analyze
    pause
) else if "%choice%"=="4" (
    echo.
    echo Inserting comments into code...
    %PYTHON_PATH% "%SCRIPT_DIR%\document_project.py" --mode insert -autofix
    pause
) else if "%choice%"=="5" (
    echo.
    echo Opening existing documentation...
    if exist "%PROJECT_ROOT%\.claude\documentation\index.html" (
        start "" "%PROJECT_ROOT%\.claude\documentation\index.html"
    ) else (
        echo No documentation found. Please generate first.
        pause
    )
) else if /i "%choice%"=="Q" (
    echo.
    echo Goodbye!
    exit /b 0
) else (
    echo.
    echo Invalid choice. Please try again.
    pause
    "%~f0"
)
