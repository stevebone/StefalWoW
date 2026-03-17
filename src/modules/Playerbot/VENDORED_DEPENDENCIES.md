# Playerbot Vendored Dependencies Guide

## Overview

Playerbot includes vendored dependencies to reduce installation friction for testers and users. This document provides complete guidance on using, managing, and understanding vendored dependencies.

## Why Vendor Dependencies?

### Problem Statement
Many testers reported difficulty installing `tbb` and `phmap`:
- Windows: vcpkg installation issues
- Linux: Package manager version mismatches
- macOS: Homebrew conflicts
- Cross-platform: Inconsistent versions across systems

### Solution: Vendored Dependencies
By including dependencies as git submodules, we achieve:

1. **Zero Installation Friction**
   - Clone repository → Initialize submodules → Build
   - No system-wide package installation required
   - No version conflicts with other projects

2. **Version Consistency**
   - All users test with identical dependency versions
   - Reproducible builds across all platforms
   - No "works on my machine" issues

3. **Self-Contained Module**
   - Playerbot remains isolated in `src/modules/Playerbot/`
   - No system pollution or conflicts
   - Easy to enable/disable via CMake flag

4. **Cross-Platform Reliability**
   - Tested configurations work identically on Windows, Linux, macOS
   - No platform-specific installation procedures
   - Consistent build behavior everywhere

## Quick Start

### For New Users

```bash
# Clone TrinityCore
git clone https://github.com/TrinityCore/TrinityCore.git
cd TrinityCore

# Initialize ALL submodules (including Playerbot vendored deps)
git submodule update --init --recursive

# Build normally
cmake -B build -S . -DBUILD_PLAYERBOT=ON
cmake --build build --config Release
```

That's it! No additional installation steps required.

### For Existing Users

If you already have TrinityCore cloned:

```bash
cd TrinityCore

# Initialize Playerbot vendored dependencies
git submodule update --init --recursive

# Rebuild
cmake -B build -S . -DBUILD_PLAYERBOT=ON
cmake --build build --config Release
```

## Architecture

### Directory Structure

```
src/modules/Playerbot/
├── deps/                           # Vendored dependencies
│   ├── README.md                   # Overview of vendored deps
│   ├── LICENSES.md                 # License information and compliance
│   ├── phmap/                      # Git submodule → parallel-hashmap
│   │   ├── parallel_hashmap/
│   │   │   └── phmap.h             # Header-only library
│   │   └── LICENSE
│   └── tbb/                        # Git submodule → oneTBB
│       ├── include/
│       │   └── tbb/
│       ├── src/
│       ├── CMakeLists.txt          # Built from source during build
│       └── LICENSE
│
├── cmake/                          # Custom CMake modules
│   └── FindPhmap.cmake             # Phmap detection module
│
└── CMakeLists.txt                  # Uses vendored deps automatically
```

### Dependency Detection Flow

CMake automatically prioritizes vendored dependencies:

```
1. Check for vendored dep in deps/
   ├── Found: Use vendored (phmap header-only, TBB builds from source)
   └── Not found: Try system-installed packages
       ├── Found: Use system
       └── Not found: Error with installation instructions
```

### Build Process

#### phmap (Header-Only)
- **Source:** `deps/phmap/` git submodule
- **Build:** None required (header-only)
- **Integration:** CMake adds include directory
- **Overhead:** Zero build time

#### TBB (Compiled Library)
- **Source:** `deps/tbb/` git submodule
- **Build:** Compiled from source via `add_subdirectory()`
- **Integration:** Links `TBB::tbb` target
- **Overhead:** +2-5 minutes initial build (cached afterward)

## CMake Integration Details

### Automatic Detection

`cmake/modules/FindPlayerbotDependencies.cmake` handles detection:

```cmake
# Priority 1: Vendored deps
set(VENDORED_PHMAP_DIR "${CMAKE_SOURCE_DIR}/src/modules/Playerbot/deps/phmap")
if(EXISTS "${VENDORED_PHMAP_DIR}/parallel_hashmap/phmap.h")
    # Use vendored phmap
endif()

# Priority 2: System packages
find_package(phmap)

# Error if neither found
```

### Build Targets

Vendored dependencies create standard CMake targets:

```cmake
# phmap target (header-only)
target_link_libraries(playerbot PRIVATE phmap::phmap)

# TBB target (compiled library)
target_link_libraries(playerbot PRIVATE TBB::tbb)
```

Code remains identical whether using vendored or system dependencies.

## Managing Vendored Dependencies

### Viewing Current Versions

```bash
# Check phmap version
cd src/modules/Playerbot/deps/phmap
git log -1 --oneline

# Check TBB version
cd ../tbb
git log -1 --oneline
cd ../../../..
```

### Updating to Latest

```bash
# Update phmap
cd src/modules/Playerbot/deps/phmap
git checkout master
git pull origin master
cd ../../../..
git add src/modules/Playerbot/deps/phmap
git commit -m "chore(deps): Update phmap to latest"

# Update TBB
cd src/modules/Playerbot/deps/tbb
git checkout master
git pull origin master
cd ../../../..
git add src/modules/Playerbot/deps/tbb
git commit -m "chore(deps): Update TBB to latest"
```

### Pinning Stable Versions (Recommended)

For production releases:

```bash
# Pin phmap to v1.3.12
cd src/modules/Playerbot/deps/phmap
git checkout tags/v1.3.12
cd ../../../..
git add src/modules/Playerbot/deps/phmap
git commit -m "chore(deps): Pin phmap to v1.3.12"

# Pin TBB to v2021.11.0
cd src/modules/Playerbot/deps/tbb
git checkout tags/v2021.11.0
cd ../../../..
git add src/modules/Playerbot/deps/tbb
git commit -m "chore(deps): Pin TBB to v2021.11.0"
```

### Removing Vendored Dependencies

To use system packages instead:

```bash
# Option 1: Remove submodules entirely
git rm -r src/modules/Playerbot/deps/phmap
git rm -r src/modules/Playerbot/deps/tbb
git commit -m "chore: Remove vendored dependencies, use system packages"

# Option 2: Don't initialize submodules
# CMake will automatically fall back to system packages
```

## Fallback to System Packages

CMake provides flexibility for different deployment scenarios:

### Development Build (Vendored)
```bash
git submodule update --init --recursive
cmake -B build -S . -DBUILD_PLAYERBOT=ON
```

### Production Build (System Packages)
```bash
# Install system packages
sudo apt-get install libtbb-dev
sudo apt-get install libphmap-dev  # or manual install

# Don't initialize submodules
cmake -B build -S . -DBUILD_PLAYERBOT=ON \
  -DPHMAP_ROOT=/usr/local \
  -DTBB_ROOT=/usr/local
```

### CI/CD Environment (Hybrid)
```bash
# Use vendored where possible, system for others
git submodule update --init src/modules/Playerbot/deps/phmap
# Let TBB use system packages

cmake -B build -S . -DBUILD_PLAYERBOT=ON
```

## License Compliance

All vendored dependencies use **Apache License 2.0**, which is:
- ✅ Compatible with GPL v2 (TrinityCore's license)
- ✅ Permits modification and redistribution
- ✅ Requires attribution (provided in `LICENSES.md`)

See `deps/LICENSES.md` for complete license information.

## Performance Considerations

### Build Time Impact

| Dependency | Type | First Build | Rebuild | Size |
|------------|------|-------------|---------|------|
| phmap | Header-only | 0 seconds | 0 seconds | ~200 KB |
| TBB | Compiled | ~2-5 minutes | <5 seconds | ~10 MB source |

**Total:** +2-5 minutes initial build, negligible rebuilds.

### Runtime Impact

**Zero.** Vendored dependencies produce identical binaries to system packages.

### Disk Space

- phmap: ~200 KB (headers only)
- TBB: ~10 MB source + ~5 MB build artifacts
- **Total:** ~15 MB additional disk usage

Negligible compared to TrinityCore's overall size (~1 GB+).

## Troubleshooting

### CMake Can't Find Vendored Dependencies

**Symptom:**
```
CMake Error: phmap headers not found
```

**Solution:**
```bash
# Ensure submodules are initialized
git submodule update --init --recursive

# Verify deps/ directory exists
ls src/modules/Playerbot/deps/

# Should show: phmap/ tbb/ README.md LICENSES.md
```

### TBB Build Errors

**Symptom:**
```
Error building TBB from deps/tbb/
```

**Solution 1:** Update TBB submodule
```bash
cd src/modules/Playerbot/deps/tbb
git checkout master
git pull
cd ../../../..
```

**Solution 2:** Use system TBB instead
```bash
# Install system TBB
sudo apt-get install libtbb-dev  # Linux
vcpkg install tbb:x64-windows    # Windows

# Remove vendored TBB submodule
git rm -r src/modules/Playerbot/deps/tbb
```

### Submodule Initialization Fails

**Symptom:**
```
fatal: could not access submodule 'deps/phmap'
```

**Solution:**
```bash
# Manually add submodules
cd src/modules/Playerbot
git submodule add https://github.com/greg7mdp/parallel-hashmap.git deps/phmap
git submodule add https://github.com/oneapi-src/oneTBB.git deps/tbb
```

### Mixed Vendored/System Dependencies

**Symptom:**
```
Warning: Using vendored phmap but system TBB
```

**Solution:** This is intentional! CMake uses vendored when available, falls back to system otherwise. No action needed.

## Best Practices

### For Developers

1. **Always initialize submodules** after `git clone`
2. **Pin stable versions** for production releases
3. **Test with vendored deps** before committing changes
4. **Update submodules** quarterly for security patches

### For CI/CD

1. **Use vendored deps** for reproducible builds
2. **Cache submodule checkouts** to speed up CI
3. **Validate submodule integrity** before building
4. **Test fallback to system packages** periodically

### For Package Maintainers

1. **Use system packages** for distribution packaging
2. **Remove vendored deps** from source packages
3. **Add build dependencies** to package metadata
4. **Document system package requirements** clearly

## FAQ

### Q: Why git submodules instead of direct inclusion?

**A:** Git submodules provide:
- Proper attribution and license tracking
- Easy updates via `git pull`
- Minimal repo size bloat
- Standard open-source practice

### Q: Can I use a specific version of phmap/TBB?

**A:** Yes! See "Pinning Stable Versions" section above.

### Q: What if I already have TBB/phmap installed?

**A:** CMake prioritizes vendored deps. To use system packages, don't initialize submodules or remove `deps/` directory.

### Q: Do vendored deps affect binary distribution?

**A:** No. Final binaries are identical whether using vendored or system deps.

### Q: How often should I update vendored deps?

**A:** Quarterly for security patches, or when upstream releases critical fixes.

### Q: Can I vendor other dependencies?

**A:** Yes, following the same pattern:
1. Add as git submodule to `deps/`
2. Update `FindPlayerbotDependencies.cmake`
3. Document in `LICENSES.md`

---

## Additional Resources

- **Git Submodules:** https://git-scm.com/book/en/v2/Git-Tools-Submodules
- **CMake find_package():** https://cmake.org/cmake/help/latest/command/find_package.html
- **phmap Repository:** https://github.com/greg7mdp/parallel-hashmap
- **TBB Repository:** https://github.com/oneapi-src/oneTBB
- **License Compliance:** See `deps/LICENSES.md`

---

**Maintainer:** TrinityCore Playerbot Team
**Last Updated:** 2025-10-29
**Version:** 1.0.0
