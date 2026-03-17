# Playerbot CI/CD Integration Guide

**Last Updated:** 2025-11-05
**Test Framework:** Google Test 1.14.0
**Coverage Tool:** gcov/lcov
**CI Platform:** GitHub Actions

## Overview

This document describes how to integrate Playerbot unit tests into continuous integration/continuous deployment (CI/CD) pipelines, configure automated test execution, and generate coverage reports.

## Table of Contents

1. [Local Test Execution](#local-test-execution)
2. [GitHub Actions Integration](#github-actions-integration)
3. [Coverage Reporting](#coverage-reporting)
4. [Performance Regression Detection](#performance-regression-detection)
5. [Test Result Publishing](#test-result-publishing)
6. [Quality Gates](#quality-gates)

---

## Local Test Execution

### Building Tests

```bash
# Configure CMake with tests enabled
cd build
cmake .. -DBUILD_PLAYERBOT=1 -DBUILD_PLAYERBOT_TESTS=ON

# Build test executable
make playerbot_tests -j8

# Or on Windows with MSBuild
MSBuild.exe playerbot_tests.vcxproj /p:Configuration=Release /m:8
```

### Running Tests

```bash
# Run all tests
./playerbot_tests

# Run specific test suite
./playerbot_tests --gtest_filter="*BloodDeathKnight*"

# Run with verbose output
./playerbot_tests --gtest_verbose

# Generate XML report for CI
./playerbot_tests --gtest_output=xml:test_results.xml

# Run performance tests only
./playerbot_tests --gtest_filter="*Performance*"
```

### Test Execution Options

| Option | Description |
|--------|-------------|
| `--gtest_filter=PATTERN` | Run only tests matching pattern |
| `--gtest_list_tests` | List all available tests |
| `--gtest_repeat=N` | Repeat tests N times |
| `--gtest_shuffle` | Randomize test execution order |
| `--gtest_break_on_failure` | Break debugger on first failure |
| `--gtest_output=xml:FILE` | Generate JUnit XML report |

---

## GitHub Actions Integration

### Workflow Configuration

Create `.github/workflows/playerbot-tests.yml`:

```yaml
name: Playerbot Tests

on:
  push:
    branches: [ master, playerbot-dev ]
    paths:
      - 'src/modules/Playerbot/**'
      - '.github/workflows/playerbot-tests.yml'
  pull_request:
    branches: [ master ]
    paths:
      - 'src/modules/Playerbot/**'

jobs:
  test:
    name: Unit Tests
    runs-on: ${{ matrix.os }}
    strategy:
      fail-fast: false
      matrix:
        os: [ubuntu-latest, windows-latest]
        compiler: [gcc, clang, msvc]
        exclude:
          - os: ubuntu-latest
            compiler: msvc
          - os: windows-latest
            compiler: gcc
          - os: windows-latest
            compiler: clang

    steps:
      - name: Checkout Repository
        uses: actions/checkout@v4
        with:
          submodules: recursive

      - name: Install Dependencies (Ubuntu)
        if: runner.os == 'Linux'
        run: |
          sudo apt-get update
          sudo apt-get install -y \
            build-essential \
            cmake \
            libgtest-dev \
            libboost-all-dev \
            libssl-dev \
            libmysqlclient-dev \
            lcov

      - name: Install Dependencies (Windows)
        if: runner.os == 'Windows'
        run: |
          choco install cmake --installargs 'ADD_CMAKE_TO_PATH=System'
          choco install ninja

      - name: Setup vcpkg (Windows)
        if: runner.os == 'Windows'
        run: |
          git clone https://github.com/microsoft/vcpkg.git
          .\vcpkg\bootstrap-vcpkg.bat
          .\vcpkg\vcpkg install gtest:x64-windows boost:x64-windows

      - name: Configure CMake
        run: |
          mkdir build && cd build
          cmake .. \
            -DBUILD_PLAYERBOT=ON \
            -DBUILD_PLAYERBOT_TESTS=ON \
            -DCMAKE_BUILD_TYPE=Debug \
            -DCMAKE_CXX_FLAGS="--coverage"

      - name: Build Tests
        run: |
          cd build
          cmake --build . --target playerbot_tests --config Debug -j4

      - name: Run Tests
        run: |
          cd build
          ./playerbot_tests --gtest_output=xml:test_results.xml

      - name: Generate Coverage Report (Linux)
        if: runner.os == 'Linux'
        run: |
          cd build
          lcov --capture --directory . --output-file coverage.info
          lcov --remove coverage.info '/usr/*' '*/tests/*' '*/gtest/*' --output-file coverage_filtered.info
          genhtml coverage_filtered.info --output-directory coverage_html

      - name: Upload Test Results
        if: always()
        uses: actions/upload-artifact@v3
        with:
          name: test-results-${{ matrix.os }}-${{ matrix.compiler }}
          path: build/test_results.xml

      - name: Upload Coverage Report
        if: runner.os == 'Linux'
        uses: actions/upload-artifact@v3
        with:
          name: coverage-report
          path: build/coverage_html

      - name: Publish Test Results
        if: always()
        uses: EnricoMi/publish-unit-test-result-action@v2
        with:
          files: build/test_results.xml
          check_name: "Test Results (${{ matrix.os }} - ${{ matrix.compiler }})"

  performance:
    name: Performance Benchmarks
    runs-on: ubuntu-latest
    steps:
      - name: Checkout Repository
        uses: actions/checkout@v4

      - name: Install Dependencies
        run: |
          sudo apt-get update
          sudo apt-get install -y build-essential cmake libgtest-dev

      - name: Build Tests
        run: |
          mkdir build && cd build
          cmake .. -DBUILD_PLAYERBOT=ON -DBUILD_PLAYERBOT_TESTS=ON -DCMAKE_BUILD_TYPE=Release
          cmake --build . --target playerbot_tests -j4

      - name: Run Performance Tests
        run: |
          cd build
          ./playerbot_tests --gtest_filter="*Performance*" --gtest_output=xml:perf_results.xml

      - name: Upload Performance Results
        uses: actions/upload-artifact@v3
        with:
          name: performance-results
          path: build/perf_results.xml
```

### Workflow Triggers

The workflow runs on:
- **Push to `master` or `playerbot-dev`** - Full test suite
- **Pull Request to `master`** - Full test suite + coverage
- **Path filter** - Only when Playerbot files change

---

## Coverage Reporting

### Generating Coverage Locally

```bash
# Configure with coverage flags
cmake .. -DBUILD_PLAYERBOT_TESTS=ON -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_FLAGS="--coverage"

# Build and run tests
make playerbot_tests
./playerbot_tests

# Generate coverage report
lcov --capture --directory . --output-file coverage.info
lcov --remove coverage.info '/usr/*' '*/tests/*' '*/gtest/*' --output-file coverage_filtered.info
genhtml coverage_filtered.info --output-directory coverage_html

# Open report
xdg-open coverage_html/index.html
```

### Coverage Targets

| Component | Target Coverage | Current |
|-----------|----------------|---------|
| ClassAI | 80% | TBD |
| Combat Systems | 85% | TBD |
| Lifecycle | 90% | TBD |
| Integration | 70% | TBD |
| **Overall** | **80%** | **TBD** |

### Coverage Quality Gates

Tests will fail CI if:
- Overall coverage < 80%
- Critical path coverage < 100%
- New code coverage < 90%

---

## Performance Regression Detection

### Baseline Metrics

Capture baseline performance metrics:

```bash
./playerbot_tests --gtest_filter="*Performance*" > baseline_performance.txt
```

### Regression Detection Script

Create `scripts/detect_performance_regression.py`:

```python
#!/usr/bin/env python3
import sys
import re

def parse_performance_output(filename):
    """Parse performance test output"""
    metrics = {}
    with open(filename, 'r') as f:
        for line in f:
            match = re.search(r'(\w+).*?(\d+\.?\d*)\s*ms', line)
            if match:
                test_name, time_ms = match.groups()
                metrics[test_name] = float(time_ms)
    return metrics

def detect_regression(baseline, current, threshold=0.1):
    """Detect performance regressions > threshold (10% default)"""
    regressions = []
    for test, baseline_time in baseline.items():
        current_time = current.get(test, 0)
        if current_time > baseline_time * (1 + threshold):
            regression_pct = ((current_time - baseline_time) / baseline_time) * 100
            regressions.append((test, baseline_time, current_time, regression_pct))
    return regressions

if __name__ == "__main__":
    baseline = parse_performance_output('baseline_performance.txt')
    current = parse_performance_output('current_performance.txt')

    regressions = detect_regression(baseline, current)

    if regressions:
        print("⚠️  Performance Regressions Detected:")
        for test, baseline_time, current_time, regression_pct in regressions:
            print(f"  - {test}: {baseline_time}ms → {current_time}ms (+{regression_pct:.1f}%)")
        sys.exit(1)
    else:
        print("✅ No performance regressions detected")
        sys.exit(0)
```

### Integration with CI

```yaml
- name: Check Performance Regression
  run: |
    ./playerbot_tests --gtest_filter="*Performance*" > current_performance.txt
    python scripts/detect_performance_regression.py
```

---

## Test Result Publishing

### JUnit XML Format

Tests generate JUnit XML compatible reports:

```xml
<?xml version="1.0" encoding="UTF-8"?>
<testsuites tests="585" failures="0" errors="0" time="12.345">
  <testsuite name="BloodDeathKnightTest" tests="15" failures="0" errors="0" time="0.234">
    <testcase name="RotationPriority_MaintainsBoneShield" time="0.015" />
    <testcase name="Performance_DecisionCycle_CompletesUnder1ms" time="0.089" />
  </testsuite>
</testsuites>
```

### GitHub Actions Test Summary

Test results appear in:
- **GitHub Actions Summary** - Test counts and failures
- **Pull Request Checks** - Pass/fail status
- **Annotations** - Failure details inline with code

---

## Quality Gates

### Pre-Merge Requirements

Pull requests must pass:

1. ✅ **All unit tests pass** (585 tests)
2. ✅ **Coverage >= 80%** (overall)
3. ✅ **No performance regressions** (> 10% slowdown)
4. ✅ **No memory leaks** (Valgrind/AddressSanitizer)
5. ✅ **Static analysis passes** (cppcheck, clang-tidy)

### Enforcement

```yaml
# Branch protection rules
required_status_checks:
  - Unit Tests (ubuntu-latest - gcc)
  - Unit Tests (windows-latest - msvc)
  - Performance Benchmarks
  - Coverage Report
```

---

## Troubleshooting

### Tests Fail to Build

**Problem:** Linker errors or missing symbols

**Solution:**
```bash
# Clean build directory
rm -rf build && mkdir build && cd build

# Reconfigure with verbose output
cmake .. -DBUILD_PLAYERBOT_TESTS=ON --trace

# Build with verbose linking
make VERBOSE=1 playerbot_tests
```

### Tests Timeout

**Problem:** Tests exceed 5-minute timeout

**Solution:**
```yaml
# Increase timeout in GitHub Actions
- name: Run Tests
  run: ./playerbot_tests
  timeout-minutes: 10
```

### Coverage Report Empty

**Problem:** No coverage data generated

**Solution:**
```bash
# Ensure coverage flags are set
cmake .. -DCMAKE_CXX_FLAGS="--coverage" -DCMAKE_EXE_LINKER_FLAGS="--coverage"

# Run tests before capturing coverage
./playerbot_tests

# Verify .gcda files exist
find . -name "*.gcda"
```

---

## Best Practices

### 1. Fast Feedback Loop

- Run affected tests locally before pushing
- Use `--gtest_filter` to run specific suites
- Enable parallel test execution where possible

### 2. Stable Tests

- Avoid flaky tests (time-dependent, race conditions)
- Use fixed random seeds for reproducibility
- Mock external dependencies

### 3. Meaningful Assertions

- Use descriptive failure messages
- Assert expected behavior, not implementation
- Test one concept per test case

### 4. Performance Awareness

- Keep individual tests < 100ms
- Total suite < 5 minutes
- Profile slow tests regularly

---

## Contact & Support

**Issues:** Report CI/CD problems to the development team
**Documentation:** See README_TESTING.md for test suite details
**Performance:** See TestHelpers.h for benchmarking utilities

---

**Last Updated:** 2025-11-05
**Test Count:** 585+ comprehensive tests
**Coverage Target:** 80% overall, 100% critical paths
