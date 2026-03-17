# TrinityCore Playerbot - Build Instructions

Complete guide for building TrinityCore with the Playerbot module on Windows, Linux, and macOS.

---

## Table of Contents

1. [Quick Start (Recommended)](#quick-start-recommended)
2. [System Requirements](#system-requirements)
3. [Dependency Installation](#dependency-installation)
4. [Building with Vendored Dependencies](#building-with-vendored-dependencies)
5. [Building with System Packages](#building-with-system-packages)
6. [Platform-Specific Instructions](#platform-specific-instructions)
7. [Troubleshooting](#troubleshooting)
8. [Advanced Build Options](#advanced-build-options)

---

## Quick Start (Recommended)

### Option 1: Vendored Dependencies (Zero Installation)

```bash
# Clone TrinityCore
git clone --recurse-submodules https://github.com/TrinityCore/TrinityCore.git
cd TrinityCore

# Build with Playerbot
cmake -B build -S . -DBUILD_PLAYERBOT=ON
cmake --build build --config Release -j$(nproc)
```

**That's it!** Vendored dependencies (`tbb` and `phmap`) are automatically detected and built.

---

## System Requirements

### Minimum Requirements

| Component | Requirement |
|-----------|-------------|
| **OS** | Windows 10+, Ubuntu 20.04+, macOS 11+ |
| **Compiler** | GCC 11+, Clang 14+, MSVC 19.30+ (VS2022) |
| **CMake** | 3.24.0 or later |
| **Memory** | 8 GB RAM (16 GB recommended for parallel builds) |
| **Disk Space** | 20 GB free space |

### Required Software

- **Git** 2.20+ (with submodule support)
- **MySQL/MariaDB** 8.0+ / 10.6+
- **Boost** 1.74.0+
- **OpenSSL** 1.1.0+

---

## Dependency Installation

### Core TrinityCore Dependencies

Follow the official TrinityCore installation guide for your platform:

**Official Guides:**
- Windows: https://trinitycore.info/en/install/windows
- Linux: https://trinitycore.info/en/install/linux
- macOS: https://trinitycore.info/en/install/macos

**Summary:**

```bash
# Ubuntu/Debian
sudo apt-get update
sudo apt-get install git cmake build-essential gcc g++ \
    libmysqlclient-dev libssl-dev libboost-all-dev libreadline-dev

# Fedora/RHEL
sudo dnf install git cmake gcc gcc-c++ make \
    mysql-devel openssl-devel boost-devel readline-devel

# macOS (Homebrew)
brew install git cmake boost openssl mysql readline

# Windows
# Install Visual Studio 2022 with C++ workload
# Install MySQL 8.0+ from https://dev.mysql.com/downloads/
# Install Boost via vcpkg: vcpkg install boost:x64-windows
```

### Playerbot-Specific Dependencies

**OPTION 1: Vendored (Recommended)**

No installation required! Initialize git submodules:

```bash
git submodule update --init --recursive
```

**OPTION 2: System Packages**

Install `tbb` and `phmap` system-wide:

```bash
# Ubuntu/Debian
sudo apt-get install libtbb-dev
# phmap not in apt - install manually or use vendored

# Fedora/RHEL
sudo dnf install tbb-devel
# phmap not in dnf - install manually or use vendored

# Windows (vcpkg)
vcpkg install tbb:x64-windows
vcpkg install parallel-hashmap:x64-windows

# macOS (Homebrew)
brew install tbb
# phmap not in homebrew - install manually or use vendored
```

---

## Building with Vendored Dependencies

### Step 1: Clone Repository

```bash
# Clone with submodules (includes vendored deps)
git clone --recurse-submodules https://github.com/TrinityCore/TrinityCore.git
cd TrinityCore

# Or if already cloned:
cd TrinityCore
git submodule update --init --recursive
```

### Step 2: Verify Vendored Dependencies

```bash
# Check submodules initialized
ls src/modules/Playerbot/deps/

# Should show:
# phmap/  tbb/  README.md  LICENSES.md
```

### Step 3: Configure Build

```bash
# Create build directory
cmake -B build -S . \
    -DBUILD_PLAYERBOT=ON \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/opt/trinitycore

# Windows (Visual Studio 2022)
cmake -B build -S . -G "Visual Studio 17 2022" -A x64 \
    -DBUILD_PLAYERBOT=ON \
    -DCMAKE_INSTALL_PREFIX=C:/TrinityCore
```

### Step 4: Build

```bash
# Linux/macOS
cmake --build build --config Release -j$(nproc)

# Windows
cmake --build build --config Release
```

### Step 5: Verify Vendored Deps Used

Check CMake output for:

```
-- Detecting Intel TBB...
-- ✅ Using vendored TBB from: /path/to/TrinityCore/src/modules/Playerbot/deps/tbb
-- Detecting Parallel Hashmap...
-- ✅ Using vendored phmap from: /path/to/TrinityCore/src/modules/Playerbot/deps/phmap
```

---

## Building with System Packages

### Step 1: Install System Dependencies

```bash
# Install tbb and phmap system-wide (see "Dependency Installation" above)
```

### Step 2: Clone Without Submodules

```bash
# Clone without initializing Playerbot deps
git clone https://github.com/TrinityCore/TrinityCore.git
cd TrinityCore

# Initialize only required TrinityCore submodules (not Playerbot deps)
git submodule update --init --recursive
# Then remove Playerbot submodules:
git submodule deinit src/modules/Playerbot/deps/phmap
git submodule deinit src/modules/Playerbot/deps/tbb
```

### Step 3: Configure with System Package Hints

```bash
cmake -B build -S . \
    -DBUILD_PLAYERBOT=ON \
    -DCMAKE_BUILD_TYPE=Release \
    -DTBB_ROOT=/usr \
    -DPHMAP_ROOT=/usr/local
```

### Step 4: Build

```bash
cmake --build build --config Release -j$(nproc)
```

---

## Platform-Specific Instructions

### Windows (Visual Studio 2022)

#### Prerequisites
- Visual Studio 2022 with "Desktop development with C++" workload
- CMake 3.24+ (included with VS2022 or standalone)
- Git for Windows with Git Bash
- MySQL 8.0+ Server

#### Build Steps

```powershell
# Open "Developer Command Prompt for VS 2022"

# Clone with submodules
git clone --recurse-submodules https://github.com/TrinityCore/TrinityCore.git
cd TrinityCore

# Configure (vendored deps)
cmake -B build -S . -G "Visual Studio 17 2022" -A x64 ^
    -DBUILD_PLAYERBOT=ON ^
    -DCMAKE_INSTALL_PREFIX=C:/TrinityCore

# Build
cmake --build build --config Release --parallel
```

#### Troubleshooting Windows

**Issue:** CMake can't find MySQL

**Solution:**
```powershell
cmake -B build -S . -G "Visual Studio 17 2022" -A x64 ^
    -DBUILD_PLAYERBOT=ON ^
    -DMYSQL_ROOT="C:/Program Files/MySQL/MySQL Server 8.0"
```

---

### Linux (Ubuntu 22.04 / Debian 12)

#### Prerequisites

```bash
sudo apt-get update
sudo apt-get install -y \
    git cmake make gcc g++ clang \
    libmysqlclient-dev libssl-dev libboost-all-dev \
    libreadline-dev libncurses-dev
```

#### Build Steps

```bash
# Clone with submodules
git clone --recurse-submodules https://github.com/TrinityCore/TrinityCore.git
cd TrinityCore

# Configure (vendored deps)
cmake -B build -S . \
    -DBUILD_PLAYERBOT=ON \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=$HOME/trinitycore

# Build (use all CPU cores)
cmake --build build --config Release -j$(nproc)

# Install
sudo cmake --install build
```

---

### macOS (13 Ventura / 14 Sonoma)

#### Prerequisites

```bash
# Install Xcode Command Line Tools
xcode-select --install

# Install Homebrew dependencies
brew install git cmake boost openssl mysql readline
```

#### Build Steps

```bash
# Clone with submodules
git clone --recurse-submodules https://github.com/TrinityCore/TrinityCore.git
cd TrinityCore

# Configure (vendored deps, with OpenSSL path)
cmake -B build -S . \
    -DBUILD_PLAYERBOT=ON \
    -DCMAKE_BUILD_TYPE=Release \
    -DOPENSSL_ROOT_DIR=$(brew --prefix openssl) \
    -DMYSQL_ROOT=$(brew --prefix mysql)

# Build
cmake --build build --config Release -j$(sysctl -n hw.ncpu)
```

---

## Troubleshooting

### Common Issues

#### CMake can't find vendored dependencies

**Symptom:**
```
CMake Error: phmap headers not found
```

**Solution:**
```bash
# Ensure submodules initialized
git submodule update --init --recursive

# Verify deps exist
ls src/modules/Playerbot/deps/
# Should show: phmap/ tbb/
```

---

#### TBB build fails

**Symptom:**
```
Error building TBB from deps/tbb/
```

**Solution 1:** Use system TBB instead
```bash
# Remove vendored TBB
git rm -r src/modules/Playerbot/deps/tbb

# Install system TBB
sudo apt-get install libtbb-dev  # Linux
brew install tbb                 # macOS
vcpkg install tbb:x64-windows    # Windows
```

**Solution 2:** Update TBB submodule
```bash
cd src/modules/Playerbot/deps/tbb
git checkout master
git pull
cd ../../../..
```

---

#### Boost not found

**Symptom:**
```
Could not find Boost
```

**Solution:**
```bash
# Linux
sudo apt-get install libboost-all-dev

# macOS
brew install boost

# Windows
vcpkg install boost:x64-windows
```

---

#### MySQL not found

**Symptom:**
```
Could not find MySQL
```

**Solution:**
```bash
# Linux
sudo apt-get install libmysqlclient-dev

# macOS
brew install mysql

# Windows - specify path manually
cmake -B build -S . -DMYSQL_ROOT="C:/Program Files/MySQL/MySQL Server 8.0"
```

---

## Advanced Build Options

### Custom Build Types

```bash
# Debug build (with symbols, no optimization)
cmake -B build-debug -S . -DCMAKE_BUILD_TYPE=Debug -DBUILD_PLAYERBOT=ON

# RelWithDebInfo (optimization + debug symbols)
cmake -B build-rel -S . -DCMAKE_BUILD_TYPE=RelWithDebInfo -DBUILD_PLAYERBOT=ON

# MinSizeRel (optimize for size)
cmake -B build-min -S . -DCMAKE_BUILD_TYPE=MinSizeRel -DBUILD_PLAYERBOT=ON
```

### Parallel Builds

```bash
# Use all CPU cores
cmake --build build -j$(nproc)  # Linux
cmake --build build -j$(sysctl -n hw.ncpu)  # macOS
cmake --build build --parallel  # Windows (auto-detects cores)
```

### Install to Custom Location

```bash
cmake -B build -S . \
    -DCMAKE_INSTALL_PREFIX=/custom/path \
    -DBUILD_PLAYERBOT=ON

cmake --install build
```

### Disable Playerbot

```bash
# Build TrinityCore without Playerbot
cmake -B build -S . -DBUILD_PLAYERBOT=OFF
```

---

## Vendored Dependency Management

See [VENDORED_DEPENDENCIES.md](VENDORED_DEPENDENCIES.md) for:
- Updating vendored dependencies
- Pinning stable versions
- Switching between vendored and system packages
- License information

---

## Post-Build: Just Works Experience

After building, the Playerbot module is designed for **zero-configuration startup**:

### First Run Experience

1. **Start the server** - no additional setup required
2. **Guided setup** automatically detects missing config:
   ```
   ================================================================================
     PLAYERBOT FIRST-TIME SETUP
   ================================================================================

     QUICK START (automatic setup):
     A default playerbots.conf will be created with tester-friendly settings.

     MANUAL SETUP:
     1. Copy playerbots.conf.dist to ./playerbots.conf
     2. Configure database settings
     3. Restart server
   ================================================================================
   ```
3. **Database auto-created** if MySQL user has CREATE privilege
4. **Migrations auto-applied** from `sql/migrations/`
5. **Bots spawn automatically** on server start (configurable)

### Quick Configuration with Profiles

For instant setup, use a pre-configured profile:

```ini
# In playerbots.conf
Playerbot.Profile = "standard"
```

**Available Profiles:**

| Profile | Bots | Use Case |
|---------|------|----------|
| `minimal` | 10 | Testing/development |
| `standard` | 100 | General use (recommended) |
| `performance` | 500 | High-population servers |
| `singleplayer` | varies | Solo play optimized |

### Configuration Location

All Playerbot settings are in `playerbots.conf` (NOT worldserver.conf):

```ini
# Essential settings
Playerbot.Enable = 1
Playerbot.MaxBots = 100
Playerbot.Spawn.OnServerStart = 1

# Database (auto-created if possible)
Playerbot.Database.Host = "127.0.0.1"
Playerbot.Database.Name = "playerbot_playerbot"
Playerbot.Database.User = "trinity"
Playerbot.Database.Password = "trinity"
```

### Verification

After startup, check the server log for:

```
[INFO] Playerbot Module initializing...
[INFO] PlayerbotDatabaseConnection: Connected to 127.0.0.1:3306/playerbot_playerbot
[INFO] PlayerbotMigrationMgr: Database schema is up to date
[INFO] BotSpawner: Beginning phased startup sequence...
[INFO] Playerbot Module initialized successfully
```

---

## Additional Resources

- **TrinityCore Wiki:** https://trinitycore.info/
- **Playerbot Documentation:** [README.md](README.md)
- **Database Installation:** [sql/INSTALLATION_README.md](sql/INSTALLATION_README.md)
- **Vendored Dependencies:** [VENDORED_DEPENDENCIES.md](VENDORED_DEPENDENCIES.md)
- **License Information:** [deps/LICENSES.md](deps/LICENSES.md)

---

**Maintainer:** TrinityCore Playerbot Team
**Last Updated:** 2026-01-06
**Version:** 1.1.0
