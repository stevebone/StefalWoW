# Unified Interrupt System - Test Framework Requirements

**Date**: 2025-11-01
**Phase**: Phase 3 - Testing & Validation
**Status**: Framework Complete - Infrastructure Requirements Documented

---

## Executive Summary

The UnifiedInterruptSystem test framework is complete with 32 comprehensive tests. However, 27 integration tests are currently disabled pending TrinityCore test infrastructure setup. This document details the exact requirements needed to enable full integration testing.

**Current Test Status:**
- ✅ **5 Enabled Tests** - Core functionality (singleton, metrics, reset) - All passing
- ⏸️ **27 Disabled Tests** - Integration tests requiring TrinityCore infrastructure
- 🎯 **Goal**: Enable all 32 tests with proper TrinityCore test infrastructure

---

## Test Framework Architecture

### Test File Structure

**Location**: `src/modules/Playerbot/Tests/UnifiedInterruptSystemTest.cpp` (900+ lines)

**Framework**: Google Test (GTest)

**Test Categories:**
1. **Core Functionality** (5 tests) - ✅ Enabled
2. **Bot Registration & Lifecycle** (4 tests) - ⏸️ Disabled
3. **Cast Detection & Tracking** (3 tests) - ⏸️ Disabled
4. **Decision Making & Planning** (4 tests) - ⏸️ Disabled
5. **Group Coordination** (5 tests) - ⏸️ Disabled
6. **Rotation System** (3 tests) - ⏸️ Disabled
7. **Fallback Logic** (3 tests) - ⏸️ Disabled
8. **Performance & Thread Safety** (5 tests) - ⏸️ Disabled

---

## Infrastructure Requirements

### 1. TrinityCore Object Factories

**Required**: Test factory classes for creating mock/real TrinityCore objects.

#### Player Factory
```cpp
class PlayerTestFactory
{
public:
    static Player* CreateTestPlayer(
        ObjectGuid guid,
        uint8 race = RACE_HUMAN,
        uint8 class_ = CLASS_WARRIOR,
        uint8 level = 80
    );

    static void DestroyTestPlayer(Player* player);

    static void AddSpellToPlayer(Player* player, uint32 spellId);
    static void SetPlayerPosition(Player* player, float x, float y, float z);
    static void SetPlayerGroup(Player* player, Group* group);
};
```

**Why Needed**: All 27 disabled tests require Player objects.

**Example Usage**:
```cpp
TEST_F(UnifiedInterruptSystemTest, BotRegistration)
{
    Player* bot = PlayerTestFactory::CreateTestPlayer(
        ObjectGuid::Create<HighGuid::Player>(1),
        RACE_HUMAN,
        CLASS_WARRIOR,
        80
    );

    // Add interrupt spell
    PlayerTestFactory::AddSpellToPlayer(bot, SPELL_KICK);

    // Test registration
    sUnifiedInterruptSystem->RegisterBot(bot, mockBotAI);

    PlayerTestFactory::DestroyTestPlayer(bot);
}
```

#### Unit Factory
```cpp
class UnitTestFactory
{
public:
    static Unit* CreateTestUnit(
        ObjectGuid guid,
        uint32 entry,
        float x, float y, float z
    );

    static void DestroyTestUnit(Unit* unit);

    static void StartCasting(Unit* unit, uint32 spellId, uint32 castTime);
    static void StopCasting(Unit* unit);
    static void SetUnitHealth(Unit* unit, uint32 health);
};
```

**Why Needed**: Cast detection tests require enemy Unit objects.

**Example Usage**:
```cpp
TEST_F(UnifiedInterruptSystemTest, CastDetection)
{
    Unit* enemy = UnitTestFactory::CreateTestUnit(
        ObjectGuid::Create<HighGuid::Creature>(100),
        12345, // entry
        100.0f, 100.0f, 100.0f // position
    );

    // Simulate enemy cast start
    UnitTestFactory::StartCasting(enemy, SPELL_FROSTBOLT, 3000);
    sUnifiedInterruptSystem->OnEnemyCastStart(enemy, SPELL_FROSTBOLT, 3000);

    UnitTestFactory::DestroyTestUnit(enemy);
}
```

#### Group Factory
```cpp
class GroupTestFactory
{
public:
    static Group* CreateTestGroup(ObjectGuid leaderGuid);
    static void DestroyTestGroup(Group* group);

    static void AddMember(Group* group, Player* player);
    static void RemoveMember(Group* group, ObjectGuid playerGuid);
    static void SetGroupLeader(Group* group, ObjectGuid leaderGuid);
};
```

**Why Needed**: Group coordination tests require Group objects.

**Example Usage**:
```cpp
TEST_F(UnifiedInterruptSystemTest, GroupCoordination)
{
    Group* group = GroupTestFactory::CreateTestGroup(
        ObjectGuid::Create<HighGuid::Player>(1)
    );

    // Add 5 bots to group
    for (int i = 1; i <= 5; ++i)
    {
        Player* bot = PlayerTestFactory::CreateTestPlayer(
            ObjectGuid::Create<HighGuid::Player>(i)
        );
        GroupTestFactory::AddMember(group, bot);
    }

    // Test coordination
    sUnifiedInterruptSystem->CoordinateGroupInterrupts(group);

    GroupTestFactory::DestroyTestGroup(group);
}
```

#### BotAI Factory
```cpp
class BotAITestFactory
{
public:
    static BotAI* CreateMockBotAI(Player* bot);
    static void DestroyMockBotAI(BotAI* ai);

    static void SetAIState(BotAI* ai, BotState state);
    static void AddAvailableSpell(BotAI* ai, uint32 spellId);
};
```

**Why Needed**: Bot registration requires BotAI instances.

**Example Usage**:
```cpp
TEST_F(UnifiedInterruptSystemTest, BotRegistration)
{
    Player* bot = PlayerTestFactory::CreateTestPlayer(...);
    BotAI* ai = BotAITestFactory::CreateMockBotAI(bot);

    sUnifiedInterruptSystem->RegisterBot(bot, ai);

    BotAITestFactory::DestroyMockBotAI(ai);
    PlayerTestFactory::DestroyTestPlayer(bot);
}
```

---

### 2. Database & Spell Data Initialization

**Required**: Minimal spell data loaded for test execution.

#### SpellMgr Initialization
```cpp
class SpellMgrTestSetup
{
public:
    static void InitializeMinimalSpellData();
    static void LoadInterruptSpells();
    static void LoadTestSpells();
    static void Cleanup();
};
```

**Spells Needed**:
```cpp
// Interrupt spells
SPELL_KICK = 1766,
SPELL_COUNTERSPELL = 2139,
SPELL_WIND_SHEAR = 57994,

// Enemy cast spells
SPELL_FROSTBOLT = 116,
SPELL_FIREBALL = 133,
SPELL_FLASH_HEAL = 2061,

// Fallback spells
SPELL_KIDNEY_SHOT = 408,
SPELL_SILENCE = 15487,
SPELL_CHARGE = 100
```

**Example Setup**:
```cpp
void SetUp() override
{
    // Initialize spell data
    SpellMgrTestSetup::InitializeMinimalSpellData();
    SpellMgrTestSetup::LoadInterruptSpells();

    // Initialize UnifiedInterruptSystem
    sUnifiedInterruptSystem->Initialize();
    sUnifiedInterruptSystem->ResetMetrics();
}

void TearDown() override
{
    sUnifiedInterruptSystem->Shutdown();
    SpellMgrTestSetup::Cleanup();
}
```

---

### 3. Map & World Environment

**Required**: Minimal map instance for position/distance calculations.

#### Map Factory
```cpp
class MapTestFactory
{
public:
    static Map* CreateTestMap(uint32 mapId = 0);
    static void DestroyTestMap(Map* map);

    static void AddObjectToMap(Map* map, WorldObject* object);
    static void RemoveObjectFromMap(Map* map, WorldObject* object);
};
```

**Why Needed**: Distance calculations (IsInRange, GetDistance) require map instance.

**Example Usage**:
```cpp
TEST_F(UnifiedInterruptSystemTest, RangeChecking)
{
    Map* map = MapTestFactory::CreateTestMap();

    Player* bot = PlayerTestFactory::CreateTestPlayer(...);
    bot->SetMap(map);
    PlayerTestFactory::SetPlayerPosition(bot, 0.0f, 0.0f, 0.0f);

    Unit* enemy = UnitTestFactory::CreateTestUnit(...);
    enemy->SetMap(map);
    // Position enemy 50 yards away
    enemy->Relocate(50.0f, 0.0f, 0.0f);

    // Test range validation
    bool inRange = bot->IsWithinDist(enemy, 40.0f); // Should be false
    EXPECT_FALSE(inRange);

    MapTestFactory::DestroyTestMap(map);
}
```

---

### 4. Threading & Timing Utilities

**Required**: Thread-safe test utilities and timing control.

#### Threading Test Utilities
```cpp
class ThreadingTestUtils
{
public:
    static void RunConcurrentTest(
        int numThreads,
        std::function<void(int threadId)> testFunc
    );

    static void SimulateHighConcurrency(
        int numBots,
        int numCasts,
        std::chrono::milliseconds duration
    );

    static void WaitForCondition(
        std::function<bool()> condition,
        std::chrono::milliseconds timeout
    );
};
```

**Example Usage**:
```cpp
TEST_F(UnifiedInterruptSystemTest, ThreadSafety)
{
    const int numThreads = 100;
    const int numBots = 10;

    ThreadingTestUtils::RunConcurrentTest(numThreads, [&](int threadId) {
        for (int i = 0; i < numBots; ++i)
        {
            Player* bot = PlayerTestFactory::CreateTestPlayer(...);
            sUnifiedInterruptSystem->RegisterBot(bot, mockAI);
            sUnifiedInterruptSystem->UnregisterBot(bot->GetGUID());
            PlayerTestFactory::DestroyTestPlayer(bot);
        }
    });

    // Verify no crashes or race conditions
    EXPECT_EQ(sUnifiedInterruptSystem->GetMetrics().botsRegistered.load(), 0);
}
```

#### Timing Control
```cpp
class TimingTestUtils
{
public:
    static void AdvanceGameTime(uint32 milliseconds);
    static uint32 GetCurrentGameTime();
    static void ResetGameTime();

    static void SimulateCastDuration(Unit* caster, uint32 castTime);
};
```

**Example Usage**:
```cpp
TEST_F(UnifiedInterruptSystemTest, CastTimingWindow)
{
    Unit* enemy = UnitTestFactory::CreateTestUnit(...);

    // Start 2-second cast
    UnitTestFactory::StartCasting(enemy, SPELL_FROSTBOLT, 2000);
    sUnifiedInterruptSystem->OnEnemyCastStart(enemy, SPELL_FROSTBOLT, 2000);

    // Advance time by 500ms (should still be casting)
    TimingTestUtils::AdvanceGameTime(500);

    auto targets = sUnifiedInterruptSystem->ScanForInterruptTargets(bot);
    EXPECT_EQ(targets.size(), 1); // Enemy still casting

    // Advance time by 2000ms (cast should finish)
    TimingTestUtils::AdvanceGameTime(2000);

    targets = sUnifiedInterruptSystem->ScanForInterruptTargets(bot);
    EXPECT_EQ(targets.size(), 0); // Cast finished
}
```

---

### 5. Mock/Stub Dependencies

**Required**: Mock implementations for integration points.

#### SpellPacketBuilder Mock
```cpp
class SpellPacketBuilderMock
{
public:
    static bool CastSpell(Player* caster, Unit* target, uint32 spellId);
    static void RecordCastAttempt(ObjectGuid casterGuid, uint32 spellId);
    static std::vector<CastAttempt> GetCastHistory();
    static void ClearHistory();
};

struct CastAttempt
{
    ObjectGuid casterGuid;
    ObjectGuid targetGuid;
    uint32 spellId;
    TimePoint timestamp;
};
```

**Why Needed**: ExecuteInterruptPlan calls SpellPacketBuilder::CastSpell.

**Example Usage**:
```cpp
TEST_F(UnifiedInterruptSystemTest, PlanExecution)
{
    SpellPacketBuilderMock::ClearHistory();

    Player* bot = PlayerTestFactory::CreateTestPlayer(...);
    Unit* enemy = UnitTestFactory::CreateTestUnit(...);

    InterruptTarget target;
    target.casterGuid = enemy->GetGUID();
    target.spellId = SPELL_FROSTBOLT;

    InterruptPlan plan = sUnifiedInterruptSystem->CreateInterruptPlan(bot, target);
    bool success = sUnifiedInterruptSystem->ExecuteInterruptPlan(bot, plan);

    EXPECT_TRUE(success);

    auto history = SpellPacketBuilderMock::GetCastHistory();
    ASSERT_EQ(history.size(), 1);
    EXPECT_EQ(history[0].casterGuid, bot->GetGUID());
    EXPECT_EQ(history[0].spellId, SPELL_KICK);
}
```

#### MovementArbiter Mock
```cpp
class MovementArbiterMock
{
public:
    static bool RequestMovement(
        Player* bot,
        Position const& destination,
        PlayerBotMovementPriority priority
    );

    static void RecordMovementRequest(ObjectGuid botGuid, Position dest);
    static std::vector<MovementRequest> GetMovementHistory();
    static void ClearHistory();
};

struct MovementRequest
{
    ObjectGuid botGuid;
    Position destination;
    PlayerBotMovementPriority priority;
    TimePoint timestamp;
};
```

**Why Needed**: Fallback logic uses MovementArbiter for positioning.

**Example Usage**:
```cpp
TEST_F(UnifiedInterruptSystemTest, FallbackMovement)
{
    MovementArbiterMock::ClearHistory();

    Player* bot = PlayerTestFactory::CreateTestPlayer(...);
    Unit* target = UnitTestFactory::CreateTestUnit(...);

    // Set bot 50 yards away (out of interrupt range)
    PlayerTestFactory::SetPlayerPosition(bot, 0.0f, 0.0f, 0.0f);
    target->Relocate(50.0f, 0.0f, 0.0f);

    FallbackMethod method = sUnifiedInterruptSystem->SelectFallbackMethod(bot, target, SPELL_KICK);
    EXPECT_EQ(method, FallbackMethod::MOVE_TO_RANGE);

    bool success = sUnifiedInterruptSystem->ExecuteFallback(bot, target, method);
    EXPECT_TRUE(success);

    auto history = MovementArbiterMock::GetMovementHistory();
    ASSERT_EQ(history.size(), 1);
    EXPECT_EQ(history[0].priority, INTERRUPT_POSITIONING);
}
```

---

## Test Execution Plan

### Phase 1: Infrastructure Setup (1-2 weeks)

**Week 1: Core Factories**
- [ ] Implement PlayerTestFactory
- [ ] Implement UnitTestFactory
- [ ] Implement BotAITestFactory
- [ ] Test basic object creation/destruction

**Week 2: Environment & Utilities**
- [ ] Implement MapTestFactory
- [ ] Implement SpellMgrTestSetup
- [ ] Implement ThreadingTestUtils
- [ ] Implement TimingTestUtils

### Phase 2: Mock Implementations (1 week)

- [ ] Implement SpellPacketBuilderMock
- [ ] Implement MovementArbiterMock
- [ ] Implement InterruptDatabase test data loader

### Phase 3: Test Enablement (2 weeks)

**Week 1: Basic Tests**
- [ ] Enable BotRegistration tests (4 tests)
- [ ] Enable CastDetection tests (3 tests)
- [ ] Verify all 7 tests pass

**Week 2: Advanced Tests**
- [ ] Enable DecisionMaking tests (4 tests)
- [ ] Enable GroupCoordination tests (5 tests)
- [ ] Enable RotationSystem tests (3 tests)
- [ ] Enable FallbackLogic tests (3 tests)
- [ ] Verify all 15 tests pass

### Phase 4: Performance & Validation (1 week)

- [ ] Enable Performance tests (5 tests)
- [ ] Run full test suite (32 tests)
- [ ] Performance benchmarking
- [ ] Memory leak detection
- [ ] Thread safety validation

---

## Expected Test Results

### Test Success Criteria

**All Tests Must:**
- ✅ Pass without crashes
- ✅ Complete within timeout (10s default, 60s for stress tests)
- ✅ Verify expected behavior with EXPECT/ASSERT macros
- ✅ Clean up all resources (no memory leaks)
- ✅ Be deterministic (reproducible results)

### Performance Benchmarks

**Assignment Time:**
- ✅ Target: <100μs per interrupt assignment
- ✅ Measurement: Average over 10,000 assignments
- ✅ Test: TEST_F(UnifiedInterruptSystemTest, DISABLED_AssignmentPerformance)

**Memory Usage:**
- ✅ Target: <1KB per registered bot
- ✅ Measurement: Memory delta after 1,000 bot registrations
- ✅ Test: TEST_F(UnifiedInterruptSystemTest, DISABLED_MemoryPerBot)

**Thread Safety:**
- ✅ Target: Zero race conditions with 100 concurrent threads
- ✅ Measurement: 100 threads × 1,000 operations each
- ✅ Test: TEST_F(UnifiedInterruptSystemTest, DISABLED_ThreadSafetyStress)

**Scalability:**
- ✅ Target: Support 5,000+ concurrent bots
- ✅ Measurement: Register/update 5,000 bots
- ✅ Test: TEST_F(UnifiedInterruptSystemTest, DISABLED_ScalabilityTest)

---

## Integration with Existing Test Infrastructure

### TrinityCore Test Framework Discovery

**Known Locations to Check:**
```
src/server/game/Tests/           # TrinityCore core tests
src/test/                         # Root test directory
cmake/tests/                      # Test CMake configuration
```

**Questions to Investigate:**
1. Does TrinityCore have existing test factories?
2. What test framework is currently used? (GTest, CppUnit, custom)
3. Are there existing object creation utilities?
4. How is database initialization handled in tests?
5. Are there existing mock implementations?

**Recommended Approach:**
```bash
# Search for existing test utilities
grep -r "CreateTestPlayer" src/
grep -r "PlayerTestFactory" src/
grep -r "UnitTestFactory" src/
grep -r "TEST_F" src/

# Search for GTest usage
grep -r "::testing::" src/
grep -r "EXPECT_EQ" src/
grep -r "ASSERT_NE" src/

# Search for test CMake configuration
find . -name "*Test*.cmake"
find . -name "CMakeLists.txt" -exec grep -l "gtest" {} \;
```

**If Infrastructure Exists:**
- ✅ Reuse existing factories and utilities
- ✅ Follow established patterns
- ✅ Integrate with existing test runner

**If Infrastructure Missing:**
- ⚠️ Propose minimal test infrastructure design
- ⚠️ Implement only what's needed for UnifiedInterruptSystem
- ⚠️ Design for future reuse by other modules

---

## Minimal Infrastructure Option

**If full infrastructure is unavailable**, implement minimal test utilities:

### Minimal Player Mock
```cpp
class MinimalPlayerMock : public Player
{
public:
    MinimalPlayerMock(ObjectGuid guid)
        : Player(nullptr), _guid(guid), _spells()
    {
    }

    ObjectGuid GetGUID() const override { return _guid; }

    void AddSpell(uint32 spellId)
    {
        _spells[spellId] = PlayerSpell();
    }

    PlayerSpellMap const& GetSpellMap() const override
    {
        return _spells;
    }

private:
    ObjectGuid _guid;
    PlayerSpellMap _spells;
};
```

### Minimal Unit Mock
```cpp
class MinimalUnitMock : public Unit
{
public:
    MinimalUnitMock(ObjectGuid guid, float x, float y, float z)
        : Unit(false), _guid(guid)
    {
        Relocate(x, y, z);
    }

    ObjectGuid GetGUID() const override { return _guid; }

private:
    ObjectGuid _guid;
};
```

**Advantages:**
- ✅ Quick to implement (1-2 days)
- ✅ Minimal dependencies
- ✅ Sufficient for basic testing

**Disadvantages:**
- ⚠️ Limited functionality (may not cover all test cases)
- ⚠️ Requires maintenance if TrinityCore APIs change
- ⚠️ Not reusable for other modules without refactoring

---

## Documentation & Maintenance

### Test Documentation Requirements

**For Each Enabled Test:**
- Document what is being tested
- Document expected behavior
- Document pass/fail criteria
- Document any known limitations

**Example Test Documentation:**
```cpp
/**
 * @test BotRegistration
 * @brief Verifies that bots can be successfully registered with the UnifiedInterruptSystem
 *
 * @details
 * Creates a test Player with interrupt spells (Kick), creates a mock BotAI, and
 * registers the bot. Verifies that:
 * - Registration succeeds without errors
 * - Bot's interrupt spell is correctly identified
 * - Bot appears in registered bots map
 * - Metrics are updated correctly
 *
 * @requirements
 * - PlayerTestFactory for Player creation
 * - BotAITestFactory for BotAI mock
 * - SpellMgr with interrupt spell data (SPELL_KICK = 1766)
 *
 * @pass_criteria
 * - Registration returns true
 * - botsRegistered metric increments by 1
 * - Bot's spellId is set to SPELL_KICK
 */
TEST_F(UnifiedInterruptSystemTest, BotRegistration)
{
    // Test implementation...
}
```

### Maintenance Plan

**Regular Maintenance:**
- Review test results monthly
- Update tests when UnifiedInterruptSystem APIs change
- Add new tests for new features
- Refactor tests if patterns change

**Version Control:**
- Tag test suite versions with UnifiedInterruptSystem versions
- Document test coverage percentage
- Track test execution time trends

---

## Appendix: Test Infrastructure Implementation Examples

### Complete PlayerTestFactory Implementation

```cpp
// PlayerTestFactory.h
#pragma once

#include "Player.h"
#include "ObjectGuid.h"

class PlayerTestFactory
{
public:
    static Player* CreateTestPlayer(
        ObjectGuid guid,
        uint8 race = RACE_HUMAN,
        uint8 class_ = CLASS_WARRIOR,
        uint8 level = 80,
        Map* map = nullptr
    );

    static void DestroyTestPlayer(Player* player);

    static void AddSpellToPlayer(Player* player, uint32 spellId);
    static void RemoveSpellFromPlayer(Player* player, uint32 spellId);

    static void SetPlayerPosition(Player* player, float x, float y, float z);
    static void SetPlayerLevel(Player* player, uint8 level);
    static void SetPlayerGroup(Player* player, Group* group);

    static void AddAura(Player* player, uint32 spellId, Unit* caster = nullptr);
    static void RemoveAura(Player* player, uint32 spellId);

private:
    static std::vector<Player*> _createdPlayers;
    static void RegisterPlayer(Player* player);
};

// PlayerTestFactory.cpp
#include "PlayerTestFactory.h"
#include "Map.h"
#include "SpellMgr.h"
#include "SpellInfo.h"

std::vector<Player*> PlayerTestFactory::_createdPlayers;

Player* PlayerTestFactory::CreateTestPlayer(
    ObjectGuid guid,
    uint8 race,
    uint8 class_,
    uint8 level,
    Map* map)
{
    // Create minimal Player instance
    Player* player = new Player(nullptr);

    // Set basic properties
    player->Create(guid);
    player->SetRace(race);
    player->SetClass(class_);
    player->SetLevel(level);

    if (map)
        player->SetMap(map);

    // Initialize spell book
    player->InitializeSpells();

    RegisterPlayer(player);
    return player;
}

void PlayerTestFactory::DestroyTestPlayer(Player* player)
{
    if (!player)
        return;

    // Remove from tracking
    auto it = std::find(_createdPlayers.begin(), _createdPlayers.end(), player);
    if (it != _createdPlayers.end())
        _createdPlayers.erase(it);

    delete player;
}

void PlayerTestFactory::AddSpellToPlayer(Player* player, uint32 spellId)
{
    if (!player)
        return;

    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId, DIFFICULTY_NONE);
    if (!spellInfo)
        return;

    player->AddSpell(spellId, true, false, false);
}

void PlayerTestFactory::SetPlayerPosition(Player* player, float x, float y, float z)
{
    if (!player)
        return;

    player->Relocate(x, y, z);
}

void PlayerTestFactory::RegisterPlayer(Player* player)
{
    _createdPlayers.push_back(player);
}
```

---

## Summary

**Test Framework Status:**
- ✅ Test file created (900+ lines, 32 tests)
- ✅ Test architecture designed
- ⏸️ Infrastructure requirements documented
- ⏸️ 27 integration tests awaiting infrastructure

**Infrastructure Required:**
1. Object Factories (Player, Unit, Group, BotAI)
2. Database & Spell Data Initialization (SpellMgr setup)
3. Map & World Environment (Map factory)
4. Threading & Timing Utilities
5. Mock Dependencies (SpellPacketBuilder, MovementArbiter)

**Estimated Effort:**
- Infrastructure Setup: 2 weeks
- Mock Implementations: 1 week
- Test Enablement: 2 weeks
- Performance Validation: 1 week
- **Total**: 6 weeks for complete test infrastructure

**Next Steps:**
1. Investigate existing TrinityCore test infrastructure
2. Implement required factories and utilities
3. Enable disabled tests incrementally
4. Validate full test suite execution
5. Performance benchmarking

**Documentation Complete**: This document provides all necessary information to implement the test infrastructure and enable the 27 disabled integration tests.

---

**Document Version**: 1.0
**Last Updated**: 2025-11-01
**Status**: ✅ Complete
**Next Phase**: Infrastructure Implementation
