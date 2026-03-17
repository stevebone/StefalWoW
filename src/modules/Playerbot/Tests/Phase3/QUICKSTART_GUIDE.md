# Phase 3 Testing Framework - Developer Quickstart Guide

## Overview

This guide gets you up and running with the Phase 3 god class refactoring testing framework in under 15 minutes.

---

## Prerequisites

### Required Software
- ✅ **CMake 3.24+**
- ✅ **C++20 Compiler** (GCC 10+, Clang 12+, MSVC 2019+)
- ✅ **Google Test (gtest)** - Auto-downloaded by CMake
- ✅ **Google Mock (gmock)** - Included with gtest
- ✅ **Git** (for baseline comparison)

### Optional Software
- 🎯 **Google Benchmark** (for performance benchmarks)
- 🎯 **Valgrind** (for memory leak detection - Linux only)
- 🎯 **AddressSanitizer** (for memory safety - built into compilers)
- 🎯 **ThreadSanitizer** (for race condition detection)
- 🎯 **lcov/genhtml** (for code coverage reports)

---

## 5-Minute Setup

### Step 1: Enable Tests in CMake (1 minute)
```bash
cd C:\TrinityBots\TrinityCore
mkdir -p build && cd build

cmake -DBUILD_PLAYERBOT_TESTS=ON \
      -DCMAKE_BUILD_TYPE=Debug \
      ..
```

**What this does:**
- Enables PlayerBot test compilation
- Downloads and configures Google Test
- Sets up test discovery for CTest

### Step 2: Build Tests (3 minutes)
```bash
make -j$(nproc) playerbot_tests
```

**Expected Output:**
```
[ 95%] Building CXX object src/modules/Playerbot/Tests/CMakeFiles/playerbot_tests.dir/Phase3/Unit/Specializations/DisciplinePriestSpecializationTest.cpp.o
[100%] Linking CXX executable playerbot_tests
[100%] Built target playerbot_tests
```

### Step 3: Run Tests (1 minute)
```bash
./playerbot_tests --gtest_filter="*Phase3*"
```

**Expected Output:**
```
[==========] Running 55 tests from 3 test suites.
[----------] Global test environment set-up.
[----------] 40 tests from DisciplinePriestSpecializationTest
[ RUN      ] DisciplinePriestSpecializationTest.Constructor_ValidParameters_InitializesCorrectly
[       OK ] DisciplinePriestSpecializationTest.Constructor_ValidParameters_InitializesCorrectly (0 ms)
...
[==========] 55 tests from 3 test suites ran. (1234 ms total)
[  PASSED  ] 55 tests.
```

---

## 10-Minute Deep Dive

### Running Specific Test Suites

#### Unit Tests Only (Fast - 30 seconds)
```bash
./playerbot_tests --gtest_filter="*Phase3Unit*"
```

#### Integration Tests Only (Medium - 2 minutes)
```bash
./playerbot_tests --gtest_filter="*Phase3Integration*"
```

#### Performance Tests Only (Slow - 5 minutes)
```bash
./playerbot_tests --gtest_filter="*Phase3*Performance*"
```

#### Run Single Test
```bash
./playerbot_tests --gtest_filter="*DisciplinePriest*FlashHeal"
```

### Debugging a Failing Test

#### Enable Verbose Output
```bash
./playerbot_tests --gtest_filter="*TestName*" --gtest_print_time=1
```

#### Run with GDB (Linux/Mac)
```bash
gdb --args ./playerbot_tests --gtest_filter="*TestName*"
(gdb) run
(gdb) bt  # Backtrace on crash
```

#### Run with Visual Studio Debugger (Windows)
```powershell
# Open Visual Studio
# Load playerbot_tests.exe
# Set breakpoint in test
# Debug → Start Debugging (F5)
```

### Writing Your First Test

#### 1. Create Test File
```bash
cd src/modules/Playerbot/Tests/Phase3/Unit/Specializations
cp DisciplinePriestSpecializationTest.cpp MySpecializationTest.cpp
```

#### 2. Minimal Test Template
```cpp
#include "Tests/Phase3/Unit/Mocks/MockFramework.h"
#include <gtest/gtest.h>

namespace Playerbot::Test
{

TEST(MySpecializationTest, BasicTest)
{
    // Arrange: Create test objects
    auto mockPlayer = MockFactory::CreateMockPlayer(CLASS_PRIEST, 80);

    // Act: Execute behavior
    bool result = mockPlayer->IsAlive();

    // Assert: Verify outcome
    EXPECT_TRUE(result);
}

} // namespace Playerbot::Test
```

#### 3. Add to CMakeLists.txt
```cmake
# In src/modules/Playerbot/Tests/Phase3/CMakeLists.txt
set(PHASE3_TEST_SOURCES
    Unit/Specializations/DisciplinePriestSpecializationTest.cpp
    Unit/Specializations/MySpecializationTest.cpp  # <-- Add this
    Integration/ClassAIIntegrationTest.cpp
)
```

#### 4. Rebuild and Run
```bash
make playerbot_tests
./playerbot_tests --gtest_filter="*MySpecialization*"
```

---

## Common Tasks

### Task 1: Check Test Coverage
```bash
# Build with coverage flags
cmake -DBUILD_PLAYERBOT_TESTS=ON \
      -DCMAKE_BUILD_TYPE=Debug \
      -DCMAKE_CXX_FLAGS="--coverage" \
      ..
make -j$(nproc) playerbot_tests

# Run tests
./playerbot_tests --gtest_filter="*Phase3*"

# Generate coverage report
lcov --capture --directory . --output-file coverage.info
lcov --remove coverage.info '/usr/*' '*/tests/*' --output-file coverage_filtered.info
genhtml coverage_filtered.info --output-directory coverage_html

# View report
# Linux: xdg-open coverage_html/index.html
# Mac: open coverage_html/index.html
# Windows: start coverage_html/index.html
```

### Task 2: Detect Memory Leaks
```bash
# Build with AddressSanitizer
cmake -DBUILD_PLAYERBOT_TESTS=ON \
      -DPLAYERBOT_TESTS_ASAN=ON \
      -DCMAKE_BUILD_TYPE=Debug \
      ..
make -j$(nproc) playerbot_tests

# Run tests (will auto-detect leaks)
./playerbot_tests --gtest_filter="*Phase3*"

# If leaks detected, output will show:
# ==12345==ERROR: LeakSanitizer: detected memory leaks
# Direct leak of 64 byte(s) in 1 object(s) allocated from:
#   #0 0x7f... in operator new
#   #1 0x56... in YourClass::Method()
```

### Task 3: Detect Race Conditions
```bash
# Build with ThreadSanitizer
cmake -DBUILD_PLAYERBOT_TESTS=ON \
      -DPLAYERBOT_TESTS_TSAN=ON \
      -DCMAKE_BUILD_TYPE=Debug \
      ..
make -j$(nproc) playerbot_tests

# Run tests (will auto-detect races)
./playerbot_tests --gtest_filter="*Phase3*"

# If races detected, output will show:
# WARNING: ThreadSanitizer: data race (pid=12345)
#   Write of size 4 at 0x7b... by thread T1:
#     #0 YourClass::UnsafeMethod()
#   Previous read of size 4 at 0x7b... by main thread:
#     #0 YourClass::UnsafeRead()
```

### Task 4: Run Performance Benchmarks
```bash
# Build benchmarks (requires Google Benchmark)
cmake -DBUILD_PLAYERBOT_TESTS=ON \
      -DCMAKE_BUILD_TYPE=Release \
      ..
make -j$(nproc) playerbot_benchmarks

# Run benchmarks
./playerbot_benchmarks

# Expected Output:
# Run on (8 X 3600 MHz CPU s)
# ------------------------------------------------------------
# Benchmark                              Time      CPU   Iterations
# ------------------------------------------------------------
# BM_DisciplineRotation              45.2 us   45.1 us    15543
# BM_ResourceCalculation              2.8 us    2.8 us   249876
# BM_TargetSelection                  8.3 us    8.3 us    84321
```

### Task 5: Capture Pre-Refactor Baseline
```bash
# Checkout pre-refactor code
git checkout playerbot-dev

# Build and run baseline tests
cmake -DBUILD_PLAYERBOT_TESTS=ON -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc) playerbot_tests
./playerbot_tests --gtest_filter="*BaselineCapture*" --gtest_output=json:baseline.json

# Save baseline with timestamp
mkdir -p baselines
cp baseline.json baselines/phase3_baseline_$(date +%Y%m%d).json

# Baseline saved! Use for regression testing after refactor
```

### Task 6: Compare Post-Refactor Against Baseline
```bash
# After refactoring, checkout feature branch
git checkout feature/phase3-refactor

# Build and run regression tests
cmake -DBUILD_PLAYERBOT_TESTS=ON -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc) playerbot_tests
./playerbot_tests --gtest_filter="*Phase3Regression*" --gtest_output=json:current.json

# Compare against baseline
./tools/compare_metrics.py \
    --baseline=baselines/phase3_baseline_20251017.json \
    --current=current.json \
    --tolerance=5

# Generate HTML report
./tools/generate_regression_report.py \
    --baseline=baselines/phase3_baseline_20251017.json \
    --current=current.json \
    --output=regression_report.html

# View report
# xdg-open regression_report.html (Linux)
# open regression_report.html (Mac)
# start regression_report.html (Windows)
```

---

## Troubleshooting

### Problem: Tests Won't Compile
**Symptom:** Compiler errors about missing headers or undefined symbols

**Solution:**
```bash
# Ensure BUILD_PLAYERBOT_TESTS is enabled
grep BUILD_PLAYERBOT_TESTS CMakeCache.txt

# If not found, reconfigure:
cmake -DBUILD_PLAYERBOT_TESTS=ON ..

# Clean and rebuild
make clean
make -j$(nproc) playerbot_tests
```

### Problem: Tests Fail to Run
**Symptom:** `./playerbot_tests: command not found`

**Solution:**
```bash
# Check if binary exists
ls -l playerbot_tests

# If not found, check build output:
find . -name "playerbot_tests"

# Binary should be in: build/src/modules/Playerbot/Tests/
cd src/modules/Playerbot/Tests
./playerbot_tests --gtest_filter="*Phase3*"
```

### Problem: All Tests Fail
**Symptom:** Every test reports `[  FAILED  ]`

**Solution:**
```bash
# Run with verbose output to diagnose
./playerbot_tests --gtest_filter="*Phase3Unit*Construct*" --gtest_print_time=1

# Check for missing dependencies
ldd playerbot_tests  # Linux/Mac
# Look for "not found" entries

# Verify test data directory exists
ls src/modules/Playerbot/Tests/Phase3/
```

### Problem: Tests Are Too Slow
**Symptom:** Tests take >10 minutes to run

**Solution:**
```bash
# Run only fast unit tests
./playerbot_tests --gtest_filter="*Phase3Unit*" --gtest_filter=-"*Performance*"

# Parallel test execution (CTest)
ctest -j$(nproc) -R Phase3

# Or build in Release mode for faster execution
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc) playerbot_tests
```

### Problem: Memory Leak False Positives
**Symptom:** AddressSanitizer reports leaks in gtest internals

**Solution:**
```bash
# Suppress known gtest leaks
export ASAN_OPTIONS=detect_leaks=1:suppressions=asan_suppressions.txt

# Create suppressions file
cat > asan_suppressions.txt << EOF
leak:testing::internal::UnitTestImpl::RunAllTests
EOF

# Run tests
./playerbot_tests --gtest_filter="*Phase3*"
```

---

## Best Practices

### ✅ DO
- ✅ Run unit tests before every commit
- ✅ Write tests alongside code (TDD)
- ✅ Keep tests focused (one assertion per test when possible)
- ✅ Use descriptive test names (`Rotation_LowHealthAlly_CastsFlashHeal`)
- ✅ Mock external dependencies completely
- ✅ Run performance tests before/after optimizations
- ✅ Capture baselines before major refactoring
- ✅ Use AddressSanitizer during development

### ❌ DON'T
- ❌ Skip tests because "they take too long" (run fast subset)
- ❌ Commit code with failing tests
- ❌ Write tests that depend on external state (database, network)
- ❌ Ignore flaky tests (fix them!)
- ❌ Mock everything (integration tests need real interactions)
- ❌ Hardcode timing assumptions (use ranges or mock time)
- ❌ Forget to clean up resources in TearDown()

---

## Quick Reference

### Test Filter Patterns
```bash
# All Phase 3 tests
--gtest_filter="*Phase3*"

# Specific test class
--gtest_filter="*DisciplinePriest*"

# Specific test case
--gtest_filter="*DisciplinePriest*FlashHeal"

# Exclude performance tests
--gtest_filter="*Phase3*" --gtest_filter=-"*Performance*"

# Multiple patterns (OR)
--gtest_filter="*Priest*:*Mage*"
```

### Useful gtest Flags
```bash
--gtest_list_tests            # List all tests without running
--gtest_repeat=100            # Run tests 100 times (stress test)
--gtest_shuffle               # Randomize test execution order
--gtest_break_on_failure      # Break into debugger on first failure
--gtest_catch_exceptions=0    # Don't catch exceptions (for debugging)
--gtest_output=xml:results.xml # Output JUnit XML for CI
--gtest_print_time=1          # Print test execution time
```

### Mock Assertion Macros
```cpp
// Basic assertions
EXPECT_TRUE(condition);
EXPECT_FALSE(condition);
EXPECT_EQ(expected, actual);
EXPECT_NE(value1, value2);
EXPECT_LT(val1, val2);
EXPECT_LE(val1, val2);
EXPECT_GT(val1, val2);
EXPECT_GE(val1, val2);

// Floating point assertions
EXPECT_FLOAT_EQ(expected, actual);
EXPECT_NEAR(expected, actual, tolerance);

// String assertions
EXPECT_STREQ(expected, actual);
EXPECT_STRCASEEQ(expected, actual);

// Custom Phase 3 assertions
EXPECT_SPELL_CAST(player, spellId);
EXPECT_SPELL_NOT_CAST(player, spellId);
EXPECT_MANA_SUFFICIENT(player, spellCost);
EXPECT_COOLDOWN_READY(player, spellId);
EXPECT_IN_COMBAT(unit);
EXPECT_TARGET_SET(unit, expectedTarget);
```

### Mock Expectation Patterns
```cpp
// Basic expectation
EXPECT_CALL(*mockPlayer, CastSpell(_, spellId, _))
    .Times(1)
    .WillOnce(Return(SPELL_CAST_OK));

// Multiple calls
EXPECT_CALL(*mockPlayer, GetHealth())
    .Times(AtLeast(1))
    .WillRepeatedly(Return(10000));

// Sequence of returns
EXPECT_CALL(*mockPlayer, GetPower(POWER_MANA))
    .WillOnce(Return(5000))
    .WillOnce(Return(4500))
    .WillRepeatedly(Return(4000));

// Parameter matching
EXPECT_CALL(*mockPlayer, CastSpell(_, FLASH_HEAL, _))
    .With(Args<0, 1>(IsCastValid()))
    .Times(1);

// Ordered expectations
{
    InSequence seq;
    EXPECT_CALL(*mockPlayer, CastSpell(_, INNER_FOCUS, _));
    EXPECT_CALL(*mockPlayer, CastSpell(_, GREATER_HEAL, _));
}
```

---

## Additional Resources

### Documentation
- 📄 **Full Architecture:** `PHASE3_TESTING_FRAMEWORK_ARCHITECTURE.md` (35 pages)
- 📄 **Implementation Summary:** `PHASE3_TESTING_IMPLEMENTATION_SUMMARY.md` (20 pages)
- 📄 **This Guide:** `QUICKSTART_GUIDE.md` (you are here)

### Example Code
- 📁 **Mock Framework:** `Unit/Mocks/MockFramework.h`
- 📁 **Unit Test Example:** `Unit/Specializations/DisciplinePriestSpecializationTest.cpp`
- 📁 **Integration Test Example:** `Integration/ClassAIIntegrationTest.cpp`

### External Resources
- 🌐 **Google Test Primer:** https://google.github.io/googletest/primer.html
- 🌐 **Google Mock Docs:** https://google.github.io/googletest/gmock_for_dummies.html
- 🌐 **AddressSanitizer:** https://github.com/google/sanitizers/wiki/AddressSanitizer
- 🌐 **ThreadSanitizer:** https://github.com/google/sanitizers/wiki/ThreadSanitizerCppManual

---

## Getting Help

### Questions?
1. Check this quickstart guide first
2. Review architecture document for design questions
3. Inspect example tests for patterns
4. Ask in team chat with `[Phase3 Testing]` tag

### Bugs in Tests?
1. Verify test environment is correct (CMake flags, dependencies)
2. Run with verbose output (`--gtest_print_time=1`)
3. Use debugger to step through failing test
4. Check if bug is in test or in code under test
5. Report issue with full repro steps

### Need New Features?
1. Check if mock framework supports it
2. Extend MockFramework.h if needed
3. Add test helper methods to TestUtilities
4. Document new patterns in architecture doc

---

**Happy Testing!** 🧪

*Remember: Good tests are not a burden, they're a superpower. They catch bugs before users do, enable fearless refactoring, and serve as living documentation.*

---

**Document Version:** 1.0
**Last Updated:** 2025-10-17
**Maintained By:** Test Automation Engineer (Claude Code)
