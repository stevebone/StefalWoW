# Claude Code Prompt: Combat System Refactoring

## Context

You are implementing a comprehensive refactoring of the TrinityCore Playerbot Combat Coordinator system. The goal is to transform the current architecture (100% polling, performance failures, missing coordinators) into an enterprise-grade event-driven system supporting 5,000 concurrent bots.

## Project Information

- **Repository**: `C:\TrinityBots\TrinityCore`
- **Module Path**: `src/modules/Playerbot/`
- **Analysis Source**: `.zenflow/worktrees/analyze-all-existing-combat-coor-358d/.zenflow/tasks/analyze-all-existing-combat-coor-358d/`
- **Plan Document**: `.claude/plans/COMBAT_REFACTORING_MASTER_PLAN.md`

## Critical Problems to Fix

1. **GroupCombatStrategy**: O(N²) = 175,000 ops @ 5K bots → Target: <10,000 ops
2. **RoleCoordinator**: O(H×T) = 62,500 ops @ 5K bots → Target: <15,000 ops
3. **Strategy Base**: 100,000 updates/sec → Target: <15,000/sec
4. **ThreatCoordinator**: 2 circular dependencies → Target: 0
5. **RaidOrchestrator/RoleCoordinator**: Layer violations → Target: 0

## Implementation Instructions

### Phase 1: Foundation (Execute First)

#### Task 1.1: Create CombatContextDetector

Create the file `src/modules/Playerbot/Core/Combat/CombatContextDetector.h`:

```cpp
#pragma once

#include "Player.h"
#include "Group.h"
#include "Map.h"
#include "Battleground.h"

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
    static CombatContext Detect(Player* player) {
        if (!player)
            return CombatContext::SOLO;
            
        if (player->InArena())
            return CombatContext::ARENA;
            
        if (player->InBattleground())
            return CombatContext::BATTLEGROUND;
        
        Group* group = player->GetGroup();
        if (!group)
            return CombatContext::SOLO;
            
        if (group->isRaidGroup())
            return CombatContext::RAID;
            
        Map* map = player->GetMap();
        if (map && map->IsDungeon())
            return CombatContext::DUNGEON;
            
        return CombatContext::GROUP;
    }
    
    static const char* ToString(CombatContext ctx) {
        switch (ctx) {
            case CombatContext::SOLO: return "Solo";
            case CombatContext::GROUP: return "Group";
            case CombatContext::DUNGEON: return "Dungeon";
            case CombatContext::RAID: return "Raid";
            case CombatContext::ARENA: return "Arena";
            case CombatContext::BATTLEGROUND: return "Battleground";
            default: return "Unknown";
        }
    }
    
    static bool RequiresCoordination(CombatContext ctx) {
        return ctx != CombatContext::SOLO;
    }
    
    static bool IsPvP(CombatContext ctx) {
        return ctx == CombatContext::ARENA || ctx == CombatContext::BATTLEGROUND;
    }
    
    static bool IsInstanced(CombatContext ctx) {
        return ctx == CombatContext::DUNGEON || ctx == CombatContext::RAID || 
               ctx == CombatContext::ARENA || ctx == CombatContext::BATTLEGROUND;
    }
    
    static uint32 GetRecommendedUpdateInterval(CombatContext ctx) {
        switch (ctx) {
            case CombatContext::ARENA: return 25;        // Fast PvP
            case CombatContext::BATTLEGROUND: return 50; // PvP, larger scale
            case CombatContext::DUNGEON: return 75;      // Mechanics timing
            case CombatContext::RAID: return 100;        // Balance CPU
            case CombatContext::GROUP: return 100;
            case CombatContext::SOLO: return 150;
            default: return 100;
        }
    }
    
    static uint32 GetMaxThreatEntries(CombatContext ctx) {
        switch (ctx) {
            case CombatContext::SOLO: return 10;
            case CombatContext::GROUP: return 20;
            case CombatContext::DUNGEON: return 25;
            case CombatContext::RAID: return 50;
            case CombatContext::ARENA: return 15;
            case CombatContext::BATTLEGROUND: return 30;
            default: return 20;
        }
    }
};

} // namespace Playerbot
```

#### Task 1.2: Add Strategy Throttling

Modify `src/modules/Playerbot/AI/Strategy/Strategy.h`:

Add to protected section:
```cpp
protected:
    uint32 _behaviorUpdateInterval = 100;
    uint32 _timeSinceLastBehaviorUpdate = 0;
```

Add to public section:
```cpp
public:
    void MaybeUpdateBehavior(BotAI* ai, uint32 diff);
    void SetBehaviorUpdateInterval(uint32 ms) { _behaviorUpdateInterval = ms; }
    uint32 GetBehaviorUpdateInterval() const { return _behaviorUpdateInterval; }
    virtual bool NeedsEveryFrameUpdate() const { return false; }
```

Modify `src/modules/Playerbot/AI/Strategy/Strategy.cpp`:

Add implementation:
```cpp
void Strategy::MaybeUpdateBehavior(BotAI* ai, uint32 diff) {
    if (NeedsEveryFrameUpdate()) {
        UpdateBehavior(ai, diff);
        return;
    }
    
    _timeSinceLastBehaviorUpdate += diff;
    if (_timeSinceLastBehaviorUpdate < _behaviorUpdateInterval)
        return;
        
    _timeSinceLastBehaviorUpdate = 0;
    UpdateBehavior(ai, diff);
}
```

Find the call site in BotAI.cpp where strategies are updated and change:
```cpp
// FROM:
strategy->UpdateBehavior(this, diff);
// TO:
strategy->MaybeUpdateBehavior(this, diff);
```

#### Task 1.3: Fix ThreatCoordinator Circular Dependencies

Modify `src/modules/Playerbot/AI/Combat/ThreatCoordinator.h`:

Find and remove these includes:
```cpp
#include "BotThreatManager.h"       // REMOVE THIS
#include "InterruptCoordinator.h"   // REMOVE THIS
```

Add forward declarations after other forward declarations:
```cpp
class BotThreatManager;
class InterruptCoordinator;
```

Modify `src/modules/Playerbot/AI/Combat/ThreatCoordinator.cpp`:

Add at the top after other includes:
```cpp
#include "BotThreatManager.h"
#include "InterruptCoordinator.h"
```

#### Task 1.4: Create IGroupCoordinator Interface

Create `src/modules/Playerbot/Core/DI/Interfaces/IGroupCoordinator.h`:

```cpp
#pragma once

#include "ObjectGuid.h"
#include <cstdint>
#include <memory>

class Player;
class Group;
class Unit;

namespace Playerbot {

enum class GroupRole : uint8 {
    UNDEFINED = 0,
    TANK = 1,
    HEALER = 2,
    DPS_MELEE = 3,
    DPS_RANGED = 4,
    SUPPORT = 5
};

class IGroupCoordinator {
public:
    virtual ~IGroupCoordinator() = default;
    
    // Lifecycle
    virtual void Update(uint32 diff) = 0;
    virtual void Initialize() = 0;
    virtual void Shutdown() = 0;
    
    // State Queries
    virtual bool IsInGroup() const = 0;
    virtual bool IsInRaid() const = 0;
    virtual bool IsLeader() const = 0;
    virtual uint32 GetGroupSize() const = 0;
    
    // References
    virtual Player* GetBot() const = 0;
    virtual Player* GetLeader() const = 0;
    virtual Group* GetGroup() const = 0;
    
    // Role System
    virtual GroupRole GetRole() const = 0;
    virtual void SetRole(GroupRole role) = 0;
    
    // Combat
    virtual ObjectGuid GetGroupTarget() const = 0;
    virtual bool IsGroupInCombat() const = 0;
};

} // namespace Playerbot
```

Modify `src/modules/Playerbot/Advanced/GroupCoordinator.h`:

Add include and inheritance:
```cpp
#include "Core/DI/Interfaces/IGroupCoordinator.h"

class GroupCoordinator : public IGroupCoordinator {
    // ... existing code ...
    // Add 'override' to implemented interface methods
};
```

Modify `src/modules/Playerbot/AI/Coordination/RaidOrchestrator.h`:

Change include:
```cpp
// REMOVE: #include "../../Advanced/GroupCoordinator.h"
#include "Core/DI/Interfaces/IGroupCoordinator.h"
```

Change member type:
```cpp
// FROM:
std::vector<std::unique_ptr<GroupCoordinator>> _groupCoordinators;
// TO:
std::vector<std::unique_ptr<IGroupCoordinator>> _groupCoordinators;
```

#### Task 1.5: Fix GroupCombatStrategy

Modify `src/modules/Playerbot/AI/Strategy/GroupCombatStrategy.h`:

Add private members:
```cpp
private:
    std::vector<ObjectGuid> _memberCache;
    uint32 _lastCacheUpdate = 0;
    bool _memberCacheDirty = true;
    static constexpr uint32 CACHE_DURATION_MS = 1000;
    
    void RefreshMemberCache(BotAI* ai);
```

Add public method:
```cpp
public:
    void OnGroupChanged() { _memberCacheDirty = true; }
```

Modify `src/modules/Playerbot/AI/Strategy/GroupCombatStrategy.cpp`:

Add cache refresh method:
```cpp
void GroupCombatStrategy::RefreshMemberCache(BotAI* ai) {
    _memberCache.clear();
    
    Group* group = ai->GetBot()->GetGroup();
    if (!group) return;
    
    _memberCache.reserve(group->GetMembersCount());
    
    for (GroupReference* ref = group->GetFirstMember(); ref; ref = ref->next()) {
        if (Player* member = ref->GetSource()) {
            _memberCache.push_back(member->GetGUID());
        }
    }
    
    _memberCacheDirty = false;
    _lastCacheUpdate = GameTime::GetGameTimeMS();
}
```

Modify UpdateBehavior to use cache:
```cpp
void GroupCombatStrategy::UpdateBehavior(BotAI* ai, uint32 diff) {
    uint32 now = GameTime::GetGameTimeMS();
    
    if (_memberCacheDirty || (now - _lastCacheUpdate > CACHE_DURATION_MS)) {
        RefreshMemberCache(ai);
    }
    
    // Use _memberCache instead of iterating group directly
    for (const auto& memberGuid : _memberCache) {
        Player* member = ObjectAccessor::FindPlayer(memberGuid);
        if (!member) continue;
        // ... process member
    }
}
```

#### Task 1.6: Fix RoleCoordinator Performance

Find the UpdateHealingAssignments method in `src/modules/Playerbot/AI/Coordination/RoleCoordinator.cpp`.

Replace the nested loop with priority queue approach:

```cpp
void HealerCoordinator::UpdateHealingAssignments() {
    struct TankPriority {
        ObjectGuid guid;
        float priority;
        uint8 assignedHealers = 0;
        
        bool operator<(const TankPriority& other) const {
            if (assignedHealers != other.assignedHealers)
                return assignedHealers > other.assignedHealers;
            return priority < other.priority;
        }
    };
    
    std::priority_queue<TankPriority> tankQueue;
    
    // Build queue - O(T log T)
    for (const auto& tank : _tanks) {
        float priority = CalculateTankPriority(tank);
        tankQueue.push({tank, priority, 0});
    }
    
    // Assign healers - O(H log T)
    _healerAssignments.clear();
    for (const auto& healer : _healers) {
        if (tankQueue.empty()) break;
        
        TankPriority top = tankQueue.top();
        tankQueue.pop();
        
        _healerAssignments[healer] = top.guid;
        
        top.assignedHealers++;
        tankQueue.push(top);
    }
}
```

### Verification Steps

After implementing each task:

1. **Compile Check**: `cmake --build build --target Playerbot`
2. **No Warnings**: Check for circular dependency warnings
3. **Unit Test**: Run any existing tests
4. **Quick Functional Test**: Start server, create bot, verify basic combat

### Important Notes

- Always create backups before modifying files
- Make incremental changes and test frequently
- Use `git status` and `git diff` to track changes
- Commit after each working task

### Files to Reference

For detailed specifications, read:
- `.zenflow/worktrees/analyze-all-existing-combat-coor-358d/.zenflow/tasks/analyze-all-existing-combat-coor-358d/ANALYSIS_REPORT.md`
- `.zenflow/worktrees/analyze-all-existing-combat-coor-358d/.zenflow/tasks/analyze-all-existing-combat-coor-358d/summaries/`
- `.zenflow/worktrees/analyze-all-existing-combat-coor-358d/.zenflow/tasks/analyze-all-existing-combat-coor-358d/feature_extraction.md`

### Expected Outcomes After Phase 1

| Metric | Before | After |
|--------|--------|-------|
| Circular Dependencies | 4 | 0 |
| Layer Violations | 2 | 0 |
| Strategy Updates/sec | 100,000 | ~15,000 |
| GroupCombatStrategy Ops | 175,000 | ~5,000 |
| RoleCoordinator Ops | 62,500 | ~10,000 |

## Execution Order

1. Task 1.1 (CombatContextDetector) - Foundation for others
2. Task 1.3 (ThreatCoordinator) - No dependencies
3. Task 1.4 (IGroupCoordinator) - No dependencies
4. Task 1.2 (Strategy Throttling) - Uses 1.1
5. Task 1.5 (GroupCombatStrategy) - Uses 1.1
6. Task 1.6 (RoleCoordinator) - Uses 1.4
7. Task 1.7-1.9 (Remaining quick wins)

Begin with Task 1.1 and proceed sequentially, verifying compilation after each task.
