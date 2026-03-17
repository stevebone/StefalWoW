# Phase 2.5: Update IdleStrategy - Observer Pattern

**Duration**: 1 week (2025-02-17 to 2025-02-24)
**Status**: ⏳ PENDING
**Owner**: Development Team

---

## Objectives

Transform IdleStrategy from heavyweight delegator to lightweight observer:
1. Remove all Automation::instance() calls
2. Replace with fast manager state queries
3. Eliminate throttling timers (no longer needed)
4. Achieve <0.1ms per update (from 100ms+)
5. Implement true observer pattern

---

## Background

### Current Problem: Strategy Does Heavyweight Work

**IdleStrategy.cpp (Current)**:
```cpp
void IdleStrategy::UpdateBehavior(BotAI* ai, uint32 diff)
{
    Player* bot = ai->GetBot();
    uint32 currentTime = getMSTime();

    // ❌ Calls expensive singleton operations
    if (currentTime - _lastQuestUpdate > 2000)
    {
        QuestAutomation::instance()->UpdateBotAutomation(bot, diff);  // 50-100ms!
        _lastQuestUpdate = currentTime;
    }

    if (currentTime - _lastGatheringUpdate > 1000)
    {
        GatheringAutomation::instance()->Update(bot, diff);  // 50-100ms!
        _lastGatheringUpdate = currentTime;
    }

    // ... etc for Trade, Auction
}
```

**Problems**:
1. **Performance**: Even throttled, takes 0-144ms per update
2. **Architecture**: Strategy shouldn't do heavyweight work
3. **Complexity**: Strategy manages timers for external systems
4. **Redundancy**: Managers already have their own throttling in Phase 2.4
5. **Scalability**: Doesn't scale to 5000+ bots

### Solution: Lightweight Observer Pattern

**IdleStrategy.cpp (New)**:
```cpp
void IdleStrategy::UpdateBehavior(BotAI* ai, uint32 diff)
{
    // ✅ Fast state queries only (atomic reads, <0.001ms each)
    QuestManager* questMgr = ai->GetQuestManager();
    GatheringManager* gatherMgr = ai->GetGatheringManager();
    TradeManager* tradeMgr = ai->GetTradeManager();

    // Priority 1: Quest behavior (observe state only)
    if (questMgr && questMgr->IsQuestingActive())
    {
        // Quest movement/behavior handled by managers
        // Strategy just observes and coordinates
        return;  // Questing has priority
    }

    // Priority 2: Gathering behavior
    if (gatherMgr && gatherMgr->HasNearbyResources())
    {
        // Gathering handled by manager
        return;
    }

    // Priority 3: Trading behavior
    if (tradeMgr && tradeMgr->NeedsRepair())
    {
        // Trading handled by manager
        return;
    }

    // Priority 4: Fallback - Simple wander
    DoWander(ai, diff);
}
```

**Benefits**:
- ✅ <0.1ms per update (vs 100ms+)
- ✅ No throttling needed (queries are instant)
- ✅ True observer pattern (strategies observe, managers work)
- ✅ Clean separation of concerns
- ✅ Scalable to 5000+ bots

---

## Technical Requirements

### Performance Constraints
- IdleStrategy::UpdateBehavior(): <0.1ms per call
- Manager state query: <0.001ms per call (atomic read)
- Total idle strategy overhead: <0.1ms per bot per frame
- Must call every frame (no throttling)

### Observer Pattern Principles
1. **Strategies Observe**: Read state from managers (fast queries)
2. **Managers Work**: Do heavyweight operations (throttled)
3. **No Cross-Calling**: Strategies never call manager Update() methods
4. **Atomic State**: All state queries are atomic (thread-safe, lock-free)

### Integration Points
- Requires all 4 managers from Phase 2.4 (Quest, Trade, Gathering, Auction)
- Managers already updating via BotAI::UpdateManagers()
- IdleStrategy just observes results

---

## Deliverables

### 1. Refactored IdleStrategy.h
Location: `src/modules/Playerbot/AI/Strategy/IdleStrategy.h`

**Before** (with throttling timers):
```cpp
class IdleStrategy : public Strategy
{
private:
    uint32 _lastWanderTime = 0;
    uint32 _wanderInterval = 30000;

    // ❌ Throttling timers - not needed anymore
    uint32 _lastQuestUpdate = 0;
    uint32 _questUpdateInterval = 2000;
    uint32 _lastGatheringUpdate = 0;
    uint32 _gatheringUpdateInterval = 1000;
    uint32 _lastTradeUpdate = 0;
    uint32 _tradeUpdateInterval = 5000;
    uint32 _lastAuctionUpdate = 0;
    uint32 _auctionUpdateInterval = 10000;
};
```

**After** (clean, no timers):
```cpp
class IdleStrategy : public Strategy
{
public:
    IdleStrategy();
    ~IdleStrategy() override = default;

    void InitializeActions() override;
    void InitializeTriggers() override;
    void InitializeValues() override;

    void OnActivate(BotAI* ai) override;
    void OnDeactivate(BotAI* ai) override;
    bool IsActive(BotAI* ai) const override;

    void UpdateBehavior(BotAI* ai, uint32 diff) override;

private:
    // Wander behavior
    void DoWander(BotAI* ai, uint32 diff);

    // State
    uint32 _lastWanderTime = 0;
    uint32 _wanderInterval = 30000;
};
```

### 2. Refactored IdleStrategy.cpp
Location: `src/modules/Playerbot/AI/Strategy/IdleStrategy.cpp`

**Complete New Implementation**:
```cpp
#include "IdleStrategy.h"
#include "BotAI.h"
#include "Player.h"
#include "Log.h"
#include "Game/QuestManager.h"
#include "Social/TradeManager.h"
#include "Professions/GatheringManager.h"
#include "Economy/AuctionManager.h"

namespace Playerbot
{

IdleStrategy::IdleStrategy() : Strategy("idle")
{
    SetPriority(50); // Lower than group strategies
}

void IdleStrategy::InitializeActions()
{
    // TODO: Add idle actions if needed
}

void IdleStrategy::InitializeTriggers()
{
    // TODO: Add idle triggers if needed
}

void IdleStrategy::InitializeValues()
{
    // TODO: Add idle values if needed
}

void IdleStrategy::OnActivate(BotAI* ai)
{
    if (!ai || !ai->GetBot())
        return;

    TC_LOG_INFO("module.playerbot", "Idle strategy activated for bot {}", ai->GetBot()->GetName());
    SetActive(true);
}

void IdleStrategy::OnDeactivate(BotAI* ai)
{
    if (!ai || !ai->GetBot())
        return;

    TC_LOG_INFO("module.playerbot", "Idle strategy deactivated for bot {}", ai->GetBot()->GetName());
    SetActive(false);
}

bool IdleStrategy::IsActive(BotAI* ai) const
{
    if (!ai || !ai->GetBot())
        return false;

    // Active when not in a group
    return _active && !ai->GetBot()->GetGroup();
}

void IdleStrategy::UpdateBehavior(BotAI* ai, uint32 diff)
{
    if (!ai || !ai->GetBot())
        return;

    // OBSERVER PATTERN: Query manager state (fast atomic reads)
    // Managers already updating via BotAI::UpdateManagers()
    // Strategy just observes and coordinates

    // Get manager references
    QuestManager* questMgr = ai->GetQuestManager();
    GatheringManager* gatherMgr = ai->GetGatheringManager();
    TradeManager* tradeMgr = ai->GetTradeManager();
    AuctionManager* auctionMgr = ai->GetAuctionManager();

    // Priority 1: Active questing (highest priority)
    if (questMgr && questMgr->IsQuestingActive())
    {
        // Quest manager is handling quest logic
        // Bot is moving to objectives, killing mobs, etc.
        // Strategy just needs to observe - don't interfere
        static uint32 questCounter = 0;
        if (++questCounter % 100 == 0)
        {
            TC_LOG_DEBUG("module.playerbot", "🎯 IdleStrategy: Bot {} actively questing",
                        ai->GetBot()->GetName());
        }
        return;
    }

    // Priority 2: Gathering resources (if available)
    if (gatherMgr && gatherMgr->HasNearbyResources())
    {
        // Gathering manager is handling resource collection
        // Strategy just observes
        static uint32 gatherCounter = 0;
        if (++gatherCounter % 100 == 0)
        {
            TC_LOG_DEBUG("module.playerbot", "⛏️ IdleStrategy: Bot {} gathering resources",
                        ai->GetBot()->GetName());
        }
        return;
    }

    // Priority 3: Trading/repair (if needed)
    if (tradeMgr && tradeMgr->NeedsRepair())
    {
        // Trade manager is handling vendor interaction
        // Strategy just observes
        static uint32 tradeCounter = 0;
        if (++tradeCounter % 100 == 0)
        {
            TC_LOG_DEBUG("module.playerbot", "🔧 IdleStrategy: Bot {} needs repair",
                        ai->GetBot()->GetName());
        }
        return;
    }

    // Priority 4: Auction house (if near and has items to sell)
    if (auctionMgr && auctionMgr->HasActiveAuctions())
    {
        // Auction manager is handling AH interaction
        static uint32 auctionCounter = 0;
        if (++auctionCounter % 100 == 0)
        {
            TC_LOG_DEBUG("module.playerbot", "💰 IdleStrategy: Bot {} using auction house",
                        ai->GetBot()->GetName());
        }
        return;
    }

    // Priority 5: Fallback - Simple wandering behavior
    DoWander(ai, diff);
}

void IdleStrategy::DoWander(BotAI* ai, uint32 diff)
{
    if (!ai || !ai->GetBot())
        return;

    uint32 currentTime = getMSTime();
    if (currentTime - _lastWanderTime < _wanderInterval)
        return;

    _lastWanderTime = currentTime;

    // TODO: Implement proper wandering with pathfinding
    // For now, just log that bot is idle
    static uint32 idleCounter = 0;
    if (++idleCounter % 100 == 0)
    {
        TC_LOG_DEBUG("module.playerbot", "💤 IdleStrategy: Bot {} is wandering (truly idle)",
                    ai->GetBot()->GetName());
    }
}

} // namespace Playerbot
```

### 3. Remove Automation Includes
Location: `src/modules/Playerbot/AI/Strategy/IdleStrategy.cpp`

**Before**:
```cpp
#include "Quest/QuestAutomation.h"
#include "Professions/GatheringAutomation.h"
#include "Social/TradeAutomation.h"
#include "Social/AuctionAutomation.h"
```

**After**:
```cpp
#include "Game/QuestManager.h"
#include "Social/TradeManager.h"
#include "Professions/GatheringManager.h"
#include "Economy/AuctionManager.h"
```

### 4. Performance Tests
Location: `tests/performance/IdleStrategyPerformanceTest.cpp`

**Test Benchmark**:
```cpp
TEST(IdleStrategyPerformance, UpdateBehaviorUnder100Microseconds)
{
    // Setup: 100 bots with idle strategy
    std::vector<Bot*> bots;
    for (int i = 0; i < 100; ++i)
    {
        Bot* bot = CreateTestBot(CLASS_WARRIOR, 30);
        bot->GetAI()->ActivateStrategy("idle");
        bots.push_back(bot);
    }

    // Benchmark: 1000 update cycles
    auto start = std::chrono::high_resolution_clock::now();

    for (int cycle = 0; cycle < 1000; ++cycle)
    {
        for (Bot* bot : bots)
        {
            bot->GetAI()->GetStrategy("idle")->UpdateBehavior(bot->GetAI(), 100);
        }
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    // Calculate average per-bot update time
    double avgPerUpdate = duration.count() / (100.0 * 1000.0);

    TC_LOG_INFO("test", "IdleStrategy avg update time: {} microseconds", avgPerUpdate);

    // Assert: Must be under 100 microseconds (0.1ms)
    ASSERT_LT(avgPerUpdate, 100.0) << "IdleStrategy update took " << avgPerUpdate << "us, exceeds 100us limit";
}
```

### 5. Integration Tests
Location: `tests/integration/IdleStrategyIntegrationTest.cpp`

**Test Scenarios**:
```cpp
TEST(IdleStrategyIntegration, PrioritizesQuesting)
{
    // Setup: Bot with active quest
    Bot* bot = CreateTestBot(CLASS_WARRIOR, 20);
    bot->GetAI()->GetQuestManager()->AddQuest(12345);
    bot->GetAI()->ActivateStrategy("idle");

    // Execute 100 updates
    for (int i = 0; i < 100; ++i)
        bot->GetAI()->UpdateAI(100);

    // Verify: Bot is following quest logic
    ASSERT_TRUE(bot->GetAI()->GetQuestManager()->IsQuestingActive());
}

TEST(IdleStrategyIntegration, FallsBackToWander)
{
    // Setup: Bot with no active tasks
    Bot* bot = CreateTestBot(CLASS_WARRIOR, 20);
    bot->GetAI()->ActivateStrategy("idle");

    // Verify: No active systems
    ASSERT_FALSE(bot->GetAI()->GetQuestManager()->IsQuestingActive());
    ASSERT_FALSE(bot->GetAI()->GetGatheringManager()->HasNearbyResources());

    // Execute updates
    for (int i = 0; i < 100; ++i)
        bot->GetAI()->UpdateAI(100);

    // Verify: Bot executes wander behavior (logged)
    // (Check logs for wander messages)
}
```

### 6. Documentation
Location: `docs/OBSERVER_PATTERN_GUIDE.md`

**Content**:
- Observer pattern explanation
- Manager/Strategy separation principles
- State query best practices
- Performance benefits explanation
- Migration from delegation to observation
- Troubleshooting guide

---

## Implementation Steps

### Day 1: Remove Automation Calls
1. Open IdleStrategy.cpp
2. Delete all Automation::instance() calls
3. Delete all throttling timer logic
4. Add manager state queries
5. Verify compilation

### Day 2: Implement Observer Pattern
1. Rewrite UpdateBehavior() with priority logic
2. Use manager state queries only
3. Add debug logging (temporary)
4. Test with 10 bots
5. Verify <0.1ms per update

### Day 3: Test Priority System
1. Test quest priority (highest)
2. Test gathering priority
3. Test trade priority
4. Test auction priority
5. Test wander fallback

### Day 4: Performance Testing
1. Benchmark with 100 bots
2. Benchmark with 500 bots
3. Benchmark with 1000 bots
4. Profile UpdateBehavior()
5. Verify <0.1ms per update

### Day 5: Integration Testing
1. Test with all managers active
2. Test priority transitions (quest → wander)
3. Test with mixed bot activities
4. Verify no regressions
5. Test 5000 bots (stress test)

### Day 6: Cleanup and Optimization
1. Remove temporary debug logging
2. Optimize state query order
3. Clean up code comments
4. Final code review
5. Performance validation

### Day 7: Documentation
1. Write observer pattern guide
2. Document priority system
3. Create migration examples
4. Update architecture docs
5. Final review

---

## Success Criteria

### Performance Requirements
- ✅ IdleStrategy::UpdateBehavior() <0.1ms per call
- ✅ Manager state queries <0.001ms each
- ✅ No throttling needed (called every frame)
- ✅ 100 bots: No performance degradation
- ✅ 500 bots: <1% CPU increase for idle strategy
- ✅ 1000 bots: <2% CPU increase
- ✅ 5000 bots: <10% CPU increase

### Functional Requirements
- ✅ All Automation::instance() calls removed
- ✅ All throttling timers removed
- ✅ Priority system works correctly
- ✅ Quest priority is highest
- ✅ Wander is fallback
- ✅ State queries are atomic
- ✅ No blocking operations

### Architecture Quality
- ✅ True observer pattern implemented
- ✅ Clean separation: strategies observe, managers work
- ✅ No cross-calling between systems
- ✅ Follows Phase 2.1 architecture design
- ✅ All managers from Phase 2.4 integrated

### Code Quality
- ✅ Clean, readable code
- ✅ Full documentation
- ✅ No compiler warnings
- ✅ Comprehensive tests
- ✅ No memory leaks

---

## Dependencies

### Requires
- Phase 2.1 complete (BehaviorManager base class)
- Phase 2.4 complete (All 4 managers refactored)
- Manager state query methods implemented
- BotAI::UpdateManagers() working

### Blocks
- Phase 2.6 (Integration testing needs observer pattern working)
- Full idle bot functionality
- Performance scalability to 5000+ bots

---

## Risk Mitigation

### Risk: Breaking idle bot behavior
**Mitigation**:
- Implement new code alongside old code temporarily
- Test extensively before removing old code
- Create rollback plan

### Risk: Performance regression
**Mitigation**:
- Benchmark before/after
- Profile every step
- Test with 5000 bots

### Risk: Priority system too complex
**Mitigation**:
- Keep priority logic simple (if/return pattern)
- Document priority order clearly
- Test each priority level independently

### Risk: State queries not thread-safe
**Mitigation**:
- Use atomic state flags in managers (from Phase 2.4)
- No locks needed (single-threaded access)
- Test with high bot counts

---

## Before/After Comparison

### Performance
| Metric | Before | After | Improvement |
|--------|--------|-------|-------------|
| Avg update time | 100-144ms | <0.1ms | 1000x faster |
| Min update time | 0ms (throttled) | <0.1ms | Consistent |
| Max update time | 600ms | <0.1ms | 6000x faster |
| CPU usage (100 bots) | 15-20% | <1% | 95% reduction |

### Code Complexity
| Metric | Before | After | Improvement |
|--------|--------|-------|-------------|
| LOC in UpdateBehavior() | 83 lines | 60 lines | Simpler |
| Timer variables | 8 timers | 1 timer | Cleaner |
| External dependencies | 4 singletons | 4 managers | Better |
| Blocking calls | 4 calls | 0 calls | No blocking |

---

## Next Phase

After completion, proceed to **Phase 2.6: Integration Testing**

---

**Last Updated**: 2025-01-13
**Next Review**: 2025-02-24
