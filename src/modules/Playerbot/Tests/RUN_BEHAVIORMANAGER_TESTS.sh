#!/bin/bash
# ============================================================================
# BehaviorManager Test Runner - Linux/macOS
# ============================================================================
#
# This script provides convenient shortcuts for running BehaviorManager
# unit tests on Linux and macOS platforms.
#
# Usage:
#   ./RUN_BEHAVIORMANAGER_TESTS.sh [command]
#
# Commands:
#   all         - Run all BehaviorManager tests (default)
#   throttling  - Run throttling mechanism tests
#   performance - Run performance tests
#   errors      - Run error handling tests
#   init        - Run initialization tests
#   edge        - Run edge case tests
#   scenarios   - Run integration scenario tests
#   verbose     - Run all tests with verbose output
#   repeat      - Run all tests 10 times (stability check)
#   coverage    - Generate code coverage report (requires gcov/lcov)
#   valgrind    - Run tests under Valgrind memory checker
#   gdb         - Run tests under GDB debugger
#
# Requirements:
#   - TrinityCore built with -DBUILD_PLAYERBOT_TESTS=ON
#   - playerbot_tests executable in build output directory
#
# ============================================================================

set -e  # Exit on error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Detect build directory
BUILD_DIR="../../../../../build"
TEST_EXE="${BUILD_DIR}/bin/playerbot_tests"

if [ ! -f "${TEST_EXE}" ]; then
    echo -e "${RED}ERROR: Could not find playerbot_tests${NC}"
    echo ""
    echo "Please build TrinityCore with -DBUILD_PLAYERBOT_TESTS=ON"
    echo "Expected location: ${TEST_EXE}"
    echo ""
    exit 1
fi

# Parse command line argument
COMMAND="${1:-all}"

echo "============================================================================"
echo "BehaviorManager Test Runner"
echo "============================================================================"
echo "Test Executable: ${TEST_EXE}"
echo "Command: ${COMMAND}"
echo "============================================================================"
echo ""

# Execute appropriate test command
case "${COMMAND}" in
    all)
        echo -e "${BLUE}Running all BehaviorManager tests...${NC}"
        "${TEST_EXE}" --gtest_filter="BehaviorManagerTest.*"
        ;;

    throttling)
        echo -e "${BLUE}Running throttling mechanism tests...${NC}"
        "${TEST_EXE}" --gtest_filter="BehaviorManagerTest.*Throttling*"
        ;;

    performance)
        echo -e "${BLUE}Running performance tests...${NC}"
        "${TEST_EXE}" --gtest_filter="BehaviorManagerTest.*Performance*"
        ;;

    errors)
        echo -e "${BLUE}Running error handling tests...${NC}"
        "${TEST_EXE}" --gtest_filter="BehaviorManagerTest.*ErrorHandling*"
        ;;

    init)
        echo -e "${BLUE}Running initialization tests...${NC}"
        "${TEST_EXE}" --gtest_filter="BehaviorManagerTest.*Initialization*"
        ;;

    edge)
        echo -e "${BLUE}Running edge case tests...${NC}"
        "${TEST_EXE}" --gtest_filter="BehaviorManagerTest.*EdgeCase*"
        ;;

    scenarios)
        echo -e "${BLUE}Running integration scenario tests...${NC}"
        "${TEST_EXE}" --gtest_filter="BehaviorManagerTest.*Scenario*"
        ;;

    verbose)
        echo -e "${BLUE}Running all tests with verbose output...${NC}"
        "${TEST_EXE}" --gtest_filter="BehaviorManagerTest.*" --gtest_color=yes
        ;;

    repeat)
        echo -e "${BLUE}Running all tests 10 times for stability check...${NC}"
        "${TEST_EXE}" --gtest_filter="BehaviorManagerTest.*" --gtest_repeat=10
        ;;

    coverage)
        echo -e "${BLUE}Generating code coverage report...${NC}"
        if ! command -v lcov &> /dev/null; then
            echo -e "${RED}ERROR: lcov not found. Please install lcov for coverage reports.${NC}"
            exit 1
        fi
        cd "${BUILD_DIR}"
        make playerbot_coverage
        echo -e "${GREEN}Coverage report generated in ${BUILD_DIR}/coverage/html/index.html${NC}"
        ;;

    valgrind)
        echo -e "${BLUE}Running tests under Valgrind...${NC}"
        if ! command -v valgrind &> /dev/null; then
            echo -e "${RED}ERROR: valgrind not found. Please install valgrind.${NC}"
            exit 1
        fi
        valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes \
                 --verbose "${TEST_EXE}" --gtest_filter="BehaviorManagerTest.*"
        ;;

    gdb)
        echo -e "${BLUE}Running tests under GDB debugger...${NC}"
        if ! command -v gdb &> /dev/null; then
            echo -e "${RED}ERROR: gdb not found. Please install gdb.${NC}"
            exit 1
        fi
        gdb --args "${TEST_EXE}" --gtest_filter="BehaviorManagerTest.*"
        ;;

    *)
        echo -e "${RED}ERROR: Unknown command \"${COMMAND}\"${NC}"
        echo ""
        echo "Available commands:"
        echo "  all         - Run all BehaviorManager tests"
        echo "  throttling  - Run throttling mechanism tests"
        echo "  performance - Run performance tests"
        echo "  errors      - Run error handling tests"
        echo "  init        - Run initialization tests"
        echo "  edge        - Run edge case tests"
        echo "  scenarios   - Run integration scenario tests"
        echo "  verbose     - Run all tests with verbose output"
        echo "  repeat      - Run all tests 10 times"
        echo "  coverage    - Generate code coverage report"
        echo "  valgrind    - Run tests under Valgrind"
        echo "  gdb         - Run tests under GDB"
        echo ""
        exit 1
        ;;
esac

# Capture exit code
EXIT_CODE=$?

echo ""
echo "============================================================================"
if [ ${EXIT_CODE} -eq 0 ]; then
    echo -e "${GREEN}TESTS PASSED${NC}"
else
    echo -e "${RED}TESTS FAILED - Exit code: ${EXIT_CODE}${NC}"
fi
echo "============================================================================"
echo ""

exit ${EXIT_CODE}
