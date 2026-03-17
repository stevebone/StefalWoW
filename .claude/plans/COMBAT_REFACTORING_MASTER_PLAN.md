# Combat System Refactoring - Master Implementation Plan

**Version**: 1.0  
**Created**: 2026-01-26  
**Source**: Zenflow Task 1.3 Analysis  
**Target**: 5,000 concurrent bots @ 20 TPS

---

## Overview

This plan transforms the Combat Coordinator system from its current state (100% polling, 2 performance failures, 3 missing coordinators) to an enterprise-grade event-driven architecture with full context support.

**Total Effort**: ~500 hours (12-14 weeks)  
**Phases**: 5  
**Priority Components**: 13 existing + 3 new

---

## Current Problems Summary

| Problem | Severity | Component |
|---------|----------|-----------|
| O(N²) nested loops | 🔴 CRITICAL | GroupCombatStrategy |
| O(H×T) healer assignments | 🔴 CRITICAL | RoleCoordinator |
| 100k strategy updates/sec | 🔴 CRITICAL | Strategy Base |
| 2 circular dependencies | 🟡 HIGH | ThreatCoordinator |
| 2 layer violations | 🟡 HIGH | RaidOrchestrator, RoleCoordinator |
| 3 feature overlaps | 🟡 HIGH | Interrupt, CC, Target |
| 100% polling | 🟡 HIGH | All components |
| Missing DungeonCoordinator | 🔴 CRITICAL | N/A |
| Missing ArenaCoordinator | 🔴 CRITICAL | N/A |
| Missing BGCoordinator | 🔴 CRITICAL | N/A |

---

## Phase 1: Foundation & Quick Wins (Week 1)

### Task 1.1: CombatContextDetector
**Priority**: P0 | **Effort**: 4h | **Dependencies**: None

**Create**: `src/modules/Playerbot/Core/Combat/CombatContextDetector.h`

```cpp
namespace Playerbot {

enum class CombatContext : uint8 {
    SOLO = 0, GROUP = 1, DUNGEON = 2, 
    RAID = 3, ARENA = 4, BATTLEGROUND = 5
};

class CombatContextDetector {
public:
    static CombatContext Detect(Player* player);
    static const char* ToString(CombatContext ctx);
    static bool RequiresCoordination(CombatContext ctx);
    static bool IsPvP(CombatContext ctx);
    static uint32 GetRecommendedUpdateInterval(CombatContext ctx);
    static uint32 GetMaxThreatEntries(CombatContext ctx);
};

}
```

---

### Task 1.2: Strategy Throttling
**Priority**: P0 | **Effort**: 6h | **Dependencies**: 1.1

**Modify**: `Strategy.h`, `Strategy.cpp`, `BotAI.cpp`

Add throttle mechanism:
```cpp
class Strategy {
protected:
    uint32 _behaviorUpdateInterval = 100;
    uint32 _timeSinceLastUpdate = 0;
public:
    void MaybeUpdateBehavior(BotAI* ai, uint32 diff);
    virtual bool NeedsEveryFrameUpdate() const { return false; }
};
```

---

### Task 1.3: Fix Circular Dependencies
**Priority**: P0 | **Effort**: 2h | **Dependencies**: None

**Modify**: `ThreatCoordinator.h`

Replace includes with forward declarations:
```cpp
// REMOVE: #include "BotThreatManager.h"
// REMOVE: #include "InterruptCoordinator.h"
class BotThreatManager;
class InterruptCoordinator;
```

---

### Task 1.4: IGroupCoordinator Interface
**Priority**: P0 | **Effort**: 6h | **Dependencies**: None

**Create**: `Core/DI/Interfaces/IGroupCoordinator.h`

```cpp
class IGroupCoordinator {
public:
    virtual ~IGroupCoordinator() = default;
    virtual void Update(uint32 diff) = 0;
    virtual bool IsInGroup() const = 0;
    virtual bool IsInRaid() const = 0;
    virtual GroupRole GetRole() const = 0;
    virtual ObjectGuid GetGroupTarget() const = 0;
};
```

---

### Task 1.5: GroupCombatStrategy Cache
**Priority**: P0 | **Effort**: 6h | **Dependencies**: 1.1

**Modify**: `GroupCombatStrategy.h`, `GroupCombatStrategy.cpp`

Add member cache:
```cpp
class GroupCombatStrategy {
private:
    std::vector<ObjectGuid> _memberCache;
    uint32 _lastCacheUpdate = 0;
    bool _memberCacheDirty = true;
    void RefreshMemberCache(BotAI* ai);
public:
    void OnGroupChanged() { _memberCacheDirty = true; }
};
```

---

### Task 1.6: RoleCoordinator Performance Fix
**Priority**: P0 | **Effort**: 8h | **Dependencies**: 1.4

**Modify**: `RoleCoordinator.cpp`

Replace O(H×T) with priority queue:
```cpp
void HealerCoordinator::UpdateHealingAssignments() {
    std::priority_queue<TankPriority> tankQueue;
    // Build queue O(T log T)
    // Assign healers O(H log T)
}
```

---

### Task 1.7: Solo Formation Skip
**Priority**: P1 | **Effort**: 2h | **Dependencies**: 1.1

**Modify**: `FormationManager.cpp`

```cpp
void FormationManager::UpdateFormation(uint32 diff) {
    if (CombatContextDetector::Detect(_bot) == CombatContext::SOLO)
        return;
    // ... existing code
}
```

---

### Task 1.8: BotThreatManager Memory
**Priority**: P1 | **Effort**: 3h | **Dependencies**: 1.1

**Modify**: `BotThreatManager.h`

Context-aware sizing:
| Context | MAX_ENTRIES |
|---------|-------------|
| SOLO | 10 |
| GROUP | 20 |
| DUNGEON | 25 |
| RAID | 50 |

---

### Task 1.9: Strategy CalculateRelevance
**Priority**: P1 | **Effort**: 4h | **Dependencies**: 1.1

**Modify**: `SoloCombatStrategy.cpp`, `GroupCombatStrategy.cpp`

Context-aware activation.

---

## Phase 2: Architecture Cleanup (Week 2)

### Task 2.1: Consolidate Interrupts
**Priority**: P1 | **Effort**: 8h

Make InterruptCoordinator single authority.

### Task 2.2: Consolidate CC
**Priority**: P1 | **Effort**: 6h

Make CrowdControlManager single authority.

### Task 2.3: Consolidate Target Selection
**Priority**: P1 | **Effort**: 6h

TacticalCoordinator owns focus target.

### Task 2.4: DR Tracking
**Priority**: P2 | **Effort**: 8h

Add Diminishing Returns to CrowdControlManager.

---

## Phase 3: Event-Driven Migration (Weeks 3-6)

### Task 3.1: Event Infrastructure
**Priority**: P0 | **Effort**: 16h

Create CombatEvent, CombatEventRouter, ICombatEventSubscriber.

### Task 3.2: TrinityCore Hooks
**Priority**: P0 | **Effort**: 16h

Add hooks for damage, spells, auras, threat.

### Task 3.3-3.6: Convert Components
- InterruptCoordinator → Event-driven (12h)
- ThreatCoordinator → Event-driven (16h)
- CrowdControlManager → Event-driven (8h)
- BotThreatManager → Event-driven (12h)

---

## Phase 4: New Coordinators (Weeks 7-12)

### Task 4.1: DungeonCoordinator
**Priority**: P0 | **Effort**: 80h

State machine, trash pulls, boss encounters, M+ support.

### Task 4.2: ArenaCoordinator
**Priority**: P1 | **Effort**: 80h

Kill targets, burst windows, CC chains, positioning.

### Task 4.3: BattlegroundCoordinator
**Priority**: P2 | **Effort**: 80h

Objectives, roles, flag carrier, node control.

---

## Phase 5: Testing (Weeks 13-14)

- Unit tests (20h)
- Integration tests (20h)
- Load testing 5K bots (20h)
- Performance validation (20h)

---

## Success Criteria

| Metric | Current | Target |
|--------|---------|--------|
| GroupCombatStrategy ops | 175,000 | <10,000 |
| RoleCoordinator ops | 62,500 | <15,000 |
| Strategy updates/sec | 100,000 | <15,000 |
| Memory @ 5K bots | 59 MB | <45 MB |
| Circular dependencies | 4 | 0 |
| Layer violations | 2 | 0 |
| Arena support | 8% | 80% |
| BG support | 8% | 80% |
