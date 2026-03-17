# Phase 5: Testing & Validation - Claude Code Start Prompt

Copy this entire prompt into Claude Code to begin Phase 5 implementation:

---

## PROMPT START

Phases 1-4 are complete. Now implement Phase 5: Testing & Validation (Enterprise Grade).

### Context
- **Repository**: `C:\TrinityBots\TrinityCore`
- **Detailed Plan**: Read `.claude/prompts/PHASE5_TESTING_VALIDATION.md`
- **Master Plan**: `.claude/plans/COMBAT_REFACTORING_MASTER_PLAN.md`

### Phase 5 Goal
Ensure all refactored systems meet enterprise quality standards through comprehensive testing, performance validation, and documentation.

### Quality Gates

| Gate | Target |
|------|--------|
| Unit Test Coverage | ≥80% |
| Performance | ≤100% baseline (no regression) |
| Memory Leaks | 0 in 24h stress test |
| Scalability | 5,000+ bots supported |

---

## Task 5.1: Test Infrastructure (16h)

**Files to CREATE** in `src/modules/Playerbot/Tests/`:

### 5.1.1: Helpers/TestHelpers.h
Test framework with macros:
```cpp
#define TEST_CASE(name) ...
#define ASSERT_TRUE(condition) ...
#define ASSERT_EQ(expected, actual) ...
#define ASSERT_NEAR(expected, actual, tolerance) ...

class TestRegistry { void Register(); void RunAll(); void PrintSummary(); };
class Benchmark { void Run(Func); double GetAverageTimeUs(); };
```

### 5.1.2: Mocks/MockPlayer.h
```cpp
class MockPlayer {
    ObjectGuid GetGUID(); void SetHealth(uint32); void SetClass(uint8);
    void AddAura(uint32 spellId, uint8 stacks); bool HasAura(uint32);
    void SetSpellCooldown(uint32 spellId, uint32 readyTime);
};
MockPlayer CreateTank(); MockPlayer CreateHealer(); MockPlayer CreateMeleeDPS();
```

### 5.1.3: Mocks/MockGroup.h
```cpp
class MockGroup {
    void AddMember(MockPlayer*); size_t GetMemberCount();
    void AssignToSubGroup(ObjectGuid, uint8);
};
MockGroup CreateDungeonGroup();  // 1T 1H 3D
MockGroup CreateArenaTeam(uint8 size);  // 2, 3, or 5
MockGroup CreateRaidGroup(uint8 size);  // 10, 25, or 40
```

### 5.1.4: Fixtures/TestFixtures.h
```cpp
class EventSystemFixture : public TestFixture { CombatEventRouter* _router; };
class DungeonFixture : public TestFixture { MockGroup _group; MockPlayer* _tank; };
class ArenaFixture : public TestFixture { MockGroup _team; std::vector<MockPlayer> _enemies; };
class RaidFixture : public TestFixture { MockGroup _raid; std::vector<MockPlayer*> _tanks, _healers; };
```

---

## Task 5.2: Unit Tests (24h)

### 5.2.1: Unit/Events/CombatEventRouterTests.cpp
```cpp
TEST_CASE(CombatEventRouter_Subscribe_RegistersSubscriber)
TEST_CASE(CombatEventRouter_Dispatch_DeliversToSubscriber)
TEST_CASE(CombatEventRouter_Dispatch_FiltersByEventType)
TEST_CASE(CombatEventRouter_Unsubscribe_RemovesSubscriber)
TEST_CASE(CombatEventRouter_QueueEvent_ProcessesOnNextTick)
TEST_CASE(CombatEventRouter_MultipleSubscribers_AllReceiveEvent)
```

### 5.2.2: Unit/Coordinators/InterruptCoordinatorTests.cpp
```cpp
TEST_FIXTURE(DungeonFixture, InterruptCoordinator_AssignsInterrupter_OnSpellCastStart)
TEST_FIXTURE(DungeonFixture, InterruptCoordinator_TracksSuccessfulInterrupt)
TEST_FIXTURE(DungeonFixture, InterruptCoordinator_TracksMissedInterrupt)
TEST_FIXTURE(DungeonFixture, InterruptCoordinator_RespectsAssignmentPriority)
```

### 5.2.3: Unit/Raid/RaidTankCoordinatorTests.cpp
```cpp
TEST_FIXTURE(RaidFixture, RaidTankCoordinator_DetectsTankSwapNeed)
TEST_FIXTURE(RaidFixture, RaidTankCoordinator_ExecutesTankSwap)
TEST_FIXTURE(RaidFixture, RaidTankCoordinator_TracksDebuffExpiration)
TEST_FIXTURE(RaidFixture, RaidTankCoordinator_AssignsAddTanks)
```

### 5.2.4: Unit/Arena/CCChainManagerTests.cpp
```cpp
TEST_FIXTURE(ArenaFixture, CCChainManager_PlansCCChain_ConsideringDR)
TEST_FIXTURE(ArenaFixture, CCChainManager_RespectsFullDR)
TEST_FIXTURE(ArenaFixture, CCChainManager_CalculatesDRReduction)
TEST_FIXTURE(ArenaFixture, CCChainManager_CoordinatesMultipleCCers)
```

### 5.2.5: Additional Unit Test Files
- `Unit/Coordinators/ThreatCoordinatorTests.cpp`
- `Unit/Coordinators/CrowdControlManagerTests.cpp`
- `Unit/Dungeon/TrashPullManagerTests.cpp`
- `Unit/Dungeon/BossEncounterManagerTests.cpp`
- `Unit/Dungeon/MythicPlusManagerTests.cpp`
- `Unit/Arena/KillTargetManagerTests.cpp`
- `Unit/Arena/BurstCoordinatorTests.cpp`
- `Unit/Battleground/BGStrategyEngineTests.cpp`
- `Unit/Raid/RaidHealCoordinatorTests.cpp`
- `Unit/Raid/KitingManagerTests.cpp`
- `Unit/Raid/AddManagementSystemTests.cpp`

---

## Task 5.3: Integration Tests (20h)

### 5.3.1: Integration/CoordinatorInteractionTests.cpp
```cpp
TEST_FIXTURE(DungeonFixture, Integration_InterruptAndCC_Coordinate)
TEST_FIXTURE(RaidFixture, Integration_TankSwap_TriggersHealerResponse)
TEST_FIXTURE(RaidFixture, Integration_AddSpawn_AssignsTankAndDPS)
TEST_FIXTURE(ArenaFixture, Integration_BurstWindow_CoordinatesCooldowns)
```

### 5.3.2: Integration/EventSystemIntegrationTests.cpp
```cpp
TEST_CASE(Integration_AllCoordinators_ReceiveEvents)
TEST_CASE(Integration_EventFilter_RoutesToCorrectSubscribers)
TEST_CASE(Integration_QueuedEvents_ProcessInOrder)
```

### 5.3.3: Integration/[Context]ScenarioTests.cpp
Full scenario tests for each context:
- `DungeonScenarioTests.cpp` - Full dungeon run with trash + boss
- `ArenaScenarioTests.cpp` - Full arena match with burst + CC
- `BattlegroundScenarioTests.cpp` - Full BG with objectives
- `RaidScenarioTests.cpp` - Full boss encounter with tank swaps

---

## Task 5.4: Performance Tests (16h)

### 5.4.1: Performance/EventRouterBenchmarks.cpp
```cpp
TEST_CASE(Benchmark_EventRouter_DispatchPerformance)
// Target: <10μs per dispatch with 100 subscribers

TEST_CASE(Benchmark_EventRouter_HighSubscriberCount)
// Target: <100μs per dispatch with 1000 subscribers

TEST_CASE(Benchmark_EventRouter_QueueProcessing)
// Target: <10ms for 1000 events
```

### 5.4.2: Performance/ScalabilityTests.cpp
```cpp
TEST_CASE(Scalability_5000Bots_EventRouting)
// Target: Handle 5000+ events/sec with 5000 bots

TEST_CASE(Scalability_RaidCoordinator_40ManRaid)
// Target: <500μs per update cycle
```

### 5.4.3: Performance/MemoryLeakTests.cpp
```cpp
TEST_CASE(MemoryLeak_EventRouter_SubscribeUnsubscribe)
// Target: <1MB growth after 10000 iterations

TEST_CASE(MemoryLeak_EventQueue_LongRunning)
// Target: <1MB growth after 1 hour simulation
```

### 5.4.4: Performance/PerformanceBaseline.h
```cpp
struct PerformanceBaseline {
    static constexpr double EVENT_DISPATCH_MAX_US = 10.0;
    static constexpr double RAID_COORD_UPDATE_MAX_US = 500.0;
    static constexpr size_t MIN_SUPPORTED_BOTS = 5000;
    static constexpr size_t MAX_MEMORY_PER_BOT_KB = 100;
};
```

---

## Task 5.5: Documentation & Reports (8h)

### 5.5.1: TestMain.cpp
```cpp
int main(int argc, char** argv) {
    TestRegistry::Instance().RunAll();
    TestRegistry::Instance().PrintSummary();
    TestReportGenerator().GenerateHTML("test_report.html");
    return TestRegistry::Instance().GetFailedTests() > 0 ? 1 : 0;
}
```

### 5.5.2: Test Report Generator
- HTML report with pass/fail summary
- JUnit XML for CI integration
- Markdown for documentation
- Benchmark comparison charts

---

## File Structure

```
src/modules/Playerbot/Tests/
├── Unit/
│   ├── Events/ (3 files)
│   ├── Coordinators/ (4 files)
│   ├── Dungeon/ (4 files)
│   ├── Arena/ (4 files)
│   ├── Battleground/ (3 files)
│   └── Raid/ (5 files)
├── Integration/ (7 files)
├── Performance/ (5 files)
├── Mocks/ (7 files)
├── Fixtures/ (5 files)
├── Helpers/ (4 files)
└── TestMain.cpp
Total: ~47 files
```

---

## Success Criteria

### Coverage
- [ ] Event System: ≥90%
- [ ] InterruptCoordinator: ≥85%
- [ ] ThreatCoordinator: ≥85%
- [ ] CrowdControlManager: ≥85%
- [ ] DungeonCoordinator: ≥80%
- [ ] ArenaCoordinator: ≥80%
- [ ] BattlegroundCoordinator: ≥75%
- [ ] RaidCoordinator: ≥80%
- [ ] **Overall: ≥80%**

### Performance
- [ ] Event dispatch: <10μs
- [ ] Coordinator update: <100μs per bot
- [ ] Memory per bot: <100KB
- [ ] Supports 5,000+ bots
- [ ] CPU usage (5000 bots): <50%

### Quality
- [ ] All tests pass
- [ ] No memory leaks (24h test)
- [ ] No performance regression
- [ ] Test reports generated

---

## Instructions

1. Read detailed plan: `.claude/prompts/PHASE5_TESTING_VALIDATION.md`
2. Execute in order: 5.1 → 5.2 → 5.3 → 5.4 → 5.5
3. Infrastructure (5.1) must be complete before writing tests
4. Run tests after each component
5. Performance tests should establish baselines
6. Generate final report

### Critical Notes

1. **Test pyramid**: 60% unit, 30% integration, 10% E2E
2. **Mock everything**: Don't depend on TrinityCore internals in tests
3. **Benchmark early**: Establish baselines before optimizing
4. **Memory matters**: Track allocations in stress tests
5. **CI-ready**: Generate JUnit XML for automation

Start with Task 5.1 (Test Infrastructure).

---

## PROMPT END
