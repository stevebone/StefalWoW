# Playerbot Vendored Dependencies

This directory contains vendored dependencies for the Playerbot module to reduce installation friction for testers and users.

## Why Vendor Dependencies?

Playerbot is an optional module that should be **easy to test and deploy** without requiring complex system-wide package installation. Vendoring critical dependencies ensures:

1. **Zero installation friction** - Clone and build, no extra steps
2. **Version consistency** - All users test with same versions
3. **Cross-platform reliability** - Works consistently on Windows, Linux, macOS
4. **Self-contained module** - No system pollution or conflicts

## Included Libraries

### phmap (Parallel Hashmap)
- **Version:** Latest from master
- **License:** Apache 2.0
- **Type:** Header-only library
- **Purpose:** High-performance concurrent hash tables for bot data structures
- **Location:** `deps/phmap/` (git submodule)

### TBB (Threading Building Blocks)
- **Version:** oneTBB 2021.x
- **License:** Apache 2.0
- **Type:** Compiled library
- **Purpose:** Thread pools and concurrent containers for bot AI systems
- **Location:** `deps/tbb/` (git submodule)

## Directory Structure

```
deps/
├── README.md              # This file
├── LICENSES.md            # License information for all vendored libraries
├── phmap/                 # Git submodule → github.com/greg7mdp/parallel-hashmap
│   └── parallel_hashmap/
│       └── phmap.h        # Header-only library
└── tbb/                   # Git submodule → github.com/oneapi-src/oneTBB
    ├── include/
    │   └── tbb/
    └── src/               # Compiled from source during build
```

## Git Submodules

Both libraries are included as **git submodules** rather than directly copying code. This provides:

- Easy version updates via `git submodule update`
- Proper attribution and license tracking
- Minimal repo size bloat
- Standard open-source practice

## Initializing Submodules

When cloning TrinityCore with Playerbot:

```bash
# Clone with submodules
git clone --recurse-submodules https://github.com/TrinityCore/TrinityCore.git

# Or initialize after clone
git submodule update --init --recursive
```

## CMake Integration

The Playerbot CMakeLists.txt automatically detects and uses vendored dependencies:

```cmake
# Fallback logic (in order of preference):
1. Use vendored deps from deps/ if available
2. Use system-installed packages (find_package)
3. Error if neither found
```

This ensures both easy testing (vendored) and production flexibility (system packages).

## Updating Vendored Dependencies

To update a submodule to a newer version:

```bash
cd src/modules/Playerbot/deps/phmap
git checkout master
git pull origin master
cd ../../../..
git add src/modules/Playerbot/deps/phmap
git commit -m "chore(deps): Update phmap to latest version"
```

## Building from Vendored Dependencies

CMake automatically detects vendored dependencies:

```bash
cmake -B build -S . -DBUILD_PLAYERBOTS=ON
cmake --build build --config Release
```

No additional flags or environment variables required!

## License Compliance

All vendored dependencies are **Apache 2.0 licensed**, which is:
- ✅ Compatible with GPL (TrinityCore license)
- ✅ Permits modification and redistribution
- ✅ Requires attribution (provided in LICENSES.md)

See `LICENSES.md` for complete license texts.

## Fallback to System Packages

If you prefer system-installed packages (e.g., for packaging or system integration):

```bash
# Remove or don't initialize submodules
# CMake will automatically fall back to find_package()

cmake -B build -S . -DBUILD_PLAYERBOTS=ON \
  -DPHMAP_ROOT=/usr/local \
  -DTBB_ROOT=/usr/local
```

## Support

For issues with vendored dependencies:
1. Ensure submodules are initialized: `git submodule update --init --recursive`
2. Clean build directory: `rm -rf build && cmake -B build`
3. Check CMake output for dependency detection messages
4. Report issues with full CMake log

---

**Maintainer:** TrinityCore Playerbot Team
**Last Updated:** 2025-10-29
**Dependencies Version:** phmap (master), TBB 2021.11
