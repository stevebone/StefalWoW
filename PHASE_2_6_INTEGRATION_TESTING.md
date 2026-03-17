# Phase 2.6: Integration Testing

**Duration**: 1 week (2025-02-24 to 2025-03-03)
**Status**: ⏳ PENDING
**Owner**: Development Team

---

## Objectives

Comprehensive testing of the complete refactored system:
1. Test all combat scenarios (solo, group, raid)
2. Test movement transitions (idle → combat → follow)
3. Test strategy activation/deactivation
4. Load testing with 100, 500, 1000, 5000 bots
5. Performance profiling and optimization
6. Bug fixes and edge case handling

---

## Background

### What We're Testing

After Phases 2.1-2.5, we have a completely refactored system:
- ✅ BehaviorManager base class (Phase 2.1)
- ✅ CombatMovementStrategy (Phase 2.2)
- ✅ Universal ClassAI activation (Phase 2.3)
- ✅ 4 refactored managers (Quest, Trade, Gathering, Auction) (Phase 2.4)
- ✅ Observer pattern IdleStrategy (Phase 2.5)

### Integration Points to Validate

**Update Flow**:
```
World::Update()
  → BotSession::Update()
    → BotAI::UpdateAI()
      → UpdateManagers()        // Phase 2.4
        → QuestManager::Update()
        → TradeManager::Update()
        → GatheringManager::Update()
        → AuctionManager::Update()
      → UpdateStrategies()      // Phase 2.5
        → IdleStrategy::UpdateBehavior()  // Observes managers
        → LeaderFollowBehavior::UpdateBehavior()
        → CombatMovementStrategy::UpdateBehavior()  // Phase 2.2
      → UpdateMovement()
      → OnCombatUpdate()        // Phase 2.3
        → ClassAI::Update()
```

### Success Criteria
- All systems work together seamlessly
- No performance regressions
- 5000 bots supported
- All edge cases handled
- Zero critical bugs

---

## Technical Requirements

### Performance Targets
- <2ms per bot per frame (total system)
- <0.8ms for all managers combined
- <0.1ms for all strategies combined
- <1ms for ClassAI combat logic
- <10% CPU usage with 5000 bots on 8-core server

### Test Coverage Requirements
- 100% of refactored systems tested
- All 13 classes tested in combat
- All 4 managers tested with real scenarios
- All strategies tested in isolation and combination
- All edge cases documented and tested

### Stability Requirements
- Zero crashes in 72-hour stress test
- Zero memory leaks detected
- Zero deadlocks detected
- Graceful degradation under extreme load
- Proper error handling for all failure modes

---

## Deliverables

### 1. Solo Combat Test Suite
Location: `tests/integration/SoloCombatTests.cpp`

**Test Matrix** (all 13 classes):
```cpp
TEST(SoloCombat, WarriorSoloQuesting)
{
    // Setup
    Bot* warrior = CreateTestBot(CLASS_WARRIOR, 30);
    Quest* quest = AssignQuest(warrior, QUEST_KILL_10_BOARS);

    // Execute: Bot should quest autonomously
    RunFor(300000); // 5 minutes

    // Verify
    ASSERT_GT(warrior->GetQuestKillCount(BOAR_NPC_ID), 0);
    ASSERT_TRUE(warrior->UsedSpell(HEROIC_STRIKE));
    ASSERT_TRUE(warrior->UsedSpell(REND));
    ASSERT_LT(GetAverageFrameTime(warrior), 2000); // <2ms per frame
}

// Repeat for all 13 classes with class-specific spell checks
```

**Classes to Test**:
- Warrior, Paladin, Hunter, Rogue, Priest
- Shaman, Mage, Warlock, Druid
- Death Knight, Monk, Demon Hunter, Evoker

### 2. Group Combat Test Suite
Location: `tests/integration/GroupCombatTests.cpp`

**Test Scenarios**:
```cpp
TEST(GroupCombat, FiveManDungeonClear)
{
    // Setup: Tank, Healer, 3 DPS
    Group* group = CreateTestGroup();
    Bot* tank = AddBot(group, CLASS_WARRIOR, SPEC_PROTECTION);
    Bot* healer = AddBot(group, CLASS_PRIEST, SPEC_HOLY);
    Bot* dps1 = AddBot(group, CLASS_MAGE, SPEC_FIRE);
    Bot* dps2 = AddBot(group, CLASS_ROGUE, SPEC_COMBAT);
    Bot* dps3 = AddBot(group, CLASS_HUNTER, SPEC_MARKSMANSHIP);

    // Enter dungeon
    TeleportGroupToDungeon(group, DEADMINES);

    // Pull first boss
    PullBoss(group, BOSS_RHAHKZOR);

    // Wait for combat to complete
    WaitForCombatEnd(group, 120000);

    // Verify
    ASSERT_TRUE(BOSS_RHAHKZOR->isDead());
    ASSERT_TRUE(tank->IsAlive());
    ASSERT_TRUE(healer->IsAlive());
    ASSERT_GT(tank->GetThreat(BOSS_RHAHKZOR), dps1->GetThreat(BOSS_RHAHKZOR));
    ASSERT_GT(healer->GetHealingDone(), 10000);

    // Verify combat movement
    ASSERT_LT(tank->GetDistance(BOSS_RHAHKZOR), 7.0f); // Melee range
    ASSERT_GT(healer->GetDistance(BOSS_RHAHKZOR), 15.0f); // Healing range
    ASSERT_GT(dps1->GetDistance(BOSS_RHAHKZOR), 20.0f); // Ranged
}

TEST(GroupCombat, TwentyFiveManRaid)
{
    // Setup: 25-bot raid composition
    Raid* raid = CreateTestRaid(25);

    // Test raid mechanics
    // ... (full raid testing)
}
```

### 3. Movement Transition Tests
Location: `tests/integration/MovementTransitionTests.cpp`

**Test Scenarios**:
```cpp
TEST(MovementTransition, IdleToFollowToIdle)
{
    // Setup: Solo bot
    Bot* bot = CreateTestBot(CLASS_WARRIOR, 30);
    ASSERT_TRUE(bot->GetAI()->HasActiveStrategy("idle"));

    // Action 1: Player invites bot to group
    Player* player = GetTestPlayer();
    player->GetGroup()->AddMember(bot);

    WaitFor(1000);

    // Verify: IdleStrategy deactivated, LeaderFollowBehavior activated
    ASSERT_FALSE(bot->GetAI()->HasActiveStrategy("idle"));
    ASSERT_TRUE(bot->GetAI()->HasActiveStrategy("follow"));

    // Action 2: Bot leaves group
    player->GetGroup()->RemoveMember(bot->GetGUID());

    WaitFor(1000);

    // Verify: Back to idle
    ASSERT_TRUE(bot->GetAI()->HasActiveStrategy("idle"));
    ASSERT_FALSE(bot->GetAI()->HasActiveStrategy("follow"));
}

TEST(MovementTransition, FollowToCombatToFollow)
{
    // Setup: Bot following player
    Player* player = GetTestPlayer();
    Bot* bot = CreateTestBot(CLASS_WARRIOR, 30);
    player->GetGroup()->AddMember(bot);

    WaitFor(1000);
    ASSERT_TRUE(bot->GetAI()->HasActiveStrategy("follow"));

    // Action: Player attacks mob
    Creature* mob = SpawnMob(MOB_DEFIAS_BANDIT, player->GetPosition());
    player->Attack(mob, true);

    WaitFor(5000); // Wait for combat

    // Verify: CombatMovementStrategy takes over (higher priority)
    ASSERT_TRUE(bot->IsInCombat());
    ASSERT_LT(bot->GetDistance(mob), 7.0f); // Warrior moved to melee

    // Wait for combat to end
    WaitForCombatEnd(bot, 30000);

    // Verify: Back to following
    ASSERT_FALSE(bot->IsInCombat());
    ASSERT_TRUE(bot->GetAI()->HasActiveStrategy("follow"));
    ASSERT_LT(bot->GetDistance(player), 10.0f); // Following again
}
```

### 4. Manager Integration Tests
Location: `tests/integration/ManagerIntegrationTests.cpp`

**Test Scenarios**:
```cpp
TEST(ManagerIntegration, QuestManagerWithIdleStrategy)
{
    // Setup: Bot with quest
    Bot* bot = CreateTestBot(CLASS_WARRIOR, 20);
    bot->GetQuestManager()->AddQuest(QUEST_KILL_10_KOBOLDS);

    // Verify: QuestManager state
    ASSERT_TRUE(bot->GetQuestManager()->IsQuestingActive());

    // Run idle strategy for 100 frames
    for (int i = 0; i < 100; ++i)
    {
        bot->GetAI()->UpdateAI(100);
    }

    // Verify: IdleStrategy observed quest state (didn't call Update)
    ASSERT_EQ(bot->GetQuestManager()->GetUpdateCount(), 5); // Only ~5 updates (throttled to 2s)
    ASSERT_LT(GetAverageFrameTime(bot), 2000); // Fast
}

TEST(ManagerIntegration, AllManagersWithIdleStrategy)
{
    // Setup: Bot with all systems active
    Bot* bot = CreateTestBot(CLASS_DRUID, 40);
    bot->GetQuestManager()->AddQuest(QUEST_GATHER_HERBS);
    bot->GetTradeManager()->SetNeedsRepair(true);
    bot->GetGatheringManager()->SpawnNearbyHerb();

    // Run for 10 seconds (100 frames at 100ms each)
    for (int i = 0; i < 100; ++i)
    {
        bot->GetAI()->UpdateAI(100);
    }

    // Verify: All managers updated (throttled)
    ASSERT_GT(bot->GetQuestManager()->GetUpdateCount(), 0);
    ASSERT_GT(bot->GetGatheringManager()->GetUpdateCount(), 0);
    ASSERT_GT(bot->GetTradeManager()->GetUpdateCount(), 0);

    // Verify: Performance still good
    ASSERT_LT(GetAverageFrameTime(bot), 2000); // <2ms per frame
}
```

### 5. Load Testing Suite
Location: `tests/stress/LoadTests.cpp`

**Test Scenarios**:
```cpp
TEST(LoadTest, OneHundredBots)
{
    // Spawn 100 bots with mixed activities
    std::vector<Bot*> bots = SpawnBots(100);

    // Mix of activities
    for (int i = 0; i < 100; ++i)
    {
        if (i % 4 == 0) AssignQuest(bots[i], RandomQuest());
        if (i % 4 == 1) StartGathering(bots[i]);
        if (i % 4 == 2) StartCombat(bots[i], SpawnMob());
        if (i % 4 == 3) CreateGroup(bots[i], bots[(i+1) % 100]);
    }

    // Run for 10 minutes
    RunFor(600000);

    // Verify: Performance
    ASSERT_LT(GetAverageServerCPU(), 5.0); // <5% CPU
    ASSERT_LT(GetAverageFrameTime(), 2000); // <2ms per bot
    ASSERT_EQ(GetCrashCount(), 0);
}

TEST(LoadTest, FiveHundredBots)
{
    // ... similar to above but with 500 bots
    ASSERT_LT(GetAverageServerCPU(), 8.0); // <8% CPU
}

TEST(LoadTest, OneThousandBots)
{
    // ... similar to above but with 1000 bots
    ASSERT_LT(GetAverageServerCPU(), 10.0); // <10% CPU
}

TEST(LoadTest, FiveThousandBots)
{
    // ... ultimate stress test
    ASSERT_LT(GetAverageServerCPU(), 15.0); // <15% CPU acceptable for extreme load
}
```

### 6. Performance Profiling Report
Location: `docs/PERFORMANCE_PROFILE_REPORT.md`

**Content**:
- Profiler results for each system component
- CPU usage breakdown by system
- Memory usage analysis
- Hotspot identification
- Optimization recommendations

### 7. Edge Case Tests
Location: `tests/integration/EdgeCaseTests.cpp`

**Test Scenarios**:
```cpp
TEST(EdgeCase, BotDisconnectDuringCombat)
TEST(EdgeCase, BotTeleportDuringGathering)
TEST(EdgeCase, QuestRemovedWhileActive)
TEST(EdgeCase, GroupDisbandedDuringDungeon)
TEST(EdgeCase, ServerShutdownWithActiveBots)
TEST(EdgeCase, MemoryPressureWithMaxBots)
TEST(EdgeCase, DuplicateStrategyActivation)
TEST(EdgeCase, NullPointerChecks)
```

### 8. Bug Tracking Document
Location: `docs/INTEGRATION_BUGS_FOUND.md`

**Format**:
```markdown
## Bug #1: Title
**Severity**: Critical/Major/Minor
**Component**: BotAI/Manager/Strategy/Movement
**Description**: ...
**Steps to Reproduce**: ...
**Expected**: ...
**Actual**: ...
**Fix**: ...
**Status**: Open/Fixed/Verified
```

---

## Implementation Steps

### Day 1: Solo Combat Testing
- Test all 13 classes in solo combat
- Verify ClassAI spell usage
- Verify combat movement
- Fix any bugs found

### Day 2: Group Combat Testing
- Test 5-man dungeon groups
- Test 10-man raid groups
- Test 25-man raid groups
- Verify all roles work correctly

### Day 3: Movement Transition Testing
- Test idle ↔ follow transitions
- Test follow ↔ combat transitions
- Test combat ↔ idle transitions
- Test complex multi-transition scenarios

### Day 4: Manager Integration Testing
- Test each manager individually
- Test all managers together
- Test manager/strategy interaction
- Verify observer pattern works

### Day 5: Load Testing (100-1000 bots)
- Spawn 100 bots, run for 30 minutes
- Spawn 500 bots, run for 30 minutes
- Spawn 1000 bots, run for 30 minutes
- Profile performance at each level

### Day 6: Extreme Load Testing (5000 bots)
- Spawn 5000 bots
- Run for 2 hours
- Monitor CPU, memory, frame times
- Identify bottlenecks
- Optimize if needed

### Day 7: Bug Fixes and Report
- Fix all critical bugs
- Fix all major bugs
- Document minor bugs for later
- Write performance report
- Write integration test summary

---

## Success Criteria

### Functional Requirements
- ✅ All 13 classes work in solo combat
- ✅ All 13 classes work in group combat
- ✅ 5-man groups clear dungeons successfully
- ✅ 25-man raids work correctly
- ✅ Movement transitions work smoothly
- ✅ All managers integrate properly
- ✅ Observer pattern works as designed

### Performance Requirements
- ✅ <2ms average per bot per frame (100 bots)
- ✅ <2.5ms average per bot per frame (500 bots)
- ✅ <3ms average per bot per frame (1000 bots)
- ✅ <5ms average per bot per frame (5000 bots)
- ✅ <10% CPU usage (8-core server, 5000 bots)
- ✅ <10GB total memory (5000 bots = 2MB per bot)

### Stability Requirements
- ✅ Zero crashes in 72-hour test
- ✅ Zero memory leaks detected
- ✅ Zero deadlocks detected
- ✅ Graceful handling of edge cases
- ✅ Proper error logging

### Code Quality
- ✅ All tests pass
- ✅ 90%+ code coverage
- ✅ All critical bugs fixed
- ✅ Performance targets met
- ✅ Documentation complete

---

## Test Environment Setup

### Hardware Requirements
- **CPU**: 8-core processor (minimum)
- **RAM**: 32GB (minimum) for 5000 bot testing
- **Storage**: SSD recommended
- **OS**: Windows Server 2019+ or Linux (Ubuntu 22.04+)

### Software Requirements
- **TrinityCore**: Latest playerbot-dev branch
- **MySQL**: 9.4+
- **Profiler**: Visual Studio Profiler or Valgrind
- **Memory Checker**: Dr. Memory or Valgrind
- **Test Framework**: Google Test

### Database Setup
```sql
-- Create test accounts (100 accounts for 1000 bots = 10 bots per account)
INSERT INTO auth.account (username, sha_pass_hash, email)
VALUES
('botaccount1', SHA1('password'), 'bot1@test.com'),
('botaccount2', SHA1('password'), 'bot2@test.com'),
-- ... repeat for 100 accounts
('botaccount100', SHA1('password'), 'bot100@test.com');

-- Create test characters (1000 bots)
-- Distributed across all classes and levels
```

---

## Performance Profiling Plan

### Tools
- **Windows**: Visual Studio Performance Profiler
- **Linux**: Valgrind, perf, gprof

### Metrics to Collect
- CPU usage per system component
- Memory usage per bot
- Frame time breakdown (UpdateManagers, UpdateStrategies, OnCombatUpdate)
- Hottest functions (top 20)
- Cache miss rates
- Lock contention (if any)

### Profiling Sessions
1. **100 bots** - Detailed profiling (1 hour)
2. **500 bots** - Performance profiling (1 hour)
3. **1000 bots** - Stress profiling (2 hours)
4. **5000 bots** - Extreme stress profiling (4 hours)

---

## Bug Severity Classification

### Critical (Must Fix)
- Crashes
- Deadlocks
- Memory leaks >10MB/hour
- Complete system failure

### Major (Should Fix)
- Performance >2x target
- Feature not working
- Major edge case failures
- Memory leaks 1-10MB/hour

### Minor (Nice to Fix)
- Small performance issues
- Edge case failures that are rare
- Visual glitches
- Log spam

---

## Dependencies

### Requires
- Phase 2.1 complete (BehaviorManager)
- Phase 2.2 complete (CombatMovementStrategy)
- Phase 2.3 complete (Universal ClassAI)
- Phase 2.4 complete (All managers refactored)
- Phase 2.5 complete (Observer pattern IdleStrategy)

### Blocks
- Phase 2.7 (Cleanup - needs stable system first)
- Phase 2.8 (Documentation - needs final system state)

---

## Risk Mitigation

### Risk: Critical bugs discovered late
**Mitigation**: Test early and often, fix bugs as found

### Risk: Performance targets not met
**Mitigation**: Profile continuously, optimize hotspots

### Risk: Extreme load causes instability
**Mitigation**: Graceful degradation, load shedding if needed

### Risk: Edge cases cause crashes
**Mitigation**: Comprehensive edge case testing, robust error handling

---

## Next Phase

After completion, proceed to **Phase 2.7: Cleanup & Consolidation**

---

**Last Updated**: 2025-01-13
**Next Review**: 2025-03-03
