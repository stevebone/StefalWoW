@echo off
echo Creating crash analysis batch file...

REM Set up Windows Error Reporting to create full crash dumps
reg add "HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\Windows Error Reporting\LocalDumps" /v DumpFolder /t REG_EXPAND_SZ /d "C:\TrinityBots\CrashDumps" /f
reg add "HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\Windows Error Reporting\LocalDumps" /v DumpCount /t REG_DWORD /d 10 /f
reg add "HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\Windows Error Reporting\LocalDumps" /v DumpType /t REG_DWORD /d 2 /f

REM Create crash dump directory
mkdir "C:\TrinityBots\CrashDumps" 2>nul

REM Enable Application Verifier for worldserver.exe to catch heap corruption
echo Enable Application Verifier for worldserver.exe manually:
echo 1. Run: appverif.exe
echo 2. Add worldserver.exe
echo 3. Enable: Basics (Heaps, Handles, Locks)
echo 4. Click Save

echo Crash analysis setup complete.
pause