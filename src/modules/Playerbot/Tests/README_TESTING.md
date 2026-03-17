# Automated World Population System - Testing Guide

## Overview

This directory contains comprehensive integration tests and performance benchmarks for the Automated World Population System.

### Test Coverage

**Systems Tested:**
- ✅ BotLevelDistribution - Level bracket selection and distribution balance
- ✅ BotGearFactory - Gear set generation and quality distribution
- ✅ BotTalentManager - Specialization selection and talent application
- ✅ BotWorldPositioner - Zone selection and placement
- ✅ BotLevelManager - Complete orchestration workflow

## Test Files

### 1. AutomatedWorldPopulationTests.cpp

**Purpose:** Integration tests for all subsystems

**Test Categories:**
- **Level Distribution Tests** (5 tests)
  - System initialization
  - Bracket selection (Alliance/Horde)
  - Distribution balance over time

- **Gear Factory Tests** (4 tests)
  - System initialization
  - Complete gear set generation
  - Level progression (iLvl scaling)
  - Quality distribution (Green/Blue/Purple)

- **Talent Manager Tests** (5 tests)
  - System initialization
  - Spec selection for all classes
  - Dual-spec support (L10+)
  - Hero talent support (L71-80)
  - Talent loadout retrieval

- **World Positioner Tests** (5 tests)
  - System initialization
  - Starter zone selection (L1-4)
  - Leveling zone selection (L5-60)
  - Endgame zone selection (L60-80)
  - Capital city fallback

- **Bot Level Manager Tests** (4 tests)
  - Orchestrator initialization
  - Level bracket selection
  - Throttling configuration
  - Statistics tracking

- **Integration Tests** (1 test)
  - End-to-end system coordination

- **Performance Tests** (3 tests)
  - Level selection speed (<0.1ms)
  - Gear generation speed (<5ms)
  - Zone selection speed (<0.05ms)

- **Stress Tests** (1 test)
  - 1000-bot distribution balance

**Total: 28 tests**

### 2. PerformanceBenchmarks.cpp

**Purpose:** Performance profiling and optimization validation

**Benchmark Categories:**
- **Level Distribution** (2 benchmarks)
  - Alliance bracket selection (10,000 iterations)
  - Horde bracket selection (10,000 iterations)
  - Target: <0.1ms per operation

- **Gear Factory** (2 benchmarks)
  - Level 20 gear generation (1,000 iterations)
  - Level 80 gear generation (1,000 iterations)
  - Target: <5ms per generation

- **Talent Manager** (2 benchmarks)
  - Specialization selection (10,000 iterations)
  - Talent loadout retrieval (10,000 iterations)
  - Target: <0.1ms per operation

- **World Positioner** (3 benchmarks)
  - Starter zone selection (10,000 iterations)
  - Leveling zone selection (10,000 iterations)
  - Endgame zone selection (10,000 iterations)
  - Target: <0.05ms per operation

- **Integrated Workflow** (1 benchmark)
  - Full worker thread preparation (1,000 iterations)
  - Target: <5ms total time

- **Memory Usage Analysis**
  - Cache size estimation
  - Per-bot memory tracking
  - Target: <10MB for 5,000 bots

- **Scalability Tests**
  - 100, 500, 1000, 2500, 5000 bot simulations
  - Throughput measurements (bots/second)

**Total: 11 benchmarks**

## Performance Targets

### Speed Targets

| Operation | Target | Component |
|-----------|--------|-----------|
| Level bracket selection | <0.1ms | BotLevelDistribution |
| Gear set generation | <5ms | BotGearFactory |
| Spec selection | <0.1ms | BotTalentManager |
| Zone selection | <0.05ms | BotWorldPositioner |
| Worker thread prep | <5ms | BotLevelManager |
| Main thread apply | <50ms | BotLevelManager |
| **Total bot creation** | **<55ms** | **End-to-End** |

### Resource Targets

| Resource | Target | Maximum |
|----------|--------|---------|
| CPU per bot | <0.1% | 0.5% |
| Memory per bot | <10MB | 15MB |
| Cache memory | <1MB | 5MB |
| Queue memory (100 bots) | ~100KB | 500KB |

### Scalability Targets

| Bot Count | Max Prep Time | Throughput |
|-----------|---------------|------------|
| 100 | 500ms | >200 bots/sec |
| 500 | 2.5s | >200 bots/sec |
| 1,000 | 5s | >200 bots/sec |
| 2,500 | 12.5s | >200 bots/sec |
| 5,000 | 25s | >200 bots/sec |

## Building Tests

### Prerequisites

**Google Test Framework:**
```bash
# Install Google Test (Ubuntu/Debian)
sudo apt-get install libgtest-dev

# Install Google Test (macOS)
brew install googletest

# Windows: Download from https://github.com/google/googletest
```

### CMake Configuration

**Note:** Google Test is not yet configured in the current CMakeLists.txt. To enable tests:

```cmake
# Add to src/modules/Playerbot/CMakeLists.txt

if(BUILD_TESTING)
    find_package(GTest REQUIRED)

    # Integration Tests
    add_executable(playerbot_integration_tests
        Tests/AutomatedWorldPopulationTests.cpp
    )

    target_link_libraries(playerbot_integration_tests
        PRIVATE
            playerbot
            GTest::GTest
            GTest::Main
    )

    gtest_discover_tests(playerbot_integration_tests)

    # Performance Benchmarks
    add_executable(playerbot_benchmarks
        Tests/PerformanceBenchmarks.cpp
    )

    target_link_libraries(playerbot_benchmarks
        PRIVATE
            playerbot
    )
endif()
```

### Build Commands

```bash
# Configure with tests enabled
cmake .. -DBUILD_PLAYERBOT=1 -DBUILD_TESTING=ON

# Build tests
make playerbot_integration_tests
make playerbot_benchmarks

# Or on Windows with MSBuild
MSBuild.exe playerbot_integration_tests.vcxproj
MSBuild.exe playerbot_benchmarks.vcxproj
```

## Running Tests

### Integration Tests

```bash
# Run all integration tests
./playerbot_integration_tests

# Run specific test suite
./playerbot_integration_tests --gtest_filter=AutomatedWorldPopulationTest.LevelDistribution*

# Run with verbose output
./playerbot_integration_tests --gtest_verbose

# Generate XML report
./playerbot_integration_tests --gtest_output=xml:test_results.xml
```

### Performance Benchmarks

```bash
# Run all benchmarks
./playerbot_benchmarks

# Output will show:
# - Individual benchmark results
# - Performance vs targets
# - Memory usage analysis
# - Scalability metrics
```

### Expected Output

**Integration Tests:**
```
[==========] Running 28 tests from 1 test suite.
[----------] Global test environment set-up.
[----------] 28 tests from AutomatedWorldPopulationTest
[ RUN      ] AutomatedWorldPopulationTest.LevelDistribution_LoadsSuccessfully
[       OK ] AutomatedWorldPopulationTest.LevelDistribution_LoadsSuccessfully (2 ms)
...
[----------] 28 tests from AutomatedWorldPopulationTest (1234 ms total)

[==========] 28 tests from 1 test suite ran. (1234 ms total)
[  PASSED  ] 28 tests.
```

**Performance Benchmarks:**
```
##########################################################
#  AUTOMATED WORLD POPULATION - PERFORMANCE BENCHMARKS   #
##########################################################

Initializing subsystems...
✅ All subsystems initialized

##########################################################
# LEVEL DISTRIBUTION BENCHMARKS                          #
##########################################################

====================================================================
BENCHMARK: Level Bracket Selection (Alliance)
====================================================================
  Iterations:      10000
  Total Time:      95 ms
  Average Time:    0.009 ms
  Min Time:        0.005 ms
  Max Time:        0.025 ms
  Std Deviation:   0.003 ms
  Throughput:      105263.2 ops/sec
====================================================================

✅ PASS: Average time within target

...
```

## Continuous Integration

### GitHub Actions Example

```yaml
name: Playerbot Tests

on: [push, pull_request]

jobs:
  test:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v2

      - name: Install Dependencies
        run: |
          sudo apt-get update
          sudo apt-get install -y libgtest-dev cmake

      - name: Build
        run: |
          mkdir build && cd build
          cmake .. -DBUILD_PLAYERBOT=1 -DBUILD_TESTING=ON
          make playerbot_integration_tests
          make playerbot_benchmarks

      - name: Run Tests
        run: |
          cd build
          ./playerbot_integration_tests --gtest_output=xml:test_results.xml

      - name: Run Benchmarks
        run: |
          cd build
          ./playerbot_benchmarks > benchmark_results.txt

      - name: Upload Results
        uses: actions/upload-artifact@v2
        with:
          name: test-results
          path: |
            build/test_results.xml
            build/benchmark_results.txt
```

## Manual Testing

### In-Game Testing

**Test Scenario 1: Single Bot Creation**
```
1. Enable system: Playerbot.LevelManager.Enabled = 1
2. Create bot character
3. Call: sBotLevelManager->CreateBotAsync(bot)
4. Wait for queue processing
5. Verify:
   - Bot leveled to correct bracket
   - Gear equipped
   - Talents applied
   - Teleported to zone
```

**Test Scenario 2: Batch Bot Creation**
```
1. Create 100 bot characters
2. Call: sBotLevelManager->CreateBotsAsync(bots)
3. Monitor queue size
4. Verify:
   - All bots processed
   - Distribution balanced
   - No failures logged
```

**Test Scenario 3: Stress Test**
```
1. Create 1000 bot characters
2. Process in batches
3. Monitor:
   - Server performance (TPS)
   - Memory usage
   - Queue drain time
4. Verify:
   - No server stalls
   - Memory within limits
   - Distribution balanced
```

## Debugging Failed Tests

### Common Issues

**1. Subsystem Not Initialized**
```
ERROR: BotLevelDistribution not ready
```
**Solution:** Check configuration files, ensure `LoadDistribution()` succeeds

**2. Database Not Populated**
```
WARN: No talent loadouts loaded
```
**Solution:** Run SQL migration scripts in `sql/base/`

**3. Performance Target Missed**
```
WARNING: Average time exceeds target (<0.1ms)
```
**Solution:** Profile with `perf`, check for lock contention

**4. Distribution Imbalance**
```
ERROR: Bracket L1 has 35% of bots (unbalanced)
```
**Solution:** Adjust bracket percentages in configuration

### Debug Logging

Enable verbose logging for troubleshooting:
```conf
Playerbot.LevelManager.VerboseLogging = 1
Playerbot.GearFactory.LogStats = 1
Playerbot.TalentManager.LogApplications = 1
Playerbot.WorldPositioner.LogPlacements = 1
```

## Performance Profiling

### CPU Profiling (Linux)

```bash
# Run with perf
perf record -g ./playerbot_benchmarks
perf report

# Generate flamegraph
perf script | stackcollapse-perf.pl | flamegraph.pl > flamegraph.svg
```

### Memory Profiling (Linux)

```bash
# Run with valgrind
valgrind --tool=massif ./playerbot_benchmarks
ms_print massif.out.*

# Check for leaks
valgrind --leak-check=full ./playerbot_integration_tests
```

### Windows Profiling

```powershell
# Use Visual Studio Profiler
devenv /DebugExe playerbot_benchmarks.exe

# Or use PerfView
PerfView.exe run playerbot_benchmarks.exe
```

## Test Maintenance

### Adding New Tests

```cpp
// Add to AutomatedWorldPopulationTests.cpp

TEST_F(AutomatedWorldPopulationTest, YourNewTest_Description)
{
    // Arrange
    auto system = sBotLevelManager;

    // Act
    auto result = system->YourMethod();

    // Assert
    EXPECT_TRUE(result);
    EXPECT_GT(result.value, 0);
}
```

### Updating Benchmarks

```cpp
// Add to PerformanceBenchmarks.cpp

void BenchmarkYourFeature()
{
    auto result = PerformanceBenchmark::Run("Your Feature", 1000, []()
    {
        // Your code to benchmark
    });

    result.Print();

    if (result.avgTimeMs > YOUR_TARGET)
        std::cout << "⚠️  WARNING: Exceeds target\n";
    else
        std::cout << "✅ PASS\n";
}
```

## Contact & Support

**Issues:** Report test failures or performance issues to the development team
**Documentation:** See main README.md for system architecture details
**Performance:** Target specifications in implementation plan document

---

**Last Updated:** 2025-01-18
**Test Framework:** Google Test 1.14.0
**Compiler:** C++20 compliant (GCC 11+, Clang 14+, MSVC 2022+)
