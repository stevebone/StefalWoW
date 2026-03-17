# Phase 3 God Class Refactoring: Testing & Validation Framework Architecture

## Executive Summary

This document defines a comprehensive, enterprise-grade testing and validation framework for Phase 3 god class refactoring of the TrinityCore PlayerBot module. The framework ensures zero functional regression while validating performance improvements across 5000+ concurrent bots.

**Scope:** 10,000+ lines of critical AI code refactoring across:
- **PriestAI.cpp:** 3,154 lines → 3 specialization classes
- **MageAI.cpp:** 2,329 lines → 3 specialization classes
- **ShamanAI.cpp:** 2,631 lines → 3 specialization classes
- **BotAI.cpp:** 1,887 lines → Clean coordinator architecture

**Quality Gates:** All tests must pass before refactored code merges to `playerbot-dev` branch.

---

## 1. Testing Framework Architecture

### 1.1 Test Hierarchy

```
Phase3Tests/
├── Unit/                           # Isolated component testing
│   ├── Specializations/           # Specialization class unit tests
│   │   ├── PriestSpecializationTests.cpp
│   │   ├── MageSpecializationTests.cpp
│   │   └── ShamanSpecializationTests.cpp
│   ├── Coordinators/              # ClassAI coordinator tests
│   │   ├── PriestAICoordinatorTest.cpp
│   │   ├── MageAICoordinatorTest.cpp
│   │   └── ShamanAICoordinatorTest.cpp
│   ├── BaseClasses/               # Base architecture tests
│   │   ├── CombatSpecializationBaseTest.cpp
│   │   ├── PositionStrategyBaseTest.cpp
│   │   └── ResourceManagerTest.cpp
│   └── Mocks/                     # Mock implementations
│       ├── MockPlayer.h
│       ├── MockUnit.h
│       ├── MockSpellInfo.h
│       └── MockBotAI.h
│
├── Integration/                   # System interaction testing
│   ├── ClassAIIntegration/       # ClassAI → BotAI integration
│   │   ├── PriestIntegrationTest.cpp
│   │   ├── MageIntegrationTest.cpp
│   │   └── ShamanIntegrationTest.cpp
│   ├── CombatBehaviors/          # Combat system integration
│   │   ├── RotationIntegrationTest.cpp
│   │   ├── CooldownCoordinationTest.cpp
│   │   └── ResourceManagementTest.cpp
│   ├── GroupCoordination/        # Group play integration
│   │   ├── GroupHealingTest.cpp
│   │   ├── GroupDPSTest.cpp
│   │   └── GroupBuffingTest.cpp
│   └── EventSystem/              # Event handler integration
│       ├── CombatEventIntegrationTest.cpp
│       ├── AuraEventIntegrationTest.cpp
│       └── ResourceEventIntegrationTest.cpp
│
├── Regression/                    # Before/after comparison testing
│   ├── Baseline/                 # Pre-refactor baseline capture
│   │   ├── BaselineCapture.cpp
│   │   ├── BaselineMetrics.json
│   │   └── BaselineScenarios.cpp
│   ├── Comparison/               # Post-refactor comparison
│   │   ├── CombatEffectivenessComparison.cpp
│   │   ├── PerformanceComparison.cpp
│   │   └── BehaviorConsistencyComparison.cpp
│   └── Reports/                  # Generated comparison reports
│       ├── regression_report_YYYYMMDD.html
│       └── performance_delta_YYYYMMDD.json
│
├── Performance/                   # Performance benchmarking
│   ├── Benchmarks/               # Microbenchmarks
│   │   ├── RotationBenchmark.cpp
│   │   ├── ResourceCalcBenchmark.cpp
│   │   └── TargetSelectionBenchmark.cpp
│   ├── LoadTests/                # Scalability testing
│   │   ├── Load_100Bots.cpp
│   │   ├── Load_1000Bots.cpp
│   │   └── Load_5000Bots.cpp
│   ├── StressTests/              # Stability under stress
│   │   ├── MemoryLeakDetection.cpp
│   │   ├── ThreadSafetyTest.cpp
│   │   └── LongRunningStabilityTest.cpp
│   └── Profiling/                # Performance profiling
│       ├── CPUProfiler.cpp
│       ├── MemoryProfiler.cpp
│       └── ProfilingReport.cpp
│
└── Manual/                        # Manual test scenarios
    ├── Scenarios/                # Detailed test scenarios
    │   ├── SoloLeveling1to80.md
    │   ├── DungeonRuns5Man.md
    │   ├── RaidScenarios10Plus.md
    │   └── PvPBattlegrounds.md
    ├── Checklists/               # Validation checklists
    │   ├── CombatEffectivenessChecklist.md
    │   ├── BehaviorConsistencyChecklist.md
    │   └── PerformanceChecklist.md
    └── Results/                  # Manual test results
        ├── test_results_YYYYMMDD.md
        └── issue_tracker.md
```

### 1.2 Test Framework Components

#### A. Test Utilities Library (`TestUtilities/`)
- **MockFactory:** Creates all mock TrinityCore objects
- **TestBot Helper:** Spawns and configures test bots
- **MetricsCollector:** Captures performance metrics
- **ScenarioRunner:** Executes pre-defined test scenarios
- **AssertionHelpers:** Custom assertion macros for bot behavior

#### B. Test Data Fixtures (`TestData/`)
- **SpellDatabase:** Test spell data for all classes
- **TalentConfigurations:** Pre-defined talent builds
- **EquipmentSets:** Standard gear configurations
- **ScenarioDefinitions:** JSON test scenario definitions
- **BaselineData:** Captured pre-refactor metrics

#### C. Continuous Integration (`CI/`)
- **GitHubActions:** Automated test execution on PR
- **Jenkins:** Internal CI pipeline configuration
- **Docker:** Containerized test environment
- **TestReports:** HTML/JSON test result generation

---

## 2. Unit Testing Strategy

### 2.1 Specialization Class Testing

**Objective:** Verify each specialization class implements combat rotations correctly in isolation.

**Test Coverage Requirements:**
- **Rotation Logic:** 100% coverage of all rotation decision paths
- **Resource Management:** All power/mana calculations validated
- **Cooldown Management:** Ability usage timing verified
- **Target Selection:** Prioritization logic tested
- **Edge Cases:** OOM, target death, interrupt scenarios

**Example Test Structure:**
```cpp
TEST_F(DisciplineSpecializationTest, Rotation_LowHealthAlly_CastsFlashHeal) {
    // Arrange: Create low-health ally
    auto ally = CreateMockPlayer(50% health);
    auto disc = CreateDisciplineSpec();

    // Act: Execute rotation
    SpellCastResult result = disc->ExecuteRotation();

    // Assert: Flash Heal was selected
    EXPECT_EQ(result.spellId, FLASH_HEAL_SPELL_ID);
    EXPECT_EQ(result.target, ally->GetGUID());
    EXPECT_SPELL_CAST_WITHIN_MS(result, 100);
}
```

**Validation Criteria:**
- All spells in rotation tested at least once
- Priority system validated across 10+ scenarios
- Resource calculations within ±1% accuracy
- Execution time <50ms per rotation cycle

### 2.2 Mock Object Strategy

**TrinityCore Dependency Mocks:**
```cpp
class MockPlayer : public ::testing::Test {
    MOCK_METHOD(uint32, GetHealth, (), (const));
    MOCK_METHOD(uint32, GetMaxHealth, (), (const));
    MOCK_METHOD(uint32, GetPower, (Powers), (const));
    MOCK_METHOD(bool, IsInCombat, (), (const));
    MOCK_METHOD(Unit*, GetVictim, (), (const));
    MOCK_METHOD(SpellInfo const*, GetSpellInfo, (uint32), (const));
    MOCK_METHOD(bool, HasAura, (uint32), (const));
    MOCK_METHOD(bool, IsWithinLOSInMap, (WorldObject const*), (const));
};

class MockSpellInfo {
    MOCK_METHOD(uint32, GetManaCost, (), (const));
    MOCK_METHOD(uint32, GetCooldown, (), (const));
    MOCK_METHOD(uint32, GetCastTime, (), (const));
    MOCK_METHOD(float, GetRange, (), (const));
};
```

**Mock Isolation Strategy:**
- Each test uses fresh mock instances
- No state shared between tests
- Strict expectation matching enabled
- Unexpected calls fail tests immediately

### 2.3 Resource Manager Testing

**Critical Tests:**
```cpp
// Power availability checks
TEST(ResourceManagerTest, HasEnoughPower_Sufficient_ReturnsTrue)
TEST(ResourceManagerTest, HasEnoughPower_Insufficient_ReturnsFalse)

// Cooldown tracking
TEST(ResourceManagerTest, IsOnCooldown_RecentCast_ReturnsTrue)
TEST(ResourceManagerTest, IsOnCooldown_CooldownExpired_ReturnsFalse)

// Resource prediction
TEST(ResourceManagerTest, PredictPowerAfterCast_AccurateCalculation)
TEST(ResourceManagerTest, PowerRegeneration_CalculatesCorrectly)
```

---

## 3. Integration Testing Strategy

### 3.1 ClassAI → BotAI Integration

**Objective:** Verify ClassAI specializations integrate seamlessly with BotAI coordinator.

**Test Scenarios:**
1. **Combat Entry:** BotAI detects combat, calls ClassAI `OnCombatUpdate()`
2. **State Transitions:** Combat → Idle → Following state changes
3. **Target Coordination:** BotAI provides target, ClassAI executes rotation
4. **Resource Sharing:** ClassAI accesses BotAI values cache
5. **Event Handling:** Combat events routed to ClassAI handlers

**Integration Test Pattern:**
```cpp
TEST_F(PriestIntegrationTest, CombatEntry_BotAttacked_ActivatesCombatRotation) {
    // Arrange: Create bot with DisciplinePriest AI
    auto bot = SpawnTestBot(CLASS_PRIEST, SPEC_DISCIPLINE);
    auto enemy = SpawnTestEnemy();

    // Act: Enemy attacks bot
    enemy->Attack(bot);
    bot->UpdateAI(100ms);

    // Assert: Discipline rotation activated
    EXPECT_COMBAT_STATE(bot, BotAIState::COMBAT);
    EXPECT_SPELL_CAST(bot, SHADOW_WORD_PAIN);
    EXPECT_TARGET(bot, enemy);
}
```

### 3.2 Combat Behavior Integration

**Rotation Coordinator Tests:**
- Spell priority system execution
- GCD management across casts
- Interrupt coordination with InterruptCoordinator
- Cooldown coordination with CooldownManager

**Validation:**
- No rotation stalls (>2 seconds idle in combat)
- GCD respected (1.5s standard, 1.0s hasted)
- Cooldown usage within optimal windows
- Resource starvation handled gracefully

### 3.3 Group Coordination Integration

**Healer Group Tests:**
- Discipline: Shield tank before pulls, heal group
- Holy: AoE healing on group damage
- Shadow: DPS rotation with minimal healing

**DPS Group Tests:**
- Follow assist target
- Execute correct DPS rotation
- Manage threat appropriately
- Use utility spells (buffs, dispels)

**Validation Metrics:**
- Group member death rate: <5% in normal dungeons
- Overhealing: <30% (healer efficiency)
- DPS variance: ±10% of solo DPS
- Buff uptime: >95%

---

## 4. Regression Testing Strategy

### 4.1 Baseline Capture System

**Pre-Refactor Baseline Capture:**
```cpp
class BaselineCaptureRunner {
public:
    struct BaselineMetrics {
        // Combat Effectiveness
        float averageDPS;
        float averageHPS;
        float averageThreatGeneration;
        float survivabilityRating;

        // Performance
        float averageCPUUsage;
        uint64 averageMemoryUsage;
        float averageUpdateTime;

        // Behavior
        uint32 rotationDecisions;
        uint32 spellCastAttempts;
        uint32 successfulCasts;
        float castSuccessRate;

        // Timestamps
        std::chrono::system_clock::time_point captureTime;
        std::string gitCommitHash;
    };

    BaselineMetrics CaptureBaseline(Class classType, Specialization spec);
    void SaveBaseline(const BaselineMetrics& metrics, const std::string& filepath);
};
```

**Baseline Scenarios:**
1. **Solo Combat:** 100 mob kills (level-appropriate)
2. **Dungeon Run:** Complete 5-man dungeon
3. **Raid Encounter:** 10-bot raid boss kill
4. **Sustained Combat:** 10-minute combat session
5. **Load Test:** 1000 bots spawned simultaneously

**Captured Metrics:**
- **Combat:** DPS, HPS, survivability, spell usage
- **Performance:** CPU%, memory MB, update time µs
- **Behavior:** Decision counts, cast success rates
- **Stability:** Crashes, errors, warnings

### 4.2 Comparison System

**Comparison Algorithm:**
```cpp
struct ComparisonResult {
    enum class Verdict {
        PASS,           // Within acceptable tolerance
        WARNING,        // Degradation detected, manual review
        FAIL            // Regression exceeds tolerance
    };

    Verdict combatEffectiveness;    // ±5% tolerance
    Verdict performance;             // +30% improvement target
    Verdict behavior;                // 100% consistency required

    std::string failureReason;
    std::map<std::string, float> metricDeltas;
};
```

**Tolerance Thresholds:**
| Metric Category | Tolerance | Action on Violation |
|----------------|-----------|---------------------|
| DPS Output | ±5% | FAIL - Block merge |
| HPS Output | ±5% | FAIL - Block merge |
| Survivability | -10% | FAIL - Block merge |
| CPU Usage | +30% improvement goal | WARNING - Investigate |
| Memory Usage | +20% | WARNING - Check leaks |
| Update Time | -30% improvement goal | PASS - Document |
| Cast Success Rate | -2% | FAIL - Fix bugs |
| Behavioral Consistency | 100% | FAIL - Logic error |

### 4.3 Automated Regression Detection

**GitHub Actions Workflow:**
```yaml
name: Phase3 Regression Tests

on:
  pull_request:
    paths:
      - 'src/modules/Playerbot/AI/ClassAI/**'
      - 'src/modules/Playerbot/AI/BotAI.cpp'

jobs:
  regression-tests:
    runs-on: ubuntu-latest
    timeout-minutes: 120

    steps:
      - name: Checkout code
        uses: actions/checkout@v3

      - name: Build PlayerBot module
        run: |
          cmake -DBUILD_PLAYERBOT_TESTS=ON ..
          make -j$(nproc) playerbot_tests

      - name: Run regression test suite
        run: |
          ./playerbot_tests --gtest_filter="*Regression*"

      - name: Compare against baseline
        run: |
          ./phase3_comparison_tool \
            --baseline=baseline/phase3_baseline.json \
            --current=test_results/current_metrics.json \
            --tolerance=config/regression_tolerance.json

      - name: Generate regression report
        run: |
          ./generate_regression_report \
            --output=reports/regression_report.html

      - name: Upload report artifact
        uses: actions/upload-artifact@v3
        with:
          name: regression-report
          path: reports/regression_report.html

      - name: Fail if regression detected
        run: |
          if [ -f "reports/REGRESSION_DETECTED" ]; then
            echo "::error::Regression detected! Review report."
            exit 1
          fi
```

---

## 5. Load & Stress Testing Strategy

### 5.1 Scalability Test Progression

**Test Ladder:**
1. **100 Bots:** Baseline performance profiling
2. **500 Bots:** Typical server load
3. **1000 Bots:** High load scenario
4. **2500 Bots:** Stress scenario
5. **5000 Bots:** Maximum capacity target

**Per-Level Validation:**
- Server remains responsive (TPS >19.5)
- No memory leaks (stable over 1 hour)
- No deadlocks or race conditions
- Graceful degradation under load

### 5.2 Load Test Implementation

```cpp
TEST(Phase3LoadTest, Load_5000Bots_SystemStable) {
    // Arrange: Configure test environment
    TestEnvironment env;
    env.SetMaxBots(5000);
    env.SetTestDuration(std::chrono::hours(1));

    // Act: Spawn bots progressively
    std::vector<TestBot*> bots;
    for (int i = 0; i < 5000; i += 100) {
        bots.push_back(env.SpawnTestBot(
            RandomClass(), RandomSpec(), RandomLevel(70, 80)
        ));

        if (i % 1000 == 0) {
            // Checkpoint: Verify stability
            ASSERT_SERVER_RESPONSIVE(env);
            ASSERT_NO_MEMORY_LEAKS(env);
        }
    }

    // Monitor: Track metrics over duration
    auto metrics = env.MonitorMetrics(std::chrono::hours(1));

    // Assert: Performance criteria met
    EXPECT_LT(metrics.avgCPUUsage, 80.0f);
    EXPECT_LT(metrics.avgMemoryPerBot, 10 * 1024 * 1024); // 10MB
    EXPECT_GT(metrics.avgTPS, 19.5f);
    EXPECT_EQ(metrics.crashes, 0);
    EXPECT_EQ(metrics.deadlocks, 0);
}
```

### 5.3 Memory Leak Detection

**Valgrind Integration:**
```bash
# Run tests under Valgrind memcheck
valgrind --leak-check=full \
         --show-leak-kinds=all \
         --track-origins=yes \
         --log-file=valgrind_phase3.log \
         ./playerbot_tests --gtest_filter="*LoadTest*"
```

**AddressSanitizer Build:**
```cmake
# CMake configuration
option(PLAYERBOT_TESTS_ASAN "Build with AddressSanitizer" ON)
if(PLAYERBOT_TESTS_ASAN)
    target_compile_options(playerbot_tests PRIVATE -fsanitize=address)
    target_link_options(playerbot_tests PRIVATE -fsanitize=address)
endif()
```

**Leak Detection Criteria:**
- Zero "definitely lost" memory
- Zero "possibly lost" memory
- <1KB "still reachable" (static allocations acceptable)
- Zero use-after-free errors
- Zero buffer overflows

### 5.4 Thread Safety Validation

**ThreadSanitizer Integration:**
```cmake
option(PLAYERBOT_TESTS_TSAN "Build with ThreadSanitizer" ON)
if(PLAYERBOT_TESTS_TSAN)
    target_compile_options(playerbot_tests PRIVATE -fsanitize=thread)
    target_link_options(playerbot_tests PRIVATE -fsanitize=thread)
endif()
```

**Concurrency Stress Tests:**
```cpp
TEST(Phase3ThreadSafety, ConcurrentClassAIUpdates_NoRaceConditions) {
    // Create 100 bots with different ClassAI instances
    std::vector<std::thread> threads;
    std::atomic<uint32> raceConditionsDetected{0};

    for (int i = 0; i < 100; ++i) {
        threads.emplace_back([&, i]() {
            auto bot = CreateTestBot(CLASS_PRIEST);

            // Hammer ClassAI with updates
            for (int j = 0; j < 10000; ++j) {
                bot->GetAI()->OnCombatUpdate(100);
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    // ThreadSanitizer will detect race conditions automatically
    EXPECT_EQ(raceConditionsDetected.load(), 0);
}
```

---

## 6. Performance Benchmarking Strategy

### 6.1 Microbenchmarks

**Google Benchmark Integration:**
```cpp
#include <benchmark/benchmark.h>

// Benchmark rotation execution time
static void BM_DisciplineRotation(benchmark::State& state) {
    auto disc = CreateDisciplineSpec();
    auto target = CreateMockEnemy();

    for (auto _ : state) {
        disc->ExecuteRotation(target);
    }

    // Report iterations per second
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_DisciplineRotation);

// Benchmark resource calculations
static void BM_ResourceCalculation(benchmark::State& state) {
    auto resMgr = CreateResourceManager();

    for (auto _ : state) {
        resMgr->HasEnoughPower(POWER_MANA, 1000);
        resMgr->PredictPowerAfterCast(FLASH_HEAL_SPELL_ID);
    }
}
BENCHMARK(BM_ResourceCalculation);

BENCHMARK_MAIN();
```

**Performance Targets:**
| Operation | Current (Pre-Refactor) | Target (Post-Refactor) | Improvement |
|-----------|----------------------|----------------------|-------------|
| Rotation Execution | 80µs | <50µs | >37% |
| Resource Calculation | 5µs | <3µs | >40% |
| Target Selection | 20µs | <10µs | >50% |
| Cooldown Check | 2µs | <1µs | >50% |
| Overall AI Update | 200µs | <100µs | >50% |

### 6.2 Performance Profiling

**CPU Profiling (perf):**
```bash
# Profile 1000-bot load test
perf record -g --call-graph dwarf \
    ./playerbot_tests --gtest_filter="*Load_1000Bots*"

# Generate flamegraph
perf script | \
    stackcollapse-perf.pl | \
    flamegraph.pl > phase3_profile.svg
```

**Memory Profiling (Massif):**
```bash
# Profile memory usage over time
valgrind --tool=massif \
         --massif-out-file=massif_phase3.out \
         ./playerbot_tests --gtest_filter="*Load_5000Bots*"

# Visualize memory growth
ms_print massif_phase3.out > massif_report.txt
```

**Performance Report Generation:**
```cpp
class PerformanceReporter {
public:
    struct Report {
        std::map<std::string, float> benchmarkResults;  // µs per operation
        std::map<std::string, float> cpuHotspots;       // % CPU time
        std::map<std::string, uint64> memoryUsage;      // bytes

        float overallImprovement;  // % improvement vs baseline
        std::vector<std::string> recommendations;
    };

    Report GenerateReport(const BenchmarkResults& results);
    void ExportHTML(const Report& report, const std::string& filepath);
    void ExportJSON(const Report& report, const std::string& filepath);
};
```

---

## 7. Manual Testing Strategy

### 7.1 Test Scenario Playbook

**Scenario 1: Solo Leveling (1-80)**
- **Objective:** Verify all specializations level effectively
- **Duration:** 2-4 hours per spec (accelerated XP rates)
- **Validation:**
  - Bot completes quests autonomously
  - Combat effectiveness at all level ranges
  - Resource management (mana, cooldowns)
  - Survivability (death count <5 per 10 levels)
  - No stuck/pathing issues

**Scenario 2: 5-Man Dungeon Runs**
- **Objective:** Verify group coordination and role performance
- **Dungeons:** Ragefire Chasm, Deadmines, Wailing Caverns, Shadowfang Keep
- **Party Composition:** 1 Tank, 1 Healer, 3 DPS (mixed classes)
- **Validation:**
  - Healer keeps group alive (death rate <10%)
  - DPS follows assist target
  - Proper buff/debuff application
  - Boss mechanic handling
  - Loot distribution works correctly

**Scenario 3: Raid Scenarios (10+ Bots)**
- **Objective:** Verify scalability and raid coordination
- **Encounters:** Molten Core, Onyxia's Lair
- **Validation:**
  - Raid buffs maintained
  - Healing assignments respected
  - DPS optimization across 10+ targets
  - Threat management
  - Encounter-specific mechanics

**Scenario 4: PvP Battlegrounds**
- **Objective:** Verify PvP behavior and target prioritization
- **Battlegrounds:** Warsong Gulch, Arathi Basin
- **Validation:**
  - Target prioritization (healers > DPS > tanks)
  - Use of CC and utility spells
  - Flag/objective participation
  - Survivability under player-like burst damage

### 7.2 Validation Checklists

**Combat Effectiveness Checklist:**
- [ ] All core rotation spells used appropriately
- [ ] Cooldowns used optimally (within 80% uptime window)
- [ ] Resource management prevents OOM
- [ ] Target selection prioritizes correctly
- [ ] Interrupt coordination works (no duplicate interrupts)
- [ ] Defensive cooldowns triggered on low health
- [ ] Movement maintains optimal positioning (range/melee)
- [ ] No rotation stalls (>2 seconds idle)

**Behavior Consistency Checklist:**
- [ ] Specialization behavior matches pre-refactor
- [ ] Group role performance unchanged (tank/heal/DPS)
- [ ] Buff application frequency consistent
- [ ] Dispel/cleanse priorities maintained
- [ ] Aggro management behavior identical
- [ ] Death recovery process unchanged
- [ ] Quest pickup/completion logic preserved

**Performance Checklist:**
- [ ] CPU usage per bot <0.1% (5000 bot scenario)
- [ ] Memory usage per bot <10MB
- [ ] AI update time <100µs per bot
- [ ] Server TPS maintains >19.5
- [ ] No frame time spikes (>50ms)
- [ ] Network bandwidth usage unchanged
- [ ] Database query rate unchanged

---

## 8. Continuous Integration Strategy

### 8.1 GitHub Actions Workflow

**Pull Request Trigger:**
```yaml
name: Phase 3 ClassAI Tests

on:
  pull_request:
    paths:
      - 'src/modules/Playerbot/AI/ClassAI/**'
      - 'src/modules/Playerbot/AI/BotAI.cpp'
      - 'src/modules/Playerbot/Tests/Phase3/**'

jobs:
  unit-tests:
    name: Unit Tests
    runs-on: ubuntu-latest
    timeout-minutes: 30
    steps:
      - uses: actions/checkout@v3
      - name: Build tests
        run: |
          cmake -DBUILD_PLAYERBOT_TESTS=ON -DCMAKE_BUILD_TYPE=Debug ..
          make -j$(nproc) playerbot_tests
      - name: Run unit tests
        run: ./playerbot_tests --gtest_filter="*Phase3Unit*" --gtest_output=xml:unit_results.xml
      - name: Upload results
        uses: actions/upload-artifact@v3
        with:
          name: unit-test-results
          path: unit_results.xml

  integration-tests:
    name: Integration Tests
    runs-on: ubuntu-latest
    timeout-minutes: 60
    steps:
      - uses: actions/checkout@v3
      - name: Build tests
        run: |
          cmake -DBUILD_PLAYERBOT_TESTS=ON -DCMAKE_BUILD_TYPE=RelWithDebInfo ..
          make -j$(nproc) playerbot_tests
      - name: Run integration tests
        run: ./playerbot_tests --gtest_filter="*Phase3Integration*"
      - name: Upload results
        uses: actions/upload-artifact@v3
        with:
          name: integration-test-results
          path: integration_results.xml

  regression-tests:
    name: Regression Tests
    runs-on: ubuntu-latest
    timeout-minutes: 90
    steps:
      - uses: actions/checkout@v3
      - name: Download baseline metrics
        run: |
          wget https://ci.trinitycore.org/baselines/phase3_baseline.json
      - name: Build tests
        run: |
          cmake -DBUILD_PLAYERBOT_TESTS=ON -DCMAKE_BUILD_TYPE=Release ..
          make -j$(nproc) playerbot_tests
      - name: Run regression tests
        run: ./playerbot_tests --gtest_filter="*Phase3Regression*"
      - name: Compare metrics
        run: |
          ./tools/compare_metrics.py \
            --baseline=phase3_baseline.json \
            --current=test_results.json \
            --tolerance=5
      - name: Generate report
        run: ./tools/generate_report.py --output=regression_report.html
      - name: Upload report
        uses: actions/upload-artifact@v3
        with:
          name: regression-report
          path: regression_report.html
      - name: Check for regressions
        run: |
          if [ -f "REGRESSION_DETECTED" ]; then
            echo "::error::Performance regression detected!"
            exit 1
          fi

  load-tests:
    name: Load Tests
    runs-on: ubuntu-latest-4core
    timeout-minutes: 120
    steps:
      - uses: actions/checkout@v3
      - name: Build tests
        run: |
          cmake -DBUILD_PLAYERBOT_TESTS=ON \
                -DCMAKE_BUILD_TYPE=Release \
                -DPLAYERBOT_TESTS_ASAN=OFF \
                ..
          make -j$(nproc) playerbot_tests
      - name: Run 1000-bot load test
        run: |
          ./playerbot_tests --gtest_filter="*Load_1000Bots*" \
            --gtest_also_run_disabled_tests
      - name: Check stability
        run: |
          if grep -q "CRASH\|DEADLOCK\|LEAK" test_output.log; then
            echo "::error::Stability issue detected!"
            exit 1
          fi

  performance-benchmarks:
    name: Performance Benchmarks
    runs-on: ubuntu-latest
    timeout-minutes: 30
    steps:
      - uses: actions/checkout@v3
      - name: Build benchmarks
        run: |
          cmake -DBUILD_PLAYERBOT_TESTS=ON \
                -DCMAKE_BUILD_TYPE=Release \
                ..
          make -j$(nproc) playerbot_benchmarks
      - name: Run benchmarks
        run: ./playerbot_benchmarks --benchmark_format=json --benchmark_out=benchmarks.json
      - name: Compare with baseline
        run: |
          ./tools/compare_benchmarks.py \
            --baseline=baseline_benchmarks.json \
            --current=benchmarks.json
      - name: Upload results
        uses: actions/upload-artifact@v3
        with:
          name: benchmark-results
          path: benchmarks.json
```

### 8.2 Quality Gates

**Merge Criteria (ALL must pass):**
1. ✅ All unit tests pass (0 failures)
2. ✅ All integration tests pass (0 failures)
3. ✅ Regression tests within tolerance (±5%)
4. ✅ Performance benchmarks meet targets (>30% improvement)
5. ✅ Load test stability verified (1000 bots, 1 hour)
6. ✅ Memory leak checks clean (0 leaks)
7. ✅ Thread safety validation clean (0 race conditions)
8. ✅ Code coverage >80% (>95% for critical paths)
9. ✅ Manual test scenarios validated (sign-off required)
10. ✅ Code review approved (2+ reviewers)

**Automatic Rejection Criteria:**
- Any test crashes or hangs
- Memory leaks detected
- Race conditions detected
- Performance regression >5%
- Combat effectiveness regression >2%
- Code coverage below threshold

---

## 9. Test Data Generation

### 9.1 Synthetic Test Data

**Spell Database Generation:**
```cpp
class TestSpellDatabase {
public:
    // Generate complete spell data for class
    std::vector<SpellInfo*> GenerateSpellsForClass(Class classType);

    // Generate talent configuration
    TalentSpec GenerateTalentSpec(Class classType, Specialization spec);

    // Generate equipment sets
    EquipmentSet GenerateGearForLevel(uint32 level, Class classType);
};
```

**Scenario Definition (JSON):**
```json
{
  "scenario_id": "solo_leveling_priest_discipline",
  "class": "PRIEST",
  "specialization": "DISCIPLINE",
  "start_level": 1,
  "end_level": 80,
  "test_duration_hours": 2,
  "validation_checkpoints": [
    {"level": 10, "expected_deaths": 0, "expected_quests_completed": 15},
    {"level": 20, "expected_deaths": 1, "expected_quests_completed": 30},
    {"level": 40, "expected_deaths": 2, "expected_quests_completed": 60},
    {"level": 60, "expected_deaths": 3, "expected_quests_completed": 90},
    {"level": 80, "expected_deaths": 5, "expected_quests_completed": 120}
  ],
  "performance_criteria": {
    "max_cpu_percent": 0.1,
    "max_memory_mb": 10,
    "max_update_time_us": 100
  }
}
```

### 9.2 Baseline Data Capture

**Pre-Refactor Baseline Script:**
```bash
#!/bin/bash
# capture_phase3_baseline.sh

echo "=== Phase 3 Baseline Capture ==="
echo "Capturing pre-refactor metrics for comparison..."

# Build current (pre-refactor) code
git checkout playerbot-dev
cmake -DBUILD_PLAYERBOT_TESTS=ON -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc) playerbot_tests

# Capture baseline metrics
echo "Running baseline capture tests..."
./playerbot_tests --gtest_filter="*BaselineCapture*" \
    --gtest_output=json:baseline_results.json

# Generate baseline report
echo "Generating baseline report..."
./tools/generate_baseline_report.py \
    --input=baseline_results.json \
    --output=baseline/phase3_baseline_$(date +%Y%m%d).json

echo "Baseline capture complete!"
echo "Baseline saved to: baseline/phase3_baseline_$(date +%Y%m%d).json"
```

---

## 10. Deliverables Checklist

### 10.1 Code Deliverables
- [ ] All unit test files implemented
- [ ] All integration test files implemented
- [ ] All regression test files implemented
- [ ] All load test files implemented
- [ ] Mock object library complete
- [ ] Test utilities library complete
- [ ] Baseline capture tool implemented
- [ ] Comparison tool implemented
- [ ] Performance benchmarks implemented
- [ ] CI/CD pipeline configured

### 10.2 Documentation Deliverables
- [ ] Test framework architecture documented
- [ ] Unit test template guide
- [ ] Integration test guide
- [ ] Regression testing methodology
- [ ] Load testing procedures
- [ ] Manual test scenario playbook
- [ ] Validation checklist templates
- [ ] CI/CD pipeline documentation
- [ ] Performance benchmarking guide
- [ ] Troubleshooting guide

### 10.3 Validation Deliverables
- [ ] Pre-refactor baseline captured
- [ ] Post-refactor metrics captured
- [ ] Regression comparison report generated
- [ ] Performance improvement report generated
- [ ] Manual test results documented
- [ ] Code coverage report generated
- [ ] Memory leak analysis report
- [ ] Thread safety validation report
- [ ] Final validation sign-off document

---

## 11. Success Criteria

### 11.1 Functional Requirements (MUST PASS)
1. ✅ **Zero Combat Regression:** All specializations perform within ±5% of baseline
2. ✅ **100% Behavioral Consistency:** No changes to observable bot behavior
3. ✅ **Zero Crashes:** 24-hour stability test with 1000 bots
4. ✅ **Zero Memory Leaks:** Valgrind clean over 24-hour test
5. ✅ **Zero Race Conditions:** ThreadSanitizer clean

### 12.0 Performance Requirements (TARGET)
1. 🎯 **>30% CPU Reduction:** Measured per bot in 5000-bot scenario
2. 🎯 **>50% Faster AI Updates:** <100µs per OnCombatUpdate() call
3. 🎯 **Memory Stable:** <10MB per bot, stable over time
4. 🎯 **Scalability Proven:** 5000 bots stable for 1+ hour
5. 🎯 **Code Quality:** >80% test coverage, >95% critical path coverage

### 11.3 Quality Requirements (MUST PASS)
1. ✅ **All Tests Pass:** 0 test failures in CI/CD pipeline
2. ✅ **Code Review Approved:** 2+ reviewers sign-off
3. ✅ **Documentation Complete:** All test documentation finalized
4. ✅ **Manual Validation:** All manual scenarios validated
5. ✅ **Performance Benchmarks:** All benchmarks meet or exceed targets

---

## 12. Timeline & Milestones

### Phase 3A: Test Framework Setup (Week 1)
- Day 1-2: Create test directory structure
- Day 3-4: Implement mock object library
- Day 5-7: Implement test utilities and helpers

### Phase 3B: Unit Test Implementation (Week 2-3)
- Week 2: Priest, Mage, Shaman specialization unit tests
- Week 3: Coordinator and base class unit tests

### Phase 3C: Integration Test Implementation (Week 4)
- Day 1-2: ClassAI → BotAI integration tests
- Day 3-4: Combat behavior integration tests
- Day 5-7: Group coordination integration tests

### Phase 3D: Regression & Performance Testing (Week 5)
- Day 1-2: Baseline capture system implementation
- Day 3-4: Comparison system implementation
- Day 5-7: Performance benchmarking implementation

### Phase 3E: Load & Stress Testing (Week 6)
- Day 1-3: Load test implementation (100, 1000, 5000 bots)
- Day 4-5: Memory leak detection setup
- Day 6-7: Thread safety validation

### Phase 3F: CI/CD Integration (Week 7)
- Day 1-3: GitHub Actions workflow configuration
- Day 4-5: Jenkins pipeline setup (if applicable)
- Day 6-7: Automated reporting configuration

### Phase 3G: Manual Testing & Validation (Week 8)
- Day 1-2: Solo leveling validation
- Day 3-4: Dungeon run validation
- Day 5-6: Raid scenario validation
- Day 7: Final sign-off and documentation

---

## 13. Risk Mitigation

### 13.1 Identified Risks

**Risk 1: Flaky Tests**
- **Probability:** Medium
- **Impact:** High (blocks CI/CD)
- **Mitigation:**
  - Implement retry logic for timing-sensitive tests
  - Use deterministic time simulation where possible
  - Avoid absolute timing assertions, use ranges

**Risk 2: Baseline Data Drift**
- **Probability:** High
- **Impact:** Medium (false regressions)
- **Mitigation:**
  - Capture multiple baselines from clean builds
  - Version baselines with git commit hashes
  - Re-baseline monthly or after major core changes

**Risk 3: Test Environment Variance**
- **Probability:** Medium
- **Impact:** Medium (inconsistent results)
- **Mitigation:**
  - Use Docker containers for consistent environment
  - Pin dependency versions
  - Document required system specifications

**Risk 4: Insufficient Coverage**
- **Probability:** Low
- **Impact:** High (regressions slip through)
- **Mitigation:**
  - Enforce >80% code coverage in CI/CD
  - Manual code review for untested paths
  - Prioritize critical path coverage >95%

### 13.2 Rollback Plan

If Phase 3 refactoring fails validation:
1. **Immediate:** Revert refactored code, restore baseline
2. **Short-term:** Analyze test failures, identify root causes
3. **Mid-term:** Fix identified issues in feature branch
4. **Re-test:** Re-run full test suite before re-attempting merge
5. **Documentation:** Document lessons learned

---

## 14. Appendices

### Appendix A: Test Naming Conventions
```
Format: [Category]_[Component]_[Scenario]_[ExpectedBehavior]

Examples:
- Unit_DisciplineSpec_LowHealthAlly_CastsFlashHeal
- Integration_PriestAI_CombatEntry_ActivatesRotation
- Regression_MageAI_DPSOutput_WithinTolerance
- Performance_RotationExecution_Under50Microseconds
- Load_5000Bots_SystemStable
```

### Appendix B: Assertion Macro Library
```cpp
// Combat effectiveness assertions
#define EXPECT_SPELL_CAST(bot, spellId) \
    EXPECT_TRUE(HasCastSpell(bot, spellId)) << "Expected spell cast: " << spellId

#define EXPECT_SPELL_CAST_WITHIN_MS(result, milliseconds) \
    EXPECT_LE(result.executionTime, milliseconds) << "Spell cast took too long"

#define EXPECT_TARGET(bot, expectedTarget) \
    EXPECT_EQ(bot->GetTarget(), expectedTarget->GetGUID())

#define EXPECT_COMBAT_STATE(bot, expectedState) \
    EXPECT_EQ(bot->GetAI()->GetAIState(), expectedState)

// Performance assertions
#define EXPECT_UPDATE_TIME_UNDER_MICROS(actualMicros, limitMicros) \
    EXPECT_LE(actualMicros, limitMicros) \
    << "Update took " << actualMicros << "µs, expected <" << limitMicros << "µs"

#define EXPECT_MEMORY_USAGE_UNDER_MB(actualBytes, limitMB) \
    EXPECT_LE(actualBytes, limitMB * 1024 * 1024) \
    << "Memory usage " << (actualBytes / 1024 / 1024) << "MB, expected <" << limitMB << "MB"

// Behavioral consistency assertions
#define EXPECT_ROTATION_DECISION(bot, expectedSpell) \
    EXPECT_EQ(GetNextRotationSpell(bot), expectedSpell)

#define EXPECT_COOLDOWN_USAGE_OPTIMAL(bot, cooldownSpell) \
    EXPECT_TRUE(IsCooldownUsedOptimally(bot, cooldownSpell))
```

### Appendix C: Performance Profiling Commands
```bash
# CPU profiling with perf
perf record -g --call-graph dwarf -F 999 \
    ./playerbot_tests --gtest_filter="*Phase3*"
perf report -g 'graph,0.5,caller'

# Memory profiling with Massif
valgrind --tool=massif --massif-out-file=massif.out \
    ./playerbot_tests --gtest_filter="*Phase3*"
ms_print massif.out

# Heap profiling with Heaptrack
heaptrack ./playerbot_tests --gtest_filter="*Phase3*"
heaptrack_gui heaptrack.playerbot_tests.*.gz

# Cache profiling with cachegrind
valgrind --tool=cachegrind --cachegrind-out-file=cachegrind.out \
    ./playerbot_tests --gtest_filter="*Phase3*"
cg_annotate cachegrind.out

# Flamegraph generation
perf script | stackcollapse-perf.pl | flamegraph.pl > phase3_flamegraph.svg
```

### Appendix D: Continuous Integration URLs
- **GitHub Actions:** https://github.com/TrinityCore/TrinityCore/actions
- **Test Results Dashboard:** (TBD)
- **Performance Metrics Dashboard:** (TBD)
- **Coverage Reports:** (TBD)
- **Benchmark History:** (TBD)

---

**Document Version:** 1.0
**Last Updated:** 2025-10-17
**Author:** Test Automation Engineer (Claude Code)
**Review Status:** Ready for Implementation
**Approval Required:** Senior Developer, QA Lead, Project Manager

---

**Next Steps:**
1. Review this architecture document with stakeholders
2. Obtain approval to proceed with implementation
3. Begin Phase 3A: Test Framework Setup
4. Schedule weekly progress reviews
5. Deliver comprehensive test suite within 8-week timeline
