# Phase 2: Architecture Cleanup - Detailed Implementation Plan

**Version**: 1.0  
**Date**: 2026-01-26  
**Prerequisites**: Phase 1 complete  
**Duration**: ~30 hours (1 week)

---

## Overview

Phase 2 eliminates feature overlaps and establishes single sources of truth for interrupt coordination, CC management, and target selection. This creates a clean architecture foundation for Phase 3's event-driven migration.

### Problems Being Fixed

| Problem | Current State | Target State |
|---------|---------------|--------------|
| Interrupt Overlap | 3 components track interrupts | InterruptCoordinator only |
| CC Overlap | 2 components track CC | CrowdControlManager only |
| Target Overlap | 3 components select targets | TacticalCoordinator only |
| No DR Tracking | CC ignores diminishing returns | Full DR support |

---

## Task 2.1: Consolidate Interrupt Coordination

**Priority**: P1  
**Effort**: 8 hours  
**Dependencies**: Phase 1 complete

### Current State (3-way overlap)

```
InterruptCoordinator (Combat)
├── AssignInterrupters()
├── OnEnemyCastStart()
├── RotateInterrupters()
└── TBB concurrent structures ← BEST implementation

TacticalCoordinator (Advanced)
├── AssignInterrupt()
├── RegisterInterrupter()
├── IsNextInterrupter()
└── UpdateInterruptRotation()

RoleCoordinator::DPSCoordinator (Coordination)
├── GetNextInterrupter()
├── AssignInterrupt()
└── UpdateInterruptRotation()
```

### Target State (Single Authority)

```
InterruptCoordinator (Combat) ← MASTER
├── All interrupt logic
├── TBB concurrent structures
└── Single source of truth

TacticalCoordinator (Advanced) ← DELEGATES
└── Forwards calls to InterruptCoordinator

DPSCoordinator (Coordination) ← DELEGATES
└── Forwards calls to InterruptCoordinator
```

### Files to Modify

```
src/modules/Playerbot/Advanced/TacticalCoordinator.h
src/modules/Playerbot/Advanced/TacticalCoordinator.cpp
src/modules/Playerbot/AI/Coordination/RoleCoordinator.h
src/modules/Playerbot/AI/Coordination/RoleCoordinator.cpp
```

### Implementation Steps

#### Step 2.1.1: Add InterruptCoordinator reference to TacticalCoordinator

**TacticalCoordinator.h**:
```cpp
// Add forward declaration at top
class InterruptCoordinator;

class TacticalCoordinator {
private:
    // ADD: Reference to interrupt authority
    InterruptCoordinator* _interruptCoordinator = nullptr;
    
    // REMOVE these if they exist:
    // std::queue<InterruptAssignment> _interruptQueue;
    // std::map<ObjectGuid, uint32> _interrupterCooldowns;
    
public:
    // ADD: Setter for DI
    void SetInterruptCoordinator(InterruptCoordinator* ic) { _interruptCoordinator = ic; }
};
```

#### Step 2.1.2: Modify TacticalCoordinator to delegate

**TacticalCoordinator.cpp**:
```cpp
// CHANGE: Delegate instead of implement
void TacticalCoordinator::AssignInterrupt(ObjectGuid caster, uint32 spellId) {
    if (_interruptCoordinator) {
        // Forward to the real authority
        _interruptCoordinator->OnEnemyCastStart(caster, spellId);
    }
}

bool TacticalCoordinator::IsNextInterrupter(ObjectGuid bot) const {
    if (_interruptCoordinator) {
        return _interruptCoordinator->IsAssignedInterrupter(bot);
    }
    return false;
}

ObjectGuid TacticalCoordinator::GetNextInterrupter() const {
    if (_interruptCoordinator) {
        return _interruptCoordinator->GetAssignedInterrupter();
    }
    return ObjectGuid::Empty;
}

// REMOVE: Internal interrupt tracking methods
// REMOVE: RegisterInterrupter() implementation (if duplicating IC)
// REMOVE: UpdateInterruptRotation() implementation (if duplicating IC)
```

#### Step 2.1.3: Modify DPSCoordinator to delegate

**RoleCoordinator.h** (DPSCoordinator section):
```cpp
class DPSCoordinator {
private:
    InterruptCoordinator* _interruptCoordinator = nullptr;
    
    // REMOVE if exists:
    // std::vector<ObjectGuid> _interruptRotation;
    // uint32 _currentInterrupterIndex;
    
public:
    void SetInterruptCoordinator(InterruptCoordinator* ic) { _interruptCoordinator = ic; }
};
```

**RoleCoordinator.cpp** (DPSCoordinator methods):
```cpp
ObjectGuid DPSCoordinator::GetNextInterrupter() {
    if (_interruptCoordinator) {
        return _interruptCoordinator->GetAssignedInterrupter();
    }
    return ObjectGuid::Empty;
}

void DPSCoordinator::AssignInterrupt(ObjectGuid caster, uint32 spellId) {
    if (_interruptCoordinator) {
        _interruptCoordinator->OnEnemyCastStart(caster, spellId);
    }
}

// REMOVE: UpdateInterruptRotation() if it duplicates InterruptCoordinator logic
```

### Verification
- [ ] Only InterruptCoordinator has interrupt state
- [ ] TacticalCoordinator delegates all interrupt calls
- [ ] DPSCoordinator delegates all interrupt calls
- [ ] Interrupts still work in combat testing
- [ ] Project compiles without errors

---

## Task 2.2: Consolidate CC Coordination

**Priority**: P1  
**Effort**: 6 hours  
**Dependencies**: Task 2.1 complete

### Current State (2-way overlap)

```
CrowdControlManager (Combat)
├── ApplyCC()
├── GetRecommendedSpell()
├── GetPriorityTarget()
├── IsTargetCCd()
└── Per-bot instance ← Cleanest

TacticalCoordinator (Advanced)
├── AssignCrowdControl()
├── GetCrowdControlTargets()
└── IsTargetCrowdControlled()
```

### Target State

```
CrowdControlManager (Combat) ← MASTER
├── All CC logic
├── CC state tracking
└── Single source of truth

TacticalCoordinator (Advanced) ← DELEGATES
└── Forwards calls to CrowdControlManager
```

### Files to Modify

```
src/modules/Playerbot/Advanced/TacticalCoordinator.h
src/modules/Playerbot/Advanced/TacticalCoordinator.cpp
```

### Implementation Steps

#### Step 2.2.1: Add CrowdControlManager reference

**TacticalCoordinator.h**:
```cpp
// Add forward declaration
class CrowdControlManager;

class TacticalCoordinator {
private:
    // ADD
    CrowdControlManager* _ccManager = nullptr;
    
    // REMOVE if exists:
    // std::vector<ObjectGuid> _ccTargets;
    // std::map<ObjectGuid, CCInfo> _ccAssignments;
    
public:
    void SetCCManager(CrowdControlManager* ccm) { _ccManager = ccm; }
};
```

#### Step 2.2.2: Modify TacticalCoordinator to delegate

**TacticalCoordinator.cpp**:
```cpp
void TacticalCoordinator::AssignCrowdControl(ObjectGuid target) {
    if (_ccManager) {
        uint32 recommendedSpell = _ccManager->GetRecommendedSpell(target);
        if (recommendedSpell) {
            _ccManager->ApplyCC(target, recommendedSpell);
        }
    }
}

std::vector<ObjectGuid> TacticalCoordinator::GetCrowdControlTargets() const {
    if (_ccManager) {
        return _ccManager->GetCCdTargets();
    }
    return {};
}

bool TacticalCoordinator::IsTargetCrowdControlled(ObjectGuid target) const {
    if (_ccManager) {
        return _ccManager->IsTargetCCd(target);
    }
    return false;
}

// REMOVE: Any internal CC tracking logic
```

### Verification
- [ ] Only CrowdControlManager has CC state
- [ ] TacticalCoordinator delegates all CC calls
- [ ] CC still works in dungeon/raid testing
- [ ] Project compiles without errors

---

## Task 2.3: Consolidate Target Selection

**Priority**: P1  
**Effort**: 6 hours  
**Dependencies**: Task 2.2 complete

### Current State (3-way overlap)

```
Strategy::SelectTarget() (Strategy)
└── Virtual, per-strategy override

GroupCombatStrategy::FindGroupCombatTarget() (Strategy)
└── Complex target finding logic (lines 301-369)

TacticalCoordinator::GetFocusTarget() (Advanced)
├── SetFocusTarget()
├── GetPriorityTargets()
└── AddPriorityTarget()
```

### Target State

```
TacticalCoordinator (Advanced) ← MASTER
├── GetGroupTarget()
├── SetGroupTarget()
├── Priority target system
└── Single source of truth

GroupCombatStrategy (Strategy) ← QUERIES
└── Gets target from TacticalCoordinator

Strategy base (Strategy) ← QUERIES or NONE
└── Default: query TacticalCoordinator
```

### Files to Modify

```
src/modules/Playerbot/Advanced/TacticalCoordinator.h
src/modules/Playerbot/Advanced/TacticalCoordinator.cpp
src/modules/Playerbot/AI/Strategy/GroupCombatStrategy.h
src/modules/Playerbot/AI/Strategy/GroupCombatStrategy.cpp
```

### Implementation Steps

#### Step 2.3.1: Ensure TacticalCoordinator has complete target API

**TacticalCoordinator.h**:
```cpp
class TacticalCoordinator {
private:
    ObjectGuid _groupTarget;
    std::vector<std::pair<ObjectGuid, uint32>> _priorityTargets;  // target, priority
    
public:
    // Primary target
    Unit* GetGroupTarget() const;
    void SetGroupTarget(Unit* target);
    void SetGroupTarget(ObjectGuid targetGuid);
    void ClearGroupTarget();
    
    // Priority system
    void AddPriorityTarget(ObjectGuid target, uint32 priority);
    void RemovePriorityTarget(ObjectGuid target);
    Unit* GetHighestPriorityTarget() const;
    std::vector<ObjectGuid> GetPriorityTargets() const;
    
    // Utility
    bool HasGroupTarget() const { return !_groupTarget.IsEmpty(); }
};
```

#### Step 2.3.2: Modify GroupCombatStrategy to query TacticalCoordinator

**GroupCombatStrategy.h**:
```cpp
class GroupCombatStrategy : public CombatStrategy {
private:
    // ADD: Reference to tactical coordinator
    TacticalCoordinator* GetTacticalCoordinator(BotAI* ai) const;
    
    // KEEP as fallback only:
    Unit* FindFallbackTarget(BotAI* ai) const;
};
```

**GroupCombatStrategy.cpp**:
```cpp
Unit* GroupCombatStrategy::GetCombatTarget(BotAI* ai) {
    // First: Try TacticalCoordinator (group coordination)
    if (TacticalCoordinator* tc = GetTacticalCoordinator(ai)) {
        if (Unit* target = tc->GetGroupTarget()) {
            return target;
        }
        
        // Try priority targets
        if (Unit* priorityTarget = tc->GetHighestPriorityTarget()) {
            return priorityTarget;
        }
    }
    
    // Fallback: Find target ourselves (solo or no coordinator)
    return FindFallbackTarget(ai);
}

Unit* GroupCombatStrategy::FindFallbackTarget(BotAI* ai) const {
    // Simplified fallback - only used when no TacticalCoordinator
    // Use cached members from Phase 1
    for (const auto& memberGuid : _memberCache) {
        Player* member = ObjectAccessor::FindPlayer(memberGuid);
        if (!member || !member->IsInCombat()) continue;
        
        if (Unit* target = member->GetVictim()) {
            return target;
        }
    }
    return nullptr;
}

TacticalCoordinator* GroupCombatStrategy::GetTacticalCoordinator(BotAI* ai) const {
    // Get from GroupCoordinator or wherever it's stored
    if (auto* gc = ai->GetGroupCoordinator()) {
        return gc->GetTacticalCoordinator();
    }
    return nullptr;
}

// REMOVE or simplify: FindGroupCombatTarget() 
// The complex logic from lines 301-369 should be moved to TacticalCoordinator
// or significantly simplified as FindFallbackTarget
```

### Verification
- [ ] TacticalCoordinator is the target authority
- [ ] GroupCombatStrategy queries TacticalCoordinator
- [ ] All group members attack same target
- [ ] Fallback works for solo bots
- [ ] Project compiles without errors

---

## Task 2.4: Add DR Tracking to CrowdControlManager

**Priority**: P2  
**Effort**: 8 hours  
**Dependencies**: Task 2.2 complete

### Purpose
Diminishing Returns (DR) tracking is essential for Arena PvP. Without it, bots will waste CC on immune targets.

### Files to Modify

```
src/modules/Playerbot/AI/Combat/CrowdControlManager.h
src/modules/Playerbot/AI/Combat/CrowdControlManager.cpp
```

### Implementation

#### Step 2.4.1: Add DR types and structures

**CrowdControlManager.h**:
```cpp
#pragma once

#include <map>
#include <cstdint>
#include "ObjectGuid.h"

namespace Playerbot {

// Diminishing Returns Categories (WoW standard)
enum class DRCategory : uint8 {
    NONE = 0,
    STUN = 1,           // Charge Stun, Hammer of Justice, etc.
    INCAPACITATE = 2,   // Polymorph, Hex, Gouge, etc.
    DISORIENT = 3,      // Fear (non-warlock), Psychic Scream
    SILENCE = 4,        // Silence effects
    FEAR = 5,           // Warlock Fear specifically
    ROOT = 6,           // Frost Nova, Entangling Roots
    HORROR = 7,         // Death Coil, Intimidating Shout
    DISARM = 8          // Disarm effects
};

struct DRState {
    uint8 stacks = 0;
    uint32 lastApplicationTime = 0;
    static constexpr uint32 DR_RESET_TIME_MS = 18000;  // 18 seconds
    
    float GetDurationMultiplier() const {
        switch (stacks) {
            case 0: return 1.0f;    // Full duration
            case 1: return 0.5f;    // 50% duration
            case 2: return 0.25f;   // 25% duration
            default: return 0.0f;   // Immune
        }
    }
    
    bool IsImmune() const { return stacks >= 3; }
    
    void Apply(uint32 currentTime) {
        stacks = std::min<uint8>(stacks + 1, 3);
        lastApplicationTime = currentTime;
    }
    
    void Update(uint32 currentTime) {
        if (currentTime - lastApplicationTime > DR_RESET_TIME_MS) {
            stacks = 0;
        }
    }
    
    void Reset() {
        stacks = 0;
        lastApplicationTime = 0;
    }
};

class CrowdControlManager {
private:
    // Existing members...
    
    // NEW: DR tracking per target per category
    std::map<ObjectGuid, std::map<DRCategory, DRState>> _drTracking;
    
    // NEW: Spell to DR category mapping
    static DRCategory GetDRCategory(uint32 spellId);
    static DRCategory GetDRCategoryFromAuraType(uint32 auraType);
    
public:
    // NEW: DR Query methods
    float GetDRMultiplier(ObjectGuid target, DRCategory category) const;
    float GetDRMultiplier(ObjectGuid target, uint32 spellId) const;
    bool IsImmune(ObjectGuid target, DRCategory category) const;
    bool IsImmune(ObjectGuid target, uint32 spellId) const;
    uint8 GetDRStacks(ObjectGuid target, DRCategory category) const;
    
    // NEW: DR Update methods
    void OnCCApplied(ObjectGuid target, uint32 spellId);
    void OnCCApplied(ObjectGuid target, DRCategory category);
    void UpdateDR(uint32 currentTime);  // Call periodically to reset expired DR
    void ClearDR(ObjectGuid target);    // Clear when target dies
    
    // MODIFY: Existing methods to consider DR
    uint32 GetRecommendedSpell(ObjectGuid target) const;  // Now considers DR
    float GetExpectedDuration(ObjectGuid target, uint32 spellId) const;
};

} // namespace Playerbot
```

#### Step 2.4.2: Implement DR logic

**CrowdControlManager.cpp**:
```cpp
#include "CrowdControlManager.h"
#include "GameTime.h"

namespace Playerbot {

// Spell ID to DR Category mapping (partial list - expand as needed)
DRCategory CrowdControlManager::GetDRCategory(uint32 spellId) {
    // This should be a comprehensive mapping
    // Using switch for common spells, could also use a static map
    switch (spellId) {
        // Stuns
        case 853:    // Hammer of Justice
        case 7922:   // Charge Stun
        case 408:    // Kidney Shot
        case 1833:   // Cheap Shot
            return DRCategory::STUN;
            
        // Incapacitates
        case 118:    // Polymorph
        case 51514:  // Hex
        case 1776:   // Gouge
        case 6770:   // Sap
            return DRCategory::INCAPACITATE;
            
        // Fears
        case 5782:   // Fear (Warlock)
        case 5484:   // Howl of Terror
            return DRCategory::FEAR;
            
        case 8122:   // Psychic Scream
        case 2094:   // Blind
            return DRCategory::DISORIENT;
            
        // Roots
        case 122:    // Frost Nova
        case 339:    // Entangling Roots
            return DRCategory::ROOT;
            
        // Silences
        case 15487:  // Silence
        case 1330:   // Garrote - Silence
            return DRCategory::SILENCE;
            
        // Horrors
        case 6789:   // Death Coil
        case 5246:   // Intimidating Shout
            return DRCategory::HORROR;
            
        default:
            return DRCategory::NONE;
    }
}

float CrowdControlManager::GetDRMultiplier(ObjectGuid target, DRCategory category) const {
    if (category == DRCategory::NONE) return 1.0f;
    
    auto targetIt = _drTracking.find(target);
    if (targetIt == _drTracking.end()) return 1.0f;
    
    auto categoryIt = targetIt->second.find(category);
    if (categoryIt == targetIt->second.end()) return 1.0f;
    
    return categoryIt->second.GetDurationMultiplier();
}

float CrowdControlManager::GetDRMultiplier(ObjectGuid target, uint32 spellId) const {
    return GetDRMultiplier(target, GetDRCategory(spellId));
}

bool CrowdControlManager::IsImmune(ObjectGuid target, DRCategory category) const {
    if (category == DRCategory::NONE) return false;
    
    auto targetIt = _drTracking.find(target);
    if (targetIt == _drTracking.end()) return false;
    
    auto categoryIt = targetIt->second.find(category);
    if (categoryIt == targetIt->second.end()) return false;
    
    return categoryIt->second.IsImmune();
}

bool CrowdControlManager::IsImmune(ObjectGuid target, uint32 spellId) const {
    return IsImmune(target, GetDRCategory(spellId));
}

uint8 CrowdControlManager::GetDRStacks(ObjectGuid target, DRCategory category) const {
    auto targetIt = _drTracking.find(target);
    if (targetIt == _drTracking.end()) return 0;
    
    auto categoryIt = targetIt->second.find(category);
    if (categoryIt == targetIt->second.end()) return 0;
    
    return categoryIt->second.stacks;
}

void CrowdControlManager::OnCCApplied(ObjectGuid target, uint32 spellId) {
    OnCCApplied(target, GetDRCategory(spellId));
}

void CrowdControlManager::OnCCApplied(ObjectGuid target, DRCategory category) {
    if (category == DRCategory::NONE) return;
    
    uint32 now = GameTime::GetGameTimeMS();
    _drTracking[target][category].Apply(now);
}

void CrowdControlManager::UpdateDR(uint32 currentTime) {
    for (auto& [target, categories] : _drTracking) {
        for (auto& [category, state] : categories) {
            state.Update(currentTime);
        }
    }
    
    // Clean up targets with all-zero DR
    for (auto it = _drTracking.begin(); it != _drTracking.end(); ) {
        bool allZero = true;
        for (const auto& [cat, state] : it->second) {
            if (state.stacks > 0) {
                allZero = false;
                break;
            }
        }
        if (allZero) {
            it = _drTracking.erase(it);
        } else {
            ++it;
        }
    }
}

void CrowdControlManager::ClearDR(ObjectGuid target) {
    _drTracking.erase(target);
}

uint32 CrowdControlManager::GetRecommendedSpell(ObjectGuid target) const {
    // MODIFIED: Consider DR when recommending CC
    for (const auto& [spellId, ccInfo] : _availableCC) {
        DRCategory category = GetDRCategory(spellId);
        
        // Skip if target is immune to this category
        if (IsImmune(target, category)) {
            continue;
        }
        
        // Prefer CC with no/low DR
        float drMult = GetDRMultiplier(target, category);
        if (drMult >= 0.25f) {  // At least 25% duration
            return spellId;
        }
    }
    
    // No good CC available
    return 0;
}

float CrowdControlManager::GetExpectedDuration(ObjectGuid target, uint32 spellId) const {
    float baseDuration = GetBaseCCDuration(spellId);  // Existing method
    float drMult = GetDRMultiplier(target, spellId);
    return baseDuration * drMult;
}

} // namespace Playerbot
```

### Verification
- [ ] DR stacks increment correctly (0→1→2→3)
- [ ] DR resets after 18 seconds
- [ ] IsImmune returns true at 3 stacks
- [ ] GetRecommendedSpell avoids immune categories
- [ ] GetExpectedDuration returns correct reduced duration
- [ ] Project compiles without errors

---

## Task 2.5: Verify Architecture

**Priority**: P2  
**Effort**: 2 hours  
**Dependencies**: Tasks 2.1-2.4 complete

### Checklist

1. **Interrupt Coordination**
   - [ ] InterruptCoordinator is only component with interrupt state
   - [ ] TacticalCoordinator.h has no interrupt containers
   - [ ] DPSCoordinator has no interrupt containers
   - [ ] All interrupt calls route to InterruptCoordinator

2. **CC Coordination**
   - [ ] CrowdControlManager is only component with CC state
   - [ ] TacticalCoordinator.h has no CC containers
   - [ ] All CC calls route to CrowdControlManager
   - [ ] DR tracking is in CrowdControlManager only

3. **Target Selection**
   - [ ] TacticalCoordinator owns group target
   - [ ] GroupCombatStrategy queries TacticalCoordinator
   - [ ] No duplicate target selection logic

4. **Dependencies**
   - [ ] No new circular dependencies introduced
   - [ ] Layer violations from Phase 1 still fixed
   - [ ] Dependency injection used where possible

5. **Compilation & Testing**
   - [ ] Project compiles without errors
   - [ ] No new warnings introduced
   - [ ] Basic combat still works

---

## Summary

| Task | Files Modified | Key Changes |
|------|----------------|-------------|
| 2.1 | TacticalCoordinator, RoleCoordinator | Delegate interrupts to InterruptCoordinator |
| 2.2 | TacticalCoordinator | Delegate CC to CrowdControlManager |
| 2.3 | TacticalCoordinator, GroupCombatStrategy | Centralize target selection |
| 2.4 | CrowdControlManager | Add DR tracking system |
| 2.5 | N/A | Verification only |

## Expected Outcomes

| Metric | Before | After |
|--------|--------|-------|
| Interrupt tracking locations | 3 | 1 |
| CC tracking locations | 2 | 1 |
| Target selection locations | 3 | 1 |
| DR support | None | Full |
| Code duplication | High | Minimal |
