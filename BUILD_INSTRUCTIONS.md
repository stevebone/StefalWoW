# TrinityCore Build Instructions

This document describes the persistent build solution for TrinityCore with Playerbot module.

## Prerequisites

- Visual Studio 2022 Enterprise with C++ tooling
- CMake 3.24+
- Git
- vcpkg at `C:\libs\vcpkg`
- Boost libraries at `C:\libs\boost_1_88_0-bin-msvc-all-32-64`

## Build Configurations

We support four build configurations:

1. **Debug** - Full debug symbols, no optimizations, for development
2. **Release** - Full optimizations, no debug symbols, for production
3. **RelWithDebInfo** - Optimized with debug symbols, for profiling/debugging production issues
4. **Test** - Debug build with testing enabled

## Build Process

### Step 1: Configure

Run the appropriate configure script **once** or when CMake configuration needs to be regenerated:

```batch
# For Debug build
configure_debug.bat

# For Release build
configure_release.bat

# For RelWithDebInfo build
configure_relwithdebinfo.bat

# For Test build (with tests enabled)
configure_test.bat
```

**What this does:**
- Sets correct vcpkg path: `C:\libs\vcpkg`
- Sets Boost paths to the correct library directories
- Creates/cleans the `build` directory
- Runs CMake with the appropriate configuration
- Generates Visual Studio 2022 solution files

**When to reconfigure:**
- After pulling new code that changes CMakeLists.txt
- When switching between configurations
- When adding/removing build options
- When CMake cache becomes corrupted

### Step 2: Build

After configuration, run the corresponding build script:

```batch
# For Debug build
build_debug.bat

# For Release build
build_release.bat

# For RelWithDebInfo build
build_relwithdebinfo.bat

# For Test build and run tests
build_test.bat
```

**What this does:**
- Verifies build directory and CMake cache exist
- Runs MSBuild with appropriate configuration
- Uses 2 CPU cores for parallel compilation
- Outputs minimal verbosity to reduce noise
- 30-minute timeout for large builds

## Build Outputs

After successful build, binaries are located in:

- Debug: `build\bin\Debug\`
- Release: `build\bin\Release\`
- RelWithDebInfo: `build\bin\RelWithDebInfo\`

Main executables:
- `worldserver.exe` - Main game server with Playerbot module
- `bnetserver.exe` - Authentication server

## Switching Configurations

To switch from one configuration to another:

1. Run the new configuration's configure script
   - Example: `configure_release.bat`
2. Run the new configuration's build script
   - Example: `build_release.bat`

The configure script will clean the CMake cache, so you don't need to manually delete the build directory.

## Troubleshooting

### CMake Configuration Fails

**Problem:** vcpkg or Boost not found

**Solution:** Verify paths in configure scripts:
- vcpkg should be at `C:\libs\vcpkg`
- Boost should be at `C:\libs\boost_1_88_0-bin-msvc-all-32-64`

### Build Fails with Missing MSBuild

**Problem:** MSBuild.exe not found

**Solution:** Verify Visual Studio 2022 Enterprise installation path matches:
```
C:\Program Files\Microsoft Visual Studio\2022\Enterprise\MSBuild\Current\Bin\MSBuild.exe
```

### Build Timeout

**Problem:** Build takes longer than 30 minutes

**Solution:** Increase timeout in build scripts by modifying the script header comment (informational only - actual timeout is system-dependent)

### Incremental Build Issues

**Problem:** Strange build errors after code changes

**Solution:**
1. Clean the solution: Run configure script again
2. Delete `build` directory entirely
3. Run configure script
4. Run build script

## Performance Tips

- Use **Release** for production/performance testing
- Use **Debug** for development and debugging
- Use **RelWithDebInfo** when you need performance with debugging capability
- Build scripts use `/m:2` to limit parallel jobs to 2 cores (adjustable in build scripts)

## Git Integration

These build scripts are designed to persist across git operations:

- Safe with `git pull`, `git merge`, `git rebase`
- Safe with `git checkout` (switching branches)
- Not tracked in git (should be .gitignore'd if needed)
- Stored at repository root for easy access

## Build Customization

To customize builds, edit the respective configure script:

**Example: Add custom CMake options**

```batch
cmake .. -G "Visual Studio 17 2022" ^
    -DCMAKE_BUILD_TYPE=Debug ^
    -DBUILD_PLAYERBOT=1 ^
    -DYOUR_CUSTOM_OPTION=ON ^
    -DCMAKE_TOOLCHAIN_FILE="%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake" ^
    ...
```

**Example: Change MSBuild parallelism**

In build scripts, change `/m:2` to `/m:4` for 4 cores:

```batch
%MSBUILD% TrinityCore.sln ^
    /p:Configuration=Debug ^
    /p:Platform=x64 ^
    /m:4 ^
    ...
```

## Quick Reference

| Task | Command |
|------|---------|
| First-time Debug build | `configure_debug.bat` then `build_debug.bat` |
| Rebuild Debug | `build_debug.bat` |
| Switch to Release | `configure_release.bat` then `build_release.bat` |
| Run tests | `configure_test.bat` then `build_test.bat` |
| Clean rebuild | `configure_*.bat` then `build_*.bat` |

## Notes

- Configure scripts will **not** delete your entire build directory, only CMakeCache.txt
- Build scripts include error checking and will stop on failure
- All scripts use `pause` at the end so you can see the results
- Timeout is set to 30 minutes as specified in requirements
