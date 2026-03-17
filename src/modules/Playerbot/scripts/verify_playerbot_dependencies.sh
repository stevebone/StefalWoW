#!/bin/bash
# TrinityCore Playerbot Dependency Verification Script
# Verifies all required enterprise-grade dependencies

set -e

echo "=== TrinityCore Playerbot Dependency Verification ==="

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Verification functions
verify_tbb() {
    echo -n "Checking Intel TBB... "

    # Check via pkg-config first
    if pkg-config --exists tbb 2>/dev/null; then
        TBB_VERSION=$(pkg-config --modversion tbb)
        echo -e "${GREEN}✅ Found TBB ${TBB_VERSION}${NC}"
        return 0
    fi

    # Check for header files
    if [ -f "/usr/include/tbb/version.h" ] || [ -f "/usr/local/include/tbb/version.h" ]; then
        echo -e "${GREEN}✅ Found TBB headers${NC}"
        return 0
    fi

    echo -e "${RED}❌ Intel TBB not found${NC}"
    echo -e "${YELLOW}  Install: sudo apt-get install libtbb-dev (Ubuntu/Debian)${NC}"
    echo -e "${YELLOW}  Or:     brew install tbb (macOS)${NC}"
    echo -e "${YELLOW}  Or:     yum install tbb-devel (RHEL/CentOS)${NC}"
    return 1
}

verify_phmap() {
    echo -n "Checking Parallel Hashmap... "

    # Check common include paths
    PHMAP_PATHS=(
        "/usr/include/parallel_hashmap/phmap.h"
        "/usr/local/include/parallel_hashmap/phmap.h"
        "/opt/include/parallel_hashmap/phmap.h"
        "$HOME/.local/include/parallel_hashmap/phmap.h"
    )

    for path in "${PHMAP_PATHS[@]}"; do
        if [ -f "$path" ]; then
            echo -e "${GREEN}✅ Found Parallel Hashmap at $path${NC}"
            return 0
        fi
    done

    echo -e "${RED}❌ Parallel Hashmap not found${NC}"
    echo -e "${YELLOW}  Install: git clone https://github.com/greg7mdp/parallel-hashmap.git${NC}"
    echo -e "${YELLOW}           cd parallel-hashmap && cmake -B build && sudo cmake --install build${NC}"
    echo -e "${YELLOW}  Or:     brew install parallel-hashmap (macOS)${NC}"
    return 1
}

verify_boost() {
    echo -n "Checking Boost... "

    # Check via pkg-config
    if pkg-config --exists boost 2>/dev/null; then
        BOOST_VERSION=$(pkg-config --modversion boost)

        # Parse version (format: 1.74.0)
        BOOST_MAJOR=$(echo $BOOST_VERSION | cut -d. -f1)
        BOOST_MINOR=$(echo $BOOST_VERSION | cut -d. -f2)

        if [ "$BOOST_MAJOR" -gt 1 ] || ([ "$BOOST_MAJOR" -eq 1 ] && [ "$BOOST_MINOR" -ge 74 ]); then
            echo -e "${GREEN}✅ Found Boost ${BOOST_VERSION}${NC}"
            return 0
        else
            echo -e "${YELLOW}⚠️  Found Boost ${BOOST_VERSION} (requires 1.74+)${NC}"
        fi
    fi

    # Check for Boost headers
    if [ -f "/usr/include/boost/version.hpp" ] || [ -f "/usr/local/include/boost/version.hpp" ]; then
        echo -e "${GREEN}✅ Found Boost headers${NC}"
        return 0
    fi

    echo -e "${RED}❌ Boost not found or version insufficient${NC}"
    echo -e "${YELLOW}  Install: sudo apt-get install libboost-all-dev (Ubuntu/Debian)${NC}"
    echo -e "${YELLOW}  Or:     brew install boost (macOS)${NC}"
    echo -e "${YELLOW}  Or:     yum install boost-devel (RHEL/CentOS)${NC}"
    return 1
}

verify_mysql() {
    echo -n "Checking MySQL client... "

    # Check via pkg-config
    if pkg-config --exists mysqlclient 2>/dev/null; then
        MYSQL_VERSION=$(pkg-config --modversion mysqlclient)
        echo -e "${GREEN}✅ Found MySQL client ${MYSQL_VERSION}${NC}"
        return 0
    fi

    # Check for mysql_config utility
    if command -v mysql_config >/dev/null 2>&1; then
        MYSQL_VERSION=$(mysql_config --version)
        echo -e "${GREEN}✅ Found MySQL client ${MYSQL_VERSION}${NC}"
        return 0
    fi

    # Check for headers
    if [ -f "/usr/include/mysql/mysql.h" ] || [ -f "/usr/local/include/mysql/mysql.h" ]; then
        echo -e "${GREEN}✅ Found MySQL headers${NC}"
        return 0
    fi

    echo -e "${RED}❌ MySQL client not found${NC}"
    echo -e "${YELLOW}  Install: sudo apt-get install libmysqlclient-dev (Ubuntu/Debian)${NC}"
    echo -e "${YELLOW}  Or:     brew install mysql (macOS)${NC}"
    echo -e "${YELLOW}  Or:     yum install mysql-devel (RHEL/CentOS)${NC}"
    return 1
}

verify_cmake() {
    echo -n "Checking CMake... "

    if command -v cmake >/dev/null 2>&1; then
        CMAKE_VERSION=$(cmake --version | head -n1 | cut -d' ' -f3)

        # Parse version (format: 3.24.0)
        CMAKE_MAJOR=$(echo $CMAKE_VERSION | cut -d. -f1)
        CMAKE_MINOR=$(echo $CMAKE_VERSION | cut -d. -f2)

        if [ "$CMAKE_MAJOR" -gt 3 ] || ([ "$CMAKE_MAJOR" -eq 3 ] && [ "$CMAKE_MINOR" -ge 24 ]); then
            echo -e "${GREEN}✅ Found CMake ${CMAKE_VERSION}${NC}"
            return 0
        else
            echo -e "${YELLOW}⚠️  Found CMake ${CMAKE_VERSION} (requires 3.24+)${NC}"
            return 1
        fi
    fi

    echo -e "${RED}❌ CMake not found${NC}"
    echo -e "${YELLOW}  Install: sudo apt-get install cmake (Ubuntu/Debian)${NC}"
    echo -e "${YELLOW}  Or:     brew install cmake (macOS)${NC}"
    return 1
}

verify_compiler() {
    echo -n "Checking C++20 compiler... "

    # Check GCC
    if command -v g++ >/dev/null 2>&1; then
        GCC_VERSION=$(g++ --version | head -n1 | grep -oP '\d+\.\d+' | head -1)
        GCC_MAJOR=$(echo $GCC_VERSION | cut -d. -f1)

        if [ "$GCC_MAJOR" -ge 11 ]; then
            echo -e "${GREEN}✅ Found GCC ${GCC_VERSION} (C++20 capable)${NC}"
            return 0
        fi
    fi

    # Check Clang
    if command -v clang++ >/dev/null 2>&1; then
        CLANG_VERSION=$(clang++ --version | head -n1 | grep -oP '\d+\.\d+' | head -1)
        CLANG_MAJOR=$(echo $CLANG_VERSION | cut -d. -f1)

        if [ "$CLANG_MAJOR" -ge 14 ]; then
            echo -e "${GREEN}✅ Found Clang ${CLANG_VERSION} (C++20 capable)${NC}"
            return 0
        fi
    fi

    echo -e "${RED}❌ No suitable C++20 compiler found${NC}"
    echo -e "${YELLOW}  Install: sudo apt-get install gcc-11 g++-11 (Ubuntu/Debian)${NC}"
    echo -e "${YELLOW}  Or:     brew install gcc (macOS)${NC}"
    return 1
}

check_system_info() {
    echo -e "${BLUE}=== System Information ===${NC}"
    echo "OS: $(uname -s) $(uname -r)"
    echo "Architecture: $(uname -m)"

    if [ -f /etc/os-release ]; then
        . /etc/os-release
        echo "Distribution: $NAME $VERSION"
    fi

    echo ""
}

# Main verification
main() {
    check_system_info

    echo -e "${BLUE}=== Dependency Verification ===${NC}"

    FAILED=0

    verify_cmake || FAILED=1
    verify_compiler || FAILED=1
    verify_tbb || FAILED=1
    verify_phmap || FAILED=1
    verify_boost || FAILED=1
    verify_mysql || FAILED=1

    echo ""

    if [ $FAILED -eq 0 ]; then
        echo -e "${GREEN}🎉 All dependencies verified successfully!${NC}"
        echo -e "${GREEN}You can now build TrinityCore with BUILD_PLAYERBOT=ON${NC}"
        echo ""
        echo -e "${BLUE}Build commands:${NC}"
        echo "  mkdir build && cd build"
        echo "  cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_PLAYERBOT=ON"
        echo "  make -j\$(nproc)"
        exit 0
    else
        echo -e "${RED}❌ Some dependencies are missing or insufficient.${NC}"
        echo -e "${RED}Please install the missing dependencies before building Playerbot.${NC}"
        echo ""
        echo -e "${BLUE}For complete installation instructions, see:${NC}"
        echo "  .claude/PHASE1/DEPENDENCY_VERIFICATION.md"
        exit 1
    fi
}

# Run verification
main