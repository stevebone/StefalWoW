# Movement System Migration Guide

**Date:** 2025-11-18
**Status:** ACTIVE MIGRATION
**Target:** Migrate from MovementArbiter to UnifiedMovementCoordinator
**Timeline:** 3 weeks (Weeks 1-3 of Phase 2)

---

## Executive Summary

The PlayerBot module is completing an **incomplete Phase 2 migration** to consolidate 4 separate movement systems into a single **UnifiedMovementCoordinator**. This guide helps developers migrate their code from the legacy `MovementArbiter` to the new unified system.

**Systems Being Consolidated:**
1. `MovementArbiter` → `UnifiedMovementCoordinator.ArbiterModule`
2. `CombatMovementStrategy` → `UnifiedMovementCoordinator.PositionModule`
3. `GroupFormationManager` → `UnifiedMovementCoordinator.FormationModule`
4. `MovementIntegration` → `UnifiedMovementCoordinator.ArbiterModule`

**Expected Benefits:**
- **69% code reduction** (~5,253 lines removed)
- **14 → 3 movement files** (simplified architecture)
- **Single entry point** for all movement operations
- **Better performance** (unified caching, reduced overhead)
- **Easier debugging** (one system to trace)

---

## Quick Start

### Before (MovementArbiter)
```cpp
// Old way - Direct MovementArbiter usage
auto* arbiter = bot->GetBotAI()->GetMovementArbiter();
auto req = MovementRequest::MakePointMovement(
    PlayerBotMovementPriority::COMBAT_AI,
    targetPos, true, {}, {}, {},
    "Combat positioning", "WarriorAI");
arbiter->RequestMovement(req);
```

### After (UnifiedMovementCoordinator)
```cpp
// New way - UnifiedMovementCoordinator
auto* coordinator = bot->GetBotAI()->GetUnifiedMovementCoordinator();
auto req = MovementRequest::MakePointMovement(
    PlayerBotMovementPriority::COMBAT_AI,
    targetPos, true, {}, {}, {},
    "Combat positioning", "WarriorAI");
coordinator->RequestMovement(req);  // Same API!
```

**Key Insight:** The API is **backwards compatible**. Most code only needs to change the getter method!

---

## Migration Patterns

### Pattern 1: Basic Movement Request

**Before:**
```cpp
MovementArbiter* arbiter = bot->GetBotAI()->GetMovementArbiter();
auto request = MovementRequest::MakePointMovement(
    PlayerBotMovementPriority::QUEST_OBJECTIVE,
    questObjectivePos, true, {}, {}, {},
    "Move to quest objective", "QuestAI"
);
bool accepted = arbiter->RequestMovement(request);
```

**After:**
```cpp
UnifiedMovementCoordinator* coordinator = bot->GetBotAI()->GetUnifiedMovementCoordinator();
auto request = MovementRequest::MakePointMovement(
    PlayerBotMovementPriority::QUEST_OBJECTIVE,
    questObjectivePos, true, {}, {}, {},
    "Move to quest objective", "QuestAI"
);
bool accepted = coordinator->RequestMovement(request);
```

**Changes:** Only the getter method name.

---

### Pattern 2: Chase Movement

**Before:**
```cpp
auto* arbiter = bot->GetBotAI()->GetMovementArbiter();
auto request = MovementRequest::MakeChaseMovement(
    PlayerBotMovementPriority::COMBAT_AI,
    target, 5.0f, 0.0f,
    "Chase enemy", "CombatAI"
);
arbiter->RequestMovement(request);
```

**After:**
```cpp
auto* coordinator = bot->GetBotAI()->GetUnifiedMovementCoordinator();
auto request = MovementRequest::MakeChaseMovement(
    PlayerBotMovementPriority::COMBAT_AI,
    target, 5.0f, 0.0f,
    "Chase enemy", "CombatAI"
);
coordinator->RequestMovement(request);
```

**Changes:** Only the getter method name.

---

### Pattern 3: Formation Movement (NEW API)

**Before (GroupFormationManager):**
```cpp
auto* formationMgr = GetGroupFormationManager();
formationMgr->SetFormation(FormationType::WEDGE);
Position formationPos = formationMgr->GetFormationPosition(botIndex);
// Then use MovementArbiter to move there...
```

**After (UnifiedMovementCoordinator):**
```cpp
auto* coordinator = bot->GetBotAI()->GetUnifiedMovementCoordinator();
coordinator->SetFormation(FormationType::WEDGE);
Position formationPos = coordinator->GetFormationPosition(botIndex);
// Movement is automatically coordinated!
```

**Benefits:**
- Single API for formation + movement
- Automatic coordination between formation and pathfinding
- No need to manually submit movement requests

---

### Pattern 4: Combat Positioning (NEW API)

**Before (CombatMovementStrategy):**
```cpp
auto* combatMovement = GetCombatMovementStrategy();
Position optimalPos = combatMovement->GetOptimalPositionForRole(ROLE_DPS_MELEE);
// Then use MovementArbiter to move there...
```

**After (UnifiedMovementCoordinator):**
```cpp
auto* coordinator = bot->GetBotAI()->GetUnifiedMovementCoordinator();
Position optimalPos = coordinator->GetOptimalCombatPosition(ROLE_DPS_MELEE);
coordinator->MoveToCombatPosition(optimalPos);  // Automatic priority and execution
```

**Benefits:**
- Automatic priority assignment (combat positioning is high priority)
- Integrated with threat management and positioning systems
- Single call instead of calculate + request

---

### Pattern 5: Stopping Movement

**Before:**
```cpp
MovementArbiter* arbiter = bot->GetBotAI()->GetMovementArbiter();
arbiter->ClearPendingRequests();
arbiter->StopMovement();
```

**After:**
```cpp
UnifiedMovementCoordinator* coordinator = bot->GetBotAI()->GetUnifiedMovementCoordinator();
coordinator->StopMovement();  // Automatically clears pending requests
```

**Benefits:** Simplified API (one call instead of two).

---

### Pattern 6: Pathfinding

**Before (PathfindingAdapter):**
```cpp
auto* pathfinder = GetPathfindingAdapter();
PathResult path = pathfinder->CalculatePath(start, end);
if (path.IsValid())
{
    // Use MovementArbiter to execute path...
}
```

**After (UnifiedMovementCoordinator):**
```cpp
auto* coordinator = bot->GetBotAI()->GetUnifiedMovementCoordinator();
PathResult path = coordinator->CalculatePath(start, end);
if (path.IsValid())
{
    coordinator->ExecutePath(path);  // Automatic execution with caching
}
```

**Benefits:**
- Automatic path caching (no duplicate calculations)
- Integrated with movement arbitration (automatic priority)
- Single system for calculate + execute

---

## API Comparison Table

| Feature | MovementArbiter (OLD) | UnifiedMovementCoordinator (NEW) |
|---------|----------------------|----------------------------------|
| **Movement Requests** | `GetMovementArbiter()->RequestMovement()` | `GetUnifiedMovementCoordinator()->RequestMovement()` |
| **Chase Movement** | `GetMovementArbiter()->RequestMovement(chase)` | `GetUnifiedMovementCoordinator()->RequestMovement(chase)` |
| **Stop Movement** | `GetMovementArbiter()->StopMovement()` | `GetUnifiedMovementCoordinator()->StopMovement()` |
| **Clear Requests** | `GetMovementArbiter()->ClearPendingRequests()` | `GetUnifiedMovementCoordinator()->StopMovement()` (automatic) |
| **Statistics** | `GetMovementArbiter()->GetStatistics()` | `GetUnifiedMovementCoordinator()->GetArbiterStatistics()` |
| **Diagnostics** | `GetMovementArbiter()->GetDiagnosticString()` | `GetUnifiedMovementCoordinator()->GetArbiterDiagnosticString()` |
| **Formation** | `GetGroupFormationManager()->SetFormation()` | `GetUnifiedMovementCoordinator()->SetFormation()` |
| **Formation Position** | `GetGroupFormationManager()->GetPosition()` | `GetUnifiedMovementCoordinator()->GetFormationPosition()` |
| **Combat Position** | `GetCombatMovementStrategy()->GetOptimalPos()` | `GetUnifiedMovementCoordinator()->GetOptimalCombatPosition()` |
| **Pathfinding** | `GetPathfindingAdapter()->CalculatePath()` | `GetUnifiedMovementCoordinator()->CalculatePath()` |
| **Execute Path** | Manual MovementRequest | `GetUnifiedMovementCoordinator()->ExecutePath()` |

---

## Step-by-Step Migration Checklist

For each file you're migrating, follow these steps:

### Step 1: Update Include Statements

**Remove:**
```cpp
#include "Movement/Arbiter/MovementArbiter.h"
#include "AI/Strategy/CombatMovementStrategy.h"
#include "Movement/GroupFormationManager.h"
#include "Movement/Pathfinding/PathfindingAdapter.h"
```

**Add:**
```cpp
#include "Movement/UnifiedMovementCoordinator.h"
```

---

### Step 2: Update Getter Calls

**Find and Replace:**
- `GetMovementArbiter()` → `GetUnifiedMovementCoordinator()`
- `GetGroupFormationManager()` → `GetUnifiedMovementCoordinator()`
- `GetCombatMovementStrategy()` → `GetUnifiedMovementCoordinator()`
- `GetPathfindingAdapter()` → `GetUnifiedMovementCoordinator()`

**Example:**
```bash
# Search for all MovementArbiter usage in your file
grep -n "GetMovementArbiter" MyFile.cpp

# Replace with UnifiedMovementCoordinator
sed -i 's/GetMovementArbiter()/GetUnifiedMovementCoordinator()/g' MyFile.cpp
```

---

### Step 3: Update Method Calls

Most method calls remain **identical**. However, some have improved APIs:

**Stopping Movement:**
```cpp
// OLD:
arbiter->ClearPendingRequests();
arbiter->StopMovement();

// NEW:
coordinator->StopMovement();  // Automatically clears pending requests
```

**Formation Movement:**
```cpp
// OLD:
formationMgr->SetFormation(FormationType::WEDGE);
Position pos = formationMgr->GetFormationPosition(index);
auto req = MovementRequest::MakePointMovement(...);
arbiter->RequestMovement(req);

// NEW:
coordinator->SetFormation(FormationType::WEDGE);
Position pos = coordinator->GetFormationPosition(index);
coordinator->RequestMovement(MovementRequest::MakePointMovement(...));
// OR use the new integrated API:
coordinator->MoveToFormationPosition(index);  // One call!
```

---

### Step 4: Test Your Changes

After migrating, verify:

1. **Compilation:** `cmake --build build/ --target game`
2. **Bot Movement:** Spawn a bot and verify it moves correctly
3. **Combat Movement:** Test combat positioning and kiting
4. **Formation Movement:** Test group formations
5. **Performance:** Check logs for any performance degradation

---

### Step 5: Remove Old Includes

Once migration is complete and tested, remove unused old includes:

```cpp
// REMOVE these after migration:
// #include "Movement/Arbiter/MovementArbiter.h"
// #include "AI/Strategy/CombatMovementStrategy.h"
// #include "Movement/GroupFormationManager.h"
```

---

## Common Migration Issues

### Issue 1: "Cannot find GetUnifiedMovementCoordinator()"

**Problem:** You're calling `GetUnifiedMovementCoordinator()` but it's not defined.

**Solution:** Make sure you're calling it on `BotAI`:
```cpp
// WRONG:
auto* coordinator = bot->GetUnifiedMovementCoordinator();

// CORRECT:
auto* coordinator = bot->GetBotAI()->GetUnifiedMovementCoordinator();
```

---

### Issue 2: "Method not found in UnifiedMovementCoordinator"

**Problem:** A method from the old system doesn't exist in the new one.

**Solution:** Check the API comparison table. Some methods have been renamed:
- `ClearPendingRequests()` → Included in `StopMovement()`
- `GetStatistics()` → `GetArbiterStatistics()`
- `GetDiagnosticString()` → `GetArbiterDiagnosticString()`

---

### Issue 3: "Null pointer when accessing coordinator"

**Problem:** `GetUnifiedMovementCoordinator()` returns nullptr.

**Solution:** This happens if BotAI hasn't initialized yet. Check initialization:
```cpp
auto* coordinator = bot->GetBotAI()->GetUnifiedMovementCoordinator();
if (!coordinator)
{
    TC_LOG_ERROR("module.playerbot", "UnifiedMovementCoordinator not initialized for bot {}", bot->GetName());
    return;
}
```

---

### Issue 4: "Movement doesn't work after migration"

**Problem:** Bot doesn't move after migrating to UnifiedMovementCoordinator.

**Solution:** Verify the priority level is correct:
```cpp
// Make sure priority is high enough (not IDLE or BACKGROUND)
auto request = MovementRequest::MakePointMovement(
    PlayerBotMovementPriority::COMBAT_AI,  // ✅ High priority
    // NOT PlayerBotMovementPriority::IDLE, ❌ Too low priority
    targetPos, true, {}, {}, {},
    "Description", "Source"
);
```

---

## Benefits of Migration

### 1. Single System
- **Before:** 4 separate systems (MovementArbiter, CombatMovementStrategy, GroupFormationManager, MovementIntegration)
- **After:** 1 unified system (UnifiedMovementCoordinator)
- **Benefit:** Easier to understand, debug, and maintain

### 2. Reduced Code
- **Before:** 7,597 lines across 14 files
- **After:** ~2,344 lines across 3 files
- **Benefit:** 69% reduction in code, fewer bugs, faster compilation

### 3. Better Performance
- **Before:** Separate caching, separate arbitration, duplicate calculations
- **After:** Unified caching, single arbitration pass, shared pathfinding
- **Benefit:** Faster movement requests, less memory usage

### 4. Improved API
- **Before:** Need to coordinate between 4 systems manually
- **After:** Single API handles coordination automatically
- **Benefit:** Less code to write, fewer errors

### 5. Better Debugging
- **Before:** Trace through 4 systems to find movement issues
- **After:** Single system with comprehensive diagnostics
- **Benefit:** Faster bug fixes, easier troubleshooting

---

## Deprecation Timeline

### Week 1 (Current)
- ✅ UnifiedMovementCoordinator integrated into BotAI
- ✅ Both systems coexist (compatibility mode)
- ⏳ Deprecation warnings added (Task 1.3)
- ⏳ Migration guide created (this document)

### Week 2
- Migrate primary systems (combat, strategy, class AI)
- Remove CombatMovementStrategy (consolidated into PositionModule)
- Remove MovementIntegration (consolidated into ArbiterModule)

### Week 3
- Migrate remaining files
- Remove GroupFormationManager (consolidated into FormationModule)
- **Remove MovementArbiter completely** (no longer needed)
- All code uses UnifiedMovementCoordinator

### Post-Migration
- Single movement system (UnifiedMovementCoordinator)
- ~5,253 lines of code removed
- 14 → 3 movement files
- 69% code reduction achieved

---

## Migration Status Tracking

Track your migration progress:

| File | Status | Migrated By | Date | Notes |
|------|--------|-------------|------|-------|
| `AI/BotAI.cpp` | ✅ COMPLETE | Claude | 2025-11-18 | Week 1 Task 1.4 |
| `Advanced/AdvancedBehaviorManager.cpp` | ⏳ PENDING | - | - | Week 1 Task 1.5 |
| `Dungeon/EncounterStrategy.cpp` | ⏳ PENDING | - | - | Week 1 Task 1.5 |
| `AI/Combat/RoleBasedCombatPositioning.cpp` | ⏳ PENDING | - | - | Week 1 Task 1.5 |
| `Dungeon/DungeonBehavior.cpp` | ⏳ PENDING | - | - | Week 1 Task 1.5 |
| `AI/Combat/CombatAIIntegrator.cpp` | ⏳ PENDING | - | - | Week 1 Task 1.5 |
| ... (remaining 200+ call sites) | ⏳ PENDING | - | - | Weeks 2-3 |

---

## Need Help?

### Questions?
- Check the API comparison table above
- Review common migration issues
- Read the UnifiedMovementCoordinator class documentation

### Found a Bug?
- Create a detailed bug report with:
  - File name and line number
  - What you expected to happen
  - What actually happened
  - Steps to reproduce

### Want to Contribute?
- Pick a file from the migration status table
- Follow the step-by-step checklist
- Test thoroughly
- Submit your changes
- Update the migration status table

---

## Summary

**Migration is simple for most files:**
1. Change `GetMovementArbiter()` → `GetUnifiedMovementCoordinator()`
2. Keep all method calls the same (API is backwards compatible)
3. Test your changes
4. Done!

**Advanced migrations** (formation, combat positioning):
- Use the new integrated APIs for better performance
- Refer to Pattern 3 and Pattern 4 above

**Timeline:**
- **Week 1:** High-traffic files + BotAI
- **Week 2:** Combat, strategy, class AI
- **Week 3:** Remaining files + cleanup

**Expected Result:**
- Single movement system
- 69% code reduction
- Better performance
- Easier maintenance

---

**Document Version:** 1.0
**Last Updated:** 2025-11-18
**Maintained By:** PlayerBot Phase 2 Migration Team

---

**END OF MIGRATION GUIDE**
