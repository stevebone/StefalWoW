# Boost Setup Guide for TrinityCore Playerbot

This guide explains how to set up Boost for compiling TrinityCore with the Playerbot module enabled.

## Quick Start

The build system automatically detects Boost in multiple ways:
1. ✅ **Environment Variable** (Recommended): Set `BOOST_ROOT` to your Boost installation
2. ✅ **Common Paths**: Checks standard installation locations automatically
3. ✅ **Standard CMake**: Falls back to system-installed Boost if needed

## Option 1: Using BOOST_ROOT Environment Variable (Recommended)

### Windows

1. **Download Boost 1.74+** (1.89 recommended for best compatibility)
   - Official prebuilt binaries: https://sourceforge.net/projects/boost/files/boost-binaries/
   - Choose the version matching your Visual Studio (e.g., `boost_1_89_0-msvc-14.3-64.exe` for VS2022)

2. **Install Boost** to a directory of your choice, e.g.:
   - `C:\local\boost_1_89_0`
   - `C:\libs\boost_1_89_0`
   - `C:\Program Files\boost\boost_1_89_0`

3. **Set BOOST_ROOT Environment Variable**:
   ```powershell
   # Temporary (current session only)
   $env:BOOST_ROOT = "C:\local\boost_1_89_0"

   # Permanent (system-wide)
   [System.Environment]::SetEnvironmentVariable("BOOST_ROOT", "C:\local\boost_1_89_0", "User")
   ```

   Or via GUI:
   - Press `Win+R`, type `sysdm.cpl`, press Enter
   - Go to **Advanced** tab → **Environment Variables**
   - Under **User variables**, click **New**
   - Variable name: `BOOST_ROOT`
   - Variable value: `C:\local\boost_1_89_0` (your path)
   - Click **OK**

4. **Restart your terminal/IDE** to pick up the new environment variable

5. **Run CMake**:
   ```powershell
   mkdir build
   cd build
   cmake .. -DBUILD_PLAYERBOT=ON
   cmake --build . --config Release
   ```

### Linux/Unix

1. **Install Boost 1.74+ via package manager**:
   ```bash
   # Ubuntu/Debian
   sudo apt-get install libboost-all-dev

   # Fedora/RHEL
   sudo dnf install boost-devel

   # Arch Linux
   sudo pacman -S boost
   ```

2. **Or download from source**:
   ```bash
   wget https://boostorg.jfrog.io/artifactory/main/release/1.89.0/source/boost_1_89_0.tar.gz
   tar -xzf boost_1_89_0.tar.gz
   cd boost_1_89_0
   ./bootstrap.sh --prefix=/usr/local/boost_1_89_0
   ./b2 install
   ```

3. **Set BOOST_ROOT** (if using custom location):
   ```bash
   export BOOST_ROOT=/usr/local/boost_1_89_0
   # Add to ~/.bashrc or ~/.zshrc for persistence
   echo 'export BOOST_ROOT=/usr/local/boost_1_89_0' >> ~/.bashrc
   ```

4. **Run CMake**:
   ```bash
   mkdir build
   cd build
   cmake .. -DBUILD_PLAYERBOT=ON
   cmake --build . --config Release -j$(nproc)
   ```

## Option 2: Using Common Installation Paths (No BOOST_ROOT)

If you don't set `BOOST_ROOT`, the build system automatically searches these locations:

### Windows
- `C:/libs/boost_1_89_0-bin-msvc-all-32-64/boost_1_89_0`
- `C:/local/boost_1_89_0`
- `C:/Program Files/boost/boost_1_89_0`

### Linux/Unix
- `/usr/local/boost_1_89_0`
- `/opt/boost_1_89_0`
- System default paths (`/usr/include`, `/usr/lib`)

**Just install Boost to one of these locations and CMake will find it automatically.**

## Option 3: Using vcpkg (Windows)

1. **Install vcpkg**:
   ```powershell
   git clone https://github.com/microsoft/vcpkg.git
   cd vcpkg
   .\bootstrap-vcpkg.bat
   ```

2. **Install Boost**:
   ```powershell
   .\vcpkg install boost:x64-windows
   ```

3. **Run CMake with vcpkg toolchain**:
   ```powershell
   mkdir build
   cd build
   cmake .. -DBUILD_PLAYERBOT=ON -DCMAKE_TOOLCHAIN_FILE=C:/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake
   cmake --build . --config Release
   ```

## Troubleshooting

### Error: "System Boost 1.89 not found at specified path"

**Cause**: Your `BOOST_ROOT` environment variable points to a non-existent path.

**Solutions**:
1. Verify Boost is actually installed at that path
2. Check the path in `BOOST_ROOT` is correct (no typos)
3. Unset `BOOST_ROOT` to let CMake search automatically:
   ```powershell
   # Windows
   [System.Environment]::SetEnvironmentVariable("BOOST_ROOT", $null, "User")

   # Linux
   unset BOOST_ROOT
   ```
4. Restart terminal/IDE after changing environment variables

### Error: "Could not auto-detect Boost library directory"

**Cause**: Boost is installed but libraries are in an unexpected location.

**Solutions**:
1. Check your Boost installation has compiled libraries (not just headers)
2. Look for directories like:
   - `lib64-msvc-14.3` (MSVC)
   - `lib` (standard)
   - `stage/lib` (custom build)
3. If using custom build, run `./b2` to compile Boost libraries

### Error: "Boost not found. Please install Boost 1.74+"

**Cause**: No Boost installation detected anywhere.

**Solutions**:
1. Install Boost using one of the methods above
2. Set `BOOST_ROOT` environment variable
3. Verify Boost version is 1.74 or newer:
   ```bash
   # Check installed version
   cat $BOOST_ROOT/boost/version.hpp | grep "BOOST_VERSION"
   ```

### CMake keeps using wrong Boost version

**Solutions**:
1. Delete CMake cache: `rm -rf build/CMakeCache.txt build/CMakeFiles`
2. Set `BOOST_ROOT` to force specific installation
3. Reconfigure from clean build directory:
   ```bash
   rm -rf build
   mkdir build
   cd build
   cmake .. -DBUILD_PLAYERBOT=ON
   ```

## Verification

After successful CMake configuration, you should see:
```
-- Using system Boost 1.89 for Playerbot compatibility
-- Using BOOST_ROOT from environment: C:\local\boost_1_89_0
-- Attempting to use system Boost from: C:\local\boost_1_89_0
-- Detected Boost version: 1.89
-- ✅ Custom Boost found at: C:/local/boost_1_89_0
-- ✅ Boost Release libraries found: ...
-- ✅ Boost Debug libraries found: ...
```

Or for standard installation:
```
-- Using standard CMake Boost finding mechanism
-- ✅ Standard Boost 1.89.0 found at: /usr/include
```

## Technical Details

### How Boost Detection Works

When `BUILD_PLAYERBOT=ON`, the build system uses `cmake/FindSystemBoost.cmake`:

1. **Phase 1: Search for Boost**
   - Check `BOOST_ROOT` environment variable
   - Check common installation paths
   - Set `USE_STANDARD_BOOST_SEARCH` if not found

2. **Phase 2: Locate Libraries**
   - Auto-detect MSVC toolset version (e.g., 14.3)
   - Auto-detect architecture (x64/x32)
   - Auto-detect Boost version from `version.hpp`
   - Search for required libraries:
     - `boost_thread`
     - `boost_filesystem`
     - `boost_program_options`
     - `boost_regex`
     - `boost_locale`

3. **Phase 3: Fallback**
   - If custom search fails, use standard CMake `find_package(Boost)`
   - Requires Boost 1.74+ minimum
   - Uses system-installed packages

### Required Boost Components

The Playerbot module requires these Boost libraries:
- **filesystem** - File and directory operations
- **program_options** - Configuration parsing
- **regex** - Regular expressions
- **locale** - Internationalization
- **thread** - Threading support (header-only in most versions)

Note: `boost_system` is header-only since Boost 1.69 and is not required as a separate library.

## FAQ

**Q: Do I need to rebuild Boost from source?**
A: No, prebuilt binaries work fine for Windows. Linux users can use package managers.

**Q: What's the minimum Boost version?**
A: 1.74 minimum, 1.89 recommended for best compatibility.

**Q: Can I use Boost installed by vcpkg?**
A: Yes! When `BOOST_ROOT` is not set, CMake will find vcpkg-installed Boost automatically.

**Q: Why do I get "Debug libraries not found"?**
A: This is a warning, not an error. Release libraries will be used for both configurations. For full Debug support, install Boost debug binaries.

**Q: Do I need different Boost versions for Debug/Release builds?**
A: No. The same Boost installation works for both. The build system automatically selects appropriate libraries.

## Support

If you encounter issues not covered here:
1. Check CMake output for diagnostic messages
2. Verify your Boost installation is complete (headers + libraries)
3. Try deleting CMake cache and reconfiguring
4. Report build errors with full CMake output to project maintainers

---

**Last Updated**: 2025-11-03
**Applies to**: TrinityCore master branch with BUILD_PLAYERBOT=ON
