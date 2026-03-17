@echo off
REM TrinityCore CMake Configuration - Release Build
REM This script configures the build directory for Release compilation

echo ========================================
echo TrinityCore Release Configuration
echo ========================================
echo.

REM Set timeout to 30 minutes (1800 seconds)
set TIMEOUT=1800

REM Set library paths
set VCPKG_ROOT=C:\libs\vcpkg
set BOOST_ROOT=C:\libs\boost_1_78_0-bin-msvc-all-32-64\boost_1_78_0
set BOOST_INCLUDEDIR=%BOOST_ROOT%\boost
set BOOST_LIBRARYDIR=%BOOST_ROOT%\lib64-msvc-14.3

echo VCPKG_ROOT: %VCPKG_ROOT%
echo BOOST_ROOT: %BOOST_ROOT%
echo.

REM Create or clean build directory
if exist build (
    echo Build directory exists. Cleaning CMake cache...
    del /q build\CMakeCache.txt 2>nul
) else (
    echo Creating build directory...
    mkdir build
)

cd build

echo.
echo Running CMake configuration for Release...
echo.

cmake .. -G "Visual Studio 17 2022" ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DBUILD_PLAYERBOT=1 ^
    -DCMAKE_TOOLCHAIN_FILE="%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake" ^
    -DBOOST_ROOT="%BOOST_ROOT%" ^
    -DBOOST_INCLUDEDIR="%BOOST_INCLUDEDIR%" ^
    -DBOOST_LIBRARYDIR="%BOOST_LIBRARYDIR%"

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo ========================================
    echo ERROR: CMake configuration failed!
    echo ========================================
    cd ..
    pause
    exit /b 1
)

cd ..

echo.
echo ========================================
echo Release Configuration Complete!
echo ========================================
echo.
echo Next step: Run build_release.bat to compile
echo.
pause
