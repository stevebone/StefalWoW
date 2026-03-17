# TrinityCore Build System Documentation

## Overview

This document describes the complete build system for TrinityCore with Playerbot module support, including both Release and Debug configurations.

## Build Configurations

### Release Configuration
- **Purpose**: Production deployment, performance testing
- **Optimization**: Full compiler optimizations enabled (/O2)
- **Debug Symbols**: Limited (PDB with optimizations)
- **Binary Size**: ~25MB (worldserver.exe)
- **Performance**: Fastest execution speed
- **Memory Usage**: Lower runtime memory footprint
- **Use Case**: Production servers, performance benchmarking, final testing

### Debug Configuration
- **Purpose**: Development, debugging, crash analysis
- **Optimization**: Disabled for easier debugging (/Od)
- **Debug Symbols**: Complete (full PDB information)
- **Binary Size**: ~76MB (worldserver.exe) + 488MB (worldserver.pdb)
- **Performance**: Slower execution (20-50% slower than Release)
- **Memory Usage**: Higher runtime memory footprint
- **Use Case**: Development, WinDbg debugging, Visual Studio debugging, crash analysis

### Configuration Comparison

| Aspect | Debug | Release | Difference |
|--------|-------|---------|------------|
| **worldserver.exe size** | 76 MB | 25 MB | 3x smaller in Release |
| **worldserver.pdb size** | 488 MB | 50 MB | ~10x smaller in Release |
| **playerbot.lib size** | 512 MB | 180 MB | ~3x smaller in Release |
| **Build time (full)** | 20-40 min | 15-30 min | 25% faster in Release |
| **Runtime performance** | Baseline | 20-50% faster | Significant improvement |
| **Memory usage** | Higher | Lower | 20-40% less in Release |
| **Debugging capability** | Full | Limited | Trade-off for performance |

## Prerequisites

### Required Software
1. **Visual Studio 2022 Enterprise**
   - C++ Desktop Development workload
   - Windows 10 SDK
   - CMake integration tools

2. **CMake 3.24+**
   - Included with Visual Studio 2022

3. **vcpkg Package Manager**
   - Location: `C:\libs\vcpkg`
   - Toolchain file: `C:\libs\vcpkg\scripts\buildsystems\vcpkg.cmake`

4. **MySQL 9.4**
   - Required for database connectivity

### Required Libraries

#### vcpkg Libraries

Install required libraries using vcpkg:

```bash
# Navigate to vcpkg directory
cd C:\libs\vcpkg

# Install libraries for x64-windows (includes both release and debug)
vcpkg.exe install tbb:x64-windows
vcpkg.exe install parallel-hashmap:x64-windows

# Verify installation
vcpkg.exe list
```

**Installed Library Structure:**
```
C:\libs\vcpkg\installed\x64-windows\
├── lib\                          # Release libraries
│   ├── tbb12.lib
│   ├── tbbmalloc.lib
│   └── tbbmalloc_proxy.lib
├── debug\
│   └── lib\                      # Debug libraries
│       ├── tbb12_debug.lib
│       ├── tbbmalloc_debug.lib
│       └── tbbmalloc_proxy_debug.lib
├── include\                      # Headers (shared)
└── bin\                          # DLLs (release and debug)
```

#### Boost 1.78 Libraries

**IMPORTANT:** TrinityCore uses a custom FindSystemBoost.cmake that automatically selects the correct Boost libraries based on build configuration.

**Location:** `C:\libs\boost_1_78_0`

**Library Structure:**
```
C:\libs\boost_1_78_0\
├── boost\                        # Header files
├── stage\
│   └── lib\                      # All compiled libraries
│       ├── libboost_*-vc143-mt-gd-x64-1_78.lib    # Debug libraries (with -gd-)
│       └── libboost_*-vc143-mt-x64-1_78.lib        # Release libraries (no -gd-)
```

**Key Libraries Used:**
- `libboost_system` - System utilities
- `libboost_thread` - Threading support
- `libboost_filesystem` - File system operations
- `libboost_program_options` - Command-line parsing
- `libboost_regex` - Regular expressions
- `libboost_locale` - Localization

**Library Selection Logic (cmake/FindSystemBoost.cmake):**
```cmake
# Automatic configuration detection
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(BOOST_LIB_SUFFIX "-vc143-mt-gd-x64-1_78")  # Debug suffix with -gd-
else()
    set(BOOST_LIB_SUFFIX "-vc143-mt-x64-1_78")     # Release suffix without -gd-
endif()
```

## Build Scripts

### Configuration Scripts

#### `configure_debug.bat`
**Purpose**: Configure CMake for Debug build

**What it does:**
1. Initializes Visual Studio 2022 environment
2. Cleans and recreates `build/` directory
3. Runs CMake with Debug configuration
4. Points to correct vcpkg installation (`C:\libs\vcpkg`)
5. Enables Playerbot module (`BUILD_PLAYERBOT=1`)

**Usage:**
```batch
configure_debug.bat
```

**CMake Parameters:**
- `-G "Visual Studio 17 2022"` - Generate VS2022 solution
- `-A x64` - Target x64 platform
- `-DCMAKE_BUILD_TYPE=Debug` - Debug configuration
- `-DCMAKE_TOOLCHAIN_FILE="C:/libs/vcpkg/scripts/buildsystems/vcpkg.cmake"` - vcpkg integration
- `-DVCPKG_TARGET_TRIPLET=x64-windows` - Target triplet
- `-DBUILD_PLAYERBOT=1` - Enable Playerbot module

**Output:**
- `build/TrinityCore.sln` - Visual Studio solution
- `build/CMakeCache.txt` - CMake configuration cache
- Various `.vcxproj` files for each project

---

#### `configure_release.bat`
**Purpose**: Configure CMake for Release build

**What it does:**
1. Initializes Visual Studio 2022 environment
2. Cleans and recreates `build/` directory
3. Runs CMake with Release configuration
4. Points to correct vcpkg installation
5. Enables Playerbot module

**Usage:**
```batch
configure_release.bat
```

**CMake Parameters:** (Same as debug, except `-DCMAKE_BUILD_TYPE=Release`)

---

### Build Scripts

#### `build_debug.bat`
**Purpose**: Build entire TrinityCore solution in Debug configuration

**What it does:**
1. Verifies build directory exists (requires prior configuration)
2. Builds entire solution using MSBuild
3. Uses debug libraries from vcpkg automatically
4. Generates full debugging symbols (PDB files)

**Usage:**
```batch
# First time or after configuration changes:
configure_debug.bat
build_debug.bat

# Incremental builds:
build_debug.bat
```

**Build Parameters:**
- `/p:Configuration=Debug` - Debug configuration
- `/p:Platform=x64` - x64 platform
- `/maxcpucount:2` - Use 2 CPU cores (adjust based on system)
- `/verbosity:minimal` - Minimal console output
- `/fileLogger` - Enable file logging
- Log output: `build_debug_output.txt`

**Output:**
- `build\src\server\worldserver\Debug\worldserver.exe`
- `build\src\server\worldserver\Debug\worldserver.pdb` (debug symbols)
- `build\src\server\bnetserver\Debug\bnetserver.exe`
- `build\src\server\bnetserver\Debug\bnetserver.pdb`
- All module libraries in their respective Debug folders

**Build Time:** 20-40 minutes (full build)

---

#### `build_release.bat`
**Purpose**: Build entire TrinityCore solution in Release configuration

**What it does:**
1. Verifies build directory exists
2. Builds entire solution with optimizations
3. Uses release libraries from vcpkg automatically
4. Generates optimized binaries

**Usage:**
```batch
# First time or after configuration changes:
configure_release.bat
build_release.bat

# Incremental builds:
build_release.bat
```

**Output:**
- `build\src\server\worldserver\Release\worldserver.exe`
- `build\src\server\bnetserver\Release\bnetserver.exe`
- All module libraries in their respective Release folders

**Build Time:** 15-30 minutes (full build)

---

#### `build_playerbot_debug.bat`
**Purpose**: Fast incremental build of ONLY Playerbot module (Debug)

**What it does:**
1. Builds only the Playerbot module project
2. Skips building worldserver and other components
3. Useful for rapid development iterations

**Usage:**
```batch
# After making changes to Playerbot code:
build_playerbot_debug.bat
```

**Note:** After building the module, you must build worldserver to link the changes:

```batch
# Option 1: Build worldserver only
"C:\Program Files\Microsoft Visual Studio\2022\Enterprise\MSBuild\Current\Bin\MSBuild.exe" ^
    "build\src\server\worldserver\worldserver.vcxproj" ^
    /p:Configuration=Debug /p:Platform=x64

# Option 2: Full rebuild
build_debug.bat
```

**Output:**
- `build\src\server\modules\Playerbot\Debug\playerbot.lib`

**Build Time:** 2-5 minutes (incremental)

---

#### `build_playerbot_and_worldserver_debug.bat`
**Purpose**: Fast incremental build of Playerbot AND worldserver (Debug)

**What it does:**
1. Builds the Playerbot module first
2. Automatically rebuilds worldserver to link the changes
3. Optimal for Playerbot development workflow

**Usage:**
```batch
# After making changes to Playerbot code:
build_playerbot_and_worldserver_debug.bat
```

**Output:**
- `build\src\server\modules\Playerbot\Debug\playerbot.lib`
- `build\bin\Debug\worldserver.exe`

**Build Time:** 5-10 minutes (incremental)

---

#### `build_playerbot_and_worldserver_release.bat`
**Purpose**: Fast incremental build of Playerbot AND worldserver (Release)

**What it does:**
1. Builds the Playerbot module in Release mode
2. Automatically rebuilds worldserver with optimizations
3. For testing release performance

**Usage:**
```batch
# For performance testing:
build_playerbot_and_worldserver_release.bat
```

**Output:**
- `build\src\server\modules\Playerbot\Release\playerbot.lib`
- `build\bin\Release\worldserver.exe`

**Build Time:** 5-8 minutes (incremental)

---

## Typical Workflows

### Initial Setup (First Time)

```batch
# 1. Install vcpkg libraries
cd C:\libs\vcpkg
vcpkg.exe install tbb:x64-windows parallel-hashmap:x64-windows

# 2. Configure for Debug
cd C:\TrinityBots\TrinityCore
configure_debug.bat

# 3. Build entire project
build_debug.bat
```

### Daily Development (Playerbot Module)

```batch
# 1. Edit code in src/modules/Playerbot/

# 2. Quick module rebuild
build_playerbot_debug.bat

# 3. Rebuild worldserver to link changes
"C:\Program Files\Microsoft Visual Studio\2022\Enterprise\MSBuild\Current\Bin\MSBuild.exe" ^
    "build\src\server\worldserver\worldserver.vcxproj" ^
    /p:Configuration=Debug /p:Platform=x64
```

### Switching Between Debug and Release

```batch
# Switch to Debug
configure_debug.bat
build_debug.bat

# Switch to Release
configure_release.bat
build_release.bat
```

**Note:** Each configuration requires reconfiguration because CMake generates different project files.

### Full Clean Rebuild

```batch
# Delete build directory
rmdir /s /q build

# Reconfigure
configure_debug.bat

# Rebuild
build_debug.bat
```

---

## Debugging

### Visual Studio Debugging

1. **Open Solution:**
   ```batch
   start build\TrinityCore.sln
   ```

2. **Set Startup Project:**
   - Right-click `worldserver` in Solution Explorer
   - Select "Set as Startup Project"

3. **Configure Command Arguments:**
   - Right-click `worldserver` → Properties
   - Configuration Properties → Debugging
   - Command Arguments: `-c worldserver.conf`
   - Working Directory: `$(OutDir)` or absolute path to server directory

4. **Start Debugging:**
   - Press `F5` to start with debugging
   - Press `Ctrl+F5` to start without debugging

### WinDbg Debugging

```batch
# Launch with WinDbg
windbg.exe -o build\src\server\worldserver\Debug\worldserver.exe -c worldserver.conf

# WinDbg commands for debugging
# Break on exception:
sxe av

# Load symbols:
.sympath+ C:\TrinityBots\TrinityCore\build\src\server\worldserver\Debug

# Analyze crash dump:
!analyze -v
```

---

## Library Linking (Debug vs Release)

### How vcpkg Integration Works

When you use `CMAKE_TOOLCHAIN_FILE`, vcpkg automatically:

1. **Debug Configuration:**
   - Links to `C:\libs\vcpkg\installed\x64-windows\debug\lib\*.lib`
   - Copies debug DLLs to output directory
   - Uses debug version of libraries (e.g., `tbb12_debug.lib`)

2. **Release Configuration:**
   - Links to `C:\libs\vcpkg\installed\x64-windows\lib\*.lib`
   - Copies release DLLs to output directory
   - Uses release version of libraries (e.g., `tbb12.lib`)

### Verifying Library Linkage

Check CMake configuration:

```batch
# View CMake cache
type build\CMakeCache.txt | findstr /I "vcpkg"

# Expected output:
# CMAKE_TOOLCHAIN_FILE:FILEPATH=C:/libs/vcpkg/scripts/buildsystems/vcpkg.cmake
# VCPKG_TARGET_TRIPLET:STRING=x64-windows
```

Check library paths in Visual Studio project:

```batch
# View library directories in project file
type build\src\server\worldserver\worldserver.vcxproj | findstr /I "AdditionalLibraryDirectories"
```

---

## Troubleshooting

### Linking Errors (LNK2019, LNK2001)

**Problem:** Undefined symbols or missing library references

**Solution:**
1. Verify vcpkg libraries are installed:
   ```batch
   C:\libs\vcpkg\vcpkg.exe list
   ```

2. Check debug libraries exist:
   ```batch
   dir C:\libs\vcpkg\installed\x64-windows\debug\lib
   ```

3. Reconfigure with correct vcpkg path:
   ```batch
   configure_debug.bat
   ```

### Boost Library Conflicts (LNK2038)

**Problem:** Runtime library mismatch error:
```
error LNK2038: mismatch detected for 'RuntimeLibrary':
value 'MD_DynamicRelease' doesn't match value 'MDd_DynamicDebug'
```

**Root Cause:** Mixing Debug and Release Boost libraries

**Solution:**
1. Verify correct Boost libraries are being used:
   ```batch
   # Check CMake cache for Boost libraries
   type build\CMakeCache.txt | findstr "Boost_.*_LIBRARY"

   # Debug should have -gd- in the library names
   # Release should NOT have -gd- in the library names
   ```

2. Check FindSystemBoost.cmake is present:
   ```batch
   dir cmake\FindSystemBoost.cmake
   ```

3. Clean and reconfigure:
   ```batch
   rmdir /s /q build
   configure_debug.bat  # or configure_release.bat
   ```

4. Verify Boost library directory has both Debug and Release libraries:
   ```batch
   # Should see both types of libraries
   dir C:\libs\boost_1_78_0\stage\lib\libboost_system*.lib

   # Expected output:
   # libboost_system-vc143-mt-gd-x64-1_78.lib    (Debug)
   # libboost_system-vc143-mt-x64-1_78.lib        (Release)
   ```

### Wrong Library Version Linked

**Problem:** Debug build tries to link release libraries (or vice versa)

**Solution:**
1. Clean build directory:
   ```batch
   rmdir /s /q build
   ```

2. Reconfigure:
   ```batch
   configure_debug.bat
   ```

3. Rebuild:
   ```batch
   build_debug.bat
   ```

### CMake Configuration Fails

**Problem:** CMake cannot find vcpkg or libraries

**Solution:**
1. Verify vcpkg path:
   ```batch
   dir C:\libs\vcpkg\vcpkg.exe
   ```

2. Verify libraries installed:
   ```batch
   C:\libs\vcpkg\vcpkg.exe list
   ```

3. Install missing libraries:
   ```batch
   C:\libs\vcpkg\vcpkg.exe install tbb:x64-windows parallel-hashmap:x64-windows
   ```

### Build Hangs or Freezes

**Problem:** MSBuild appears to hang during compilation

**Solution:**
1. Reduce CPU cores used:
   ```batch
   # Edit build script and change /maxcpucount:2 to /maxcpucount:1
   ```

2. Check for antivirus interference:
   - Add build directory to antivirus exclusions

3. Check for disk space:
   - Debug builds require ~15-20 GB free space

### Out of Memory Errors

**Problem:** Compiler runs out of memory (C1060, C1076)

**Solution:**
1. Reduce parallel build jobs:
   ```batch
   # Edit build script: /maxcpucount:1
   ```

2. Close other applications

3. Increase virtual memory (page file size)

---

## Performance Considerations

### Build Times

| Configuration | Full Build | Incremental | Playerbot Only |
|--------------|------------|-------------|----------------|
| Debug        | 20-40 min  | 2-10 min    | 2-5 min        |
| Release      | 15-30 min  | 2-8 min     | 2-5 min        |

### Disk Space Requirements

| Configuration | Build Directory | Total Size |
|--------------|----------------|------------|
| Debug        | ~10-12 GB      | ~15-18 GB  |
| Release      | ~4-6 GB        | ~8-10 GB   |

### Memory Requirements

- **Minimum:** 8 GB RAM
- **Recommended:** 16 GB RAM
- **Optimal:** 32 GB RAM (for parallel builds)

---

## Advanced Usage

### Building Specific Projects

```batch
# Build only worldserver (Debug)
"C:\Program Files\Microsoft Visual Studio\2022\Enterprise\MSBuild\Current\Bin\MSBuild.exe" ^
    "build\src\server\worldserver\worldserver.vcxproj" ^
    /p:Configuration=Debug /p:Platform=x64

# Build only Playerbot module (Debug)
"C:\Program Files\Microsoft Visual Studio\2022\Enterprise\MSBuild\Current\Bin\MSBuild.exe" ^
    "build\src\server\modules\Playerbot\playerbot.vcxproj" ^
    /p:Configuration=Debug /p:Platform=x64

# Build only game library (Debug)
"C:\Program Files\Microsoft Visual Studio\2022\Enterprise\MSBuild\Current\Bin\MSBuild.exe" ^
    "build\src\server\game\game.vcxproj" ^
    /p:Configuration=Debug /p:Platform=x64
```

### Parallel Builds with Higher CPU Count

```batch
# Use all CPU cores (faster but more memory intensive)
"C:\Program Files\Microsoft Visual Studio\2022\Enterprise\MSBuild\Current\Bin\MSBuild.exe" ^
    "build\TrinityCore.sln" ^
    /p:Configuration=Debug /p:Platform=x64 ^
    /maxcpucount ^
    /verbosity:minimal
```

### Clean Build (Delete Intermediate Files)

```batch
# Clean Debug build
"C:\Program Files\Microsoft Visual Studio\2022\Enterprise\MSBuild\Current\Bin\MSBuild.exe" ^
    "build\TrinityCore.sln" ^
    /t:Clean /p:Configuration=Debug /p:Platform=x64

# Clean Release build
"C:\Program Files\Microsoft Visual Studio\2022\Enterprise\MSBuild\Current\Bin\MSBuild.exe" ^
    "build\TrinityCore.sln" ^
    /t:Clean /p:Configuration=Release /p:Platform=x64
```

---

## File Structure Reference

```
C:\TrinityBots\TrinityCore\
├── configure_debug.bat              # Configure CMake for Debug
├── configure_release.bat            # Configure CMake for Release
├── build_debug.bat                  # Build full solution (Debug)
├── build_release.bat                # Build full solution (Release)
├── build_playerbot_debug.bat        # Build Playerbot only (Debug)
├── BUILD_DOCUMENTATION.md           # This file
├── build/                           # CMake build directory (generated)
│   ├── TrinityCore.sln             # Visual Studio solution
│   ├── CMakeCache.txt              # CMake configuration cache
│   ├── src/
│   │   └── server/
│   │       ├── worldserver/
│   │       │   ├── Debug/          # Debug binaries
│   │       │   └── Release/        # Release binaries
│   │       ├── bnetserver/
│   │       │   ├── Debug/
│   │       │   └── Release/
│   │       └── modules/
│   │           └── Playerbot/
│   │               ├── Debug/
│   │               └── Release/
├── src/
│   └── modules/
│       └── Playerbot/              # Playerbot source code
└── build_debug_output.txt          # Build log (generated)
```

---

## Quick Reference

### Configure Commands
```batch
configure_debug.bat      # Configure for Debug
configure_release.bat    # Configure for Release
```

### Build Commands
```batch
build_debug.bat          # Build all (Debug)
build_release.bat        # Build all (Release)
build_playerbot_debug.bat # Build Playerbot only (Debug)
```

### vcpkg Commands
```batch
# Install libraries
C:\libs\vcpkg\vcpkg.exe install tbb:x64-windows
C:\libs\vcpkg\vcpkg.exe install parallel-hashmap:x64-windows

# List installed libraries
C:\libs\vcpkg\vcpkg.exe list

# Update libraries
C:\libs\vcpkg\vcpkg.exe upgrade --no-dry-run

# Remove library
C:\libs\vcpkg\vcpkg.exe remove tbb:x64-windows
```

### CMake Commands (Manual)
```batch
# Configure Debug
cmake .. -G "Visual Studio 17 2022" -A x64 ^
    -DCMAKE_BUILD_TYPE=Debug ^
    -DCMAKE_TOOLCHAIN_FILE="C:/libs/vcpkg/scripts/buildsystems/vcpkg.cmake" ^
    -DVCPKG_TARGET_TRIPLET=x64-windows ^
    -DBUILD_PLAYERBOT=1

# Configure Release
cmake .. -G "Visual Studio 17 2022" -A x64 ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DCMAKE_TOOLCHAIN_FILE="C:/libs/vcpkg/scripts/buildsystems/vcpkg.cmake" ^
    -DVCPKG_TARGET_TRIPLET=x64-windows ^
    -DBUILD_PLAYERBOT=1
```

---

## Support

For build issues:
1. Check `build_debug_output.txt` or `build_release_output.txt`
2. Verify vcpkg libraries are installed correctly
3. Ensure Visual Studio 2022 Enterprise is properly installed
4. Check disk space and memory availability

For TrinityCore-specific issues:
- TrinityCore Documentation: https://trinitycore.info/
- TrinityCore GitHub: https://github.com/TrinityCore/TrinityCore
