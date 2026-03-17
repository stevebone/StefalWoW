# Phase 5: Testing & Validation - Enterprise Grade Implementation Plan

**Version**: 1.0  
**Date**: 2026-01-26  
**Prerequisites**: Phase 1, 2, 3, 4 complete  
**Duration**: ~80 hours (2 weeks)

---

## Overview

Phase 5 ensures all refactored systems meet enterprise quality standards through comprehensive testing, performance validation, and documentation.

### Testing Pyramid

```
                    /\
                   /  \
                  / E2E \        (10%) - Full scenario tests
                 /--------\
                /Integration\    (30%) - Component interaction
               /--------------\
              /   Unit Tests    \ (60%) - Individual functions
             /--------------------\
```

### Quality Gates

| Gate | Requirement | Target |
|------|-------------|--------|
| Unit Test Coverage | Line coverage | ≥80% |
| Integration Tests | All coordinator combinations | 100% |
| Performance | No regression from baseline | ≤100% baseline |
| Memory | No leaks in 24h stress test | 0 leaks |
| Scalability | Support bot count | 5,000+ |

---

## Files to Create

```
src/modules/Playerbot/Tests/
├── Unit/
│   ├── Events/
│   │   ├── CombatEventTests.cpp
│   │   ├── CombatEventRouterTests.cpp
│   │   └── EventSubscriberTests.cpp
│   ├── Coordinators/
│   │   ├── InterruptCoordinatorTests.cpp
│   │   ├── ThreatCoordinatorTests.cpp
│   │   ├── CrowdControlManagerTests.cpp
│   │   └── BotThreatManagerTests.cpp
│   ├── Dungeon/
│   │   ├── DungeonCoordinatorTests.cpp
│   │   ├── TrashPullManagerTests.cpp
│   │   ├── BossEncounterManagerTests.cpp
│   │   └── MythicPlusManagerTests.cpp
│   ├── Arena/
│   │   ├── ArenaCoordinatorTests.cpp
│   │   ├── KillTargetManagerTests.cpp
│   │   ├── CCChainManagerTests.cpp
│   │   └── BurstCoordinatorTests.cpp
│   ├── Battleground/
│   │   ├── BattlegroundCoordinatorTests.cpp
│   │   ├── BGStrategyEngineTests.cpp
│   │   └── ObjectiveManagerTests.cpp
│   └── Raid/
│       ├── RaidCoordinatorTests.cpp
│       ├── RaidTankCoordinatorTests.cpp
│       ├── RaidHealCoordinatorTests.cpp
│       ├── KitingManagerTests.cpp
│       └── AddManagementSystemTests.cpp
├── Integration/
│   ├── EventSystemIntegrationTests.cpp
│   ├── CoordinatorInteractionTests.cpp
│   ├── DungeonScenarioTests.cpp
│   ├── ArenaScenarioTests.cpp
│   ├── BattlegroundScenarioTests.cpp
│   ├── RaidScenarioTests.cpp
│   └── CrossCoordinatorTests.cpp
├── Performance/
│   ├── EventRouterBenchmarks.cpp
│   ├── CoordinatorBenchmarks.cpp
│   ├── ScalabilityTests.cpp
│   ├── MemoryLeakTests.cpp
│   └── PerformanceBaseline.cpp
├── Mocks/
│   ├── MockPlayer.h
│   ├── MockGroup.h
│   ├── MockUnit.h
│   ├── MockSpellInfo.h
│   ├── MockAura.h
│   ├── MockBattleground.h
│   └── MockMap.h
├── Fixtures/
│   ├── TestFixtures.h
│   ├── DungeonFixtures.h
│   ├── ArenaFixtures.h
│   ├── RaidFixtures.h
│   └── EventFixtures.h
├── Helpers/
│   ├── TestHelpers.h
│   ├── TestHelpers.cpp
│   ├── AssertHelpers.h
│   └── BenchmarkHelpers.h
└── TestMain.cpp
```

---

## Task 5.1: Test Infrastructure Setup

**Priority**: P0 (Foundation)  
**Effort**: 16 hours  
**Dependencies**: Phase 4 complete

### 5.1.1: Create TestHelpers.h

```cpp
#pragma once

#include "ObjectGuid.h"
#include <vector>
#include <string>
#include <functional>
#include <chrono>

namespace Playerbot {
namespace Testing {

// ============================================================================
// TEST FRAMEWORK MACROS
// ============================================================================

#define TEST_CASE(name) \
    void Test_##name(); \
    static TestRegistrar registrar_##name(#name, Test_##name); \
    void Test_##name()

#define TEST_FIXTURE(fixture, name) \
    void Test_##fixture##_##name(fixture& f); \
    static TestRegistrar registrar_##fixture##_##name(#fixture "/" #name, \
        []() { fixture f; f.SetUp(); Test_##fixture##_##name(f); f.TearDown(); }); \
    void Test_##fixture##_##name(fixture& f)

#define ASSERT_TRUE(condition) \
    if (!(condition)) { \
        throw TestFailure(__FILE__, __LINE__, "ASSERT_TRUE failed: " #condition); \
    }

#define ASSERT_FALSE(condition) \
    if (condition) { \
        throw TestFailure(__FILE__, __LINE__, "ASSERT_FALSE failed: " #condition); \
    }

#define ASSERT_EQ(expected, actual) \
    if ((expected) != (actual)) { \
        throw TestFailure(__FILE__, __LINE__, \
            "ASSERT_EQ failed: expected " + ToString(expected) + ", got " + ToString(actual)); \
    }

#define ASSERT_NE(expected, actual) \
    if ((expected) == (actual)) { \
        throw TestFailure(__FILE__, __LINE__, "ASSERT_NE failed: values are equal"); \
    }

#define ASSERT_LT(a, b) \
    if (!((a) < (b))) { \
        throw TestFailure(__FILE__, __LINE__, "ASSERT_LT failed"); \
    }

#define ASSERT_LE(a, b) \
    if (!((a) <= (b))) { \
        throw TestFailure(__FILE__, __LINE__, "ASSERT_LE failed"); \
    }

#define ASSERT_GT(a, b) \
    if (!((a) > (b))) { \
        throw TestFailure(__FILE__, __LINE__, "ASSERT_GT failed"); \
    }

#define ASSERT_GE(a, b) \
    if (!((a) >= (b))) { \
        throw TestFailure(__FILE__, __LINE__, "ASSERT_GE failed"); \
    }

#define ASSERT_NEAR(expected, actual, tolerance) \
    if (std::abs((expected) - (actual)) > (tolerance)) { \
        throw TestFailure(__FILE__, __LINE__, "ASSERT_NEAR failed"); \
    }

#define ASSERT_NULL(ptr) \
    if ((ptr) != nullptr) { \
        throw TestFailure(__FILE__, __LINE__, "ASSERT_NULL failed: pointer is not null"); \
    }

#define ASSERT_NOT_NULL(ptr) \
    if ((ptr) == nullptr) { \
        throw TestFailure(__FILE__, __LINE__, "ASSERT_NOT_NULL failed: pointer is null"); \
    }

#define ASSERT_THROWS(expression, exceptionType) \
    { \
        bool caught = false; \
        try { expression; } \
        catch (const exceptionType&) { caught = true; } \
        catch (...) { } \
        if (!caught) { \
            throw TestFailure(__FILE__, __LINE__, "ASSERT_THROWS failed: expected " #exceptionType); \
        } \
    }

#define EXPECT_NO_THROW(expression) \
    try { expression; } \
    catch (const std::exception& e) { \
        throw TestFailure(__FILE__, __LINE__, \
            std::string("EXPECT_NO_THROW failed: ") + e.what()); \
    }

// ============================================================================
// TEST FAILURE
// ============================================================================

class TestFailure : public std::exception {
public:
    TestFailure(const char* file, int line, const std::string& message)
        : _file(file), _line(line), _message(message) {
        _fullMessage = std::string(file) + ":" + std::to_string(line) + " - " + message;
    }
    
    const char* what() const noexcept override { return _fullMessage.c_str(); }
    const char* File() const { return _file; }
    int Line() const { return _line; }
    const std::string& Message() const { return _message; }
    
private:
    const char* _file;
    int _line;
    std::string _message;
    std::string _fullMessage;
};

// ============================================================================
// TEST REGISTRY
// ============================================================================

using TestFunction = std::function<void()>;

struct TestInfo {
    std::string name;
    TestFunction function;
    bool passed = false;
    std::string failureMessage;
    double durationMs = 0.0;
};

class TestRegistry {
public:
    static TestRegistry& Instance();
    
    void Register(const std::string& name, TestFunction func);
    void RunAll();
    void RunByName(const std::string& pattern);
    void RunByTag(const std::string& tag);
    
    size_t GetTotalTests() const { return _tests.size(); }
    size_t GetPassedTests() const;
    size_t GetFailedTests() const;
    
    void PrintSummary() const;
    void GenerateReport(const std::string& filename) const;
    
private:
    std::vector<TestInfo> _tests;
};

class TestRegistrar {
public:
    TestRegistrar(const std::string& name, TestFunction func) {
        TestRegistry::Instance().Register(name, func);
    }
};

// ============================================================================
// TEST FIXTURE BASE
// ============================================================================

class TestFixture {
public:
    virtual ~TestFixture() = default;
    virtual void SetUp() {}
    virtual void TearDown() {}
};

// ============================================================================
// BENCHMARK HELPERS
// ============================================================================

class Benchmark {
public:
    Benchmark(const std::string& name, size_t iterations = 1000)
        : _name(name), _iterations(iterations) {}
    
    template<typename Func>
    void Run(Func&& func) {
        // Warmup
        for (size_t i = 0; i < _iterations / 10; ++i) {
            func();
        }
        
        // Actual benchmark
        auto start = std::chrono::high_resolution_clock::now();
        for (size_t i = 0; i < _iterations; ++i) {
            func();
        }
        auto end = std::chrono::high_resolution_clock::now();
        
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        _totalTimeUs = duration.count();
        _avgTimeUs = static_cast<double>(_totalTimeUs) / _iterations;
    }
    
    void PrintResults() const;
    double GetAverageTimeUs() const { return _avgTimeUs; }
    uint64_t GetTotalTimeUs() const { return _totalTimeUs; }
    
private:
    std::string _name;
    size_t _iterations;
    uint64_t _totalTimeUs = 0;
    double _avgTimeUs = 0.0;
};

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

template<typename T>
std::string ToString(const T& value) {
    if constexpr (std::is_same_v<T, bool>) {
        return value ? "true" : "false";
    } else if constexpr (std::is_arithmetic_v<T>) {
        return std::to_string(value);
    } else if constexpr (std::is_same_v<T, std::string>) {
        return value;
    } else if constexpr (std::is_same_v<T, ObjectGuid>) {
        return value.ToString();
    } else {
        return "<unknown>";
    }
}

// Generate random ObjectGuid for testing
ObjectGuid GenerateTestGuid();
ObjectGuid GeneratePlayerGuid();
ObjectGuid GenerateCreatureGuid();

// Time helpers
uint32 GetTestTime();
void AdvanceTestTime(uint32 ms);
void ResetTestTime();

// Random helpers
int RandomInt(int min, int max);
float RandomFloat(float min, float max);

} // namespace Testing
} // namespace Playerbot
```

### 5.1.2: Create Mock Classes

**MockPlayer.h**:
```cpp
#pragma once

#include "ObjectGuid.h"
#include <string>
#include <map>

namespace Playerbot {
namespace Testing {

class MockPlayer {
public:
    MockPlayer(ObjectGuid guid = GeneratePlayerGuid())
        : _guid(guid) {}
    
    // Identity
    ObjectGuid GetGUID() const { return _guid; }
    void SetName(const std::string& name) { _name = name; }
    std::string GetName() const { return _name; }
    
    // Class/Spec
    void SetClass(uint8 classId) { _class = classId; }
    uint8 GetClass() const { return _class; }
    void SetSpec(uint32 specId) { _specId = specId; }
    uint32 GetSpec() const { return _specId; }
    
    // Stats
    void SetHealth(uint32 health) { _health = health; }
    void SetMaxHealth(uint32 maxHealth) { _maxHealth = maxHealth; }
    uint32 GetHealth() const { return _health; }
    uint32 GetMaxHealth() const { return _maxHealth; }
    float GetHealthPct() const { return _maxHealth > 0 ? (float)_health / _maxHealth * 100.0f : 0.0f; }
    
    void SetPower(uint32 power) { _power = power; }
    void SetMaxPower(uint32 maxPower) { _maxPower = maxPower; }
    uint32 GetPower() const { return _power; }
    float GetPowerPct() const { return _maxPower > 0 ? (float)_power / _maxPower * 100.0f : 0.0f; }
    
    // Position
    void SetPosition(float x, float y, float z) { _x = x; _y = y; _z = z; }
    float GetPositionX() const { return _x; }
    float GetPositionY() const { return _y; }
    float GetPositionZ() const { return _z; }
    
    // Combat state
    void SetInCombat(bool inCombat) { _inCombat = inCombat; }
    bool IsInCombat() const { return _inCombat; }
    void SetAlive(bool alive) { _alive = alive; }
    bool IsAlive() const { return _alive; }
    
    // Cooldowns
    void SetSpellCooldown(uint32 spellId, uint32 readyTime);
    bool IsSpellReady(uint32 spellId, uint32 currentTime) const;
    uint32 GetSpellCooldownRemaining(uint32 spellId, uint32 currentTime) const;
    
    // Auras
    void AddAura(uint32 spellId, uint8 stacks = 1);
    void RemoveAura(uint32 spellId);
    bool HasAura(uint32 spellId) const;
    uint8 GetAuraStacks(uint32 spellId) const;
    
    // Group
    void SetGroup(void* group) { _group = group; }
    void* GetGroup() const { return _group; }
    void SetSubGroup(uint8 subGroup) { _subGroup = subGroup; }
    uint8 GetSubGroup() const { return _subGroup; }
    
private:
    ObjectGuid _guid;
    std::string _name = "TestPlayer";
    uint8 _class = 1;  // Warrior
    uint32 _specId = 0;
    
    uint32 _health = 100000;
    uint32 _maxHealth = 100000;
    uint32 _power = 100;
    uint32 _maxPower = 100;
    
    float _x = 0, _y = 0, _z = 0;
    
    bool _inCombat = false;
    bool _alive = true;
    
    std::map<uint32, uint32> _spellCooldowns;  // spellId -> readyTime
    std::map<uint32, uint8> _auras;            // spellId -> stacks
    
    void* _group = nullptr;
    uint8 _subGroup = 0;
};

// Factory functions
MockPlayer CreateTank(ObjectGuid guid = GeneratePlayerGuid());
MockPlayer CreateHealer(ObjectGuid guid = GeneratePlayerGuid());
MockPlayer CreateMeleeDPS(ObjectGuid guid = GeneratePlayerGuid());
MockPlayer CreateRangedDPS(ObjectGuid guid = GeneratePlayerGuid());

} // namespace Testing
} // namespace Playerbot
```

**MockGroup.h**:
```cpp
#pragma once

#include "MockPlayer.h"
#include <vector>

namespace Playerbot {
namespace Testing {

class MockGroup {
public:
    MockGroup() = default;
    
    void AddMember(MockPlayer* player);
    void RemoveMember(ObjectGuid guid);
    MockPlayer* GetMember(ObjectGuid guid);
    std::vector<MockPlayer*>& GetMembers() { return _members; }
    size_t GetMemberCount() const { return _members.size(); }
    
    void SetLeader(ObjectGuid guid) { _leader = guid; }
    ObjectGuid GetLeader() const { return _leader; }
    
    bool IsRaidGroup() const { return _isRaid; }
    void SetRaidGroup(bool isRaid) { _isRaid = isRaid; }
    
    // Sub-groups (for raids)
    void AssignToSubGroup(ObjectGuid player, uint8 subGroup);
    std::vector<MockPlayer*> GetSubGroupMembers(uint8 subGroup);
    
private:
    std::vector<MockPlayer*> _members;
    ObjectGuid _leader;
    bool _isRaid = false;
    std::map<ObjectGuid, uint8> _subGroups;
};

// Factory functions
MockGroup CreateDungeonGroup();  // 1 tank, 1 healer, 3 DPS
MockGroup CreateArenaTeam(uint8 size);  // 2, 3, or 5
MockGroup CreateRaidGroup(uint8 size);  // 10, 25, or 40

} // namespace Testing
} // namespace Playerbot
```

### 5.1.3: Create Test Fixtures

**TestFixtures.h**:
```cpp
#pragma once

#include "TestHelpers.h"
#include "MockPlayer.h"
#include "MockGroup.h"
#include "Core/Events/CombatEventRouter.h"

namespace Playerbot {
namespace Testing {

// ============================================================================
// BASE FIXTURES
// ============================================================================

class EventSystemFixture : public TestFixture {
public:
    void SetUp() override {
        ResetTestTime();
        _router = &CombatEventRouter::Instance();
        _router->UnsubscribeAll();
    }
    
    void TearDown() override {
        _router->UnsubscribeAll();
    }
    
protected:
    CombatEventRouter* _router;
};

class CoordinatorFixture : public TestFixture {
public:
    void SetUp() override {
        ResetTestTime();
        CombatEventRouter::Instance().UnsubscribeAll();
    }
    
    void TearDown() override {
        CombatEventRouter::Instance().UnsubscribeAll();
    }
};

// ============================================================================
// DUNGEON FIXTURES
// ============================================================================

class DungeonFixture : public TestFixture {
public:
    void SetUp() override {
        ResetTestTime();
        _group = CreateDungeonGroup();
        _tank = _group.GetMembers()[0];
        _healer = _group.GetMembers()[1];
        // DPS are members 2, 3, 4
    }
    
    void TearDown() override {
        // Cleanup
    }
    
protected:
    MockGroup _group;
    MockPlayer* _tank;
    MockPlayer* _healer;
    
    void SimulatePull();
    void SimulateBossEngage(uint32 bossId);
    void SimulateWipe();
};

// ============================================================================
// ARENA FIXTURES
// ============================================================================

class ArenaFixture : public TestFixture {
public:
    void SetUp() override {
        ResetTestTime();
        _team = CreateArenaTeam(3);  // 3v3
        _enemies.clear();
        for (int i = 0; i < 3; ++i) {
            _enemies.push_back(CreateMeleeDPS());
        }
    }
    
protected:
    MockGroup _team;
    std::vector<MockPlayer> _enemies;
    
    void SimulateArenaStart();
    void SimulateBurstWindow();
    void SimulateCCChain(ObjectGuid target);
};

// ============================================================================
// RAID FIXTURES
// ============================================================================

class RaidFixture : public TestFixture {
public:
    void SetUp() override {
        ResetTestTime();
        _raid = CreateRaidGroup(25);
        
        // Identify tanks (first 2 warriors/paladins/DKs)
        int tankCount = 0;
        for (auto* member : _raid.GetMembers()) {
            if (tankCount < 2 && (member->GetClass() == 1 || member->GetClass() == 2 || member->GetClass() == 6)) {
                _tanks.push_back(member);
                tankCount++;
            }
        }
        
        // Identify healers (priests, paladins, druids, shamans, monks)
        for (auto* member : _raid.GetMembers()) {
            uint8 cls = member->GetClass();
            if (cls == 5 || cls == 7 || cls == 11 || cls == 10) {
                _healers.push_back(member);
            }
        }
    }
    
protected:
    MockGroup _raid;
    std::vector<MockPlayer*> _tanks;
    std::vector<MockPlayer*> _healers;
    
    void SimulateBossPull();
    void SimulateTankSwapScenario(uint32 debuffSpellId, uint8 stacks);
    void SimulateRaidDamage(float percentDamage);
    void SimulateAddSpawn(uint32 creatureId, uint8 count);
};

} // namespace Testing
} // namespace Playerbot
```

---

## Task 5.2: Unit Tests

**Priority**: P0  
**Effort**: 24 hours  
**Dependencies**: Task 5.1 complete

### 5.2.1: Event System Unit Tests

**CombatEventRouterTests.cpp**:
```cpp
#include "TestHelpers.h"
#include "Core/Events/CombatEventRouter.h"
#include "Core/Events/CombatEvent.h"

using namespace Playerbot;
using namespace Playerbot::Testing;

// Test subscriber for verification
class TestSubscriber : public ICombatEventSubscriber {
public:
    int eventCount = 0;
    CombatEvent lastEvent;
    
    void OnCombatEvent(const CombatEvent& event) override {
        eventCount++;
        lastEvent = event;
    }
    
    CombatEventType GetSubscribedEventTypes() const override {
        return subscribedTypes;
    }
    
    CombatEventType subscribedTypes = CombatEventType::ALL_EVENTS;
};

TEST_CASE(CombatEventRouter_Subscribe_RegistersSubscriber) {
    auto& router = CombatEventRouter::Instance();
    router.UnsubscribeAll();
    
    TestSubscriber subscriber;
    router.Subscribe(&subscriber);
    
    ASSERT_EQ(1, router.GetSubscriberCount());
}

TEST_CASE(CombatEventRouter_Dispatch_DeliversToSubscriber) {
    auto& router = CombatEventRouter::Instance();
    router.UnsubscribeAll();
    
    TestSubscriber subscriber;
    subscriber.subscribedTypes = CombatEventType::DAMAGE_TAKEN;
    router.Subscribe(&subscriber);
    
    auto event = CombatEvent::CreateDamageTaken(
        GeneratePlayerGuid(), GenerateCreatureGuid(), 1000, 0, nullptr);
    router.Dispatch(event);
    
    ASSERT_EQ(1, subscriber.eventCount);
    ASSERT_EQ(CombatEventType::DAMAGE_TAKEN, subscriber.lastEvent.type);
    ASSERT_EQ(1000, subscriber.lastEvent.amount);
}

TEST_CASE(CombatEventRouter_Dispatch_FiltersByEventType) {
    auto& router = CombatEventRouter::Instance();
    router.UnsubscribeAll();
    
    TestSubscriber damageSubscriber;
    damageSubscriber.subscribedTypes = CombatEventType::DAMAGE_TAKEN;
    router.Subscribe(&damageSubscriber);
    
    TestSubscriber healSubscriber;
    healSubscriber.subscribedTypes = CombatEventType::HEALING_DONE;
    router.Subscribe(&healSubscriber);
    
    // Dispatch damage event
    auto damageEvent = CombatEvent::CreateDamageTaken(
        GeneratePlayerGuid(), GenerateCreatureGuid(), 1000, 0, nullptr);
    router.Dispatch(damageEvent);
    
    ASSERT_EQ(1, damageSubscriber.eventCount);
    ASSERT_EQ(0, healSubscriber.eventCount);
    
    // Dispatch healing event
    auto healEvent = CombatEvent::CreateHealingDone(
        GeneratePlayerGuid(), GeneratePlayerGuid(), 500, 0, nullptr);
    router.Dispatch(healEvent);
    
    ASSERT_EQ(1, damageSubscriber.eventCount);
    ASSERT_EQ(1, healSubscriber.eventCount);
}

TEST_CASE(CombatEventRouter_Unsubscribe_RemovesSubscriber) {
    auto& router = CombatEventRouter::Instance();
    router.UnsubscribeAll();
    
    TestSubscriber subscriber;
    router.Subscribe(&subscriber);
    ASSERT_EQ(1, router.GetSubscriberCount());
    
    router.Unsubscribe(&subscriber);
    ASSERT_EQ(0, router.GetSubscriberCount());
    
    // Event should not be delivered after unsubscribe
    auto event = CombatEvent::CreateDamageTaken(
        GeneratePlayerGuid(), GenerateCreatureGuid(), 1000, 0, nullptr);
    router.Dispatch(event);
    
    ASSERT_EQ(0, subscriber.eventCount);
}

TEST_CASE(CombatEventRouter_QueueEvent_ProcessesOnNextTick) {
    auto& router = CombatEventRouter::Instance();
    router.UnsubscribeAll();
    
    TestSubscriber subscriber;
    router.Subscribe(&subscriber);
    
    auto event = CombatEvent::CreateDamageTaken(
        GeneratePlayerGuid(), GenerateCreatureGuid(), 1000, 0, nullptr);
    router.QueueEvent(event);
    
    // Event not delivered yet
    ASSERT_EQ(0, subscriber.eventCount);
    
    // Process queue
    router.ProcessQueuedEvents();
    
    ASSERT_EQ(1, subscriber.eventCount);
}

TEST_CASE(CombatEventRouter_MultipleSubscribers_AllReceiveEvent) {
    auto& router = CombatEventRouter::Instance();
    router.UnsubscribeAll();
    
    TestSubscriber sub1, sub2, sub3;
    router.Subscribe(&sub1);
    router.Subscribe(&sub2);
    router.Subscribe(&sub3);
    
    auto event = CombatEvent::CreateDamageTaken(
        GeneratePlayerGuid(), GenerateCreatureGuid(), 1000, 0, nullptr);
    router.Dispatch(event);
    
    ASSERT_EQ(1, sub1.eventCount);
    ASSERT_EQ(1, sub2.eventCount);
    ASSERT_EQ(1, sub3.eventCount);
}
```

### 5.2.2: Interrupt Coordinator Tests

**InterruptCoordinatorTests.cpp**:
```cpp
#include "TestHelpers.h"
#include "TestFixtures.h"
#include "AI/Combat/InterruptCoordinator.h"

using namespace Playerbot;
using namespace Playerbot::Testing;

TEST_FIXTURE(DungeonFixture, InterruptCoordinator_AssignsInterrupter_OnSpellCastStart) {
    InterruptCoordinator coordinator(&_group);
    coordinator.Initialize();
    
    ObjectGuid enemyGuid = GenerateCreatureGuid();
    uint32 interruptibleSpellId = 12345;
    
    // Simulate enemy starting to cast
    auto event = CombatEvent::CreateSpellCastStart(
        enemyGuid, nullptr, GeneratePlayerGuid());
    event.spellId = interruptibleSpellId;
    
    CombatEventRouter::Instance().Dispatch(event);
    
    // Verify an interrupter was assigned
    ObjectGuid interrupter = coordinator.GetAssignedInterrupter(enemyGuid);
    ASSERT_FALSE(interrupter.IsEmpty());
}

TEST_FIXTURE(DungeonFixture, InterruptCoordinator_TracksSuccessfulInterrupt) {
    InterruptCoordinator coordinator(&_group);
    coordinator.Initialize();
    
    ObjectGuid enemyGuid = GenerateCreatureGuid();
    ObjectGuid interrupterGuid = _group.GetMembers()[2]->GetGUID();  // DPS
    
    // Start cast
    auto castEvent = CombatEvent::CreateSpellCastStart(
        enemyGuid, nullptr, GeneratePlayerGuid());
    CombatEventRouter::Instance().Dispatch(castEvent);
    
    // Interrupt it
    auto interruptEvent = CombatEvent::CreateSpellInterrupted(
        enemyGuid, nullptr, interrupterGuid);
    CombatEventRouter::Instance().Dispatch(interruptEvent);
    
    // Check metrics
    auto metrics = coordinator.GetMetrics();
    ASSERT_EQ(1, metrics.successfulInterrupts);
    ASSERT_EQ(0, metrics.missedInterrupts);
}

TEST_FIXTURE(DungeonFixture, InterruptCoordinator_TracksMissedInterrupt) {
    InterruptCoordinator coordinator(&_group);
    coordinator.Initialize();
    
    ObjectGuid enemyGuid = GenerateCreatureGuid();
    
    // Start cast
    auto castEvent = CombatEvent::CreateSpellCastStart(
        enemyGuid, nullptr, GeneratePlayerGuid());
    CombatEventRouter::Instance().Dispatch(castEvent);
    
    // Cast succeeds (not interrupted)
    auto successEvent = CombatEvent::CreateSpellCastSuccess(
        enemyGuid, nullptr);
    CombatEventRouter::Instance().Dispatch(successEvent);
    
    // Check metrics
    auto metrics = coordinator.GetMetrics();
    ASSERT_EQ(0, metrics.successfulInterrupts);
    ASSERT_EQ(1, metrics.missedInterrupts);
}
```

### 5.2.3: Raid Tank Coordinator Tests

**RaidTankCoordinatorTests.cpp**:
```cpp
#include "TestHelpers.h"
#include "TestFixtures.h"
#include "AI/Coordination/Raid/RaidTankCoordinator.h"

using namespace Playerbot;
using namespace Playerbot::Testing;

TEST_FIXTURE(RaidFixture, RaidTankCoordinator_DetectsTankSwapNeed) {
    RaidTankCoordinator tankCoord(nullptr);  // Parent coordinator
    tankCoord.Initialize();
    
    ObjectGuid tank1 = _tanks[0]->GetGUID();
    ObjectGuid tank2 = _tanks[1]->GetGUID();
    
    tankCoord.RegisterTank(tank1, TankRole::ACTIVE);
    tankCoord.RegisterTank(tank2, TankRole::SWAP_READY);
    
    // Configure swap trigger: swap at 3 stacks
    uint32 debuffSpellId = 99999;
    tankCoord.ConfigureSwapTrigger({debuffSpellId, 3, 30000, false});
    
    // Add stacks to active tank
    tankCoord.OnSwapDebuffApplied(tank1, debuffSpellId, 1);
    ASSERT_FALSE(tankCoord.NeedsTankSwap());
    
    tankCoord.OnSwapDebuffApplied(tank1, debuffSpellId, 2);
    ASSERT_FALSE(tankCoord.NeedsTankSwap());
    
    tankCoord.OnSwapDebuffApplied(tank1, debuffSpellId, 3);
    ASSERT_TRUE(tankCoord.NeedsTankSwap());
}

TEST_FIXTURE(RaidFixture, RaidTankCoordinator_ExecutesTankSwap) {
    RaidTankCoordinator tankCoord(nullptr);
    tankCoord.Initialize();
    
    ObjectGuid tank1 = _tanks[0]->GetGUID();
    ObjectGuid tank2 = _tanks[1]->GetGUID();
    
    tankCoord.RegisterTank(tank1, TankRole::ACTIVE);
    tankCoord.RegisterTank(tank2, TankRole::SWAP_READY);
    tankCoord.SetActiveTank(tank1);
    
    ASSERT_EQ(tank1, tankCoord.GetActiveTank());
    ASSERT_EQ(TankRole::ACTIVE, tankCoord.GetTankRole(tank1));
    
    // Execute swap
    tankCoord.CallTankSwap();
    tankCoord.ExecuteTankSwap();
    
    ASSERT_EQ(tank2, tankCoord.GetActiveTank());
    ASSERT_EQ(TankRole::RECOVERING, tankCoord.GetTankRole(tank1));
    ASSERT_EQ(TankRole::ACTIVE, tankCoord.GetTankRole(tank2));
}

TEST_FIXTURE(RaidFixture, RaidTankCoordinator_TracksDebuffExpiration) {
    RaidTankCoordinator tankCoord(nullptr);
    tankCoord.Initialize();
    
    ObjectGuid tank1 = _tanks[0]->GetGUID();
    tankCoord.RegisterTank(tank1, TankRole::ACTIVE);
    
    uint32 debuffSpellId = 99999;
    uint32 debuffDuration = 30000;  // 30 seconds
    
    tankCoord.ConfigureSwapTrigger({debuffSpellId, 3, debuffDuration, false});
    tankCoord.OnSwapDebuffApplied(tank1, debuffSpellId, 1);
    
    ASSERT_EQ(1, tankCoord.GetDebuffStacks(tank1));
    ASSERT_GT(tankCoord.GetDebuffTimeRemaining(tank1), 0);
    
    // Simulate time passing
    AdvanceTestTime(debuffDuration + 1000);
    tankCoord.Update(debuffDuration + 1000);
    
    // Debuff should have expired
    ASSERT_EQ(0, tankCoord.GetDebuffStacks(tank1));
}
```

### 5.2.4: CC Chain Manager Tests

**CCChainManagerTests.cpp**:
```cpp
#include "TestHelpers.h"
#include "TestFixtures.h"
#include "AI/Coordination/Arena/CCChainManager.h"

using namespace Playerbot;
using namespace Playerbot::Testing;

TEST_FIXTURE(ArenaFixture, CCChainManager_PlansCCChain_ConsideringDR) {
    CCChainManager ccManager(nullptr, nullptr);  // Will mock dependencies
    
    ObjectGuid target = _enemies[0].GetGUID();
    uint32 desiredDuration = 20000;  // 20 seconds
    
    CCChain chain = ccManager.PlanChain(target, desiredDuration);
    
    // Chain should have multiple links
    ASSERT_GT(chain.links.size(), 1);
    
    // Total duration should be close to desired
    ASSERT_GE(chain.totalDuration, desiredDuration * 0.8);  // Within 80%
}

TEST_FIXTURE(ArenaFixture, CCChainManager_RespectsFullDR) {
    CCChainManager ccManager(nullptr, nullptr);
    
    ObjectGuid target = _enemies[0].GetGUID();
    
    // Simulate 3 CCs already applied (full DR)
    ccManager.OnCCApplied(GeneratePlayerGuid(), target, 12345);
    ccManager.OnCCApplied(GeneratePlayerGuid(), target, 12345);
    ccManager.OnCCApplied(GeneratePlayerGuid(), target, 12345);
    
    // Target should now be immune
    ASSERT_TRUE(ccManager.IsImmune(target, 12345));
    
    // Expected duration should be 0
    uint32 expectedDuration = ccManager.GetExpectedDuration(target, 12345);
    ASSERT_EQ(0, expectedDuration);
}

TEST_FIXTURE(ArenaFixture, CCChainManager_CalculatesDRReduction) {
    CCChainManager ccManager(nullptr, nullptr);
    
    ObjectGuid target = _enemies[0].GetGUID();
    uint32 fullDuration = 8000;  // 8 second CC
    
    // First CC: 100% duration
    uint8 stacks0 = ccManager.GetDRStacks(target, 12345);
    ASSERT_EQ(0, stacks0);
    
    // After first CC: 50% duration
    ccManager.OnCCApplied(GeneratePlayerGuid(), target, 12345);
    uint32 duration1 = ccManager.GetExpectedDuration(target, 12345);
    ASSERT_NEAR(fullDuration * 0.5, duration1, 100);
    
    // After second CC: 25% duration
    ccManager.OnCCApplied(GeneratePlayerGuid(), target, 12345);
    uint32 duration2 = ccManager.GetExpectedDuration(target, 12345);
    ASSERT_NEAR(fullDuration * 0.25, duration2, 100);
    
    // After third CC: immune
    ccManager.OnCCApplied(GeneratePlayerGuid(), target, 12345);
    uint32 duration3 = ccManager.GetExpectedDuration(target, 12345);
    ASSERT_EQ(0, duration3);
}
```

---

## Task 5.3: Integration Tests

**Priority**: P0  
**Effort**: 20 hours  
**Dependencies**: Task 5.2 complete

### 5.3.1: Coordinator Interaction Tests

**CoordinatorInteractionTests.cpp**:
```cpp
#include "TestHelpers.h"
#include "TestFixtures.h"

using namespace Playerbot;
using namespace Playerbot::Testing;

TEST_FIXTURE(DungeonFixture, Integration_InterruptAndCC_Coordinate) {
    // Both interrupt and CC coordinators should work together
    // When interrupt is on cooldown, CC should be used instead
    
    InterruptCoordinator interruptCoord(&_group);
    CrowdControlManager ccManager(&_group);
    
    interruptCoord.Initialize();
    ccManager.Initialize();
    
    ObjectGuid enemyGuid = GenerateCreatureGuid();
    
    // Simulate enemy casting important spell
    auto castEvent = CombatEvent::CreateSpellCastStart(
        enemyGuid, nullptr, _tank->GetGUID());
    CombatEventRouter::Instance().Dispatch(castEvent);
    
    // Interrupter should be assigned
    ObjectGuid interrupter = interruptCoord.GetAssignedInterrupter(enemyGuid);
    ASSERT_FALSE(interrupter.IsEmpty());
    
    // If interrupt unavailable, CC should be recommended
    if (!interruptCoord.CanInterrupt(interrupter)) {
        bool canCC = ccManager.CanCCTarget(enemyGuid);
        ASSERT_TRUE(canCC);
    }
}

TEST_FIXTURE(RaidFixture, Integration_TankSwap_TriggersHealerResponse) {
    RaidCoordinator raidCoord(_raid);
    raidCoord.Initialize();
    
    ObjectGuid tank1 = _tanks[0]->GetGUID();
    ObjectGuid tank2 = _tanks[1]->GetGUID();
    
    // Configure tank swap
    auto tankCoord = raidCoord.GetTankCoordinator();
    tankCoord->ConfigureSwapTrigger({99999, 3, 30000, false});
    
    // Simulate stacks building
    for (int i = 1; i <= 3; ++i) {
        tankCoord->OnSwapDebuffApplied(tank1, 99999, i);
    }
    
    // Tank swap should be needed
    ASSERT_TRUE(tankCoord->NeedsTankSwap());
    
    // Execute swap
    raidCoord.CallTankSwap();
    
    // Heal coordinator should have requested external on old tank
    auto healCoord = raidCoord.GetHealCoordinator();
    // Old tank (tank1) should be priority target during swap
    ObjectGuid priorityTarget = healCoord->GetHighestPriorityTarget();
    // Could be either tank during swap
    ASSERT_TRUE(priorityTarget == tank1 || priorityTarget == tank2);
}

TEST_FIXTURE(RaidFixture, Integration_AddSpawn_AssignsTankAndDPS) {
    RaidCoordinator raidCoord(_raid);
    raidCoord.Initialize();
    
    // Spawn priority add
    ObjectGuid addGuid = GenerateCreatureGuid();
    raidCoord.OnAddSpawned(addGuid, 12345);  // Creature ID 12345
    raidCoord.GetTargetManager()->SetAddPriority(addGuid, 1);  // Highest priority
    
    // Should have tank assigned
    ObjectGuid assignedTank = raidCoord.GetTargetManager()->GetTankForAdd(addGuid);
    ASSERT_FALSE(assignedTank.IsEmpty());
    
    // Should be highest priority target
    ObjectGuid killTarget = raidCoord.GetTargetManager()->GetHighestPriorityAdd();
    ASSERT_EQ(addGuid, killTarget);
}
```

### 5.3.2: Full Scenario Tests

**RaidScenarioTests.cpp**:
```cpp
#include "TestHelpers.h"
#include "TestFixtures.h"

using namespace Playerbot;
using namespace Playerbot::Testing;

TEST_FIXTURE(RaidFixture, Scenario_FullBossEncounter_WithTankSwaps) {
    RaidCoordinator raidCoord(_raid);
    raidCoord.Initialize();
    
    // Configure encounter
    uint32 bossId = 99999;
    uint32 tankSwapDebuff = 11111;
    
    raidCoord.LoadEncounter(bossId);
    raidCoord.GetTankCoordinator()->ConfigureSwapTrigger({tankSwapDebuff, 3, 30000, false});
    
    // Start encounter
    raidCoord.OnEncounterStart(bossId);
    ASSERT_EQ(RaidState::COMBAT, raidCoord.GetState());
    
    // Simulate 2 minutes of combat with tank swaps
    uint32 totalTime = 0;
    uint32 swapCount = 0;
    
    while (totalTime < 120000) {  // 2 minutes
        // Simulate boss attacking tank
        ObjectGuid activeTank = raidCoord.GetTankCoordinator()->GetActiveTank();
        
        // Every 10 seconds, add debuff stack
        if (totalTime % 10000 == 0) {
            uint8 currentStacks = raidCoord.GetTankCoordinator()->GetDebuffStacks(activeTank);
            raidCoord.GetTankCoordinator()->OnSwapDebuffApplied(activeTank, tankSwapDebuff, currentStacks + 1);
        }
        
        // Check for swap
        if (raidCoord.GetTankCoordinator()->NeedsTankSwap()) {
            raidCoord.CallTankSwap();
            swapCount++;
        }
        
        // Update
        raidCoord.Update(1000);
        totalTime += 1000;
    }
    
    // Should have had multiple tank swaps
    ASSERT_GE(swapCount, 2);
    
    // End encounter
    raidCoord.OnEncounterEnd(true);
    ASSERT_EQ(RaidState::LOOTING, raidCoord.GetState());
}

TEST_FIXTURE(ArenaFixture, Scenario_FullArenaMatch_WithBurstAndCC) {
    ArenaCoordinator arenaCoord(nullptr, _team.GetMembers());
    arenaCoord.Initialize();
    
    // Start match
    arenaCoord.SetState(ArenaState::COMBAT);
    
    // Initial target selection
    ObjectGuid killTarget = arenaCoord.GetKillTargetManager()->EvaluateBestKillTarget();
    arenaCoord.SetKillTarget(killTarget);
    
    // Simulate burst window
    arenaCoord.CallBurst(killTarget);
    ASSERT_TRUE(arenaCoord.IsBurstWindowActive());
    
    // Simulate CC on healer
    ObjectGuid enemyHealer = _enemies[1].GetGUID();  // Assume index 1 is healer
    arenaCoord.CallCCChain(enemyHealer);
    
    // Simulate damage to kill target
    _enemies[0].SetHealth(_enemies[0].GetMaxHealth() * 0.3f);  // 30% health
    
    // Should still be targeting same player (don't switch at low health)
    ObjectGuid currentTarget = arenaCoord.GetKillTarget();
    ASSERT_EQ(killTarget, currentTarget);
    
    // Simulate kill
    _enemies[0].SetAlive(false);
    arenaCoord.Update(100);
    
    // Should switch to new target
    ObjectGuid newTarget = arenaCoord.GetKillTarget();
    ASSERT_NE(killTarget, newTarget);
}
```

---

## Task 5.4: Performance Tests

**Priority**: P1  
**Effort**: 16 hours  
**Dependencies**: Task 5.2 complete

### 5.4.1: Event Router Benchmarks

**EventRouterBenchmarks.cpp**:
```cpp
#include "TestHelpers.h"
#include "Core/Events/CombatEventRouter.h"

using namespace Playerbot;
using namespace Playerbot::Testing;

class NullSubscriber : public ICombatEventSubscriber {
public:
    void OnCombatEvent(const CombatEvent& event) override {
        _eventCount++;
    }
    CombatEventType GetSubscribedEventTypes() const override {
        return CombatEventType::ALL_EVENTS;
    }
    uint64 _eventCount = 0;
};

TEST_CASE(Benchmark_EventRouter_DispatchPerformance) {
    auto& router = CombatEventRouter::Instance();
    router.UnsubscribeAll();
    
    // Add 100 subscribers (simulate 100 bots)
    std::vector<NullSubscriber> subscribers(100);
    for (auto& sub : subscribers) {
        router.Subscribe(&sub);
    }
    
    auto event = CombatEvent::CreateDamageTaken(
        GeneratePlayerGuid(), GenerateCreatureGuid(), 1000, 0, nullptr);
    
    Benchmark bench("EventRouter Dispatch (100 subscribers)", 10000);
    bench.Run([&]() {
        router.Dispatch(event);
    });
    
    bench.PrintResults();
    
    // Should be under 10 microseconds per dispatch
    ASSERT_LT(bench.GetAverageTimeUs(), 10.0);
}

TEST_CASE(Benchmark_EventRouter_HighSubscriberCount) {
    auto& router = CombatEventRouter::Instance();
    router.UnsubscribeAll();
    
    // Add 1000 subscribers (simulate 1000 bots)
    std::vector<NullSubscriber> subscribers(1000);
    for (auto& sub : subscribers) {
        router.Subscribe(&sub);
    }
    
    auto event = CombatEvent::CreateDamageTaken(
        GeneratePlayerGuid(), GenerateCreatureGuid(), 1000, 0, nullptr);
    
    Benchmark bench("EventRouter Dispatch (1000 subscribers)", 1000);
    bench.Run([&]() {
        router.Dispatch(event);
    });
    
    bench.PrintResults();
    
    // Should scale linearly, under 100 microseconds
    ASSERT_LT(bench.GetAverageTimeUs(), 100.0);
}

TEST_CASE(Benchmark_EventRouter_QueueProcessing) {
    auto& router = CombatEventRouter::Instance();
    router.UnsubscribeAll();
    
    std::vector<NullSubscriber> subscribers(100);
    for (auto& sub : subscribers) {
        router.Subscribe(&sub);
    }
    
    // Queue 1000 events
    for (int i = 0; i < 1000; ++i) {
        auto event = CombatEvent::CreateDamageTaken(
            GeneratePlayerGuid(), GenerateCreatureGuid(), 1000, 0, nullptr);
        router.QueueEvent(event);
    }
    
    Benchmark bench("EventRouter ProcessQueue (1000 events)", 100);
    bench.Run([&]() {
        router.ProcessQueuedEvents();
        
        // Re-queue for next iteration
        for (int i = 0; i < 1000; ++i) {
            auto event = CombatEvent::CreateDamageTaken(
                GeneratePlayerGuid(), GenerateCreatureGuid(), 1000, 0, nullptr);
            router.QueueEvent(event);
        }
    });
    
    bench.PrintResults();
    
    // Should process 1000 events in under 10ms
    ASSERT_LT(bench.GetTotalTimeUs() / 100, 10000);
}
```

### 5.4.2: Scalability Tests

**ScalabilityTests.cpp**:
```cpp
#include "TestHelpers.h"

using namespace Playerbot;
using namespace Playerbot::Testing;

TEST_CASE(Scalability_5000Bots_EventRouting) {
    auto& router = CombatEventRouter::Instance();
    router.UnsubscribeAll();
    
    // Simulate 5000 bots
    std::vector<std::unique_ptr<NullSubscriber>> subscribers;
    for (int i = 0; i < 5000; ++i) {
        auto sub = std::make_unique<NullSubscriber>();
        router.Subscribe(sub.get());
        subscribers.push_back(std::move(sub));
    }
    
    ASSERT_EQ(5000, router.GetSubscriberCount());
    
    // Simulate 1 second of combat (60 ticks at 60fps)
    uint32 totalEvents = 0;
    auto startTime = std::chrono::high_resolution_clock::now();
    
    for (int tick = 0; tick < 60; ++tick) {
        // Each tick: simulate damage, healing, spell events
        for (int i = 0; i < 100; ++i) {  // 100 events per tick
            auto event = CombatEvent::CreateDamageTaken(
                GeneratePlayerGuid(), GenerateCreatureGuid(), 1000, 0, nullptr);
            router.Dispatch(event);
            totalEvents++;
        }
    }
    
    auto endTime = std::chrono::high_resolution_clock::now();
    auto durationMs = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
    
    // Should complete in under 1 second (real-time)
    ASSERT_LT(durationMs, 1000);
    
    // Calculate events per second
    double eventsPerSecond = (double)totalEvents / (durationMs / 1000.0);
    std::cout << "Events per second: " << eventsPerSecond << std::endl;
    
    // Should handle at least 5000 events per second
    ASSERT_GT(eventsPerSecond, 5000);
}

TEST_CASE(Scalability_RaidCoordinator_40ManRaid) {
    MockGroup raid = CreateRaidGroup(40);
    
    // This would need actual RaidCoordinator implementation
    // For now, verify group creation
    ASSERT_EQ(40, raid.GetMemberCount());
    
    // Benchmark update cycle
    Benchmark bench("RaidCoordinator Update (40-man)", 1000);
    bench.Run([&]() {
        // Simulate coordinator update
        // In real test: raidCoord.Update(16);  // 16ms = 60fps
    });
    
    bench.PrintResults();
}
```

### 5.4.3: Memory Leak Tests

**MemoryLeakTests.cpp**:
```cpp
#include "TestHelpers.h"

using namespace Playerbot;
using namespace Playerbot::Testing;

TEST_CASE(MemoryLeak_EventRouter_SubscribeUnsubscribe) {
    auto& router = CombatEventRouter::Instance();
    router.UnsubscribeAll();
    
    size_t initialMemory = GetCurrentMemoryUsage();
    
    // Subscribe and unsubscribe many times
    for (int iteration = 0; iteration < 10000; ++iteration) {
        NullSubscriber subscriber;
        router.Subscribe(&subscriber);
        
        // Dispatch some events
        for (int i = 0; i < 10; ++i) {
            auto event = CombatEvent::CreateDamageTaken(
                GeneratePlayerGuid(), GenerateCreatureGuid(), 1000, 0, nullptr);
            router.Dispatch(event);
        }
        
        router.Unsubscribe(&subscriber);
    }
    
    size_t finalMemory = GetCurrentMemoryUsage();
    
    // Memory should not have grown significantly
    // Allow 1MB tolerance for normal heap fragmentation
    ASSERT_LT(finalMemory - initialMemory, 1024 * 1024);
}

TEST_CASE(MemoryLeak_EventQueue_LongRunning) {
    auto& router = CombatEventRouter::Instance();
    router.UnsubscribeAll();
    
    NullSubscriber subscriber;
    router.Subscribe(&subscriber);
    
    size_t initialMemory = GetCurrentMemoryUsage();
    
    // Simulate 1 hour of event processing (compressed)
    for (int minute = 0; minute < 60; ++minute) {
        // Queue many events
        for (int i = 0; i < 1000; ++i) {
            auto event = CombatEvent::CreateDamageTaken(
                GeneratePlayerGuid(), GenerateCreatureGuid(), 1000, 0, nullptr);
            router.QueueEvent(event);
        }
        
        // Process queue
        router.ProcessQueuedEvents();
    }
    
    size_t finalMemory = GetCurrentMemoryUsage();
    
    // Memory should not have grown
    ASSERT_LT(finalMemory - initialMemory, 1024 * 1024);
}
```

---

## Task 5.5: Documentation & Reports

**Priority**: P2  
**Effort**: 8 hours  
**Dependencies**: Tasks 5.2-5.4 complete

### 5.5.1: Test Report Generator

```cpp
#pragma once

#include <string>
#include <vector>

namespace Playerbot {
namespace Testing {

class TestReportGenerator {
public:
    void GenerateHTML(const std::string& filename);
    void GenerateJUnit(const std::string& filename);
    void GenerateMarkdown(const std::string& filename);
    
    void AddTestResult(const TestInfo& result);
    void AddBenchmarkResult(const std::string& name, double avgTimeUs, double totalTimeUs);
    void AddCoverageData(const std::string& file, float coverage);
    
private:
    std::vector<TestInfo> _testResults;
    std::vector<BenchmarkResult> _benchmarkResults;
    std::map<std::string, float> _coverageData;
};

} // namespace Testing
} // namespace Playerbot
```

### 5.5.2: Performance Baseline

```cpp
// PerformanceBaseline.h
#pragma once

namespace Playerbot {
namespace Testing {

struct PerformanceBaseline {
    // Event System
    static constexpr double EVENT_DISPATCH_MAX_US = 10.0;       // Per dispatch
    static constexpr double EVENT_QUEUE_PROCESS_MAX_MS = 10.0;  // Per 1000 events
    
    // Coordinators
    static constexpr double INTERRUPT_COORD_UPDATE_MAX_US = 50.0;
    static constexpr double THREAT_COORD_UPDATE_MAX_US = 100.0;
    static constexpr double RAID_COORD_UPDATE_MAX_US = 500.0;
    
    // Scalability
    static constexpr size_t MIN_SUPPORTED_BOTS = 5000;
    static constexpr double MAX_CPU_PER_BOT_PERCENT = 0.01;  // 0.01% per bot
    
    // Memory
    static constexpr size_t MAX_MEMORY_PER_BOT_KB = 100;
    static constexpr size_t MAX_MEMORY_GROWTH_MB = 10;  // Over 1 hour
};

} // namespace Testing
} // namespace Playerbot
```

---

## Summary

| Task | Effort | Deliverables |
|------|--------|--------------|
| 5.1 | 16h | Test infrastructure, mocks, fixtures |
| 5.2 | 24h | Unit tests for all coordinators |
| 5.3 | 20h | Integration & scenario tests |
| 5.4 | 16h | Performance benchmarks, scalability tests |
| 5.5 | 8h | Documentation, reports |
| **Total** | **84h** | |

## Test Coverage Targets

| Component | Target Coverage |
|-----------|----------------|
| Event System | 90% |
| InterruptCoordinator | 85% |
| ThreatCoordinator | 85% |
| CrowdControlManager | 85% |
| DungeonCoordinator | 80% |
| ArenaCoordinator | 80% |
| BattlegroundCoordinator | 75% |
| RaidCoordinator | 80% |
| **Overall** | **≥80%** |

## Performance Targets

| Metric | Target |
|--------|--------|
| Event dispatch latency | <10μs |
| Coordinator update (per bot) | <100μs |
| Memory per bot | <100KB |
| Supported bot count | 5,000+ |
| CPU usage (5000 bots) | <50% |
