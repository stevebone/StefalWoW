# Combat System Refactoring - Implementation Plan

**Version**: 1.0  
**Date**: 2026-01-26  
**Source**: Zenflow Task 1.3 Analysis  
**Target**: Claude Code Execution

---

## Overview

This plan transforms the Combat Coordinator architecture from the current state (100% polling, 2 performance failures, 4 circular dependencies) to an enterprise-grade event-driven system supporting 5,000 concurrent bots.

### Current Problems
- **GroupCombatStrategy**: O(N²) = 175,000 ops @ 5K bots (FAIL)
- **RoleCoordinator**: O(H×T) = 62,500 ops @ 5K bots (FAIL)
- **Strategy Base**: 100,000 updates/sec without throttling
- **ThreatCoordinator**: 2 circular dependencies
- **RaidOrchestrator/RoleCoordinator**: Layer violations
- **Missing**: DungeonCoordinator, ArenaCoordinator, BGCoordinator

### Target State
- All components PASS performance budget (9.6μs/bot/tick)
- Zero circular dependencies
- Zero layer violations
- Context-aware component activation
- Event-driven architecture foundation

---

## Execution Phases

### Phase 1: Foundation & Quick Wins (Days 1-3)

#### Task 1.1: Create CombatContextDetector Utility
**Priority**: P0 (Foundation for other tasks)  
**Effort**: 2 hours  
**Dependencies**: None

**Files to Create**:
```
src/modules/Playerbot/Core/Combat/CombatContextDetector.h
```

**Specification**:
```cpp
namespace Playerbot {

enum class CombatContext : uint8 {
    SOLO = 0,
    GROUP = 1,
    DUNGEON = 2,
    RAID = 3,
    ARENA = 4,
    BATTLEGROUND = 5
};

class CombatContextDetector {
public:
    static CombatContext Detect(Player* player);
    static const char* ToString(CombatContext ctx);
    static bool RequiresCoordination(CombatContext ctx);
    static bool IsPvP(CombatContext ctx);
    static bool IsPvE(CombatContext ctx);
    static uint32 GetRecommendedUpdateInterval(CombatContext ctx);
};

} // namespace Playerbot
```

**Implementation Logic**:
1. Check `player->InArena()` → ARENA
2. Check `player->InBattleground()` → BATTLEGROUND
3. Check `player->GetGroup()` is null → SOLO
4. Check `group->isRaidGroup()` → RAID
5. Check `player->GetMap()->IsDungeon()` → DUNGEON
6. Default → GROUP

**Verification**:
- Unit test all 6 context detections
- Verify no false positives between contexts

---

#### Task 1.2: Fix ThreatCoordinator Circular Dependencies
**Priority**: P0  
**Effort**: 2 hours  
**Dependencies**: None

**Files to Modify**:
```
src/modules/Playerbot/AI/Combat/ThreatCoordinator.h
src/modules/Playerbot/AI/Combat/ThreatCoordinator.cpp
```

**Changes**:

**ThreatCoordinator.h** - Remove includes, add forward declarations:
```cpp
// REMOVE these includes:
// #include "BotThreatManager.h"
// #include "InterruptCoordinator.h"

// ADD forward declarations:
class BotThreatManager;
class InterruptCoordinator;
```

**ThreatCoordinator.cpp** - Add includes at top:
```cpp
#include "BotThreatManager.h"
#include "InterruptCoordinator.h"
```

**Verification**:
- Project compiles without errors
- No circular dependency warnings
- ThreatCoordinator functions correctly

---

#### Task 1.3: Add Strategy Throttling Mechanism
**Priority**: P0  
**Effort**: 4 hours  
**Dependencies**: Task 1.1 (CombatContextDetector)

**Files to Modify**:
```
src/modules/Playerbot/AI/Strategy/Strategy.h
src/modules/Playerbot/AI/Strategy/Strategy.cpp
src/modules/Playerbot/AI/BotAI.cpp (call site)
```

**Changes**:

**Strategy.h** - Add throttle mechanism:
```cpp
class Strategy {
protected:
    // NEW: Throttle mechanism
    uint32 _behaviorUpdateInterval = 100;  // 100ms default
    uint32 _timeSinceLastBehaviorUpdate = 0;
    
public:
    // NEW: Throttled update (replaces direct UpdateBehavior calls)
    bool MaybeUpdateBehavior(BotAI* ai, uint32 diff);
    
    // NEW: Configuration
    void SetBehaviorUpdateInterval(uint32 ms) { _behaviorUpdateInterval = ms; }
    uint32 GetBehaviorUpdateInterval() const { return _behaviorUpdateInterval; }
    
    // NEW: Override for strategies needing every-frame updates (movement)
    virtual bool RequiresEveryFrameUpdate() const { return false; }
};
```

**Strategy.cpp** - Implement throttle:
```cpp
bool Strategy::MaybeUpdateBehavior(BotAI* ai, uint32 diff) {
    // Always allow if strategy requires every-frame updates
    if (RequiresEveryFrameUpdate()) {
        UpdateBehavior(ai, diff);
        return true;
    }
    
    _timeSinceLastBehaviorUpdate += diff;
    if (_timeSinceLastBehaviorUpdate < _behaviorUpdateInterval)
        return false;
        
    _timeSinceLastBehaviorUpdate = 0;
    UpdateBehavior(ai, diff);
    return true;
}
```

**BotAI.cpp** - Update call site:
```cpp
void BotAI::UpdateStrategies(uint32 diff) {
    for (auto& strategy : _activeStrategies) {
        if (strategy->IsActive()) {
            strategy->MaybeUpdateBehavior(this, diff);  // Changed from UpdateBehavior
        }
    }
}
```

**Verification**:
- Strategy updates reduced to ~10,000/sec (from 100,000)
- Combat behavior still responsive
- SoloCombatStrategy still has smooth movement

---

#### Task 1.4: Extract IGroupCoordinator Interface
**Priority**: P0  
**Effort**: 4 hours  
**Dependencies**: None

**Files to Create**:
```
src/modules/Playerbot/Core/DI/Interfaces/IGroupCoordinator.h
```

**Files to Modify**:
```
src/modules/Playerbot/Advanced/GroupCoordinator.h
src/modules/Playerbot/AI/Coordination/RaidOrchestrator.h
src/modules/Playerbot/AI/Coordination/RoleCoordinator.h
```

**IGroupCoordinator.h**:
```cpp
#pragma once

#include "ObjectGuid.h"

class Player;
class Group;

namespace Playerbot {

class IGroupCoordinator {
public:
    virtual ~IGroupCoordinator() = default;
    
    // Lifecycle
    virtual void Update(uint32 diff) = 0;
    
    // State queries
    virtual bool IsInGroup() const = 0;
    virtual bool IsInRaid() const = 0;
    virtual uint32 GetGroupSize() const = 0;
    virtual uint32 GetRaidSize() const = 0;
    virtual Player* GetLeader() const = 0;
    virtual Group* GetGroup() const = 0;
    
    // Role queries
    virtual bool IsTank() const = 0;
    virtual bool IsHealer() const = 0;
    virtual bool IsDPS() const = 0;
    
    // Combat queries
    virtual bool IsInCombat() const = 0;
    virtual Unit* GetGroupTarget() const = 0;
};

} // namespace Playerbot
```

**GroupCoordinator.h** - Implement interface:
```cpp
#include "Core/DI/Interfaces/IGroupCoordinator.h"

class GroupCoordinator : public IGroupCoordinator {
    // ... existing code ...
    
    // Implement interface methods
    bool IsInGroup() const override;
    bool IsInRaid() const override;
    // ... etc
};
```

**RaidOrchestrator.h** - Use interface:
```cpp
// REMOVE: #include "../../Advanced/GroupCoordinator.h"
#include "Core/DI/Interfaces/IGroupCoordinator.h"

class RaidOrchestrator {
    // CHANGE: Use interface instead of concrete class
    std::vector<std::unique_ptr<IGroupCoordinator>> _groupCoordinators;
};
```

**Verification**:
- Compiles without layer violation
- RaidOrchestrator works with interface
- RoleCoordinator works with interface

---

#### Task 1.5: Cache GroupCombatStrategy Members
**Priority**: P0 (Critical Performance Fix)  
**Effort**: 4 hours  
**Dependencies**: None

**Files to Modify**:
```
src/modules/Playerbot/AI/Strategy/GroupCombatStrategy.h
src/modules/Playerbot/AI/Strategy/GroupCombatStrategy.cpp
```

**GroupCombatStrategy.h** - Add cache:
```cpp
class GroupCombatStrategy : public CombatStrategy {
private:
    // NEW: Member cache to avoid O(N²) iteration
    std::vector<ObjectGuid> _memberCache;
    uint32 _lastCacheUpdate = 0;
    bool _memberCacheDirty = true;
    static constexpr uint32 CACHE_REFRESH_INTERVAL = 1000;  // 1 second
    
    void RefreshMemberCache(BotAI* ai);
    
public:
    // NEW: Called when group composition changes
    void OnGroupChanged() { _memberCacheDirty = true; }
    
    // Override
    void UpdateBehavior(BotAI* ai, uint32 diff) override;
};
```

**GroupCombatStrategy.cpp** - Implement cache:
```cpp
void GroupCombatStrategy::RefreshMemberCache(BotAI* ai) {
    _memberCache.clear();
    
    Group* group = ai->GetBot()->GetGroup();
    if (!group)
        return;
        
    for (GroupReference* ref = group->GetFirstMember(); ref; ref = ref->next()) {
        if (Player* member = ref->GetSource()) {
            _memberCache.push_back(member->GetGUID());
        }
    }
}

void GroupCombatStrategy::UpdateBehavior(BotAI* ai, uint32 diff) {
    uint32 now = GameTime::GetGameTimeMS();
    
    // Refresh cache if dirty or stale
    if (_memberCacheDirty || (now - _lastCacheUpdate) > CACHE_REFRESH_INTERVAL) {
        RefreshMemberCache(ai);
        _memberCacheDirty = false;
        _lastCacheUpdate = now;
    }
    
    // FIXED: O(N) single iteration instead of O(N²)
    for (const auto& memberGuid : _memberCache) {
        Player* member = ObjectAccessor::FindPlayer(memberGuid);
        if (!member)
            continue;
            
        // Process member with O(1) operations
        // ... existing logic refactored to single pass ...
    }
}
```

**Verification**:
- Operations reduced from 175,000 to ~5,000
- Target selection still works correctly
- No behavior regression

---

#### Task 1.6: Fix RoleCoordinator O(H×T) Complexity
**Priority**: P0 (Critical Performance Fix)  
**Effort**: 8 hours  
**Dependencies**: Task 1.4 (IGroupCoordinator)

**Files to Modify**:
```
src/modules/Playerbot/AI/Coordination/RoleCoordinator.h
src/modules/Playerbot/AI/Coordination/RoleCoordinator.cpp
```

**Problem Code** (RoleCoordinator.cpp:595-731):
```cpp
// CURRENT: O(H×T) nested loop
for (auto& healer : _healers) {
    for (auto& tank : _tanks) {
        // Assignment logic
    }
}
```

**Fixed Code**:
```cpp
void HealerCoordinator::UpdateHealingAssignments() {
    // Build priority queue of tanks - O(T log T)
    struct TankPriority {
        ObjectGuid guid;
        float priority;
        bool operator<(const TankPriority& other) const {
            return priority < other.priority;  // Max heap
        }
    };
    
    std::priority_queue<TankPriority> tankQueue;
    for (const auto& tank : _tanks) {
        float priority = CalculateTankPriority(tank);
        tankQueue.push({tank, priority});
    }
    
    // Assign healers to highest-priority tanks - O(H)
    for (const auto& healer : _healers) {
        if (tankQueue.empty())
            break;
            
        TankPriority topTank = tankQueue.top();
        tankQueue.pop();
        
        AssignHealerToTank(healer, topTank.guid);
        
        // Re-add tank with reduced priority (already has healer)
        topTank.priority *= 0.5f;
        tankQueue.push(topTank);
    }
}

float HealerCoordinator::CalculateTankPriority(ObjectGuid tankGuid) {
    Player* tank = ObjectAccessor::FindPlayer(tankGuid);
    if (!tank)
        return 0.0f;
        
    float priority = 100.0f;
    
    // Lower health = higher priority
    priority += (100.0f - tank->GetHealthPct());
    
    // Main tank gets bonus
    if (IsMainTank(tankGuid))
        priority += 50.0f;
        
    // Fewer assigned healers = higher priority
    uint32 assignedHealers = GetAssignedHealerCount(tankGuid);
    priority -= assignedHealers * 30.0f;
    
    return priority;
}
```

**Verification**:
- Operations reduced from 62,500 to ~10,000
- Healer assignments still correct
- Tank survival not affected

---

#### Task 1.7: Implement Strategy CalculateRelevance
**Priority**: P1  
**Effort**: 3 hours  
**Dependencies**: Task 1.1 (CombatContextDetector)

**Files to Modify**:
```
src/modules/Playerbot/AI/Strategy/SoloCombatStrategy.cpp
src/modules/Playerbot/AI/Strategy/GroupCombatStrategy.cpp
```

**SoloCombatStrategy.cpp**:
```cpp
float SoloCombatStrategy::CalculateRelevance(BotAI* ai) const {
    auto context = CombatContextDetector::Detect(ai->GetBot());
    
    // Only relevant when solo
    if (context != CombatContext::SOLO)
        return 0.0f;
        
    // Only relevant in combat
    if (!ai->GetBot()->IsInCombat())
        return 0.0f;
        
    return 70.0f;  // Standard solo combat priority
}

bool SoloCombatStrategy::RequiresEveryFrameUpdate() const {
    return true;  // Need smooth movement
}
```

**GroupCombatStrategy.cpp**:
```cpp
float GroupCombatStrategy::CalculateRelevance(BotAI* ai) const {
    auto context = CombatContextDetector::Detect(ai->GetBot());
    
    // Not relevant when solo
    if (context == CombatContext::SOLO)
        return 0.0f;
        
    // Higher priority in dungeons/raids
    switch (context) {
        case CombatContext::DUNGEON:
        case CombatContext::RAID:
            return 90.0f;
        case CombatContext::GROUP:
            return 80.0f;
        default:
            return 0.0f;  // Arena/BG need different strategies
    }
}
```

**Verification**:
- SoloCombatStrategy deactivates when grouped
- GroupCombatStrategy deactivates when solo
- Correct strategy activates per context

---

#### Task 1.8: Optimize FormationManager for Solo
**Priority**: P1  
**Effort**: 2 hours  
**Dependencies**: Task 1.1 (CombatContextDetector)

**Files to Modify**:
```
src/modules/Playerbot/AI/Combat/FormationManager.cpp
```

**Changes**:
```cpp
void FormationManager::UpdateFormation(uint32 diff) {
    // NEW: Skip for solo bots (no one to form with)
    auto context = CombatContextDetector::Detect(_bot);
    if (context == CombatContext::SOLO) {
        return;  // Early exit - saves all formation calculations
    }
    
    // Existing formation logic...
    if (!_inFormation)
        return;
        
    // ... rest of existing code
}
```

**Verification**:
- Solo bots skip formation updates entirely
- Grouped bots still form correctly
- ~40% reduction in formation operations

---

#### Task 1.9: Reduce BotThreatManager Memory
**Priority**: P2  
**Effort**: 1 hour  
**Dependencies**: Task 1.1 (CombatContextDetector)

**Files to Modify**:
```
src/modules/Playerbot/AI/Combat/BotThreatManager.h
src/modules/Playerbot/AI/Combat/BotThreatManager.cpp
```

**Changes**:

**BotThreatManager.h**:
```cpp
class BotThreatManager {
private:
    // NEW: Context-aware max entries
    uint32 GetMaxThreatEntries() const;
    
    // CHANGE: Dynamic sizing based on context
    // OLD: static constexpr uint32 MAX_THREAT_ENTRIES = 50;
};
```

**BotThreatManager.cpp**:
```cpp
uint32 BotThreatManager::GetMaxThreatEntries() const {
    auto context = CombatContextDetector::Detect(_bot);
    switch (context) {
        case CombatContext::SOLO:
            return 10;   // Solo rarely fights 10+ mobs
        case CombatContext::GROUP:
        case CombatContext::DUNGEON:
            return 20;   // Dungeons have limited pulls
        case CombatContext::RAID:
            return 50;   // Raids can have many adds
        case CombatContext::ARENA:
            return 5;    // Arena is small-scale
        case CombatContext::BATTLEGROUND:
            return 30;   // BG can have many enemies
        default:
            return 20;
    }
}
```

**Verification**:
- Memory reduced from ~47MB to ~25MB at 5K bots
- No threat tracking issues
- Context-appropriate entries

---

### Phase 2: Architecture Consolidation (Days 4-5)

#### Task 2.1: Consolidate Interrupt Coordination
**Priority**: P1  
**Effort**: 4 hours  
**Dependencies**: Phase 1 complete

**Goal**: Make InterruptCoordinator the single authority for interrupts

**Files to Modify**:
```
src/modules/Playerbot/Advanced/TacticalCoordinator.h
src/modules/Playerbot/Advanced/TacticalCoordinator.cpp
src/modules/Playerbot/AI/Coordination/RoleCoordinator.cpp
```

**Changes**:
- TacticalCoordinator delegates interrupt methods to InterruptCoordinator
- RoleCoordinator::DPSCoordinator delegates to InterruptCoordinator
- Remove duplicate interrupt tracking

---

#### Task 2.2: Consolidate CC Coordination
**Priority**: P1  
**Effort**: 4 hours  
**Dependencies**: Phase 1 complete

**Goal**: Make CrowdControlManager the single authority for CC

**Files to Modify**:
```
src/modules/Playerbot/Advanced/TacticalCoordinator.h
src/modules/Playerbot/Advanced/TacticalCoordinator.cpp
```

**Changes**:
- TacticalCoordinator delegates CC methods to CrowdControlManager
- Remove duplicate CC tracking from TacticalCoordinator

---

#### Task 2.3: Consolidate Target Selection
**Priority**: P1  
**Effort**: 4 hours  
**Dependencies**: Task 1.5 complete

**Goal**: TacticalCoordinator owns focus target, strategies query it

**Files to Modify**:
```
src/modules/Playerbot/AI/Strategy/GroupCombatStrategy.cpp
src/modules/Playerbot/Advanced/TacticalCoordinator.h
```

**Changes**:
- GroupCombatStrategy queries TacticalCoordinator for group target
- Remove duplicate target selection logic from strategy

---

### Phase 3: Testing & Validation (Day 6)

#### Task 3.1: Unit Tests
**Priority**: P0  
**Effort**: 4 hours

**Test Cases**:
1. CombatContextDetector - all 6 contexts
2. Strategy throttling - verify update reduction
3. GroupCombatStrategy cache - verify member tracking
4. RoleCoordinator priority queue - verify assignments
5. IGroupCoordinator - interface compliance

#### Task 3.2: Integration Tests
**Priority**: P0  
**Effort**: 4 hours

**Test Scenarios**:
1. Solo bot combat (verify SoloCombatStrategy active)
2. Group combat (verify GroupCombatStrategy active)
3. Dungeon run (verify coordination)
4. Raid encounter (verify performance)

#### Task 3.3: Performance Validation
**Priority**: P0  
**Effort**: 4 hours

**Metrics to Verify**:
| Metric | Before | Target | Validation Method |
|--------|--------|--------|-------------------|
| GroupCombatStrategy ops | 175,000 | <10,000 | Profiler |
| RoleCoordinator ops | 62,500 | <15,000 | Profiler |
| Strategy updates/sec | 100,000 | <15,000 | Counter |
| Memory @ 5K bots | ~59 MB | <45 MB | Task Manager |

---

## Execution Summary

### Timeline
| Day | Phase | Tasks | Hours |
|-----|-------|-------|-------|
| 1 | Foundation | 1.1, 1.2, 1.3 | 8h |
| 2 | Quick Wins | 1.4, 1.5 | 8h |
| 3 | Quick Wins | 1.6, 1.7, 1.8, 1.9 | 14h |
| 4 | Architecture | 2.1, 2.2 | 8h |
| 5 | Architecture | 2.3 | 4h |
| 6 | Testing | 3.1, 3.2, 3.3 | 12h |

**Total**: 54 hours (~6 working days)

### Success Criteria
- [ ] All components PASS performance budget
- [ ] Zero circular dependencies
- [ ] Zero layer violations
- [ ] Context-aware component activation
- [ ] Memory < 45MB at 5K bots
- [ ] All tests pass

### Risk Mitigation
1. **Compilation failures**: Small, incremental changes with verification
2. **Behavior regressions**: Comprehensive testing after each task
3. **Performance issues**: Profiling validation before/after
4. **Integration problems**: Interface-based design for loose coupling
