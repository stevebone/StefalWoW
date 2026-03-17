@echo off
setlocal EnableDelayedExpansion

REM Windows-optimized PlayerBot Python Scripts Launcher
REM Automatically detects Python installation and runs scripts

title PlayerBot Automation System

echo ===============================================
echo PlayerBot Python Scripts Launcher (Windows)
echo ===============================================
echo.

REM Check Python installation
echo Checking Python installation...
set "PYTHON_CMD="

REM Try different Python commands in order of preference
for %%i in (python py.exe python3 python.exe) do (
    %%i --version >nul 2>&1
    if !ERRORLEVEL! EQU 0 (
        set "PYTHON_CMD=%%i"
        goto :python_found
    )
)

:no_python
echo ERROR: Python not found!
echo.
echo Please install Python 3.6+ from:
echo https://python.org/downloads/
echo.
echo Make sure to:
echo 1. Check "Add Python to PATH" during installation
echo 2. Install for all users (recommended)
echo.
pause
exit /b 1

:python_found
echo ✓ Found Python: !PYTHON_CMD!
for /f "tokens=*" %%a in ('!PYTHON_CMD! --version 2^>^&1') do set "PYTHON_VERSION=%%a"
echo ✓ Version: !PYTHON_VERSION!
echo.

REM Check if scripts exist
set "SCRIPT_DIR=%~dp0"
set "MISSING_SCRIPTS="

if not exist "%SCRIPT_DIR%daily_automation.py" (
    set "MISSING_SCRIPTS=!MISSING_SCRIPTS! daily_automation.py"
)
if not exist "%SCRIPT_DIR%setup_automation.py" (
    set "MISSING_SCRIPTS=!MISSING_SCRIPTS! setup_automation.py"
)
if not exist "%SCRIPT_DIR%document_project.py" (
    set "MISSING_SCRIPTS=!MISSING_SCRIPTS! document_project.py"
)

if not "!MISSING_SCRIPTS!"=="" (
    echo ERROR: Missing script files:!MISSING_SCRIPTS!
    echo.
    echo Please ensure all Python scripts are in the same directory as this batch file.
    pause
    exit /b 1
)

:main_menu
cls
echo ===============================================
echo PlayerBot Automation System
echo ===============================================
echo Python: !PYTHON_CMD! ^| !PYTHON_VERSION!
echo Scripts Directory: %SCRIPT_DIR%
echo ===============================================
echo.

echo Select a script to run:
echo.
echo 1. 🚀 Daily Automation - Run scheduled tasks and code reviews
echo 2. ⚙️  Setup Automation - Configure the complete automation system  
echo 3. 📚 Document Project - Generate comprehensive documentation
echo 4. 🔧 System Information - Show system and script information
echo 5. ❌ Exit
echo.
set /p "choice=Enter your choice (1-5): "

if "%choice%"=="1" goto :daily_automation
if "%choice%"=="2" goto :setup_automation
if "%choice%"=="3" goto :document_project
if "%choice%"=="4" goto :system_info
if "%choice%"=="5" goto :exit
echo Invalid choice! Please try again.
timeout /t 2 /nobreak >nul
goto :main_menu

:daily_automation
cls
echo ===============================================
echo Daily Automation - PlayerBot
echo ===============================================
echo.
echo Select automation type:
echo.
echo 1. 🌅 Morning Checks - Quick health verification
echo 2. 🌞 Midday Checks - Security scan and performance  
echo 3. 🌅 Evening Summary - Daily summary report
echo 4. 🔍 Full Review - Complete code review and analysis
echo 5. 🚨 Critical Checks - Emergency security and stability
echo 6. 🔧 Custom Options - Advanced configuration
echo 7. ⬅️  Back to Main Menu
echo.
set /p "subChoice=Select automation type (1-7): "

set "checkType=full"
set "extraArgs="

if "%subChoice%"=="1" set "checkType=morning"
if "%subChoice%"=="2" set "checkType=midday"  
if "%subChoice%"=="3" set "checkType=evening"
if "%subChoice%"=="4" set "checkType=full"
if "%subChoice%"=="5" set "checkType=critical"
if "%subChoice%"=="6" goto :daily_custom
if "%subChoice%"=="7" goto :main_menu

goto :run_daily

:daily_custom
echo.
echo Custom Daily Automation Options:
echo.
set /p "customType=Enter check type (morning/midday/evening/full/critical): "
set "checkType=!customType!"

echo.
set /p "autoFix=Enable auto-fix? (Y/N): "
if /i "!autoFix!"=="Y" set "extraArgs=!extraArgs! --auto-fix"

echo.
set /p "emailReport=Send email report? (Y/N): "
if /i "!emailReport!"=="Y" set "extraArgs=!extraArgs! --email-report"

echo.
set /p "customRoot=Custom project root? (Leave empty for default): "
if not "!customRoot!"=="" set "extraArgs=!extraArgs! --project-root "!customRoot!""

:run_daily
echo.
echo ===============================================
echo Running Daily Automation
echo ===============================================
echo Check Type: !checkType!
echo Extra Arguments: !extraArgs!
echo ===============================================
echo.

!PYTHON_CMD! "%SCRIPT_DIR%daily_automation.py" --check-type !checkType! !extraArgs!
set "EXIT_CODE=!ERRORLEVEL!"

echo.
echo ===============================================
if !EXIT_CODE! EQU 0 (
    echo ✅ Daily automation completed successfully!
) else (
    echo ❌ Daily automation failed ^(exit code !EXIT_CODE!^)
)
echo ===============================================
echo.
echo Press any key to return to main menu...
pause >nul
goto :main_menu

:setup_automation
cls
echo ===============================================
echo Setup Automation System
echo ===============================================
echo.
echo This will configure the complete PlayerBot automation system.
echo.
echo What will be installed:
echo ✓ Directory structure
echo ✓ Python dependencies  
echo ✓ Agent templates
echo ✓ Workflow templates
echo ✓ Automation scripts
echo ✓ Task scheduler configuration
echo.
echo WARNING: This may require Administrator privileges for:
echo - Installing Python packages
echo - Configuring Windows Task Scheduler
echo.

set /p "confirm=Continue with setup? (Y/N): "
if /i not "%confirm%"=="Y" goto :main_menu

echo.
echo Setup Options:
echo.
set /p "skipDeps=Skip dependency installation? (Y/N): "
set "setupArgs="
if /i "%skipDeps%"=="Y" set "setupArgs=!setupArgs! --skip-dependencies"

set /p "skipScheduler=Skip scheduler configuration? (Y/N): "
if /i "%skipScheduler%"=="Y" set "setupArgs=!setupArgs! --skip-scheduler"

set /p "noTest=Skip test run? (Y/N): "
if /i "%noTest%"=="Y" set "setupArgs=!setupArgs! --no-test"

set /p "customRoot=Custom project root? (Leave empty for default): "
if not "!customRoot!"=="" set "setupArgs=!setupArgs! --project-root "!customRoot!""

echo.
echo ===============================================
echo Running Setup Automation
echo ===============================================
echo Arguments: !setupArgs!
echo ===============================================
echo.

!PYTHON_CMD! "%SCRIPT_DIR%setup_automation.py" !setupArgs!
set "EXIT_CODE=!ERRORLEVEL!"

echo.
echo ===============================================
if !EXIT_CODE! EQU 0 (
    echo ✅ Setup completed successfully!
    echo.
    echo Next Steps:
    echo 1. Run daily automation to test the system
    echo 2. Configure Windows Task Scheduler if not done automatically
    echo 3. Customize agent templates as needed
) else (
    echo ❌ Setup failed ^(exit code !EXIT_CODE!^)
    echo Please check the error messages above.
)
echo ===============================================
echo.
echo Press any key to return to main menu...
pause >nul
goto :main_menu

:document_project
cls
echo ===============================================
echo Document Project - PlayerBot
echo ===============================================
echo.
echo Select documentation mode:
echo.
echo 1. 🔍 Analyze Only - Analyze code structure and metrics
echo 2. 📄 Generate Docs - Create documentation from analysis
echo 3. ✍️  Insert Comments - Add documentation comments to source
echo 4. 🚀 Full Workflow - Complete documentation process
echo 5. 🔧 Custom Options - Advanced configuration
echo 6. ⬅️  Back to Main Menu
echo.
set /p "docChoice=Select documentation mode (1-6): "

set "docMode=full"
set "docArgs="

if "%docChoice%"=="1" set "docMode=analyze"
if "%docChoice%"=="2" set "docMode=document"
if "%docChoice%"=="3" set "docMode=insert"
if "%docChoice%"=="4" set "docMode=full"
if "%docChoice%"=="5" goto :doc_custom
if "%docChoice%"=="6" goto :main_menu

goto :run_documentation

:doc_custom
echo.
echo Custom Documentation Options:
echo.
set /p "customMode=Enter mode (analyze/document/insert/full): "
set "docMode=!customMode!"

echo.
echo Output format options:
echo 1. HTML only
echo 2. Markdown only  
echo 3. JSON only
echo 4. All formats
set /p "formatChoice=Select output format (1-4): "

set "outputFormat=all"
if "%formatChoice%"=="1" set "outputFormat=html"
if "%formatChoice%"=="2" set "outputFormat=markdown"
if "%formatChoice%"=="3" set "outputFormat=json"
if "%formatChoice%"=="4" set "outputFormat=all"

set "docArgs=!docArgs! --output-format !outputFormat!"

echo.
set /p "autoFix=Enable auto-fix? (Y/N): "
if /i "!autoFix!"=="Y" set "docArgs=!docArgs! --auto-fix"

set /p "noBackup=Skip backup creation? (Y/N): "
if /i "!noBackup!"=="Y" set "docArgs=!docArgs! --no-backup"

set /p "customDirs=Custom directories? (Leave empty for default): "
if not "!customDirs!"=="" set "docArgs=!docArgs! --directories !customDirs!"

set /p "customRoot=Custom project root? (Leave empty for default): "
if not "!customRoot!"=="" set "docArgs=!docArgs! --project-root "!customRoot!""

:run_documentation
echo.
echo ===============================================
echo Running Documentation Workflow
echo ===============================================
echo Mode: !docMode!
echo Arguments: !docArgs!
echo ===============================================
echo.

!PYTHON_CMD! "%SCRIPT_DIR%document_project.py" --mode !docMode! !docArgs!
set "EXIT_CODE=!ERRORLEVEL!"

echo.
echo ===============================================
if !EXIT_CODE! EQU 0 (
    echo ✅ Documentation workflow completed successfully!
    echo.
    echo Check the .claude\documentation directory for generated files.
) else (
    echo ❌ Documentation workflow failed ^(exit code !EXIT_CODE!^)
)
echo ===============================================
echo.
echo Press any key to return to main menu...
pause >nul
goto :main_menu

:system_info
cls
echo ===============================================
echo System Information
echo ===============================================
echo.

echo Python Information:
echo ==================
!PYTHON_CMD! --version
echo Python Command: !PYTHON_CMD!
echo.

echo Python Packages:
echo ================
echo Checking key packages...
for %%p in (psutil requests click) do (
    !PYTHON_CMD! -c "import %%p; print('✓ %%p:', %%p.__version__)" 2>nul || echo "✗ %%p: Not installed"
)
echo.

echo Script Information:
echo ===================
if exist "%SCRIPT_DIR%daily_automation.py" (
    echo ✓ daily_automation.py
) else (
    echo ✗ daily_automation.py - Missing
)

if exist "%SCRIPT_DIR%setup_automation.py" (
    echo ✓ setup_automation.py  
) else (
    echo ✗ setup_automation.py - Missing
)

if exist "%SCRIPT_DIR%document_project.py" (
    echo ✓ document_project.py
) else (
    echo ✗ document_project.py - Missing
)
echo.

echo System Environment:
echo ===================
echo OS: %OS%
echo Processor: %PROCESSOR_IDENTIFIER%
echo Script Directory: %SCRIPT_DIR%
echo.

echo Press any key to return to main menu...
pause >nul
goto :main_menu

:exit
echo.
echo Thanks for using PlayerBot Automation System!
echo Goodbye! 👋
timeout /t 2 /nobreak >nul
exit /b 0
