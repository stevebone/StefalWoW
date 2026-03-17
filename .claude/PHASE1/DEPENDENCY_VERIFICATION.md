# TrinityCore Playerbot Dependencies Verification

## Overview
Comprehensive dependency verification system for TrinityCore Playerbot enterprise-grade implementation.

## Required Dependencies

### Intel Threading Building Blocks (TBB)
**Purpose**: Work-stealing thread pool, concurrent containers, parallel algorithms
**Minimum Version**: 2021.5.0
**Components Required**:
- `tbb::task_arena` - Thread pool management
- `tbb::concurrent_queue` - Lock-free packet queues
- `tbb::parallel_for` - Batch session updates
- `tbb::concurrent_hash_map` - Thread-safe session storage

### Parallel Hashmap (phmap)
**Purpose**: High-performance concurrent hash maps
**Minimum Version**: 1.3.8
**Components Required**:
- `phmap::parallel_flat_hash_map` - Session storage
- `phmap::parallel_node_hash_map` - Bot metadata caching

### Boost Libraries
**Purpose**: Circular buffers, object pools, async I/O
**Minimum Version**: 1.74.0
**Components Required**:
- `boost::circular_buffer` - Fixed-size packet queues
- `boost::object_pool` - Zero-allocation memory pools
- `boost::lockfree::queue` - Lock-free data structures
- `boost::asio` - Async database operations

### MySQL Connector C++
**Purpose**: Database connectivity for bot metadata
**Minimum Version**: 8.0.33
**Components Required**:
- `mysqlcppconn` - C++ connector library
- Connection pooling support
- Prepared statement support

## Verification Scripts

### CMake Dependency Detection
```cmake
# File: cmake/modules/FindPlayerbotDependencies.cmake

# Intel TBB Detection
find_package(TBB 2021.5 REQUIRED COMPONENTS tbb)
if(NOT TBB_FOUND)
    message(FATAL_ERROR "Intel TBB 2021.5+ is required for Playerbot enterprise features")
endif()

# Verify TBB components
include(CheckCXXSourceCompiles)
set(CMAKE_REQUIRED_LIBRARIES TBB::tbb)
check_cxx_source_compiles("
    #include <tbb/task_arena.h>
    #include <tbb/concurrent_queue.h>
    #include <tbb/parallel_for.h>
    #include <tbb/concurrent_hash_map.h>
    int main() {
        tbb::task_arena arena;
        tbb::concurrent_queue<int> queue;
        return 0;
    }
" TBB_COMPONENTS_AVAILABLE)

if(NOT TBB_COMPONENTS_AVAILABLE)
    message(FATAL_ERROR "Required TBB components not available")
endif()

# Parallel Hashmap Detection
find_path(PHMAP_INCLUDE_DIR
    NAMES parallel_hashmap/phmap.h
    HINTS ${PHMAP_ROOT} $ENV{PHMAP_ROOT}
    PATH_SUFFIXES include)

if(NOT PHMAP_INCLUDE_DIR)
    message(FATAL_ERROR "Parallel Hashmap (phmap) headers not found. Install from: https://github.com/greg7mdp/parallel-hashmap")
endif()

# Verify phmap version
file(READ "${PHMAP_INCLUDE_DIR}/parallel_hashmap/phmap_config.h" PHMAP_CONFIG)
string(REGEX MATCH "#define PHMAP_VERSION_MAJOR ([0-9]+)" _ "${PHMAP_CONFIG}")
set(PHMAP_VERSION_MAJOR ${CMAKE_MATCH_1})
string(REGEX MATCH "#define PHMAP_VERSION_MINOR ([0-9]+)" _ "${PHMAP_CONFIG}")
set(PHMAP_VERSION_MINOR ${CMAKE_MATCH_1})

if(PHMAP_VERSION_MAJOR LESS 1 OR (PHMAP_VERSION_MAJOR EQUAL 1 AND PHMAP_VERSION_MINOR LESS 3))
    message(FATAL_ERROR "Parallel Hashmap version 1.3.8+ required, found ${PHMAP_VERSION_MAJOR}.${PHMAP_VERSION_MINOR}")
endif()

# Boost Detection with required components
find_package(Boost 1.74.0 REQUIRED COMPONENTS system thread)
if(NOT Boost_FOUND)
    message(FATAL_ERROR "Boost 1.74.0+ is required for Playerbot")
endif()

# Verify Boost components
check_cxx_source_compiles("
    #include <boost/circular_buffer.hpp>
    #include <boost/pool/object_pool.hpp>
    #include <boost/lockfree/queue.hpp>
    #include <boost/asio.hpp>
    int main() {
        boost::circular_buffer<int> cb(10);
        boost::object_pool<int> pool;
        boost::lockfree::queue<int> queue(10);
        boost::asio::io_context ctx;
        return 0;
    }
" BOOST_COMPONENTS_AVAILABLE)

if(NOT BOOST_COMPONENTS_AVAILABLE)
    message(FATAL_ERROR "Required Boost components not available")
endif()

# MySQL Connector Detection
find_package(MySQL REQUIRED)
if(NOT MYSQL_FOUND)
    message(FATAL_ERROR "MySQL client libraries required for Playerbot database")
endif()

# Verify MySQL Connector C++
find_library(MYSQL_CPPCONN_LIB
    NAMES mysqlcppconn mysqlcppconn8
    HINTS ${MYSQL_ROOT}/lib ${MYSQL_ROOT}/lib64
    PATH_SUFFIXES mysql)

if(NOT MYSQL_CPPCONN_LIB)
    message(FATAL_ERROR "MySQL Connector C++ not found. Install MySQL Connector C++ 8.0.33+")
endif()

message(STATUS "✅ All Playerbot dependencies verified successfully")
```

### Runtime Dependency Verification
```cpp
// File: src/modules/Playerbot/Config/DependencyValidator.h

#pragma once

#include <string>
#include <vector>

namespace Playerbot {

struct DependencyInfo
{
    std::string name;
    std::string version;
    std::string status;
    bool required;
};

class TC_GAME_API DependencyValidator
{
public:
    static bool ValidateAllDependencies();
    static std::vector<DependencyInfo> GetDependencyStatus();
    static void LogDependencyReport();

private:
    static bool ValidateTBB();
    static bool ValidatePhmap();
    static bool ValidateBoost();
    static bool ValidateMySQL();

    static std::string GetTBBVersion();
    static std::string GetBoostVersion();
    static std::string GetMySQLVersion();
};

} // namespace Playerbot
```

```cpp
// File: src/modules/Playerbot/Config/DependencyValidator.cpp

#include "DependencyValidator.h"
#include "Define.h"
#include "Log.h"

// TBB Headers
#include <tbb/version.h>
#include <tbb/task_arena.h>
#include <tbb/concurrent_queue.h>
#include <tbb/parallel_for.h>

// Parallel Hashmap Headers
#include <parallel_hashmap/phmap.h>

// Boost Headers
#include <boost/version.hpp>
#include <boost/circular_buffer.hpp>
#include <boost/pool/object_pool.hpp>
#include <boost/lockfree/queue.hpp>
#include <boost/asio/version.hpp>

// MySQL Headers
#include <mysql.h>

namespace Playerbot {

bool DependencyValidator::ValidateAllDependencies()
{
    TC_LOG_INFO("module.playerbot.dependencies", "Validating Playerbot dependencies...");

    bool success = true;

    // Validate Intel TBB
    if (!ValidateTBB()) {
        TC_LOG_ERROR("module.playerbot.dependencies", "Intel TBB validation failed");
        success = false;
    }

    // Validate Parallel Hashmap
    if (!ValidatePhmap()) {
        TC_LOG_ERROR("module.playerbot.dependencies", "Parallel Hashmap validation failed");
        success = false;
    }

    // Validate Boost
    if (!ValidateBoost()) {
        TC_LOG_ERROR("module.playerbot.dependencies", "Boost validation failed");
        success = false;
    }

    // Validate MySQL
    if (!ValidateMySQL()) {
        TC_LOG_ERROR("module.playerbot.dependencies", "MySQL validation failed");
        success = false;
    }

    if (success) {
        TC_LOG_INFO("module.playerbot.dependencies", "✅ All dependencies validated successfully");
    } else {
        TC_LOG_ERROR("module.playerbot.dependencies", "❌ Dependency validation failed");
    }

    return success;
}

bool DependencyValidator::ValidateTBB()
{
    try {
        // Check TBB version
        int major = TBB_VERSION_MAJOR;
        int minor = TBB_VERSION_MINOR;

        if (major < 2021 || (major == 2021 && minor < 5)) {
            TC_LOG_ERROR("module.playerbot.dependencies",
                "Intel TBB version {}.{} insufficient, required 2021.5+", major, minor);
            return false;
        }

        // Test TBB functionality
        tbb::task_arena arena;
        tbb::concurrent_queue<int> queue;

        // Test parallel_for
        std::vector<int> test_data(100);
        tbb::parallel_for(0, 100, [&](int i) {
            test_data[i] = i * 2;
        });

        TC_LOG_INFO("module.playerbot.dependencies",
            "✅ Intel TBB {}.{} validated successfully", major, minor);
        return true;
    }
    catch (std::exception const& e) {
        TC_LOG_ERROR("module.playerbot.dependencies",
            "Intel TBB validation exception: {}", e.what());
        return false;
    }
}

bool DependencyValidator::ValidatePhmap()
{
    try {
        // Test parallel_flat_hash_map
        phmap::parallel_flat_hash_map<uint32, std::string> test_map;
        test_map[1] = "test";
        test_map[2] = "validation";

        if (test_map.size() != 2) {
            TC_LOG_ERROR("module.playerbot.dependencies",
                "Parallel hashmap basic functionality test failed");
            return false;
        }

        TC_LOG_INFO("module.playerbot.dependencies",
            "✅ Parallel Hashmap validated successfully");
        return true;
    }
    catch (std::exception const& e) {
        TC_LOG_ERROR("module.playerbot.dependencies",
            "Parallel Hashmap validation exception: {}", e.what());
        return false;
    }
}

bool DependencyValidator::ValidateBoost()
{
    try {
        // Check Boost version
        int major = BOOST_VERSION / 100000;
        int minor = (BOOST_VERSION / 100) % 1000;

        if (major < 1 || (major == 1 && minor < 74)) {
            TC_LOG_ERROR("module.playerbot.dependencies",
                "Boost version {}.{} insufficient, required 1.74+", major, minor);
            return false;
        }

        // Test circular_buffer
        boost::circular_buffer<int> cb(10);
        cb.push_back(1);
        cb.push_back(2);

        // Test object_pool
        boost::object_pool<int> pool;
        int* obj = pool.malloc();
        if (!obj) {
            TC_LOG_ERROR("module.playerbot.dependencies",
                "Boost object_pool allocation failed");
            return false;
        }
        pool.free(obj);

        // Test lockfree queue
        boost::lockfree::queue<int> queue(10);
        queue.push(42);
        int value;
        if (!queue.pop(value) || value != 42) {
            TC_LOG_ERROR("module.playerbot.dependencies",
                "Boost lockfree queue test failed");
            return false;
        }

        // Test asio basic functionality
        boost::asio::io_context ctx;

        TC_LOG_INFO("module.playerbot.dependencies",
            "✅ Boost {}.{} validated successfully", major, minor);
        return true;
    }
    catch (std::exception const& e) {
        TC_LOG_ERROR("module.playerbot.dependencies",
            "Boost validation exception: {}", e.what());
        return false;
    }
}

bool DependencyValidator::ValidateMySQL()
{
    try {
        // Check MySQL client library version
        const char* version = mysql_get_client_info();
        if (!version) {
            TC_LOG_ERROR("module.playerbot.dependencies",
                "MySQL client library not available");
            return false;
        }

        // Parse version string (format: "8.0.33")
        std::string versionStr(version);
        size_t first_dot = versionStr.find('.');
        size_t second_dot = versionStr.find('.', first_dot + 1);

        if (first_dot == std::string::npos || second_dot == std::string::npos) {
            TC_LOG_ERROR("module.playerbot.dependencies",
                "Cannot parse MySQL version: {}", version);
            return false;
        }

        int major = std::stoi(versionStr.substr(0, first_dot));
        int minor = std::stoi(versionStr.substr(first_dot + 1, second_dot - first_dot - 1));
        int patch = std::stoi(versionStr.substr(second_dot + 1));

        if (major < 8 || (major == 8 && minor == 0 && patch < 33)) {
            TC_LOG_ERROR("module.playerbot.dependencies",
                "MySQL version {}.{}.{} insufficient, required 8.0.33+", major, minor, patch);
            return false;
        }

        TC_LOG_INFO("module.playerbot.dependencies",
            "✅ MySQL {}.{}.{} validated successfully", major, minor, patch);
        return true;
    }
    catch (std::exception const& e) {
        TC_LOG_ERROR("module.playerbot.dependencies",
            "MySQL validation exception: {}", e.what());
        return false;
    }
}

std::vector<DependencyInfo> DependencyValidator::GetDependencyStatus()
{
    std::vector<DependencyInfo> dependencies;

    dependencies.push_back({
        "Intel TBB",
        GetTBBVersion(),
        ValidateTBB() ? "✅ OK" : "❌ FAILED",
        true
    });

    dependencies.push_back({
        "Parallel Hashmap",
        "1.3.8+",
        ValidatePhmap() ? "✅ OK" : "❌ FAILED",
        true
    });

    dependencies.push_back({
        "Boost",
        GetBoostVersion(),
        ValidateBoost() ? "✅ OK" : "❌ FAILED",
        true
    });

    dependencies.push_back({
        "MySQL",
        GetMySQLVersion(),
        ValidateMySQL() ? "✅ OK" : "❌ FAILED",
        true
    });

    return dependencies;
}

void DependencyValidator::LogDependencyReport()
{
    auto dependencies = GetDependencyStatus();

    TC_LOG_INFO("module.playerbot.dependencies", "=== Playerbot Dependency Report ===");
    for (auto const& dep : dependencies) {
        TC_LOG_INFO("module.playerbot.dependencies",
            "{:<20} | {:<10} | {}", dep.name, dep.version, dep.status);
    }
    TC_LOG_INFO("module.playerbot.dependencies", "================================");
}

std::string DependencyValidator::GetTBBVersion()
{
    return std::to_string(TBB_VERSION_MAJOR) + "." + std::to_string(TBB_VERSION_MINOR);
}

std::string DependencyValidator::GetBoostVersion()
{
    int major = BOOST_VERSION / 100000;
    int minor = (BOOST_VERSION / 100) % 1000;
    return std::to_string(major) + "." + std::to_string(minor);
}

std::string DependencyValidator::GetMySQLVersion()
{
    const char* version = mysql_get_client_info();
    return version ? std::string(version) : "Unknown";
}

} // namespace Playerbot
```

### Build-Time Verification Script
```bash
#!/bin/bash
# File: scripts/verify_playerbot_dependencies.sh

set -e

echo "=== TrinityCore Playerbot Dependency Verification ==="

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Verification functions
verify_tbb() {
    echo -n "Checking Intel TBB... "
    if pkg-config --exists tbb; then
        TBB_VERSION=$(pkg-config --modversion tbb)
        echo -e "${GREEN}✅ Found TBB ${TBB_VERSION}${NC}"
        return 0
    else
        echo -e "${RED}❌ Intel TBB not found${NC}"
        echo "  Install: sudo apt-get install libtbb-dev (Ubuntu/Debian)"
        echo "  Or:     brew install tbb (macOS)"
        return 1
    fi
}

verify_phmap() {
    echo -n "Checking Parallel Hashmap... "
    if [ -f "/usr/include/parallel_hashmap/phmap.h" ] || [ -f "/usr/local/include/parallel_hashmap/phmap.h" ]; then
        echo -e "${GREEN}✅ Found Parallel Hashmap headers${NC}"
        return 0
    else
        echo -e "${RED}❌ Parallel Hashmap not found${NC}"
        echo "  Install: git clone https://github.com/greg7mdp/parallel-hashmap.git"
        echo "           cd parallel-hashmap && cmake -B build && cmake --install build"
        return 1
    fi
}

verify_boost() {
    echo -n "Checking Boost... "
    if pkg-config --exists boost; then
        BOOST_VERSION=$(pkg-config --modversion boost)
        echo -e "${GREEN}✅ Found Boost ${BOOST_VERSION}${NC}"
        return 0
    else
        echo -e "${RED}❌ Boost not found${NC}"
        echo "  Install: sudo apt-get install libboost-all-dev (Ubuntu/Debian)"
        echo "  Or:     brew install boost (macOS)"
        return 1
    fi
}

verify_mysql() {
    echo -n "Checking MySQL... "
    if pkg-config --exists mysqlclient; then
        MYSQL_VERSION=$(pkg-config --modversion mysqlclient)
        echo -e "${GREEN}✅ Found MySQL ${MYSQL_VERSION}${NC}"
        return 0
    else
        echo -e "${RED}❌ MySQL client not found${NC}"
        echo "  Install: sudo apt-get install libmysqlclient-dev (Ubuntu/Debian)"
        echo "  Or:     brew install mysql (macOS)"
        return 1
    fi
}

# Run verifications
FAILED=0

verify_tbb || FAILED=1
verify_phmap || FAILED=1
verify_boost || FAILED=1
verify_mysql || FAILED=1

echo ""
if [ $FAILED -eq 0 ]; then
    echo -e "${GREEN}🎉 All dependencies verified successfully!${NC}"
    echo "You can now build TrinityCore with BUILD_PLAYERBOT=ON"
    exit 0
else
    echo -e "${RED}❌ Some dependencies are missing.${NC}"
    echo "Please install the missing dependencies before building Playerbot."
    exit 1
fi
```

### Windows PowerShell Verification Script
```powershell
# File: scripts/verify_playerbot_dependencies.ps1

param(
    [string]$VcpkgRoot = $env:VCPKG_ROOT
)

Write-Host "=== TrinityCore Playerbot Dependency Verification ===" -ForegroundColor Cyan

$Failed = $false

function Test-VcpkgPackage {
    param([string]$PackageName)

    if (-not $VcpkgRoot) {
        Write-Host "❌ VCPKG_ROOT not set" -ForegroundColor Red
        return $false
    }

    $InstalledPath = Join-Path $VcpkgRoot "installed\x64-windows\lib"
    $PackageFiles = Get-ChildItem -Path $InstalledPath -Filter "*$PackageName*" -ErrorAction SilentlyContinue

    return $PackageFiles.Count -gt 0
}

# Verify Intel TBB
Write-Host "Checking Intel TBB... " -NoNewline
if (Test-VcpkgPackage "tbb") {
    Write-Host "✅ Found" -ForegroundColor Green
} else {
    Write-Host "❌ Not found" -ForegroundColor Red
    Write-Host "  Install: vcpkg install tbb:x64-windows" -ForegroundColor Yellow
    $Failed = $true
}

# Verify Parallel Hashmap
Write-Host "Checking Parallel Hashmap... " -NoNewline
if (Test-VcpkgPackage "parallel-hashmap") {
    Write-Host "✅ Found" -ForegroundColor Green
} else {
    Write-Host "❌ Not found" -ForegroundColor Red
    Write-Host "  Install: vcpkg install parallel-hashmap:x64-windows" -ForegroundColor Yellow
    $Failed = $true
}

# Verify Boost
Write-Host "Checking Boost... " -NoNewline
if (Test-VcpkgPackage "boost") {
    Write-Host "✅ Found" -ForegroundColor Green
} else {
    Write-Host "❌ Not found" -ForegroundColor Red
    Write-Host "  Install: vcpkg install boost:x64-windows" -ForegroundColor Yellow
    $Failed = $true
}

# Verify MySQL
Write-Host "Checking MySQL... " -NoNewline
if (Test-VcpkgPackage "mysql") {
    Write-Host "✅ Found" -ForegroundColor Green
} else {
    Write-Host "❌ Not found" -ForegroundColor Red
    Write-Host "  Install: vcpkg install mysql:x64-windows" -ForegroundColor Yellow
    $Failed = $true
}

Write-Host ""
if (-not $Failed) {
    Write-Host "🎉 All dependencies verified successfully!" -ForegroundColor Green
    Write-Host "You can now build TrinityCore with BUILD_PLAYERBOT=ON"
    exit 0
} else {
    Write-Host "❌ Some dependencies are missing." -ForegroundColor Red
    Write-Host "Please install the missing dependencies before building Playerbot."
    exit 1
}
```

### Integration with Build System
```cmake
# File: src/modules/Playerbot/CMakeLists.txt (partial)

# Always verify dependencies when building Playerbot
if(BUILD_PLAYERBOT)
    message(STATUS "Validating Playerbot dependencies...")

    # Include dependency finder
    include(${CMAKE_SOURCE_DIR}/cmake/modules/FindPlayerbotDependencies.cmake)

    # Create dependency validation executable
    add_executable(playerbot-dependency-test
        Config/DependencyValidator.cpp
        Config/DependencyValidator.h)

    target_link_libraries(playerbot-dependency-test
        PRIVATE
            TBB::tbb
            ${MYSQL_LIBRARIES}
            Boost::system
            Boost::thread)

    target_include_directories(playerbot-dependency-test
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}
            ${PHMAP_INCLUDE_DIR})

    # Run dependency validation as part of build
    add_custom_command(
        TARGET playerbot-dependency-test POST_BUILD
        COMMAND playerbot-dependency-test
        COMMENT "Validating Playerbot runtime dependencies"
        VERBATIM)
endif()
```

## Installation Instructions

### Ubuntu/Debian
```bash
# Intel TBB
sudo apt-get update
sudo apt-get install libtbb-dev

# Boost (1.74+ required)
sudo apt-get install libboost-all-dev

# MySQL
sudo apt-get install libmysqlclient-dev

# Parallel Hashmap (header-only)
git clone https://github.com/greg7mdp/parallel-hashmap.git
cd parallel-hashmap
cmake -B build -DCMAKE_INSTALL_PREFIX=/usr/local
sudo cmake --install build
```

### macOS (Homebrew)
```bash
# Intel TBB
brew install tbb

# Boost
brew install boost

# MySQL
brew install mysql

# Parallel Hashmap
brew install parallel-hashmap
```

### Windows (vcpkg)
```powershell
# Setup vcpkg if not already done
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
.\bootstrap-vcpkg.bat
.\vcpkg integrate install

# Install dependencies
.\vcpkg install tbb:x64-windows
.\vcpkg install boost:x64-windows
.\vcpkg install mysql:x64-windows
.\vcpkg install parallel-hashmap:x64-windows
```

This comprehensive dependency verification system ensures all enterprise-grade dependencies are properly validated before implementing the BotSession system.