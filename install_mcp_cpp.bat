@echo off
REM ============================================
REM MCP-CPP-SERVER Installation Script
REM ============================================
REM WICHTIG: Schließe ALLE IDEs (VS Code, Cursor, etc.) vor dem Ausführen!

echo.
echo ============================================
echo MCP-CPP-SERVER Installation
echo ============================================
echo.
echo WARNUNG: Bitte schließe alle IDEs (VS Code, Cursor, etc.) bevor du fortfährst!
echo.
pause

echo.
echo [1/4] Setze Rust Toolchain...
rustup default stable
if %ERRORLEVEL% NEQ 0 (
    echo FEHLER: Rust Toolchain konnte nicht gesetzt werden.
    echo Bitte stelle sicher, dass alle IDEs geschlossen sind.
    pause
    exit /b 1
)

echo.
echo [2/4] Installiere mcp-cpp-server...
cargo install mcp-cpp-server
if %ERRORLEVEL% NEQ 0 (
    echo FEHLER: mcp-cpp-server Installation fehlgeschlagen.
    pause
    exit /b 1
)

echo.
echo [3/4] Generiere compile_commands.json...
cd /d C:\TrinityBots\TrinityCore
cmake -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DBUILD_PLAYERBOT=1

echo.
echo [4/4] Verifiziere Installation...
where mcp-cpp-server
if %ERRORLEVEL% NEQ 0 (
    echo WARNUNG: mcp-cpp-server nicht im PATH gefunden.
    echo Pfad: %USERPROFILE%\.cargo\bin\mcp-cpp-server.exe
)

echo.
echo ============================================
echo Installation abgeschlossen!
echo ============================================
echo.
echo Nächste Schritte:
echo 1. Starte Claude Code neu
echo 2. Die .mcp.json ist bereits konfiguriert
echo.
pause
